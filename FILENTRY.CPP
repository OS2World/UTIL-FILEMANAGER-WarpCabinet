// FILENTRY.CPP -- Simple class to manage files in listboxes (implementation).
#define INCL_WINSHELLDATA
#include <os2.h>
#include <owl\owlpch.h>
#include <owl\applicat.h>
#include <cstring.h>
#include "resource.h"
#include "filentry.h"

int FILEENTRY::nSortMethod = SORT_BY_NAME;

FILEENTRY::FILEENTRY(struct ffblk& FileInfo, BOOL bIsDir, int nMySortMethod)
{
	int nFound;

	// Get entry from the contents of this struct ffblk.
	nSortMethod = nMySortMethod;  // Note this is a static member. One value per all objects of this class!
											// Change this and we change the way our entries are sorted.

	lFileSize = FileInfo.ff_fsize;
	ulFileAttrib = FileInfo.ff_attrib;
	uFileTime = FileInfo.ff_ftime;
	uFileDate = FileInfo.ff_fdate;

	strcpy(szFileName, FileInfo.ff_name);

	for(LPSTR p = szFileName; *p != '\0' && *p != '.'; p++);

	if(*p == '.')
		strcpy(szExt, p + 1);  // Save extension.
	else
		strcpy(szExt, "");  // (No extension.)

	if(bIsDir)
	{
		nType = 1;
		iImage = ID_FOLDER;
	}
	else
	{
		if(strcmpi(szExt, "EXE") == 0)
		{
			nType = 2;
			iImage = ID_EXE;
		}
		else if(strcmpi(szExt, "CMD") == 0)
		{
			nType = 3;
			iImage = ID_CMD;
		}
		else if(strcmpi(szExt, "BAT") == 0)
		{
			nType = 4;
			iImage = ID_BAT;
		}
		else if(strcmpi(szExt, "COM") == 0)
		{
			nType = 5;
			iImage = ID_COM;
		}
		else
		{
			// Then it's some kind of document...
			nType = 6;
			// Find association, if any.
			iImage = ID_DOCUMENT;
		}
	}
}

void FILEENTRY::UpperCase(char *pszOut, char *pszIn, int nMaxLen)
{
	if(!pszOut || !pszIn)
		return;

	for(int i = 0; i < strlen(pszIn) && i < nMaxLen; i++)
		pszOut[i] = toupper(pszIn[i]);

	if(i < nMaxLen)
		pszOut[i] = '\0';
}

int FILEENTRY::operator<(const FILEENTRY& CompareItem)
{
	int bIsLessThan = FALSE;
	int rc;

	char szTemp1[257];
	char szTemp2[257];

	// Folders always appear before files!!!
	if(nType == 1 && CompareItem.nType != 1)
		return TRUE;
	else if(nType != 1 && CompareItem.nType == 1)
		return FALSE;

	// Otherwise, sort within folders or files
	switch(nSortMethod)
	{
		case SORT_BY_NAME:
				UpperCase(szTemp1, szFileName, 256);
				UpperCase(szTemp2, (char *)CompareItem.szFileName, 256);
				rc = strcmpi(szTemp1, szTemp2);
				if(rc < 0)
					bIsLessThan = TRUE;
				break;
		case SORT_BY_DATE:
				if(uFileDate == CompareItem.uFileDate)
					// Latest files first!
					bIsLessThan = uFileTime > CompareItem.uFileTime;
				else
					bIsLessThan = uFileDate > CompareItem.uFileDate;
				break;
		case SORT_BY_EXT:
				rc = strcmpi(szExt, CompareItem.szExt);
				if(rc < 0)
					bIsLessThan = TRUE;
				break;
		case SORT_BY_SIZE:
				// Largest files first!
				bIsLessThan = lFileSize > CompareItem.lFileSize;
				break;
	}
	return bIsLessThan;
}

int FILEENTRY::operator==(const FILEENTRY& CompareItem)
{
	// Assumes equivalent filenames are equivalent objects.
	if(nType == CompareItem.nType && strcmpi(szFileName, CompareItem.szFileName) == 0)
		return TRUE;
	else
		return FALSE;
}


