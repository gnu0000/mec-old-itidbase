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
 * dbkernel.h                                                           *
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

extern HLOGIN DbLogin (PSZ  pszAppName,
                       PSZ  pszHostName,
                       PSZ  pszUserName,
                       PSZ  pszPassword,
                       BOOL bBCP);

extern void DbFreeLogin (HLOGIN hlogin);


extern BOOL DbLogout (HDB hdb);

extern HDB  DbOpen (HLOGIN hlogin, PSZ pszServer, PSZ pszDatabase);

extern BOOL DbExecQuery (HDB hdb, PSZ pszQuery, USHORT *uCols, USHORT *uState);

/* returns TRUE if there are more results */
extern BOOL DbResults (HDB hdb);

extern void DbCancel (HDB hdb);

extern BOOL DbNextRow (HDB hdb, USHORT *uState);

extern BOOL DbGetCol (HDB hdb, USHORT uCol, PSZ *ppszCol, USHORT *uType, USHORT *uLen);

extern USHORT DbGetFmtCol (HDB    hdb,
                    USHORT uCol,
                    PSZ    pszFormat,
                    PSZ    pszCol,
                    USHORT uStrLen,
                    USHORT *puDestLen);

extern USHORT DbNumCols (HDB hdb);

/* --- col # start from 0 in my api --- */
extern PSZ    DbColName (HDB hdb, USHORT uCol);

extern USHORT DbColLen  (HDB hdb, USHORT uCol);

extern USHORT DbColType (HDB hdb, USHORT uCol);

extern USHORT DbDatLen  (HDB hdb, USHORT uCol);

/* -- this proc is optomized for the buffer module -- */
extern BOOL DbGetRowForBuffer(HDB      hdb,
                              PCOLDATA pcol,
                              PSZ      pszRow,
                              USHORT   uMaxLen,
                              USHORT   *puLen);

/*
 * returns 0 if error
 */
extern BOOL DbStrFmat (HDB    hdb,
                       PBYTE  pData,
                       USHORT uType,
                       PSZ    pszCol,
                       USHORT uStrLen);

/*
 * DbInit calls the dblib dbinit function.
 */
extern void DbInit (void);





/*
 * BcpSetTable returns TRUE if pszTableName in database hdb can be 
 * bulk loaded.
 */

extern BOOL BcpSetTable (HDB hdb, PSZ pszTableName);


extern BOOL BcpBindColumn (HDB    hdb,
                           PSZ    pszData,
                           USHORT usPrefixSize,
                           SHORT  sLength,
                           PSZ    pszTerminator,
                           USHORT usTerminatorSize,
                           USHORT usColumnOrder);

extern BOOL BcpSendRow (HDB hdb);


extern ULONG BcpTableDone (HDB hdb);
extern ULONG BcpBatch (HDB hdb);

