// DFilePropertiesDlg -- Class definition
#if !defined(_FILEPROP_H)
	#define _FILEPROP_H
#include "os2api.h"
#include "resource.h"
#include "fileasoc.h"
#include "filentry.h"

class DFilePropertiesDlg : public TDialog
{
public:
	LPSTR lpszCurrentDir;

	DFilePropertiesDlg(TWindow *parent, LPSTR lpszMyDir) : TDialog(parent, IDD_FILE_PROP_DLG, 0) {
			lpszCurrentDir = lpszMyDir;
		}

	void CmOk();

	void SetupWindow();

	DECLARE_RESPONSE_TABLE (DFilePropertiesDlg);
};

// Global template-based array of file selections to change
extern TISArrayAsVector <FILEENTRY> SelectedFileList;

#endif
