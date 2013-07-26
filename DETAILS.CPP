// DFileDetailsDlg -- Class implementation
#include <assert.h>
#include <cstring.h>
#include <dir.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\dialog.h>
#include <owl\edit.h>
#include <owl\static.h>
#include <owl\button.h>
#include "os2api.h"
#include "resource.h"
#include "details.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DFileDetailsDlg, TDialog)
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
END_RESPONSE_TABLE;

void DFileDetailsDlg::SetupWindow()
{
	TDialog::SetupWindow();

	// If confirm all, then other checkboxes are disabled.
	CheckDlgButton(IDC_FILESIZE, bOldShowSize);
	CheckDlgButton(IDC_LAST_MODIFIED, bOldShowTimeStamp);
	CheckDlgButton(IDC_ATTRIBUTES, bOldShowSize);
}

void DFileDetailsDlg::CmOk()
{
	// Retrieve new settings and write them back to parent's data.
	*pbOldShowTimeStamp = IsDlgButtonChecked(IDC_LAST_MODIFIED);
	*pbOldShowAttr = IsDlgButtonChecked(IDC_ATTRIBUTES);
	*pbOldShowSize = IsDlgButtonChecked(IDC_FILESIZE);

	CloseWindow(TRUE);
}


