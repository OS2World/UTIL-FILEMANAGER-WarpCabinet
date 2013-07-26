// DFileChangeDlg -- Class implementation
#include <assert.h>
#include <cstring.h>
#include <process.h>
#include <dir.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\dialog.h>
#include <owl\edit.h>
#include <owl\static.h>
#include <owl\button.h>
#include <owl\textgadg.h>
#include "resource.h"
#include "filentry.h"
#include "filetool.h"
#include "filechng.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DFileChangeDlg, TDialog)
	EV_CHILD_NOTIFY(IDC_TO, EN_CHANGE, OnTargetBoxChange),
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
END_RESPONSE_TABLE;

void DFileChangeDlg::SetupWindow()
{
	TDialog::SetupWindow();

	FILEENTRY *pFileEntry;

	string strCaption;

	SendDlgItemMsg(IDC_FROM, EM_SETTEXTLIMIT, 4096, 0);
	SendDlgItemMsg(IDC_TO, EM_SETTEXTLIMIT, 4096, 0);

	// Load up our text.
	switch(nCommand)
	{
		case CM_MOVE:  strCaption = "Move";
							WinEnableWindow(GetDlgItem(DID_OK), 0);
							break;

		case CM_COPY:  strCaption = "Copy";
							WinEnableWindow(GetDlgItem(DID_OK), 0);
							break;

		case CM_DELETE: strCaption = "Delete";
							{
								SetDlgItemText(IDC_FROM_PROMPT, "Delete:");

								WinShowWindow(GetDlgItem(IDC_TO_PROMPT), FALSE);
								WinShowWindow(GetDlgItem(IDC_TO), FALSE);
							}
							break;

		default: MessageBox("Bad command in DFileChangeDlg::SetupWindow()!", "Error", MB_ICONSTOP);
	}

	SetWindowText(strCaption.c_str());

	SetDlgItemText(IDC_CURRENT_DIR, lpszCurrentDir);

	string strSourceFiles("");

	// Build list of files
	for(int i = 0; i < SelectedFileList.GetItemsInContainer(); i++)
	{
		pFileEntry = SelectedFileList[i];

		if(pFileEntry)
		{
			if(strSourceFiles.length() > 0)
				strSourceFiles += ", ";
			strSourceFiles += pFileEntry->szFileName;
		}
	}
	SetDlgItemText(IDC_FROM, strSourceFiles.c_str());
}

void DFileChangeDlg::OnTargetBoxChange()
{
	// Enable or disable OK button based on whether target box is empty.
	int cc = WinQueryWindowTextLength(GetDlgItem(IDC_TO));

	WinEnableWindow(GetDlgItem(DID_OK), cc);
}

void DFileChangeDlg::CmOk()
{
	int rc;

	char szFullTarget[256];
	char szTargetDir[256];
	char szTargetFile[256];

	string strTestDir;

	GetDlgItemText(IDC_TO, szFullTarget, 256);

	int nLen = strlen(szFullTarget);

	// Special case #1 -- For entries not ending in '\' add one and
	// test if this is a directory.
	if(nLen > 0 && szFullTarget[nLen - 1] != '\\')
	{
		rc = chdir(szFullTarget);
		if(!rc)
			// This is a directory, so use it with globals...
			strcat(szFullTarget, "\\*.*");
	}

	// Special case #2, always append '*.*' to directories ending in '\'
	if(nLen > 0 && szFullTarget[nLen - 1] == '\\')
		strcat(szFullTarget, "*.*");

	string strTargetDir = szFullTarget;
	string strTemp;
	szTargetFile[0] = 0;

	//strTargetDir.to_upper();  // Case-sensitive for HPFS!

	string strAll("*.*");
	size_t pos = strTargetDir.find(strAll);

	if(pos != NPOS)
		strTargetDir = strTargetDir.substr(0, pos - 1);
	else
	{
		string strQuestion("?");
		string strGlobalStar("*");
		string strSlash("\\");
		string strColon(":");

		// Check to see if any '\' is provided...
		pos = strTargetDir.find_last_of(strSlash);

		if(pos == NPOS)
			// Could be a colon without a slash...
			pos = strTargetDir.find_last_of(strColon);
		if(pos != NPOS)		{
			// Then split directory and filename.
			strTemp = strTargetDir.substr(pos + 1);
			strcpy(szTargetFile, strTemp.c_str());
			strTemp = strTargetDir.substr(0, pos);
			strTargetDir = strTemp;
		}
		else
		{
			// Then it has to be just a file
			pos = strTargetDir.find_last_of(strSlash);
			strcpy(szTargetFile, strTargetDir.c_str());
			strTargetDir = lpszCurrentDir;  // Source and target dir's. are the same.
		}
	}

	SetCursor(0, IDC_WAIT);

	SetDlgItemText(IDC_STATUS, "Computing total size...");

	int nCount = CollectAllFilesInSelections(lpszCurrentDir, szMask, ulShowAttrBits, lpszCurrentDir);
	if(nCount == 0)
	{
		MessageBox("Select some files first!", "No Files Selected", MB_ICONEXCLAMATION);
		CloseWindow(TRUE);
		return;
	}

	SetCursor(0, IDC_ARROW);

	SetDlgItemText(IDC_STATUS, "");

	static FILEOPPARAM FileOpParam;
	FileOpParam.pStatus = NULL;
	// Use our dialog box's status control to display status info.
	FileOpParam.hwndStatus = GetDlgItem(IDC_STATUS);
	FileOpParam.pParent = pGlobalParent;
	FileOpParam.lpszSourceDir = lpszCurrentDir;
	FileOpParam.lpszTargetDir = (LPSTR)strTargetDir.c_str();
	FileOpParam.lpszTargetFile = szTargetFile;

	string strConfirm("Are you sure you want to ");

	string strPlural("");

	char szTotal[20];

	int bIsCopy = TRUE;

	itoa(nCount, szTotal, 10);

	if(nCount == 1)
		strPlural = "";
	else
		strPlural = "s";

	FileOpParam.nFileOp = nCommand;

	// Load up our text.
	switch(nCommand)
	{
		case CM_MOVE:  // Load files...
		case CM_COPY:
							// Confirmation options.
							if(bConfirmCopy || (nCommand == CM_MOVE && bConfirmDelete) || bConfirmAll)
							{
								bIsCopy = nCommand == CM_COPY ? TRUE : FALSE;
								if(bIsCopy)
									strConfirm += "copy ";
								else
									strConfirm += "move ";
								strConfirm += szTotal;
								strConfirm += " object";
								strConfirm += strPlural;
								strConfirm += " (";
								strConfirm += GetFileSizeAsStr(ulTotalFilesBytes);
								strConfirm += ") to ";
								strConfirm += strTargetDir;
								strConfirm += "?";
								rc = MessageBox(strConfirm.c_str(),
												 bIsCopy ? "Confirm Copy" : "Confirm Move",
												 MB_YESNOCANCEL);
								if(rc != IDYES)
								{
									CleanupArrays();
									CloseWindow(FALSE);
									return;
								}
							}
							// Confirm new directory, if this is one.
							// For drive letters only, we can't create anything new!!!
							if(strTargetDir[1] == ':' && strTargetDir.length() == 2)
							{
								strTestDir = strTargetDir + "\\";
								rc = chdir((LPSTR)strTestDir.c_str());
								if(rc)
								{
									strConfirm = "Invalid drive ";
									strConfirm += strTargetDir;
									MessageBox(strConfirm.c_str(), "Invalid Target Directory", MB_OK);
									CleanupArrays();
									CloseWindow(FALSE);
									return;
								}
							}
							else
							{
								// If drive letter provided, use target path unchanged.
								// Otherwise, use source path + target path.
								if(strTargetDir[1] != ':')
								{
									strTemp = lpszCurrentDir;
									strTemp += "\\";
									strTemp += strTargetDir;
									strTargetDir = strTemp;
								}
								// First, check that this directory doesn't already exist.  If not, confirm creating it.
								rc = chdir((LPSTR)strTargetDir.c_str());
								if(rc)
								{
									strConfirm = "Destination directory ";
									strConfirm += strTargetDir;
									strConfirm += " does not exist. Create?";
									rc = MessageBox(strConfirm.c_str(), "Confirm Directory Create", MB_YESNOCANCEL);
									if(rc != IDYES)
									{
										CleanupArrays();
										CloseWindow(FALSE);
										return;
									}
								}
							}
							//strTargetDir = strTestDir;
							FileOpParam.lpszTargetDir = (LPSTR)strTargetDir.c_str();

							rc = CopyOrMoveFiles(&FileOpParam);
							if(!rc)
							{
								if(bIsCopy)
									MessageBox(bIsCopy ? "Can't copy files!" : "Can't move files!",
												  "Copy Files Failed", MB_ICONSTOP);
								CleanupArrays();
								CloseWindow(FALSE);
							}
							else
							{
								CleanupArrays();
								CloseWindow(TRUE);
							}
							break;

		case CM_DELETE:
							if(bConfirmDelete || bConfirmAll)
							{
								strConfirm += "delete ";
								strConfirm += szTotal;
								strConfirm += " object";
								strConfirm += strPlural;
								strConfirm += " (";
								strConfirm += GetFileSizeAsStr(ulTotalFilesBytes);
								strConfirm += ") from ";
								strConfirm += strTargetDir;
								strConfirm += "?  This action can't be undone.";
								rc = MessageBox(strConfirm.c_str(), "Confirm Delete", MB_YESNOCANCEL);
								if(rc != IDYES)
								{
									CloseWindow(FALSE);
									return;
								}
							}
							rc = DeleteFiles(&FileOpParam);
							if(!rc)
							{
								MessageBox("Can't delete files!", "Delete Failed", MB_ICONSTOP);
								CleanupArrays();
								CloseWindow(FALSE);
								return;
							}
							else
							{
								CleanupArrays();
								CloseWindow(TRUE);
							}
							break;

		default: MessageBox("Bad command in DFileChangeDlg::SetupWindow()!", "Error", MB_ICONSTOP);
	}

	CloseWindow(TRUE);
}


