#=============================================================
#
#	WARPCAB.MAK - Makefile for project C:\WARPCAB\WARPCAB.PRJ
#		Created on 10/20/95 at 20:31
#
#=============================================================

.AUTODEPEND

#=============================================================
#		Translator Definitions
#=============================================================
CC = bcc +WARPCAB.CFG
TASM = tasm.exe
TLIB = tlib.exe
TLINK = tlink
RC = brcc.exe
RB = rc.exe
LIBPATH = C:\BCOS2\LIB;c:\bcos2\owl\lib
INCLUDEPATH = C:\BCOS2\INCLUDE;c:\bombos2\owl\include


#=============================================================
#		Implicit Rules
#=============================================================
.c.obj:
  $(CC) -c {$< }

.cpp.obj:
  $(CC) -c {$< }

.asm.obj:
  $(TASM) -Mx $*.asm,$*.obj

.rc.res:
  $(RC) -r $*.rc

#=============================================================
#		List Macros
#=============================================================
LINK_EXCLUDE =  \
 warpcab.res

LINK_INCLUDE =  \
 warpcab.obj \
 warpcab.def \
 warpcab.obj \
 filentry.obj \
 filechng.obj \
 filetool.obj \
 runcmd.obj \
 creatdir.obj \
 selfiles.obj \
 fileprop.obj \
 fileasoc.obj \
 do_drag.obj \
 confirm.obj \
 details.obj \
 filemask.obj \
 associat.obj

#=============================================================
#		Explicit Rules
#=============================================================
warpcab.exe: warpcab.cfg $(LINK_INCLUDE) $(LINK_EXCLUDE)
  $(TLINK) /v /x /wdef /wimt /wdpl /Toe /aa /L$(LIBPATH) @&&|
C:\BCOS2\LIB\C02.OBJ+
warpcab.obj+
warpcab.obj+
filentry.obj+
filechng.obj+
filetool.obj+
runcmd.obj+
creatdir.obj+
selfiles.obj+
fileprop.obj+
fileasoc.obj+
do_drag.obj+
confirm.obj+
details.obj+
filemask.obj+
associat.obj
warpcab
		# no map file
C:\BCOS2\LIB\BIDS2.LIB+
C:\BCOS2\LIB\OWLPM.LIB+
C:\BCOS2\LIB\C2MT.LIB+
C:\BCOS2\LIB\OS2.LIB
warpcab.def
|
  rc.exe WARPCAB.res warpcab.exe

#=============================================================
#		Individual File Dependencies
#=============================================================
WARPCAB.obj: warpcab.cfg WARPCAB.ICO 
	$(CC) -c WARPCAB.ICO

WARPCAB.res: warpcab.cfg WARPCAB.RC 
	brcc.exe -R -I$(INCLUDEPATH) -FO WARPCAB.res WARPCAB.RC

WARPCAB.obj: warpcab.cfg WARPCAB.CPP 

FILENTRY.obj: warpcab.cfg FILENTRY.CPP 

FILECHNG.obj: warpcab.cfg FILECHNG.CPP 

FILETOOL.obj: warpcab.cfg FILETOOL.CPP 

RUNCMD.obj: warpcab.cfg RUNCMD.CPP 

CREATDIR.obj: warpcab.cfg CREATDIR.CPP 

SELFILES.obj: warpcab.cfg SELFILES.CPP 

FILEPROP.obj: warpcab.cfg FILEPROP.CPP 

FILEASOC.obj: warpcab.cfg FILEASOC.CPP 

DO_DRAG.obj: warpcab.cfg DO_DRAG.CPP 

CONFIRM.obj: warpcab.cfg CONFIRM.CPP 

DETAILS.obj: warpcab.cfg DETAILS.CPP 

FILEMASK.obj: warpcab.cfg FILEMASK.CPP 

ASSOCIAT.obj: warpcab.cfg ASSOCIAT.CPP 

#=============================================================
#		Compiler Configuration File
#=============================================================
warpcab.cfg: warpcab.mak
  copy &&|
-L$(LIBPATH)
-I$(INCLUDEPATH)
-H=C:\WARPCAB\WARPCAB.CSM
-vi-
-sm
-y
-v
| warpcab.cfg


