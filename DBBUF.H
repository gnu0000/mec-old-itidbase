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
 * dbbuf.h                                                              *
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

// The following procs are defined in dbbuf.c
// and declared in database.h
//
//HBUF    ItiDbCreateBuffer
//void    ItiDbDeleteBuffer
//PSZ     ItiDbGetBufferRow 
//MRESULT ItiDbQueryBuffer


BOOL DbFillStaticBuffer (HBUF hbuf, HDB hdb, HMODULE hmod);

/* --- ret true if ok ---*/
BOOL DbFillListBuffer (HBUF hbuf, HDB hdb, HMODULE hmod);

BOOL EXPORT DbDelBuf(HBUF hbuf, HWND hwnd);

BOOL DbSendMsg (HWND hwnd, USHORT umsg, MPARAM mp1, MPARAM mp2);

