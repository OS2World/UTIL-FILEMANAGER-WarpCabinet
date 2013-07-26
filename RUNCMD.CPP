// DRunCmdDlg -- Class implementation
#include <assert.h>
#include <cstring.h>
#include <process.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\dialog.h>
#include <owl\edit.h>
#include <owl\static.h>
#include <owl\button.h>
#include "resource.h"
#include "filentry.h"
#include "filetool.h"
#include "runcmd.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DRunCmdDlg, TDialog)
	EV_CHILD_NOTIFY(IDC_CMD_LINE, EN_CHANGE, OnCmdLineBoxChange),
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
END_RESPONSE_TABLE;

void DRunCmdDlg::SetupWindow()
{
	TDialog::SetupWindow();

	SetDlgItemText(IDC_CURRENT_DIR, lpszCurrentDir);

	SendDlgItemMsg(IDC_CMD_LINE, EM_SETTEXTLIMIT, 1025, 0);

	if(lpszInitCmd && strlen(lpszInitCmd) == 0)
		WinEnableWindow(GetDlgItem(DID_OK), 0);
	else
		SetDlgItemText(IDC_CMD_LINE, lpszInitCmd);
}

void DRunCmdDlg::OnCmdLineBoxChange()
{
	// Enable or disable OK button based on whether target box is empty.
	int cc = WinQueryWindowTextLength(GetDlgItem(IDC_CMD_LINE));
	WinEnableWindow(GetDlgItem(DID_OK), cc);
}

void DRunCmdDlg::CmOk()
{
	int rc;
	int i;

	const int MAX_ARGS = 32;

	static char szCmdLine[1025];

	string strCmdRunIt;

	GetDlgItemText(IDC_CMD_LINE, szCmdLine, 1025);

	static LPSTR pArgs[MAX_ARGS + 1];

	LPSTR p;
	int nArgCount = 0;
	int bFirst = TRUE;
	while(TRUE)
	{
		if(bFirst)
		{
			p = strtok(szCmdLine, " ");
			bFirst = FALSE;
		}
		else
		{
			p = strtok(NULL, " ");
		}

		if(p)
		{
			if(strlen(p))
			{
				if(nArgCount >= MAX_ARGS - 1)
				{
					MessageBox("Too many command line arguments!", "Can't Run Command", MB_ICONEXCLAMATION);
					CloseWindow(FALSE);
					return;
				}
				nArgCount++;
				pArgs[nArgCount - 1] = p;
			}
		}
		else
			break;  // Nothing more to parse!
	}

	if(nArgCount == 0)
	{
		MessageBox("Nothing to run.", "Can't Run Command", MB_ICONEXCLAMATION);
		CloseWindow(FALSE);
		return;
	}
	else
		pArgs[nArgCount] = NULL;  // Append last null to array of pointers.

	// Check which kind of .BAT / .EXE / .CMD / .COM file
	strCmdRunIt = pArgs[0];
	strCmdRunIt.to_upper();

	string strTemp1;
	string strPgm;
	string strArgs;

	int bUseArgs = FALSE;

	strTemp1 = ".CMD";
	if(strCmdRunIt.find_first_of(strTemp1) != NPOS)
	{
		// Then it's a .CMD file
		strPgm = "CMD.EXE";

		strArgs = "/K";
		for(i = 0; i < nArgCount; i++)
		{
			strArgs += " ";
			strArgs += pArgs[i];
		}
		bUseArgs = TRUE;
	}
	else
	{
		strTemp1 = ".BAT";
		// Then it's a .BAT file
		if(strCmdRunIt.find_first_of(strTemp1) != NPOS)
		{
			strPgm = "COMMAND.COM";

			strArgs = "";
			for(i = 0; i < nArgCount; i++)
			{
				strArgs += " ";
				strArgs += pArgs[i];
			}
			bUseArgs = TRUE;
		}
		else
		{
			// Treat it as a runnable file on its own--an .EXE or .BAT
			strPgm = pArgs[0];

			strArgs = "";
			for(i = 1; i < nArgCount; i++)
			{
				if(i > 1)
					strArgs += " ";
				strArgs += pArgs[i];
			}
			if(i > 1)
				bUseArgs = TRUE;		}
	}

	// To here, run program and report result.
	rc = RunProgramWithArgs((char *)strPgm.c_str(),
									bUseArgs ? (char *)strArgs.c_str() : NULL);
	if(!rc)
	{
		MessageBox("There was an error running this program.  Check that path is correct or close some programs to free up memory and try again.", "Can't Run", MB_ICONSTOP);
		CloseWindow(FALSE);
	}
	else
		CloseWindow(TRUE);
}


