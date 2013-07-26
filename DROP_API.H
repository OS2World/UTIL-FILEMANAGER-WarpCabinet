#if !defined (_DROP_API_)
	#define _DROP_API_
// Prototypes for public Direct Manipulation functions.
#ifdef __cplusplus
	extern "C" {
#endif

	typedef short int SHORT;
	typedef short unsigned int USHORT;

	typedef struct _DRAGINFO     /* dinfo */
	{
		ULONG    cbDraginfo;               /* Size of DRAGINFO and DRAGITEMs*/
		USHORT   cbDragitem;               /* size of DRAGITEM              */
		USHORT   usOperation;              /* current drag operation        */
		HWND     hwndSource;               /* window handle of source       */
		SHORT    xDrop;                    /* x coordinate of drop position */
		SHORT    yDrop;                    /* y coordinate of drop position */
		USHORT   cditem;                   /* count of DRAGITEMs            */
		USHORT   usReserved;               /* reserved for future use       */
	} DRAGINFO;
	typedef DRAGINFO *PDRAGINFO;

	typedef char *PSZ;

	MRESULT Drop            (HAB, HWND, PDRAGINFO, PSZ);
	MRESULT DragLeave       (VOID);
	MRESULT DragOver        (HAB, PDRAGINFO, PSZ);
	MRESULT DropHelp        (HWND, PDRAGINFO);

#ifdef __cplusplus
	}
#endif

#endif
