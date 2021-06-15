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
 * qbutton.c                                                            *
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
#include "..\include\itiutil.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itiwin.h"
#include "..\INCLUDE\itidbase.h"
#include "dbkernel.h"
#include "dbase.h"
#include "dbbuf.h"
#include "dbparse.h"
#include "dbutil.h"
#include "dbqueue.h"
#include "buflist.h"
#include "..\include\colid.h"
#include "qbutton.h"
#include "initdll.h"

typedef struct
   {
   HBUF  hbuf;
   HWND  hwnd;
   } JUNK;

typedef JUNK *PJUNK;


MRESULT EXPORT QButtonDlg (HWND   hwnd,
                           USHORT usMessage,
                           MPARAM mp1,
                           MPARAM mp2)
   {
   static PJUNK  pjunk;

   switch (usMessage)
      {
      case WM_INITDLG:
         {
         char  szTemp [1024];
         HWND  hwndListBox;
         USHORT i;
         MPARAM mp11, mp22;

         pjunk = (PJUNK) PVOIDFROMMP (mp2);
         if (pjunk->hbuf == NULL)
            {
            WinSetDlgItemText (hwnd, QBQUERY, "Null HBUF!");
            break;
            }

         /* --- put query in edit box --- */
         WinSetDlgItemText (hwnd, QBQUERY, pjunk->hbuf->pszQuery);

         /* --- put data in buff stuff list box --- */
         hwndListBox = WinWindowFromID (hwnd, QBBUFFSTUFF);
         mp11 = MPFROMSHORT (LIT_END);
         mp22 = MPFROMP (szTemp);
   
         sprintf (szTemp, "HBUF = %p", pjunk->hbuf);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "REF COUNT = %d", pjunk->hbuf->uRefCount);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "CMD REF COUNT =  %d", pjunk->hbuf->uCmdRefCount);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "ID = %u",        pjunk->hbuf->uId);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "ROWS = %u",   pjunk->hbuf->uNumRows);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "COLUMNS = %u",   pjunk->hbuf->uNumCols);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "SEGMENTS = %u",  pjunk->hbuf->uSegCount);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "windows = %d",    pjunk->hbuf->uUsers);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);

         sprintf (szTemp, "==GLOBAL==");
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP0 = %p",      hheapDB0);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP0 size = %u",      ItiMemQuerySeg (hheapDB0));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP0 free = %u",      ItiMemQueryAvail (hheapDB0));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP1 = %p",      hheapDB1);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP1 size = %u",      ItiMemQuerySeg (hheapDB1));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP1 free = %u",      ItiMemQueryAvail (hheapDB1));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP2 = %p",      hheapDB2);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP2 size = %u",      ItiMemQuerySeg (hheapDB2));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP2 free = %u",      ItiMemQueryAvail (hheapDB2));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP3 = %p",      hheapDB3);
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP3 size = %u",      ItiMemQuerySeg (hheapDB3));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         sprintf (szTemp, "HEAP3 free = %u",      ItiMemQueryAvail (hheapDB3));
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);

         sprintf (szTemp, "==WINDOWS==");
         WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
         for (i = 0; i < pjunk->hbuf->uUsers; i++)
            {
            sprintf (szTemp, "%p",   pjunk->hbuf->hwndUsers[i]);
            WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
            }

         /* --- put cols in col list box --- */
         hwndListBox = WinWindowFromID (hwnd, QBCOLNAMES);
         for (i = 0; i < pjunk->hbuf->uNumCols; i++)
            {
            sprintf (szTemp, "(%d)-%s-%s",
                     pjunk->hbuf->pcol[i].uColId,
                     ItiColColIdToString (pjunk->hbuf->pcol[i].uColId),
                     pjunk->hbuf->pcol[i].pszFormat);
            WinSendMsg (hwndListBox, LM_INSERTITEM, mp11, mp22);
            }
         }
         break;

      case WM_COMMAND:
         switch (SHORT1FROMMP (mp1))
            {
            case DID_OK:
               {
               PSZ pszTemp;
               
               pszTemp = (PSZ) ItiMemAlloc (hheapDB0, 4096, 0);

               if (WinQueryDlgItemText (hwnd, QBQUERY, 4096, pszTemp)
                   && ItiStrCmp (pszTemp, pjunk->hbuf->pszQuery) != 0)
                  {
                  WinSendMsg (pjunk->hwnd, WM_CHANGEQUERY, MPFROMLONG (-1), 
                              MPFROMP (pszTemp));
                  }
               ItiMemFree (hheapDB0, pszTemp);

               WinDismissDlg (hwnd, TRUE);
               }
               return 0;
               break;
            
            case DID_CANCEL:
               WinDismissDlg (hwnd, FALSE);
               return 0;
               break;
            }
         break;
      }
   return WinDefDlgProc (hwnd, usMessage, mp1, mp2);
   }



void EXPORT ItiDbQButton (HWND hwnd, HBUF hbuf)
   {
   JUNK junk;

   junk.hwnd = hwnd;
   junk.hbuf = hbuf;
   WinDlgBox (HWND_DESKTOP, hwnd, QButtonDlg, hmodModule, 1000, &junk);
   }
