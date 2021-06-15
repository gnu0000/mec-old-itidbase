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
 * dbbcp.c                                                              *
 *                                                                      *
 * Copyright (C) 1990-1992 Info Tech, Inc.                              *
 * Mark Hampton                                                         *
 *                                                                      *
 * This file provides the batch bulk load API.                          *
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
#include "..\INCLUDE\itibase.h"
#include "..\INCLUDE\itiutil.h"
#include "..\INCLUDE\itierror.h"
#define DBMSOS2
#include <sqlfront.h>
#include <sqldb.h>
#include <stdio.h>
#define DATABASE_INTERNAL
#include "..\INCLUDE\itidbase.h"
#include "dbkernel.h"


static HDB hdbBcp;

/* TRUE if OK, FALSE if error */
BOOL EXPORT ItiBcpInit (void)
   {
   HLOGIN   hlogin;
   PSZ      pszServerName, pszAppName, pszHostName,
            pszUserName, pszPassword, pszDatabase;

   if (DosScanEnv ("SERVER", &pszServerName))
      pszServerName = "";

   if (DosScanEnv ("APPNAME", &pszAppName))
      pszAppName = "DS/Shell client";

   if (DosScanEnv ("HOSTNAME", &pszHostName))
      pszHostName = "";

   if (DosScanEnv ("USERNAME", &pszUserName))
      pszUserName = "sa";

   if (DosScanEnv ("PASSWORD", &pszPassword))
      pszPassword = "";

   if (DosScanEnv ("IMPORTDATABASE", &pszDatabase))
      pszDatabase = "DSShellImport";

   hlogin = DbLogin (pszAppName, pszHostName, pszUserName, pszPassword, TRUE);

   /* -- init bcp connection -- */
   hdbBcp = DbOpen (hlogin, pszServerName, pszDatabase);
   if (hdbBcp == NULL)
      {
      return FALSE;
      }
#if defined (DEBUG)
   else
      {
      char szTemp [256];

      sprintf (szTemp, "DBPROCESS (HDB) for BCP connection is %p", hdbBcp);
      ItiErrWriteDebugMessage (szTemp);
      }
#endif

   /* free up login structure -- mdh */
   DbFreeLogin (hlogin);

   return TRUE;
   }



/* TRUE if OK, FALSE if error */
BOOL EXPORT ItiBcpSetTable (PSZ pszTableName)
   {
   return BcpSetTable (hdbBcp, pszTableName);
   }



/*
 * ItiBcpBindColumn binds a column to a table.  ItiBcpSetTable must
 * be called first.
 *
 * Parameter: usColumnOrder         The order of the column in the
 *                                  database.
 *
 *            pszData               The location in memory of the data.
 *                                  The data is assumed to be in a null
 *                                  terminated string
 *
 */

BOOL EXPORT ItiBcpBindColumn (USHORT usColumnOrder, PSZ pszData)
   {
   return BcpBindColumn (hdbBcp, 
                         pszData, 
                         0, 
                         (pszData == NULL) ? 0 : -1, 
                         "", 
                         sizeof (char),
                         usColumnOrder);
   }


BOOL EXPORT ItiBcpSendRow (void)
   {
   return BcpSendRow (hdbBcp);
   }

ULONG EXPORT ItiBcpTableDone (void)
   {
   return BcpTableDone (hdbBcp);
   }

ULONG EXPORT ItiBcpBatch (void)
   {
   return BcpBatch (hdbBcp);
   }

