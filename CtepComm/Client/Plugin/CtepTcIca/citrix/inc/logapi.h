/********************************************************************
*
*  LOGAPI.H
*
*  Header file for Logging and Tracing
*
*  Copyright (c) 1990 - 2003 Citrix Systems, Inc. All Rights Reserved.
*
*********************************************************************/

#ifndef __LOGAPI_H__
#define __LOGAPI_H__

#if defined(PLATFORM_PRAGMAS) && defined(MKDBG)
#pragma MKDBG("FIX: engtypes.h included by logapi.h")
#endif

#include <engtypes.h>
#include <ctxdebug.h>

#ifdef __cplusplus
extern "C" {
#endif
	
/* 
 *  Logging and Tracing flag definitions
 */
	
#include "logflags.h"
	
#include <stdarg.h>
	
/* 
 *  Logging structures
 */
	
typedef struct _LOGOPEN {
	ULONG LogFlags;
	ULONG LogClass;
	ULONG LogEnable;
      ULONG LogBufferSize;
	FILEPATH LogFile;
} LOGOPEN, FAR *PLOGOPEN;
	
	
	
/* 
 *  Logging functions
 */
	
	/*
	 * Note: These function typedefs must be maintained in sync with the
	 *       above function prototypes
	 */
	/* 
	 *   Typedef Functions and Call Table
	 */
	
typedef int  (PWFCAPI PFNLOGOPEN)( PLOGOPEN pLogOpen );
typedef int  (PWFCAPI PLOGCLOSE)( void );
typedef void (PWFCAPI PLOGPRINTF)( ULONG LogClass, ULONG LogEnable, PCHAR pFormat, ... );
typedef void (PWFCAPI PLOGBUFFER)( ULONG LogClass, ULONG LogEnable,
		      LPVOID pBuffer, ULONG ByteCount );
#ifdef __GNUC__
typedef void (PWFCAPI PLOGASSERT)( PCHAR, PCHAR, int, PCHAR, int );
#else
typedef void (PWFCAPI PLOGASSERT)( PCHAR, PCHAR, int, int );
#endif
typedef struct _LOGTABLE
	{
		PFNLOGOPEN  pLogOpen;
		PLOGCLOSE   pLogClose;
		PLOGPRINTF  pLogPrintf;
		PLOGBUFFER  pLogBuffer;
		PLOGASSERT  pLogAssert;
	}   LOGTABLE,FAR *PLOGTABLE,FAR * FAR *PPLOGTABLE;
	
	/* 
	 *   External functions called by config library
	 */
	
int WFCAPI LogLoad( PPLOGTABLE );
#ifdef __GNUC__
int WFCAPI logWhere( char *, int, char * );
#else
int WFCAPI logWhere( char *, int );
#endif
void WFCAPI logPrintf( ULONG LogClass, ULONG LogEnable, PCHAR pFormat, va_list arg_marker);
	
#define LOG_STRING "CTXLOG"
	
#ifdef USE_LOG_CALL_TABLE
#if defined(PLATFORM_PRAGMAS) && defined(MKVERB)
#pragma MKVERB (__FILE__":    LOG_CALL_TABLE defined")
#endif
	
#define LOG_GLOBAL_INIT PLOGTABLE g_pLogCallTable = NULL
#define LOG_EXTERN_INIT extern PLOGTABLE g_pLogCallTable
	
#define LogRunTimeInit(pLibMgr, pVer) \
    ((*pLibMgr->pLMgrUse)(LOG_STRING,pVer,(PPLMGRTABLE)&g_pLogCallTable))
#define LogRunTimeRelease(pLibMgr) \
    ((*pLibMgr->pLMgrRelease)((PLMGTABLE)g_pLogCallTable))
#define LogSetCallTable(pLogCallTable) \
    (g_pLogCallTable = (PLOGTABLE)pLogCallTable)
	
	LOG_EXTERN_INIT;
	
#define LogOpen     (*(g_pLogCallTable)->pLogOpen)
#define LogClose    (*(g_pLogCallTable)->pLogClose)
#define LogPrintf   if (g_pLogCallTable) (*(g_pLogCallTable)->pLogPrintf)
#define LogBuffer   if (g_pLogCallTable) (*(g_pLogCallTable)->pLogBuffer)
#define LogAssert   if (g_pLogCallTable) (*(g_pLogCallTable)->pLogAssert)
	
#else /* !USE_LOG_CALL_TABLE */
	
	/*
	 * Note: These function prototypes must be maintained in sync with the
	 *       function typedefs below
	 */
int  WFCAPI LogOpen( PLOGOPEN pLogOpen );
int  WFCAPI LogClose( void );
void WFCAPI LogPrintf( ULONG LogClass, ULONG LogEnable, PCHAR pFormat, ... );
void WFCAPI LogBuffer( ULONG LogClass, ULONG LogEnable,
		               LPVOID pBuffer, ULONG ByteCount );
#ifdef __GNUC__
void WFCAPI LogAssert( PCHAR, PCHAR, int, PCHAR, int );
#else
void WFCAPI LogAssert( PCHAR, PCHAR, int, int );
#endif
	
#define LOG_GLOBAL_INIT
#define LOG_EXTERN_INIT
#define LogRunTimeInit(pLibMgr, pVer)
#define LogRunTimeRelease(pLibMgr)
#define LogSetCallTable(pLogCallTable)
	
#endif /* !USE_LOG_CALL_TABLE */
	
#ifdef __cplusplus
}
#endif

#endif /* __LOGAPI_H__ */