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
 * dbase.c                                                              *
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
#define INCL_DOSMISC
#include "..\INCLUDE\Iti.h"
#include "..\INCLUDE\itiwin.h"
#include "..\INCLUDE\itimbase.h"
#include "..\INCLUDE\Itierror.h"
#include "..\INCLUDE\itibase.h"
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itiglob.h"
#include "..\include\colid.h"
#include "..\include\winid.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <string.h>
#include <process.h>
#include <stddef.h>
#include <stdio.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "dbase.h"
#include "dbutil.h"
#include "dbkernel.h"
#include "dbqueue.h"
#include "buflist.h"
#include "dbqry.h"
#include "dbparse.h"
#include "update.h"
#include "calc.h"
#include "initdll.h"
#include "dialog.h"

/***************************************************************/
/*                 initialization procedures                   */
/***************************************************************/


PGLOBALS pglobals;
HHEAP    hheapDB0; /*--- temporary usage hheap              ---*/
HHEAP    hheapDB1; /*--- hbuf, pszQuery, *hwndUsers, dbblock---*/
HHEAP    hheapDB2; /*--- pcol, pseghdr, pszFormat           ---*/
HHEAP    hheapDB3; /*--- pszTable                           ---*/

BOOL bVerbose = FALSE;

#define DEFSTACKSIZE    0x4000


/*  this has a hheap param because it is for both buffers and
 *  non buffer dbase uses
 *  if this fn is called for buffers hheap should be called using hheapDB2
 */
void FreePCOL (PCOLDATA pcol, HHEAP hheap, USHORT uNumCols)
   {
   USHORT i;

   if (pcol != NULL)
      {
      for (i = 0; i < uNumCols; i++)              /* free colnames */
         if (pcol[i].pszFormat != NULL)
            ItiMemFree (hheap, pcol[i].pszFormat);
      ItiMemFree (hheap, pcol);
      }
   }


/*  this has a hheap param because it is for both buffers and
 *  non buffer dbase uses
 *  if this fn is called for buffers hheap should be called using hheapDB2
 */
BOOL ItiDbGetColData (HDB      hdb,
                      HHEAP    hheap,
                      HMODULE  hmod,
                      USHORT   uResType,
                      USHORT   uResId,
                      BOOL     bDlgRes,
                      USHORT   uBuffId,
                      USHORT   *puNumCols,
                      PCOLDATA *ppcol)
   {
   PEDT     pedt;
   PDGI     pdgi;
   USHORT   i, j, uNum, uCol;
   PSZ      *ppsz;

   *puNumCols = DbNumCols(hdb);
   *ppcol = (PCOLDATA) ItiMemAlloc (hheap, *puNumCols * sizeof (COLDATA), 0);
   for (i = 0; i < *puNumCols; i++)
      {
      (*ppcol)[i].pszFormat = NULL;
      (*ppcol)[i].uColId    = ItiColStringToColId (DbColName (hdb, i));
      (*ppcol)[i].uMaxLen   = (USHORT) DbColLen  (hdb, i);
      (*ppcol)[i].uColType  = (USHORT) DbColType (hdb, i);
      }

   if (uResId == ITIRID_NONE)
      {
      return 1;
      }
   else if (bDlgRes) /*--- dialog formats ---*/
      {
      if (ItiMbGetDialogInfo (hmod, uResId, uBuffId, hheapDB0, &pdgi, &ppsz, &uNum))
         return 1;
      }
   else if (uResType == ITIRT_DATA)
      {
      if (ItiMbGetDataInfo   (hmod, uResId, uBuffId, hheapDB0, &pedt, &ppsz, &uNum))
         return 1;
      }
   else if (uResType == ITIRT_FORMAT)
      {
      if (ItiMbGetFormatText (hmod, uResId, uBuffId, hheapDB0, &ppsz, &uNum))
         return 1;
      }
   else
      {
      DbErr ("Unknown format type", uResType);
      }

   if (uResType == ITIRT_FORMAT && uNum > *puNumCols)
      DbErr ("More formats than Columns in format resource", uNum);
   if (uResType == ITIRT_FORMAT && uNum < *puNumCols)
      DbErr ("More Columns than Formats in format resource", uNum);

   /*---Get format strings array for this window---*/
   for (i = 0; i < *puNumCols; i++)
      {
      if (bDlgRes) /*--- data is for dialog ---*/
         {
         for (j = 0; j < uNum; j++)
            {
            uCol = pdgi[j].uColId;

            if ((pdgi[j].uChangeQuery == uBuffId || pdgi[j].uAddQuery == uBuffId) &&
                (i + 32769U == uCol || (*ppcol)[i].uColId == uCol))
               {
               (*ppcol)[i].pszFormat=ItiStrDup(hheap, ppsz[j]);
               break;
               }
            }
         }
      else if (uResType == ITIRT_DATA)
         {
         /* --- now we look at resource to see if we have a fmat for the col --- */
         for (j = 0; j < uNum; j++)
            {
            uCol = pedt[j].uIndex;
            if (i + 32769U == uCol || (*ppcol)[i].uColId == uCol)
               {
               (*ppcol)[i].pszFormat=ItiStrDup(hheap, ppsz[j]);
               break;
               }
            }
         }
      else /* --- type is format, so formats are in order --- */
         {
         (*ppcol)[i].pszFormat = ItiStrDup(hheap, ppsz[i]);
         }
      }
   ItiFreeStrArray (hheapDB0, ppsz, uNum);

   if (bDlgRes)     /*--- dialog formats ---*/
      ItiMemFree (hheapDB0, pdgi);
   else if (uResType == ITIRT_DATA)
      ItiMemFree (hheapDB0, pedt);

   return 0;
   }










//
//
//
//
//void DbStrFmat (PVOID pData, USHORT uType, PSZ pszCol)
//   {
//   }
//
//static void FormatCol (PSZ      pszRow,
//                       HBUF     hbuf,
//                       USHORT   uCol,
//                       PSZ      pszCol)
//   {
//   BYTE     b;
//   USHORT   i, uRelCol = 0xFFFF;
//
//      case SQLCHAR:
//         strcpy (pszCol, pData);
//         break;
//
//      case SQLFLT8:
//         printf (pszCol, "%lf", &((double *)pData));
//         break;
//
//      case SQLINT1:
//         b = *pData;
//         sprintf (pszCol, "%d", (USHORT) b);
//         break;
//
//      case SQLINT2:
//         sprintf (pszCol, "%d", *((SHORT *) pData));
//         break;
//
//      case SQLINT4:
//         sprintf (pszCol, "%ld", *((LONG *) pData));
//         break;
//
//      case SQLTEXT:
//      case SQLDATETIME:
//      case SQLMONEY:
//      default:
//         strcpy (pszCol, pData);
//      }
//   }
//
//
//
//

#define DATABASE_NAME_SIZE  33
static char szDatabaseName [DATABASE_NAME_SIZE] = "";

MRESULT EXPORT AskDatabaseDProc (HWND     hwnd, 
                                 USHORT   usMessage, 
                                 MPARAM   mp1, 
                                 MPARAM   mp2)
   {
   switch (usMessage)
      {
      case WM_COMMAND:
         switch (SHORT1FROMMP (mp1))
            {
            case DID_OK:
               {
               WinQueryDlgItemText (hwnd, DID_DATABASENAME, 
                                    sizeof szDatabaseName, szDatabaseName);
               WinDismissDlg (hwnd, TRUE);
               }
               break;

            default:
               WinDismissDlg (hwnd, FALSE);
               break;
            }
         break;
      }
   return WinDefDlgProc (hwnd, usMessage, mp1, mp2);
   }


PSZ EXPORT ItiDbGetDatabaseName (void)
   {
   PSZ psz;

   szDatabaseName [0] = '\0';
   if (!DosScanEnv ("ASKDB", &psz) && pglobals->habMainThread)
      {
      /* ask the user for a database name */
      WinDlgBox (HWND_DESKTOP,
                 pglobals->hwndAppFrame ? pglobals->hwndAppFrame : HWND_DESKTOP,
                 AskDatabaseDProc,
                 hmodModule,
                 IDD_ASKDATABASE,
                 NULL);
      }

   if (szDatabaseName [0] == '\0')
      if (DosScanEnv ("DATABASE", &psz))
         ItiStrCpy (szDatabaseName, "DSShell", sizeof szDatabaseName);
      else
         ItiStrCpy (szDatabaseName, psz, sizeof szDatabaseName);
   return szDatabaseName;
   }



static BOOL InitConnection (PSZ pszServer, PSZ pszDatabase, HLOGIN hlogin)
   {
   HDB   hdb;

   if ((hdb = DbOpen (hlogin, pszServer, pszDatabase)) != NULL)
      _beginthread (DbReadCmdQ, NULL, DEFSTACKSIZE, (VOID *)hdb);
   return hdb != NULL;
   }

static void DoDump (void)
   {
   PSZ psz;

   /* -- get sqlserver version -- */
   psz = ItiDbGetRow1Col1 ("SELECT @@version", hheapDB0, 0, 0, 0);
   ItiErrWriteDebugMessage (psz);
   ItiMemFree (hheapDB0, psz);

   /* -- get sqlserver number of connections -- */
   psz = ItiDbGetRow1Col1 (" SELECT 'User Connections = '+convert (char (4),value)+"
                           " ', Max Connections = '+convert (char (4),high)"
                           " FROM master..spt_values s, master..sysconfigures c"
                           " WHERE s.name = 'user connections'"
                           " AND s.number = c.config",
                           hheapDB0, 0, 0, 0);
   ItiErrWriteDebugMessage (psz);
   ItiMemFree (hheapDB0, psz);
   }


BOOL EXPORT ItiDbInit (USHORT usNonBlocking, USHORT usBlocking)
   {
   HLOGIN   hlogin;
   USHORT   i;
   PSZ      pszServerName, pszAppName, pszHostName;
   PSZ      pszUserName, pszPassword, pszDatabase;
   char     szTemp [1024];

   pglobals = ItiGlobQueryGlobalsPointer ();

   /* create heap for database module */
   hheapDB0 = ItiMemCreateHeap (16384U);
   hheapDB1 = ItiMemCreateHeap (32768U);
   hheapDB2 = ItiMemCreateHeap (32768U);
   hheapDB3 = ItiMemCreateHeap (8192U);

   if (hheapDB0 == NULL || hheapDB1 == NULL || hheapDB2 == NULL || hheapDB3 == NULL)
      {
      ItiErrDisplayHardError ("Could not allocate heaps for the database module!\n"
                              "This may mean one or more of the following:\n"
                              "  1) The drive your swappfile is located on is full.\n"
                              "  2) You need more memory.\n"
                              "  3) Your running too many applications\n");
      return FALSE;
      }

   if (DosScanEnv ("SERVER", &pszServerName))
      pszServerName = "";

   pszAppName = pglobals->pszAppName;

   if (!DosScanEnv ("ITIDBASE", &pszHostName))
      bVerbose = TRUE;

   if (DosScanEnv ("HOSTNAME", &pszHostName))
      pszHostName = "";

   if (DosScanEnv ("USERNAME", &pszUserName))
      pszUserName = "sa";

   if (DosScanEnv ("PASSWORD", &pszPassword))
      pszPassword = "";

   pszDatabase = ItiDbGetDatabaseName ();

   sprintf (szTemp, "Server=%s, User=%s, Database=%s", 
            pszServerName, pszUserName, pszDatabase);
   ItiErrWriteDebugMessage (szTemp);

   DbInit ();
   hlogin = DbLogin (pszAppName, pszHostName, pszUserName, pszPassword, FALSE);
   if (hlogin == NULL)
      {
      ItiErrDisplayHardError ("Could not get a login structure for the SQL server.\n"
                              "This may mean one or more of the following:\n"
                              "  1) Your drive your swappfile is located on is full.\n"
                              "  2) You need more memory.\n"
                              "  3) Your running too many applications.\n"
                              "  4) SQL Server is not running.\n"
                              "  5) The network is not running.\n"
                              "  6) Your SERVER environment variable is not correct.\n"
                              "  7) There is something wrong with the database.\n");
      return FALSE;
      }

   DbInitCmdQ ();
   DbInitLL ();

   if (!ItiColInitialize ())
      return FALSE;

   if (!InitUpdate (pszDatabase))
      return FALSE;

   /* -- for buffer querys -- */
   for (i = 0; i < usNonBlocking; i++)
      if (!InitConnection (pszServerName, pszDatabase, hlogin))
         return FALSE;

   /* -- for single threaded querys -- */
   if (!DbInitQry (hlogin, pszServerName, pszDatabase, usBlocking))
      return FALSE;

   /* -- free up login structure -- */
   DbFreeLogin (hlogin);

   /* dump out neat info to log file, in case of BSR... */
   DoDump ();

   /* -- Startup the calc queue -- */
   ItiDbInitCalcQ ();

   return TRUE;
   }


void EXPORT ItiDbTerminate (void)
   {
   ItiDbDeInitCalcQ ();
   return;
   }






