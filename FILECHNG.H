// DFileChangeDlg -- Class definition
#if !defined(_FILECHNG_H)
	#define _FILECHNG_H
#include "os2api.h"
#include "resource.h"
#include "filentry.h"

extern int bConfirmCopy;
extern int bConfirmDelete;
extern int bConfirmAll;

class DFileChangeDlg : public TDialog
{
public:
	LPSTR lpszCurrentDir;
	int nCommand;

	DFileChangeDlg(TWindow *parent, LPSTR lpszMyDir, int nMyCommand) : TDialog(parent, IDD_FILE_CHANGE_DLG, 0) {
			lpszCurrentDir = lpszMyDir;
			nCommand = nMyCommand;
		}

	void CmOk();

	void SetupWindow();

	void OnTargetBoxChange();

	DECLARE_RESPONSE_TABLE (DFileChangeDlg);
};

// Global template-based array of file selections to change
extern TISArrayAsVector <FILEENTRY> SelectedFileList;
extern TISArrayAsVector <string> AllFilesList;

extern char szMask[80];
extern unsigned long ulShowAttrBits;
extern TTextGadget *pGlobalStatusBar;
extern TWindow *pGlobalParent;

extern ULONG ulTotalFilesBytes;

#endif
