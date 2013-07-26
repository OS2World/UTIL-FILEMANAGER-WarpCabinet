// DFileMaskDlg -- Class definition
#if !defined(_FILEMASK_H)
	#define _FILEMASK_H

class DFileMaskDlg : public TDialog
{
private:
	char szOldFileMask[257];
	int bOldShowSpecialObjs;

	LPSTR lpszOldFileMask;
	int *pbOldShowSpecialObjs;

public:

	DFileMaskDlg(TWindow *parent, LPSTR lpszFileMask,
		int *pbShowSpecialObjs) : TDialog(parent, IDD_VIEW_FILES_DLG, 0)
		{
			strcpy(szOldFileMask, "");
			bOldShowSpecialObjs = FALSE;

			if(!lpszFileMask ||
				!pbShowSpecialObjs)
			{
				MessageBox("Bad pointer in DFileMaskDlg::DFileMaskDlg!", "Error", MB_ICONSTOP);
				return;
			}

			lpszOldFileMask = lpszFileMask;
			pbOldShowSpecialObjs = pbShowSpecialObjs;

			strcpy(szOldFileMask, lpszOldFileMask);
			bOldShowSpecialObjs = *pbOldShowSpecialObjs;
		}

	void SetupWindow();
	void OnFileMaskBoxChange();
	void OnAllFilesBtnClick();
	void CmOk();

	DECLARE_RESPONSE_TABLE (DFileMaskDlg);
};

#endif


