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
 * dbparse.c                                                            *
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
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itidbase.h"
#include "..\INCLUDE\colid.h"
#include "..\INCLUDE\itierror.h"
#include "..\INCLUDE\itiglob.h"
#include <string.h>
#include "dbparse.h"
#include "dbutil.h"
#include "dbase.h"
#include <stdlib.h>

#define WORDSIZE  80

static CHAR szMetric[]    = " 1 ";
static CHAR szEnglish[]   = " 0 ";
static CHAR szDefSpecYr[] = " 1900 "; /* default spec year */

/*
 * FindUnquotedWord finds the first occurance of pszWord in the string
 * pszString that is not contained in single or double quotes.  
 * If bSkipParens is TRUE, then any occurance of pszWord contained in
 * parenthesis is ignored.
 *
 * If the word cannot be found, then NULL is returned, otherwise a
 * pointer to the word in the string is returned.
 */

static PSZ FindUnquotedWord (PSZ pszWord, PSZ pszString, BOOL bSkipParens)
   {
   USHORT usParens, usQuotes, usDoubleQuotes;
   USHORT usWordLen;

   usWordLen = ItiStrLen (pszWord);
   usParens = 0;
   usQuotes = 0;
   usDoubleQuotes = 0;
   for (;*pszString; pszString++)
      {
      if (bSkipParens && *pszString == '(')
         {
         if (usQuotes == 0 && usDoubleQuotes == 0)
            usParens++;
         }
      else if (bSkipParens && *pszString == ')')
         {
         if (usQuotes == 0 && usDoubleQuotes == 0)
            usParens--;
         }
      else if (*pszString == '\'')
         {
         if (usQuotes > 0)
            if (*(pszString + 1) == '\'')
               pszString++;
            else
               usQuotes--;
         else if (usDoubleQuotes == 0)
            usQuotes++;
         }
      else if (*pszString == '"')
         {
         if (usDoubleQuotes > 0)
            if (*(pszString + 1) == '\"')
               pszString++;
            else
               usDoubleQuotes--;
         else if (usQuotes == 0)
            usDoubleQuotes++;
         }
      else if (ItiCharLower (*pszString) == ItiCharLower (*pszWord))
         {
         if (usParens == 0 && usQuotes == 0 && usDoubleQuotes == 0)
            if (ItiStrNICmp (pszString, pszWord, usWordLen) == 0)
               return pszString;
         }
      }
   return NULL;
   }




/*
 * This function returns a pointer to the FROM clause of an SQL statement.
 * Note that any "FROM"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

PSZ EXPORT ItiDbFindFromClause (PSZ pszQuery)
   {
   return FindUnquotedWord ("from", pszQuery, TRUE);
   }


/*
 * This function returns a pointer to the SELECT clause of an SQL statement.
 * Note that any "SELECTS"s in quotes, or double quotes are ignored.
 */

PSZ EXPORT ItiDbFindSelectClause (PSZ pszQuery)
   {
   return FindUnquotedWord ("select", pszQuery, FALSE);
   }


/*
 * This function returns a pointer to the WHERE clause of an SQL statement.
 * Note that any "WHERE"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

PSZ EXPORT ItiDbFindWhereClause (PSZ pszQuery)
   {
   return FindUnquotedWord ("where", pszQuery, TRUE);
   }



/*
 * FindByClause returns a pointer to a phrase that starts with the
 * word pszWord, and contains the word "BY" after it.
 */

static PSZ FindByClause (PSZ pszQuery, PSZ pszWord)
   {
   PSZ pszTemp, pszTemp2;

   pszTemp = pszQuery;
   while (pszTemp != NULL)
      {
      pszTemp = FindUnquotedWord (pszWord, pszTemp, TRUE);
      if (pszTemp == NULL)
         return NULL;

      pszTemp2 = pszTemp + ItiStrLen (pszWord); /* skip past the word */
      while (ItiCharIsSpace (*pszTemp2))
         pszTemp2++;
      if (ItiStrNICmp ("by", pszTemp2, 2) == 0)
         break;
      }
   return pszTemp;
   }


/*
 * This function returns a pointer to the ORDER BY clause of an SQL statement.
 * Note that any "ORDER BY"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

PSZ EXPORT ItiDbFindOrderByClause (PSZ pszQuery)
   {
   return FindByClause (pszQuery, "order");
   }



/*
 * This function returns a pointer to the GROUP BY clause of an SQL statement.
 * Note that any "GROUP BY"s in quotes, double quotes, or parenthesis are 
 * ignored.
 */

PSZ EXPORT ItiDbFindGroupByClause (PSZ pszQuery)
   {
   return FindByClause (pszQuery, "group");
   }





/*
 * DbGetCountQuery creates a SELECT COUNT (*) query based on the 
 * SQL statement pszQuery.  pszQuery must be either a SELECT 
 * or a DELETE FROM SQL statement.  If the word DISTINCT comes
 * after SELECT, then the DISTINCT is also placed in the count 
 * query.
 * 
 * Currently, ORDER BYs and GROUP BYs are not stripped from the
 * query.
 *
 * The query is allocated from heap hheap, and it is the caller's 
 * responsibility to free the query.
 */

PSZ DbGetCountQuery (HHEAP hheap, PSZ pszQuery)
   {
   PSZ    pszFromClause;
   PSZ    pszCountQuery;
   USHORT usCountQueryLen;
   PSZ    pszSelect, pszTemp;
   BOOL   bDistinct;

   pszFromClause = ItiDbFindFromClause (pszQuery);

   pszSelect = ItiDbFindSelectClause (pszQuery);
   if (pszSelect != NULL && pszSelect < pszFromClause)
      {
      pszTemp = pszSelect + 6; /* skip past the word 'select' */
      while (ItiCharIsSpace (*pszTemp))
         pszTemp++;
      bDistinct = ItiStrNICmp (pszTemp, "distinct", 8) == 0;
      }

   if (pszFromClause == NULL)
      return NULL;

   usCountQueryLen = ItiStrLen (pszFromClause) + 
                     7 +                   /* "SELECT " */
                     (bDistinct ? 9 : 0) + /* "DISTINCT " */
                     10 +                  /* "COUNT (*) " */
                     1;                    /* null terminator */

   pszCountQuery = ItiMemAlloc (hheap, usCountQueryLen, 0);
   if (pszCountQuery == NULL)
      return NULL;

   ItiStrCpy (pszCountQuery, "SELECT ", usCountQueryLen);
   if (bDistinct)
      ItiStrCat (pszCountQuery, "DISTINCT ", usCountQueryLen);
   ItiStrCat (pszCountQuery, "COUNT (*) ", usCountQueryLen);
   ItiStrCat (pszCountQuery, pszFromClause, usCountQueryLen);

   /* we should have stripped any ORDER BY clause, but I'm lazy */
   return pszCountQuery;
   }



/*
 *    This procedure replaces the %colid 
 *  with the value in that column in row uRow
 *  this is for child querys and child window
 *  titles
 *    hbufParent is checked for the column first.
 *  If it does not contain the needed col. hbufStatic
 *  is used.
 *
 *    if hbufParent is actually -1, this is a flag
 *  saying that the buffers are not ready yet. In this
 *  case hbufStatic contains a string to replace
 *
 *   pszStr ... query string to fix
 *   hbuf   ... buffer with replacement source
 *   uRow   ... row in hbuf to use
 *
 *   returns 0 if OK
 */
BOOL EXPORT ItiDbReplaceParams (PSZ    pszDest,
                                PSZ    pszSrc,
                                HBUF   hbufParent,
                                HBUF   hbufStatic,
                                USHORT uBuffParentRow,
                                USHORT uMaxSize)
   {
   char     szColName [128];
   char     szColVal  [255];
   char     cDelim;
   PSZ      pszSrcCpy;
   USHORT   j, k, uRow, uColId, uState;
   HBUF     hbuf;
   BOOL     bParent, bStatic, bReturn = 0;
   PSZ      pszTmpSrc;
//   PGLOBALS pglobDB;

   j = 0;
   *pszDest = '\0';
   pszSrcCpy = pszSrc;

   pszTmpSrc = pszSrc;
   ItiDbReplacePGlobParams (pszDest, pszTmpSrc, uMaxSize);
   ItiStrCpy (pszSrc, pszDest, uMaxSize);

//   pglobDB = ItiGlobQueryGlobalsPointer ();

   while (TRUE)
      {
      if (*pszSrcCpy == '\0' || j > uMaxSize)
         break;
      if (*pszSrcCpy != '%')
         pszDest [j++] = *(pszSrcCpy++);
      else if (pszSrcCpy[1] == '%')
         {
         pszSrcCpy++;
         pszDest [j++] = *(pszSrcCpy++);
         }
      else
         {
         pszSrcCpy++;

         cDelim = (char) ItiGetWord (&pszSrcCpy, szColName, " ,\n\t'\"@%()", TRUE);

         /* -- ---------------------------- */
      //   /* -- Added 95 for metrication.  Put in this session's UserKey. */
      //   if (0 == ItiStrCmp (szColName, "CurUserKey") )
      //      {
      //      k = 0;
      //      while ((pszDest [j++] = pglobDB->pszUserKey [k++]) != '\0')
      //         ;
      //      pszDest [j-1] = cDelim;
      //      pszDest [j] = '\0';
      //      continue;
      //      }
      //
      //   if (0 == ItiStrCmp (szColName, "CurUnitType") )
      //      {
      //      k = 0;
      //      if (pglobDB->bUseMetric)
      //         while ((pszDest [j++] = szMetric[k++]) != '\0')
      //            ;
      //      else
      //         while ((pszDest [j++] = szEnglish[k++]) != '\0')
      //            ;
      //
      //      pszDest [j-1] = cDelim;
      //      pszDest [j] = '\0';
      //      continue;
      //      }
      //
      //   if (0 == ItiStrCmp (szColName, "CurSpecYear") )
      //      {
      //      k = 0;
      //      if ((NULL != pglobDB) && (NULL != pglobDB->pszCurrentSpecYear))
      //         {
      //         while ((pszDest [j++] = pglobDB->pszCurrentSpecYear[k++]) != '\0')
      //            ;
      //         }
      //      else
      //         {
      //         }
      //
      //      pszDest [j-1] = cDelim;
      //      pszDest [j] = '\0';
      //      continue;
      //      }
      //
         /* -- ---------------------------- */

         if ((uColId = ItiColStringToColId (szColName)) == 0xFFFF)
            {
            pszDest [j++] = '-';
            pszDest [j++] = '1';
            pszDest [j++] = cDelim;
            pszDest [j] = '\0';

            DbSUSErr ("Invalid Column Name Requested ", uColId, szColName);
            bReturn = (bReturn ? bReturn : ITIERR_BADCOL);
            continue;
            }

         /*--- Special case of hbuf1 == -1 then hbuf2 == psz ---*/
         if (hbufParent == (HBUF)-1L)
            {
            k = 0;
            while ((pszDest [j++] = ((PSZ)hbufStatic)[k++]) != '\0')
               ;
            pszDest [j-1] = cDelim;
            pszDest [j] = '\0';
            continue;
            }

//         ItiDbWaitOnBuffer (hbufParent);
//         ItiDbWaitOnBuffer (hbufStatic);

         bParent = ItiDbColExists (hbufParent, uColId) &&
                                   uBuffParentRow != 0xFFFF;
         bStatic = ItiDbColExists (hbufStatic, uColId);


         /*--- case of nobody having the requested key ---*/
         if (!bParent && !bStatic)
            {
            pszDest [j++] = '-';
            pszDest [j++] = '1';
            pszDest [j++] = cDelim;
            pszDest [j] = '\0';
//
// no msg because of dialog add mode
//            DbSUSErr ("Parent Does not have requested Column", uColId, szColName);

            bReturn = (bReturn ? bReturn : ITIERR_NOCOL);
            continue;
            }

         /*--- normal case of someone having key---*/
         hbuf = (bParent ? hbufParent : hbufStatic);
         uRow = (bParent ? uBuffParentRow : 0);

         do /*--- busy wait for row to become available ---*/
            {
            uState = ItiDbGetBufferRowCol (hbuf, uRow, uColId, szColVal);
            } while (uState == ITIDB_WAIT || uState == ITIDB_GRAY);

         if (uState != ITIDB_VALID)
            {
            DbSUSErr ("unable to get replacable parameter uColId:", uColId, ItiColColIdToString (uColId));
            bReturn =  (bReturn ? bReturn : ITIERR_BADBUFF);
            }

         k = 0;
         while ((pszDest [j++] = szColVal [k++]) != '\0')
            ;
         pszDest [j-1] = cDelim;
         pszDest [j] = '\0';
         }
      }
   pszDest [j] = '\0';
   return bReturn;
   }




/*
 * ItiGetTableFromInsert returns the table that is being inserted or updates
 * into from the given SQL statment.  This function works with the following 
 * SQL statements: INSERT, UPDATE, DELETE.
 * The table name is copied into pszTable.
 */

BOOL EXPORT ItiDbGetTableFromInsert (PSZ    pszInsert,
                                     PSZ    pszTable,
                                     USHORT usTableMax)
   {
   PSZ      pszWord;
   BOOL     bFound;
   PSZ      pszTemp;
   PSZ      pszTemp2;
   USHORT   usSize;

   *pszTable = '\0';

   /* make a copy of the insert string, since ItiStrTok clobbers the
      input string */
   usSize = ItiStrLen (pszInsert) + 1;
   pszTemp = ItiMemAllocSeg (usSize, 0, 0);
   if (pszTemp == NULL)
      return FALSE;
   ItiStrCpy (pszTemp, pszInsert, usSize);

   bFound = FALSE;
   pszWord = ItiStrTok (pszTemp, " \t\n\r(");
   while (pszWord != NULL && !bFound)
      {
      if (0 == ItiStrICmp (pszWord, "insert") ||
          0 == ItiStrICmp (pszWord, "update") ||
          0 == ItiStrICmp (pszWord, "delete"))
         {
         pszWord = ItiStrTok (NULL, " \t\n\r(");
         if (0 == ItiStrICmp (pszWord, "into") ||
             0 == ItiStrICmp (pszWord, "from"))
            pszWord = ItiStrTok (NULL, " \t\n\r(");
         bFound = TRUE;
         }
      else
         {
         pszWord = ItiStrTok (NULL, " \t\n\r(");
         }
      }
   if (bFound)
      {
      /* strip off server, database, and owner names */
      if ((pszTemp2 = ItiStrRChr (pszWord, '.')) != NULL)
         pszWord = pszTemp2 + 1;

      ItiStrCpy (pszTable, pszWord, usTableMax);
      }

   ItiMemFreeSeg (pszTemp);
   return bFound;
   }









/*  this has a hheap param because it is may be used for both buffers and
 *  non buffer dbase uses
 *  if this fn is called for buffers, hheap should be called using hheapDB3
 *
 */
BOOL DbTablesFromQuery (PSZ    pszQ,
                        HHEAP  hheap,
                        PSZ    **pppszTables,
                        USHORT *pusCount)
   {
   PSZ      psz, pszTmp, pszQuery;
   USHORT   uTables = 0;
   char     szWord [WORDSIZE];
   char     cDelim;
   USHORT   usParens;

   *pppszTables = NULL;
   *pusCount = 0;
   if (pszQ == NULL || *pszQ == '\0')
      return FALSE;

   psz = pszQuery = ItiStrDup (hheapDB0, pszQ);

//   while (ItiEat(&pszQuery, "from") == 1)
//      ;

   usParens = 0;
   while (TRUE)
      {
      cDelim = (char) ItiGetWord (&pszQuery, szWord, " ,\t\n()", TRUE);
      if (cDelim == -1 || cDelim == '\0')
         break;
      switch (cDelim)
         {
         case '(':
            usParens++;
            break;

         case ')':
            if (usParens > 0)
               usParens--;
            break;
         }
      if (usParens == 0 && stricmp (szWord, "from") == 0)
         break;
      }

   while (TRUE)
      {
      cDelim = (char) ItiGetWord (&pszQuery, szWord, " ,\t\n", TRUE);
      if (cDelim == (char) 0xFF        || 
          !stricmp (szWord, "where")   ||
          !stricmp (szWord, "group")   ||
          !stricmp (szWord, "order")   ||
          !stricmp (szWord, "compute") ||
          !stricmp (szWord, "for"))
         break;

      /*--- skip past optional owner & dbase names ---*/
      if ((pszTmp = strrchr (szWord, '.')) == NULL)
         pszTmp = szWord;
      else
         pszTmp++;

      *pusCount += 1;
      *pppszTables = (PSZ *) ItiMemRealloc (hheap, *pppszTables,
                                            *pusCount * sizeof (PSZ), 0);
      if (*pppszTables == NULL)
         /* bummer */
         {
         ItiMemFree (hheapDB0, psz);
         *pusCount = 0;
         return FALSE;
         }

      (*pppszTables)[*pusCount-1] = ItiStrDup (hheap, pszTmp);
   
      /*--- skip optional alias or holdlock ---*/
      if (cDelim != ',')
         while (*pszQuery != ',' && *pszQuery != '\0')
            pszQuery++;
      }
   ItiMemFree (hheapDB0, psz);
   return TRUE;
   }




/* ItiDbReplacePGlobParams only replaces the   */
/* following params with values from pglobals. */
/*       %CurUserKey                           */
/*       %CurUnitType                          */
/*       %CurSpecYear                          */
/*                                             */     
BOOL EXPORT ItiDbReplacePGlobParams (PSZ    pszDest,
                                     PSZ    pszSrc,
                                     USHORT uMaxSize)
   {
   char     szColName [128];
   char     szColVal  [255];
   char     cDelim;
   PSZ      pszSrcCpy;
   USHORT   j, k;
   BOOL     bReturn = 1;
   PGLOBALS pglobDBLoc;

   j = 0;
   *pszDest = '\0';
   pszSrcCpy = pszSrc;

   pglobDBLoc = ItiGlobQueryGlobalsPointer ();

   while (TRUE)
      {
      if (*pszSrcCpy == '\0' || j > uMaxSize)
         break;
      if (*pszSrcCpy != '%')
         pszDest [j++] = *(pszSrcCpy++);
      else if ((pszSrcCpy[0] == '%')
               && !(   (pszSrcCpy[1] == 'C')
                    && (pszSrcCpy[2] == 'u')
                    && (pszSrcCpy[3] == 'r')
                   )
              )
         {  /* Skip the ID that does NOT start with '%Cur' */
         pszDest [j++] = *(pszSrcCpy++);
         continue;
         }
      else if (pszSrcCpy[1] == '%')   /* case of %%Xxxx */
         {
         pszSrcCpy++;
         pszDest [j++] = *(pszSrcCpy++);
         }
      else /* clause */
         {
         pszSrcCpy++;

         cDelim = (char) ItiGetWord (&pszSrcCpy, szColName, " ,\n\t'\"@%()", TRUE);

         /* -- Added 95 for metrication.  Put in this session's UserKey. */
         if (0 == ItiStrCmp (szColName, "CurUserKey") )
            {
            k = 0;
            while ((pszDest [j++] = pglobDBLoc->pszUserKey [k++]) != '\0')
               ;
            pszDest [j-1] = cDelim;
            pszDest [j] = '\0';
            continue;
            }

         if (0 == ItiStrCmp (szColName, "CurUnitType") )
            {
            k = 0;
            if (pglobDBLoc->bUseMetric)
               while ((pszDest [j++] = szMetric[k++]) != '\0')
                  ;
            else
               while ((pszDest [j++] = szEnglish[k++]) != '\0')
                  ;

            pszDest [j-1] = cDelim;
            pszDest [j] = '\0';
            continue;
            }

         if (0 == ItiStrCmp (szColName, "CurSpecYear") )
            {
            k = 0;
            if ((NULL != pglobDBLoc) && (NULL != pglobDBLoc->pszCurrentSpecYear))
               {
               while ((pszDest [j++] = pglobDBLoc->pszCurrentSpecYear[k++]) != '\0')
                  ;
               }
            else  /* use 1900 as a substitute for the missing SpecYear. */
               {
               while ((pszDest [j++] = szDefSpecYr[k++]) != '\0')
                  ;
               }

            pszDest [j-1] = cDelim;
            pszDest [j] = '\0';
            continue;
            }

         /* -- ---------------------------- */

         // k = 0;
         // while ((pszDest [j++] = szColVal [k++]) != '\0')
         //    ;
         // pszDest [j-1] = cDelim;
         // pszDest [j] = '\0';

         } /* end of else clause */

      } /* end while true */

   pszDest [j] = '\0';
   return bReturn;
   } /* End of Function */
