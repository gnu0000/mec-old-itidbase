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
 * dbqueue.c                                                            *
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
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "..\INCLUDE\itierror.h"
#include "dbase.h"
#include "dbqueue.h"
#include "dbutil.h"
#include "dbbuf.h"
#include "buflist.h"
#include <stdio.h>

#define FOREVER SEM_INDEFINITE_WAIT   


/*--- buflist.c ---*/
BOOL RemoveHwndFromList (HBUF hbuf, HWND hwnd);



/***************************************************************/
/*                   cmd queue procedures                      */
/***************************************************************/

/*  --- QELEMENT ---
 * This structure defines the elements
 * of the queue's linked list
 */
typedef struct _qelement
   {
   struct _qelement  *forward;
   struct _qelement  *backward;
   HBUF              hbuf;
   HMODULE           hmod;
   USHORT            uCmd;
   HWND              hwnd;
   } QELEMENT;

typedef QELEMENT *PQELEMENT;


/*
 * The following semaphores are used to controll the access to
 * the queue
 */
static HSEM hsemQMutex;       // access to the queue
static HSEM hsemQEvent;       // event - cmd added to queue

/*
 * These are the pointers to the linked list
 */
PQELEMENT pCmdQHead;   // head of command queue (read side)
PQELEMENT pCmdQTail;   // tail of command queue (write side)





/*
 * This procedure must be called before the command queue can be used
 */
void DbInitCmdQ (void)
   {
   pCmdQHead = pCmdQTail = NULL;
   ItiSemCreateMutex (NULL, 0, &hsemQMutex);
   ItiSemCreateEvent (NULL, 0, &hsemQEvent);
   /*--- set event semaphore if not done by default ---*/
   }

BOOL PqeExists (PQELEMENT pqe)
   {
   PQELEMENT pqePtr;

   pqePtr = pCmdQTail;

   while (pqePtr != NULL)
      {
      if (pqe->hbuf == pqePtr->hbuf &&
          pqe->uCmd == pqePtr->uCmd)
         return TRUE;
      pqePtr = pqePtr->forward;
      }
   return FALSE;
   }


BOOL AddToHead (PQELEMENT pqe, BOOL bUnique)
   {
   if (bUnique && PqeExists (pqe))
      return FALSE;

   pqe->forward  = NULL;
   pqe->backward = pCmdQHead;
   if (pCmdQHead != NULL)
      pCmdQHead->forward = pqe;
   else
      pCmdQTail = pqe;
   pCmdQHead = pqe;
   return TRUE;
   }


BOOL AddToTail (PQELEMENT pqe, BOOL bUnique)
   {
   if (bUnique && PqeExists (pqe))
      return FALSE;

   pqe->forward  = pCmdQTail;
   pqe->backward = NULL;
   if (pCmdQTail != NULL)
      pCmdQTail->backward = pqe;
   else
      pCmdQHead = pqe;
   pCmdQTail = pqe;
   return TRUE;
   }

PQELEMENT RemoveFromQueue (void)
   {
   PQELEMENT pqe;
   /*--- remove command from queue ---*/
   pqe = pCmdQHead;
   if (pqe->backward != NULL)
      {
      pCmdQHead = pqe->backward;
      pCmdQHead->forward = NULL;
      }
   else
      {
      pCmdQTail = pCmdQHead = NULL;
      }
   return pqe;
   }









/*
 * this can be called by anyone who wishes to
 * perform a buffer update. It returns right away
 *
 * it assumes the associated hbuf is safe to read from
 *
 * ITI_STATIC ITI_LIST
 *
 */
void DbWriteCmdQ (HBUF hbuf, HMODULE hmod, USHORT uCmd, HWND hwnd)
   {
   PQELEMENT   pqe;
   BOOL        bAdd = FALSE;

   ItiSemRequestMutex (hsemQMutex, FOREVER);

   /*--- buffer is already marked for deletion ---*/
   if (!hbuf || !hbuf->uRefCount)
      {
      ItiSemReleaseMutex (hsemQMutex);
      return;
      }

   pqe = (PQELEMENT) ItiMemAlloc (hheapDB1, sizeof (QELEMENT), 0);
   pqe->hbuf = hbuf;     
   pqe->uCmd = uCmd;
   pqe->hmod = hmod;
   pqe->hwnd = hwnd;

   if (uCmd == ITIDB_DELETEBUFFER)
      {
      /*--- do not add message if buf should not be deleted ---*/
      if (DbDecRefCount (hbuf))
         {
         RemoveHwndFromList (hbuf, hwnd);
         ItiMemFree (hheapDB1, pqe);
         }
      else
         {
         if (bAdd = AddToTail (pqe, TRUE)) /*-- AddToHead (pqe, TRUE)) --*/
            DbIncCmdRefCount (hbuf);
         }
      }
   else /*--- fill buffer cmd ---*/
      {
      if (bAdd = AddToTail (pqe, TRUE))
         DbIncCmdRefCount (hbuf);
      }
   ItiSemReleaseMutex (hsemQMutex);
   if (bAdd)
      ItiSemClearEvent (hsemQEvent);
   }



/*
 * this is called only by threads created to execute the 
 * buffer update. Threads do not return from here. Each 
 * invocation of this proc must have a unique hdb.
 */
void _cdecl DbReadCmdQ (HDB hdb)
   {
   PQELEMENT   pqe;
   HMQ         hmq;
   HAB         hab;
   BOOL        bAdd;

   hab = WinInitialize (0);
   hmq = WinCreateMsgQueue (hab, DEFAULT_QUEUE_SIZE);

#if defined (DEBUG)
   {
   char szTemp [256];
   sprintf (szTemp, "DBPROCESS (HDB) for this queue thread is %p", hdb);
   ItiErrWriteDebugMessage (szTemp);
   }
#endif

   while (TRUE)
      {
      ItiSemWaitEvent (hsemQEvent, FOREVER);

      /* --- loop until queue is clear --- */
      while (TRUE)
         {
         ItiSemRequestMutex (hsemQMutex, FOREVER);
         if (pCmdQHead != NULL)
            {
            pqe = RemoveFromQueue ();

            /*--- if busy, replace cmd in back of queue ---*/
            if (ItiSemRequestMutex (pqe->hbuf->hsemWriteMutex, 0))
               {
               bAdd = AddToTail (pqe, FALSE);
               ItiSemReleaseMutex (hsemQMutex);
               /*--- wait a sec (slow down loop)---*/
               DosSleep (1000L);
               if (bAdd)
                  ItiSemClearEvent (hsemQEvent);
               continue;
               }

            ItiSemReleaseMutex (pqe->hbuf->hsemWriteMutex);
            ItiSemReleaseMutex (hsemQMutex);
               
            /*--- execute command ---*/
            switch (pqe->uCmd)
               {
               case ITIDB_FILLLIST:
                  DbFillListBuffer (pqe->hbuf, hdb, pqe->hmod);
                  break;

               case ITIDB_FILLSTATIC:
                  DbFillStaticBuffer (pqe->hbuf, hdb, pqe->hmod);
                  break;

               case ITIDB_DELETEBUFFER:
                  DbDelBuf (pqe->hbuf, pqe->hwnd);
                  break;
               }
            ItiMemFree (hheapDB1, pqe);
            }
         else
            {
            ItiSemSetEvent (hsemQEvent);
            ItiSemReleaseMutex (hsemQMutex);
            break;
            }
         }
      } /*--- bottom of infinite loop ---*/
   }
