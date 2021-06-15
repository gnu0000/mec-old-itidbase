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
 * update.c                                                             *
 *                                                                      *
 * Copyright (C) 1990-1992 Info Tech, Inc.                              *
 * Mark Hampton                                                         *
 *                                                                      *
 * This module handles updates to the database magically.               *
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

#define DATABASE_INTERNAL
#include "..\include\iti.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include "..\include\itibase.h"
#include "..\include\itidbase.h"
#include "..\include\itimbase.h"
#include "..\include\itiutil.h"
#include "..\include\itierror.h"
#include "..\include\itiglob.h"
#include <limits.h>
#include <stdio.h>
#include "dbkernel.h"
#include "update.h"
#include "dbase.h"





BOOL EXPORT ItiDbBeginTransaction (HHEAP hheap)
   {
//   return 0 == ItiDbExecSQLStatement (hheap, " BEGIN TRANSACTION ");
   return TRUE;
   }


BOOL EXPORT ItiDbCommitTransaction (HHEAP hheap)
   {
//   return 0 == ItiDbExecSQLStatement (hheap, " COMMIT TRANSACTION ");
   return TRUE;
   }


BOOL EXPORT ItiDbRollbackTransaction (HHEAP hheap)
   {
//   return 0 == ItiDbExecSQLStatement (hheap, " ROLLBACK TRANSACTION ");
   return TRUE;
   }



BOOL EXPORT ItiDbUpdateDeletedBuffers (PSZ pszDelete)
   {
   PSZ      pszWord;
   BOOL     bFound;
   PSZ      pszTemp;
   PSZ      pszTemp2;
   USHORT   usSize;

   /* make a copy of the insert string, since ItiStrTok clobbers the
      input string */
   usSize = ItiStrLen (pszDelete) + 1;
   pszTemp = ItiMemAllocSeg (usSize, 0, 0);
   if (pszTemp == NULL)
      return FALSE;
   ItiStrCpy (pszTemp, pszDelete, usSize);

   bFound = FALSE;
   pszWord = ItiStrTok (pszTemp, " \t\n\r(");
   while (pszWord != NULL)
      {
      if (0 == ItiStrICmp (pszWord, "delete"))
         {
         pszWord = ItiStrTok (NULL, " \t\n\r(");
         if (0 == ItiStrICmp (pszWord, "from"))
            pszWord = ItiStrTok (NULL, " \t\n\r(");

         /* strip off server, database, and owner names */
         if ((pszTemp2 = ItiStrRChr (pszWord, '.')) != NULL)
            pszWord = pszTemp2 + 1;
   
         /* update the buffers */
         ItiDbUpdateBuffers (pszWord);
         }
      else if (0 == ItiStrICmp (pszWord, "update"))
         {
         pszWord = ItiStrTok (NULL, " \t\n\r(");

         /* strip off server, database, and owner names */
         if ((pszTemp2 = ItiStrRChr (pszWord, '.')) != NULL)
            pszWord = pszTemp2 + 1;
   
         /* update the buffers */
         ItiDbUpdateBuffers (pszWord);
         }
      else
         {
         pszWord = ItiStrTok (NULL, " \t\n\r(");
         }
      }

   ItiMemFreeSeg (pszTemp);
   }


static PSZ pszGetKeyLock = NULL;
static PSZ pszGetKeyValue = NULL;
static PSZ pszGetKeyCommit = NULL;


/*
 * ItiDbGetKey returns a key for the specified table.  
 *
 * WARNING: This uses a direct, non blocking connection to the server.
 *
 * Returns NULL on error, or a pointer to a string containing the 
 * key value.  The string was allocated from hheap, and the caller is
 * responsible for freeing it.
 */

PSZ EXPORT ItiDbGetKey (HHEAP   hheap, 
                        PSZ     pszTableName,
                        ULONG   ulNumKeys)
   {
   USHORT   usColumns, usState;
   HQRY     hqry;
   PSZ      *ppszData;
   PSZ      pszKey;
   char     szQuery [1024];
   ULONG    ulKey;

   if (ulNumKeys == 0)
      ulNumKeys = 1;

   /* begin the transaction */
   ItiDbBeginTransaction (hheap);

   /* lock the row */
   sprintf (szQuery, pszGetKeyLock, pszTableName);
   ItiDbExecSQLStatement (hheap, szQuery);

   /* get the value */
   sprintf (szQuery, pszGetKeyValue, pszTableName);
   hqry = ItiDbExecQuery (szQuery, hheap, 0, 0, 0, &usColumns, &usState);
   if (hqry == NULL) 
      return NULL;

   for (pszKey = NULL; ItiDbGetQueryRow (hqry, &ppszData, &usState);)
      {
      if (pszKey != NULL)
         ItiMemFree (hheap, pszKey);
      pszKey = ppszData [0];
      ItiMemFree (hheap, ppszData);
      }

   if (pszKey == NULL ||
       !ItiStrToULONG (pszKey, &ulKey) || 
       ulKey > ULONG_MAX - ulNumKeys)
      {
      ItiDbRollbackTransaction (hheap);
      if (pszKey)
         ItiMemFree (hheap, pszKey);
      return NULL;
      }

   sprintf (szQuery, pszGetKeyCommit, ulNumKeys, pszTableName);

   hqry = ItiDbExecQuery (szQuery, hheap, 0, 0, 0, &usColumns, &usState);
   if (hqry == NULL) 
      return NULL;

   while (ItiDbGetQueryRow (hqry, &ppszData, &usState))
      ItiFreeStrArray (hheap, ppszData, usColumns);

   /* commit the transaction */
   ItiDbCommitTransaction (hheap);

   return pszKey;
   }





static PSZ *apszGetDbVal;

static PSZ apszDbValPats[10];
static PSZ apszDbValQrys[10];

static CHAR sz0[180] = "";
static CHAR sz1[128] = "";
static CHAR sz2[32] = "";
static CHAR sz3[32] = "";
static CHAR sz4[80] = "";
static CHAR sz5[128] = "";
static CHAR sz6[128] = "";
static CHAR sz7[180] = "";
static CHAR sz8[15] = "";
static CHAR sz9[15] = "";

// 0
static char szGetMaxSpecYear2 [180] = 
   "SELECT MAX (SpecYear) "
   " FROM %s..StandardItem "
   " WHERE UnitType = %s OR"
   " UnitType = NULL ";

// 1
static char szGetMaxBaseDate [128] =
   "SELECT \"'\"+CONVERT(varchar,MAX (BaseDate),109)+\"'\" "
   "FROM %s..BaseDate ";

// 2
static char szGetCurSpecYear [32] =
     "SELECT %s ";

// 3
static char szGetCurUnitSys [32] =
   "SELECT  %s ";

// 4 
static char szGetMaxSpecYear1 [80] = 
   "SELECT MAX (SpecYear) "
   " FROM %s..StandardItem ";

// 5
static char szGetKeyLock [128] =
   " /* szGetKeyLock */ "
   "UPDATE %s..KeyMaster "
   "SET KeyValue = KeyValue "
   "WHERE TableName='%%s' ";

// 6
static char szGetKeyValue [128] =
   " /* szGetKeyValue */ "
   "SELECT KeyValue "
   "FROM %s..KeyMaster "
   "WHERE TableName='%%s' ";

// 7
static char szGetKeyCommit [180] =
   " /* szGetKeyCommit */ "
   "UPDATE %s..KeyMaster "
   "SET KeyValue = KeyValue + %%lu "
   "WHERE TableName='%%s' ";

static char szMetric[6]  = " 1 ";
static char szEnglish[6] = " 0 ";


BOOL EXPORT RefreshGetDbQueries (void)
   {
   PGLOBALS pglb;
   PSZ pszDbName;

   pszDbName = ItiDbGetDatabaseName();
   pglb = ItiGlobQueryGlobalsPointer();

   apszDbValQrys[0] = sz0;
   apszDbValQrys[1] = sz1;
   apszDbValQrys[2] = sz2;
   apszDbValQrys[3] = sz3;
   apszDbValQrys[4] = sz4;
   apszDbValQrys[5] = sz5;
   apszDbValQrys[6] = sz6;
   apszDbValQrys[7] = sz7;
   apszDbValQrys[8] = sz8;
   apszDbValQrys[9] = sz9;



   apszDbValPats[0] = szGetMaxSpecYear2;
   apszDbValPats[1] = szGetMaxBaseDate;
   apszDbValPats[2] = szGetCurSpecYear;
   apszDbValPats[3] = szGetCurUnitSys;
   apszDbValPats[4] = szGetMaxSpecYear1;
   apszDbValPats[5] = szGetKeyLock;
   apszDbValPats[6] = szGetKeyValue;
   apszDbValPats[7] = szGetKeyCommit;
   apszDbValPats[8] = NULL;
   apszDbValPats[9] = NULL;


   if (pglb->bUseMetric)
      {
      sprintf (apszDbValQrys[0], apszDbValPats[0], pszDbName, szMetric);
      sprintf (apszDbValQrys[3], apszDbValPats[3], szMetric);
      }
   else
      {
      sprintf (apszDbValQrys[0], apszDbValPats[0], pszDbName, szEnglish);
      sprintf (apszDbValQrys[3], apszDbValPats[3], szEnglish);
      }
   
   sprintf (apszDbValQrys[1], apszDbValPats[1], pszDbName);
   sprintf (apszDbValQrys[2], apszDbValPats[2], pglb->pszCurrentSpecYear);
   sprintf (apszDbValQrys[4], apszDbValPats[4], pszDbName);


  /* szGetKeyLock */
  sprintf (apszDbValQrys[5], apszDbValPats[5], pszDbName);

  /* szGetKeyValue */
  sprintf (apszDbValQrys[6], apszDbValPats[6], pszDbName);

  /* szGetKeyCommit */
  sprintf (apszDbValQrys[7], apszDbValPats[7], pszDbName);

   return TRUE;
   }/* End of Function RefreshGetDbQueries */



PSZ EXPORT ItiDbGetDbValue (HHEAP hheap, USHORT usDbValType)
   {

   if (usDbValType < ITIDB_MIN_DB_QUERY || usDbValType > ITIDB_MAX_DB_QUERY)
      return NULL;

   /* -- Refresh values. -------------------------------------- */
   RefreshGetDbQueries();
   /* -- End of refresh. -------------------------------------- */

   ItiErrWriteDebugMessage("\n ItiDBase.update.c seeking a DB value with: ");
   // ItiErrWriteDebugMessage(apszGetDbVal[usDbValType - 1]);
   ItiErrWriteDebugMessage(apszDbValQrys [usDbValType - 1]);

   // return ItiDbGetRow1Col1 (apszGetDbVal [usDbValType - 1], hheap, 0, 0, 0);
   return ItiDbGetRow1Col1 (apszDbValQrys [usDbValType - 1], hheap, 0, 0, 0);
   }




BOOL InitUpdate (PSZ pszDatabaseName)
   {
   USHORT usPos;
   PGLOBALS pglob;

   /* set the database name in the key master queries */
   pszGetKeyLock   = ItiMemAllocSeg (ItiStrLen (szGetKeyLock)     + 
                                     ItiStrLen (szGetKeyValue)    + 
                                     ItiStrLen (szGetKeyCommit)   +
                                     ItiStrLen (szGetMaxSpecYear1) +
                                     ItiStrLen (szGetMaxSpecYear2) +
                                     ItiStrLen (szGetCurSpecYear) +
                                     ItiStrLen (szGetCurUnitSys) +
                                     ItiStrLen (szGetMaxBaseDate) +
                                     ItiStrLen (szMetric) +
                                     ItiStrLen (szEnglish) +
                                     2 * sizeof (PSZ)             +
                                     7 * ItiStrLen (pszDatabaseName), 
                                     SEG_NONSHARED, MEM_ZERO_INIT);
   if (pszGetKeyLock == NULL)
      return FALSE;

   usPos = 1 + sprintf (pszGetKeyLock, szGetKeyLock, pszDatabaseName);

   pszGetKeyValue = pszGetKeyLock + usPos;
   usPos += 1 + sprintf (pszGetKeyValue,  szGetKeyValue, pszDatabaseName);

   pszGetKeyCommit = pszGetKeyLock + usPos;
   usPos += 1 + sprintf (pszGetKeyCommit, szGetKeyCommit, pszDatabaseName);

   apszGetDbVal = (PSZ *) (pszGetKeyLock + usPos);
   usPos += sizeof (PSZ) * 2;

   apszGetDbVal [0] = pszGetKeyLock + usPos;
   pglob = ItiGlobQueryGlobalsPointer();
   if (pglob->bUseMetric)
      usPos += 1 + sprintf (apszGetDbVal [0], szGetMaxSpecYear2,
                            pszDatabaseName, szMetric);
   else
      usPos += 1 + sprintf (apszGetDbVal [0], szGetMaxSpecYear2,
                            pszDatabaseName, szEnglish);
   
//  usPos += 1 + sprintf (apszGetDbVal [0], szGetMaxSpecYear1, pszDatabaseName);

   apszGetDbVal [1] = pszGetKeyLock + usPos;
   usPos += 1 + sprintf (apszGetDbVal [1], szGetMaxBaseDate, pszDatabaseName);

   apszGetDbVal [2] = pszGetKeyLock + usPos;
   usPos += 1 + sprintf (apszGetDbVal [2], szGetCurSpecYear, pglob->pszCurrentSpecYear);
   //usPos += 1 + sprintf (apszGetDbVal [2], szGetCurSpecYear, pszDatabaseName);

   apszGetDbVal [3] = pszGetKeyLock + usPos;
   if (pglob->bUseMetric)
       usPos += 1 + sprintf (apszGetDbVal [3], szGetCurUnitSys, szMetric);
   else
       usPos += 1 + sprintf (apszGetDbVal [3], szGetCurUnitSys, szEnglish);
   //usPos += 1 + sprintf (apszGetDbVal [3], szGetCurUnitSys, pszDatabaseName);

   /* -- New array to replace above. */
   apszDbValQrys[0] = sz0;
   apszDbValQrys[1] = sz1;
   apszDbValQrys[2] = sz2;
   apszDbValQrys[3] = sz3;
   apszDbValQrys[4] = sz4;
   apszDbValQrys[5] = sz5;
   apszDbValQrys[6] = sz6;
   apszDbValQrys[7] = sz7;
   apszDbValQrys[8] = sz8;
   apszDbValQrys[9] = sz9;

   apszDbValPats[0] = szGetMaxSpecYear2;
   apszDbValPats[1] = szGetMaxBaseDate;
   apszDbValPats[2] = szGetCurSpecYear;
   apszDbValPats[3] = szGetCurUnitSys;
   apszDbValPats[4] = szGetMaxSpecYear1;
   apszDbValPats[5] = szGetKeyLock;
   apszDbValPats[6] = szGetKeyValue;
   apszDbValPats[7] = szGetKeyCommit;
   apszDbValPats[8] = NULL;
   apszDbValPats[9] = NULL;

   return TRUE;
   }

