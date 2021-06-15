/*--------------------------------------------------------------------------+
|                                                                           |
|       Copyright (c) 1992 by Info Tech, Inc.  All Rights Reserved.         |
|                                                                           |
|       This program module is part of DS/Shell (PC), Info Tech's           |
|       database interfaces for OS/2, a proprietary product of              |
|       Info Tech, Inc., no part of which may be reproduced or              |
|       transmitted in any form or by any means, electronic,                |
|       mechanical, or otherwise, including photocopying and recording      |
|       or in connection with any information storage or retrieval          |
|       system, without permission in writing from Info Tech, Inc.          |
|                                                                           |
+--------------------------------------------------------------------------*/


/************************************************************************
 *                                                                      *
 * initdll.c                                                            *
 *                                                                      *
 * Copyright (C) 1990-1992 Info Tech, Inc.                              *
 * Mark Hampton                                                         *
 *                                                                      *
 * This is an almost generic DLL initialization module.  The function   *
 * DllInit is called by either the ITIDLLI.OBJ or ITIDLLC.OBJ module.   *
 *                                                                      *
 * Use the ITIDLLI.OBJ module when your DLL does not use the C run time *
 * library.  Use the ITIDLLC.OBJ module when you need to use the C run  *
 * time library, and you need to initialize your DLL.  Note that when   *
 * you do this, you will have to link with CRTDLL_I.OBJ, a standard     *
 * Microsoft module.                                                    *
 *                                                                      *
 * Note: Do not rename this file to DLLINIT, since this file name       *
 * is already used by Microsoft for C run time library initialization.  *
 *                                                                      *
 * This file is part of the Database system of DS/SHELL. This is a      *
 * proprietary product of Info Tech. and no part of which may be        *
 * reproduced or transmitted in any form or by any means, including     *
 * photocopying and recording or in connection with any information     *
 * storage or retrieval system, without permission in writing           *
 * from Info Tech, Inc.                                                 *
 *                                                                      *
 *                                                                      *
 ************************************************************************/

#define  INCL_DOSMISC

#include "..\include\iti.h"
#include "initdll.h"


/*
 * hmodModule is the handle to this DLL
 */

HMODULE hmodModule;
BOOL  bTraceBuffer;


/*
 * DllInit is called by either ITIDLLI.OBJ or ITIDLLC.OBJ.  This functino
 * must return non-zero if the initialization succeded, or FALSE
 * if initialization failed.
 */


BOOL DLLINITPROC DllInit (HMODULE hmodDll)
   {
   /* call your module initialization functions here. */
#if defined (DEBUG)
   PSZ      pszE;
   if (!DosScanEnv ("TRBUF", &pszE))
      bTraceBuffer = TRUE;
   else
      bTraceBuffer = FALSE;
#endif

   hmodModule = hmodDll;
   return TRUE;
   }


