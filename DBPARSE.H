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
 * dbparse.h                                                            *
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

extern BOOL DbTablesFromQuery (PSZ     pszQuery, 
                               HHEAP   hheap,  
                               PSZ     **pppszTables,
                               USHORT  *uCount);

// DBLIB (dbkernel) function provides this
//extern BOOL DbColumnsFromQuery(PSZ     pszQuery, 
//                               HHEAP   hheap, 
//                               PSZ     **pppszCols);
//                               USHORT  *uCount);


extern BOOL ReplaceParams (PSZ pszStr, HBUF hbuf, USHORT uRow, USHORT uMaxSize);





/*
 * DbGetCountQuery creates a SELECT COUNT (*) query based on the 
 * SQL statement pszQuery.  pszQuery must be either a SELECT 
 * or a DELETE FROM SQL statement.  If the word DISTINCT comes
 * after SELECT, then the DISTINCT is also placed in the count 
 * query.
 * 
 * Currently, ORDER BYs and GROUP BYs are not stripped from the
 * query.
 *
 * The query is allocated from heap hheap, and it is the caller's 
 * responsibility to free the query.
 */

extern PSZ DbGetCountQuery (HHEAP hheap, PSZ pszQuery);
