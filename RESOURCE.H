// RESOURCE.H

#define IDD_FILE_CHANGE_DLG	1000

#define IDC_STATIC		-1
#define IDC_FROM_PROMPT	100
#define IDC_FROM			101
#define IDC_TO_PROMPT   102
#define IDC_TO				103
#define IDC_CURRENT_DIR	104
#define IDC_STATUS		105

#define IDD_FILE_PROP_DLG	1001

#define IDC_READONLY		110
#define IDC_ARCHIVE		111
#define IDC_HIDDEN		112
#define IDC_SYSTEM		113
#define IDC_FILENAME		114
#define IDC_FILESIZE		115
#define IDC_MODIFIED		116
#define IDC_PATH			117

#define IDD_RUN_DLG				1002

#define IDC_CMD_LINE		120
// IDC_CURRENT_DIR defined above
// IDC_STATUS defined above

#define IDD_ASSOCIATE_DLG		1003

#define IDC_EXT				130
#define IDC_ASSOC_DESC		131
#define IDC_ASSOC_PATH		132
#define IDC_BROWSE			133
#define IDC_ASSOC_LIST		134
#define IDC_DELETE_ASSOC	111

#define IDD_CREATE_DIR_DLG		1004

#define IDC_NEW_DIR     180

#define IDD_SEL_FILES_DLG		1005

#define IDC_FILE_MASK	140
#define IDC_DESELECT		141
// DID_OK is 'Select' button

#define IDD_VIEW_FILES_DLG		1006

// IDC_FILE_MASK define above.
#define IDC_ALL_FILES		150
#define IDC_SYSHIDDEN_OBJS	151

#define IDD_FILE_DETAILS_DLG	1007

// IDC_SIZE defined above.
#define IDC_LAST_MODIFIED	160
#define IDC_ATTRIBUTES		161

#define IDD_SET_CONFIRM_OPTS_DLG	1008

#define IDC_CONFIRM_COPY		170
#define IDC_CONFIRM_DELETE		171
#define IDC_CONFIRM_REPLACE	172
#define IDC_CONFIRM_ALL			173

#define IDD_ABOUT				2000

#define ID_MAIN       		1000

#define ID_MOVE_DOCS   		2100
#define ID_COPY_DOCS   		2200

// Command ID's
#define CM_OPEN				100
#define CM_MOVE         	101
#define CM_COPY         	102
#define CM_DELETE       	103
#define CM_RENAME       	104
#define CM_PROPERTIES   	105
#define CM_RUN          	106
#define CM_ASSOCIATE      	107
#define CM_CREATE_DIR      108
#define CM_APP_EXIT			109

#define CM_SELECTALL			200
#define CM_SELECTFILES 		201
#define CM_CLEARSELS    	202

#define CM_FILE_NAME    	303
#define CM_ALL_DETAILS  	304
#define CM_PARTIAL_DETAILS 305
#define CM_SORTNAME        306
#define CM_SORTTYPE        307
#define CM_SORTSIZE        308
#define CM_SORTDATE        309
#define CM_SHOWFILES       310

#define PM_FILE_RESULT_DONE   (WM_USER + 1)

#define CM_CONFIRMATION    400
#define CM_STOP				401
#define CM_REFRESH			402
#define CM_RELOAD_FOLDERS	403
#define CM_DRAG_LEFT_BTN	404

#define CM_ABOUT				500

#define LN_INITDRAG             WM_USER+3
#define LN_DRAGOVER             WM_USER+4
#define LN_DRAGLEAVE            WM_USER+5
#define LN_DROPHELP             WM_USER+6
#define LN_DROP                 WM_USER+7
#define WM_TARGETCOMPLETE       WM_USER+8

#define IDC_DIRLIST 			1000
#define IDC_FILELIST      	1001

// Image/Bitmap ID's
#define IMAGE_BASE			1

#define ID_FLOPPY 			1
#define ID_HDD	 	   		2
#define ID_CDROM   			3
#define ID_FOLDER	   		4
#define ID_OPEN_FOLDER  	5
#define ID_DOCUMENT 			6
#define ID_EXE					7
#define ID_CMD       		8
#define ID_BAT       		9
#define ID_COM       		10

