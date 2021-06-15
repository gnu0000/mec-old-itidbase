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
 * buflist.h                                                            *
 *                                                                      *
 * Copyright (C) 1990-1992 Info Tech, Inc.                              *
 *                                                                      *
 * Maintains linked list of buffers                                     *
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
 * This must be called before attempting to use
 * the linked list
 */
extern void DbInitLL (void);


USHORT DbIncRefCount (HBUF hbuf);
USHORT DbDecRefCount (HBUF hbuf);
USHORT DbIncCmdRefCount (HBUF hbuf);
USHORT DbDecCmdRefCount (HBUF hbuf);

/* 
 * Adds a new buffer to the list. If buffer is already created
 * its reference count is increased and the existing buffer is
 * returned.
 * this fn returns TRUE if the buffer is created and FALSE
 * if the buffer already exists.
 */
extern BOOL DbAddBufferToList (HWND   hwnd,
                               USHORT uId,
                               PSZ    pszNewQuery,
                               HBUF   *phbuf);


/*
 * removes hbuf. If the hbuf reference count > 1 the buffer is not
 * actually removed. This fn does NOT free any memory!.
 * this fn returns TRUE if the hbuf is removed.
 */
extern BOOL DbRemoveBufferFromList (HBUF hbuf, HWND hwnd);


/*
 * returns NULL if not found
 */
extern HBUF DbFindBuffer (USHORT uId);

