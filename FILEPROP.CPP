// DFilePropertiesDlg -- Class implementation
#include <assert.h>
#include <cstring.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\dialog.h>
#include <owl\edit.h>
#include <owl\static.h>
#include <owl\button.h>
#include "resource.h"
#include "fileasoc.h"
#include "filentry.h"
#include "filetool.h"
#include "fileprop.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DFilePropertiesDlg, TDialog)
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
END_RESPONSE_TABLE;

void DFilePropertiesDlg::SetupWindow()
{
	TDialog::SetupWindow();

	// Load first file's attributes to our property dialog.
	FILEENTRY *pFileEntry;

	ULONG ulTotalBytes = 0;
	ULONG ulTotalKiloBytes = 0;

	char szTemp[40];

	string strInfo("");
	string strSize("");

	// On each click, update total dir.'s and files in status bar.
	int nSelCount = SelectedFileList.GetItemsInContainer();

	for(int i = 0; i < nSelCount; i++)
	{
		pFileEntry = SelectedFileList[i];
		// Total up our selected files.
		if(pFileEntry)
			ulTotalBytes += (ULONG)pFileEntry->lFileSize;
	}

	pFileEntry = (FILEENTRY *)SelectedFileList[0];

	if(!pFileEntry)
		return;

	if((pFileEntry->ulFileAttrib & FA_RDONLY) != 0)
		CheckDlgButton(IDC_READONLY, 1);
	if((pFileEntry->ulFileAttrib & FA_ARCH) != 0)
		CheckDlgButton(IDC_ARCHIVE, 1);
	if((pFileEntry->ulFileAttrib & FA_HIDDEN) != 0)
		CheckDlgButton(IDC_HIDDEN, 1);
	if((pFileEntry->ulFileAttrib & FA_SYSTEM) != 0)
		CheckDlgButton(IDC_SYSTEM, 1);

	if(lpszCurrentDir)
	{
		strInfo = "Path: ";
		strInfo += lpszCurrentDir;
		SetDlgItemText(IDC_PATH, strInfo.c_str());
	}

	ulTotalKiloBytes = ulTotalBytes / 1000;

	if(nSelCount == 1)
	{
		char szDateTime[30];

		GetFileTimeStampAsString(szDateTime, pFileEntry->uFileDate, pFileEntry->uFileTime);

		strInfo = "File: ";
		strInfo += pFileEntry->szFileName;
		SetDlgItemText(IDC_FILENAME, strInfo.c_str());

		strSize = "Size: ";
		strSize += ltoa(ulTotalKiloBytes, szTemp, 10);
		strSize += " KB";

		strInfo = "Modified: ";
		strInfo += szDateTime;
		SetDlgItemText(IDC_MODIFIED, strInfo.c_str());

		SetDlgItemText(IDC_FILESIZE, strSize.c_str());
	}
	else if(nSelCount > 1)
	{
		// Hide last modified.
		WinShowWindow(GetDlgItem(IDC_MODIFIED), 0);

		// Hide filename, use size for both # of objects and total size.
		WinShowWindow(GetDlgItem(IDC_FILENAME), 0);

		strInfo = itoa(nSelCount, szTemp, 10);
		strInfo += " objects selected, ";

		strInfo += ltoa(ulTotalKiloBytes, szTemp, 10);
		strInfo += " KB";

		SetDlgItemText(IDC_FILESIZE, strInfo.c_str());
	}
}

void DFilePropertiesDlg::CmOk()
{
	int rc;

	// Retrieve new attributes from buttons.
	ULONG ulNewAttrib = 0;

	if(IsDlgButtonChecked(IDC_READONLY) == 1)
		ulNewAttrib |= FA_RDONLY;
	if(IsDlgButtonChecked(IDC_ARCHIVE) == 1)
		ulNewAttrib |= FA_ARCH;
	if(IsDlgButtonChecked(IDC_HIDDEN) == 1)
		ulNewAttrib |= FA_HIDDEN;
	if(IsDlgButtonChecked(IDC_SYSTEM) == 1)
		ulNewAttrib |= FA_SYSTEM;

	rc = ChangeFileAttributes(lpszCurrentDir, ulNewAttrib);

	rc; // Kill warning.

	CloseWindow(TRUE);
}


