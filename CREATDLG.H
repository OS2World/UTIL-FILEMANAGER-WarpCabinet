// DRunCmdDlg -- Class definition
#if !defined(_RUNCMD_H)
	#define _RUNCMD_H
#include "os2api.h"
#include "resource.h"
#include "fileasoc.h"
#include "filentry.h"

class DCreateDirDlg : public TDialog
{
public:
	LPSTR lpszCurrentDir;

	DCreateDirDlg(TWindow *parent, LPSTR lpszMyDir) : TDialog(parent, IDD_CREATE_DIR_DLG, 0) {
			lpszCurrentDir = lpszMyDir;
		}

	void CmOk();

	void SetupWindow();

	void OnNewDirBoxChange();

	DECLARE_RESPONSE_TABLE (DCreateDirDlg);
};

#endif
