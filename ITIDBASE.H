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


/*
 * ItiDBase.h
 * Judy A. Siggelkow
 * Craig Fitzgerald
 * This module is part of DS/Shell (CS/Base)
 *
 */

#if !defined (DATABASE_INCLUDED)
#define DATABASE_INCLUDED


/* --- commands used by QueryBuffer and QueryQuery ---*/
#define ITIDB_ID            1
#define ITIDB_TYPE          2
#define ITIDB_HWND          3
#define ITIDB_NUMROWS       5
#define ITIDB_NUMCOL        6
#define ITIDB_NUMTABLES     7
#define ITIDB_QUERY         8
#define ITIDB_COLID         9
#define ITIDB_TABLENAME     10
#define ITIDB_COLTYPE       11
#define ITIDB_HMODULE       12
#define ITIDB_RESID         13
#define ITIDB_NEXTROW       14
#define ITIDB_COLMAXLEN     15
#define ITIDB_COLFMT        16
#define ITIDB_HWNDCOUNT     17

/* --- messages broadcast while creating/filling/deleting buffers --- */
#define LINESPERMSG         16

#define WM_ITIDBAPPENDEDROWS  (DBMSG_START + 0)
#define WM_ITIDBCHANGED       (DBMSG_START + 1)
#define WM_ITIDBDELETE        (DBMSG_START + 2)
#define WM_ITIDBBUFFERDONE    (DBMSG_START + 3)
#define WM_ITIDBGRAY          (DBMSG_START + 4)

/*********************** buffer struct *******************************/

/* --- return types from GetRow --- */
#define ITIDB_VALID           0
#define ITIDB_INVALID         1
#define ITIDB_WAIT            2
#define ITIDB_GRAY            3

///* --- buffer types --- */
//#define ITI_ADD          0
//#define ITI_LIST         1
//#define ITI_DIALOG       2



#if defined (DATABASE_INTERNAL)

typedef LOGINREC  *HLOGIN;
typedef DBPROCESS *HDB;
typedef DBPROCESS **PHDB;

#else

typedef PVOID HLOGIN;

typedef PVOID HDB;

#endif


/* ---COLDATA---
 * This structure maintains anything you could possibly want
 * to know about a column in the buffer (and more) the data
 * structure is an array of these structures as an element in
 * DBBUF
 */
typedef struct
   {
   USHORT   uColId;       /* -- id of column name -- */
   PSZ      pszFormat;
   USHORT   uMaxLen;
   USHORT   uColType;
   } COLDATA;
typedef COLDATA *PCOLDATA;
   


/* ---ROWIDX---
 * This structure is used to hold the offset to a row
 * and to store the state of the row.
 * These structures are allocated as an array, 1 per row
 * in the current buffer seg. 
 */
typedef struct
   {
   USHORT   uOffset;
   USHORT   uState;
   } ROWIDX;
typedef  ROWIDX *PROWIDX;



/* ---BUFHDR---
 * This structure holds information about a specific buffer seg
 * there is one of these for each buffer segments.
 */
typedef struct
   {
   USHORT   uFirst;
   USHORT   uNumRows;
   PSZ      pszBuffer;
   PROWIDX  pridx;
   } SEGHDR;
typedef SEGHDR *PSEGHDR;



typedef struct ItiDbBuf
   {
   HSEM        hsemWriteMutex;   /* set when being updated          */
   HSEM        hsemReadMutex;    /* set to disallow reading         */
                                 /* only set within WriteMutex!!!   */
   USHORT      uRefCount;        /* only modified by buflist.c      */
   USHORT      uCmdRefCount;     /* only modified by buflist.c      */

   HWND        *hwndUsers;       /* array of using windows          */
   USHORT      uUsers;           /* size of above array             */

   USHORT      uId;              /* ID for this buffer              */
   USHORT      uType;            /* type of this buffer             */
   struct ItiDbBuf *Next;        /* next HBUF in linked list        */
   USHORT      uNumRows;         /* total num rows in query         */
   USHORT      uSegCount;        /* # buffer segments used for data */
   PSZ         *ppszTables;      /* table names from query          */
   USHORT      uNumTables;       /* number of tables used in query  */
   USHORT      uNumCols;         /* number of columns returned      */
   PCOLDATA    pcol;             /* ptr to column info array        */
   PSZ         pszQuery;         /* ptr to query string             */
   PSEGHDR     pseghdr;          /* ptr to SEGHDR array             */
   BOOL        bDlgRes;          /* 0=reg fmat 1=fmt from :dialog res*/
   } DBBUF;

typedef DBBUF  BUF;
typedef BUF  *HBUF;



typedef struct
   {
   HDB         hdb;
   HHEAP       hheap;
   PCOLDATA    pcol;             /* ptr to column info array        */
   USHORT      uNumCols;
   } QRY;
typedef QRY *HQRY;




/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

/* Proposed New minimal DB api */




/*    -------- MESSAGES GENERATED BY BUFFER FOR WINDOWS --------
----------------------------------------------------------------------------
| LIST WINDOW             | -MP1-      | -MP2- | -comments-                |
|   MESSAGE               | L    H L   |       |                           |
|-------------------------+------------+-------+---------------------------|
| WM_ITIDBAPPENDEDROWS    | type uRows | HBUF  | generated during fill     |
| WM_ITIDBBUFFERDONE      | type uRows | HBUF  | generated after fill      |
| WM_ITIDBCHANGED         | type uId   | HBUF  | start of buffer filling   |
| WM_ITIDBDELETE          | type uId   | HBUF  | when buffer is going away |
----------------------------------------------------------------------------
| Where type refers to the type of window which will receive the           |
|  message (I.E. ITI_ADD or ITI_LIST). This tells the frame         |
|  windows where to broadcast the message to                               |
| The unique entity for the message id HBUF rather than the buffer Id      |
|  because there may be several buffers/windows with the same buffer id    |
----------------------------------------------------------------------------
*/


/*
 * This fn creates a buffer of pre-formatted data ready for display.
 * This fn is multi-threaded and returns immediately.
 * Error and Buffer-being-filled messages are broadcast(post) to the windows
 *    while buffer is being created.
 * Buffers update automatically in responce to table-data changes.
 * Updating buffers broadcasts(post) an add/del/update message, make the
 *    change, and send a done msg.
 * This fn does not create a buffer if one already exists with the
 *    same uId, but will broadcast(post) the necessary startup messages.
 *
 * --- PARAMS ---
 * uId    ...... ID for buffer also used to retrieve format metadata
 * hheap  ...... hheap to use for storing buffer information & ptrs
 * uBufferType.. tells list/single-row type (single rows have no
 *               separate buff seg)
 *
 * --- RETURNS ---
 * HBUF   ...... the handle to the buffer for subsequent calls.
 *       This procedure also broadcasts(post) messages while getting data:
 * error message .... if an error occurred. if it is a terminal error
 *                    a delete buffer msg is also generated 
 * got rows/current row count .... sends this message every N rows telling
 * the window that this data is now ready to display. this message also
 * tells how many rows there are so far.
 *
 */

extern HBUF EXPORT ItiDbCreateBuffer(HWND   hwnd,
                                     USHORT uId,
                                     USHORT uBuffType,
                                     PSZ    pszQuery,
                                     USHORT *uNumRows);

/*
 * Deletes a buffer, broadcasts(send) msg that it is doing so
 */

extern BOOL EXPORT ItiDbDeleteBuffer (HBUF hbuf, HWND hwnd);


//extern BOOL EXPORT ItiDbRedoQuery (HBUF hbuf);
//extern void EXPORT ItiDbRedoQuerys (USHORT uBuffId);


/* This fn gets a row from the buffer.
 *       RETURNS
 * PSZ ..... columns in txt format separated by nulls
 * uState .. ITIDB_VALID   data is ok
 *           ITIDB_INVALID uRow is out of range
 *           ITIDB_WAIT    data is not yet in buffer
 *           ITIDB_GRAY    data is displayable but marked for change (so gray)
 */

//extern PSZ EXPORT ItiDbGetBufferRow (HBUF   hbuf,
//                                     USHORT uRow,
//                                     USHORT *uState);


extern USHORT EXPORT ItiDbGetBufferRowCol(HBUF   hbuf,
                                          USHORT uRow,
                                          USHORT uCol,
                                          PSZ    pszCol);





/* This fn retrieves information about a window buffer.
 *
 *  uCmd              uSubCmd          MRESULT
 *  USHORT            USHORT    LOWWORD        
 *-------------------------------------------------------------------------
 *| ITIDB_ID        | 0       | uId             | ID of buffer            |
 *| ITIDB_TYPE      | 0       | uType           | type of buffer          |
 *| ITIDB_HWND      | index   |        hwnd     | root of broadcast tree  |
 *| ITIDB_HWNDCOUNT | 0       | uWindows        | num of windows w/msgs   |
 *| ITIDB_HHEAP     | 0       |        hheap    | heap of stored data     |
 *| ITIDB_NUMROWS   | 0       | uRows           | current num rows (or 0) |
 *| ITIDB_NUMCOL    | 0       | uCols           | current num cols        |
 *| ITIDB_NUMTABLES | 0       | uTables         | tables in query         |
 *| ITIDB_QUERY     | 0       |        pszQuery | query string            |
 *| ITIDB_COLNAME   | uCol    |        pszCol   | name of column n        |
 *| ITIDB_COLID     | uCol    | uId             | id of  column n         |
 *| ITIDB_TABLENAME | uTable  |        pszTab   | name of table n         |
 *| ITIDB_COLTYPE   | uColId  | uColType        | type of column n        |
 *| ITIDB_COLMAXLEN | uColId  | uColType        | maxlen of column n      |
 *| ITIDB_COLFMT    | uColId  | uColType        | format str of column n  |
 *-------------------------------------------------------------------------
 */                             
extern MRESULT EXPORT ItiDbQueryBuffer (HBUF   hbuf,
                                        USHORT uCmd,
                                        USHORT uSubCmd);



/*
 *    This procedure replaces the %colid 
 *  with the value in that column in row uRow
 *  this is for child querys and child window
 *  titles
 *    hbufParent is checked for the column first.
 *  If it does not contain the needed col. hbufStatic
 *  is used.
 *
 *   pszStr ... query string to fix
 *   hbuf   ... buffer with replacement source
 *   uRow   ... row in hbuf to use
 */
extern BOOL EXPORT ItiDbReplaceParams (PSZ    pszDest,
                                       PSZ    pszSrc,
                                       HBUF   hbufParent,
                                       HBUF   hbufStatic,
                                       USHORT uBuffParentRow,
                                       USHORT uMaxSize);


/* This only replaces the following params    */
/* with values from pglobals.                 */
/*       %CurUserKey                          */
/*       %CurUnitType                         */
/*       %CurSpecYear                         */
/*                                            */     
extern BOOL EXPORT ItiDbReplacePGlobParams (PSZ    pszDest,
                                     PSZ    pszSrc,
                                     USHORT uMaxSize);


extern BOOL EXPORT ItiDbColExists (HBUF hbuf, USHORT uColId);


/* returns true if any buffers are marked for change
 */
extern BOOL EXPORT ItiDbUpdateBuffers (PSZ pszChangedTable);

/*************************************************************************/
/*************************************************************************/
/*************************************************************************/


extern PSZ *EXPORT ItiDbGetRow1 (PSZ       pszQuery,
                                 HHEAP     hheap, 
                                 HMODULE   hmod,    
                                 USHORT    usResId,  
                                 USHORT    usId,
                                 PUSHORT   pusNumCols);


extern PSZ EXPORT ItiDbGetRow1Col1 (PSZ       pszQuery,
                                    HHEAP     hheap, 
                                    HMODULE   hmod,    
                                    USHORT    usResId,  
                                    USHORT    usId);


/* This procedure executes a query and checks for results.
 * This procedure is single threaded and will wait for a free connection,
 *    exec query, and process dbresults before returning to  the caller.
 *
 *  --- PARAMS ---
 *
 *  pszQuery .. the query string
 *  uId ....... id for formatting metadata (0 = no formatting)
 *
 *  --- RETURNS ---
 * uNumCols ... the number of columns in the returned rows
 * uError ..... the error value (call ItiDbGetQueryError) for str.
 */

extern HQRY EXPORT ItiDbExecQuery (PSZ     pszQuery,
                                   HHEAP   hheap,  
                                   HMODULE hmod,    
                                   USHORT  uResId,  
                                   USHORT  uId,     
                                   USHORT  *puNumCols,
                                   USHORT  *puState);


/* This fn gets the next row from the query.
 *
 *  --- RETURNS ---
 * BOOL ... TRUE if a row has been gotten, FALSE if done. This proc
 *             should always be called until FALSE is returned or
 *             ItiDbTerminateQuery is called.
 * pppsz ...Newly allocated ptr to array of string ptrs with formatted
 *             row data.
 */

extern BOOL EXPORT ItiDbGetQueryRow (HQRY   hqry,
                                     PSZ    **pppsz,
                                     USHORT *uError);





/* This fn returns the number of rows that will be returned
 *    when this query is executed.
 * This fn modifies the query to select a count (*) and removes
 *    the order by clause.
 * This function cancels the query before returning.
 * On error uError returns an error code. This is all you get for
 *    an error (I.E. QueryError CANNOT be called because you don't
 *    get a HQRY, the error is already cleared, and the query is
 *    already canceled.)
 * This fn should only be called when you really need it because it
 * is as slow as the actual query and busies the database module.
 */

extern USHORT EXPORT ItiDbGetQueryCount (PSZ     pszQuery,
                                         USHORT  *uNumCols,
                                         USHORT  *uError);





/* This fn returns a string describing the error that had just occurred
 * this error is cleared.
 */

extern void EXPORT ItiDbGetQueryError (HQRY    hqry,
                                       PSZ     pszErrStr,
                                       USHORT  uStrLen);




/*
 * This fn terminates the query request (I.E after an error)
 */

extern void EXPORT ItiDbTerminateQuery (HQRY hqry);




/* This fn retrieves information about a query
 *
 *  uCmd              uSubCmd          MRESULT
 *  USHORT            USHORT    LOWWORD        
 *-------------------------------------------------------------------------
 *| ITIDB_HMODULE   | 0       |        hmod     | ID of buffer            |
 *| ITIDB_RESID     | 0       | uResId          | ID of buffer            |
 *| ITIDB_ID        | 0       | uId             | ID of buffer            |
 *| ITIDB_NEXTROW   | 0       | uNextRow        | current num rows (or 0) |
 *| ITIDB_NUMCOL    | 0       | uCols           | current num cols        |
 *| ITIDB_NUMTABLES | 0       | uTables         | tables in query         |
 *| ITIDB_QUERY     | 0       |        pszQuery | query string            |
 *| ITIDB_COLNAME   | uCol    |        pszCol   | name of column n        |
 *| ITIDB_TABLENAME | uTable  |        pszTab   | name of table n         |
 *| ITIDB_COLTYPE   | uCol    | uColType        | type of column n        |
 *-------------------------------------------------------------------------
 */                             
extern MRESULT EXPORT ItiDbQueryQuery (HQRY   hqry,
                                       USHORT uCmd,
                                       USHORT uSubCmd);







extern USHORT EXPORT ItiDbFindRow (HBUF hbuf,      // buffer to look in
                                   PSZ pszMatch,   // matching val
                                   USHORT uColId,  // col to look in
                                   USHORT uStart); // row to start at



/*************************************************************************/
/*************************************************************************/
/*************************************************************************/




/*
 * This fn initializes the dbbuffer module
 * usNonBlocking is the number of non-blocking database connections.
 * usBlocking is the number of blocking database connections.
 *    to use in this process.
 */

extern BOOL EXPORT ItiDbInit (USHORT usNonBlocking, USHORT usBlocking);




/*
 * This fn shuts down the read and write connections
 */

extern void EXPORT ItiDbTerminate (void);



/*************************************************************************/
/*************************************************************************/
/*************************************************************************/

/* Utility procedures  */

/*
 * ItiDbExecSQLStatement sends an SQL statement to the server. 
 * This function is usefull if you don't care about results from the
 * server.
 *
 * Return value: 0 if no errors, otherwise a number whose meaning
 * is unknown is returned.
 */

extern USHORT EXPORT ItiDbExecSQLStatement (HHEAP hheap, 
                                            PSZ   pszSQLStatement);


/*
 * ItiGetTableFromInsert returns the table that is being inserted
 * into from the given SQL statment.  
 * The table name is copied into pszTable.
 */

extern BOOL EXPORT ItiDbGetTableFromInsert (PSZ    pszInsert,
                                            PSZ    pszTable,
                                            USHORT usTableMax);

extern BOOL EXPORT ItiDbUpdateDeletedBuffers (PSZ pszDelete);


/* Update procedures  */

extern BOOL EXPORT ItiDbBeginTransaction (HHEAP hheap);

extern BOOL EXPORT ItiDbCommitTransaction (HHEAP hheap);

extern BOOL EXPORT ItiDbRollbackTransaction (HHEAP hheap);


/*
 * ItiDbGetKey returns a key for the specified table.  
 *
 * WARNING: This uses a direct, non blocking connection to the server.
 *
 * Returns NULL on error, or a pointer to a string containing the 
 * key value.  The string was allocated from hheap, and the caller is
 * responsible for freeing it.
 */

extern PSZ EXPORT ItiDbGetKey (HHEAP   hheap, 
                               PSZ     pszTableName,
                               ULONG   ulNumKeys);


/*************************************************************************/
/*************************************************************************/
/*************************************************************************/


extern void EXPORT ItiDbQButton (HWND hwnd, HBUF hbuf);


#define ITIDB_MIN_DB_QUERY    1
#define ITIDB_MAX_SPEC_YEAR   1
#define ITIDB_MAX_BASE_DATE   2
#define ITIDB_CUR_SPEC_YEAR   3
#define ITIDB_CUR_UNIT_SYS    4
#define ITIDB_MAX_DB_QUERY    4


extern PSZ EXPORT ItiDbGetDbValue (HHEAP hheap, USHORT usDbValType);



/**********************************************************************/
/* BCP API                                                            */
/**********************************************************************/

/* TRUE if OK, FALSE if error */
extern BOOL EXPORT ItiBcpInit (void);


/* TRUE if OK, FALSE if error */
extern BOOL EXPORT ItiBcpSetTable (PSZ pszTableName);


/*
 * ItiBcpBindColumn binds a column to a table.  ItiDbSetBcpTable must
 * be called first.
 *
 * Parameter: usColumnOrder         The order of the column in the
 *                                  database.
 *
 *            pszData               The location in memory of the data.
 *                                  The data is assumed to be in a null
 *                                  terminated string
 *
 * Return Value: TRUE if OK, FALSE if error.
 */

extern BOOL EXPORT ItiBcpBindColumn (USHORT usColumnOrder, PSZ pszData);

extern BOOL EXPORT ItiBcpSendRow (void);

extern ULONG EXPORT ItiBcpBatch (void);

/* returns number of rows inserted */
extern ULONG EXPORT ItiBcpTableDone (void);


/*
 * This function returns a pointer to the FROM clause of an SQL statement.
 * Note that any "FROM"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

extern PSZ EXPORT ItiDbFindFromClause (PSZ pszQuery);


/*
 * This function returns a pointer to the SELECT clause of an SQL statement.
 * Note that any "SELECTS"s in quotes, or double quotes are ignored.
 */

extern PSZ EXPORT ItiDbFindSelectClause (PSZ pszQuery);


/*
 * This function returns a pointer to the WHERE clause of an SQL statement.
 * Note that any "WHERE"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

extern PSZ EXPORT ItiDbFindWhereClause (PSZ pszQuery);


/*
 * This function returns a pointer to the ORDER BY clause of an SQL statement.
 * Note that any "ORDER BY"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

extern PSZ EXPORT ItiDbFindOrderByClause (PSZ pszQuery);


/*
 * This function returns a pointer to the GROUP BY clause of an SQL statement.
 * Note that any "GROUP BY"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

extern PSZ EXPORT ItiDbFindGroupByClause (PSZ pszQuery);






/*
 * This is the external function used to perform all calculations.
 *
 * pfnCalc ....... Ptr to calc function
 * *ppszParams ... key values
 * *ppszPNames ... key names
 * uPriority ..... priority to use for queue. the new queue entry will
 *                 be inserted in front of all entrys with a lower priority.
 *                 0 = default priority (currently 100)   
 *
 * returns insert position relative to tail (as if anyone really cared)
 *
 */

typedef int (* _pascal PFNCALCFN) (HHEAP hheap, PSZ *ppszPNames, PSZ *ppszParams);

USHORT EXPORT ItiDBDoCalc ( PFNCALCFN pfnCalc,
                            PSZ       *ppszPNames,
                            PSZ       *ppszParams,
                            USHORT    uPriority);
   





extern PSZ EXPORT ItiDbGetZStr (PSZ pszZStr, USHORT uIndex);



/*
 * This function returns columns for all selected rows for a given
 * LIST window. the columns to return are specified by a zero
 * terminated array of colId's (add 0x8000 to get positional cols)
 *
 *
 * this function returns a pointer
 * to newly allocated data that looks like this:
 *
 *  ----->|-|    ----------------   This is a null terminated
 *        | |--->|   0    0   00|   array of pointers to ZStrings
 *        |_|    ----------------
 *        | |    ----------------   A ZString is an array of chars
 *        | |--->|   0    0   00|   that contains \0 terminated
 *        |_|    ----------------   strings and id \0\0 terminated
 *        | |    ----------------   (like say env. vars)
 *        | |--->|   0    0   00|
 *        |_|    ----------------
 *        | |
 *        |0|
 *        |_|
 *
 *
 * Example:
 *
 *  USHORT PleaseKillMyCatProc (USHORT uWeaponIndex)
 *    {
 *    PSZ   pszSIC;
 *    PSZ   *ppszKeys;
 *    USHORT uCols = {uJobKey, uStandardItemKey, 0};
 *
 *    .
 *    .
 *    ItiDbBuildSelectedKeyArray (hwnd, hheap, &ppszKeys, uCols);
 *    .
 *    .
 *    for (i=0; ppszKeys[i] != NULL; i++)
 *       {
 *       pszSIC = ItiDbGetZStr (ppszKeys[0], 1)
 *       }
 *
 */
extern BOOL EXPORT ItiDbBuildSelectedKeyArray (HWND    hwnd,
                                               HHEAP   hheap,
                                               PSZ     **pppszKeys,
                                               USHORT  uKeys[]);


/* see above function for general usage. This function is like the
 * one above except it uses a query string rather than a window
 * buffer, and columns are specified positionally rather than by name.
 *
 * Errorcode return values:
 *
 * 0 - no error
 * 1 - DB Exec Error
 * 2 - DB Exec Error
 * 3 - you asked for a column greater than the number of columns
 *
 */

extern USHORT EXPORT ItiDbBuildSelectedKeyArray2 (HHEAP    hheap,
                                           HMODULE  hmod,
                                           PSZ      pszQuery,
                                           PSZ      **pppszKeys,
                                           USHORT   uKeys[]);


extern VOID EXPORT ItiDbAddZStr (HHEAP hheap, PSZ *ppszZStr, PSZ psz);



extern USHORT EXPORT ItiDbFreeZStrArray (HHEAP hheap, PPSZ ppszKeys);



extern USHORT EXPORT ItiDbXlateKeys (HHEAP hhp, PSZ pszInBuffer, PSZ pszOutBuffer, USHORT usOutSize);

#endif
