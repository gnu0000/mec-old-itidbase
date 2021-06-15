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
 * dbbuf.c                                                              *
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
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itierror.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "dbkernel.h"
#include "dbase.h"
#include "dbbuf.h"
#include "dbparse.h"
#include "dbutil.h"
#include "dbqueue.h"
#include "buflist.h"
#include "..\include\colid.h"
#include "..\INCLUDE\itimbase.h"

#define ITIDB_DEFBUFSIZE   0xFFFF
#define ITIDB_ROWSTRLEN    0x1000
#define TMPIDXLEN          3000

#define FOREVER SEM_INDEFINITE_WAIT   

/***********************************************************************/
/*******************  general buffer procedures  ***********************/
/***********************************************************************/




/* This procedure assumes that any synchronization that needs to 
 * be handled has already been done!
 *
 * returns TRUE if data needed to be removed
 */



/* change 9/30/91
 *
 * from now on the hbuf linked list and segment headers
 * are allocated from hbufDB but all
 * other alloced entities associated with hbuf are still allocated from
 * the specified hbuf. at delete time only delete the hbuf and allocated
 * segments. data allocated in the specified hbuf is probably already freed
 */


static BOOL FreeSEGHDR (HBUF hbuf)
   {
   USHORT   i;
   BOOL     bReturn = FALSE;

   /*--- First, Free up any old data segments ---*/ 
   hbuf->uNumRows = 0;
   if (hbuf->pseghdr != NULL)
      {
      bReturn = TRUE;
      for (i = 0; i < hbuf->uSegCount; i++)
         ItiMemFreeSeg (hbuf->pseghdr[i].pszBuffer);
      }
   if (hbuf->uSegCount)
      ItiMemFree (hheapDB2, hbuf->pseghdr);
   else if (hbuf->uType == ITI_STATIC) /* mdh 4/27/92 -- free a static window's seg hdr */
      ItiMemFree (hheapDB2, hbuf->pseghdr);

   hbuf->uSegCount = 0;
   hbuf->pseghdr = NULL;
   return bReturn;
   }


/***********************************************************************/
/*******************  procedures  for update     ***********************/
/***********************************************************************/


BOOL DbSendMsg (HBUF hbuf, USHORT umsg, MPARAM mp1, MPARAM mp2)
   {
   USHORT i;

   for (i = 0; i < hbuf->uUsers; i++)
      if (WinIsWindow (pglobals->habMainThread, hbuf->hwndUsers[i]))
         WinPostMsg (hbuf->hwndUsers[i], umsg, mp1, mp2);
   return 0;
   }






BOOL DbFillStaticBuffer (HBUF hbuf, HDB hdb, HMODULE hmod)
   {
   char     szRow [ITIDB_ROWSTRLEN];
   USHORT   uRowLen, uCols, uState, i;

   if (i = ItiSemRequestMutex (hbuf->hsemWriteMutex, 10000))
      {
      DbErr ("Could not get Write semaphore in DbFillStaticBuffer.  usError=", i);
      return FALSE;
      }

   if (DbDecCmdRefCount (hbuf) || !hbuf || !hbuf->uRefCount)
      {
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      return FALSE;
      }

   if (i = ItiSemRequestMutex (hbuf->hsemReadMutex, 2000))
      {
      DbErr ("Could not get Read semaphore in DbFillStaticBuffer.  usError=", i);
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      return FALSE;
      }

   if (!DbExecQuery (hdb, hbuf->pszQuery, &uCols, &uState))
      {
      hbuf->uNumRows = 0;
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      DbSUSErr ("Query could not be executed, uState=", uState, hbuf->pszQuery);
      return FALSE;
      }

   if (!hbuf->uNumCols)
      ItiDbGetColData (hdb, hheapDB2, hmod, ITIRT_DATA, ITIRID_WND,
                hbuf->bDlgRes, hbuf->uId, &(hbuf->uNumCols), &(hbuf->pcol));

   /* --- tell windows to prepare for new data --- */
   DbSendMsg (hbuf, WM_ITIDBCHANGED, MPFROM2SHORT (ITI_STATIC, hbuf->uId), MPFROMP (hbuf));

   if (!DbGetRowForBuffer(hdb, hbuf->pcol, szRow, sizeof szRow, &uRowLen))
      {
      for (i=0; i < hbuf->uNumCols; i++)
         szRow [i] = '\0';
      uRowLen = hbuf->uNumCols;
      hbuf->uNumRows = 0;
      }
   else
      {
      hbuf->uNumRows = 1;
      }

   hbuf->pseghdr = (PSEGHDR) ItiMemRealloc (hheapDB2, hbuf->pseghdr,
                                            uRowLen+1,   0);

   ItiMemCpy ((PSZ)hbuf->pseghdr, szRow, uRowLen+1);
   while (DbGetRowForBuffer(hdb, hbuf->pcol, szRow, sizeof szRow, &uRowLen))
      ;
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   ItiSemReleaseMutex (hbuf->hsemWriteMutex);

   DbSendMsg (hbuf, WM_ITIDBBUFFERDONE, 
              MPFROM2SHORT (ITI_STATIC, hbuf->uNumRows), MPFROMP (hbuf));
   return TRUE;
   }








/* --- ret true if ok ---*/
BOOL DbFillListBuffer (HBUF hbuf, HDB hdb, HMODULE hmod)
   {
   USHORT      uLastByte,  uRow,    uOffset,
               uSeg,       uIndex,  uRowLen,
               uState,     uCols;
   char        szRow [ITIDB_ROWSTRLEN];
   BOOL        bFillingBuf;
   PSEGHDR     pseghdr;
   PSZ         pszBuffer;
   PROWIDX     pri;

   ItiSemRequestMutex (hbuf->hsemWriteMutex, FOREVER);

   if (DbDecCmdRefCount (hbuf) || !hbuf->uRefCount)
      {
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      return FALSE;
      }

   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
   if (!DbExecQuery (hdb, hbuf->pszQuery, &uCols, &uState))
      {
      DbSUSErr ("Query could not be executed, uState=", uState, hbuf->pszQuery);
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      return FALSE;
      }
   ItiSemReleaseMutex (hbuf->hsemReadMutex);

   if (!hbuf->uNumCols)
      ItiDbGetColData (hdb, hheapDB2, hmod, ITIRT_DATA, ITIRID_WND,
                       hbuf->bDlgRes, hbuf->uId, &(hbuf->uNumCols),
                       &(hbuf->pcol));

   pri = (PROWIDX) ItiMemAlloc (hheapDB0, TMPIDXLEN * sizeof (ROWIDX), 0);

   /*--- First, Free up any old data segments ---*/ 
   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
   FreeSEGHDR (hbuf);
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   uSeg = uRow = 0;

   /* --- tell windows to prepare for new data --- */
   DbSendMsg (hbuf, WM_ITIDBCHANGED,
              MPFROM2SHORT (ITI_LIST, hbuf->uId), MPFROMP (hbuf));
   do
      {
      /* --- allocate new segment header for new segment --- */
      ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
      hbuf->pseghdr = (PSEGHDR) ItiMemRealloc (hheapDB2, hbuf->pseghdr,
                                            (uSeg + 1) * sizeof (SEGHDR), 0);
      if ( hbuf->pseghdr == NULL)
         {
         ItiMemFree (hheapDB0, pri);
         ItiSemReleaseMutex (hbuf->hsemReadMutex);
         ItiSemReleaseMutex (hbuf->hsemWriteMutex);
         DbCancel (hdb);
         return FALSE;
         }

      pseghdr = &(hbuf->pseghdr[uSeg]);
      /* --- allocate new segment --- */
      pszBuffer = pseghdr->pszBuffer = (PSZ) ItiMemAllocSeg
                                      (ITIDB_DEFBUFSIZE, SEG_NONSHARED, 0);
      if (pszBuffer == NULL)
         {
         ItiMemFree (hheapDB0, pri);
         ItiSemReleaseMutex (hbuf->hsemReadMutex);
         ItiSemReleaseMutex (hbuf->hsemWriteMutex);
         DbCancel (hdb);
         return FALSE;
         }

      hbuf->uSegCount = uSeg+1;

      /* temporarily set the index to the temp index array
       * this is to allow row requests as we work
       */
      pseghdr->pridx = pri;

      /*
       *  FILL UP CURRENT SEGMENT
       */
      pseghdr->uFirst   = uRow;  // 1st row # in current seg
      pseghdr->uNumRows = 0;
      uOffset   = 0;           // offset of string into buffer
      uIndex    = 0;           // index into rowindex
      /* possible kludge fix by mdh -- sizeof (ROWIDX) * 2 instead of
         sizeof (ROWIDX), to get rid of segmentation violation */
      uLastByte = ITIDB_DEFBUFSIZE - sizeof (ROWIDX) * 2; // end of str space
      ItiSemReleaseMutex (hbuf->hsemReadMutex);

      while (TRUE /*hbuf->uRefCount*/)
         {
         /*--- break & end if no more rows ---*/
         if (!(bFillingBuf = DbGetRowForBuffer(hdb, hbuf->pcol, szRow, sizeof szRow, &uRowLen)))
            break;

         /*--- break if not enough room in buffer ---*/
         if ((long)uOffset + (long)uRowLen >= (long) uLastByte)
            break;

         /*--- break if index is full ---*/
         if (uIndex >= TMPIDXLEN)
            break;

         /*--- break if it should be deleted or redone ---*/
         if (hbuf->uCmdRefCount)
            break;

         /*--- add new row to buffer ---*/
         ItiMemCpy (pszBuffer + uOffset, szRow, uRowLen);

         uRow++;
         pri[uIndex].uOffset = uOffset;
         pri[uIndex].uState  = ITIDB_VALID;
         uOffset += uRowLen;
         uIndex++;
         uLastByte -= sizeof (ROWIDX);
         pseghdr->uNumRows++;
         ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
         hbuf -> uNumRows++;
         ItiSemReleaseMutex (hbuf->hsemReadMutex);

         if (!(hbuf->uNumRows % LINESPERMSG))
            DbSendMsg (hbuf, WM_ITIDBAPPENDEDROWS,
                        MPFROM2SHORT (ITI_LIST, hbuf->uNumRows),
                        MPFROMP (hbuf));
         }

      /*--- move index into end of buffer ---*/
      ItiMemCpy (pszBuffer + uOffset, pri, (uIndex+1) * sizeof (ROWIDX));

      ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
      pseghdr->pridx = (PROWIDX)(pszBuffer + uOffset);

      /*--- shrink buffer as much as you can ---*/
      pseghdr->pszBuffer = (PSZ)ItiMemReallocSeg ( pseghdr->pszBuffer,
                                    uOffset+ (uIndex+1)* sizeof (ROWIDX),
                                    0);
      if ( pseghdr->pszBuffer == NULL)
         {
         ItiSemReleaseMutex (hbuf->hsemReadMutex);
         ItiSemReleaseMutex (hbuf->hsemWriteMutex);
         DbCancel (hdb);
         return DbErr ("Unable to resize buffer", 0);
         }
      uSeg++;
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      } while (bFillingBuf && !hbuf->uCmdRefCount);


   ItiMemFree (hheapDB0, pri);

   ItiSemReleaseMutex (hbuf->hsemWriteMutex);

   if (bFillingBuf)  /*--- if exit due to refcount, free hdb ---*/
      DbCancel (hdb);
   else
      /*--- this message is now sent only if the query is indeed ---*/
      /*--- completed, not just terminated ---*/
      DbSendMsg (hbuf, WM_ITIDBBUFFERDONE,
               MPFROM2SHORT (ITI_LIST, hbuf->uNumRows), MPFROMP (hbuf));
   return TRUE;
   }



/* change 9/30/91
 *
 * from now on the hbuf linked list and associated data
 * are allocated from hbufDB1 and hheapDB2
 */

BOOL EXPORT DbDelBuf(HBUF hbuf, HWND hwnd)
   {
   if (hbuf == NULL)
      return FALSE;

   ItiSemRequestMutex (hbuf->hsemWriteMutex, FOREVER);
   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);

   if (DbDecCmdRefCount (hbuf))
      {
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      return FALSE;
      }

   if (!DbRemoveBufferFromList (hbuf, hwnd))
      {
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      ItiSemReleaseMutex (hbuf->hsemWriteMutex);
      return TRUE;
      }

   DbSendMsg (hbuf, WM_ITIDBDELETE,
                MPFROM2SHORT (hbuf->uType, hbuf->uId), MPFROMP (hbuf));


   FreeSEGHDR (hbuf);

   ItiMemFree (hheapDB1, hbuf->pszQuery);          /* free query  */
   ItiFreeStrArray (hheapDB3, hbuf->ppszTables, hbuf->uNumTables);

   FreePCOL (hbuf->pcol, hheapDB2, hbuf->uNumCols);
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   ItiSemReleaseMutex (hbuf->hsemWriteMutex);
   ItiMemFree (hheapDB1, hbuf->hwndUsers);
   ItiMemFree (hheapDB1, hbuf);
   return TRUE;
   }




/***********************************************************************/
/*******************  procedures  for export     ***********************/
/***********************************************************************/








HBUF EXPORT ItiDbCreateBuffer(HWND   hwnd,
                              USHORT uId,
                              USHORT uBuffType,
                              PSZ    pszQuery,
                              USHORT *uNumRows)
   {
   HBUF     hbuf;
   HMODULE  hmod;

   /* --- See if already created --- */
   if (!DbAddBufferToList (hwnd, uId, pszQuery, &hbuf))
      {
      *uNumRows = hbuf->uNumRows;

      if (!ItiSemRequestMutex (hbuf->hsemWriteMutex, 0))
         {
         ItiSemReleaseMutex (hbuf->hsemWriteMutex);
         DbSendMsg (hbuf, WM_ITIDBBUFFERDONE,
                    MPFROM2SHORT (hbuf->uType, hbuf->uNumRows),
                    MPFROMP (hbuf));
         }
      return hbuf;
      }

   *uNumRows = 0;
   ItiSemCreateMutex (NULL, 0, &(hbuf->hsemReadMutex));
   ItiSemCreateMutex (NULL, 0, &(hbuf->hsemWriteMutex));

   ItiSemRequestMutex (hbuf->hsemWriteMutex, FOREVER);
   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);


   /* the buffer type is either ITI_LIST or ITI_STATIC
    * but uType may also have ITI_DIALOG ORed with it
    * stripping the second bit removes the ITI_DIALOG
    */
   hbuf->uType    = uBuffType & ~0x0002;
   hbuf->bDlgRes  = !!(uBuffType & 0x0002);

   hbuf->uNumRows = 0;
   hbuf->pszQuery = ItiStrDup (hheapDB1, pszQuery);
   hbuf->Next     = NULL;
   hbuf->pseghdr  = NULL;
   hbuf->uSegCount = 0;
   hbuf->uNumCols = 0; /* - this triggers GetColData in fill buffer */

   DbTablesFromQuery (pszQuery, hheapDB3, &(hbuf->ppszTables),
                      &(hbuf->uNumTables));

   ItiMbQueryHMOD (uId, &hmod);
   DbWriteCmdQ (hbuf, hmod,
                (hbuf->uType == ITI_LIST ? ITIDB_FILLLIST : ITIDB_FILLSTATIC),
                hwnd);
       
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   ItiSemReleaseMutex (hbuf->hsemWriteMutex);
   return hbuf;
   }



//
//BOOL EXPORT ItiDbRedoQuery (HBUF hbuf)
//   {
//   HMODULE  hmod;
//
//   if (hbuf == NULL)
//      return FALSE;
//
//   ItiMbQueryHMOD (hbuf->uId, &hmod);
//
//   /*--- this needs sem protection
//   DbWriteCmdQ (hbuf->hheap, hbuf, hmod,
//           (hbuf->uType == ITI_LIST ? ITIDB_FILLLIST : ITIDB_FILLSTATIC));
//   return TRUE;
//   }
//
//











BOOL EXPORT ItiDbDeleteBuffer(HBUF hbuf, HWND hwnd)
   {
   if (hbuf == NULL)
      return FALSE;
   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
   DbWriteCmdQ (hbuf, (HMODULE)0, ITIDB_DELETEBUFFER, hwnd);
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   return TRUE;
   }


static void FormatCol (PSZ      pszRow,
                       HBUF     hbuf,
                       USHORT   uCol,
                       PSZ      pszCol)
   {
   USHORT   i, uRelCol = 0xFFFF;
   CHAR  szTmp[250] = "";

   /* --- possible for static buffer to not have data yet ---*/
   if (pszRow == NULL)
      {
      pszCol[0] = '\0';
      return;
      }

   /* this loop increments ptr to corrent string in row */

   /* --- uCol has relative value with high bit set --- */
   if (uCol > 32768U && uCol < 65535U)
      uRelCol = uCol - (USHORT)32769;  /* -- 1 is really 0 offset---*/
   else if (uCol == 0)
      uRelCol = 0xFFFF;
   else
      {   
      for (i = 0; i < hbuf->uNumCols; i++)
         if ((hbuf->pcol[i].uColId) == uCol)
            {
            uRelCol = i;
            break;
            }
      }
   if (uRelCol == 0xFFFF)
      {
      /*
      DbSUSErr ("Invalid Column Requested: uCol=", uCol, ItiColColIdToString (uCol));
      This was commented out for the StandardItem edit/chg units field query,
      instead use the quieter message below.
      */
       sprintf (szTmp,
          "Note: ItiDBase, Non-standard Column Requested: uCol= %d (uCol of 43 is okay here) %s", uCol, ItiColColIdToString (uCol));
       ItiErrWriteDebugMessage (szTmp);

      uRelCol = 0;
      }

   for (i = 0; i < uRelCol; i += (*((pszRow)++) == '\0'))
      ;
   strcpy (pszCol, pszRow);
   }




/*  This procedure uses the hsemReadMutex semaphore of the
 *  current buffer. This proc may not use hsemWriteMutex nor
 *  call any procedure which may use hsemReadMutex or life 
 *  will end as we know it.
 */
USHORT EXPORT ItiDbGetBufferRowCol(HBUF   hbuf,
                                   USHORT uRow,
                                   USHORT uCol,
                                   PSZ    pszCol)
   {
   USHORT uOffset, uIndex, i, uState;
   PSZ    pszRow;

   uState = ITIDB_INVALID;
   *pszCol = '\0';

   if (hbuf == NULL)
      return ITIDB_INVALID;

   if (hbuf->uType == ITI_STATIC)
      {
      ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
      pszRow = (PSZ)hbuf->pseghdr;
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      FormatCol (pszRow, hbuf, uCol, pszCol);
      return ITIDB_VALID;
      }

   if (uRow >= hbuf->uNumRows)
      return ITIDB_INVALID;

   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
   for (i = 0; TRUE; i++)
      {
      if (hbuf->pseghdr[i].uNumRows == 0 ||
          hbuf->pseghdr[i].uFirst > uRow)
         {
         ItiSemReleaseMutex (hbuf->hsemReadMutex);
         return ITIDB_INVALID;
         }

      uIndex = uRow - hbuf->pseghdr[i].uFirst;
      if (uIndex < hbuf->pseghdr[i].uNumRows)
         {
         uState = hbuf->pseghdr[i].pridx [uIndex].uState;
         uOffset = hbuf->pseghdr[i].pridx [uIndex].uOffset;
         pszRow = &(hbuf->pseghdr[i].pszBuffer [uOffset]);
         ItiSemReleaseMutex (hbuf->hsemReadMutex);

         FormatCol (pszRow, hbuf, uCol, pszCol);
         return uState;
         }
      }
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   return uState;
   }





/*  This procedure uses the hsemReadMutex semaphore of the
 *  current buffer. This proc may not use hsemWriteMutex nor
 *  call any procedure which may use hsemReadMutex or life 
 *  will end as we know it.
 */
PSZ EXPORT ItiDbGetBufferRow(HBUF hbuf, USHORT uRow, USHORT *uState)
   {
   USHORT uOffset, uIndex, i;
   PSZ    pszReturn;

   *uState = ITIDB_INVALID;

   if (hbuf == NULL)
      return NULL;

   if (hbuf->uType == ITI_STATIC)
      {
      *uState = ITIDB_VALID;
      ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
      pszReturn = (PSZ)hbuf->pseghdr;
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      return pszReturn;
      }

   if (uRow >= hbuf->uNumRows)
      return NULL;

   ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
   for (i = 0; TRUE; i++)
      {
      if (hbuf->pseghdr[i].uNumRows == 0 ||
          hbuf->pseghdr[i].uFirst > uRow)
         {
         ItiSemReleaseMutex (hbuf->hsemReadMutex);
         return NULL;
         }

      uIndex = uRow - hbuf->pseghdr[i].uFirst;
      if (uIndex < hbuf->pseghdr[i].uNumRows)
         {
         *uState = hbuf->pseghdr[i].pridx [uIndex].uState;
         uOffset = hbuf->pseghdr[i].pridx [uIndex].uOffset;
         pszReturn = &(hbuf->pseghdr[i].pszBuffer [uOffset]);
         ItiSemReleaseMutex (hbuf->hsemReadMutex);

         return pszReturn;
         }
      }
   ItiSemReleaseMutex (hbuf->hsemReadMutex);
   return NULL;
   }




//
//BOOL EXPORT ItiDbColExists (HBUF hbuf, USHORT uColId)
//   {
//   USHORT i;
//
//   if (hbuf == NULL)
//      return FALSE;
//
//   if (i = ItiSemRequestMutex (hbuf->hsemReadMutex, 10000))
//      {
//      DbErr ("Could not get semaphore in ItiDbColExists. Error =", i);
//      return FALSE;
//      }
//
//   if (uColId > 32768U)
//      {
//      ItiSemReleaseMutex (hbuf->hsemReadMutex);
//      return (hbuf->uNumCols + 32768U >= uColId);
//      }
//
//   for (i = 0; i < hbuf->uNumCols; i++)
//      if (hbuf->pcol[i].uColId == uColId)
//         {
//         ItiSemReleaseMutex (hbuf->hsemReadMutex);
//         return TRUE;
//         }
//
//   ItiSemReleaseMutex (hbuf->hsemReadMutex);
//   return FALSE;
//   }
//



BOOL EXPORT ItiDbColExists (HBUF hbuf, USHORT uColId)
   {
   USHORT i;

   if (hbuf == NULL)
      return FALSE;


   if (!hbuf->uNumCols)
      {
      ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
      ItiSemReleaseMutex (hbuf->hsemReadMutex);
      }

   if (uColId > 32768U)
      {
      return (hbuf->uNumCols + 32768U >= uColId);
      }

   for (i = 0; i < hbuf->uNumCols; i++)
      if (hbuf->pcol[i].uColId == uColId)
         {
         return TRUE;
         }

   return FALSE;
   }





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
 *| ITIDB_TABLENAME | uTable  |        pszTab   | name of table n         |
 *| ITIDB_COLTYPE   | uCol    | uColType        | type of column n        |
 *-------------------------------------------------------------------------
 */                             

MRESULT EXPORT ItiDbQueryBuffer (HBUF   hbuf,
                                 USHORT uCmd,
                                 USHORT uSubCmd)
   {
   USHORT   i;

   if (hbuf == NULL)
      return 0;

   switch (uCmd)
      {
      case ITIDB_ID       : return MPFROMSHORT (hbuf->uId);
      case ITIDB_TYPE     : return MPFROMSHORT (hbuf->uType);
      case ITIDB_HWND     : return MPFROMP     (hbuf->hwndUsers [uSubCmd]);
      case ITIDB_NUMROWS  : return MPFROMSHORT (hbuf->uNumRows);
      case ITIDB_NUMCOL   : return MPFROMSHORT (hbuf->uNumCols);
      case ITIDB_NUMTABLES: return MPFROMSHORT (hbuf->uNumTables);
      case ITIDB_QUERY    : return MPFROMP     (hbuf->pszQuery);
      case ITIDB_TABLENAME: return MPFROMP     (hbuf->ppszTables [uSubCmd]);
      case ITIDB_COLID    : return MPFROMSHORT (hbuf->pcol[uSubCmd].uColId);
      case ITIDB_HWNDCOUNT: return MPFROMSHORT (hbuf->uUsers);

      case ITIDB_COLTYPE  :
      case ITIDB_COLMAXLEN:
      case ITIDB_COLFMT   :
         for (i = 0; i < hbuf->uNumCols; i++)
            {
            if (hbuf->pcol[i].uColId != uSubCmd)
               continue;
            switch (uCmd)
               {
               case ITIDB_COLTYPE  : return MPFROMSHORT (hbuf->pcol[i].uColType);
               case ITIDB_COLMAXLEN: return MPFROMSHORT (hbuf->pcol[i].uMaxLen);
               case ITIDB_COLFMT   : return MPFROMP     (hbuf->pcol[i].pszFormat);
            }

         }
         return 0;
      }
   return 0;
   }





USHORT EXPORT ItiDbFindRow (HBUF hbuf, PSZ pszMatch, USHORT uColId, USHORT uStart)
   {
   USHORT uRows, i;
   char   szTmp [257];

   uRows = (UM) ItiDbQueryBuffer (hbuf, ITIDB_NUMROWS, 0);

   for (i=uStart; i<uRows; i++)
      {
      ItiDbGetBufferRowCol (hbuf, i, uColId, szTmp);
      if (!strcmp (szTmp, pszMatch))
         return i;
      }
   return 0xFFFF;
   }


///*  if the buffer is in the process of re-querying, this proc will
// *  wait for the dbexec to return, but will not wait for the rows
// *  to be filled in 
// */
//
//USHORT ItiDbWaitOnBuffer (HBUF hbuf)
//   {
//   if (!hbuf)
//      return 0;
//
//
//   }
