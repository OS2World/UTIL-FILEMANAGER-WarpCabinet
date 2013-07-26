// FILETOOL.CPP  -- Functions to manipulate files
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\applicat.h>
#include <owl\decframe.h>
#include <owl\textgadg.h>
#include <dir.h>
#include <dos.h>
#include <stdio.h>
#include <io.h>
#include <ctype.h>
#include <cstring.h>
#include <assert.h>
#include "os2api.h"
#include "resource.h"
#include "filentry.h"
#include "filetool.h"

int SafeDirectoryCreate(LPSTR lpszDir)
{
	// Create a directory safely.  If directory already exists, will return as well!
	// Assume Disk drive letter in first position. 'C:\GUEST' (and uppercase!)
	char szDir[257];
	char szNewDir[257];
	char szOldDir[257];

	LPSTR p;
	int rc;

	int nTokenCount = 0;

	strcpy(szDir, lpszDir);

	strcpy(szNewDir, "");

	// Append '\' if not provided for drive letter only (e.g., "C:")
	// This is needed to change directory to a drive letter.
	if(szDir[1] == ':' && strlen(szDir) == 2)
	{
		strcat(szDir, "\\");
	}

	rc = chdir(szDir);
	if(rc == 0)
	{
		chdir(szOldDir);
		return TRUE;
	}

	char chDisk = toupper(szDir[0]) - 'A' + 1;

	rc = getcurdir(chDisk, szOldDir);
	if(rc)
		// Can't create this directory since there is no current dir associated with this disk!
		return FALSE;

	p = strtok(szDir, "\\");
	while(TRUE)
	{
		if(p)
		{
			if(nTokenCount == 0)
			{
				strcat(szNewDir, p);
				strcat(szNewDir, "\\");
			}
			else if(nTokenCount == 1)
			{
				strcat(szNewDir, p);
			}
			else
			{
				strcat(szNewDir, "\\");
				strcat(szNewDir, p);
			}
			rc = chdir(szNewDir);
			if(rc) // Dir. may not exist, attempt to create it.
			{
				rc = mkdir(szNewDir);
				if(rc) // Dir. created failed.
				{
					chdir(szOldDir);
					return FALSE;
				}
			}
			p = strtok(NULL, "\\");
		}
		else
			break;
		nTokenCount++;
	}
	// Restore old directory
	chdir(szOldDir);
	return TRUE;
}

void CleanupArrays()
{
	AllFilesList.Flush(TShouldDelete::Delete);
	CriticalDirsList.Flush(TShouldDelete::Delete);
}

LPSTR GetFileSizeAsStr(ULONG ulFileSize)
{
	static string strOut;

	char szTemp[40];

	if(ulFileSize < 1000)
	{
		strOut = ltoa(ulFileSize, szTemp, 10);
		if(ulFileSize == 1)
			strOut += " byte";
		else
			strOut += " bytes";
	}
	else
	{
		ULONG ulKB = ulFileSize / 1000;
		strOut = ltoa(ulKB, szTemp, 10);
		strOut += " KB";
	}
	return (char *)strOut.c_str();
}

int CopyMoveFilePrimitive(TWindow *pParent, string& strDest, string& strSrc, int bIsCopy)
{
	ULONG ulSrcSize;
	char szSrcTimeStamp[40];

	ULONG ulDestSize;
	char szDestTimeStamp[40];

	string strMsg;

	int rc = DoesFileExist((LPSTR)strDest.c_str(), ulDestSize, szDestTimeStamp);
	if(rc)
	{
		rc = DoesFileExist((LPSTR)strSrc.c_str(), ulSrcSize, szSrcTimeStamp);
		if(!rc)
		{
			pParent->MessageBox("Unexpected error. Source file doesn't exist.", "Source File Not Found", MB_ICONEXCLAMATION);
			return FALSE;
		}

		if(bConfirmReplace || bConfirmAll)
		{
			strMsg = "Replace file: ";
			strMsg += strDest;
			strMsg += " (";
			strMsg += GetFileSizeAsStr(ulDestSize);
			strMsg += ", ";
			strMsg += szDestTimeStamp;
			strMsg += ")\n    With file: ";
			strMsg += strSrc;
			strMsg += " (";
			strMsg += GetFileSizeAsStr(ulSrcSize);
			strMsg += ", ";
			strMsg += szSrcTimeStamp;
			strMsg += ")?";

			rc = pParent->MessageBox((LPSTR)strMsg.c_str(), "Confirm Overwrite", MB_YESNOCANCEL);
			if(rc == IDCANCEL)
				return -5;  // Tell caller to abort!
			else if(rc == IDNO)
				return FALSE;
		}
	}

	if(bIsCopy)
	{
		// To here, copy source and target files.
		rc = DosCopy(strSrc.c_str(), strDest.c_str(), 5);  // DCPY_EXISTING | DCPY_FAILAS
		if(rc)
		{
			if(rc == 112)
				strMsg = "Error copying file (Out of disk space?): ";
			else
				strMsg = "Error copying file: ";
			strMsg += strSrc;
			strMsg += "\n to ";
			strMsg += strDest;
			strMsg += ".";
			pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Copy File", MB_ICONEXCLAMATION);
			return FALSE;
		}
	}
	else
	{
		// Then we're moving
		rc = DosCopy(strSrc.c_str(), strDest.c_str(), 5);  // DCPY_EXISTING | DCPY_FAILAS
		if(rc)
		{
			if(rc == 112)
				strMsg = "Error moving file (Out of disk space?): ";
			else if(rc == 250)
				strMsg = "Error moving file (Parent can't be moved to its own subdirectory!): ";
			else
				strMsg = "Error moving file: ";
			strMsg += strSrc;
			strMsg += "\n to ";
			strMsg += strDest;
			strMsg += ".";
			pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Move File", MB_ICONEXCLAMATION);
			return FALSE;
		}
		else
		{
			// We need to unlink after we copy.
			rc = _rtl_chmod(strSrc.c_str(), 1, FA_ARCH);
			if(!rc)
				rc = unlink((LPSTR)strSrc.c_str());
			if(rc)
			{
				strMsg = "Can't remove file on source: ";
				strMsg += strSrc;
				strMsg += ".  This file may be in use by another process.";
				pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Remove Source File", MB_ICONEXCLAMATION);
				return FALSE;
			}
		}
	}
	return TRUE;
}

int CopyOrMoveFiles(FILEOPPARAM *pParam)
{
	PFILEOPPARAM pFileOpParam = (PFILEOPPARAM)pParam;

	int nFileOp = pFileOpParam->nFileOp;
	TTextGadget *pStatus = pFileOpParam->pStatus;
	HWND hwndStatus = pFileOpParam->hwndStatus;
	TWindow *pParent = pFileOpParam->pParent;

	char szSourceDir[257];
	strcpy(szSourceDir, pFileOpParam->lpszSourceDir);

	char szTargetDir[257];
	strcpy(szTargetDir, pFileOpParam->lpszTargetDir);

	char szTargetFile[257];
	if(pFileOpParam->lpszTargetFile)
		strcpy(szTargetFile, pFileOpParam->lpszTargetFile);
	else
		szTargetFile[0] = 0;

	string strTarget;
	string strSource;
	string strMsg;

	int i;
	int rc;

	if(pStatus)
		pStatus->SetText("Creating subdirectories...");
	if(hwndStatus)
		WinSetWindowText(hwndStatus, "Creating subdirectories...");
	if(pParent)
		pParent->UpdateWindow();
	rc = SafeDirectoryCreate(szTargetDir);
	if(!rc)
	{
		strMsg = "Unable to create ";
		strMsg += szTargetDir;
		pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Create Directory", MB_ICONSTOP);
		CleanupArrays();
		return FALSE;
	}

	int nDirs = CriticalDirsList.GetItemsInContainer();
	if(nDirs > 0)
	{
		for(i = 0; i < nDirs; i++)
		{
			strTarget = szTargetDir;
			strTarget += *CriticalDirsList[i];
#ifdef _TESTING_
			pParent->MessageBox(strTarget.c_str(), "Creating Dir.", MB_OK);
#endif
			rc = SafeDirectoryCreate((LPSTR)strTarget.c_str());
			if(!rc)
			{
				strMsg = "Unable to create ";
				strMsg += strTarget.c_str();
				pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Create Directory", MB_ICONSTOP);
				CleanupArrays();
				return FALSE;
			}
		}
	}

	// Now copy or move files...
	string *pStr;

	int bIsCopy = nFileOp == CM_COPY ? TRUE : FALSE;

	int bUseFileMask = strlen(szTargetFile) > 0 ? TRUE : FALSE;

	string strTargetFileMask = szTargetFile;

	for(i = 0; i < AllFilesList.GetItemsInContainer(); i++)
	{
		strSource = szSourceDir;
		strSource += *AllFilesList[i];

		strTarget = szTargetDir;
		strTarget += *AllFilesList[i];

		string strErrMsg;
		string strNewTarget;

		if(bUseFileMask)
		{
			rc = MakeNewSourcePathWithWildcardMask(strNewTarget, strSource, strTargetFileMask, strErrMsg);
			if(!rc)
			{
				strMsg = "Can't make new target filename (Error = ";
				strMsg += strErrMsg;
				strMsg += ") Continue?";
				rc = pParent->MessageBox((LPSTR)strMsg.c_str(), "Bad Filename", MB_ICONEXCLAMATION | MB_YESNOCANCEL);
				if(rc != IDYES)
					break;
				else
					continue;
			}
			strTarget = strNewTarget;
		}

		if(nFileOp == CM_COPY)
			strMsg = "Copying to file ";
		else
			strMsg = "Moving file to ";
		strMsg += strTarget;
		strMsg += "...";
		if(pStatus)
			pStatus->SetText(strMsg.c_str());
		if(hwndStatus)
			WinSetWindowText(hwndStatus, (LPSTR)strMsg.c_str());
		if(pParent)
			pParent->UpdateWindow();
		rc = CopyMoveFilePrimitive(pParent, strTarget, strSource, bIsCopy);
		if(rc == -5)
			return FALSE;  // Stop processing, user has stopped on an error.
	}

	// Lastly, if we're moving, delete our "empty" directory tree.
	if(nFileOp == CM_MOVE)
	{
		int nDirs = CriticalDirsList.GetItemsInContainer();
		if(nDirs > 0)
		{
			if(pStatus)
				pStatus->SetText("Cleaning-up directories...");
			if(hwndStatus)
				WinSetWindowText(hwndStatus, "Cleaning-up directories...");
			if(pParent)
				pParent->UpdateWindow();
			// Ensure correct working directory.
			chdir(szSourceDir);
			for(i = nDirs - 1; i >= 0; i--)  // A trick, doing these backwards should clean out an empty directory.
			{
				strSource = szSourceDir;
				strSource += *CriticalDirsList[i];
#ifdef _TESTING_
				pParent->MessageBox(strSource.c_str(), "Deleting (Empty) Dir.", MB_OK);
#endif
				rc = rmdir((LPSTR)strSource.c_str());
#ifdef _TESTING_
				if(!rc)
				{
					strMsg = "Unable to remove ";
					strMsg += strSource.c_str();
					strMsg += " This directory might not be empty. Continue anyway?";
					rc = pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Remove Directory", MB_YESNOCANCEL);
					if(rc != IDYES)
					{
						CleanupArrays();
						return FALSE;
					}
				}
#endif
			}
		}
	}
	return TRUE;  // Success!
}

int DeleteFiles(FILEOPPARAM *pParam)
{
	PFILEOPPARAM pFileOpParam = (PFILEOPPARAM)pParam;

	int nFileOp = pFileOpParam->nFileOp;

	assert(nFileOp == CM_DELETE);

	TTextGadget *pStatus = (TTextGadget *)pFileOpParam->pStatus;
	HWND hwndStatus = pFileOpParam->hwndStatus;
	TWindow *pParent = pFileOpParam->pParent;
	char szSourceDir[257];
	strcpy(szSourceDir, pFileOpParam->lpszSourceDir);

	string strDeleteSource;
	string strMsg;

	int nRetVal;

	// Now copy or move files...
	string *pStr;

	int bSpecialFile = FALSE;
	int i;
	int rc;

	struct ffblk FileInfo;

	for(i = 0; i < AllFilesList.GetItemsInContainer(); i++)
	{
		strDeleteSource = szSourceDir;
		strDeleteSource += *AllFilesList[i];

		bSpecialFile = FALSE;

		strMsg = "Deleting file ";
		strMsg += strDeleteSource;
		strMsg += "...";
		if(pStatus)
			pStatus->SetText(strMsg.c_str());
		if(hwndStatus)
			WinSetWindowText(hwndStatus, (LPSTR)strMsg.c_str());
		if(pParent)
			pParent->UpdateWindow();
		rc = findfirst(strDeleteSource.c_str(), &FileInfo, 0);
		if(rc)
		{
			strMsg = "Can't find file ";
			strMsg += strDeleteSource;
			strMsg += ". Continue anyway?";
			rc = pParent->MessageBox(strMsg.c_str(), "File Not Found", MB_YESNOCANCEL);
			if(rc == IDCANCEL)
			{
				CleanupArrays();
				return FALSE;
			}
			else if(rc == IDYES)
			{
				// Skip to next file.
				continue;
			}
		}
		else // Check for system or hidden or read-only file
		{
			if((FileInfo.ff_attrib & FA_RDONLY) != 0 ||
				(FileInfo.ff_attrib & FA_SYSTEM) != 0 ||
				(FileInfo.ff_attrib & FA_HIDDEN) != 0)
			{
				bSpecialFile = TRUE;

				strMsg = "This is a hidden/read-only/or system file. Delete ";
				strMsg += strDeleteSource;
				strMsg += " anyway?";
				rc = pParent->MessageBox(strMsg.c_str(), "Confirm Delete", MB_YESNOCANCEL);
				if(rc == IDCANCEL)
				{
					CleanupArrays();
					return FALSE;
				}
				else if(rc == IDNO)
				{
					// Skip to next file.
					continue;
				}
			}
		}

		if(bSpecialFile)
		{
			// Change attrib. to normal, archive file.
			nRetVal = _rtl_chmod(strDeleteSource.c_str(), 1, FA_ARCH);
			rc = nRetVal == -1 ? FALSE : TRUE;
		}
		else
			rc = TRUE;

		if(rc)
		{
			nRetVal = unlink(strDeleteSource.c_str());
			rc = nRetVal ? FALSE : TRUE;
		}

		if(!rc)
		{
			strMsg = "Unable to delete file ";
			strMsg += strDeleteSource;
			strMsg += ". (It may be in-use by another application,) Continue?";
			rc = pParent->MessageBox(strMsg.c_str(), "Can't Delete", MB_YESNOCANCEL);
			if(rc == IDCANCEL)
			{
				CleanupArrays();
				return FALSE;
			}
			else if(rc == IDNO)
			{
				// Skip to next file.
				continue;
			}
		}
	}

	// Lastly, delete our "empty" directory tree.
	if(nFileOp == CM_MOVE || nFileOp == CM_DELETE)
	{
		int nDirs = CriticalDirsList.GetItemsInContainer();
		if(nDirs > 0)
		{
			if(pStatus)
				pStatus->SetText("Cleaning-up directories...");
			if(hwndStatus)
				WinSetWindowText(hwndStatus, "Cleaning-up directories...");
			if(pParent)
				pParent->UpdateWindow();
			// Ensure correct working directory.
			chdir(szSourceDir);
			for(i = nDirs - 1; i >= 0; i--)  // A trick, doing these backwards should clean out an empty directory.
			{
				strDeleteSource = szSourceDir;
				strDeleteSource += *CriticalDirsList[i];
#ifdef _TESTING_
				pParent->MessageBox(strDeleteSource.c_str(), "Deleting (Empty) Dir.", MB_OK);
#endif
				rc = DosDeleteDir(strDeleteSource.c_str());
#ifdef _TESTING_
				if(rc)
				{
					strMsg = "Unable to remove ";
					strMsg += strDeleteSource.c_str();
					strMsg += " This directory might not be empty. Continue anyway?";
					rc = pParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Remove Directory", MB_YESNOCANCEL);
					if(rc != IDYES)
					{
						CleanupArrays();
						return FALSE;
					}
				}
#endif
			}
		}
	}
	return TRUE;  // Success!
}

BOOL DoesFileExist(LPSTR lpszThisFile, ULONG& ulSize, LPSTR lpszTimeStamp)
{
	struct ffblk FileInfo;

	int rc = findfirst(lpszThisFile, &FileInfo, 0);

	if(rc)
		return FALSE;
	else
	{
		// Get old file so we can get the user to confirm over-writes...
		ulSize = FileInfo.ff_fsize;
		if(lpszTimeStamp)
			GetFileTimeStampAsString(lpszTimeStamp, FileInfo.ff_fdate, FileInfo.ff_ftime);
		return TRUE;
	}
}

LPSTR GetFileTimeStampAsString(LPSTR lpszDateTime, unsigned short int uFileDate, unsigned short int uFileTime)
{
	char szStamp[256];

	// Unpack file timestamp.
	int nSec = (uFileTime % 32) * 2;    // 0 - 4    Number of 2-second increments (0 - 29).
	int nMin = (uFileTime >> 5) % 64;   // 5 - 10 	Minutes (0 - 59)
	int nHour = uFileTime >> 11;        // 11 - 15 	Hours (0 - 23)

	int nDay = uFileDate % 32;				// 0 - 4    Day of month (1-31)
	int nMonth = (uFileDate >> 5) % 16; // 5 - 8    Month (1-12)
	int nYear = (uFileDate >> 9) + 80;  // 9 - 15   Year (relative to 1980)

	// Format.
	int bIsPM = FALSE;
	if(nHour >= 12)
	{
		bIsPM = TRUE;
		nHour -= 12;
		if(nHour == 0)
			nHour = 12;
	}
	wsprintf(szStamp, "%2d/%02d/%02d %2d:%02d:%02d %s\0", nMonth, nDay, nYear, nHour, nMin, nSec, bIsPM ? "PM" : "AM");

	strcpy(lpszDateTime, szStamp);

	return lpszDateTime;
}

int RemoveSourceDirFromPath(string& strResultPath, string& strRawPath, string& strSourceDir)
{
	int nRetVal = FALSE;
	string strCriticalDir("");
	// strRawPath contains full path--Find source dir inside this.
	// If found, copy only the 'critical' part of path
	size_t rc = strRawPath.find_first_of(strSourceDir);
	if(rc != NPOS)
	{
		// Then get 'critical' dir above the source dir.
		strCriticalDir = strRawPath.substr(rc + strSourceDir.length());
		if(strCriticalDir.length() > 0)
		{
			strResultPath = strCriticalDir;
			nRetVal = TRUE;
		}
	}
	return nRetVal;
}

int CollectAllFilesInSelections(LPSTR lpszThisDir, LPSTR lpszMask, ULONG ulAttrs, LPSTR lpszSourceDir)
{
	// Copies selected file list into all files array.  Walks through sub-dir trees too
	// using current mask and selected attributes.
	ulTotalFilesBytes = 0;

	AllFilesList.Flush(TShouldDelete::Delete);  // Make sure total files array is empty.
	CriticalDirsList.Flush(TShouldDelete::Delete);  // Make sure total dir. array is empty.

	FILEENTRY *pThisEntry = NULL;

	string strNewFile;

	string strFullDir = lpszThisDir;
	string strThisDir;

	for(int i = 0; i < SelectedFileList.GetItemsInContainer(); i++)
	{
		pThisEntry = SelectedFileList[i];

		if(!pThisEntry)
			continue;

		if(pThisEntry->nType == 1)  // If it's a directory, get all files in tree.
		{
			strThisDir = strFullDir;

			strThisDir = "\\";
			strThisDir += pThisEntry->szFileName;

			CriticalDirsList.Add(new string(strThisDir));

			strThisDir = strFullDir;
			strThisDir += "\\";
			strThisDir += pThisEntry->szFileName;

			LoadFilesInDir((char *)strThisDir.c_str(), lpszMask, ulAttrs, lpszSourceDir);
		}
		else
		{
			// Just add this file to complete list.
			ulTotalFilesBytes += (ULONG)pThisEntry->lFileSize;
			strNewFile = "\\";
			strNewFile += pThisEntry->szFileName;
			AllFilesList.Add(new string(strNewFile));
		}
	}
	return AllFilesList.GetItemsInContainer();
}


// Scan this directory for all files and sub-directories.
// This method can be called recursively.  Writes results to AllFilesList global array.
int LoadFilesInDir(LPSTR lpszThisDir, LPSTR lpszMask, ULONG ulAttrs, LPSTR lpszSourceDir)
{
	BOOL bFirstTime;

	struct ffblk FileInfo;

	int i;
	int rc;

	// Build search directory.
	string strSearchDir = lpszThisDir;
	strSearchDir += "\\*.*";

	string strSourceDir(lpszSourceDir);

	string strPath;
	string strCriticalDir;

	//string strFullPath = strSourceDir + strSearchDir;

	char szNewDir[257];

	unsigned long ulSearchBits = ulAttrs;

	TISArrayAsVector <string> DirList(4,0,1);  // Store our directories in this path
													// This must be local to this procedure so
													// we can recurse levels of sub-dirs!

	// Hunt down all directories and files that match selected attributes.
	bFirstTime = TRUE;
	while(1)
	{
		if(bFirstTime)
		{
			rc = findfirst(strSearchDir.c_str(), &FileInfo, FA_DIREC);
			bFirstTime = FALSE;
		}
		else
			rc = findnext(&FileInfo);
		if(rc)
			break;  // We're finished

		// Otherwise we might have a match.
		// Check to see if it's a directory and if so, make sure that
		// if its hidden, system or read-only, these bits are selected as well.
		if((FileInfo.ff_attrib & FA_DIREC) == FA_DIREC && (FileInfo.ff_name[0] != '.'))
		{
			if(ulSearchBits > 0)
				if((ulSearchBits & FileInfo.ff_attrib) == 0)
					continue;
			// To here, add this directory name to our array.
			strPath = lpszThisDir;
			strPath += "\\";
			strPath += FileInfo.ff_name;

			// strPath contains full path--Find source dir inside this.
			// If found, copy only the 'critical' part of path
			rc = RemoveSourceDirFromPath(strCriticalDir, strPath, strSourceDir);
			if(rc && strCriticalDir.length() > 0)
			{
				CriticalDirsList.Add(new string(strCriticalDir));
			}
#ifdef _TESTING_
			else
				assert(0);
#endif
			DirList.Add(new string(strPath));
		}
	}

	// Now get files in the current directory.  These are collected in global!!!
	bFirstTime = TRUE;

	strSearchDir = lpszThisDir;
	strSearchDir += "\\";
	strSearchDir += lpszMask;

	while(1)
	{
		if(bFirstTime)
		{
			rc = findfirst(strSearchDir.c_str(), &FileInfo, 0);
			bFirstTime = FALSE;
		}
		else
			rc = findnext(&FileInfo);
		if(rc)
			break;  // We're finished

		// Otherwise we might have a match.
		// Check to see if it's a directory.  If not, make sure that
		// if its hidden, system or read-only, these bits are selected as well.
		if((FileInfo.ff_attrib & FA_DIREC) == FA_DIREC)
			continue;
		if(ulSearchBits > 0)
			if((ulSearchBits & FileInfo.ff_attrib) == 0)
				continue;
		// To here, add this file information to our array.
		// This will be automatically sorted!
		strPath = lpszThisDir;
		strPath += "\\";
		strPath += FileInfo.ff_name;

		ulTotalFilesBytes += (ULONG)FileInfo.ff_fsize;

		rc = RemoveSourceDirFromPath(strPath, strPath, strSourceDir);
		if(rc)
		{
#ifdef _TESTING_
			pGlobalParent->MessageBox((LPSTR)strPath.c_str(), "Adding File:", MB_OK);
#endif
			AllFilesList.Add(new string(strPath));
		}
#ifdef _TESTING_
		else
			assert(0);
#endif
	}

	// Now the tricky part.
	// Walk through directory array and process each entry with this same function.
	string *pThisDir = NULL;

	for(i = 0; i < DirList.GetItemsInContainer(); i++)
	{
		pThisDir = DirList[i];
		if(pThisDir)
		{
			// Build new path for receiver.
			strcpy(szNewDir, pThisDir->c_str());

			// Recursion -- make sure our stack is large enough.
			rc = LoadFilesInDir(szNewDir, lpszMask, ulAttrs, lpszSourceDir);
		}
	}

	// Clean-up our linked list.
	DirList.Flush(TShouldDelete::Delete);

	return TRUE;
}

int MakeNewSourcePathWithWildcardMask(string& strNewFullSourcePath, string& strOldFullSourcePath,
												  string& strWildcardMask, string& strErrMsg)
{
	strErrMsg = "";

	string strRawFileOnly("");
	string strRawDirOnly("");
	string strNewFile("");

	strNewFullSourcePath = "";

	string strSlash("\\");

	size_t p;

	p = strOldFullSourcePath.find_last_of(strSlash);
	if(p != NPOS)
	{
		strRawFileOnly = strOldFullSourcePath.substr(p + 1);
		strRawDirOnly = strOldFullSourcePath.substr(0, p + 1);
	}
	else
	{
		strErrMsg = "Missing full directory in source.";
		return FALSE;
	}

	// Now meld wildcard in mask with file to get new filename.
	int bPeriodFound = FALSE;

	string chMask("");
	string chSource("");

	string strPeriod(".");

	int bNoPeriodInSource = FALSE;

	int nSourceMarker = 0;

	try
	{

		for(int i = 0; i < strWildcardMask.length() && (nSourceMarker < strRawFileOnly.length() || bNoPeriodInSource); i++)
		{
			chMask = strWildcardMask.substr(i, 1);

			if(chMask == "?")
			{
				if(bNoPeriodInSource)
					break;
				chSource = strRawFileOnly.substr(nSourceMarker, 1);
				strNewFile += chSource;
				nSourceMarker++;
			}
			else if(chMask == "*")
			{
				if(bNoPeriodInSource)
					break;
				if(bPeriodFound)
				{
					// Then take rest of source file.
					strNewFile += strRawFileOnly.substr(nSourceMarker);
					break;  // We must be done.
				}
				else  // Then take source file to period or rest of string if no period is found.
				{
					p = strRawFileOnly.find_first_of(strPeriod);
					if(p == NPOS)
					{
						strNewFile += strRawFileOnly.substr(nSourceMarker);
						nSourceMarker = p;
						bNoPeriodInSource = TRUE;
					}
					else
					{
						strNewFile += strRawFileOnly.substr(nSourceMarker, p - nSourceMarker);
						p = strWildcardMask.find_first_of(strPeriod);
						if(p != NPOS)
							i = p - 1;
						else
							break;  // No period means there nothing else to add
					}
				}
			}
			else if(chMask == ".")
			{
				if(bPeriodFound)
				{
					strErrMsg = "Invalid target filename";
					return FALSE;
				}
				else
				{
					strNewFile += ".";
					// Move past "." in source
					p = strRawFileOnly.find_first_of(strPeriod);
					if(p != NPOS)
						nSourceMarker = p + 1;
					else
						bNoPeriodInSource = TRUE;
					bPeriodFound = TRUE;
				}
			}
			else
			{
				// Write in mask into target file.
				strNewFile += chMask;
				nSourceMarker++;
			}
		}
	}
	catch(...)
	{
		strErrMsg = "Bad mask or source file.";
		strNewFullSourcePath = "";
		return FALSE;
	}
	strNewFullSourcePath = strRawDirOnly + strNewFile;
	return TRUE;
}

int ChangeFileAttributes(LPSTR lpszCurrentDir, ULONG ulNewAttrib)
{
	// Change attributes for this list of files.
	FILEENTRY *pFileEntry;

	string strMsg;

	int bFilePropSet;

	if(pGlobalParent)
		pGlobalParent->SetCursor(0, IDC_WAIT);

	if(pGlobalStatusBar)
		pGlobalStatusBar->SetText("Changing attributes...");

	int rc;

	for(int i = 0; i < SelectedFileList.GetItemsInContainer(); i++)
	{
		string szThisFile;

		bFilePropSet = FALSE;
		pFileEntry = (FILEENTRY *)SelectedFileList[i];
		if(pFileEntry)
		{
			szThisFile = lpszCurrentDir;
			szThisFile += "\\";
			szThisFile += pFileEntry->szFileName;
			rc = _rtl_chmod((LPSTR)szThisFile.c_str(), 1, ulNewAttrib);
			if(!rc)
				bFilePropSet = TRUE;
		}

		if(!bFilePropSet)
		{
			if(!pGlobalParent)
				break;
			strMsg = "Can't set attributes for file: ";
			strMsg += pFileEntry->szFileName;
			strMsg += ". Continue anyway?";
			rc = pGlobalParent->MessageBox((LPSTR)strMsg.c_str(), "Can't Set Properties", MB_YESNOCANCEL | MB_ICONEXCLAMATION);
			if(rc != IDYES)
				break;
			else
				if(pGlobalParent)
					pGlobalParent->SetCursor(0, IDC_WAIT);

		}
	}
	if(pGlobalStatusBar)
		pGlobalStatusBar->SetText("");

	if(pGlobalParent)
		pGlobalParent->SetCursor(0, IDC_ARROW);

	return TRUE;
}

BOOL RunProgramWithArgs(LPSTR lpszExeName, LPSTR lpszArgs)
{
	STARTDATA   StartData;
	ULONG       SessID;
	PID         PID;
	UCHAR       PgmName[256]; // Program pathname string
	UCHAR       PgmArgs[256]; // Arguments
	UCHAR       ObjBuf[100];
	int rc;

	//  Specify the various session start parameters
	StartData.Length = sizeof(STARTDATA);
	StartData.Related =  SSF_RELATED_INDEPENDENT;  // New session.
	StartData.FgBg = SSF_FGBG_FORE;  					// Run in foreground.
	StartData.TraceOpt = SSF_TRACEOPT_NONE;

	StartData.PgmTitle = 0;

	strcpy(PgmName, lpszExeName);
	StartData.PgmName = PgmName;

	if(lpszArgs)
	{
		strcpy(PgmArgs, lpszArgs); // Arguments
		StartData.PgmInputs = PgmArgs;
	}
	else
		StartData.PgmInputs = 0;

	StartData.TermQ = 0;
	StartData.Environment = 0;
	StartData.InheritOpt = 0;

	StartData.SessionType = SSF_TYPE_DEFAULT;

	StartData.IconFile = 0;
	StartData.PgmHandle = 0;

	StartData.PgmControl = SSF_CONTROL_VISIBLE;

	StartData.InitXPos = 0;
	StartData.InitYPos = 0;
	StartData.InitXSize = 0;
	StartData.InitYSize = 0;

	StartData.Reserved = 0;

	StartData.ObjectBuffer = ObjBuf;
	StartData.ObjectBuffLen = 100;

	rc = DosStartSession(&StartData, &SessID, &PID);
	if (rc != 0)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}



