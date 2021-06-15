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
 * dbutil.c                                                             *
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
#include "..\INCLUDE\itiwin.h"
#include "..\INCLUDE\itiglob.h"
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itierror.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "dbase.h"
#include "dbUtil.h"
#include <string.h>
#include <stdio.h>



BOOL DbErr (PSZ psz, USHORT uInfo)
   {
   char  szTmp [255];

   if (uInfo == 0)
      sprintf (szTmp, "ITIDBASE: %s", psz);
   else
      sprintf (szTmp, "ITIDBASE: %s %d", psz, uInfo);
   ItiErrWriteDebugMessage (szTmp);

//   DosBeep (500, 50);
//   DosBeep (1000, 50);
   DosBeep (1000, 20);
   DosBeep (2000, 20);

   if (pglobals != NULL && 
       pglobals->hwndDesktop != NULL &&
       pglobals->hwndAppFrame != NULL)
      {
      WinMessageBox (pglobals->hwndDesktop, pglobals->hwndAppFrame, szTmp,
                     "ITIDBASE", 0, MB_OK | MB_SYSTEMMODAL | MB_MOVEABLE);
      }

   return (FALSE);
   }


BOOL DbSUSErr (PSZ psz, USHORT uInfo, PSZ psz2)
   {
   char  szTmp [2048];

   sprintf (szTmp, "ITIDBASE: %s %d %s", psz, uInfo, psz2);
   ItiErrWriteDebugMessage (szTmp);

   DosBeep (1000, 20);
   DosBeep (2000, 20);

   if (pglobals != NULL && 
       pglobals->hwndDesktop != NULL &&
       pglobals->hwndAppFrame != NULL)
      {
      WinMessageBox (pglobals->hwndDesktop, pglobals->hwndAppFrame, szTmp,
                     "ITIDBASE", 0, MB_OK | MB_SYSTEMMODAL | MB_MOVEABLE);
      }

   return (FALSE);
   }



           
PSZ EXPORT ItiDbGetZStr (PSZ pszZStr, USHORT uIndex)
   {
   USHORT i;

   for (i = 0; i < uIndex; )
      if (*pszZStr++ == '\0')
         {
         i++;
         if (*pszZStr == '\0')
            return NULL;
         }
   return pszZStr;
   }

// this code has never been tested.  use at your own risk.
//
//PSZ *_far _cdecl _loadds ItiDbMakeZStr (HHEAP hheap, USHORT usCount, ...)
//   {
//   va_list  va;
//   PSZ      psz, pszZStr;
//   USHORT   usLen, i;
//
//   if (usCount == 0)
//      return NULL;
//
//   va_start (va, usCount);      /* Initialize variable arguments  */
//
//   for (usLen = 1, i=0; i < usCount; i++)
//      {
//      psz = va_arg (va, PSZ);
//
//      usLen += (psz ? ItiStrLen (psz) + 1 : 1);
//      }
//
//   va_start (va, usCount);      /* Initialize variable arguments  */
//   pszZStr = ItiMemAlloc (hheap, usLen, 0);
//   if (pszZStr == NULL)
//      return NULL;
//
//   for (usLen = 0, i=0; i < usCount; i++)
//      {
//      psz = va_arg (va, PSZ);
//
//      pszZStr = (psz ? strcpy (pszZstr + usLen, psz) : NULL);
//      usLen = ItiStrLen (psz) + 1;
//      }
//   pszZStr [usLen] = '\0';
//   return pszZStr;
//   }


VOID EXPORT ItiDbAddZStr (HHEAP hheap, PSZ *ppszZStr, PSZ psz)
   {
   PSZ     p, p2;
   USHORT  uLen;

   uLen = (psz ? strlen (psz) : 0);

   if (*ppszZStr == NULL)
      {
      *ppszZStr = ItiMemAlloc (hheap, uLen + 2, 0);
      strcpy (*ppszZStr, psz);
      (*ppszZStr)[strlen (psz) + 1] = '\0';
      return;
      }

   p = *ppszZStr;
   while (*p != '\0')
      while (*p++ != '\0')
         ;
   p2 = ItiMemAlloc (hheap, p - *ppszZStr + uLen + 2, 0);

   memcpy (p2, *ppszZStr, p - *ppszZStr);

   if (psz)
      strcpy (p2 + (p - *ppszZStr), psz);

   p2[p - *ppszZStr + uLen] = '\0';
   p2[p - *ppszZStr + uLen + 1] = '\0';
   ItiMemFree (hheap, *ppszZStr);
   *ppszZStr = p2;
   }







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
BOOL EXPORT ItiDbBuildSelectedKeyArray (HWND  hwnd,
                                       HHEAP  hheap,
                                       PSZ    **pppszKeys,
                                       USHORT uKeys[])
   {
   USHORT i, uSel, uSelCount, uMatchLen;
   PSZ    pszCol;
   char   szMatchStr[512];

   uSelCount = 0;
   *pppszKeys = NULL;
   uSel = 0;

   *pppszKeys = (PSZ *) ItiMemRealloc (hheap, *pppszKeys, sizeof (PSZ), 0);

   while (0xFFFF != (uSel = (UM) QW (hwnd, ITIWND_SELECTION, uSel, 0, 0)))
      {
      uSelCount++;

      *pppszKeys = (PSZ *) ItiMemRealloc (hheap, *pppszKeys,
                                          (uSelCount+1) * sizeof (PSZ), 0);
      uMatchLen = 0;
      for (i=0; uKeys[i] != 0; i++)
         {
         pszCol = (PSZ) QW (hwnd, ITIWND_DATA, 0, uSel, uKeys[i]);

         strcpy (szMatchStr + uMatchLen, pszCol);
         uMatchLen += strlen (pszCol) + 1;
         }

      szMatchStr[uMatchLen++] = '\0'; /*---double null terminate---*/
      (*pppszKeys)[uSelCount -1] = ItiMemDup (hheap, szMatchStr, uMatchLen);
      uSel++;

      /*--- 11/16/92 fix infinite loop with static querys -CLF ---*/
      if ((UM) QW (hwnd, ITIWND_TYPE, 0, 0, 0) == ITI_STATIC)
         break;
      }

   (*pppszKeys)[uSelCount] = NULL;
   return 0;
   }


                                 
                                 
                                 
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

USHORT EXPORT ItiDbBuildSelectedKeyArray2 (HHEAP    hheap,
                                           HMODULE  hmod,
                                           PSZ      pszQuery,
                                           PSZ      **pppszKeys,
                                           USHORT   uKeys[])
   {
   USHORT i, uSel, uSelCount, uMatchLen;
   char   szMatchStr[512];
   HQRY    hqry;
   USHORT  uCols, uErr;
   PSZ     *ppszTmp;

   uSelCount = 0;
   *pppszKeys = NULL;
   uSel = 0;

   if (!(hqry = ItiDbExecQuery(pszQuery, hheap, hmod, 0, 0, &uCols, &uErr)))
      return 1;

   for (i=0; uKeys[i] != 0; i++)
      if (uKeys[i] > uCols)
         return 3;

   *pppszKeys = (PSZ *) ItiMemRealloc (hheap, *pppszKeys, sizeof (PSZ), 0);

   while (ItiDbGetQueryRow(hqry, &ppszTmp, &uErr))
      {
      uSelCount++;

      *pppszKeys = (PSZ *) ItiMemRealloc (hheap, *pppszKeys,
                                          (uSelCount+1) * sizeof (PSZ), 0);
      uMatchLen = 0;
      for (i=0; uKeys[i] != 0; i++)
         {
         strcpy (szMatchStr + uMatchLen, ppszTmp[uKeys[i]-1]);
         uMatchLen += strlen (ppszTmp[uKeys[i]-1]) + 1;
         }

      szMatchStr[uMatchLen++] = '\0'; /*---double null terminate---*/
      (*pppszKeys)[uSelCount -1] = ItiMemDup (hheap, szMatchStr, uMatchLen);
      uSel++;

      ItiFreeStrArray (hheap, ppszTmp, uCols);
      }
   (*pppszKeys)[uSelCount] = NULL;
   return 0;
   }


USHORT EXPORT ItiDbFreeZStrArray (HHEAP hheap, PPSZ ppszKeys)
   {
   USHORT i;

   for (i=0; ppszKeys[i]; i++)
      ItiMemFree (hheap, ppszKeys[i]);
   ItiMemFree (hheap, ppszKeys);
   return i;
   }

USHORT EXPORT ItiDbXlateKeys (HHEAP hhp, PSZ pszInBuffer, PSZ pszOutBuffer, USHORT usOutSize)
   {
#define  BUFLEN  1000
   HQRY hqry;
   PSZ pszWrk, pszRoad, pszDist, pszMI, pszKey, pszMarker, pszX, pszN; 
   BOOL bW, bR, bD, bM;
   USHORT usCnt, uCols, uErr, usLen;
   PSZ *ppszTmp;
   CHAR szKeyList[BUFLEN] = "";
   CHAR szIDList [BUFLEN] = "";

   pszWrk  = pszInBuffer;
   pszRoad = pszInBuffer;
   pszDist = pszInBuffer;
   pszMI  = pszInBuffer;


   /* -- WorkType section. */
   bW = FALSE;
   while ( (pszWrk != NULL) && ( (*pszWrk) != '\0' ) )
      {
      if (   ((*pszWrk) == 'W') 
          && ((*(pszWrk+1)) == 'o') 
          && ((*(pszWrk+2)) == 'r') 
          && ((*(pszWrk+3)) == 'k') 
          && ((*(pszWrk+4)) == 'T') 
          && ((*(pszWrk+5)) == 'y') 
          && ((*(pszWrk+6)) == 'p') 
          && ((*(pszWrk+7)) == 'e') 
          && ((*(pszWrk+8)) == ' ') 
          && ((*(pszWrk+9)) == 'I') 
          && ((*(pszWrk+10)) == 'N') 
          && ((*(pszWrk+11)) == ' ') )
         {
         pszKey = (pszWrk+11);

         pszX = pszInBuffer;
         usCnt = 1;
         while (pszX != (pszWrk+11))
            {
            usCnt++;
            pszX++;
            }
         ItiStrCpy(pszOutBuffer, pszInBuffer, usCnt);
         ItiStrCat(pszOutBuffer, " ( ", usOutSize);

         pszMarker = (pszWrk+11);
         usCnt = 1;
         while ((pszMarker != NULL) && (*pszMarker != ')') )
            {
            usCnt++;
            pszMarker++;
            }
         pszMarker++; /* skip over the closing bracket. */
         (*pszMarker) = '\0'; 
         ItiStrCpy(szKeyList,
            "select distinct CodeValueID from CodeValue where CodeTableKey = 1000024 AND CodeValueKey IN "
            , sizeof szKeyList);
         ItiStrCat(szKeyList, pszKey, sizeof szKeyList);
                                      // usCnt);

         if (!(hqry = ItiDbExecQuery(szKeyList, hhp, 0, 0, 0, &uCols, &uErr)))
            {
            return 13;
            }
         while (ItiDbGetQueryRow(hqry, &ppszTmp, &uErr))
            {
            ItiStrCat(pszOutBuffer, ppszTmp[0], usOutSize);
            ItiStrCat(pszOutBuffer, " ", usOutSize);
            ItiFreeStrArray (hhp, ppszTmp, uCols);
            }
         ItiStrCat(pszOutBuffer, ") ", usOutSize);

         (*pszMarker) = ' '; 
         bW = TRUE;
         break;
         }/* end if */

      pszWrk++;
      } /* end while */
   /* -- end of WorkType section. */
   


   /* -- MajorItemKey section. */
   if (bW)
      {
      pszMI = pszMarker+1;
      pszInBuffer = pszMI;
      }

   bM = FALSE;
   while ( (pszMI != NULL) && ( (*pszMI) != '\0' ) )
      {
      if (   ((*pszMI) == 'M') 
          && ((*(pszMI+1)) == 'a') 
          && ((*(pszMI+2)) == 'j') 
          && ((*(pszMI+3)) == 'o') 
          && ((*(pszMI+4)) == 'r') 
          && ((*(pszMI+5)) == 'I') 
          && ((*(pszMI+6)) == 't') 
          && ((*(pszMI+7)) == 'e') 
          && ((*(pszMI+8)) == 'm') 
          && ((*(pszMI+9)) == 'K') 
          && ((*(pszMI+10)) == 'e') 
          && ((*(pszMI+11)) == 'y') )
         {
         pszKey = (pszMI+12);

         pszX = pszInBuffer;
         usCnt = 1;
         while (pszX != (pszMI+9))
            {
            usCnt++;
            pszX++;
            }

         ItiStrNCat(pszOutBuffer, pszInBuffer, (usCnt - 1), usOutSize);
         ItiStrCat(pszOutBuffer, " IN ( ", usOutSize);

         pszMarker = (pszMI+12);
         usCnt = 1;
         while ((pszMarker != NULL) && (*pszMarker != ')') )
            {
            usCnt++;
            pszMarker++;
            }
         ItiStrCpy(szKeyList,
            "select distinct MajorItemID from MajorItem where MajorItemKey ", sizeof szKeyList);
         ItiStrCat(szKeyList, pszKey, sizeof szKeyList);
                                      // usCnt);

         if (!(hqry = ItiDbExecQuery(szKeyList, hhp, 0, 0, 0, &uCols, &uErr)))
            {
            return 13;
            }
         while (ItiDbGetQueryRow(hqry, &ppszTmp, &uErr))
            {
            ItiStrCat(pszOutBuffer, ppszTmp[0], usOutSize);
            ItiStrCat(pszOutBuffer, " ", usOutSize);
            ItiFreeStrArray (hhp, ppszTmp, uCols);
            }
         ItiStrCat(pszOutBuffer, ") ", usOutSize);

         bM = TRUE;
         break;
         }/* end if */

      pszMI++;
      } /* end while */
   /* -- end of MajorItemKey section. */
   

   return 0;
   }








                                 
