#if !defined (_DO_DRAG_H_)
#define _DO_DRAG_H_

extern TISArrayAsVector <FILEENTRY> SelectedFileList;
extern ULONG ulTotalFilesBytes;  // Total bytes of these selected files.
extern HWND hwndGlobalParent;
extern int bDragCopy;
extern int bDragLeftBtn;

#ifdef __cplusplus
	extern "C" {
#endif

BOOL InitDrag (HAB hab, HWND hwndClient, int nFilesToDrag,
					HPOINTER hptrFile, HPOINTER hptrFolder, HPOINTER hptrMulti,
					LPSTR pContainerName);
MRESULT Drop            (HAB, HWND, MPARAM, LPSTR);
MRESULT DragLeave       (VOID);
MRESULT DragOver        (HAB, MPARAM, LPSTR);
MRESULT DropHelp        (HWND, MPARAM);
MRESULT EndConversation (HAB, HWND, MPARAM, MPARAM);

#if !defined (_DROP_API_ONLY_)

#define STACKSIZE            11228
#define MESSAGELEN           50    // maximum length for messages

#define ID_RESOURCE             1
#define ID_BUSY                 260

#define IDD_OPEN                300
#define IDD_STATICTEXT          301
#define IDD_EDIT                302

#define IDM_MENU                10
#define IDM_FILE                20
#define IDM_FILEOPEN            21
#define IDM_EXIT                22
#define IDM_WINDOW              30
#define IDM_WINDOWREFRESH       31
#define IDM_WINDOWSELECTALL     32
#define IDM_WINDOWDESELECTALL   33
#define IDM_RENDER              40
#define IDM_TARGET              41
#define IDM_NOTARGET            42
#define IDM_FILENATIVE          43
#define IDM_DDENATIVE           44
#define IDM_DRAGFILES           50
#define IDM_DROPFILES           51
#define IDM_SRCRENDER           52
#define IDM_HELP                60
#define IDM_HELPINDEX           61
#define IDM_HELPGENERAL         62
#define IDM_HELPUSINGHELP       63
#define IDM_HELPPRODUCTINFO     64

#define IDS_APPNAME             16
#define IDS_CLIENTCLASS         17
#define IDS_HELPLIBRARYNAME     18

#define IDD_PRODUCTINFO     1001

#define IDC_OK              1
#define IDC_CANCEL          2
#define IDC_HELP            3
#define IDC_ICON            4
#define IDC_BITMAP          5
#define IDR_BITMAP          6

#define INITIAL_SEGSIZE         4096
#define INCREMENT_SEGSIZE       1024

#define MAX_RETRIES             10

#define LIST_ID                 256
#define MSGBOXID                1002

#define IDMSG_INITFAILED                1
#define IDMSG_MAINWINCREATEFAILED       2
#define IDMSG_CANTCREATELIST            3
#define IDMSG_NOTSUBDIRECTORY           4
#define IDMSG_MUSTBESELECTED            5
#define IDMSG_BADDIRECTORY              6
#define IDMSG_ATLEASTONE                7
#define IDMSG_CANTLOCATE                8
#define IDMSG_CANTCREATESTACK           9
#define IDMSG_HEAPERROR                10
#define IDMSG_HEAPALLOCERROR           11
#define IDMSG_ALLOCDRAGINFO            12
#define IDMSG_CANNOTLOADSTRING         13
#define IDMSG_HELPNOTAVAILABLE         14
#define IDMSG_HELPCREATEERROR          15
#define IDMSG_HELPASSOCIATEERROR       16
#define IDMSG_HELPDISPLAYERROR         17
#define IDMSG_CANTSTARTDIALOG          18
#define IDMSG_DRAGERROR                19
#define IDMSG_DRAGACCESSERROR          20
#define IDMSG_DRAGFREEERROR            21
#define IDMSG_DRAGSETITEMERROR         22
#define IDMSG_DRAGQUERYSTRINGERROR     23
#define IDMSG_DRAGQUERYITEMERROR       24
#define IDMSG_FINDAPPDRAGINFO          25
#define IDMSG_MEMFREEERROR             26
#define IDMSG_TARGETRENDER             27

// return values for initialization routines 
#define RETURN_SUCCESS      0   // successful return
#define RETURN_ERROR        1   // error return 
#define BEEP_WARN_FREQ      60  // frequency of warning beep 
#define BEEP_WARN_DUR      100  // duration of warning beep

// User defined commands

#define WM_REFRESH              WM_USER+1
#define WM_CREATELIST           WM_USER+2
#define LN_INITDRAG             WM_USER+3
#define LN_DRAGOVER             WM_USER+4
#define LN_DRAGLEAVE            WM_USER+5
#define LN_DROPHELP             WM_USER+6
#define LN_DROP                 WM_USER+7
#define WM_TARGETCOMPLETE       WM_USER+8

#if !defined(_NOPDRAGINFO_)
typedef struct _DRAGTARGETTHREAD
		  {
		  PDRAGINFO       pDraginfo;
		  HWND            hwndClient;
		  USHORT          usOperation;
		  CHAR            szTargetDir [CCHMAXPATH];
		  } DRAGTARGETTHREAD;

typedef DRAGTARGETTHREAD FAR *PDRAGTARGETTHREAD;
#endif

// type definitions

VOID    Message (PSZ pszMsg);
SHORT   MessageBox(HWND hwndOwner, SHORT idMsg, SHORT fsStyle, BOOL fBeep);

MRESULT EndTarget       (HWND, PDRAGTARGETTHREAD);
MRESULT CheckClose      (HWND);
BOOL    CheckBusy          (VOID);
VOID    PreventClose    (HWND hwndClient, ULONG cDrag);
VOID    RemoveFinalBackslash (PSZ);
void    TargetThread   (void *pDummy);

VOID Message (PSZ pszMsg);
SHORT MessageBox(HWND hwndOwner, SHORT idMsg, SHORT fsStyle, BOOL fBeep);

// Turn off the "code has no effect" warnings these macros may generate
#pragma warn -eff
#define WinDeleteLboxAll(hwndLbox) \
	 ((BOOL)WinSendMsg((hwndLbox), LM_DELETEALL, NULL, NULL))

#define WinQueryLboxSelItemFirst(hwndLbox) \
	 ((SHORT)WinSendMsg((hwndLbox), LM_QUERYSELECTION, \
				MPFROMSHORT (LIT_FIRST), NULL))

#define WinQueryLboxSelItemNext(hwndLbox, index) \
	 ((SHORT)WinSendMsg((hwndLbox), LM_QUERYSELECTION, \
				MPFROMSHORT (index), NULL))

#define WinScrollLboxItemToTop(hwndLbox, index) \
	 ((BOOL)WinSendMsg((hwndLbox), LM_SETTOPINDEX, MPFROMSHORT (index), \
			  NULL))

#define WinFindLboxItem(hwndLbox, psz) \
	 ((USHORT)WinSendMsg((hwndLbox), LM_SEARCHSTRING, \
				 MPFROM2SHORT (0, LIT_FIRST), (MPARAM) psz))

#define WinSelectLboxItem(hwndLbox, index, fSelect) \
	 ((USHORT)WinSendMsg((hwndLbox), LM_SELECTITEM, \
				 (MPARAM) (index), (MPARAM) (fSelect)))

#define ParentOf(hwnd)              WinQueryWindow ((hwnd), QW_PARENT)
#define OwnerOf(hwnd)               WinQueryWindow ((hwnd), QW_OWNER)

#define SendOwner(hwnd,msg,mp2,id)  \
			  (WinSendMsg (OwnerOf (hwnd), WM_CONTROL, \
								MPFROM2SHORT (id,msg), mp2))

#endif

#ifdef __cplusplus
	}
#endif

#endif

