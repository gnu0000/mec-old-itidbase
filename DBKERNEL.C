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
 * dbkernel.c                                                           *
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

#define INCL_WIN
#include "..\INCLUDE\Iti.h"
#include "..\INCLUDE\itibase.h"
#include "..\INCLUDE\itiglob.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "..\INCLUDE\itifmt.h"
#include "dbase.h"
#include "dbkernel.h"
#include "dbutil.h"
#include "..\INCLUDE\itierror.h"
#include "..\include\itiutil.h"


#define MAX_DBK_QUERY_SIZE     2000

static CHAR szQryBuf[MAX_DBK_QUERY_SIZE-2] = "";



void DbInit (void)
   {
   ItiErrWriteDebugMessage (dbinit ());
   }



int API _loadds err_handler (DBPROCESS *dbproc, 
                             int       iSeverity, 
                             int       iDbError, 
                             int       iOsError, 
                             char      *pszDbErrorString, 
                             char      *pszOsErrorString)
   
   {
   char szTemp [250] = "";

   if ((dbproc == NULL) || (DBDEAD(dbproc)))
      {
      /* 4/17/1992 mdh: changed this to cancel, instead of INT_EXIT */
      return INT_CANCEL;
      }
   else
      {
      ItiErrWriteDebugMessage ("DB-Library error follows:");
      ItiErrWriteDebugMessage (pszDbErrorString);
      if (iOsError != DBNOERR)
         {
         ItiErrWriteDebugMessage ("OS/2 error follows: ");
         ItiErrWriteDebugMessage (pszOsErrorString);
         }
      if (iSeverity != 0)
         {
         sprintf (szTemp, "Severity = %d, dberr = %d, oserr = %d, dbproc = %p", 
                  iSeverity, iDbError, iOsError, dbproc);
         ItiErrWriteDebugMessage (szTemp);
         }
      return INT_CANCEL;
      }
   }

int API _loadds msg_handler (DBPROCESS    *dbproc, 
                             DBINT        lMessageNumber, 
                             DBSMALLINT   sMessageState, 
                             DBSMALLINT   sSeverity, 
                             char         *pszMessage)
   {
   char szTemp [256];

   if (!bVerbose && 0 == sSeverity)
      return 0;

   ItiErrWriteDebugMessage ("DB-Library message follows:");
   ItiErrWriteDebugMessage (pszMessage);
   if (0 != sSeverity)
      {
      sprintf (szTemp, "SQL Server message %ld, state %d, severity %d, dbproc = %p",
               lMessageNumber, sMessageState, sSeverity, dbproc);
      ItiErrWriteDebugMessage (szTemp);
      }

   return 0;
   }




/*
 * Logs on to DB-Lib
 * returns NULL on error
 */
HLOGIN DbLogin (PSZ  pszAppName,
                PSZ  pszHostName,
                PSZ  pszUserName,
                PSZ  pszPassword,
                BOOL bBCP)
   {
   LOGINREC *plogin;

   /* set time to wait for login to 5 seconds. -- mdh */
   if (FAIL == dbsetlogintime (5))
      {
      DbErr ("dbsetlogintime returned FAIL.  This shouldn't have happened.", 0);
      }

   plogin = dblogin();
   DBSETLAPP  (plogin, pszAppName); 
   DBSETLHOST (plogin, pszHostName);
   DBSETLUSER (plogin, pszUserName);
   DBSETLPWD  (plogin, pszPassword);
   BCP_SETL (plogin, (DBBOOL) bBCP);

   /* set up error handlers -- mdh */
   dberrhandle (err_handler);
   dbmsghandle (msg_handler);

   return plogin;
   }

void DbFreeLogin (HLOGIN hlogin)
   {
   dbfreelogin (hlogin);
   }



BOOL DbLogout (HDB hdb)
   {
   return TRUE;
   }



HDB DbOpen (HLOGIN hlogin, PSZ pszServer, PSZ pszDatabase)
   {
   HDB   hdb;

   if ((hdb = dbopen(hlogin, pszServer)) == NULL)
      {
      DbErr ("Unable to connect to the SQL Server.  Check with your system manager.", 0);
      return NULL;
      }

   if ((dbuse(hdb, pszDatabase)) == FAIL)
      {
      DbErr ("Unable to use the database.  Check with your system manager.", 0);
      dbclose (hdb);
      return NULL;
      }

   return hdb;
   }




BOOL DbExecQuery (HDB hdb, PSZ pszQuery, USHORT *pusCols, USHORT *pusState)
   {
   *pusCols = 0;

   if (pszQuery == NULL || *pszQuery == '\0')
      return FALSE;

   /* -- New, added JUN 95 for metric. */
   ItiDbReplacePGlobParams (szQryBuf, pszQuery, (MAX_DBK_QUERY_SIZE-2) );


//   while (ItiStrLen (pszQuery) > MAX_DBK_QUERY_SIZE)
   while (ItiStrLen (szQryBuf) > MAX_DBK_QUERY_SIZE)
      {
      char szTemp [MAX_DBK_QUERY_SIZE + 1];

//      ItiStrCpy (szTemp, pszQuery, sizeof szTemp);
      ItiStrCpy (szTemp, szQryBuf, sizeof szTemp);
      if ((*pusState = dbcmd (hdb, szTemp)) == FAIL)
         {
         DbErr ("Unable to send big query to SQL Server. ", *pusState);
         return FALSE;
         }

//      pszQuery += MAX_DBK_QUERY_SIZE;
      pszQuery += MAX_DBK_QUERY_SIZE;
      }

   /*--- passing the sql text to the dblib ---*/
//   if ((*pusState = dbcmd (hdb, pszQuery)) == FAIL)
   if ((*pusState = dbcmd (hdb, szQryBuf)) == FAIL)
      {
      DbErr ("Unable to send query to SQL Server. ", *pusState);
      return FALSE;
      }

   if ((*pusState = dbsqlexec (hdb)) == FAIL)
      {
      DbErr ("Unable to execute query.", *pusState);
      dbcancel (hdb);
      return FALSE;
      }

   if ((*pusState = dbresults (hdb)) != SUCCEED)
      {
//      DbSUSErr ("Query did not succeed in executing.", *pusState, pszQuery);
      DbSUSErr ("Query did not succeed in executing.", *pusState, szQryBuf);
      dbcancel (hdb);
      return FALSE;
      }
   *pusCols = dbnumcols (hdb);
   return TRUE;
   }


/* returns TRUE if there are more results */
BOOL DbResults (HDB hdb)
   {
   return SUCCEED == dbresults (hdb);
   }



BOOL DbNextRow (HDB hdb, USHORT *pusState)
   {
   if ((*pusState = dbnextrow(hdb)) != MORE_ROWS)
      return FALSE;
   return TRUE;
   }



BOOL DbGetCol (HDB      hdb,
               USHORT   usCol,
               PSZ      *ppszCol,
               USHORT   *pusType,
               USHORT   *pusLen)
   {
   *ppszCol = (PSZ) dbdata  (hdb, usCol + 1);
   *pusLen  = (USHORT) DbDatLen  (hdb, usCol);
   *pusType = (USHORT) DbColType (hdb, usCol);
   return *pusLen != 0;
   }


static SHORT Unround (PSZ pszNumber, SHORT sPos)
   {
   if (sPos < 0)
      return sPos;
   else if (pszNumber [sPos] > '9' || pszNumber [sPos] < '0')
      return Unround (pszNumber, sPos - 1);
   else if (pszNumber [sPos] == '0')
      {
      pszNumber [sPos] = '9';
      return Unround (pszNumber, sPos - 1);
      }
   else
      {
      pszNumber [sPos]--;
      return sPos;
      }
   }


/* like above proc except that the col is converted to a string
 *
 * returns 0 if successful
 *
 */
USHORT DbGetFmtCol (HDB    hdb,
                    USHORT usCol,
                    PSZ    pszFormat,
                    PSZ    pszCol,
                    USHORT usStrLen,
                    USHORT *pusDestLen)
   {
   BYTE     *pData;
   SHORT    iLen;
   USHORT   usLen, usType;
   CHAR     szTemp [257];
   DBMONEY  bucks;

   szTemp[0] = pszCol[0] = '\0';
   if (DbGetCol (hdb, usCol, &pData, &usType, &usLen))
      iLen = dbconvert (hdb, usType, pData, usLen, SQLCHAR, szTemp, sizeof szTemp);
   else
      iLen = 0;

   if (iLen == -1)
      return 2;

   if (usType == SQLMONEY &&
       -1 != dbconvert (hdb, usType, pData, usLen, SQLMONEY, 
                        (PVOID) &bucks, sizeof bucks))
      {
      /* we need to get the 1/100ths and 1/1000ths place, since DB-Lib
         doesn't think that they are important */
      szTemp [iLen++] = (char) ((bucks.mnylow /  10) % 10) + (char) '0';
      szTemp [iLen++] = (char) ((bucks.mnylow)       % 10) + (char) '0';
      if ((bucks.mnylow % 100) >= 50)
         Unround (szTemp, iLen - 3);
      }

   szTemp[iLen] = '\0';

   ItiFmtFormatString (szTemp,       // source string in char fmat
                       pszCol,      // destination str
                       usStrLen,     // max dest len
                       pszFormat,   // format string
                       pusDestLen);  // actual dest length
   return 0;
   }


void DbCancel (HDB hdb)
   {
   dbcancel (hdb);
   }


USHORT DbNumCols (HDB hdb)
   {
   return dbnumcols (hdb);
   }

PSZ DbColName (HDB hdb, USHORT usCol)
   {
   return (PSZ) dbcolname (hdb, usCol + 1);
   }

USHORT DbColLen  (HDB hdb, USHORT usCol)
   {
   return (USHORT)dbcollen (hdb, usCol + 1);
   }

USHORT DbDatLen  (HDB hdb, USHORT usCol)
   {
   return (USHORT)dbdatlen (hdb, usCol + 1);
   }

USHORT DbColType (HDB hdb, USHORT usCol)
   {
   return (USHORT)dbcoltype (hdb, usCol + 1);
   }




/* -- this proc is optomized for the buffer module -- */
extern BOOL DbGetRowForBuffer(HDB      hdb,
                              PCOLDATA pcol,
                              PSZ      pszRow,
                              USHORT   usMaxLen,
                              USHORT   *pusLen)

   {
   USHORT      i, usNumCols, usState, usDestLen;

   *pszRow = '\0';
   *pusLen = 0;

   if (!DbNextRow (hdb, &usState))
      return FALSE;

   usNumCols = DbNumCols (hdb);
   for (i = 0; i < usNumCols; i++)
      {
      DbGetFmtCol (hdb,
                   i,
                   pcol[i].pszFormat,
                   pszRow + *pusLen,
                   usMaxLen - *pusLen,
                   &usDestLen);

      *pusLen += usDestLen + 1;
      }
   return TRUE;
   }


/*
 * BcpSetTable returns TRUE if pszTableName in database hdb can be 
 * bulk loaded.
 */

BOOL BcpSetTable (HDB hdb, PSZ pszTableName)
   {
   return SUCCEED == bcp_init (hdb, pszTableName, NULL, NULL, DB_IN);
   }


BOOL BcpBindColumn (HDB    hdb,
                    PSZ    pszData,
                    USHORT usPrefixSize,
                    SHORT  sLength,
                    PSZ    pszTerminator,
                    USHORT usTerminatorSize,
                    USHORT usColumnOrder)
   {
   return SUCCEED == bcp_bind (hdb,
                               pszData,
                               usPrefixSize,
                               sLength,
                               pszTerminator,
                               usTerminatorSize,
                               SQLCHAR,
                               usColumnOrder);
   }

BOOL BcpSendRow (HDB hdb)
   {
   return SUCCEED == bcp_sendrow (hdb);
   }


ULONG BcpTableDone (HDB hdb)
   {
   return bcp_done (hdb);
   }

ULONG BcpBatch (HDB hdb)
   {
   return bcp_batch (hdb);
   }
