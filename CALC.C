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
 * calc.c                                                               *
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
#include "..\include\iti.h"
#include "..\include\itiutil.h"
#include "..\include\itiglob.h"
#include "..\include\itibase.h"
#include "..\include\itidbase.h"
#include "..\include\itierror.h"
#include "calc.h"
#include <stdio.h>
#include <process.h>
#include <stddef.h>

#define FOREVER SEM_INDEFINITE_WAIT


typedef struct _pcelement
   {
   struct _pcelement *forward;
   struct _pcelement *backward;
   PFNCALCFN         pfnCalc;
   PSZ               *ppszPNames;
   PSZ               *ppszParams;
   USHORT            uPriority;
   } CALCQELEMENT;

typedef CALCQELEMENT *PCALCQELEMENT;



//   Tail (put) ***--->***--->***--->***--->*** Head (get)

static PCALCQELEMENT pCalcQHead;
static PCALCQELEMENT pCalcQTail;

static HHEAP hheapCalc;

static HSEM hMutexCQ;       // access to the calc command queue
static HSEM hEventCQ;       // net entry notification


static void _cdecl CalcReaderThread (void far *nothing);
void FreePCQ (PCALCQELEMENT pcq);
USHORT PushCalcQ (PCALCQELEMENT pcq);

PCALCQELEMENT MakePCQ (PFNCALCFN pfnCalc,
                       PSZ       *ppszParams,
                       PSZ       *ppszPNames,
                       USHORT    uPriority);





/*
 * Required init fn to set up calculation queue
 *
 */

SHORT ItiDbInitCalcQ (void)
   {
   pCalcQHead = NULL;
   pCalcQTail = NULL;
   ItiSemCreateMutex (NULL, 0, &hMutexCQ);
   ItiSemCreateEvent (NULL, 0, &hEventCQ);
   ItiSemSetEvent (hEventCQ);

   hheapCalc = ItiMemCreateHeap (MAX_HEAP_SIZE);

   return _beginthread (CalcReaderThread, NULL, 0x4000, NULL);
   }





/*
 * This function is for terminating the damn thread
 * which the os cannot seem to kill
 */
void ItiDbDeInitCalcQ (void)
   {
   PCALCQELEMENT pcq;

   pcq = (PCALCQELEMENT) MakePCQ (NULL, NULL, NULL, 0xFFFF);
   PushCalcQ (pcq);
   }





/*
 * This fn will push a pcq onto the calc command queue
 *  in order of priority
 * returns insert position (as if anyone really cared)
 */

USHORT PushCalcQ (PCALCQELEMENT pcq)
   {
   PCALCQELEMENT ptmpPrev = NULL;
   PCALCQELEMENT ptmpNext = NULL;
   PCALCQELEMENT ptmp;
   USHORT        i = 0;

   ItiSemRequestMutex (hMutexCQ, FOREVER);

   ptmp = pCalcQTail;

   while (ptmp && ptmp->uPriority < pcq->uPriority)
      ptmpPrev = ptmp++, i++;

   pcq->forward  = ptmp;
   pcq->backward = ptmpPrev;

   if (ptmp)
      ptmp->backward = pcq;
   else
      pCalcQHead = pcq;

   if (ptmpPrev)
      ptmpPrev->forward = pcq;
   else
      pCalcQTail = pcq;

   ItiSemClearEvent (hEventCQ);    /*--- notify reader of a new entry ---*/
                                   /*--- in the event he is asleep    ---*/
   ItiSemReleaseMutex (hMutexCQ);
   return i;
   }



/*
 *  Pops the pcq from the calc command queue
 *  returns 0 if there is nothing to return;
 */

USHORT PopCalcQ (PCALCQELEMENT *ppcq)
   {
   ItiSemRequestMutex (hMutexCQ, FOREVER);

   *ppcq = pCalcQHead;

   if (pCalcQHead)
      pCalcQHead = pCalcQHead->backward;
   if (!pCalcQHead)
      pCalcQTail = NULL;

   ItiSemReleaseMutex (hMutexCQ);

   return !!(*ppcq);
   }


/*
 * DO NOT CALL THIS FN!
 * for this is a place of no return.
 * this fn is called by, and is home to, the calc thread
 */

static void _cdecl CalcReaderThread (void far *nothing)
   {
   PCALCQELEMENT  pcq;
   HAB            hab;
   HMQ            hmq;

   /* lower the calc threads priority slightly */
   DosSetPrty (PRTYS_THREAD, PRTYC_REGULAR, -2, 0);

   /* do this so OS/2 doesn't crash */
   hab = WinInitialize (0);
   hmq = WinCreateMsgQueue (hab, 10);

   while (TRUE)
      {
//  ItiSemRequestMutex (hStopCalcQ, FOREVER);
//  ItiSemReleaseMutex (hStopCalcQ, FOREVER);

      ItiSemWaitEvent (hEventCQ, FOREVER);   /*-- sleep while q is empty --*/

      while (PopCalcQ (&pcq))
         {
         if (!pcq->ppszPNames && !pcq->ppszParams && !pcq->pfnCalc)
            {
            WinDestroyMsgQueue (hmq);
            WinTerminate (hab);
            return;
            }

         pcq->pfnCalc (hheapCalc, pcq->ppszPNames, pcq->ppszParams);

         FreePCQ (pcq);
         }

      ItiSemSetEvent (hEventCQ);
      }
   }


/*
 * if uCount = 0, the assumption is made that the entrys are null
 * terminated in both arrays
 */

PCALCQELEMENT MakePCQ (PFNCALCFN pfnCalc,
                       PSZ       *ppszParams,
                       PSZ       *ppszPNames,
                       USHORT    uPriority)
   {
   USHORT        i, uCount, usSizeRequest, usAvailable;
   PCALCQELEMENT pcq;

   pcq = (PCALCQELEMENT) ItiMemAlloc (hheapCalc, sizeof (CALCQELEMENT), 0);
   pcq->forward    = NULL;
   pcq->backward   = NULL;
   pcq->pfnCalc    = pfnCalc;
   pcq->uPriority  = uPriority;
   pcq->ppszParams = NULL;
   pcq->ppszPNames = NULL;

   if (!ppszParams && !ppszPNames)
      return pcq;


   for (uCount = 1; ppszPNames[uCount-1]; uCount++)
      ;

   /* -- Need to first test available memory space in the heap. 2 JUN 94 */
   usSizeRequest = sizeof (PSZ) * uCount;
   usAvailable = ItiMemQueryAvail (hheapCalc);
   if (usAvailable > usSizeRequest)
      {
      pcq->ppszParams = (PSZ *) ItiMemAlloc (hheapCalc, sizeof (PSZ) * uCount, 0);
      pcq->ppszPNames = (PSZ *) ItiMemAlloc (hheapCalc, sizeof (PSZ) * uCount, 0);
      }
   else
      {
      // Make note of memory lack in the log.
      ItiErrWriteDebugMessage ("Calc thread, MakePCQ ran out of heap space, will attempt to Realloc...");
      pcq->ppszParams = (PSZ *) ItiMemRealloc (hheapCalc, NULL, sizeof (PSZ) * uCount, 0);
      pcq->ppszPNames = (PSZ *) ItiMemRealloc (hheapCalc, NULL, sizeof (PSZ) * uCount, 0);
      }

   for (i=0; i < uCount-1; i++)
      {
      pcq->ppszParams[i] = NULL;
      pcq->ppszPNames[i] = NULL;

      if (ppszParams[i])
         pcq->ppszParams[i] = ItiStrDup (hheapCalc, ppszParams[i]);
      if (ppszPNames[i])
         pcq->ppszPNames[i] = ItiStrDup (hheapCalc, ppszPNames[i]);
      }
   pcq->ppszParams[uCount-1] = NULL;
   pcq->ppszPNames[uCount-1] = NULL;

   return pcq;
   }




void FreePCQ (PCALCQELEMENT pcq)
   {
   USHORT i;

   for (i=0; pcq->ppszPNames[i]; i++)
      {
      if (pcq->ppszParams[i])
         ItiMemFree (hheapCalc, pcq->ppszParams[i]);
      if (pcq->ppszPNames[i])
         ItiMemFree (hheapCalc, pcq->ppszPNames[i]);
      }
   ItiMemFree (hheapCalc, pcq->ppszParams);
   ItiMemFree (hheapCalc, pcq->ppszPNames);
   ItiMemFree (hheapCalc, pcq);
   }








/*
 * This is the external function used to perform all calculations.
 *
 * pfnCalc ....... Ptr to calc function
 * *ppszParams ... key values
 * *ppszPNames ... key names
 * uPriority ..... priority level >Num = >priority (0 = default priority)
 *
 * returns insert position relative to tail (as if anyone really cared)
 *
 */

USHORT EXPORT ItiDBDoCalc ( PFNCALCFN pfnCalc,
                     PSZ       *ppszPNames,
                     PSZ       *ppszParams,
                     USHORT    uPriority)
   {
   PCALCQELEMENT pcq;

   uPriority = (uPriority ? uPriority : 100);
   pcq = MakePCQ (pfnCalc, ppszParams, ppszPNames, uPriority);
   return PushCalcQ (pcq);
   }

