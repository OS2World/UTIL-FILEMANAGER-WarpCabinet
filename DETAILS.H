// DFileDetailsDlg -- Class definition
#if !defined(_DETAILS_H)
	#define _DETAILS_H

class DFileDetailsDlg : public TDialog
{
private:
	int bOldShowTimeStamp;
	int bOldShowAttr;
	int bOldShowSize;

	int *pbOldShowTimeStamp;
	int *pbOldShowAttr;
	int *pbOldShowSize;

public:

	DFileDetailsDlg(TWindow *parent, int *pbShowTimeStamp,
			int *pbShowAttr, int *pbShowSize) : TDialog(parent, IDD_FILE_DETAILS_DLG, 0)
		{
			bOldShowTimeStamp = FALSE;
			bOldShowAttr = FALSE;
			bOldShowSize = FALSE;

			if(!pbShowTimeStamp ||
				!pbShowAttr ||
				!pbShowSize)
			{
				MessageBox("Bad pointer in DFileDetailsDlg::DFileDetailsDlg!", "Error", MB_ICONSTOP);
				return;
			}

			pbOldShowTimeStamp = pbShowTimeStamp;
			pbOldShowAttr = pbShowAttr;
			pbOldShowSize = pbShowSize;

			bOldShowSize = *pbOldShowSize;

			bOldShowTimeStamp = *pbOldShowTimeStamp;
			bOldShowAttr = *pbOldShowAttr;
			bOldShowSize = *pbOldShowSize;
		}

	void SetupWindow();
	void CmOk();

	DECLARE_RESPONSE_TABLE (DFileDetailsDlg);
};

#endif


