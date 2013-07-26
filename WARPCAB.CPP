/*********************************************************************
	WARPCAB.CPP  -- 	Warp Cabinet-- A File Viewer using Borland's
							ObjectWindows Library (OWL) for OS/2 Warp.

							(Uses Borland C++ for OS/2, v.2.x.)

							v. 1.00 -- Source Code

							PC Magazine,	(c)1996 -- Ziff-Davis Publishing Co.
												All rights reserved.
												First Published in PC Magazine,
												US Edition, Feb. 26, 1996.
												Written by Richard V. Dragan.
**********************************************************************/
#include <cstring.h>
#include <assert.h>
#include <process.h>
#include <dir.h>
#include <io.h>
#include <stdio.h>
#include <classlib\arrays.h>
#include <owl\owlpch.h>
#include <owl\applicat.h>
#include <owl\decframe.h>
#include <owl\gadget.h>
#include <owl\textgadg.h>
#include <owl\statusba.h>
#include <owl\listbox.h>
#include <owl\dialog.h>
#include <owl\menu.h>

#include "os2api.h"

#include "resource.h"
#include "fileasoc.h"
#include "filentry.h"
#include "filetool.h"
#include "fileprop.h"
#include "filechng.h"  // File Copy/Move/Rename/Delete dialog box
#include "runcmd.h"    // Run a command in a dialog box
#include "associat.h"  // Associations dialog box
#include "creatdir.h"  // Create new dir. dialog
#include "selfiles.h"  // Select files by mask.
#include "filemask.h"  // Set File mask dialog
#include "details.h"   // Select File Details dialog
#include "confirm.h"   // Set Confirmation Options dialog

#define _DROP_API_ONLY_
#include "do_drag.h"    // Drag-and-drop Direct Manipulation routines
#undef _DROP_API_ONLY_

#pragma hdrstop

#define NO_BTN		0
#define RIGHT_BTN	1
#define LEFT_BTN	2

BOOL bContinueFileOp = TRUE;

// Object to store our list of associations--file types + programs that work with them.
TISArrayAsVector <FILEASSOCIATION> FileAssociations(10, 0, 1);
TISArrayAsVector <FILEENTRY> SelectedFileList(10, 0, 1);
TISArrayAsVector <string> AllFilesList(10, 0, 1);
// Dirs. that must be created (for copying, moving or renaming) or deleted for deleting.
TISArrayAsVector <string> CriticalDirsList(8, 0, 1);

ULONG ulTotalFilesBytes = 0;

TStatusBar *pGlobalStatus = NULL;
TTextGadget *pGlobalStatusBar = NULL;
TWindow *pGlobalParent = NULL;
HWND hwndGlobalParent = NULL;

HPOINTER hptrMove;
HPOINTER hptrCopy;
int bDragCopy;  // FALSE = MOVE

// Which files to show.
char szMask[80];
char szSelByMask[80];
unsigned long ulShowAttrBits = 0;
int bShowHiddenOrSysObjs = FALSE;

// Confirmation options.
int bConfirmDelete = FALSE;
int bConfirmReplace = FALSE;
int bConfirmCopy = FALSE;
int bConfirmAll = TRUE;  // By default, confirm everything.

int bDragLeftBtn = FALSE;  // By default, drag by right mouse button a la OS/2.

const int MAXDIRTEXT = 256;
const int CYLBHEIGHT = 18;

const unsigned long CTRL_MOUSE_KEY = 1048576;  // (2^20)

char szINIPath[256];

typedef struct _TREEVIEW_ITEM
{
	int nListBoxIndex;  // Index # of this entry.
	int nLevel;         // In hierarchical list, this is the level of this entry
	int nExpandCode;    // 0 = Nothing to expand, 1 == Can Expand [+], 2 Already Expanded [-]
	int bIsEndOfLevel;
	LPSTR Parent; // Our parent's index on hierarchy, (NULL) for root entries
	int iImage;
	int iSelectedImage;
	LPSTR lpszDirText;
	int nTextLen;
} TREEVIEW_ITEM;

void ToUpperCase(char *pszOut, char *pszIn, int nMaxLen)
{
	if(!pszOut || !pszIn)
		return;

	for(int i = 0; i < strlen(pszIn) && i < nMaxLen; i++)
		pszOut[i] = toupper(pszIn[i]);

	if(i < nMaxLen)
		pszOut[i] = '\0';
}

class DIRLIST_ITEM
{
public:
	TREEVIEW_ITEM *pTVI;  // This is not allocated in this class!

	DIRLIST_ITEM(TREEVIEW_ITEM *pMyTVI = NULL) { pTVI = pMyTVI; }

	int operator<(const DIRLIST_ITEM& CompareItem) {
		if(!pTVI && !CompareItem.pTVI)
			return TRUE;
		else if(!pTVI && CompareItem.pTVI)
			return FALSE;
		else if(pTVI && !CompareItem.pTVI)
			return FALSE;
		else // Two valid entries, compare strings--case insensitive here..
		{
			int rc = strcmpi(pTVI->lpszDirText, (char *)CompareItem.pTVI->lpszDirText);
			if(rc < 0)
				return TRUE;
			else
				return FALSE;
		}
	}
	int operator==(const DIRLIST_ITEM& CompareItem) {
		if(pTVI == CompareItem.pTVI)  // If data items are equal, we have a match.
			return TRUE;
		else
			return FALSE;
	}
};

typedef TREEVIEW_ITEM *PTREEVIEW_ITEM;

class LBHITTEST
{
public:
	int nIndex;
	TRect rect;

	LBHITTEST(int nNewIndex, TRect& r) { nIndex = nNewIndex; rect = r; }

	int operator==(LBHITTEST& OldRec)
	{
		if(OldRec.nIndex == nIndex && OldRec.rect == rect)
			return TRUE;
		else
			return FALSE;
	}
};

TIArrayAsVector <LBHITTEST> FolderDragTargets(16, 0, 1);

const char PRF_INI_FILENAME[] = "WARPCAB.INI";
const char PRF_INI_SECTION[] = "filetypes";

typedef unsigned int uint;

const int MAXIMAGES = 11;
const int CXIMAGE = 16;
const int CYIMAGE = 16;

const int cxSplitterWidth = 8;

class CustomListBox : public TListBox
{
public:
	HWND hwndParent;

	int bForwarding;

	CustomListBox(TWindow* parent, int id,
				  int x, int y, int w, int h,
				  TModule* module = 0) : TListBox(parent, id, x, y, w, h, module)
					{
						if(parent)
							hwndParent = parent->HWindow;
						else
							hwndParent = NULL;

						bForwarding = FALSE;
					}

	void SetForwarding(BOOL bOn = TRUE) { bForwarding = bOn; }


	BOOL EvSetCursor(HWND hwnd, uint hittest, uint mousemsg) { return TRUE; }

	void EvMouseMove(UINT keys, TPoint& point);

	// OS/2 drag and drop messages.
	LRESULT DoDragOver(WPARAM wParam, LPARAM lParam) { return (LRESULT)WinSendMsg(hwndParent, DM_DRAGOVER, (MPARAM)wParam, (MPARAM)GetId()); }
	LRESULT DoDragLeave(WPARAM wParam, LPARAM lParam) { return (LRESULT)WinSendMsg(hwndParent, DM_DRAGLEAVE, (MPARAM)wParam, (MPARAM)GetId()); }
	LRESULT DoDropHelp(WPARAM wParam, LPARAM lParam) { return (LRESULT)WinSendMsg(hwndParent, DM_DROPHELP, (MPARAM)wParam, (MPARAM)GetId()); }
	LRESULT DoDrop(WPARAM wParam, LPARAM lParam) { return (LRESULT)WinSendMsg(hwndParent, DM_DROP, (MPARAM)wParam, (MPARAM)GetId()); }

	DECLARE_RESPONSE_TABLE(CustomListBox);
};

DEFINE_RESPONSE_TABLE1(CustomListBox, TListBox)
	EV_WM_MOUSEMOVE,
	EV_WM_SETCURSOR,
	EV_MESSAGE(DM_DRAGOVER, DoDragOver),
	EV_MESSAGE(DM_DRAGLEAVE, DoDragLeave),
	EV_MESSAGE(DM_DROPHELP, DoDropHelp),
	EV_MESSAGE(DM_DROP, DoDrop),
END_RESPONSE_TABLE;

//	Define our Main Window.
class DFrame : public TWindow
{
	public:
		DFrame(TWindow *parent = NULL);
		~DFrame();
		void SetupWindow();

		// Menu Commands
		int nOpenFileIndex;

		void CmOpen();
		void CmEnableOpen(TCommandEnabler& ce);

		void CmMove();
		void CmEnableMove(TCommandEnabler& ce);

		void CmCopy();
		void CmEnableCopy(TCommandEnabler& ce);

		void CmDelete();
		void CmEnableDelete(TCommandEnabler& ce);

		void CmProperties();
		void CmEnableProperties(TCommandEnabler& ce);

		void CmRun();

		void CmAssociate();

		void CmCreateDir();
		void CmExit();

		void CmShowName();
		void CmAllFileDetails();
		void CmPartialFileDetails();

		void CmSelectFiles();
		void CmSelectAll();
		void CmSelectNone();

		void CmSortByName();
		void CmSortByType();
		void CmSortBySize();
		void CmSortByDate();

		void CmRefreshOneFolder();
		void CmRefreshAllFolders();

		void CmShowFileTypes();

		void CmConfirmOptions();

		void CmDragWithLeftBtn();

		void CmAbout();

		int bShowAllFileDetails;

		int bShowSize;
		int bShowTimeStamp;
		int bShowAttr;

		HWND hwndMenu;

		TMemoryDC *dcMem;

		// Sent by threads to report a result of a file operation.
		LRESULT PMFileOperationResult(WPARAM wParam, LPARAM lParam);

		// Mouse and drag and drop messages.
		void EvLButtonDown(UINT keys, TPoint& point);
		void EvRButtonDown(UINT keys, TPoint& point);

		void EvMouseMove(UINT keys, TPoint& point);

		void EvLButtonUp(UINT keys, TPoint& point);
		void EvRButtonUp(UINT keys, TPoint& point);

		// Owner-draw listbox support.
		UINT EvMeasureItem(UINT ctrlId, UINT index) { return CYLBHEIGHT; }

		// Custom handler for WM_OWNERDRAW--The EvDrawItem() version is gutted and
		// supports only a dummy OWNERITEM structure.
		LRESULT DoDrawItem(WPARAM, LPARAM);
		void DrawImage(TDC& dc, int nImage, int x, int y, UINT dwROP = SRCCOPY, BOOL bUseMask = FALSE);

		unsigned int wLastMouseMsg;
		TPoint ptLastMouse;

		TRect rOldRect;

		HPOINTER hptrFile;
		HPOINTER hptrFolder;
		HPOINTER hptrMulti;
		HPOINTER hptrBusy;

		void UpdateFileHdrText(LPSTR lpszNewText = NULL);
		void Paint(TDC& dc, BOOL erase, TRect& rect);

		void EvSize(uint sizeType, TSize& size);

		void DrawSplitterWindowBar(TDC& dc, TRect& rect);

		// (Functions that pass important control items from frame...)
		void SetStatusTextCtrl(TTextGadget *pMyStatusText) { pStatusText = pMyStatusText;
					pGlobalStatusBar = pMyStatusText; }
		void SetFrame(TWindow *pMyFrame) { pFrame = pMyFrame; }

		int GetSelectedDir(string& strDir, int nTargetIndex = -1);
		int GetFileListDblClickIndex();

		int bFolderOnlyNoSelections;

		void LoadInitDirs(void);

		BOOL IsNextNodeExpandable(LPSTR lpszThisDir);
		BOOL ExpandDirNode(TREEVIEW_ITEM *Parent, int nStartIndex, int nLevel, LPSTR lpszThisDir, LPSTR lpszInitSearch);

		BOOL ResortFileEntries(int nNewSortMethod = SORT_BY_NAME);

		int GetFileSelections();
		int nFileListSelCount;

		// Handle listbox notification commands for clicking and double-clicking.
		void OnFolderListClick();
		void OnFolderListDblClick();
		void OnFileListClick();
		void OnFileListDblClick();

		int cxMaxFileListHorizExt;

		// These fields are used to track how wide each element to display is.
		int cxName;
		int cxSize;
		int cxTimeStamp;
		int cxAttribute;

		BOOL bResizingPanes;
		BOOL bOverResizer;

		TRect rFolderListHdr;
		TRect rFileListHdr;

		CustomListBox *FolderList;
		TRect rFolderList;

		TRect rSplitter;
		TRect rOldSplitter;

		CustomListBox *FileList;
		TRect rFileList;

		int cxWin;
		int cyWin;

		int cyFolderList;
		int cyFileList;

		int cxBorder;
		int cyTitleBar;

		string strDirHdr;
		string strFileHdr;
		string strStatus;

		TFont *pfontWork;
		TFont *pfontLB;

		TTextGadget *pStatusText;

		TWindow *pFrame;

		TPen *HighlightPen;

		TPen *ConnectorPen;
		TPen *SolidConnectorPen;

		TBrush *MaskBrush;

		DWORD rgbLBBackground;

		// Used to track bitmaps/images in ownerdraw listboxes
		TBitmap *ImageList[MAXIMAGES];

		char szCurrentListViewDir[256];
		int nFileSourceCount;

		int nSortCode;
		int nOldSortCheckedItem;

		// For dragging files.
		int nTargetIndex;
		int bDraggingFiles;
		int bFirstDragMovement;

		int LoadDirectoryToFilesLB(TListBox *FileLB, LPSTR lpszDir, LPSTR lpszMask, int bShowHiddenSysFilesToo, int nSortCode = SORT_BY_NAME);

		int AddTreeViewItem(TListBox* pLB, TREEVIEW_ITEM& TreeViewItem, int nPos = -1);
		int AddListViewItem(TListBox* pLB, FILEENTRY *ListViewItem, int nPos = -1);

		void ClearFolderList();
		void ClearFileList();

		BOOL IsWindowIconic();

		long lCount;

		// Change the cursor.
		void ShowArrowCursor() { SetCursor(0, IDC_ARROW); }
		void ShowWaitCursor() { SetCursor(0, IDC_WAIT); }

		int cxCurrentDir; // Used to size current directory listbox;

		LRESULT DoDragOver(WPARAM wParam, LPARAM lParam);
		LRESULT DoDragLeave(WPARAM wParam, LPARAM lParam);
		LRESULT DoDropHelp(WPARAM wParam, LPARAM lParam);
		LRESULT DoDrop(WPARAM wParam, LPARAM lParam);

		int nMouseBtnDownCode;
		int bCouldDrag;

		int bLocalDragging;
		int bRemoteDragging;
		int bRemoteDraggingInit;

	DECLARE_RESPONSE_TABLE (DFrame);
};

// The 'RESPONSE TABLE' here is a definition of what
// 'events' or Windows messages our window is prepared to handle.
DEFINE_RESPONSE_TABLE1(DFrame, TWindow)
	EV_WM_SIZE,
	EV_WM_LBUTTONDOWN,
	EV_WM_MOUSEMOVE,
	EV_WM_LBUTTONUP,
	EV_WM_RBUTTONDOWN,
	EV_WM_RBUTTONUP,
	EV_WM_MEASUREITEM,
	EV_MESSAGE(WM_DRAWITEM, DoDrawItem),
	EV_CHILD_NOTIFY(IDC_DIRLIST, LBN_SELCHANGE, OnFolderListClick),
	EV_CHILD_NOTIFY(IDC_DIRLIST, LBN_DBLCLK, OnFolderListDblClick),
	EV_CHILD_NOTIFY(IDC_FILELIST, LBN_SELCHANGE, OnFileListClick),
	EV_CHILD_NOTIFY(IDC_FILELIST, LBN_DBLCLK, OnFileListDblClick),
	EV_COMMAND(CM_OPEN, CmOpen),
	EV_COMMAND(CM_MOVE, CmMove),
	EV_COMMAND(CM_COPY, CmCopy),
	EV_COMMAND(CM_DELETE, CmDelete),
	EV_COMMAND(CM_PROPERTIES, CmProperties),
	EV_COMMAND(CM_RUN, CmRun),
	EV_COMMAND(CM_ASSOCIATE, CmAssociate),
	EV_COMMAND(CM_CREATE_DIR, CmCreateDir),
	EV_COMMAND(CM_APP_EXIT, CmExit),
	EV_COMMAND(CM_SELECTFILES, CmSelectFiles),
	EV_COMMAND(CM_SELECTALL, CmSelectAll),
	EV_COMMAND(CM_CLEARSELS, CmSelectNone),
	EV_COMMAND(CM_FILE_NAME, CmShowName),
	EV_COMMAND(CM_ALL_DETAILS, CmAllFileDetails),
	EV_COMMAND(CM_PARTIAL_DETAILS, CmPartialFileDetails),
	EV_COMMAND(CM_SHOWFILES, CmShowFileTypes),
	EV_COMMAND(CM_SORTNAME, CmSortByName),
	EV_COMMAND(CM_SORTTYPE, CmSortByType),
	EV_COMMAND(CM_SORTSIZE, CmSortBySize),
	EV_COMMAND(CM_SORTDATE,CmSortByDate),
	EV_COMMAND(CM_CONFIRMATION, CmConfirmOptions),
	EV_COMMAND(CM_REFRESH, CmRefreshOneFolder),
	EV_COMMAND(CM_RELOAD_FOLDERS, CmRefreshAllFolders),
	EV_COMMAND(CM_DRAG_LEFT_BTN, CmDragWithLeftBtn),
	EV_COMMAND(CM_ABOUT, CmAbout),
	EV_COMMAND_ENABLE(CM_OPEN, CmEnableOpen),
	EV_COMMAND_ENABLE(CM_MOVE, CmEnableMove),
	EV_COMMAND_ENABLE(CM_COPY, CmEnableCopy),
	EV_COMMAND_ENABLE(CM_DELETE, CmEnableDelete),
	EV_COMMAND_ENABLE(CM_PROPERTIES, CmEnableProperties),
	EV_MESSAGE(DM_DRAGOVER, DoDragOver),
	EV_MESSAGE(DM_DRAGLEAVE, DoDragLeave),
	EV_MESSAGE(DM_DROPHELP, DoDropHelp),
	EV_MESSAGE(DM_DROP, DoDrop),
END_RESPONSE_TABLE;

// Class constructor.
DFrame::DFrame(TWindow *parent)
{
	Init(parent, 0, 0);

	// Load bitmaps images.
	HINSTANCE hInst = GetModule()->GetInstance();

	assert(hInst);

	dcMem = NULL;

	cxName = 0;
	cxSize = 0;
	cxTimeStamp = 0;
	cxAttribute = 0;

	nMouseBtnDownCode = NO_BTN;
	bCouldDrag = FALSE;

	bLocalDragging = FALSE;
	bRemoteDragging = FALSE;
	bRemoteDraggingInit = FALSE;

	ImageList[ID_FLOPPY] = new TBitmap(hInst, ID_FLOPPY);
	ImageList[ID_HDD] = new TBitmap(hInst, ID_HDD);
	ImageList[ID_CDROM] = new TBitmap(hInst, ID_CDROM);
	ImageList[ID_FOLDER] = new TBitmap(hInst, ID_FOLDER);
	ImageList[ID_OPEN_FOLDER] = new TBitmap(hInst, ID_OPEN_FOLDER);
	ImageList[ID_DOCUMENT] = new TBitmap(hInst, ID_DOCUMENT);
	ImageList[ID_EXE] = new TBitmap(hInst, ID_EXE);
	ImageList[ID_CMD] = new TBitmap(hInst, ID_CMD);
	ImageList[ID_BAT] = new TBitmap(hInst, ID_BAT);
	ImageList[ID_COM] = new TBitmap(hInst, ID_COM);

	pfontWork = new TFont(
		 "Helv",                     // facename -- Available by default
		 -(cyTitleBar * 2) / 3,      // height,
		 0, 0, 0, FW_NORMAL,         // width, esc, orientation, weight
		 VARIABLE_PITCH | FF_SWISS,  // Pitch and Family
		 FALSE, FALSE, FALSE,        // Italic, Underline, Strikeout
		 ANSI_CHARSET,               // Charset
		 OUT_CHARACTER_PRECIS,       // Output precision
		 CLIP_DEFAULT_PRECIS,        // Clip precision
		 PROOF_QUALITY               // Quality
	  );

	pfontLB = new TFont(
		 "Helv",                     // facename -- Available by default
		 -8,							     // height,
		 0, 0, 0, FW_NORMAL,         // width, esc, orientation, weight
		 VARIABLE_PITCH | FF_SWISS,  // Pitch and Family
		 FALSE, FALSE, FALSE,        // Italic, Underline, Strikeout
		 ANSI_CHARSET,               // Charset
		 OUT_CHARACTER_PRECIS,       // Output precision
		 CLIP_DEFAULT_PRECIS,        // Clip precision
		 PROOF_QUALITY               // Quality
	  );

	ConnectorPen = new TPen(TColor(128, 128, 128), 1, PS_DOT);
	SolidConnectorPen = new TPen(TColor(128, 128, 128), 1, PS_SOLID);
	MaskBrush = new TBrush(TColor(0, 0, 255));

	rgbLBBackground = WinQuerySysColor(HWND_DESKTOP,
												  SYSCLR_ENTRYFIELD,
												  0);
	bShowAllFileDetails = TRUE;

	bShowTimeStamp = TRUE;
	bShowAttr = TRUE;
	bShowSize = TRUE;

	bDraggingFiles = FALSE;
	bResizingPanes = FALSE;
	bOverResizer = FALSE;

	FolderList = NULL;
	FileList = NULL;

	ulShowAttrBits = 0;

	cxBorder = WinQuerySysValue(HWND_DESKTOP, SV_CXBORDER);
	cyTitleBar = WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR);

	strDirHdr = "All Folders";
	strFileHdr = "";
	strStatus = "No objects selected";

	bFolderOnlyNoSelections = FALSE;

	pStatusText = NULL;

	cxCurrentDir = 0;

	// Retrieve working directory of Warp Cabinet and construct .INI file
	// path accordingly.
	strcpy(szINIPath, "WARPCAB.INI");
	ULONG DiskNum;
	ULONG LogDisk;
	int rc;

	BYTE szBuff[256];
	ULONG cbBytes = 256;
	rc = DosQueryCurrentDisk(&DiskNum, &LogDisk);
	if(rc == 0)
	{
		rc = DosQueryCurrentDir(DiskNum,
										szBuff,
										&cbBytes);
		if(rc == 0)
		{
			szINIPath[0] = DiskNum + 'A' - 1;
			szINIPath[1] = ':';
			szINIPath[2] = '\\';
			szINIPath[3] = 0;
			strcat(szINIPath, szBuff);
			strcat(szINIPath, "\\WARPCAB.INI");
		}
	}

	FILEASSOCIATION *pNewFileAssoc = NULL;

	nTargetIndex = -1;

	// .INI file is WARPCAB.INI
	int nCount = 1;
	while(1)
	{
		pNewFileAssoc = new FILEASSOCIATION;
		if(!pNewFileAssoc)
		{
			MessageBox("Can't load file associations.", "Not Enough Memory", MB_ICONSTOP);
			break;
		}
		rc = pNewFileAssoc->LoadFromProfileFile(HWindow, szINIPath, (LPSTR)PRF_INI_SECTION, nCount);

		if(!rc)   // Break on first fail!
			break;

		// Add to our array...
		FileAssociations.Add(pNewFileAssoc);
		nCount++;
	}

	strcpy(szMask, "*.*");
	strcpy (szSelByMask, "*.*");

	nSortCode = SORT_BY_NAME;
	nOldSortCheckedItem = CM_SORTNAME;

	hwndMenu = NULL;

	pFrame = NULL;

	strcpy(szCurrentListViewDir, "");
	nFileListSelCount = 0;

	cxMaxFileListHorizExt = 0;

	bDraggingFiles = FALSE;
	bFirstDragMovement = FALSE;

	bDragCopy = FALSE; // The default here.

	wLastMouseMsg = -1;
	ptLastMouse.x = -1;
	ptLastMouse.y = -1;

	HighlightPen = new TPen(TColor(0,0,0), 2, PS_DOT);

	rOldRect.Set(-1,-1,-1,-1);

	hptrFile   = WinQuerySysPointer (HWND_DESKTOP, SPTR_FILE, FALSE);
	hptrFolder = WinQuerySysPointer (HWND_DESKTOP, SPTR_FOLDER, FALSE);
	hptrMulti  = WinQuerySysPointer (HWND_DESKTOP, SPTR_MULTFILE, FALSE);

	pGlobalParent = this;
	hwndGlobalParent = HWindow;

	nOpenFileIndex = -1;
}

DFrame::~DFrame()
{
	ClearFolderList();
	delete FolderList;

	ClearFileList();
	delete FileList;

	if(dcMem)
	{
		// Clean-up our memory DC.
		dcMem->RestoreObjects();
		delete dcMem;
	}

	if(pfontWork)
		delete pfontWork;

	if(pfontLB)
		delete pfontLB;

	// Clean-up images.
	delete ImageList[ID_FLOPPY];
	delete ImageList[ID_HDD];
	delete ImageList[ID_CDROM];
	delete ImageList[ID_FOLDER];
	delete ImageList[ID_OPEN_FOLDER];
	delete ImageList[ID_DOCUMENT];
	delete ImageList[ID_EXE];
	delete ImageList[ID_CMD];
	delete ImageList[ID_BAT];
	delete ImageList[ID_COM];

	// Clean-up drawing tools.
	delete ConnectorPen;

	delete SolidConnectorPen;

	delete MaskBrush;

	delete HighlightPen;

	WinDestroyPointer(hptrFile);
	WinDestroyPointer(hptrFolder);
	WinDestroyPointer(hptrMulti);
	WinDestroyPointer(hptrBusy);

	CleanupArrays();

	FolderDragTargets.Flush(TShouldDelete::Delete);

	int rc;
	int bError = FALSE;

	// Save association array and then clean-it up.
	int nCount = 1;

	unlink((LPSTR)szINIPath);

	for(int i = 0; i < FileAssociations.GetItemsInContainer(); i++)
	{
		FILEASSOCIATION *pFileAssoc = FileAssociations[i];
		if(pFileAssoc)
		{
			rc = pFileAssoc->SaveToProfileFile(HWindow, (LPSTR)szINIPath, (LPSTR)PRF_INI_SECTION, nCount);
			if(!rc)
				bError = TRUE;
		}
		else
			bError = TRUE;

		if(bError)
		{
			MessageBox("Can't save file types to .INI file.  Associations may be lost.", "Can't Save File", MB_ICONSTOP);
			break;
		}
		// Otherwise, go to next type.
		nCount++;
	}
	// Flush and kill off records inside array.
	FileAssociations.Flush(TShouldDelete::Delete);
}

void DFrame::ClearFolderList()
{
	int nItems = FolderList->GetCount();

	TREEVIEW_ITEM *tvi;

	for(int i = 0; i < nItems; i++)
	{
		tvi = (TREEVIEW_ITEM *)FolderList->GetItemData(i);
		if(tvi)
		{
			if(tvi->lpszDirText)
				delete[] tvi->lpszDirText;
			delete tvi;
		}
	}
	FolderList->ClearList();
}

void DFrame::ClearFileList()
{
	int nItems = FileList->GetCount();

	FILEENTRY *lvi;

	for(int i = 0; i < nItems; i++)
	{
		lvi = (FILEENTRY *)FileList->GetItemData(i);
		if(lvi)
			delete lvi;
	}

	cxMaxFileListHorizExt = 0;

	FileList->ClearList();
}

BOOL DFrame::IsNextNodeExpandable(LPSTR lpszThisDir)
{
	struct ffblk FileInfo;

	char szBaseDir[256]; // Final '\\' not used.
	char szNewDir[256];
	char szInitSearch[256];

	strcpy(szBaseDir, lpszThisDir);

	strcpy(szInitSearch, szBaseDir);
	strcat(szInitSearch, "\\*.*");

	unsigned long ulShowBits = FA_DIREC | ulShowAttrBits;

	int rc;

	int bFirstTime = TRUE;

	// Check for a sub-directory in this directory.
	// If we have one, we're expandable.
	while(1)
	{
		if(bFirstTime)
		{
			rc = findfirst((LPSTR)szInitSearch, &FileInfo, FA_DIREC);
			bFirstTime = FALSE;
		}
		else
			rc = findnext(&FileInfo);
		if(rc)
			return FALSE;  // No sub-dirs found.

		if((ulShowBits & FileInfo.ff_attrib) == ulShowBits && FileInfo.ff_name[0] != '.')
			return TRUE; // Then a valid sub-dir.  We're expandable!
	}
}

BOOL DFrame::ResortFileEntries(int nNewSortMethod)
{
	int nCount = FileList->GetCount();

	if(nNewSortMethod	== nSortCode || nCount < 2)
		return TRUE;  // Already done.

	// Add FileEntry records back into array to sort them.
	TISArrayAsVector <FILEENTRY> FileEntries(10, 0, 1);

	FILEENTRY *pThisFileEntry;

	ShowWaitCursor();

	FileList->SetRedraw(FALSE);

	FILEENTRY *lvi;

	for(int i = 0; i < nCount; i++)
	{
		lvi = (FILEENTRY *)FileList->GetItemData(i);
		if(lvi)
		{
			lvi->nSortMethod = nNewSortMethod;
			FileEntries.Add(lvi);
		}
	}
	FileList->ClearList();

	for(i = 0; i < FileEntries.GetItemsInContainer(); i++)
	{
		pThisFileEntry = FileEntries[i];
		AddListViewItem(FileList, pThisFileEntry);
	}

	FileList->SetRedraw(TRUE);
	ShowArrowCursor();
	return TRUE;
}

BOOL DFrame::ExpandDirNode(TREEVIEW_ITEM *Parent, int nStartIndex, int nLevel, LPSTR lpszThisDir, LPSTR lpszInitSearch)
{
	int bFirstTime = TRUE;
	int bCanExpand;

	TREEVIEW_ITEM tvi;

	// Use an array of pointers to dir, list items--this is needed to sort items alphabetically here.
	// (Fortunately, for directories, we can always just sort by folder name.
	TISArrayAsVector <string> DirectorySorter (10,0,1);

	int rc;
	int i;

	int nCount = 0;

	int nCurrentLBIndex = nStartIndex;

	struct ffblk FileInfo;

	char szBaseDir[256]; // Final '\\' not used.
	char szNewDir[256];
	char szInitSearch[256];

	unsigned long ulShowBits = FA_DIREC | ulShowAttrBits;

	strcpy((LPSTR)szBaseDir, lpszThisDir);

	strcpy((LPSTR)szInitSearch, szBaseDir);
	strcat(szInitSearch, "\\");
	// No backslash here.
	strcat(szInitSearch, lpszInitSearch);

	bFirstTime = TRUE;

	int nLastAddIndex = -1;

	while(1)
	{
		if(bFirstTime)
		{
			rc = findfirst((LPSTR)szInitSearch, &FileInfo, FA_DIREC);
			bFirstTime = FALSE;
		}
		else
			rc = findnext(&FileInfo);
		if(rc)
			break; // We're done.

		if((ulShowBits & FileInfo.ff_attrib) == ulShowBits && FileInfo.ff_name[0] != '.')
		{
			string *pNewStr = new string(FileInfo.ff_name);
			pNewStr->set_case_sensitive(0);
			DirectorySorter.Add(pNewStr);
		}
	}

	// To here we should have an array of sorted directory strings.
	string *pStr;

	nCount = DirectorySorter.GetItemsInContainer();
	for(i = 0; i < nCount; i++)
	{
		pStr = DirectorySorter[i];

		if(!pStr)
			continue;

		strcpy(szNewDir, szBaseDir);
		strcat(szNewDir, "\\");
		strcat(szNewDir, pStr->c_str());

		// To here, insert new node at provided point in listbox.
		memset(&tvi, 0, sizeof(tvi));

		tvi.nListBoxIndex = nCurrentLBIndex + 1;  // Index # of this entry.
		tvi.nLevel = nLevel;         // In hierarchical list, this is the level of this entry
		bCanExpand = IsNextNodeExpandable(szNewDir);
		if(bCanExpand)
			tvi.nExpandCode = 1;
		else
			tvi.nExpandCode = 0;

		tvi.bIsEndOfLevel = FALSE;
		tvi.Parent = (LPSTR)Parent;
		tvi.iImage = ID_FOLDER;
		tvi.iSelectedImage = ID_OPEN_FOLDER;
		int len = pStr->length();
		if(len == 0)
			tvi.lpszDirText = NULL;
		else
		{
			tvi.lpszDirText = new char[len + 1];
			if(tvi.lpszDirText)
			{
				strcpy(tvi.lpszDirText, pStr->c_str());
				tvi.nTextLen = len;
			}
		}

		nCurrentLBIndex = AddTreeViewItem(FolderList, tvi, nCurrentLBIndex);

		if(nCurrentLBIndex > -1)
			nLastAddIndex = nCurrentLBIndex;
		nCurrentLBIndex++;
	}

	if(nLastAddIndex != -1)
	{
		// Then mark last node in this directory as 'last'.
		TREEVIEW_ITEM *ptvi = (TREEVIEW_ITEM *)FolderList->GetItemData(nLastAddIndex);
		if(ptvi)
			ptvi->bIsEndOfLevel = TRUE;
	}
	DirectorySorter.Flush(TShouldDelete::Delete);
	return TRUE;
}

int DFrame::GetSelectedDir(string& strDir, int nTargetIndex)
{
	// Fills a passed string with a complete path (no final '\') of selected directory.
	strDir = "";

	string strThisSubDir("");
	string strSlash ="\\";
	string strPath("");

	int nIndex;

	if(nTargetIndex == -1)
		nIndex = FolderList->GetSelIndex();
	else
	{
		if(nTargetIndex > FolderList->GetCount() - 1)
			return FALSE;
		nIndex = nTargetIndex;
	}
	if(nIndex < 0)
		return FALSE;

	TREEVIEW_ITEM *tviThis = (TREEVIEW_ITEM *)FolderList->GetItemData(nIndex);

	while(1)
	{
		if(!tviThis)
			break;

		strThisSubDir = tviThis->lpszDirText;

		if(strPath.length() == 0)
			strPath = strThisSubDir;
		else
			strPath = strThisSubDir + strSlash + strPath;

		tviThis = (TREEVIEW_ITEM *)tviThis->Parent;
	}

	if(strPath.length() > 0)
	{
		strDir = strPath;
		return TRUE;
	}
	else
		return FALSE;
}

void DFrame::OnFolderListClick()
{
	// Retrieve current directory.
	string strDir;
	string strNew;

	ClearFileList();

	int rc = GetSelectedDir(strDir);
	if(!rc)
		return;

	ShowWaitCursor();

	rc = LoadDirectoryToFilesLB(FileList, (LPSTR)strDir.c_str(), szMask, bShowHiddenOrSysObjs, nSortCode);

	// Set new prompt info.
	if(rc)
	{
		strNew ="Contents of '";
		strNew += szCurrentListViewDir;
		strNew += "'";
	}
	else
		strNew = "Documents:";

	UpdateFileHdrText((LPSTR)strNew.c_str());

	FileList->SetTopIndex(0);

	// Force a repaint.
	TRect rActualFileList = FileList->GetClientRect();
	FileList->InvalidateRect(rActualFileList);

	if(FileList->GetCount() == 0)
		bFolderOnlyNoSelections = TRUE;
	else
		bFolderOnlyNoSelections = FALSE;

	ShowArrowCursor();
}

int DFrame::LoadDirectoryToFilesLB(TListBox *FileLB, LPSTR lpszDir, LPSTR lpszMask, int bShowHiddenSysFilesToo, int nSortCode)
{
	// Loads contents of specified directory to listbox.
	// Includes support for different sort options and which attributes to show.
	BOOL bFirstTime;
	struct ffblk FileInfo;
	int i, rc;

	// Arrays to hold directories and files in this directory.
	TISArrayAsVector <FILEENTRY> FileEntries(10, 0, 1);

	// Build search directory.
	string strSearchDir = lpszDir;
	strSearchDir += "\\*.*";

	unsigned long ulSearchBits;
	if(bShowHiddenSysFilesToo)
		ulSearchBits = FA_SYSTEM | FA_HIDDEN;
	else
		ulSearchBits = 0;

	// Hunt down all directories and files that match selected attributes.
	bFirstTime = TRUE;
	while(1)
	{
		if(bFirstTime)
		{
			rc = findfirst(strSearchDir.c_str(), &FileInfo, FA_DIREC | ulSearchBits);
			bFirstTime = FALSE;
		}
		else
			rc = findnext(&FileInfo);
		if(rc)
			break;  // We're finished

		// Otherwise we might have a match.
		// Check to see if it's a directory and if so, make sure that
		// if it's hidden, system, these bits are selected as well.
		if((FileInfo.ff_attrib & FA_DIREC) == FA_DIREC && (FileInfo.ff_name[0] != '.'))
		{
			// To here, add this directory name to our array.
			FileEntries.Add(new FILEENTRY(FileInfo, TRUE, nSortCode));
		}
	}

	bFirstTime = TRUE;
	strSearchDir = lpszDir;
	strSearchDir += "\\";
	strSearchDir += lpszMask;

	while(1)
	{
		if(bFirstTime)
		{
			rc = findfirst(strSearchDir.c_str(), &FileInfo, ulSearchBits);
			bFirstTime = FALSE;
		}
		else
			rc = findnext(&FileInfo);
		if(rc)
			break;  // We're finished

		// Otherwise we might have a match.
		// Check to see if it's a directory.  If not, make sure that
		// if its hidden, system or read-only, these bits are selected as well.
		if((FileInfo.ff_attrib & FA_DIREC) == FA_DIREC)
			continue;
		// To here, add this file information to our array.
		// This will be automatically sorted!
		FileEntries.Add(new FILEENTRY(FileInfo, FALSE, nSortCode));
	}

	// Now we have array of directories and files.
	// Add them to our list box.
	string strEntry;
	string strDir;

	FileLB->SetRedraw(FALSE);
	for(i = 0; i < FileEntries.GetItemsInContainer(); i++)
	{
		FILEENTRY *pThisFileEntry = FileEntries[i];
		rc = AddListViewItem(FileList, pThisFileEntry);
	}

	FileLB->SetRedraw(TRUE);

	// Clear arrays
	FileEntries.Flush(TShouldDelete::NoDelete);  // These file entries are attached
																// to our listbox.
	strcpy(szCurrentListViewDir, lpszDir);
	strStatus = "No objects selected";
	if(pStatusText)
	{
		pStatusText->SetText(strStatus.c_str());
	}
	return TRUE;
}

void DFrame::OnFolderListDblClick()
{
	// Process double-click -- Used to expand and collapse nodes in hierarchichal
	// sub-directory list.
	string strNewPath;
	string strBackslash("\\");
	string strThisToken;

	int nIndex;

	TREEVIEW_ITEM *tvi;
	TREEVIEW_ITEM *tviSearch = NULL;

	nIndex = FolderList->GetSelIndex();

	if(nIndex < 0)
		return; // Nothing selected.

	tvi = (TREEVIEW_ITEM *)FolderList->GetItemData(nIndex);
	if(!tvi)
	{
		MessageBox("Bad tree view data.", "Tree View Error", MB_ICONSTOP);
		return;
	}

	ShowWaitCursor();
	if(tvi->nExpandCode == 1)	// Expand this level.
	{
		strNewPath = "";
		tviSearch = tvi;
		while(1)
		{
			if(tviSearch)
			{
				// Build this path...
				strThisToken = tviSearch->lpszDirText;
				if(strThisToken.length())
				{
					// Note path is built backwards, working up the tree to root!
					if(strNewPath.length() > 0)
						strNewPath = strThisToken + strBackslash + strNewPath;
					else
						strNewPath = strThisToken;  // First time, no backslash.
				}
				else
					break;
			}
			else
				break;
			if(tviSearch->Parent)  // Work our way up to root of hierarchical list.
			{
				tviSearch = (TREEVIEW_ITEM *)tviSearch->Parent;
			}
			else
				break;
		}
		if(strNewPath.length() == 0)
		{
			// Can't retrieve path.
			ShowArrowCursor();
			return;
		}
		// Otherwise, go ahead and load these entries.
		FolderList->ShowWindow(SW_HIDE);
		ExpandDirNode(tvi, nIndex + 1, tvi->nLevel + 1, (LPSTR)strNewPath.c_str(), "*.*");

		tvi->nExpandCode = 2; // Mark this entry as expanded.

		FolderList->Invalidate();
		FolderList->ShowWindow(SW_SHOWNORMAL);
	}
	else if(tvi->nExpandCode == 2)	// Collapse this level.
	{
		int nNext = nIndex + 1;
		int nTargetLevel = tvi->nLevel;
		int bLastOne;

		FolderList->ShowWindow(SW_HIDE);
		tviSearch = (TREEVIEW_ITEM *)FolderList->GetItemData(nNext);
		if(!tviSearch)
		{
			ShowArrowCursor();
			return;
		}

		while(1)
		{
			if(!tviSearch)
				break;

			FolderList->DeleteString(nNext);

			if(tviSearch->lpszDirText)
				delete[] tviSearch->lpszDirText;
			delete tviSearch;

			tviSearch = (TREEVIEW_ITEM *)FolderList->GetItemData(nNext);

			if(!tviSearch || tviSearch->nLevel <= nTargetLevel)
				break;
		}
		tvi->nExpandCode = 1; // Mark this entry as expandable.

		FolderList->Invalidate();
		FolderList->ShowWindow(SW_SHOWNORMAL);
	}
	if(nIndex < FolderList->GetCount())
		FolderList->SetSelIndex(nIndex);
	nFileListSelCount = 0;
	ShowArrowCursor();
}

const int MAXENTRIES = 2049;  // 2 KB of selections.

void DFrame::OnFileListClick()
{
	ULONG ulTotalBytes = 0;   // 4GB max should do it.
	ULONG ulTotalKB;

	FILEENTRY *lvi;

	if(!pStatusText)  // Must have a status bar to set!!!
		return;

	strStatus = "";

	// On each click, update total dir.'s and files in status bar.
	int anSelIndexes[MAXENTRIES];

	nFileListSelCount = FileList->GetSelIndexes(anSelIndexes, MAXENTRIES);

	for(int i = 0; i < nFileListSelCount; i++)
	{
		lvi = (FILEENTRY *)FileList->GetItemData(anSelIndexes[i]);
		// Total up our selected files.
		if(lvi)
			ulTotalBytes += (ULONG)lvi->lFileSize;
	}

	if(nFileListSelCount == 0)
	{
		strStatus = "No objects selected";
	}
	else
	{
		char szTemp[40];
		if(nFileListSelCount == 1)
			strStatus = "1 object selected -- ";
		else
		{
			strStatus = itoa(nFileListSelCount, szTemp, 10);
			strStatus += " objects selected -- ";
		}
		ulTotalKB = ulTotalBytes / 1000;
		ltoa(ulTotalKB, szTemp, 10);

		strStatus += szTemp;
		strStatus += " KB";
	}
	pStatusText->SetText(strStatus.c_str());
}

void DFrame::OnFileListDblClick()
{
	char szCmdLine[256];
	char szNewCmdLine[256];
	char szParam[256];

	char szErrorInfo[256];
	RESULTCODES ReturnCodes;
	char szArgs[256];
	int bHasArgs = FALSE;

	int rc;

	static FILEENTRY *lvi;

	// Check for .EXE file or related file.
	int n;
	if(nOpenFileIndex != -1)
		n = nOpenFileIndex;
	else
		n = GetFileListDblClickIndex();  // Important! For multiple-selection listboxes.
	if(n < 0)
		return;
	else
	{
		lvi = (FILEENTRY *)FileList->GetItemData(n);
		if(!lvi)
			return;
	}

	if(lvi->nType == 1)
	{
		// It's a directory--change our open folder to this one.
		int nFolderIndex = FolderList->GetSelIndex();
		int nTargetLevel = -1;
		if(nFolderIndex < 0)
			return;  // Unexpected
		TREEVIEW_ITEM *tvi = (TREEVIEW_ITEM *)FolderList->GetItemData(nFolderIndex);

		if(tvi->nExpandCode == 1)
			OnFolderListDblClick();

		// Only folder name.
		string strTarget = lvi->szFileName;

		for(int i = nFolderIndex + 1; i <= FolderList->GetCount() - 1; i++)
		{
			tvi = (TREEVIEW_ITEM *)FolderList->GetItemData(i);
			if(!tvi || !tvi->lpszDirText)
				continue;
			if(nTargetLevel == -1)
				// If first time through, save our level in tree.
				nTargetLevel = tvi->nLevel;
			else if(nTargetLevel != tvi->nLevel)
				// If level has changed, we didn't find what we need.
				break;
			if(strcmpi(tvi->lpszDirText, (LPSTR)strTarget.c_str()) == 0)
			{
				// A match!
				FolderList->SetSelIndex(i);
				break;
			}
		}
		return;
	}
	else if(lvi->nType != 6)
	{
		if(nFileListSelCount > 1)
			return;

		// Then it's a runnable .EXE/.CMD/.BAT/.COM file...
		if(lvi->nType == 2 || lvi->nType == 5)
		{
			// An .EXE or .COM file -- just run this file.
			strcpy(szCmdLine, szCurrentListViewDir);
			strcat(szCmdLine, "\\");
			strcat(szCmdLine, lvi->szFileName);
		}
		else if(lvi->nType == 3)
		{
			// A .CMD file -- run with CMD.EXE
			strcpy(szCmdLine, "CMD.EXE");

			strcpy(szArgs, "/K ");
			strcat(szArgs, szCurrentListViewDir);
			strcat(szArgs, "\\");
			strcat(szArgs, lvi->szFileName);

			bHasArgs = TRUE;
		}
		else if(lvi->nType == 4)
		{
			// A BAT file -- run with COMMAND.COM
			strcpy(szCmdLine, "COMMAND.COM");

			strcpy(szArgs, szCurrentListViewDir);
			strcat(szArgs, "\\");
			strcat(szArgs, lvi->szFileName);

			bHasArgs = TRUE;
		}
		else
		{
			return;  // Undefined.
		}
		rc = RunProgramWithArgs(szCmdLine,
										bHasArgs ? szArgs : NULL);

		if(!rc)
		{
			if(lvi->nType == 2)
			{
				// Allow Windows 3.1 .EXE's to run too...
				rc = MessageBox("Can't run this program as a DOS or OS/2 executable.  Try to run as a WIN-OS/2 application instead?.", "Can't Run", MB_ICONQUESTION | MB_YESNOCANCEL);
				if(rc == IDYES)
				{
					strcpy(szArgs, "/K progman.exe ");
					strcat(szArgs, szCmdLine);

					rc = RunProgramWithArgs("CMD.EXE", szArgs);
					if(!rc)
						MessageBox("There was an error running this program.  Check that path is correct or close some programs to free up memory and try again.", "Can't Run", MB_ICONSTOP);
				}
			}
			else
				MessageBox("There was an error running this program.  Check that path is correct or close some programs to free up memory and try again.", "Can't Run", MB_ICONSTOP);

		}
		return;
	}
	else
	{
		// Then it's a document (== 6)--Run it using the list of associations.
		if(nFileListSelCount > 1)
			return;

		FILEASSOCIATION *pThisFileAssoc = FileAssociations.FirstThat(FindFileAssociation, lvi->szExt);

		if(!pThisFileAssoc)
		{
			// No association found.
			string strError;
			strError = "There is currently no program associated with the '";
			strError += lvi->szExt;
			strError += "' file extension.  Use the 'File | Associate' menu option and try again.";
			MessageBox( strError.c_str(), "Unknown File Type", MB_ICONSTOP);
			return;
		}

		// Otherwise build a program + document command line.
		strcpy(szCmdLine, pThisFileAssoc->szEXEPathName);

		strcpy(szArgs, szCurrentListViewDir);
		strcat(szArgs, "\\");
		strcat(szArgs, lvi->szFileName);

		rc = RunProgramWithArgs(szCmdLine, szArgs);
		if(!rc)
			MessageBox("There was an error running this program.  Check that path is correct or close some programs to free up memory and try again.", "Can't Run", MB_ICONSTOP);
		return;
	}
}

void DFrame::SetupWindow()
{
	// Create listboxes
	FolderList = new CustomListBox(this, IDC_DIRLIST,
							rFolderList.left,
							rFolderList.top,
							rFolderList.right - rFolderList.left,
							rFolderList.bottom - rFolderList.top);

	FileList = new CustomListBox(this, IDC_FILELIST,
									rFileList.left,
									rFileList.top,
									rFileList.right - rFileList.left,
									rFileList.bottom - rFileList.top);

	FolderList->Attr.Style &= (LBS_SORT ^ 0xFFFFFFF); // Clear sort bit
	FolderList->Attr.Style |= LBS_OWNERDRAWFIXED;  // Make this 'ownerdraw'.

	FileList->Attr.Style &= (LBS_SORT ^ 0xFFFFFFF); // Clear sort bit
	FileList->Attr.Style |= LBS_MULTIPLESEL;
	FileList->Attr.Style |= LBS_OWNERDRAWFIXED;  // Make this 'ownerdraw'.

	TClientDC dcScreen(HWindow);

	dcMem = new TMemoryDC(dcScreen);

	// Positions to put info. in list box
	dcScreen.SelectObject(*pfontLB);

	TSize sizeText = dcScreen.GetTextExtent("XXXXX",5);

	cxName = sizeText.cx * 6;
	cxSize = sizeText.cx * 2;
	cxTimeStamp = sizeText.cx * 3 + sizeText.cx / 2;
	cxAttribute = sizeText.cx / 3;

	TWindow::SetupWindow();

	dcScreen.RestoreObjects();

	LoadInitDirs();

	pGlobalParent = this;

	nOldSortCheckedItem = CM_SORTNAME;
}

void DFrame::LoadInitDirs(void)
{
	char szTemp[8];

	TREEVIEW_ITEM tvi;
	tvi.nListBoxIndex = 0;  // Index # of this entry.
	tvi.nLevel = 1;         // In hierarchical list, this is the level of this entry
	tvi.nExpandCode = 1;    // 0 = Nothing to expand, 1 == Can Expand [+], 2 Already Expanded [-]
	tvi.bIsEndOfLevel = FALSE;
	tvi.Parent = NULL;
	tvi.lpszDirText = szTemp;
	tvi.nTextLen = 2;

	int nLastIndex = -1;

	// Go through possible letters for drives.
	int nTotalFound = 0;
	char szCurDir[257];
	char szTestFile[257];

	char szTestDir[10];

	int bUseCDImage = FALSE;

	int rc;
	char ch = 0;

	for(int i = 1; i < 26; i++)
	{
		// Turn off that ugly system message for not finding a disk (usually the A:/B: drives).
		DosError(FERR_DISABLEHARDERR);
		rc = getcurdir(i, szCurDir);
		if(rc == 0)
		{
			nTotalFound++;
			ch= 'A' + i - 1;
			tvi.nListBoxIndex = nTotalFound;  // Index # of this entry.

			if(ch == 'A' || ch == 'B')
			{
				tvi.iImage = ID_FLOPPY;
				tvi.iSelectedImage = ID_FLOPPY;
			}
			else
			{
				// Check for CD-ROM drive.
				char szDeviceName[8];
				szDeviceName[0] = ch;
				szDeviceName[1] = ':';
				szDeviceName[2] = 0;

				ULONG cbBuff;

				BYTE FSInfo[sizeof(FSQBUFFER2) + 3 * 256];
				PFSQBUFFER2  pfsqBuffer = (PFSQBUFFER2) FSInfo;
				rc = DosQueryFSAttach(
					 szDeviceName,
					 0,
					 1,  // FSAIL_QUERYNAME
					 pfsqBuffer,
					 &cbBuff);

				bUseCDImage = FALSE;
				if(rc == 0)
				{
					// Offset is 11...
					if(strcmpi(&FSInfo[11], "CDFS") == 0)
						bUseCDImage = TRUE;
				}
				if(bUseCDImage)
				{
					tvi.iImage = ID_CDROM;
					tvi.iSelectedImage = ID_CDROM;
				}
				else
				{
					tvi.iImage = ID_HDD;
					tvi.iSelectedImage = ID_HDD;
				}
			}

			szTemp[0] = ch;
			szTemp[1] = ':';
			szTemp[2] = 0;

			tvi.lpszDirText = new char[3];
			strcpy(tvi.lpszDirText, szTemp);

			tvi.nExpandCode = IsNextNodeExpandable(tvi.lpszDirText);

			nLastIndex = AddTreeViewItem(FolderList, tvi);
		}
	}

	// Mark last index as last item in root of hierarchy.
	if(nLastIndex > -1)
	{
		TREEVIEW_ITEM *pItem = (TREEVIEW_ITEM *)FolderList->GetItemData(nLastIndex);
		if(pItem)
			pItem->bIsEndOfLevel = TRUE;
	}

	// Re-paint our list.
	FolderList->Invalidate();
}

void DFrame::UpdateFileHdrText(LPSTR lpszNewText)
{
	if(lpszNewText)
		strFileHdr = lpszNewText;

	// Otherwise, just paint.
	TClientDC dc(HWindow);

	if(pfontWork)
		dc.SelectObject(*pfontWork);

	dc.ExtTextOut(rFileListHdr.left, rFileListHdr.top, ETO_CLIPPED | ETO_OPAQUE,
					  &rFileListHdr, strFileHdr.c_str(), strFileHdr.length(), NULL);
}

void DFrame::Paint(TDC& dc, BOOL erase, TRect& rect)
{
	TWindow::Paint(dc, erase, rect);

	if(pfontWork)
		dc.SelectObject(*pfontWork);

	// Paint Dir. Header
	dc.ExtTextOut(rFolderListHdr.left, rFolderListHdr.top, ETO_CLIPPED | ETO_OPAQUE,
					  &rFolderListHdr, strDirHdr.c_str(), strDirHdr.length(), NULL);

	// Paint Dir. Listbox
	FolderList->Invalidate(TRUE);

	// Paint Files Header
	dc.ExtTextOut(rFileListHdr.left, rFileListHdr.top, ETO_CLIPPED | ETO_OPAQUE,
					  &rFileListHdr, strFileHdr.c_str(), strFileHdr.length(), NULL);

	// Paint Files Listbox
	FileList->Invalidate(TRUE);

	// Update status bar.
	if(pStatusText)
		pStatusText->SetText(strStatus.c_str());

	dc.RestoreObjects();
}

void DFrame::DrawImage(TDC& dc, int nImage, int x, int y, UINT dwROP, BOOL bUseMask)
{
	// Renders image (a bitmap) to selected DC.
	if(nImage < 1 || nImage > MAXIMAGES - 1 || !dcMem)
	{
#ifdef __DEBUG
		MessageBox("Bad data in DrawImage()", "Error", MB_OK);
#endif
		return;
	}

	UINT dwUseROP = dwROP;

	TBitmap *pImage = ImageList[nImage];

	if(!pImage)
	{
#ifdef __DEBUG
		char szMsg[40];
		sprintf(szMsg, "Bad image index in DrawImage() = %d", nImage);
		MessageBox(szMsg, "Error", MB_OK);
#endif
		return;
	}
	dcMem->SelectObject(*ImageList[nImage]);

	if(bUseMask)
	{
		dc.SelectObject(*MaskBrush);
		dwUseROP = SRCCOPY;
	}

	dc.BitBlt(x, y, CXIMAGE, CYIMAGE, *dcMem, 0, 0, dwUseROP);
}

void DFrame::DrawSplitterWindowBar(TDC& dc, TRect& rect)
{
	dc.SelectStockObject(BLACK_PEN);
	dc.SelectStockObject(BLACK_BRUSH);

	dc.SetROP2(R2_NOT);

	dc.Rectangle(rect);

	dc.RestoreObjects();
}

void DFrame::EvLButtonDown(UINT keys, TPoint& point)
{
	TPoint ptMouse;

	if(rSplitter.Contains(point))
	{
		// Start splitter re-sizing operation.
		SetCapture();
		bResizingPanes = TRUE;
		bOverResizer = FALSE;

		TClientDC dc(HWindow);

		// Don't allow splitter to move all the way left or right.
		point.x = max(point.x, cyTitleBar);
		point.x = min(point.x, cxWin - 2 * cyTitleBar);

		rOldSplitter.Set(point.x - 3, rSplitter.top, point.x + 6 * cxBorder, rSplitter.bottom);

		DrawSplitterWindowBar(dc, rOldSplitter);
	}
	else
	{
		// Check if we're inside FileList folder rectangle for possible dragging.
		GetCursorPos(ptMouse);
		ScreenToClient(ptMouse);

		nFileListSelCount = FileList->GetSelCount();

		if(nFileListSelCount > 0 && rFileList.Contains(ptMouse))
		{
			if(nMouseBtnDownCode == NO_BTN)
			{
				nMouseBtnDownCode = LEFT_BTN;
				FileList->SetForwarding(TRUE);
				FolderList->SetForwarding(TRUE);
			}

			if((bDragLeftBtn && nMouseBtnDownCode == RIGHT_BTN) ||
				(!bDragLeftBtn && nMouseBtnDownCode == LEFT_BTN))
			{
				// Then button prefs. don't match!
				nMouseBtnDownCode = NO_BTN;
				return;
			}

			bDragCopy = keys & CTRL_MOUSE_KEY ? TRUE : FALSE;

			SetCapture();
			bDraggingFiles = TRUE;
			bFirstDragMovement = TRUE;

			// Calculate possible targets--hit test triangles and their indexes in
			//    FolderList.
			int nTopIndex = FolderList->GetTopIndex();

			FolderDragTargets.Flush(TShouldDelete::Delete);

			TRect rItem;
			int rc;

			nTargetIndex = -1;

			TRect rActualFolder = FolderList->GetClientRect();
			int nCount = 0;
			// Collect hit test rectangles in targets
			int nItems = FolderList->GetCount();
			for(int i = nTopIndex; i < nItems; i++)
			{
				rItem.Set(rActualFolder.left,
							 rActualFolder.top + nCount * CYLBHEIGHT,
							 rActualFolder.right,
							 rActualFolder.top + ((nCount + 1) * CYLBHEIGHT));
				FolderDragTargets.Add(new LBHITTEST(i, rItem));
				nCount++;
			}
			rOldRect.Set(-1,-1,-1,-1);
		}
		else if(rFolderList.Contains(ptMouse))
		{
			// Check for click over [+] or [-] rectangle.
			// This is an analogous operation to a double-click!!!
			int nTopIndex = FolderList->GetTopIndex();

			GetCursorPos(ptMouse);
			FolderList->ScreenToClient(ptMouse);

			TRect rExpandBox;
			TRect rActualFolder = FolderList->GetClientRect();
			// Collect hit test rectangles in targets
			int nItems = FolderList->GetCount();
			int nCount = 0;
			for(int i = nTopIndex; i < nItems; i++)
			{
				// Check for all possible entries in this FolderList.
				// First, check for [+] or [-]
				TREEVIEW_ITEM *tvi = (TREEVIEW_ITEM *) FolderList->GetItemData(i);
				if(!tvi || tvi->nExpandCode == 0)
				{
					nCount++;
					continue;  // Go to next entry.
				}
				int cxStart = 0;
				for(int ThisLevel = 0; ThisLevel < tvi->nLevel; ThisLevel++)
				{
					if(ThisLevel == 0)
						cxStart = rActualFolder.left + 9;
					else
						cxStart += 20;
				}

				if(cxStart == 0)
					cxStart = rActualFolder.left + 9;

				// To here create our hit-test rectangle.
				int cyMid = rActualFolder.top + nCount * CYLBHEIGHT + (CYLBHEIGHT / 2);

				rExpandBox.Set(cxStart - 4,
									cyMid - 4,
									cxStart + 7,
									cyMid + 7);

				if(rExpandBox.Contains(ptMouse))
				{
					// Simulate a double-click on this control.
					FolderList->SetSelIndex(i);
					OnFolderListDblClick();
					return;
				}
				nCount++;
			}
		}
	}
}

void DFrame::EvMouseMove(UINT keys, TPoint& point)
{
	TPoint pt;
	GetCursorPos(pt);
	ScreenToClient(pt);

	TClientDC *pDC;

	if(bDraggingFiles)
	{
		TPoint ptMouse;
		GetCursorPos(ptMouse);

		bDragCopy = keys & CTRL_MOUSE_KEY ? TRUE : FALSE;

		// Check that we're inside DFrame
		TRect r;
		GetWindowRect(r);

		if(!r.Contains(ptMouse)) // If we're outside our frame initialize remote dragging.
		{

			if(rOldRect.left != -1)
			{
				pDC = new TClientDC(FolderList->HWindow);
				pDC->SetROP2(R2_NOT);
				pDC->SelectStockObject(NULL_BRUSH);
				pDC->SelectObject(*HighlightPen);

				if(rOldRect.left != -1)
					pDC->Rectangle(rOldRect);

				pDC->RestoreObjects();

				delete pDC;

				rOldRect.Set(-1,-1,-1,-1);  // Nothing to erase next time.
			}

			nTargetIndex = -1;  // Not a local copy -- assume a remote copy to other OS/2 desktop objects.

			if(!bRemoteDraggingInit)
			{
				// Start 'remote' dragging process--dragging to OS/2 desktop...
				int nFiles = GetFileSelections();
				if(!nFiles)
					return;

				pGlobalParent = this;

				int nCount = CollectAllFilesInSelections(szCurrentListViewDir, szMask, ulShowAttrBits, szCurrentListViewDir);
				if(nCount == 0)
				{
					MessageBox("Select some files first!", "No Files Selected", MB_ICONEXCLAMATION);
					return;
				}

				ReleaseCapture();
				int rc = InitDrag (WinQueryAnchorBlock(HWindow), HWindow, nCount, hptrFile,
								hptrFolder, hptrMulti, szCurrentListViewDir);
				if(rc)
				{
					bDraggingFiles = FALSE;
					OnFolderListClick();
					ShowArrowCursor();
				}
				else
					SetCapture();
			}
		}
		else
		{
			FolderList->ScreenToClient(ptMouse);

			// Drag files support.

			// Change cursor to appropriate pointer/cursor
			SetCursor(GetModule(), bDragCopy ? ID_COPY_DOCS : ID_MOVE_DOCS);

			bFirstDragMovement = FALSE;

			TPoint ptMouse;
			GetCursorPos(ptMouse);
			FolderList->ScreenToClient(ptMouse);

			LBHITTEST *pTestIt;

			pDC = new TClientDC(FolderList->HWindow);
			pDC->SetROP2(R2_NOT);
			pDC->SelectStockObject(NULL_BRUSH);
			pDC->SelectObject(*HighlightPen);

			nTargetIndex = -1;  // Cancel out old target index.

			if(rOldRect.left != -1)
				pDC->Rectangle(rOldRect);

			for(int i = 0; i < FolderDragTargets.GetItemsInContainer(); i++)
			{
				pTestIt = FolderDragTargets[i];
				if(!pTestIt)
					continue;

				if(pTestIt->rect.Contains(ptMouse))
				{
					// A match! -- highlight this rectangle.
					// Unselect old rectangle first!
					pDC->Rectangle(pTestIt->rect);
					rOldRect = pTestIt->rect;
					nTargetIndex = pTestIt->nIndex;
					break;
				}
			}
			if(nTargetIndex == -1)
				rOldRect.Set(-1,-1,-1,-1);  // Nothing to erase next time.

			pDC->RestoreObjects();

			delete pDC;
		}
	}
	else if(!bOverResizer)
	{
		// Check for hit-test over 'splitter' area.
		if(rSplitter.Contains(point))
		{
			SetCursor(0, IDC_SIZEWE);
			bOverResizer = TRUE;
		}
	}
	else if(bOverResizer)
	{
		if(bResizingPanes)  // Then set cursor to re-size in all cases.
								  // Update splitter bar.
		{
			SetCursor(0, IDC_SIZEWE);

			TClientDC dc(HWindow);

			// Erase bar at previous position
			DrawSplitterWindowBar(dc, rOldSplitter);

			// In all cases, update splitter window and save old position

			// Don't allow splitter to move all the way left or right.
			if(pt.x < 0) // Special case, mouse left of window.
				point.x = cyTitleBar;
			else
			{
				point.x = max(point.x, cyTitleBar);
				point.x = min(point.x, cxWin - 2 * cyTitleBar);
			}

			rOldSplitter.Set(point.x - 3, rSplitter.top, point.x + 6 * cxBorder, rSplitter.bottom);

			DrawSplitterWindowBar(dc, rOldSplitter);
		}
		else
		{
			// Check for hit-test over 'splitter' area.
			if(rSplitter.Contains(point))
			{
				SetCursor(0, IDC_SIZEWE);
				bOverResizer = TRUE;
			}
			else
			{
				// Then we're no longer over splitter area.
				ShowArrowCursor();
				bOverResizer = FALSE;
			}
		}
	}
	else
	{
		SetCursor(0, IDC_ARROW);
		bOverResizer = FALSE;
	}
}

void DFrame::EvRButtonUp(UINT keys, TPoint& point)
{
	nMouseBtnDownCode = NO_BTN;
	bCouldDrag = FALSE;

	EvLButtonUp(keys, point);

	FileList->SetForwarding(FALSE);
	FolderList->SetForwarding(FALSE);

	bLocalDragging = FALSE;
	bRemoteDragging = FALSE;
	bRemoteDraggingInit = FALSE;

	keys; point; // Kill warnings.
}

void DFrame::EvLButtonUp(UINT keys, TPoint& point)
{
	TClientDC *dcFolder = NULL;

	if(bResizingPanes)
	{
		// End splitter re-sizing operation.
		ReleaseCapture();
		bResizingPanes = FALSE;
		bOverResizer = FALSE;

		// Re-draw both headers and list boxes with new size, if we've moved.
		TClientDC dc(HWindow);

		// Erase bar at previous position
		DrawSplitterWindowBar(dc, rOldSplitter);

		cxCurrentDir = rOldSplitter.left + 5;

		// Simulate a re-size operation.  This re-paints us with new dimensions.
		TSize size(cxWin, cyWin);
		EvSize(0, size);
	}
	else if(bDraggingFiles)
	{
		// Process drag over to folder, if one is selected.
		// Assumes bDragCopy is previously set (it should be).

		// Clean-up for next time.
		ReleaseCapture();
		ShowArrowCursor();
		bDraggingFiles = FALSE;

		int bFilesCouldHaveChanged = FALSE;

		FolderDragTargets.Flush(TShouldDelete::Delete);

		if(nTargetIndex != -1)
		{
			int rc;
			while(1)  // For flow control
			{
				string strTargetDir;

				rc = GetSelectedDir(strTargetDir, nTargetIndex);
				if(!rc)
					break;

				// Now get selected files.
				int n = GetFileSelections();
				if(n == 0)
					break;

				if(strTargetDir == szCurrentListViewDir)
				{
					MessageBox("Source and target directories are the same.",
									bDragCopy ? "Can't Copy Files" : "Can't Move Files",
									MB_ICONEXCLAMATION);
					break;
				}

				char szFullTarget[256];
				char szTargetDir[256];

				string strTestDir;

				string strAll("*.*");
				size_t pos = strTargetDir.find_first_of(strAll);

				if(pos != NPOS)
					strTargetDir = strTargetDir.substr(0, pos - 1);

				ShowWaitCursor();

				pStatusText->SetText("Computing total size...");

				int nCount = CollectAllFilesInSelections((LPSTR)strTargetDir.c_str(), szMask, ulShowAttrBits, szCurrentListViewDir);
				if(nCount == 0)
				{
					MessageBox("Select some files first!", "No Files Selected", MB_ICONEXCLAMATION);
					break;
				}

				ShowArrowCursor();
				pStatusText->SetText("");

				static FILEOPPARAM FileOpParam;
				FileOpParam.pStatus = pGlobalStatusBar;
				FileOpParam.pParent = pGlobalParent;
				FileOpParam.lpszSourceDir = szCurrentListViewDir;
				FileOpParam.lpszTargetDir = (LPSTR)strTargetDir.c_str();
				static char szNoFile[] = "";
				FileOpParam.lpszTargetFile = szNoFile; // Not used here.

				string strConfirm("Are you sure you want to ");

				string strPlural("");

				char szTotal[20];

				if((bDragCopy && bConfirmCopy) || (!bDragCopy && bConfirmDelete) || bConfirmAll)
				{
					itoa(nCount, szTotal, 10);

					if(nCount == 1)
						strPlural = "";
					else
						strPlural = "s";

					// Load up our text.
					if(bDragCopy)
						strConfirm += "copy ";
					else
						strConfirm += "move ";

					strConfirm += szTotal;
					strConfirm += " object";
					strConfirm += strPlural;
					strConfirm += " (";
					strConfirm += GetFileSizeAsStr(ulTotalFilesBytes);
					strConfirm += ") to ";
					strConfirm += strTargetDir;
					strConfirm += "?";
					rc = MessageBox(strConfirm.c_str(),
										 bDragCopy ? "Confirm Copy" : "Confirm Move",
										 MB_YESNOCANCEL);
					if(rc != IDYES)
					{
						CleanupArrays();
						break;
					}
				}

				bFilesCouldHaveChanged = TRUE;

				FileOpParam.lpszTargetDir = (LPSTR)strTargetDir.c_str();
				FileOpParam.nFileOp = bDragCopy ? CM_COPY : CM_MOVE;

				rc = CopyOrMoveFiles(&FileOpParam);
				if(!rc)
					MessageBox(bDragCopy ? "Can't copy files!" : "Can't move files!",
								  "File Operation Failed", MB_ICONSTOP);
				CleanupArrays();

				if(pStatusText)
					pStatusText->SetText("");

				break;  // In all cases exit our 'dummy' while loop.
			}
		}
		if(rOldRect.left != -1)
		{
			// Erase last highlighted rectangle.
			dcFolder = new TClientDC(FolderList->HWindow);
			dcFolder->SetROP2(R2_NOT);
			dcFolder->SelectStockObject(NULL_BRUSH);
			dcFolder->SelectObject(*HighlightPen);
			dcFolder->Rectangle(rOldRect);
			dcFolder->RestoreObjects();
			delete dcFolder;
		}
		if(bFilesCouldHaveChanged)
			// Refresh files list as necessary.
			OnFolderListClick();
	}
	else
	{
		bResizingPanes = FALSE;
		bOverResizer = FALSE;
	}

	nMouseBtnDownCode = NO_BTN;
	bCouldDrag = FALSE;

	bLocalDragging = FALSE;
	bRemoteDragging = FALSE;
	bRemoteDraggingInit = FALSE;

	keys; point; //  Kill warnings;
}

void DFrame::CmEnableOpen(TCommandEnabler& ce)
{
	// Enable/disable Open option.
	// Open option requires that only one file be selected.
	if(nFileListSelCount == 1)
		ce.Enable(TRUE);
	else
		ce.Enable(FALSE);
}

void DFrame::CmEnableMove(TCommandEnabler& ce)
{
	if(nFileListSelCount > 0)
		ce.Enable(TRUE);
	else
		ce.Enable(FALSE);
}

void DFrame::CmEnableCopy(TCommandEnabler& ce)
{
	if(nFileListSelCount > 0)
		ce.Enable(TRUE);
	else
		ce.Enable(FALSE);
}

void DFrame::CmEnableDelete(TCommandEnabler& ce)
{
	int nIndex = FolderList->GetSelIndex();
	if(nIndex > -1)
		ce.Enable(TRUE);
	else
		ce.Enable(FALSE);
}

void DFrame::CmEnableProperties(TCommandEnabler& ce)
{
	if(nFileListSelCount > 0)
		ce.Enable(TRUE);
	else
		ce.Enable(FALSE);
}

void DFrame::CmOpen()
{
	// Run currently selected (only one) file.
	int anSelIndexes[MAXENTRIES];
	int nCount = FileList->GetSelIndexes(anSelIndexes, MAXENTRIES);
	if(nCount > 0)
	{
		nOpenFileIndex = anSelIndexes[0];
		OnFileListDblClick();
		nOpenFileIndex = -1;
	}
}

void DFrame::CmMove()
{
	int n = GetFileSelections();

	if(n == 0)
		return;

	DFileChangeDlg MoveDlg(this, szCurrentListViewDir, CM_MOVE);
	MoveDlg.Execute();
	CmRefreshOneFolder();
}

void DFrame::CmCopy()
{
	int n = GetFileSelections();

	if(n == 0)
		return;

	DFileChangeDlg CopyDlg(this, szCurrentListViewDir, CM_COPY);
	CopyDlg.Execute();
	CmRefreshOneFolder();
}

void DFrame::CmDelete()
{
	int rc;
	TREEVIEW_ITEM *tvi;

	if(bFolderOnlyNoSelections && strlen(szCurrentListViewDir) > 2)   // Not root.
	{
		int nIndex = FolderList->GetSelIndex();
		if(nIndex < 0)
			return;
		string strMsg = "Remove empty directory ";
		strMsg += szCurrentListViewDir;
		rc = MessageBox((LPSTR)strMsg.c_str(), "Confirm Delete", MB_YESNOCANCEL);
		if(rc != IDYES)
			return;
		char szRootDir[8];
		szRootDir[0] = szCurrentListViewDir[0];
		szRootDir[1] = '\\';
		szRootDir[2] = ';';
		szRootDir[3] = 0;
		chdir(szRootDir);
		rc = DosDeleteDir(szCurrentListViewDir);
		if(rc)
			MessageBox("Unable to remove this directory.", "Can't Remove Directory", MB_ICONEXCLAMATION);
		else
		{
			if(nIndex > 0)
			{
				string strNewDir;
				int nNewIndex = nIndex - 1;
				tvi = (TREEVIEW_ITEM *)FolderList->GetItemData(nIndex);
				if(!tvi)
				{
					CmRefreshAllFolders();
					return;
				}
				int nOldLevel = tvi->nLevel;

				FolderList->SetSelIndex(nNewIndex);
				tvi = (TREEVIEW_ITEM *)FolderList->GetItemData(nNewIndex);
				if(!tvi)
				{
					CmRefreshAllFolders();
					return;
				}

				FolderList->DeleteString(nIndex);

				if(tvi->nLevel < nOldLevel)
				{
					rc = GetSelectedDir(strNewDir, nNewIndex);
					if(rc)
					{
						rc = IsNextNodeExpandable((LPSTR)strNewDir.c_str());
						if(!rc)
						{
							// This was the last entry in an expandable branch.
							// Mark it as no longer expandable and reload this instance.
							if(tvi->nExpandCode == 1 || tvi->nExpandCode == 2)
							{
								tvi->nExpandCode = 0;
								FolderList->DeleteString(nNewIndex);
								AddTreeViewItem(FolderList, *tvi, nNewIndex);
							}
						}
						FolderList->SetSelIndex(nNewIndex);
					}
					else
					{
						CmRefreshAllFolders();
						return;
					}
				}
				else
					CmRefreshAllFolders();
			}
			else
				CmRefreshAllFolders();
		}
	}
	int n = GetFileSelections();

	if(n == 0)
		return;

	DFileChangeDlg DeleteDlg(this,  szCurrentListViewDir, CM_DELETE);
	rc = DeleteDlg.Execute();
	CmRefreshOneFolder();
}

void DFrame::CmRun()
{
	int nSelCount = GetFileSelections();

	FILEENTRY *pFileEntry = NULL;

	static char szNothing[2] = "";

	LPSTR lpszMyInitCmd = szNothing;

	if(nSelCount > 0)
	{
		pFileEntry = (FILEENTRY *)SelectedFileList[0];
		if(pFileEntry)
			lpszMyInitCmd = pFileEntry->szFileName;
	}

	DRunCmdDlg RunCommandDlg(this, szCurrentListViewDir, lpszMyInitCmd);

	RunCommandDlg.Execute();
}

void DFrame::CmProperties()
{
	int n = GetFileSelections();
	if(n == 0)
		return;

	DFilePropertiesDlg SetFilePropertiesDlg(this, szCurrentListViewDir);
	SetFilePropertiesDlg.Execute();
	OnFolderListClick();
}

void DFrame::CmCreateDir()
{
	DCreateDirDlg CreateNewDirDlg(this, szCurrentListViewDir);

	CreateNewDirDlg.Execute();
	CmRefreshOneFolder();
}

void DFrame::CmAssociate()
{
	char szNewExt[4];

	szNewExt[0] = 0;

	// Show 'File Associations' dialog box.
	DAssociationsDlg FileAssocDlg(this, &FileAssociations, szNewExt);
	FileAssocDlg.Execute();
}

void DFrame::CmExit()
{
	if(pFrame)
		pFrame->CloseWindow();   // Close our application.
}

void DFrame::CmSelectFiles()
{
	// Allow user to specify a wildcard and choose files in current directory.
	if(strlen(szCurrentListViewDir) == 0)
		return;  // Must have some files.

	DSelFilesDlg SelectFilesDlg(this, szSelByMask);
	int rc = SelectFilesDlg.Execute();
	if(rc)
	{
		int bDeselecting = rc == -5 ? TRUE : FALSE;

		FILEENTRY *pFileEntry = NULL;

		string strTestIt;
		string strNew;

		strTestIt.set_case_sensitive(0);
		strNew.set_case_sensitive(0);

		string strSelByMask(szSelByMask);
		string strErr;

		int bIsSelected;

		ShowWaitCursor();

		// Then walk through current file list and select all files
		// that match this pattern.
		for(int i = 0; i < FileList->GetCount(); i++)
		{
			pFileEntry = (FILEENTRY *)FileList->GetItemData(i);
			if(!pFileEntry || pFileEntry->nType == 1)  // Skip bad entries and directories.
				continue;

			strTestIt = "X:\\";   // Dummy fully qualified path--this is needed for the upcoming call.
			strTestIt += pFileEntry->szFileName;

			rc = MakeNewSourcePathWithWildcardMask(strNew, strTestIt, strSelByMask, strErr);
			if(rc && (strNew == strTestIt))
				FileList->SetSel(i, !bDeselecting);  // Then select or de-select this file.
		}
		ShowArrowCursor();
	}
}

void DFrame::CmSelectAll()
{
	int nCount = FileList->GetCount();

	FileList->SetSelItemRange(TRUE, 0, nCount - 1);
}

void DFrame::CmSelectNone()
{
	int nCount = FileList->GetCount();

	FileList->SetSelItemRange(FALSE, 0, nCount - 1);
}

void DFrame::CmShowName()
{
	if(pFrame)
	{
		hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
		WinCheckMenuItem(hwndMenu, CM_FILE_NAME, TRUE);
		WinCheckMenuItem(hwndMenu, CM_ALL_DETAILS, FALSE);
		WinCheckMenuItem(hwndMenu, CM_PARTIAL_DETAILS, FALSE);
	}
	// Then show file details.
	bShowAllFileDetails = FALSE;

	bShowSize = FALSE;
	bShowTimeStamp = FALSE;
	bShowAttr = FALSE;

	OnFolderListClick();
}

void DFrame::CmAllFileDetails()
{
	if(pFrame)
	{
		hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
		WinCheckMenuItem(hwndMenu, CM_FILE_NAME, FALSE);
		WinCheckMenuItem(hwndMenu, CM_ALL_DETAILS, TRUE);
		WinCheckMenuItem(hwndMenu, CM_PARTIAL_DETAILS, FALSE);
	}

	bShowSize = TRUE;
	bShowTimeStamp = TRUE;
	bShowAttr = TRUE;

	OnFolderListClick();
}

void DFrame::CmPartialFileDetails()
{
	// Show partial file options dialog box.
	DFileDetailsDlg FileDetailsDlg(this, &bShowTimeStamp,
					&bShowAttr, &bShowSize);
	int rc = FileDetailsDlg.Execute();

	if(rc)
	{
		if(pFrame)
		{
			hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
			WinCheckMenuItem(hwndMenu, CM_FILE_NAME, FALSE);
			WinCheckMenuItem(hwndMenu, CM_ALL_DETAILS, FALSE);
			WinCheckMenuItem(hwndMenu, CM_PARTIAL_DETAILS, TRUE);
		}
		// Then repaint our dialog with new info.
		FileList->Invalidate();
	}
}

void DFrame::CmRefreshOneFolder()
{
	OnFolderListClick();
}

void DFrame::CmRefreshAllFolders()
{
	// Clear and re-load.
	ClearFolderList();
	ClearFileList();

	// Clear and re-load.
	LoadInitDirs();
}

void DFrame::CmDragWithLeftBtn()
{
	if(pFrame)
	{
		hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
		bDragLeftBtn = !bDragLeftBtn;
		// If we're dragging with copy, the move option is NOT checked.
		WinCheckMenuItem(hwndMenu, CM_DRAG_LEFT_BTN, bDragLeftBtn);
	}
}

void DFrame::CmShowFileTypes()
{
	DFileMaskDlg SetFileMaskDlg(this, szMask, &bShowHiddenOrSysObjs);

	int rc = SetFileMaskDlg.Execute();
	if(rc)
		// If we've set new view options, re-load files.
		OnFolderListClick();
}

void DFrame::CmSortByName()
{
	if(nSortCode != SORT_BY_NAME)
	{
		ResortFileEntries(SORT_BY_NAME);

		nSortCode = SORT_BY_NAME;

		if(pFrame)
		{
			hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);

			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 0);
			nOldSortCheckedItem = CM_SORTNAME;
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 1);
		}
		FileList->Invalidate();
	}
}

void DFrame::CmSortByType()
{
	if(nSortCode != SORT_BY_EXT)
	{
		ResortFileEntries(SORT_BY_EXT);

		nSortCode = SORT_BY_EXT;

		if(pFrame)
		{
			hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 0);
			nOldSortCheckedItem = CM_SORTTYPE;
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 1);
		}
		FileList->Invalidate();
	}
}

void DFrame::CmSortBySize()
{
	if(nSortCode != SORT_BY_SIZE)
	{
		ResortFileEntries(SORT_BY_SIZE);

		nSortCode = SORT_BY_SIZE;

		if(pFrame)
		{
			hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 0);
			nOldSortCheckedItem = CM_SORTSIZE;
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 1);
		}
		FileList->Invalidate();
	}
}

void DFrame::CmSortByDate()
{
	if(nSortCode != SORT_BY_DATE)
	{
		ResortFileEntries(SORT_BY_DATE);

		nSortCode = SORT_BY_DATE;

		if(pFrame)
		{
			hwndMenu = WinWindowFromID(pFrame->HWindow, FID_MENU);
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 0);
			nOldSortCheckedItem = CM_SORTDATE;
			WinCheckMenuItem(hwndMenu, nOldSortCheckedItem, 1);
		}
		FileList->Invalidate();
	}
}

void DFrame::CmConfirmOptions()
{
	// Show confirmation options dialog box.
	DConfirmDlg ConfirmOpts(this, &bConfirmDelete, &bConfirmReplace, &bConfirmCopy, &bConfirmAll);
	ConfirmOpts.Execute();
}

void DFrame::CmAbout()
{
	TDialog AboutDlg(this, IDD_ABOUT);
	AboutDlg.Execute();
}

int DFrame::GetFileSelections()
{
	int anSelIndexes[MAXENTRIES];

	SelectedFileList.Flush(TShouldDelete::NoDelete);

	int nCount = FileList->GetSelIndexes(anSelIndexes, MAXENTRIES);

	FILEENTRY *lvi;

	pGlobalParent = this;

	for(int i = 0; i < nCount; i++)
	{
		lvi = (FILEENTRY *)FileList->GetItemData(anSelIndexes[i]);
		if(lvi)
			SelectedFileList.Add(lvi);
	}

	nFileListSelCount = SelectedFileList.GetItemsInContainer();

	return nFileListSelCount;
}

int DFrame::GetFileListDblClickIndex()
{
	// Retrieves index of double-clicked item, at
	int nTopIndex = FileList->GetTopIndex();

	TRect rItem;
	int rc;

	nTargetIndex = -1;

	TRect rActualFiles = FileList->GetClientRect();
	int nCount = 0;

	TPoint pt;
	GetCursorPos(pt);
	FileList->ScreenToClient(pt);

	// Test for hit-test in our list box.
	int nItems = FileList->GetCount();

	int nFoundIndex = -1;
	for(int i = nTopIndex; i < nItems; i++)
	{
		rItem.Set(rActualFiles.left,
					rActualFiles.top + nCount * CYLBHEIGHT,
					rActualFiles.right,
					rActualFiles.top + ((nCount + 1) * CYLBHEIGHT));

		if(rItem.Contains(pt))
		{
			nFoundIndex = i;
			break;
		}
		nCount++;
	}
	if(nFoundIndex > -1)
		return nFoundIndex;
	else
		return -1;
}

void DFrame::EvSize(uint sizeType, TSize& size)
{
	// Resize our on-screen objects.

	// Call parent's method first.--This re-sizes status bar at bottom of screen.
	TWindow::EvSize(sizeType, size);

	cxWin = size.cx;
	cyWin = size.cy;

	// If first time with sizing, use mid-point of window for directory listbox.
	if(cxCurrentDir == 0)
		cxCurrentDir = cxWin / 3 - 4 * cxBorder;

	// Re-size our rectangles.

	// Column Header (Left)
	rFolderListHdr.left = 2 * cxBorder;
	rFolderListHdr.top = 2 * cxBorder;
	rFolderListHdr.right = rFolderListHdr.left + cxCurrentDir;
	rFolderListHdr.bottom = rFolderListHdr.top + cyTitleBar + 2 * cxBorder;

	// Left Listbox (Folders/Dir.'s)
	rFolderList.left = rFolderListHdr.left;
	rFolderList.top = rFolderListHdr.bottom + 2 * cxBorder;
	rFolderList.right = rFolderListHdr.right;
	rFolderList.bottom = cyWin - 2 * cxBorder;

	FolderList->MoveWindow(rFolderList.left,
							  rFolderList.top,
							  rFolderList.right - rFolderList.left,
							  rFolderList.bottom - rFolderList.top,
							  TRUE);

	TRect rActualFolderList = FolderList->GetClientRect();
	cyFolderList = rActualFolderList.bottom - rActualFolderList.top;

	FolderList->ShowWindow(SW_SHOWNORMAL);

	// Splitter Rectangle -- Used to re-size two lists--like a splitter window
	rSplitter.left = rFolderList.right - 5 * cxBorder;
	rSplitter.top = rFolderList.top;
	rSplitter.right = rSplitter.left + 10 * cxBorder;
	rSplitter.bottom = rFolderList.bottom;

	// Column Header (Right)
	rFileListHdr.left = rSplitter.right + cxBorder;
	rFileListHdr.top = rFolderListHdr.top;
	rFileListHdr.right = cxWin - 2 * cxBorder;
	rFileListHdr.bottom = rFolderListHdr.bottom;

	// Right Listbox (Documents/Files)
	rFileList.left = rFileListHdr.left;
	rFileList.top = rFolderList.top;
	rFileList.right = rFileListHdr.right;
	rFileList.bottom = rFolderList.bottom;

	FileList->MoveWindow(rFileList.left,
								rFileList.top,
								rFileList.right - rFileList.left,
								rFileList.bottom - rFileList.top,
								TRUE);

	TRect rActualFileList = FileList->GetClientRect();
	cyFileList = rActualFileList.bottom - rActualFileList.top;

	FileList->ShowWindow(SW_HIDE);
	FileList->ShowWindow(SW_SHOWNORMAL);

	FileList->InvalidateRect(rActualFileList);

	Invalidate();
}

// TreeView / ListView emulation methods start here.
int DFrame::AddTreeViewItem(TListBox* pLB, TREEVIEW_ITEM& TreeViewItem, int nPos)
{
	// Adds new tree item at specified position or if -1, appends item to list.
	assert(pLB);

	TREEVIEW_ITEM *pNewItem;

	pNewItem = new TREEVIEW_ITEM;

	if(!pNewItem)
		return -1;  // Failed.

	memcpy(pNewItem, &TreeViewItem, sizeof(TREEVIEW_ITEM));

	int nNewIndex = pLB->InsertString("NA", nPos);   // We don't use strings for owner-draw items.

	TRect rNone(-1,-1,-1,-1);

	if(nNewIndex > -1)
	{
		pLB->SetItemData(nNewIndex, (ULONG)(LPSTR)pNewItem);
		pLB->SetItemHeight(nNewIndex, CYLBHEIGHT);
		return nNewIndex;
	}
	else
	{
		MessageBox("Can't add item to list.", "Too Many List Items", MB_OK | MB_ICONSTOP);
		return -1;
	}
}

int DFrame::AddListViewItem(TListBox* pLB, FILEENTRY *ListViewItem, int nPos)
{
	// Adds new 'list view' item at specified position or if -1, appends item to list.
	assert(pLB);

	int nNewIndex;

	if(!ListViewItem)
		MessageBox("Bad ListViewItem in AddListViewItem!", "Error", MB_ICONEXCLAMATION);

	// Write text inside string--we could save memory by just writing the number.
	nNewIndex = pLB->InsertString("NA", nPos);

	if(nNewIndex > -1)
	{
		pLB->SetItemData(nNewIndex, (ULONG)(LPSTR)ListViewItem);
		pLB->SetItemHeight(nNewIndex, CYLBHEIGHT);
		return nNewIndex;
	}
	else
	{
		MessageBox("Can't add item to list.", "Too Many List Items", MB_OK | MB_ICONSTOP);
		return -1;
	}
}

LRESULT DFrame::DoDrawItem(WPARAM wParam, LPARAM lParam)
{
	// The OS/2 OWNERITEM structure contains these fields:
	//    HWND    hwnd;
	//		HPS     hps;
	//		ULONG   fsState;
	//		ULONG   fsAttribute;
	//		ULONG   fsStateOld;
	//		ULONG   fsAttributeOld;
	//		RECTL   rclItem;
	//		LONG    idItem; // This field contains idItem for menus, iItem for listboxes.
	//		ULONG   hItem;
	POWNERITEM pItem = (POWNERITEM)lParam;

	const int MAX_NODES = 256;
	int NodesFromRoot[256];

	TRect rItem;
	TRect rText;

	string strText;

	TSize sizeText;

	int nThisImage;

	int ctrlId = wParam;

	int i;

	// Just in case our background has changed.
	rgbLBBackground = WinQuerySysColor(HWindow,
												  SYSCLR_ENTRYFIELD,
												  0);

	TBrush *brBackground = new TBrush(TColor(rgbLBBackground));
	TPen *penBackground = new TPen(TColor(rgbLBBackground));

	if(ctrlId == IDC_DIRLIST)
	{
		// Folder List (TreeView) processing:

		// The TREEVIEW_ITEM structure contains these fields:
		//int nListBoxIndex;  // Index # of this entry.
		//int nLevel;         // In hierarchical list, this is the level of this entry
		//int nExpandCode;    // 0 = Nothing to expand, 1 == Can Expand [+], 2 Already Expanded [-]
		//int bIsEndOfLevel;
		//int iImage;
		//int iSelectedImage;
		//LPSTR lpszDirText;
		//int nTextLen;

		// Draw item with or without highlight with appropriate levels information
		TREEVIEW_ITEM *tvi = (TREEVIEW_ITEM *) FolderList->GetItemData(pItem->idItem);
		if(!tvi)
		{
			delete brBackground;
			delete penBackground;
			return TRUE;
		}

		int nLevelCount = tvi->nLevel - 2;
		assert(nLevelCount < MAX_NODES);
		for(TREEVIEW_ITEM *tviThis = (TREEVIEW_ITEM *)tvi->Parent;
				tviThis != NULL && nLevelCount > -1; tviThis = (TREEVIEW_ITEM *)tviThis->Parent)
		{
			NodesFromRoot[nLevelCount] = tviThis->bIsEndOfLevel;
			nLevelCount--;
		}

		// Get a device context for this control.
		TClientDC dc(pItem->hwnd);

		// Draw with listbox font.
		if(pfontLB)
			dc.SelectObject(*pfontLB);

		// Get current dimensions of the Folder List listbox.
		TRect rActualFolder = FolderList->GetClientRect();

		// Calculate the dimensions of this listbox item.
		rItem.Set(pItem->rclItem.xLeft,
					 rActualFolder.bottom - rActualFolder.top - pItem->rclItem.yTop,     					 pItem->rclItem.xRight,
					 rActualFolder.bottom - rActualFolder.top - pItem->rclItem.yBottom);

		// Clear entry first.
		dc.SelectObject(*brBackground);
		dc.SelectObject(*penBackground);
		dc.Rectangle(rItem);

		// Draw hierarchy levels as required...
		int cxStart = 0;

		dc.SelectObject(*ConnectorPen);

		for(int i = 0; i < tvi->nLevel; i++)
		{
			if(i == 0)
				cxStart = rItem.left + 9;
			else
				cxStart += 20;

			// If not last time, draw this line.
			if(i < tvi->nLevel - 1)
			{
				if(!NodesFromRoot[i])
				//if(!(i == tvi->nLevel - 2 && tvi->Parent && ((TREEVIEW_ITEM *)tvi->Parent)->bIsEndOfLevel))
				{
					// Draw connectors to other nodes in level(s).
					// The special case is an expanded node that is the last in its level.
					dc.MoveTo(cxStart, rItem.top);
					dc.LineTo(cxStart, rItem.bottom);
				}
			}
		}

		// Last connector is more work...
		if(cxStart == 0)
			cxStart = rItem.left + 9;

		int cyMid = (rItem.top + rItem.bottom) / 2;

		dc.MoveTo(cxStart, rItem.top);
		if(tvi->bIsEndOfLevel)
			dc.LineTo(cxStart, cyMid);
		else
			dc.LineTo(cxStart, rItem.bottom);

		dc.MoveTo(cxStart, cyMid);
		dc.LineTo(cxStart + 11, cyMid);

		// If this node is expandable or collapsible, draw a box.
		if(tvi->nExpandCode == 1 || tvi->nExpandCode == 2)
		{
			dc.SelectObject(*SolidConnectorPen);
			dc.SelectObject(*brBackground);

			dc.Rectangle(cxStart - 4,
							 cyMid - 4,
							 cxStart + 4,
							 cyMid + 4);

			// Draw [-]
			dc.SetPixel(cxStart - 2, cyMid, TColor(0,0,0));
			dc.SetPixel(cxStart - 1, cyMid, TColor(0,0,0));
			dc.SetPixel(cxStart, cyMid, TColor(0,0,0));
			dc.SetPixel(cxStart + 1, cyMid, TColor(0,0,0));
			dc.SetPixel(cxStart + 2, cyMid, TColor(0,0,0));

			if(tvi->nExpandCode == 1)  // Draw pixels for additional [+]
			{
				dc.SetPixel(cxStart, cyMid - 2, TColor(0,0,0));
				dc.SetPixel(cxStart, cyMid - 1, TColor(0,0,0));
				dc.SetPixel(cxStart, cyMid + 1, TColor(0,0,0));
				dc.SetPixel(cxStart, cyMid + 2, TColor(0,0,0));
			}
		}

		// Measure our current text.
		sizeText = dc.GetTextExtent(tvi->lpszDirText, strlen(tvi->lpszDirText));

		rText.Set(rItem.left + cxStart + 30,
					 rItem.top + 2,
					 rItem.left + cxStart + 30 + sizeText.cx + 2,
					 rItem.top + sizeText.cy + 3);

		if(pItem->fsState == TRUE) // Highlighted
		{
			// Highlight text only in blue
			dc.SetBkColor(TColor(0,0,255));
			dc.SetTextColor(TColor(255,255,255));  // Text in white.
			// Draw selected image.
			nThisImage = tvi->iSelectedImage;

			// Draw text next to image.
			dc.ExtTextOut(rText.left + 1,
							  rText.top + 1,
							  ETO_OPAQUE,
							  &rText, tvi->lpszDirText, strlen(tvi->lpszDirText), NULL);
		}
		else
		{
			// Draw normal text and normal image..
			dc.SetBkColor(TColor(rgbLBBackground));
			dc.SetTextColor(TColor(0,0,0));  // Text in black.
			nThisImage = tvi->iImage;
			dc.TextOut(rText.left + 1, rText.top + 1, tvi->lpszDirText, strlen(tvi->lpszDirText));
		}

		DrawImage(dc, nThisImage, cxStart + 11, rItem.top + 1);  // +4

		if(tvi->nExpandCode == 2)
		{
			// Extra connector under bitmap for expanded levels.
			dc.SelectObject(*ConnectorPen);
			cxStart += 20;
			dc.MoveTo(cxStart, cyMid + 7);
			dc.LineTo(cxStart, rItem.bottom);
		}

		dc.RestoreObjects();

		// Tell OS/2 not to draw highlighting.
		pItem->fsState = pItem->fsStateOld;
		// Tell OS/2 not to draw focus.
		pItem->fsAttribute = pItem->fsAttributeOld;

		delete brBackground;
		delete penBackground;

		return TRUE;
	}
	else if(ctrlId == IDC_FILELIST)
	{
		// Files List (ListView) processing:

		// The FILEENTRY class contains the following relevant members:
		//   long lFileSize;
		//   unsigned long  ulFileAttrib;
		//   unsigned short uFileTime;
		//   unsigned short uFileDate;
		//   char szFileName[256];
		//   char szExt[4];  // File Extension
		//   int nType;   // 1 for Directory, 2 for .EXE, 3 for .CMD, 4 for .BAT, 5 for .COM and 6 for document.
		//   int iImage;	// Which bitmap/-image to draw.

		// Draw list view item with appropriate icon.
		FILEENTRY *lvi = (FILEENTRY *) FileList->GetItemData(pItem->idItem);
		if(!lvi)
		{
			delete brBackground;
			delete penBackground;
			return TRUE;
		}

		// Get device context for the Files List listbox.
		TClientDC dc(pItem->hwnd);

		// Draw with listbox font.
		if(pfontLB)
			dc.SelectObject(*pfontLB);

		// Get current dimensions of the Folder List listbox.
		TRect rActualFileList = FileList->GetClientRect();

		// Calculate our height.
		cyFileList = rActualFileList.bottom - rActualFileList.top;

		// Determine the size of this item.
		rItem.Set(pItem->rclItem.xLeft,
					 rActualFileList.bottom - rActualFileList.top - pItem->rclItem.yTop,
					 pItem->rclItem.xRight,
					 rActualFileList.bottom - rActualFileList.top - pItem->rclItem.yBottom);

		// Clear entry first.
		dc.SelectObject(*brBackground);
		dc.SelectObject(*penBackground);

		dc.Rectangle(rItem);

		int cxStart = 2;

		sizeText = dc.GetTextExtent(lvi->szFileName, strlen(lvi->szFileName));

		// Get the dimensions of the text required for our filename / directory name.
		rText.Set(rItem.left + cxStart + 20,
					 rItem.top + 2,
					 rItem.left + cxStart + 20 + sizeText.cx + 2,
					 rItem.top + sizeText.cy + 3);

		// Draw the image next to our text
		DrawImage(dc, lvi->iImage, cxStart, rItem.top + 1, 0, TRUE);  // + 4

		if(pItem->fsState == TRUE) // Highlighted
		{
			// Highlight text in blue
			dc.SetBkColor(TColor(0,0,255));
			dc.SetTextColor(TColor(255,255,255));  // Text in white.
			// Draw text next to image.
			dc.ExtTextOut(rText.left + 1,
							  rText.top + 1,
							  ETO_OPAQUE,
							  &rText, lvi->szFileName, strlen(lvi->szFileName), NULL);
		}
		else
		{
			// Draw normal text.
			dc.SetBkColor(TColor(rgbLBBackground));
			dc.SetTextColor(TColor(0,0,0));  // Text in black.
			dc.TextOut(rText.left + 1, rText.top + 1, lvi->szFileName, strlen(lvi->szFileName));
		}

		// Check which file details are selected (if any), and draw them in normal text.
		dc.SetBkColor(TColor(rgbLBBackground));
		dc.SetTextColor(TColor(0,0,0));  // Text in black.

		string strDisplay;

		int cxNextPos = rText.left + cxName;

		if(bShowSize)
		{
			// Then display file size (except for folders!).
			if(lvi->nType != 1)
			{
				strDisplay = GetFileSizeAsStr(lvi->lFileSize);
				dc.TextOut(cxNextPos, rText.top + 1, strDisplay.c_str(), strDisplay.length());
			}
			// Advance to next 'column' in list.
			cxNextPos += cxSize;
		}
		if(bShowTimeStamp)
		{
			// Then display file's date/time of last modification.
			char szTemp[40];
			strDisplay = GetFileTimeStampAsString(szTemp, lvi->uFileDate, lvi->uFileTime);
			dc.TextOut(cxNextPos, rText.top + 1, strDisplay.c_str(), strDisplay.length());
			// Advance to next 'column' in list.
			cxNextPos += cxTimeStamp;
		}
		if(bShowAttr)
		{
			// Check for each type of attribute--move over the same
			// distance for each whether or not each attribute is set or not.
			// This ensures alignment of each attribute type.

			// Check and display 'System' attribute.
			if((lvi->ulFileAttrib & FA_SYSTEM) != 0)
				dc.TextOut(cxNextPos, rText.top + 1, "S", 1);

			// Check and display 'Hidden' attribute.
			if((lvi->ulFileAttrib & FA_HIDDEN) != 0)
				dc.TextOut(cxNextPos + cxAttribute, rText.top + 1, "H", 1);

			// Check and display 'Read-Only' attribute.
			if((lvi->ulFileAttrib & FA_RDONLY) != 0)
				dc.TextOut(cxNextPos + (cxAttribute * 2), rText.top + 1, "R", 1);

			// Check and display 'Archive' attribute.
			if((lvi->ulFileAttrib & FA_ARCH) != 0)
				dc.TextOut(cxNextPos + (cxAttribute * 3), rText.top + 1, "A", 1);

			// Check and display 'Directory' attribute.
			if((lvi->ulFileAttrib & FA_DIREC) != 0)
				dc.TextOut(cxNextPos + (cxAttribute * 4), rText.top + 1, "D", 1);
		}
		dc.RestoreObjects();

		// Tell OS/2 not to draw highlighting.
		pItem->fsState = pItem->fsStateOld;
		// Tell OS/2 not to draw focus.
		pItem->fsAttribute = pItem->fsAttributeOld;

		delete brBackground;
		delete penBackground;

		return TRUE;
	}
	return 0L;
}

void DFrame::EvRButtonDown(UINT keys, TPoint& point)
{
	nMouseBtnDownCode = RIGHT_BTN;
	EvLButtonDown(keys, point);
}

LRESULT DFrame::DoDragOver(WPARAM wParam, LPARAM lParam)
{
	lParam;
	return ((LRESULT)DragOver(WinQueryAnchorBlock(HWindow),
									  (MPARAM)wParam,
									  szCurrentListViewDir));
}

LRESULT DFrame::DoDragLeave(WPARAM wParam, LPARAM lParam)
{
	wParam;
	lParam;
	return ((LRESULT)DragLeave());
}

LRESULT DFrame::DoDropHelp(WPARAM wParam, LPARAM lParam)
{
	lParam;
	return ((LRESULT)DropHelp(HWindow, (MPARAM)wParam));
}

LRESULT DFrame::DoDrop(WPARAM wParam, LPARAM lParam)
{
	lParam;
	return ((LRESULT)Drop(WinQueryAnchorBlock(HWindow),
								HWindow,
								(MPARAM)wParam,
								szCurrentListViewDir));
}

void CustomListBox::EvMouseMove(UINT keys, TPoint& point)
{
	// Another OS/2 / OWL anomaly--mouse move messages are not delivered
	// to the DFrame window when the left mouse button is pressed.
	// The solution here is to use a derived class that will
	// receive this message and call the parent's member function.
	if(pGlobalParent && bForwarding)
	{
		((DFrame *)pGlobalParent)->EvMouseMove(keys, point);
	}
	DefaultProcessing();
}

class DApp : public TApplication
{
	public:
		DApp();  // Our constructor
		void InitMainWindow();
};


class CustomDecFrame : public TDecoratedFrame
{
private:
	int cxMin;
	int cyMin;

public:
	CustomDecFrame(TWindow *pParent, char *pTitle, TWindow *pClient) :
		TDecoratedFrame(pParent, pTitle, pClient) {
			cxMin = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
			cyMin = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);
		}

	// Inline for speed here...
	BOOL PreProcessMsg(MSG &msg) {
		if(msg.message == WM_PAINT)
		{
			// Remove status bar if we're iconized.
			// This is required to overcome an apparent problem in OWL
			// where a TDecoratedFrame object paints over the icon when minimized.
			TRect r;
			GetClientRect(r);

			if(((r.right - r.left) <= cxMin) && ((r.bottom - r.top) <= cyMin))
			{
				// Erase status bar;
				if(pGlobalStatus)
					pGlobalStatus->ShowWindow(SW_HIDE);
			}
			else
			{
				// Paint as before.
				// Show status bar.
				if(pGlobalParent)
					pGlobalParent->ShowWindow(SW_SHOW);
			}
		}
		return TDecoratedFrame::PreProcessMsg(msg);
	}
};

//*****************************************************************************
// Define our DApp class methods below.
//*****************************************************************************

DApp::DApp() : TApplication()
{
}

void DApp::InitMainWindow()
{
	// Do some initializing...
	DFrame *MyFrame = new DFrame(NULL);
	MyFrame->Attr.AccelTable = ID_MAIN;

	//TDecoratedFrame *pBaseFrame = new TDecoratedFrame(0, "Warp Cabinet", MyFrame);
	CustomDecFrame *pBaseFrame = new CustomDecFrame(0, "Warp Cabinet", MyFrame);

	pBaseFrame->AssignMenu(TResId(ID_MAIN));

	// Let us draw our icon in minimized state.
	pBaseFrame->SetIcon(this, ID_MAIN);

	SetMainWindow(pBaseFrame);

	// Create status bar on bottom of our window.
	TStatusBar* sb = new TStatusBar(0, TGadget::Recessed);
	sb->Attr.Style |= WS_CHILD;
	pBaseFrame->Insert(*sb, TDecoratedFrame::Bottom);

	TTextGadget *stattext = new TTextGadget(0, TGadget::Recessed, TTextGadget::Left, 70);

	sb->Insert(*stattext, TStatusBar::Before);
	EnableCtl3d(TRUE);

	MyFrame->SetStatusTextCtrl(stattext);
	MyFrame->SetFrame(pBaseFrame);

	hptrCopy = (HPOINTER)LoadCursor(ID_COPY_DOCS);
	hptrMove = (HPOINTER)LoadCursor(ID_MOVE_DOCS);

	pGlobalStatus = sb;
}

// Entry point to an OWL application.
OwlMain(int, char **)
{
	DApp WarpFilerApp;  // Create our application
	int rc;

	rc = WarpFilerApp.Run();

	return rc;
}

