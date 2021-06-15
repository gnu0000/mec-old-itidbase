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
 * dbqueue.h                                                            *
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

/*
 *  Commands for the command queue
 */
#define ITIDB_FILLLIST        0
#define ITIDB_FILLSTATIC      1
#define ITIDB_DELETEBUFFER    2

/*
 * This procedure must be called before the command queue can be used
 */
extern void DbInitCmdQ (void);


/*
 * this can be called by anyone who wishes to
 * perform a buffer update. It returns right away
 */
extern void DbWriteCmdQ (HBUF hbuf, HMODULE hmod, USHORT uCmd, HWND hwnd);


/*
 * this is called only by threads created to execute the 
 * buffer update. Threads do not return from here. Each 
 * invocation of this proc must have a unique hdb.
 */
extern void _cdecl DbReadCmdQ (HDB hdb);
