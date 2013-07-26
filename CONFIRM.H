// DConfirmDlg -- Class definition
#if !defined(_CONFIRM_H)
	#define _CONFIRM_H

class DConfirmDlg : public TDialog
{
private:
	int bOldConfirmDelete;
	int bOldConfirmReplace;
	int bOldConfirmCopy;
	int bOldConfirmAll;

	int *pbOldConfirmDelete;
	int *pbOldConfirmReplace;
	int *pbOldConfirmCopy;
	int *pbOldConfirmAll;

public:

	DConfirmDlg(TWindow *parent, int *pbConfirmDelete,
						int *pbConfirmReplace, int *pbConfirmCopy,
						int *pbConfirmAll);

	void SetupWindow();
	void OnDeleteBoxClick();
	void OnCopyBoxClick();
	void OnReplaceBoxClick();
	void OnAllBoxClick();
	void CmOk();

	DECLARE_RESPONSE_TABLE (DConfirmDlg);
};

#endif


