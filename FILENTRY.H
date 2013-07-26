// FILENTRY.H -- Simple class to manage files in listboxes.
#if !defined(_FILENTRY_H)
	#define _FILENTRY_H

const int SORT_BY_NAME = 0;
const int SORT_BY_DATE = 1;
const int SORT_BY_EXT = 2;
const int SORT_BY_SIZE = 3;

class FILEENTRY
{
public:
	static int nSortMethod;  // Change this member to change the sort method used to store file entries.

	long lFileSize;
	unsigned long  ulFileAttrib;
	unsigned short uFileTime;
	unsigned short uFileDate;
	char szFileName[256];
	char szExt[4];  // File Extension
	int nType;
	int iImage;

	FILEENTRY(struct ffblk& FileInfo, BOOL bIsDir, int nMySortMethod = SORT_BY_NAME);

	// In order to use this class with a template-based container,
	// we need to define < and == operators.  (Note this isn't a full-fledged class.)
	int operator<(const FILEENTRY& CompareItem);

	int operator==(const FILEENTRY& CompareItem);

	void UpperCase(char *pszOut, char *pszIn, int nMaxLen);
};

#endif