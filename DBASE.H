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
 * dbase.h                                                              *
 *                                                                      *
 * Copyright (C) 1990-1992 Info Tech, Inc.                              *
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

extern PGLOBALS pglobals;
extern HHEAP    hheapDB0; /*--- temporary usage hheap    ---*/
extern HHEAP    hheapDB1; /*--- hbuf, pszQuery, dbblock  ---*/
extern HHEAP    hheapDB2; /*--- pcol, pseghdr, pszFormat ---*/
extern HHEAP    hheapDB3; /*--- ppszTable                ---*/

extern BOOL bVerbose;

// The following procs are defined in dbmain.c
// and declared in database.h
//
//
//ItiDbInit EXPORT (USHORT uNumConnections)
//ItiDbTerminate ()
//

extern BOOL ItiDbGetColData (HDB      hdb,
                            HHEAP    hheap,
                            HMODULE  hmod,
                            USHORT   uResType,
                            USHORT   uResId,
                            BOOL     bDlgRes,
                            USHORT   uBuffId,
                            USHORT   *uNumCols,
                            PCOLDATA *ppcol);

extern void FreePCOL (PCOLDATA pcol, HHEAP hheap, USHORT uNumCols);

extern PSZ EXPORT ItiDbGetDatabaseName (void);

