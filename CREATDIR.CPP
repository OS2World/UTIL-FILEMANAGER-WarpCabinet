// DCreateDirDlg -- Class implementation
#include <assert.h>
#include <cstring.h>
#include <dir.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\dialog.h>
#include <owl\edit.h>
#include <owl\static.h>
#include <owl\button.h>
#include "resource.h"
#include "filentry.h"
#include "filetool.h"
#include "creatdir.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DCreateDirDlg, TDialog)
	EV_CHILD_NOTIFY(IDC_NEW_DIR, EN_CHANGE, OnNewDirBoxChange),
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
END_RESPONSE_TABLE;

void DCreateDirDlg::SetupWindow()
{
	TDialog::SetupWindow();

	SendDlgItemMsg(IDC_NEW_DIR, EM_SETTEXTLIMIT, 257, 0);
	SetDlgItemText(IDC_CURRENT_DIR, lpszCurrentDir);

	WinEnableWindow(GetDlgItem(DID_OK), 0);
}

void DCreateDirDlg::OnNewDirBoxChange()
{
	// Enable or disable OK button based on whether target box is empty.
	int cc = WinQueryWindowTextLength(GetDlgItem(IDC_NEW_DIR));
	WinEnableWindow(GetDlgItem(DID_OK), cc);
}

void DCreateDirDlg::CmOk()
{
	int rc;

	char szNewDir[257];

	GetDlgItemText(IDC_NEW_DIR, szNewDir, 257);

	string strTemp = szNewDir;
	// strTemp.to_upper();  HPFS dirs. are case sensitive!
	string strNewDir;

	if(strTemp.length() > 0 && strTemp[1] != ':')
	{
		// If 'Drive' ':' provided assume this is a complete path.
		// Otherwise attach working directory to this one.
		strNewDir = lpszCurrentDir;
		strNewDir += "\\";
		strNewDir += strTemp;
	}
	else
	{
		strNewDir = strTemp;
	}

	// No final '\\'
	int nLen = strNewDir.length();
	if(nLen > 0 && strNewDir[nLen - 1] == '\\')
		strNewDir = strNewDir.substr(0, nLen - 1);

#ifdef _TESTING_
	MessageBox((LPSTR)strNewDir.c_str(), "Creating New Directory", MB_OK);
#endif

	rc = chdir((LPSTR)strNewDir.c_str());
	if(!rc)
	{
		strTemp = "Directory ";
		strTemp += strNewDir;
		strTemp += " already exists";

		MessageBox((LPSTR)strNewDir.c_str(), "Directory Found", MB_OK);
		CloseWindow(TRUE);
	}
	else
	{
		rc = SafeDirectoryCreate((LPSTR)strNewDir.c_str());
		if(!rc)
		{
			strTemp = "Unable to create directory '";
			strTemp += strNewDir;
			strTemp += "'.";

			MessageBox((LPSTR)strTemp.c_str(), "Can't Create Dir.", MB_OK);
			CloseWindow(FALSE);
		}
		else
			// Otherwise, success!
			CloseWindow(TRUE);
	}
}


