// OS2API.H -- Call's to OS/2 API, normally disguised by OWL PM classes.

#if !defined(_OS2API_H)
#define _OS2API_H

#ifdef __cplusplus
	extern "C" {
#endif
	#define SPTR_FILE                  19
   #define SPTR_FOLDER                20
	#define SPTR_MULTFILE              21

	#define DM_DROP                0x032f
   #define DM_DRAGOVER            0x032e
	#define DM_DRAGLEAVE           0x032d
   #define DM_DROPHELP            0x032c
	#define DM_ENDCONVERSATION     0x032b
   #define DM_PRINT               0x032a
   #define DM_RENDER              0x0329
   #define DM_RENDERCOMPLETE      0x0328
	#define DM_RENDERPREPARE       0x0327
   #define DM_DRAGFILECOMPLETE    0x0326
	#define DM_EMPHASIZETARGET     0x0325
   #define DM_DRAGERROR           0x0324
   #define DM_FILERENDERED        0x0323
   #define DM_RENDERFILE          0x0322
	#define DM_DRAGOVERNOTIFY      0x0321
   #define DM_PRINTOBJECT         0x0320
	#define DM_DISCARDOBJECT       0x031f

   #define DOR_NODROP             0x0000   /* DM_DRAGOVER response codes */
   #define DOR_DROP               0x0001
	#define DOR_NODROPOP           0x0002
   #define DOR_NEVERDROP          0x0003

	#define SV_CXBORDER                26
	#define SV_CYTITLEBAR              30
	#define SV_CXICON                  38
	#define SV_CYICON                  39

	#define EM_SETTEXTLIMIT            0x0143

	LONG APIENTRY WinQuerySysValue(HWND hwndDesktop,
											 LONG iSysValue);

	typedef VOID *MPARAM;    /* mp    */
	typedef VOID *MRESULT;   /* mres  */

	typedef VOID *PVOID;

	typedef CHAR           *PCHAR;

	MRESULT APIENTRY WinSendMsg(HWND hwnd,
										 ULONG msg,
										 MPARAM mp1,
										 MPARAM mp2);

	#define WM_SYSCOLORCHANGE		0x002b
	#define WM_MINMAXFRAME        0x0046

   #define FS_ICON                    0x00000001L

	typedef LHANDLE   HPS;         /* hps  */

	typedef struct _RECTL          /* rcl */
	{
		LONG  xLeft;
		LONG  yBottom;
		LONG  xRight;
		LONG  yTop;
	} RECTL;
	typedef RECTL *PRECTL;

	typedef struct _OWNERITEM    /* oi */
	{
		HWND    hwnd;
		HPS     hps;
		ULONG   fsState;
		ULONG   fsAttribute;
		ULONG   fsStateOld;
		ULONG   fsAttributeOld;
		RECTL   rclItem;
		LONG    idItem; /* This field contains idItem for menus, iItem for lb. */
		ULONG   hItem;
	} OWNERITEM;
	typedef OWNERITEM *POWNERITEM;

	#define MM_SETITEMATTR             0x0192
	#define MIA_CHECKED                0x2000
	#define MIA_DISABLED               0x4000
	#define HWNDFROMMP(mp)             ((HWND)(mp))
	#define MPFROM2SHORT(s1, s2)       ((MPARAM)MAKELONG(s1, s2))

	#define WinCheckMenuItem(hwndMenu, id, fcheck) \
	((BOOL)WinSendMsg(hwndMenu, MM_SETITEMATTR, \
	MPFROM2SHORT(id, TRUE), \
	MPFROM2SHORT(MIA_CHECKED, (USHORT)(fcheck) ? MIA_CHECKED : 0)))

	#define WinEnableMenuItem(hwndMenu, id, fEnable) \
	((BOOL)WinSendMsg(hwndMenu, MM_SETITEMATTR, \
	MPFROM2SHORT(id, TRUE), \
	MPFROM2SHORT(MIA_DISABLED, (USHORT)(fEnable) ? 0 : MIA_DISABLED)))

	typedef VOID *MPARAM;    		  /* mp    */

	#define MPFROMP(p)                 ((MPARAM)((ULONG)(p)))
	#define MPFROMSHORT(s)             ((MPARAM)(USHORT)(s))

	HAB    APIENTRY WinQueryAnchorBlock(HWND hwnd);

	BOOL APIENTRY WinShowWindow(HWND hwnd, BOOL fShow);

	BOOL APIENTRY WinEnableWindow(HWND hwnd, BOOL fEnable);

	LONG APIENTRY WinQueryWindowTextLength(HWND hwnd);

	#define FERR_DISABLEHARDERR     0x00000000L     /* disable hard error popups */

	typedef unsigned long  APIRET;

	APIRET APIENTRY DosError(ULONG error);

	typedef HWND *PHWND;

	#define FID_SYSMENU                0x8002
	#define FID_MENU                   0x8005
	#define FID_CLIENT                 0x8008

	HWND APIENTRY WinWindowFromID(HWND hwndParent, ULONG id);

	typedef const char          *PCSZ;

	APIRET APIENTRY  DosCopy(PCSZ pszOld, PCSZ pszNew, ULONG option);
	APIRET APIENTRY  DosMove(PCSZ pszOld, PCSZ pszNew);

	BOOL APIENTRY WinSetWindowText(HWND hwnd, PCSZ pszText);

	typedef LHANDLE HPOINTER;   /* hptr */

	HPOINTER APIENTRY WinQuerySysPointer(HWND hwndDesktop,
													 LONG iptr,
													 BOOL fLoad);

	BOOL     APIENTRY WinDestroyPointer(HPOINTER hptr);

	#define SYSCLR_ENTRYFIELD (-47L)

	LONG APIENTRY WinQuerySysColor(HWND hwndDesktop,
											 LONG clr,
											 LONG lReserved);
	#define QWL_STYLE                  (-2)
	#define QW_PARENT       5

	ULONG  APIENTRY WinQueryWindowULong(HWND hwnd,
													LONG index);
	BOOL   APIENTRY WinSetWindowULong(HWND hwnd,
												 LONG index,
												 ULONG ul);
	HWND    APIENTRY WinQueryWindow(HWND hwnd,
											  LONG cmd);

	typedef struct _RESULTCODES
	{   //
		ULONG codeTerminate;      // Termination code or process ID
		ULONG codeResult;         // Exit code
	} RESULTCODES;

	typedef RESULTCODES *PRESULTCODES;

  typedef LHANDLE PID;            // pid
  typedef PID *PPID;

  typedef ULONG *PULONG;

  typedef char          *PSZ;

  typedef struct _STARTDATA     /* stdata */
  {
		USHORT  Length;
		USHORT  Related;
		USHORT  FgBg;
		USHORT  TraceOpt;
		PSZ     PgmTitle;
      PSZ     PgmName;
      PBYTE   PgmInputs;
      PBYTE   TermQ;
      PBYTE   Environment;
      USHORT  InheritOpt;
      USHORT  SessionType;
		PSZ     IconFile;
      ULONG   PgmHandle;
		USHORT  PgmControl;
      USHORT  InitXPos;
		USHORT  InitYPos;
      USHORT  InitXSize;
      USHORT  InitYSize;
      USHORT  Reserved;
      PSZ     ObjectBuffer;
		ULONG   ObjectBuffLen;
  } STARTDATA;
  typedef STARTDATA *PSTARTDATA;

  APIRET APIENTRY  DosStartSession(PSTARTDATA psd,
												PULONG pidSession,
												PPID ppid);

	#define SSF_RELATED_INDEPENDENT 0
	#define SSF_RELATED_CHILD       1

	#define SSF_FGBG_FORE           0
	#define SSF_FGBG_BACK           1

	#define SSF_TRACEOPT_NONE       0
	#define SSF_TRACEOPT_TRACE      1
	#define SSF_TRACEOPT_TRACEALL   2

	#define SSF_INHERTOPT_SHELL     0
	#define SSF_INHERTOPT_PARENT    1

	/* note that these types are identical to those in pmshl.h for PROG_* */
	#define SSF_TYPE_DEFAULT        0
	#define SSF_TYPE_FULLSCREEN     1
	#define SSF_TYPE_WINDOWABLEVIO  2
	#define SSF_TYPE_PM             3
	#define SSF_TYPE_VDM            4
	#define SSF_TYPE_GROUP          5
	#define SSF_TYPE_DLL            6
	#define SSF_TYPE_WINDOWEDVDM    7
	#define SSF_TYPE_PDD            8
	#define SSF_TYPE_VDD            9

	/* note that these flags are identical to those in pmshl.h for SHE_* */
	#define SSF_CONTROL_VISIBLE     0x0000
	#define SSF_CONTROL_INVISIBLE   0x0001
	#define SSF_CONTROL_MAXIMIZE    0x0002
	#define SSF_CONTROL_MINIMIZE    0x0004
	#define SSF_CONTROL_NOAUTOCLOSE 0x0008
	#define SSF_CONTROL_SETPOS      0x8000

	BOOL     APIENTRY WinSetPointer(HWND hwndDesktop,
											  HPOINTER hptrNew);

	typedef struct _FSQBUFFER2       /* fsqbuf Data structure for QFSAttach*/
	{
		USHORT  iType;
		USHORT  cbName;
		USHORT  cbFSDName;
		USHORT  cbFSAData;
		UCHAR   szName[1];
		UCHAR   szFSDName[1];
		UCHAR   rgFSAData[1];
	} FSQBUFFER2;
	typedef FSQBUFFER2 *PFSQBUFFER2;

	APIRET APIENTRY  DosQueryFSAttach(PCSZ    pszDeviceName,
												 ULONG  ulOrdinal,
												 ULONG  ulFSAInfoLevel,
												 PFSQBUFFER2 pfsqb,
												 PULONG pcbBuffLength);

	APIRET APIENTRY  DosQueryCurrentDisk(PULONG pdisknum,
													 PULONG plogical);

	APIRET APIENTRY  DosQueryCurrentDir(ULONG disknum,
													PBYTE pBuf,
													PULONG pcbBuf);

	APIRET APIENTRY  DosDeleteDir(PCSZ pszDir);
#ifdef __cplusplus
	}
#endif

#endif