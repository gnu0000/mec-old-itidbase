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
 * buflist.c                                                            *
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
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <string.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "..\INCLUDE\itimbase.h"
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itierror.h"
#include "buflist.h"
#include "dbqueue.h"
#include "..\INCLUDE\itiglob.h"
#include "dbase.h"
#include "initdll.h"

#define FOREVER SEM_INDEFINITE_WAIT   

static HSEM hsemLLMutex;       /* access to the queue           */
static HBUF hbufTop    = NULL; /* initialize ptr to Buff Header */
static HBUF hbufBottom = NULL; /* pointer to last link in list  */


/*
 * This must be called before attempting to use
 * the linked list
 */
void DbInitLL (void)
   {
   ItiSemCreateMutex (NULL, 0, &hsemLLMutex);
   hbufTop    = NULL;
   hbufBottom = NULL;
   }



USHORT DbIncRefCount (HBUF hbuf)
   {
   hbuf->uRefCount++;
   return hbuf->uRefCount;
   }

USHORT DbDecRefCount (HBUF hbuf)
   {
   if (hbuf->uRefCount)
      hbuf->uRefCount--;
   return hbuf->uRefCount;
   }

USHORT DbIncCmdRefCount (HBUF hbuf)
   {
   hbuf->uCmdRefCount++;
   return hbuf->uCmdRefCount;
   }

USHORT DbDecCmdRefCount (HBUF hbuf)
   {
   if (hbuf->uCmdRefCount)
      hbuf->uCmdRefCount--;
   return hbuf->uCmdRefCount;
   }



/*  0  = hwnd already present
 *  !0 = added (val = added index)
 */
static BOOL AddHwndToList (HBUF hbuf, HWND hwnd)
   {
   USHORT i;

   if (hbuf == NULL || hwnd == NULL)
      return 0;

   for (i = 0; i < hbuf->uUsers; i++)
      if (hbuf->hwndUsers[i] == hwnd)
         return 0;

   hbuf->uUsers++;
   hbuf->hwndUsers = (HWND *) ItiMemRealloc (hheapDB1, hbuf->hwndUsers,
                                             hbuf->uUsers * sizeof (HBUF), 0);
   hbuf->hwndUsers[hbuf->uUsers-1] = hwnd;
   return hbuf->uUsers-1;
   }



/*
 *  0 = hwnd not present to remove
 * !0 = removed (val = index removed)
 */
BOOL RemoveHwndFromList (HBUF hbuf, HWND hwnd)
   {
   USHORT i, j;

   if (hbuf == NULL || hwnd == NULL)
      return 0;

   for (i = 0; i < hbuf->uUsers; i++)
      if (hbuf->hwndUsers[i] == hwnd)
         {
         for (j = i+1; j < hbuf->uUsers; j++)
            hbuf->hwndUsers[j-1] = hbuf->hwndUsers[j];
         hbuf->uUsers--;
         hbuf->hwndUsers = (HWND *) ItiMemRealloc (hheapDB1, hbuf->hwndUsers,
                            hbuf->uUsers * sizeof (HBUF), 0);
         return i;
         }
   return 0;
   }




/* 
 * Adds a new buffer to the list. If buffer is already created
 * its reference count is increased and the existing buffer is
 * returned.
 * this fn returns TRUE if the buffer is created and FALSE
 * if the buffer already exists.
 */

BOOL DbAddBufferToList (HWND   hwnd,
                        USHORT uId,
                        PSZ    pszNewQuery,
                        HBUF   *phbuf)
   {
   ItiSemRequestMutex (hsemLLMutex, FOREVER); /* exclusive access to LList */

   *phbuf = hbufTop;                        /* search for existing buf */
   while (*phbuf != NULL)
      {
      if ((*phbuf)->uId == uId)             /* if id is found, make sure */
         {                                  /* the query is the same     */
         if (!ItiStrCmp ((*phbuf)->pszQuery, pszNewQuery))
            break;
         }
      *phbuf = (*phbuf)->Next;
      }

   if (*phbuf != NULL)                      /* buff already created */
      {
      DbIncRefCount (*phbuf);
      AddHwndToList (*phbuf, hwnd);
      ItiSemReleaseMutex (hsemLLMutex);
      return FALSE;
      }

   *phbuf = (HBUF) ItiMemAlloc (hheapDB1, sizeof (BUF), 0);     /* create */
   (*phbuf)->uId          = uId;
   (*phbuf)->uRefCount    = 1;
   (*phbuf)->uCmdRefCount = 0;

   (*phbuf)->hsemWriteMutex = NULL;
   (*phbuf)->hsemReadMutex  = NULL;
//   (*phbuf)->hwnd           = NULL;
   (*phbuf)->Next           = NULL;
   (*phbuf)->ppszTables     = NULL;
   (*phbuf)->uNumTables     = 0;
   (*phbuf)->uNumCols       = 0;
   (*phbuf)->pcol           = NULL;
   (*phbuf)->pszQuery       = NULL;
   (*phbuf)->pseghdr        = NULL;
   (*phbuf)->hwndUsers      = NULL;
   (*phbuf)->uUsers         = 0;

   if (hbufTop != NULL)                     /* add to llist */
      {
      hbufBottom->Next = *phbuf;
      hbufBottom = *phbuf;
      }
   else
      {
      hbufTop = hbufBottom = *phbuf;
      }
   AddHwndToList (*phbuf, hwnd);
   ItiSemReleaseMutex (hsemLLMutex);
   return TRUE;
   }



/*
 * removes hbuf. If the hbuf reference count > 1 the buffer is not
 * actually removed. This fn does NOT free any memory!.
 * this fn returns TRUE if the hbuf is removed.
 *
 */
BOOL DbRemoveBufferFromList (HBUF hbuf, HWND hwnd)
   {
   HBUF  hbufTmp;

   if (hbuf == NULL)                      /* invalid request */
      return FALSE;
      
   RemoveHwndFromList (hbuf, hwnd);
   ItiSemRequestMutex (hsemLLMutex, FOREVER);

   if (hbuf->uRefCount)
      {
      ItiSemReleaseMutex (hsemLLMutex);
      return FALSE;
      }

   hbufTmp = hbufTop;
   if (hbufTop == hbufBottom)             /* if only entry   */
      hbufTop = hbufBottom = NULL;

   else if (hbuf == hbufTop)              /* if first entry  */
      hbufTop = hbufTop->Next;

   else
      {
      while (hbufTmp->Next != NULL)       /* other cases     */
         {
         if (hbufTmp->Next == hbuf)
            {
            if ((hbufTmp->Next = hbuf->Next) == NULL)
               hbufBottom = hbufTmp;
            break;
            }
         hbufTmp = hbufTmp->Next;
         }
      }
   ItiSemReleaseMutex (hsemLLMutex);
   return TRUE;
   }





/*
 * returns NULL if not found
 */
HBUF DbFindBuffer (USHORT uId)
   {
   HBUF  hbuf;

   hbuf = hbufTop;
   ItiSemRequestMutex (hsemLLMutex, FOREVER);
   while (hbuf != NULL)
      {
      if (hbuf->uId == uId)
         {
         ItiSemReleaseMutex (hsemLLMutex);
         return hbuf;
         }
      hbuf = hbuf->Next;
      }
   ItiSemReleaseMutex (hsemLLMutex);
   return NULL;
   }



//void EXPORT ItiDbRedoQuerys (USHORT uBuffId)
//   {
//   HMODULE  hmod;
//
//   ItiMbQueryHMOD (uBuffId, &hmod);
//   ItiSemRequestMutex (hsemLLMutex, FOREVER);
//   while (hbuf != NULL)
//      {
//      if (hbuf->uId == uBuffId)
//         {
//         DbWriteCmdQ (hbuf->hheap, hbuf, hmod,
//           (hbuf->uType == ITI_LIST ? ITIDB_FILLLIST : ITIDB_FILLSTATIC));
//         }
//      hbuf = hbuf->Next;
//      }
//   ItiSemReleaseMutex (hsemLLMutex);
//   }
//





BOOL DbBufferHasTable (HBUF hbuf, PSZ pszTable)
   {
   USHORT i;

   for (i = 0; i < hbuf->uNumTables ; i++)
      if (!stricmp (hbuf->ppszTables[i], pszTable))
         return TRUE;
   return FALSE;
   }



/* returns true if any buffers are marked for change
 */
BOOL EXPORT ItiDbUpdateBuffers (PSZ pszChangedTable)
   {
   BOOL     bReturn = FALSE;
   HBUF     hbuf;
   HMODULE  hmod;

#if defined (DEBUG)
     if (bTraceBuffer)
      ItiErrWriteDebugMessage (pszChangedTable);
#endif

   hbuf = hbufTop;
   ItiSemRequestMutex (hsemLLMutex, FOREVER);
   while (hbuf != NULL)
      {
      if (hbuf->uRefCount && DbBufferHasTable (hbuf, pszChangedTable))
         {
         bReturn = TRUE;
         ItiMbQueryHMOD (hbuf->uId, &hmod);
         ItiSemRequestMutex (hbuf->hsemReadMutex, FOREVER);
         DbWriteCmdQ (hbuf, hmod,
                      (hbuf->uType == ITI_LIST ? ITIDB_FILLLIST : ITIDB_FILLSTATIC),
                      NULL);
         ItiSemReleaseMutex (hbuf->hsemReadMutex);
         }
      hbuf = hbuf->Next;
      }
   ItiSemReleaseMutex (hsemLLMutex);
   return bReturn;
   }

