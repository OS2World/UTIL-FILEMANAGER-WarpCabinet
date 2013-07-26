// DSelFilesDlg -- Class definition
#if !defined(_SELFILES_H)
	#define _SELFILES_H

class DSelFilesDlg : public TDialog
{
private:
	char szOldFileMask[257];

	LPSTR lpszOldFileMask;
public:

	DSelFilesDlg(TWindow *parent, LPSTR lpszFileMask) :
		TDialog(parent, IDD_SEL_FILES_DLG, 0)
		{
			strcpy(szOldFileMask, "");

			if(!lpszFileMask)
			{
				MessageBox("Bad pointer in DSelFilesDlg::DSelFilesDlg!", "Error", MB_ICONSTOP);
				return;
			}

			lpszOldFileMask = lpszFileMask;

			strcpy(szOldFileMask, lpszOldFileMask);
		}

	void SetupWindow();
	void OnFileMaskBoxChange();
	void CmOk();
	void CmDeselect();

	DECLARE_RESPONSE_TABLE (DSelFilesDlg);
};

#endif


