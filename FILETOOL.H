// FILETOOL.H  -- Functions to manipulate files
#if !defined(_FILETOOL_H)
	#define _FILETOOL_H
#include <owl\statusba.h>

// List of files in global template-based array.
extern TISArrayAsVector <FILEENTRY> SelectedFileList;
extern TISArrayAsVector <string> AllFilesList;
// Dirs. that must be created (for copying, moving or renaming) or deleted for deleting.
extern TISArrayAsVector <string> CriticalDirsList;

extern ULONG ulTotalFilesBytes;

extern TTextGadget *pGlobalStatusBar;
extern TWindow *pGlobalParent;

// Confirmation options -- In WARPCAB.CPP
extern int bConfirmReplace;
extern int bConfirmAll;

typedef struct _FILEOPPARAM
{
	int nFileOp;
	TTextGadget *pStatus;
	HWND hwndStatus;
	TWindow *pParent;
	LPSTR lpszSourceDir;
	LPSTR lpszTargetDir;
	LPSTR lpszTargetFile;
} FILEOPPARAM;

typedef FILEOPPARAM * PFILEOPPARAM;
int CopyMoveFilePrimitive(TWindow *pParent, string& strDest, string& strSrc, int bIsCopy);
int CopyOrMoveFiles(FILEOPPARAM *pParam);
int DeleteFiles(FILEOPPARAM *pParam);
LPSTR GetFileTimeStampAsString(LPSTR lpszDateTime, unsigned short int uFileDate, unsigned short int uFileTime);
int LoadFilesInDir(LPSTR lpszThisDir, LPSTR lpszMask, ULONG ulAttrs, LPSTR lpszSourceDir);
int CollectAllFilesInSelections(LPSTR lpszThisDir, LPSTR lpszMask, ULONG ulAttrs, LPSTR lpszSourceDir);
BOOL DoesFileExist(LPSTR lpszThisFile, ULONG& ulSize, LPSTR lpszTimeStamp);
int RemoveSourceDirFromPath(string& strResultPath, string& strRawPath, string& strSourceDir);
void CleanupArrays();
int SafeDirectoryCreate(LPSTR lpszDir);
LPSTR GetFileSizeAsStr(ULONG ulFileSize);
int MakeNewSourcePathWithWildcardMask(string& strNewFullSourcePath, string& strOldFullSourcePath,
												  string& strWildcardMask, string& strErrMsg);
int ChangeFileAttributes(LPSTR lpszCurrentDir, ULONG ulNewAttrib);
BOOL RunProgramWithArgs(LPSTR lpszExeName, LPSTR lpszArgs);
#endif
