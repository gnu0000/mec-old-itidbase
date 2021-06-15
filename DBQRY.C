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
 * dbqry.c                                                              *
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

#define  INCL_DOSMISC

#define INCL_WIN
#include "..\INCLUDE\Iti.h"
#include "..\INCLUDE\itibase.h"
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itiglob.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <string.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "..\INCLUDE\itimbase.h"
#include "..\INCLUDE\itierror.h"
#include "dbase.h"
#include "dbparse.h"
#include "dbkernel.h"
#include "dbutil.h"
#include <stdio.h>


#define FOREVER SEM_INDEFINITE_WAIT   
#define MAX_WAIT  10000

typedef struct
   {
   HDB   hdb;
   BOOL  bInUse;
   PSZ   pszQuery;   /* the query -- only for debugging purposes */   
   HQRY  hqry;       /* currently for debugging info only */
   TID   tidOwner;   /* thread ID of user */
   } DBBLOCK;

typedef DBBLOCK *PDBBLOCK;


static PDBBLOCK adbblock = NULL;
static USHORT usBlockingConnections;

static HSEM hsemHDBMutex = NULL; // access to the queue
static CHAR szNUL[] = "  ";


#ifdef DEBUG
static BOOL bDumpQuery = FALSE;
#endif

BOOL DbInitQry (HLOGIN  hlogin, 
                PSZ     pszServerName, 
                PSZ     pszDatabase, 
                USHORT  usNumConnections)
   {
   char     szTemp [256];
   USHORT   i;

#ifdef DEBUG
   {
   PSZ      psz;

   if (!DosScanEnv ("TRQRY", &psz) ||
       !DosScanEnv ("TRACEQUERY", &psz) ||
       !DosScanEnv ("DUMPQUERY", &psz))
      bDumpQuery = TRUE;
   }
#endif

   if (ItiSemCreateMutex (NULL, 0, &hsemHDBMutex))
      {
      ItiErrDisplayHardError ("Could not create blocking protection semaphore!");
      return FALSE;
      }

   adbblock = (PDBBLOCK) ItiMemAlloc (hheapDB1, 
                                      sizeof (DBBLOCK) * usNumConnections,
                                      MEM_ZERO_INIT);

   if (adbblock == NULL)
      {
      ItiErrDisplayHardError ("Could allocate memory for blocking database connections!");
      return FALSE;
      }

   usBlockingConnections = usNumConnections;
   for (i=0; i < usNumConnections; i++)
      {
      adbblock [i].hdb = DbOpen (hlogin, pszServerName, pszDatabase);
      adbblock [i].bInUse = 0;
      adbblock [i].tidOwner = -1;
      adbblock [i].hqry = NULL;
      if (adbblock [i].hdb == NULL)
         {
         sprintf (szTemp, 
                  "Could not create blocking database connection number %d!\n"
                  "You may have run out of database connections.\n"
                  "See your network or database administrator.",
                  i + 1);
         ItiErrDisplayHardError (szTemp);
         return FALSE;
         }

#if defined (DEBUG)
      sprintf (szTemp, "DBPROCESS (HDB) for the blocking connection is %p", 
               adbblock [i].hdb);
      ItiErrWriteDebugMessage (szTemp);
#endif
      }
   return TRUE;
   }



static BOOL GetSemaphore (PSZ pszFunction, HSEM hsemMutex)
   {
   char szTemp [256];
   USHORT   usError;

   switch (usError = ItiSemRequestMutex (hsemMutex, MAX_WAIT))
      {
      case 0:
         return TRUE;

      case ITIBASE_SEM_TIMEOUT:
         sprintf (szTemp, "Semaphore time out in %s:\n"
                          "This might mean a coding error.  "
                          "Contact Info Tech Technical Support.", pszFunction);
         ItiErrDisplayHardError (szTemp);
         return FALSE;

      default:
         {
         sprintf (szTemp, "%s: Unknow semaphore error %u."
                          "\nContact Info Tech Techincal Support.", 
                          pszFunction, usError);
         ItiErrDisplayHardError (szTemp);
         }
         return FALSE;
      }
   }


static PDBBLOCK GetDBBlock (PSZ pszQuery)
   {
   PDBBLOCK pdbblock;
   USHORT   i, j;

   pdbblock = NULL;

#ifdef DEBUG
   /* 8/9/93 mdh */
   if (bDumpQuery)
      ItiErrWriteDebugMessage(pszQuery);
#endif

   /* Code by SUPER KLUDGE (tm).  For some reason, this helps: */
   for (j = 0; j < 6; j++)
      {
      if (!GetSemaphore ("GetDBBlock", hsemHDBMutex))
         return NULL;

#ifdef DEBUG
      if (j)
         ItiErrWriteDebugMessage ("BOING! Looping again in GetDBBlock");
#endif

      for (i=0; i < usBlockingConnections; i++)
         {
         if (!adbblock [i].bInUse)
            {
            adbblock [i].bInUse = TRUE;
            adbblock [i].hqry = NULL;
            adbblock [i].pszQuery = ItiStrDup (hheapDB1, pszQuery);
            adbblock [i].tidOwner = pglobals->plis->tidCurrent;
            pdbblock = adbblock + i;
            break;
            }
         }

      /* do this BEFORE error processing, or other threads will time out. 
         mdh 6/3/92 */
      ItiSemReleaseMutex (hsemHDBMutex);
   
      if (pdbblock)
         break;
      else
         DosSleep (1000);  // hey, wait a second...
      }

   if (pdbblock == NULL)
      {
      ItiErrDisplayHardError ("Could not find a free blocking database connection.\n"
          "You need to increase the number of blocking database connections.\n"
          "Contact your network or datbase administrator.\nSee the errorlog for more information.");
      for (i=0; i < usBlockingConnections; i++)
         {
         char szTemp [256];
         
         sprintf (szTemp, "adbblock [%2u].tidOwner = %d. [bInUse=%d]", i,
                  adbblock [i].tidOwner, adbblock [i].bInUse);
         ItiErrWriteDebugMessage (szTemp);

         sprintf (szTemp, "adbblock [%2u].hqry = %p (%s)\nQuery: %s", i,
                  adbblock [i].hqry,
                  ItiMemQueryRead (adbblock [i].hqry) ? "Readable" : "I/O Error",
                  ItiMemQueryRead (adbblock [i].pszQuery) ? adbblock [i].pszQuery : "I/O Error");
         ItiErrWriteDebugMessage (szTemp);
         }
      }

   return pdbblock;
   }


void FreeHDB (HDB hdb)
   {
   USHORT i;
   BOOL   bFound;

   if (!GetSemaphore ("FreeHDB", hsemHDBMutex))
      return;

   for (i=0, bFound = FALSE; i < usBlockingConnections && !bFound; i++)
      {
      if (adbblock [i].hdb == hdb)
         {
         adbblock [i].bInUse = FALSE;
         adbblock [i].hqry = NULL;
         ItiMemFree (hheapDB1, adbblock [i].pszQuery);
         adbblock [i].pszQuery = NULL;
         adbblock [i].tidOwner = -1;
         bFound = TRUE;
         }
      }

   /* do this BEFORE error processing, or other threads will time out. 
      mdh 6/3/92 */
   ItiSemReleaseMutex (hsemHDBMutex);

   if (!bFound)
      {
      char szTemp [256];

      sprintf (szTemp, "Could not find hdb %p (%s) in the adbblock list!", 
               hdb, ItiMemQueryRead (hdb) ? "Readable" : "I/O Error");
      ItiErrDisplayHardError (szTemp);
      }
   }



void FreeHQRY (HQRY hqry)
   {
   FreePCOL (hqry->pcol, hqry->hheap, hqry->uNumCols);
   FreeHDB (hqry->hdb);

   /* let's be paranoid */
   hqry->pcol = NULL;
   hqry->uNumCols = 0;
   hqry->hdb = NULL;

   ItiMemFree (hqry->hheap, hqry);
   }




/*
 * If an error occurs, this fn returns NULL
 * and you can check uState for the error code. 
 * If tou get NULL but error code = SUCCEED, there was insuficient memory
 */
HQRY EXPORT ItiDbExecQuery (PSZ     pszQuery,
                            HHEAP   hheap,  
                            HMODULE hmod,    
                            USHORT  uResId,  
                            USHORT  uId,     
                            USHORT  *puNumCols,
                            USHORT  *puState)
   {
   PDBBLOCK pdbblock;
   HQRY     hqry;

   pdbblock = GetDBBlock (pszQuery);
   if (pdbblock == NULL)
      return NULL;

   if (!DbExecQuery (pdbblock->hdb, pszQuery, puNumCols, puState))
      {
      ItiErrWriteDebugMessage (pszQuery);
      FreeHDB (pdbblock->hdb);
      return NULL;
      }
   if ((hqry = (HQRY) ItiMemAlloc (hheap, sizeof (QRY), 0)) == NULL)
      {
      FreeHDB (pdbblock->hdb);
      return NULL;
      }
   hqry->hdb   = pdbblock->hdb;
   hqry->hheap = hheap;
   pdbblock->hqry = hqry;
   ItiDbGetColData (pdbblock->hdb, hheap, hmod,
                    ITIRT_FORMAT, uResId, FALSE, uId,
                    &(hqry->uNumCols), &(hqry->pcol));
   return hqry;
   }



BOOL EXPORT ItiDbGetQueryRow (HQRY   hqry,
                              PSZ    **pppsz,
                              USHORT *puState)
   {
   CHAR     szCol[1024];
   USHORT   i;
   PSZ      pszFormat;

   *pppsz = NULL;
   if (!DbNextRow (hqry->hdb, puState))
      {
      if (DbResults (hqry->hdb))
         DbCancel (hqry->hdb);
      FreeHQRY (hqry);
      return FALSE;
      }

   if ((*pppsz = ItiMemAlloc (hqry->hheap, sizeof (PSZ)* hqry->uNumCols, 0))== NULL)
      {
      DbCancel (hqry->hdb);
      FreeHQRY (hqry);
      return FALSE;
      }

   for (i = 0; i < hqry->uNumCols; i++)
      {
      pszFormat = (hqry->pcol == NULL ? NULL : hqry->pcol[i].pszFormat);
      DbGetFmtCol (hqry->hdb, i, pszFormat, szCol, sizeof szCol, NULL);
      (*pppsz)[i] = ItiStrDup (hqry->hheap, szCol);
      }
   return TRUE;
   }



/*
 * ItiDbExecSQLStatement sends an SQL statement to the server. 
 * This function is usefull if you don't care about results from the
 * server.
 *
 * This function will execute string with only a single SQL statement.
 *
 * Return value: 0 if no errors, otherwise a number whose meaning
 * is unknown is returned.
 */

USHORT EXPORT ItiDbExecSQLStatement (HHEAP hheap, PSZ pszSQLStatement)
   {
   PDBBLOCK pdbblock;
   USHORT   usNumCols;
   USHORT   usState;


   pdbblock = GetDBBlock (pszSQLStatement);
   if (pdbblock == NULL)
      return -1;

   if (!DbExecQuery (pdbblock->hdb, pszSQLStatement, &usNumCols, &usState))
      {
      ItiErrWriteDebugMessage (pszSQLStatement);
      FreeHDB (pdbblock->hdb);
      return 0xffff;
      }

   do
      {
      while (DbNextRow (pdbblock->hdb, &usState))
         ;
      } while (DbResults (pdbblock->hdb));

   FreeHDB (pdbblock->hdb);

   return 0;

//   USHORT usColumns, usState;
//   HQRY hqry;
//   PSZ  *ppszData;
//
//   hqry = ItiDbExecQuery (pszCommand, hheap, 0, 0, 0, &usColumns, &usState);
//   if (hqry == NULL) 
//      {
//      return (usState == 0) ? ERROR_UNKNOWN : usState;
//      }
//
//   while (ItiDbGetQueryRow (hqry, &ppszData, &usState))
//      ItiMbFreeStrArray (hheap, ppszData, usColumns);
//
//   return 0;
   }



USHORT EXPORT ItiDbGetQueryCount (PSZ     pszQuery,
                                  USHORT  *puNumCols,
                                  USHORT  *puError)
   {
   USHORT i = 0;
   PSZ    *ppszRow, pszCountQuery;
   HQRY   hqry;

   pszCountQuery = DbGetCountQuery (hheapDB0, pszQuery);
   if (0 == ItiStrNICmp (pszCountQuery, "SELECT DISTINCT", 15))
      {
      ItiMemFree (hheapDB0, pszCountQuery);
      pszCountQuery = NULL;
      }

   if (pszCountQuery)
      {
      hqry = ItiDbExecQuery (pszCountQuery, hheapDB0, 0, 0, 0, puNumCols, puError);
      if (hqry == NULL)
         {
         ItiErrWriteDebugMessage (pszCountQuery);
         ItiMemFree (hheapDB0, pszCountQuery);
         return 0;
         }
      i = 0;
      if (ItiDbGetQueryRow (hqry, &ppszRow, puError))
         {
         if (!ItiStrToUSHORT (ppszRow [0], &i))
            i = 0;
         ItiFreeStrArray (hheapDB0, ppszRow, *puNumCols);
         ItiDbTerminateQuery (hqry);
         }

      ItiMemFree (hheapDB0, pszCountQuery);
      }
   else
      {   
      hqry = ItiDbExecQuery (pszQuery, hheapDB0, 0, 0, 0, puNumCols, puError);
      if (hqry == NULL)
         {
         ItiErrWriteDebugMessage ("MSG: Query had a null result.");
         return 0;
         }
      while (ItiDbGetQueryRow (hqry, &ppszRow, puError))
         {
         i++;
         ItiFreeStrArray (hheapDB0, ppszRow, *puNumCols);
         }
      }
   return i;
   }



void EXPORT ItiDbGetQueryError (HQRY    hqry,
                                PSZ     pszErrStr,
                                USHORT  uStrLen)
   {
   return;
   }




void EXPORT ItiDbTerminateQuery (HQRY hqry)
   {
   if (!ItiMemQueryRead (hqry))
      {
      char szTemp [256];

      sprintf (szTemp, 
               "ItiDbTerminateQuery: Somebody passed me a bad pointer!\n"
               "hqry=%p", 
               hqry);
      ItiErrDisplayHardError (szTemp);
      }
   else
      {
      DbCancel (hqry->hdb);
      FreeHQRY (hqry);
      }
   }


MRESULT EXPORT ItiDbQueryQuery (HQRY   hqry,
                                USHORT uCmd,
                                USHORT uSubCmd)
   {
   return 0;
   }



PSZ *EXPORT ItiDbGetRow1 (PSZ       pszQuery,
                          HHEAP     hheap, 
                          HMODULE   hmod,    
                          USHORT    usResId,  
                          USHORT    usId,
                          PUSHORT   pusNumCols)
   {
   HQRY hqry;
   USHORT usState;
   PSZ *ppsz = NULL;


   hqry = ItiDbExecQuery (pszQuery, hheap, hmod, usResId, usId, pusNumCols,
                          &usState);
   if (hqry != NULL)
      {
      if (ItiDbGetQueryRow (hqry, &ppsz, &usState))
         {
         ItiDbTerminateQuery (hqry);
         return ppsz;
         }
      }
   else
      ItiErrWriteDebugMessage (pszQuery);
   return NULL;
   }


PSZ EXPORT ItiDbGetRow1Col1 (PSZ       pszQuery,
                             HHEAP     hheap, 
                             HMODULE   hmod,    
                             USHORT    usResId,  
                             USHORT    usId)
   {
   HQRY hqry;
   USHORT usState, usNumCols;
   PSZ *ppsz, pszCol1;

   pszCol1 = NULL;

   hqry = ItiDbExecQuery (pszQuery, hheap, hmod, usResId, usId, &usNumCols,
                          &usState);
   if (hqry != NULL)
      {
      if (ItiDbGetQueryRow (hqry, &ppsz, &usState))
         {
         if (ppsz == NULL)
            {
            pszCol1 = &szNUL[0];
            ItiDbTerminateQuery (hqry);
            }
         else
            {
            pszCol1 = *ppsz;
            ItiDbTerminateQuery (hqry);

            /*--- Free unused columns ---*/
            for (--usNumCols; usNumCols; usNumCols--)
               ItiMemFree (hheap, ppsz[usNumCols]);

            /*--- Free ptr array ---*/
            ItiMemFree (hheap, ppsz);
            }/* end else */
         } /* end then */
      }
   else
      ItiErrWriteDebugMessage (" - - ");
      // ItiErrWriteDebugMessage (pszQuery);
   return pszCol1;
   }

