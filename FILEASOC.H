// FILEASOC.H -- Class for to manage file types and associations
#if !defined(_FILEASOC_H)
#define _FILEASOC_H

class FILEASSOCIATION
{
public:
	char szExt[4];
	char szDesc[128];
	char szEXEPathName[256];
	int iImage;   // Image used to represent this file type in listbox.

	// Constructor
	FILEASSOCIATION(LPSTR lpszMyFileExt = NULL, LPSTR lpszMyDesc = NULL, LPSTR lpszMyEXEPath = NULL, int nMyImage = ID_DOCUMENT);

	// Copy constructor
	FILEASSOCIATION::FILEASSOCIATION(const FILEASSOCIATION& OldAssoc);

	BOOL SaveToProfileFile(HWND hwnd, LPSTR lpszPRFFilename, LPSTR lpszSection, int nIndex);

	BOOL LoadFromProfileFile(HWND hwnd, LPSTR lpszPRFFilename, LPSTR lpszSection, int nIndex);

	const FILEASSOCIATION operator=(const FILEASSOCIATION& OldAssoc)
	{
		strcpy(szExt, OldAssoc.szExt);
		strcpy(szDesc, OldAssoc.szDesc);
		strcpy(szEXEPathName, OldAssoc.szEXEPathName);
		iImage = OldAssoc.iImage;

		return *this;
	}

	int operator<(const FILEASSOCIATION& CompareItem) { int rc = strcmpi(szExt, CompareItem.szExt);
																 if(rc < 0) return TRUE; else return FALSE; }

	int operator==(const FILEASSOCIATION& CompareItem) { if(strcmpi(szExt, CompareItem.szExt) == 0)
																  return TRUE; else return FALSE; }
};

// Search function for use with TISArrayAsVector::FirstThat(), TISArrayAsVector::LastThat().
int FindFileAssociation(const FILEASSOCIATION& ThisRec, void *pMatchExt);

#endif
