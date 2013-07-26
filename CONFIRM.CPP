// DConfirmDlg -- Class implementation
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
#include "confirm.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DConfirmDlg, TDialog)
	EV_CHILD_NOTIFY(IDC_CONFIRM_ALL, BN_CLICKED, OnAllBoxClick),
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
END_RESPONSE_TABLE;


DConfirmDlg::DConfirmDlg(TWindow *parent, int *pbConfirmDelete,
						int *pbConfirmReplace, int *pbConfirmCopy,
						int *pbConfirmAll) : TDialog(parent, IDD_SET_CONFIRM_OPTS_DLG, 0)
{
	bOldConfirmDelete = FALSE;
	bOldConfirmReplace = FALSE;
	bOldConfirmCopy = FALSE;
	bOldConfirmAll = FALSE;

	if(!pbConfirmDelete ||
		!pbConfirmDelete ||
		!pbConfirmCopy ||
		!pbConfirmAll)
	{
		MessageBox("Bad pointer in DConfirmDlg::DConfirmDlg!", "Error", MB_ICONSTOP);
		return;
	}

	pbOldConfirmDelete = pbConfirmDelete;
	pbOldConfirmReplace = pbConfirmReplace;
	pbOldConfirmCopy = pbConfirmCopy;
	pbOldConfirmAll = pbConfirmAll;

	bOldConfirmDelete = *pbOldConfirmDelete;
	bOldConfirmReplace = *pbOldConfirmReplace;
	bOldConfirmCopy = *pbOldConfirmCopy;
	bOldConfirmAll = *pbOldConfirmAll;
}

void DConfirmDlg::SetupWindow()
{
	TDialog::SetupWindow();

	// If confirm all, then other checkboxes are disabled.

	if(bOldConfirmAll)
	{
		WinEnableWindow(GetDlgItem(IDC_CONFIRM_DELETE), 0);
		WinEnableWindow(GetDlgItem(IDC_CONFIRM_REPLACE), 0);
		WinEnableWindow(GetDlgItem(IDC_CONFIRM_COPY), 0);

		CheckDlgButton(IDC_CONFIRM_ALL, TRUE);
	}
	else
	{
		CheckDlgButton(IDC_CONFIRM_DELETE, bOldConfirmDelete);
		CheckDlgButton(IDC_CONFIRM_COPY, bOldConfirmCopy);
		CheckDlgButton(IDC_CONFIRM_REPLACE, bOldConfirmReplace);
	}
}

void DConfirmDlg::OnAllBoxClick()
{
	// Check to see if this is turned off/on enable or disable other 3 options
	// accordingly.
	int rc = IsDlgButtonChecked(IDC_CONFIRM_ALL);
	{
		WinEnableWindow(GetDlgItem(IDC_CONFIRM_DELETE), !rc);
		WinEnableWindow(GetDlgItem(IDC_CONFIRM_REPLACE), !rc);
		WinEnableWindow(GetDlgItem(IDC_CONFIRM_COPY), !rc);
	}

}

void DConfirmDlg::CmOk()
{
	// Retrieve new settings and write them back to parent's data.
	if(IsDlgButtonChecked(IDC_CONFIRM_ALL))
	{
		*pbOldConfirmDelete = FALSE;
		*pbOldConfirmCopy = FALSE;
		*pbOldConfirmReplace = FALSE;

		*pbOldConfirmAll = TRUE;
	}
	else
	{
		*pbOldConfirmAll = FALSE;

		*pbOldConfirmDelete = IsDlgButtonChecked(IDC_CONFIRM_DELETE);
		*pbOldConfirmCopy = IsDlgButtonChecked(IDC_CONFIRM_COPY);
		*pbOldConfirmReplace = IsDlgButtonChecked(IDC_CONFIRM_REPLACE);
	}
	CloseWindow(TRUE);
}


