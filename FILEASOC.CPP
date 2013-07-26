// FILEASOC.CPP -- Class implementation to manage file types and associations.
#define INCL_WINSHELLDATA
#include <os2.h>
#include <owl\owlpch.h>
#include <owl\applicat.h>
#include <cstring.h>
#include "resource.h"
#include "fileasoc.h"

int FindFileAssociation(const FILEASSOCIATION& ThisRec, void *pMatchExt)
{
	LPSTR lpszMatchExt = (LPSTR) pMatchExt;

	if(strcmpi(ThisRec.szExt, lpszMatchExt) == 0)
		return TRUE;
	else
		return FALSE;
}

FILEASSOCIATION::FILEASSOCIATION(LPSTR lpszMyFileExt, LPSTR lpszMyDesc, LPSTR lpszMyEXEPath, int nMyImage)
{
	if(!lpszMyFileExt)
		strcpy(szExt,"");
	else
		strcpy(szExt, lpszMyFileExt);

	if(!lpszMyDesc)
		strcpy(szDesc, "N.A.");
	else
		strcpy(szDesc, lpszMyDesc);

	if(!lpszMyEXEPath)
		strcpy(szEXEPathName, "");
	else
		strcpy(szEXEPathName, lpszMyEXEPath);

	iImage = nMyImage;
}

FILEASSOCIATION::FILEASSOCIATION(const FILEASSOCIATION& OldAssoc)
{
	// Copy data from source object.
	strcpy(szExt, OldAssoc.szExt);
	strcpy(szDesc, OldAssoc.szDesc);
	strcpy(szEXEPathName, OldAssoc.szEXEPathName);
	iImage = OldAssoc.iImage;   // Image used to represent this file type in listbox.
}

BOOL FILEASSOCIATION::SaveToProfileFile(HWND hwnd, LPSTR lpszPRFFilename, LPSTR lpszSection, int nIndex)
{
	// OS/2 Version uses profile functions.
	string strKeyword;

	strKeyword = "Type";

	// E.g. Type1 - Type20, etc.
	char szTemp[20];
	itoa(nIndex, szTemp, 10);
	strKeyword += szTemp;

	string strData;

	// Save this record as a series of comma-delimited fields.
	strData = szExt;
	strData += ",";
	strData += szDesc;
	strData += ",";
	strData += szEXEPathName;
	strData += ",";

	itoa(iImage, szTemp, 10);
	strData += szTemp;

	HINI hini = PrfOpenProfile(WinQueryAnchorBlock(hwnd), lpszPRFFilename);

	if(hini == NULLHANDLE)
		return FALSE;

	int rc = PrfWriteProfileString(hini, lpszSection, strKeyword.c_str(), strData.c_str());

	// (For Windows!) int rc = WritePrivateProfileString(lpszSection, strKeyword.c_str(), strData.c_str(), lpszPRFFilename);

	PrfCloseProfile(hini);

	return rc;
}

BOOL FILEASSOCIATION::LoadFromProfileFile(HWND hwnd, LPSTR lpszPRFFilename, LPSTR lpszSection, int nIndex)
{
	char szData[1024];

	// NULL out our data...
	strcpy(szExt,"");
	strcpy(szDesc, "");
	strcpy(szEXEPathName, "");
	iImage = ID_DOCUMENT;   // Image used to represent this file type in listbox.

	string strKeyword;

	strKeyword = "Type";

	// I.e., Type1 - Type20, etc.
	char szTemp[20];
	itoa(nIndex, szTemp, 10);
	strKeyword += szTemp;

	HINI hini = PrfOpenProfile(WinQueryAnchorBlock(hwnd), lpszPRFFilename);

	if(hini == NULLHANDLE)
		return FALSE;

	ULONG ulBytes = PrfQueryProfileString(hini,
								lpszSection, strKeyword.c_str(), "",
								szData, 1024);

	// (For Windows) GetPrivateProfileString(lpszSection, strKeyword.c_str(), "", szData, 1024, lpszPRFFilename);

	PrfCloseProfile(hini);

	if(ulBytes == 0 || strlen(szData)==0)
		return FALSE;

	// Otherwise, parse out our fields.
	LPSTR p;
	p = strtok(szData, ",");

	// File extension
	if(p)
		strcpy(szExt, p);
	else
		return FALSE;

	p = strtok(NULL, ",");

	// File description
	if(p)
		strcpy(szDesc, p);
	else
		return FALSE;

	// .EXE path used for this document
	p = strtok(NULL, ",");
	if(p)
		strcpy(szEXEPathName, p);
	else
		return FALSE;

	// Image index used for this document
	p = strtok(NULL, ",");
	if(p)
		iImage = atoi(p);
	else
		iImage = ID_DOCUMENT;

	return TRUE;
}


