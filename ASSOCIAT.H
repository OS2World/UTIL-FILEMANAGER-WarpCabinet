// DAssociationsDlg -- Class definition
#if !defined(_ASSOCIAT_H)
	#define _ASSOCIAT_H

#include <classlib\arrays.h>

class DAssociationsDlg : public TDialog
{
private:
	// Copy of old data--modify this copy.
	TISArrayAsVector <FILEASSOCIATION> *pNewFileAssociations;

	// Pointer to old data--write it back to parent's data.
	TISArrayAsVector <FILEASSOCIATION> *pOldFileAssociations;

	char szInitExt[8];

	int nCurrentEditIndex;

	int bAssocDeleted;

public:
	DAssociationsDlg(TWindow *parent, const TISArrayAsVector <FILEASSOCIATION> *pFileAssociations,
						  const LPSTR lpszNewExtension);
	~DAssociationsDlg() { if(pNewFileAssociations) pNewFileAssociations->Flush(TShouldDelete::Delete); delete pNewFileAssociations; }

	void SetupWindow();
	void CmOk();
	void CmCancel();
	void OnAssocListClick();
	void CmDeleteAssoc();

	DECLARE_RESPONSE_TABLE (DAssociationsDlg);
};

#endif


