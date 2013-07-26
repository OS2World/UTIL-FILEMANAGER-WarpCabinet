// DO_DRAG.CPP  -- Drag API functions to allow dragging files from Warp Cabinet to other
//				  		system objects that accept files.
#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#define INCL_DOSERRORS
#define INCL_SHLERRORS
#define INCL_WIN
#define INCL_WINSTDDRAG
#define INCL_WINDIALOGS

#include <os2.h>
#include <string.h>
#include <process.h>
#include <classlib\arrays.h>
#include "filentry.h"
#include "resource.h"

typedef char *LPSTR;

#include "do_drag.h"

// Complete list of selected files is in WARPFILE.CPP
static PDRAGTARGETTHREAD vpThreadParms = NULL;

// Globals for this module.
static char   szRendMechFmtFILE []  = "<DRM_OS2FILE,DRF_UNKNOWN>";
static USHORT    usOperation;  // Stores user operation.
static ULONG    ulDragCount = 0L;  // Number of drag operations.
static PDRAGINFO pGlobalDraginfo = NULL;

static HAB   habGlobal;
static CHAR  szClientClass[12];        // "DragDrop"
static HWND  hwndClient;               // Needed by change dir. function
static HWND  hwndGlobalFileList;       // Handle to list control..

static HPOINTER hptrBusy;              // Mouse Pointer to show when a
													// drag is in progress

// Cleans-up all resources needed for a drag operation.
BOOL FreeSourceResources (HAB hab, PDRAGINFO pDraginfo)
{
	PDRAGITEM       pDragitem;
	ULONG           ulItemsDragged, i;

	if(!DrgAccessDraginfo(pDraginfo) && DrgDeleteDraginfoStrHandles(pDraginfo))
	{
		// Drag access not OK.
		MessageBox(HWND_DESKTOP, IDMSG_DRAGACCESSERROR, MB_OK | MB_ERROR, TRUE);
		return FALSE;
	}

	if(DrgFreeDraginfo(pDraginfo)
					|| PMERR_SOURCE_SAME_AS_TARGET == WinGetLastError(hab))
		return TRUE;
	else
	{
		MessageBox(HWND_DESKTOP, IDMSG_DRAGFREEERROR, MB_OK | MB_ERROR, TRUE);
		return FALSE;
	}
}

// Prevents our app. from closing during a drag operation.
VOID PreventClose(HWND hwndClient, ULONG cDrag)
{
	HWND hwndSysMenu = WinWindowFromID(ParentOf(hwndClient), FID_SYSMENU);
	WinSendMsg(hwndSysMenu, MM_SETITEMATTR, MPFROM2SHORT(SC_CLOSE, (SHORT)TRUE),
				 MPFROM2SHORT(MIA_DISABLED, (SHORT)(cDrag ? MIA_DISABLED : FALSE)));
	return;
}

// Removes final backslash from an OS/2 directory.
VOID RemoveFinalBackslash (PSZ p)
{
	for (; *p; ++p);  // Go to the end of the string.
	if (*--p == '\\')
	{
		*p = '\0';
	}
}

// Starts the drag operation.
BOOL InitDrag (HAB hab, HWND hwndClient,  int nFilesToDrag, HPOINTER hptrFile,
					HPOINTER hptrFolder, HPOINTER hptrMulti, LPSTR pContainerName)
{
	CHAR             chDrive;
	SHORT            item;
	ULONG            i;
	ULONG 			  ulItemsDragged = nFilesToDrag;
	HWND             hwndTarget;
	LHANDLE          hImage;
	DRAGITEM         Dragitem;
	HSTR             hstrType, hstrRMF, hstrContainerName;
	USHORT           fsControl;
	USHORT           fsSupportedOps;
	CHAR             szItemName [64];
	CHAR             szFullContainerName [CCHMAXPATH];
	PDRAGINFO        pSourceDraginfo;
	DRAGIMAGE        dimg;

	PDRAGINFO pdinfo;       // Pointer to DRAGINFO structure
	HWND      hwnd;         // Handle of calling (source) window
	BOOL      flResult;     // Result indicator
	DRAGITEM  ditem;        // DRAGITEM structure
	HBITMAP   hbm;          // Bit-map handle
	HWND      hwndDrop;     // Handle of drop (target) window

	habGlobal = hab;

	LONG lDragButton;

	// Get number of items to drag.
	if(!nFilesToDrag)
	{
		MessageBox(HWND_DESKTOP, IDMSG_ATLEASTONE, MB_OK | MB_ERROR, TRUE);
		return FALSE;
	}

	// Allocate the DRAGINFO structure and the APPDRAGINFO structure.
	pSourceDraginfo = DrgAllocDraginfo(nFilesToDrag);

	// Save to global handle.
	pGlobalDraginfo = pSourceDraginfo;

	// Initialize variable used to initialize the internal DRAGITEM
	// structure.
	hstrType  = DrgAddStrHandle ((PSZ)DRT_UNKNOWN);       // File type is unknown
	hstrRMF   = DrgAddStrHandle ((PSZ)szRendMechFmtFILE); // os2file, unknown
	strcpy (szFullContainerName, (char*)pContainerName);  // container name  

	RemoveFinalBackslash ((PSZ)szFullContainerName);
	strcat (szFullContainerName, "\\");
	hstrContainerName = DrgAddStrHandle((PSZ)szFullContainerName);

	fsControl = 0;
	chDrive = (CHAR) pContainerName[0];

	if((chDrive=='A') || (chDrive=='B') || (chDrive=='a') || (chDrive=='b'))
	{
		// Is source container removable media?
		fsControl = (SHORT)(fsControl | ((SHORT)DC_REMOVEABLEMEDIA));
	}

	fsSupportedOps = DO_COPYABLE | DO_MOVEABLE;    // Can only move or copy.

	// Initialize common portions of the internal Dragitem buffer.
	Dragitem.hwndItem          = hwndClient;
	Dragitem.hstrType          = hstrType;
	Dragitem.hstrRMF           = hstrRMF;
	Dragitem.hstrContainerName = hstrContainerName;
	Dragitem.fsControl         = fsControl;
	Dragitem.fsSupportedOps    = fsSupportedOps;

	// Write selected files/dirs. to OS/2.
	FILEENTRY *pFileEntry;

	for (i = 0L; i < nFilesToDrag; i++)
	{
		pFileEntry = (FILEENTRY *)SelectedFileList[i];
		if(!pFileEntry)
			continue;

		Dragitem.hstrSourceName = DrgAddStrHandle ((PSZ)pFileEntry->szFileName);
		Dragitem.hstrTargetName = Dragitem.hstrSourceName;

		Dragitem.ulItemID = i;
		if (pFileEntry->nType == 1)
			Dragitem.fsControl = (SHORT)((Dragitem.fsControl | ((SHORT)DC_CONTAINER)));

		if(!DrgSetDragitem(pSourceDraginfo, &Dragitem, sizeof(DRAGITEM), i))
		{
			MessageBox(HWND_DESKTOP, IDMSG_DRAGSETITEMERROR, MB_OK | MB_ERROR, TRUE);
			return FALSE;
		}
	}
	//  Drag the items.
	if (ulItemsDragged == 1L)
		hImage = (Dragitem.fsControl & DC_CONTAINER) ? hptrFolder : hptrFile;
	else
		hImage = hptrMulti;

	++ulDragCount;                     // Remember drag is in progress
	PreventClose(hwndClient, ulDragCount);

	dimg.cb       = sizeof (DRAGIMAGE);
	dimg.hImage   = hImage;
	dimg.fl       = DRG_ICON;
	dimg.cxOffset = 0;
	dimg.cyOffset = 0;

	pSourceDraginfo->hwndSource = hwndClient;

	// End drag when either left or right button is released.
	lDragButton = bDragLeftBtn ? VK_BUTTON1 : VK_BUTTON2;

	hwndTarget = DrgDrag(hwndClient, pSourceDraginfo, &dimg, 1L, lDragButton, NULL);

	// User cancelled the drag or asked for help, so we can release
	// the resources allocated to the drag operation.	--ulDragCount;
	PreventClose (hwndClient, ulDragCount);
	if (hwndTarget == NULLHANDLE)
		return FALSE;
	else
		return TRUE;// Turn off 'pAppDraginfo' is assigned a value that is never used warning.
#pragma warn -aus
}
#pragma warn +aus

MRESULT EndConversation(HAB hab, HWND hwndClient, MPARAM mp1, MPARAM mp2)
{
	USHORT i;
	// Did the target properly render the item?
	if(mp2 == (MPARAM)DMFL_TARGETFAIL)
		MessageBox(HWND_DESKTOP, IDMSG_TARGETRENDER, MB_OK | MB_ERROR, TRUE);
	// Refresh our listbox.
	WinSendMsg(hwndGlobalParent, CM_REFRESH, 0, 0);

	if(DrgFreeDraginfo(pGlobalDraginfo)
		|| PMERR_SOURCE_SAME_AS_TARGET == WinGetLastError(hab))
	{
		--ulDragCount;                   // Indicate drag completed.
		PreventClose(hwndClient, ulDragCount);
	}
	else
		MessageBox(HWND_DESKTOP, IDMSG_DRAGFREEERROR, MB_OK | MB_ERROR, TRUE);
	mp1; // Kill warning.
	return (MRESULT)NULL;
}

MRESULT DragLeave (void)
{
	return (MRESULT)NULL;
}

// Displays help message for dragging files.
MRESULT DropHelp (HWND hwndClient, MPARAM mp1)
{
	PDRAGINFO pDraginfo = (PDRAGINFO)mp1;

	char message[81];

	if(!DrgAccessDraginfo (pDraginfo))
	{
		MessageBox(HWND_DESKTOP, IDMSG_DRAGACCESSERROR, MB_OK | MB_ERROR, TRUE);
	}
	strcpy (message, "Dropping here would perform ");

	switch (pDraginfo->usOperation)
	{
		case DO_DEFAULT:
			strcat (message, "the default ");
			break;
		case DO_COPY:
			strcat (message, "a copy ");
			break;
		case DO_MOVE:
			strcat (message, "a move ");
			break;
		case DO_UNKNOWN:
			strcat (message, "an unknown (private) ");
			break;
	}
	strcat (message, "operation.");
	Message ((PSZ)message);

	if(DrgDeleteDraginfoStrHandles(pDraginfo) && DrgFreeDraginfo(pDraginfo));
	else
	{
		MessageBox(HWND_DESKTOP, IDMSG_DRAGERROR, MB_OK | MB_ERROR, TRUE);
	}
	return (MRESULT)NULL;
// turn off warnings for unreferenced formal parameters.
#pragma warn -par
}
#pragma warn +par

// Provides visual feedback for dragging over this object.
MRESULT DragOver(HAB hab, MPARAM mp1, LPSTR pCurrentDir)
{
	PDRAGINFO pDraginfo = (PDRAGINFO)mp1;
	USHORT        usIndicator, cItems, i;
	ULONG         ulBytesWritten;
	PDRAGITEM     pditem;
	CHAR          SourceDir [CCHMAXPATH], chDrive;

	// Get access to the DRAGINFO structure.
	if(!DrgAccessDraginfo(pDraginfo))
	{
		MessageBox(HWND_DESKTOP, IDMSG_DRAGACCESSERROR, MB_OK | MB_ERROR, TRUE);
		return (MRFROM2SHORT (DOR_NODROPOP, 0));
	}
	usOperation = (USHORT)DO_MOVE;          // the default operation is move

	// Determine if a drop can be accepted.
	switch (pDraginfo->usOperation)
	{
		// return DOR_NODROPOP if current operation is link or unknown.

		case DO_UNKNOWN:
			if(!DrgFreeDraginfo (pDraginfo))
				MessageBox(HWND_DESKTOP, IDMSG_DRAGFREEERROR, MB_OK | MB_ERROR, TRUE);
			return (MRFROM2SHORT (DOR_NODROPOP, 0));


		// Determine default operation if current operation is default.
		// The default is move unless either the source or the target is
		// on different removable media.
		case DO_DEFAULT:
			pditem = DrgQueryDragitemPtr (pDraginfo, 0);
			ulBytesWritten = DrgQueryStrName(pditem->hstrContainerName,
													sizeof(SourceDir),
													(PSZ)SourceDir);
			if(ulBytesWritten == 0L)
			{
				MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYSTRINGERROR,
									  MB_OK | MB_ERROR, TRUE);
				return (MRFROM2SHORT (DOR_NODROPOP, 0));
			}

			// If the source and target are on the
			// same drive, the operation is a move
			//
			if (*pCurrentDir == SourceDir[0])
				usOperation = (USHORT)DO_MOVE;
				// Otherwise, different drives
			else
			{
				chDrive = (CHAR)pCurrentDir[0];
				// If the target is A: or B:, or the source
				// is removeable media, the operation is a copy
				if((chDrive == 'A') || (chDrive == 'B') || (chDrive == 'a') ||
					(chDrive=='b') || pditem->fsControl & DC_REMOVEABLEMEDIA)
				{
					usOperation = (USHORT)DO_COPY;
				}
				// Otherwise, the default operation is a move
				else
				{
					usOperation = (USHORT)DO_MOVE;
				}
			}
			break;

		case DO_MOVE:
		case DO_COPY:
			usOperation = pDraginfo->usOperation;
			break;
	}

	// In order to support the operation, the source must support the
	// operation this target has decided upon.  The source must also
	// support a rendering mechanism of <DRM_OS2FILE,DRF_UNKNOWN>.
	// This target doesn't care about the file type since we are copying the file only.
	usIndicator = DOR_DROP;
	cItems = (SHORT)DrgQueryDragitemCount (pDraginfo);

	// Inspect each item to see if it is acceptable.
	for (i = 0; i < cItems; ++i)
	{
		pditem = DrgQueryDragitemPtr (pDraginfo, i);

		// The item is acceptable only if it is copyable and the
		// operation is a copy, or it is moveable and the operation
		// is a move, and it can render <DRM_OS2FILE,DRF_UNKNOWN>
		if (((pditem->fsSupportedOps & DO_COPYABLE) &&
								(usOperation == (USHORT)DO_COPY)) ||
			 ((pditem->fsSupportedOps & DO_MOVEABLE) &&
								  (usOperation == (USHORT)DO_MOVE)))
		{
			if (DrgVerifyRMF(pditem, (PSZ)"DRM_OS2FILE", (PSZ)"DRF_UNKNOWN"))
			{
				usIndicator = DOR_DROP;
			}
			else
			{
				usIndicator = DOR_NEVERDROP;
				break;
			}
		}
		else
		{
			usIndicator = DOR_NODROPOP;
			break;
		}
	}

	if(!DrgFreeDraginfo(pDraginfo))
		if(PMERR_SOURCE_SAME_AS_TARGET != WinGetLastError(hab))
		{
			MessageBox(HWND_DESKTOP, IDMSG_DRAGFREEERROR, MB_OK | MB_ERROR, TRUE);
			return (MRFROM2SHORT (DOR_NODROPOP, 0));
		}
	return (MRFROM2SHORT(usIndicator, usOperation));
}

MRESULT Drop(HAB hab, HWND hwndClient, MPARAM mp1, LPSTR pCurrentDir)
{
	PDRAGINFO pDraginfo = (PDRAGINFO)mp1;
	PDRAGTARGETTHREAD     pParm;
	PBYTE          aStack;
	TID           tid;
	USHORT        i;
	PDRAGTRANSFER pdxfer;
	PDRAGITEM     pditem;
	HSTR          hstrRMF;
	char          szFullTargetName  [CCHMAXPATH];
	char          szTargetFile [CCHMAXPATHCOMP];
	QMSG          qmsg;
	USHORT        cRetry, idTimer;
	ULONG         mr, ulBytesWritten;

	// Get access to the Draginfo structure.
	if(!DrgAccessDraginfo (pDraginfo))
		MessageBox(HWND_DESKTOP, IDMSG_DRAGACCESSERROR, MB_OK | MB_ERROR, TRUE);
	if (pDraginfo->hwndSource == hwndClient)
	{
		// Source window and target window are the same, so don't do the
		// operation... just release the resources allocated for the drag.
		FreeSourceResources (hab, pDraginfo);
		--ulDragCount;
		PreventClose (hwndClient, ulDragCount);
		return (MRESULT)NULL;
	}

	pditem = DrgQueryDragitemPtr (pDraginfo, 0);

	// If the source name wasn't provided, let the source render
	if (!pditem->hstrSourceName)
	{
		pdxfer   = DrgAllocDragtransfer (pDraginfo->cditem);
		hstrRMF  = DrgAddStrHandle ((PSZ)"<DRM_OS2FILE, DRF_UNKNOWN>");

		for (i=0; i<pDraginfo->cditem; i++)
		{
			pditem = DrgQueryDragitemPtr (pDraginfo, i);

			// Build the fully qualified target file name.
			strcpy (szFullTargetName, (char*)pCurrentDir);
			RemoveFinalBackslash ((PSZ)szFullTargetName);
			ulBytesWritten =DrgQueryStrName(pditem->hstrTargetName,
													  sizeof(szTargetFile),
													  (PSZ)szTargetFile);
			if(ulBytesWritten == 0L)
				MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYSTRINGERROR,
									MB_OK | MB_ERROR, TRUE);

			if (szTargetFile[0] != '\\')
				strcat(szFullTargetName, "\\");
			strcat(szFullTargetName, szTargetFile);

			// Initialize the DRAGTRANSFER structure.
			pdxfer[i].cb               = sizeof (DRAGTRANSFER);
			pdxfer[i].hwndClient       = hwndClient;
			pdxfer[i].pditem           = pditem;
			pdxfer[i].hstrSelectedRMF  = hstrRMF;
			pdxfer[i].hstrRenderToName = DrgAddStrHandle ((PSZ)szFullTargetName);
			pdxfer[i].ulTargetInfo     = 0L;
			pdxfer[i].usOperation      = pDraginfo->usOperation;
			pdxfer[i].fsReply          = 0;

			// Send the DM_RENDER to the source, retrying as necessary
			for (cRetry=0, mr=0; cRetry<MAX_RETRIES && !mr; cRetry++)
			{
				mr = (ULONG)DrgSendTransferMsg(pDraginfo->hwndSource,
														 DM_RENDER,
														 (MPARAM)(pdxfer + i),
														 (MPARAM)NULL);
				//  If the source didn't render, retry if we can
				if (!mr)
				{
					if (pdxfer[i].fsReply & DMFL_RENDERRETRY)
					{
						idTimer = (USHORT)WinStartTimer (0L, 0L, 0L, 1000L);

						// Start a timer to wait, but process
						// messages so we don't hang the queue
						while (WinGetMsg (0L, &qmsg, 0L, 0, 0))
						{
							if ((qmsg.msg == WM_TIMER) &&
										 ((USHORT) qmsg.mp1 == idTimer))
							{
								WinStopTimer (0L, 0L, idTimer);
								break;
							}
							else
								WinDispatchMsg (0L, &qmsg);
						}
					}
					else
						DrgFreeDragtransfer(pdxfer + i);
				}
			}
		}
	}
	// Allocate the stack segment for the target thread.
	else if (DosGetNamedSharedMem((PPVOID)&aStack, (PSZ)"\\sharemem\\dragmem",
											PAG_READ | PAG_WRITE) != 0LU)
	{
		MessageBox(HWND_DESKTOP, IDMSG_CANTCREATESTACK, MB_OK | MB_ERROR, TRUE);
		DrgDeleteDraginfoStrHandles(pDraginfo);
		DrgFreeDraginfo(pDraginfo);
	}
	else
	{
		// Build the parameter list structure for the target thread.
		DosSubAllocMem(aStack, (PPVOID)&pParm, sizeof(DRAGTARGETTHREAD));

		pParm->pDraginfo   = pDraginfo;
		pParm->hwndClient  = hwndClient;
		pParm->usOperation = usOperation;
		strcpy(pParm->szTargetDir, (char*)pCurrentDir);

		// Remember that a drag is in progress and start the target thread.
		++ulDragCount;
		PreventClose (hwndClient, ulDragCount);

		vpThreadParms = pParm;

		_beginthread(TargetThread, STACKSIZE, (void *)vpThreadParms);
	}
	return (MRESULT)NULL;
}


// Clean-up.
MRESULT EndTarget (HWND hwnd, PDRAGTARGETTHREAD pParm)
{
	USHORT                cItems, i;
	DRAGITEM              Dragitem;

	// Decrement the count of drag operations in progress and prepare to
	// release the resources allocated for the drag operation.
	--ulDragCount;
	PreventClose (hwnd, ulDragCount);
	cItems = (SHORT)DrgQueryDragitemCount (pParm->pDraginfo);

	// End the conversation for each item in the DRAGINFO structure and
	// let source know we have completed the drag operation for each item.
	for (i = 0; i < cItems; ++i)
	{
		if(!DrgQueryDragitem (pParm->pDraginfo, sizeof(Dragitem), &Dragitem, i))
			MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYITEMERROR,
						  MB_OK | MB_ERROR, TRUE);
		DrgSendTransferMsg(Dragitem.hwndItem, DM_ENDCONVERSATION,
								 MPFROMLONG(Dragitem.ulItemID),
								 MPFROMLONG(DMFL_TARGETSUCCESSFUL));
	}
	// Free the draginfo structure.
	if(DrgDeleteDraginfoStrHandles(pParm->pDraginfo)
					  && DrgFreeDraginfo(pParm->pDraginfo));
	else
		MessageBox(HWND_DESKTOP, IDMSG_DRAGERROR, MB_OK | MB_ERROR, TRUE);
	WinPostMsg(hwndGlobalParent, CM_REFRESH, NULL, NULL);
	return (MRESULT)NULL;
}

BOOL CheckBusy (void)
{
	return ((BOOL) ulDragCount);
}

MRESULT CheckClose (HWND hwnd)
{
	if (!ulDragCount)
		WinPostMsg (hwnd, WM_QUIT, (MPARAM)0L, (MPARAM)0L);

	return ((MPARAM)NULL);
}

void TargetThread (void *pDummy)
{
	PDRAGTARGETTHREAD pParm;
	USHORT        cItems, i;
	ULONG         ulBytesWritten;
	DRAGITEM      Dragitem;
	char          szFullSourceName  [CCHMAXPATH];
	char          szFullTargetName  [CCHMAXPATH];
	char          szSourceFile [CCHMAXPATHCOMP];
	char          szTargetFile [CCHMAXPATHCOMP];

	pDummy; // Kill warning.

	pParm = vpThreadParms;

	// Get the number of items being dragged.
	cItems = (SHORT)DrgQueryDragitemCount (pParm->pDraginfo);

	// If the source has provided the file name in the DRAGITEM
	// structures, we can go ahead and do a target copy without getting
	// the source involved in the operation.  We will always copy the file
	// using the suggested target name.  In this application the names
	// are always the same.
	for (i = 0; i < cItems; ++i)
	{
		if(!DrgQueryDragitem (pParm->pDraginfo, sizeof(Dragitem), &Dragitem, i))
			MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYITEMERROR,
							 MB_OK | MB_ERROR, TRUE);
		if(Dragitem.hstrSourceName    != 0L &&
			Dragitem.hstrTargetName    != 0L &&
			Dragitem.hstrContainerName != 0L)
		{
			// Build the fully qualified source file name.
			ulBytesWritten = DrgQueryStrName(Dragitem.hstrContainerName,
														sizeof(szFullSourceName),
														(PSZ)szFullSourceName);
			if(ulBytesWritten == 0)
				MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYSTRINGERROR,
							  MB_OK | MB_ERROR, TRUE);
			ulBytesWritten = DrgQueryStrName(Dragitem.hstrSourceName,
														sizeof(szSourceFile),
														(PSZ)szSourceFile);
			if(ulBytesWritten == 0)
				MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYSTRINGERROR,
								MB_OK | MB_ERROR, TRUE);

			if (szSourceFile[0] == '\\')
				strcpy (szSourceFile, szSourceFile+1);

			strcat (szFullSourceName, szSourceFile);

			// Build the fully qualified target file name.
			strcpy (szFullTargetName, pParm->szTargetDir);
			RemoveFinalBackslash ((PSZ)szFullTargetName);
			ulBytesWritten = DrgQueryStrName(Dragitem.hstrTargetName,
														sizeof(szTargetFile),
														(PSZ)szTargetFile);
			if(ulBytesWritten == 0)
				MessageBox(HWND_DESKTOP, IDMSG_DRAGQUERYSTRINGERROR,
									MB_OK | MB_ERROR, TRUE);

			if (szTargetFile[0] != '\\')
				strcat (szFullTargetName, "\\");

			strcat (szFullTargetName, szTargetFile);

			// Do the operation.  DosCopy works with different drives;
			// DosMove does not, so we will use DosCopy and DosDelete.
			switch (pParm->usOperation)
			{
				case DO_MOVE:     //  Mimic DosMove with DosCopy and DosDelete.
					 if(DosCopy((PSZ)szFullSourceName, (PSZ)szFullTargetName, DCPY_EXISTING)
									  == NO_ERROR)
						 DosDelete((PSZ)szFullSourceName);
					 break;
				case DO_COPY:
					 DosCopy ((PSZ)szFullSourceName, (PSZ)szFullTargetName, DCPY_EXISTING);
					 break;
			}
		}
	}

	// Post ourselves a target complete message so the client window
	// knows we are done and can free the stack segment and the draginfo
	// structure.
	DosEnterCritSec();                   // Make sure we finish before the
													 // main thread frees our stack.
	WinPostMsg (pParm->hwndClient, WM_TARGETCOMPLETE, pParm, NULL);
}

VOID Message (PSZ pszMsg)
{
	WinMessageBox (HWND_DESKTOP, hwndClient, pszMsg, (PSZ)szClientClass, 0,
					  MB_MOVEABLE | MB_ICONEXCLAMATION);
}

SHORT MessageBox(HWND hwndOwner,
					  SHORT idMsg,
					  SHORT fsStyle,
					  BOOL fBeep)
{
	CHAR szText[MESSAGELEN];

	if(!WinLoadMessage(habGlobal, (HMODULE)0L, idMsg, MESSAGELEN, (PSZ)szText))
	{
		 WinAlarm(HWND_DESKTOP, WA_ERROR);
		 return MBID_ERROR;
	}

	if(fBeep)
		 WinAlarm(HWND_DESKTOP, WA_ERROR);

	return (SHORT)(WinMessageBox(HWND_DESKTOP, hwndOwner,(PSZ)szText,(PSZ)NULL,
					  (USHORT)MSGBOXID,(ULONG)fsStyle));
}

