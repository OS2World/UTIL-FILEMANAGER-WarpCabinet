// DRunCmdDlg -- Class definition
#if !defined(_CREATDIR_H)
	#define _CREATDIR_H
#include "os2api.h"
#include "resource.h"
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
