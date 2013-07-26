// DRunCmdDlg -- Class definition
#if !defined(_RUNCMD_H)
	#define _RUNCMD_H
#include "os2api.h"
#include "resource.h"
#include "filentry.h"
#include "filetool.h"

class DRunCmdDlg : public TDialog
{
public:
	LPSTR lpszCurrentDir;
	LPSTR lpszInitCmd;

	DRunCmdDlg(TWindow *parent, LPSTR lpszMyDir, LPSTR lpszMyInitCmd) : TDialog(parent, IDD_RUN_DLG, 0) {
			lpszCurrentDir = lpszMyDir;
			lpszInitCmd = lpszMyInitCmd;
		}

	void CmOk();

	void SetupWindow();

	void OnCmdLineBoxChange();

	DECLARE_RESPONSE_TABLE (DRunCmdDlg);
};

#endif
