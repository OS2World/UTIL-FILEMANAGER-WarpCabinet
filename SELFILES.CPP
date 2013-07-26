// DSelFilesDlg -- Class implementation
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
#include "selfiles.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DSelFilesDlg, TDialog)
	EV_CHILD_NOTIFY(IDC_FILE_MASK, EN_CHANGE, OnFileMaskBoxChange),
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
	EV_CHILD_NOTIFY(IDC_DESELECT, BN_CLICKED, CmDeselect),
END_RESPONSE_TABLE;

void DSelFilesDlg::SetupWindow()
{
	TDialog::SetupWindow();

	// If confirm all, then other checkboxes are disabled.
	SetDlgItemText(IDC_FILE_MASK, szOldFileMask);

	int cc = strlen(szOldFileMask);
	WinEnableWindow(GetDlgItem(DID_OK), cc);
}

void DSelFilesDlg::OnFileMaskBoxChange()
{
	// Enable or disable OK button based on whether file mask box is empty.
	int cc = WinQueryWindowTextLength(GetDlgItem(IDC_FILE_MASK));
	WinEnableWindow(GetDlgItem(DID_OK), cc);
	WinEnableWindow(GetDlgItem(IDC_DESELECT), cc);
}

void DSelFilesDlg::CmOk()
{
	// Retrieve new settings and write them back to parent's data.
	char szTemp[80];

	GetDlgItemText(IDC_FILE_MASK, szTemp, 80);
	strcpy(lpszOldFileMask, szTemp);

	CloseWindow(TRUE);
}

void DSelFilesDlg::CmDeselect()
{
	// Retrieve new settings and write them back to parent's data.
	char szTemp[80];

	GetDlgItemText(IDC_FILE_MASK, szTemp, 80);
	strcpy(lpszOldFileMask, szTemp);

	CloseWindow(-5);  // Return code for deselecting.
}



