// DAssociationsDlg -- Class implementation
#include <cstring.h>
#include <dir.h>
#include <string.h>
#include <stdio.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\dialog.h>
#include <owl\edit.h>
#include <owl\listbox.h>
#include <owl\static.h>
#include <owl\button.h>
#include "resource.h"
#include "os2api.h"
#include "fileasoc.h"
#include "associat.h"

#define DID_OK      1
#define DID_CANCEL  2

DEFINE_RESPONSE_TABLE1(DAssociationsDlg, TDialog)
	EV_CHILD_NOTIFY(IDC_ASSOC_LIST, LBN_SELCHANGE, OnAssocListClick),
	EV_CHILD_NOTIFY(IDC_DELETE_ASSOC, BN_CLICKED, CmDeleteAssoc),
	EV_CHILD_NOTIFY(DID_OK, BN_CLICKED, CmOk),
	EV_CHILD_NOTIFY(DID_CANCEL, BN_CLICKED, CmCancel),
END_RESPONSE_TABLE;

DAssociationsDlg::DAssociationsDlg(TWindow *parent, const TISArrayAsVector <FILEASSOCIATION> *pFileAssociations,
		const LPSTR lpszNewExtension) : TDialog(parent, IDD_ASSOCIATE_DLG, 0)
{
	nCurrentEditIndex = -1;
	bAssocDeleted = FALSE;

	// Save pointer, and copy original data for editing.
	if(!pFileAssociations)
	{
		MessageBox("Bad pointer in DAssociationsDlg::DAssociationsDlg!", "Error", MB_ICONSTOP);
		return;
	}

	pOldFileAssociations = (TISArrayAsVector <FILEASSOCIATION> *)pFileAssociations;
	pNewFileAssociations = new TISArrayAsVector <FILEASSOCIATION>(10, 0, 1);

	FILEASSOCIATION *pOldAssoc;
	FILEASSOCIATION *pNewAssoc;

	// Copy old assocation table array.
	for(int i = 0; i < pFileAssociations->GetItemsInContainer(); i++)
	{
		pOldAssoc = (*pFileAssociations)[i];
		if(!pOldAssoc)
			continue;
		pNewAssoc = new FILEASSOCIATION(*pOldAssoc);
		pNewFileAssociations->Add(pNewAssoc);
	}
	if(lpszNewExtension)
		strcpy(szInitExt, lpszNewExtension);
	else
		szInitExt[0] = 0;
}

void DAssociationsDlg::SetupWindow()
{
	TDialog::SetupWindow();

	TListBox AssocLB(this, IDC_ASSOC_LIST);
	AssocLB.Attr.Style |= LBS_SORT; // Set sort bit.

	// Load current associations into our listbox. [Ext. + ( Desc, )]
	string strEntry;
	string strTemp;

	SetDlgItemText(IDC_EXT, szInitExt);

	FILEASSOCIATION *pAssoc;
	int nNewIndex;
	int nArrayIndex;
	int nMatchIndex = -1;

	int nCount = pNewFileAssociations->GetItemsInContainer();
	for(int i = -1; i < nCount; i++)
	{
		if(i == -1)  // First time, just add 'none'.
			strEntry = "(None)";
		else
		{
			pAssoc = (*pNewFileAssociations)[i];
			if(!pAssoc)
				continue;

			if(strlen(pAssoc->szExt) > 0)
			{
				strTemp = pAssoc->szExt;
				strTemp.to_upper();
			}
			else
				strTemp = "";

			strEntry = strTemp;
			if(strlen(pAssoc->szDesc) > 0)
			{
				strEntry += " (";
				strEntry += pAssoc->szDesc;
				strEntry += ")";
			}
			else
				strEntry += "(N.A.)";
		}
		nNewIndex = SendDlgItemMessage(IDC_ASSOC_LIST, LB_ADDSTRING, -1, (LPARAM)(LPSTR)strEntry.c_str());
		if(nNewIndex != -1)
		{
			nArrayIndex = i > -1 ? i : -1;  // No array index for "None".
			// Store index to file association array along with list item.
			SendDlgItemMessage(IDC_ASSOC_LIST, LB_SETITEMDATA, nNewIndex, (LPARAM)nArrayIndex);
			// Save matching index, if this matches.
			if(nArrayIndex != -1)
				if(strcmpi(pAssoc->szExt, szInitExt) == 0)
					nMatchIndex = nNewIndex;
		}
	}
	if(nMatchIndex != -1)
	{
		SendDlgItemMessage(IDC_ASSOC_LIST, LB_SETCURSEL, nMatchIndex, 1);
		nCurrentEditIndex = nMatchIndex;
	}
	else
	{
		// 'None'.
		SendDlgItemMessage(IDC_ASSOC_LIST, LB_SETCURSEL, 0, 1);
		nCurrentEditIndex = 0;
	}
}

void DAssociationsDlg::CmOk()
{
	// Retrieve new association settings and write them back to parent's data.

	// Now update new extension.
	int nSel = SendDlgItemMessage(IDC_ASSOC_LIST, LB_GETCURSEL, 0, 0);
	FILEASSOCIATION TempAssoc;
	FILEASSOCIATION *pThisFileAssoc;
	FILEASSOCIATION *pAssoc;
	int nIndex;
	char szTemp[256];

	if(nCurrentEditIndex == -1 && bAssocDeleted == FALSE)
		CloseWindow(FALSE);

	// Get new settings.
	GetDlgItemText(IDC_EXT, TempAssoc.szExt, 4);
	GetDlgItemText(IDC_ASSOC_DESC, TempAssoc.szDesc, 128);
	GetDlgItemText(IDC_ASSOC_PATH, TempAssoc.szEXEPathName, 256);

	if(strlen(TempAssoc.szExt) > 0)
	{
		if(strlen(TempAssoc.szDesc) == 0)
		{
			MessageBox("Fill-in a description first!", "Description Missing", MB_ICONEXCLAMATION);
			return;
		}

		if(strlen(TempAssoc.szEXEPathName) == 0)
		{
			MessageBox("Fill-in a valid path first!", "Path Missing", MB_ICONEXCLAMATION);
			return;
		}
	}

	// Copy all entries to global array.
	pOldFileAssociations->Flush(TShouldDelete::Delete);
	int nCount = SendDlgItemMessage(IDC_ASSOC_LIST, LB_GETCOUNT, 0, 0);
	for(int i = 0; i < nCount; i++)
	{
		// Copy each association from listbox.
		if((i == -1 || i == nSel) && strlen(TempAssoc.szExt) > 0)
			// New entry or edited entry.
			pOldFileAssociations->Add(new FILEASSOCIATION(TempAssoc));
		else
		{
			nIndex = SendDlgItemMessage(IDC_ASSOC_LIST, LB_GETITEMDATA, i, 0);			if(nIndex != -1)
			{
				pAssoc = (*pNewFileAssociations)[nIndex];
				if(pAssoc)
					pOldFileAssociations->Add(new FILEASSOCIATION(*pAssoc));
			}
		}
	}

	pNewFileAssociations->Flush(TShouldDelete::Delete);

	delete pNewFileAssociations;
	pNewFileAssociations = NULL;

	CloseWindow(TRUE);
}

void DAssociationsDlg::CmCancel()
{
	pNewFileAssociations->Flush(TShouldDelete::Delete);

	delete pNewFileAssociations;
	pNewFileAssociations = NULL;

	CloseWindow(FALSE);
}

void DAssociationsDlg::OnAssocListClick()
{
	// Fill edit controls with current association if one is selected.
	int nIndex;
	FILEASSOCIATION *pAssoc;
	int nSel = SendDlgItemMessage(IDC_ASSOC_LIST, LB_GETCURSEL, 0, 0);
	if(nSel > 0)
	{
		nIndex =(int)SendDlgItemMessage(IDC_ASSOC_LIST, LB_GETITEMDATA, nSel, 0);
		if(nIndex > -1)
		{
			pAssoc = (FILEASSOCIATION *)(*pNewFileAssociations)[nIndex];
			if(pAssoc)
			{
				SetDlgItemText(IDC_EXT, pAssoc->szExt);
				SetDlgItemText(IDC_ASSOC_DESC, pAssoc->szDesc);
				SetDlgItemText(IDC_ASSOC_PATH, pAssoc->szEXEPathName);
				nCurrentEditIndex = nIndex;
			}
		}
	}
	else
	{
		// Note: 0 = 'None'
		SetDlgItemText(IDC_EXT, "");
		SetDlgItemText(IDC_ASSOC_DESC, "");
		SetDlgItemText(IDC_ASSOC_PATH, "");
		nIndex = -1;
	}
}

void DAssociationsDlg::CmDeleteAssoc()
{
	int nTotal;
	// Deletes entry in LB if one is selected.
	int nSel = SendDlgItemMessage(IDC_ASSOC_LIST, LB_GETCURSEL, 0, 0);
	if(nSel > 0)  // Can't delete '(None)'--the first choice!
	{
		SendDlgItemMessage(IDC_ASSOC_LIST, LB_DELETESTRING, nSel, 0);
		SendDlgItemMessage(IDC_ASSOC_LIST, LB_SETCURSEL, -1, 0);
		bAssocDeleted = TRUE;

		SetDlgItemText(IDC_EXT, "");
		SetDlgItemText(IDC_ASSOC_DESC, "");
		SetDlgItemText(IDC_ASSOC_PATH, "");

		nCurrentEditIndex = -1;
	}
}

