#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include "xDebug.h"

tBoolean xDbg_gbOutput   = FALSE;
FILE*    xDbg_gStream    = NULL;
int      xDbg_gType      = xDebug_Nothing;
char*    xDbg_gsRequest  = NULL;
char xDbg_sStackDesc[xDbg_knMaxDescLength];
char xDbg_sCurNoteDesc[xDbg_knMaxDescLength];



char masStack[xDbg_knMaxStackDepth][xDbg_knMaxDescLength];
int  mCurrentStackDepth = 0;
void (*mSegfaultFunction)(int);

void xDbg_Init ( char* isFileName ) {

  char sFileName[256] = "";

  if( isFileName == NULL ) {
    strcpy( sFileName, ".xdebug" );
  } else {
#ifdef IRIX
    sprintf( sFileName, ".xdebug_%s", isFileName );
#else
    snprintf( sFileName, 256, ".xdebug_%s", isFileName );
#endif
  }

  /* if env variable is defined, check it and set debug type if something is
     requested. do file if nothing is recognizable. otherwise try to do 
     file. */
  xDbg_gsRequest = getenv( "XDEBUG" );                   
  if( NULL == xDbg_gsRequest ) {                         
    xDbg_gType = xDebug_File;                            
  } else {                                              
    if( strcmp( "file", xDbg_gsRequest ) == 0 )
      xDbg_gType = xDebug_File;                          
    else if( strcmp( "stderr", xDbg_gsRequest ) == 0 )   
      xDbg_gType = xDebug_Print;                         
    else if( strcmp( "none", xDbg_gsRequest ) == 0 )     
      xDbg_gType = xDebug_Nothing;                       
    else                                                 
      xDbg_gType = xDebug_Print;                         
  }                                                      
  if( xDebug_File == xDbg_gType ) {                      
    xDbg_gStream = fopen( sFileName, "w" );      
    if( NULL == xDbg_gStream ) {
      fprintf( stdout, "Couldn't create output file %s", sFileName );
      fflush( stdout );
      xDbg_gType= xDebug_Nothing;
    }
  }                                                      
  if( xDebug_Print == xDbg_gType )                       
    xDbg_gStream = stderr;                              
  if( xDebug_Nothing == xDbg_gType ) {                 
    xDbg_gbOutput = FALSE;                            
  } else {                                             
    xDbg_gbOutput = TRUE;                               
  }  

  mCurrentStackDepth = 0;
  mSegfaultFunction = NULL;
  signal( SIGSEGV, xDbg_SegfaultHandler );
}

void xDbg_RegisterSegfaultHandler ( void(*iFunction)(int) ) {

  mSegfaultFunction = iFunction;
}

void xDbg_ShutDown () {

  /* close file if we opened it */
  if( xDebug_File == xDbg_gType                     
      && NULL != xDbg_gStream )                     
    fclose( xDbg_gStream );
}


void xDbg_PrintStatus () {

  fprintf( stderr, "output = %d\n", (int)xDbg_gbOutput );
  fprintf( stderr, "type = %s\n",
     (xDbg_gType == xDebug_Nothing) ? "nothing" :
     (xDbg_gType == xDebug_Print) ? "print" :
     (xDbg_gType == xDebug_File) ? "file" : "" );
  fprintf( stderr, "env var = %s\n", 
     (xDbg_gsRequest != NULL) ? xDbg_gsRequest : "undefined" );
  if( xDbg_gStream == stderr ) 
    fprintf( stderr, "stream = stderr\n" );
  else if( NULL != xDbg_gStream )
    fprintf( stderr, "stream = probably a file\n" );
  else
    fprintf( stderr, "stream = NULL\n" );
}

void xDbg_PushStack ( char* isString ) {

  if( mCurrentStackDepth+1 < xDbg_knMaxStackDepth ) {
    
    strncpy( masStack[mCurrentStackDepth], isString, 
       xDbg_knMaxDescLength );
    
    ++mCurrentStackDepth;
  } else {
    
    DebugPrint( ("Stack limit reached, can't store name.\n" ) );
  }
}

void xDbg_PopStack () {

  if( mCurrentStackDepth-1 >= 0 ) {

    --mCurrentStackDepth;

  } else {

    DebugPrint( ("ERROR: xDbg_PopStack call when stack is empty.\n"
      ) );
  }
}

char* xDbg_GetCurrentFunction () {

  if( mCurrentStackDepth > 0 ) {
    return masStack[mCurrentStackDepth-1];
  } else {
    return "(No current function)";
  }
}

void xDbg_PrintStack () {

  int nCurDesc = 0;

  DebugPrint( ("xDebug stack\n\tLength: %d\n", mCurrentStackDepth ) );
  for( nCurDesc = mCurrentStackDepth-1; nCurDesc >= 0; nCurDesc-- ) {
    DebugPrint( ("\t%02d: %s\n", nCurDesc, masStack[nCurDesc] ) );
  }
}

void xDbg_SegfaultHandler ( int inSignal ) {

  DebugPrint( ("\nSegfault\n%s\n", xDbg_sCurNoteDesc ) );

  xDbg_PrintStack();

  if( NULL != mSegfaultFunction )
    mSegfaultFunction( inSignal );
}

void xDbg_Segfault () {

  char* pBadPtr = 0x0;
  *pBadPtr = 1;
}


void xDbg_Printf ( const char* iFormat, ... ) {

  va_list args;

  if( xDbg_gbOutput ) {

    va_start( args, iFormat );
    vfprintf( xDbg_gStream, iFormat, args );
    va_end( args );
  }

  fflush( xDbg_gStream );
}

void xDbg_SetStackDesc ( const char* iFormat, ... ) {

#ifdef Solaris
  strncpy( xDbg_sStackDesc, iFormat, xDbg_knMaxDescLength );
#else

  va_list args;

  va_start( args, iFormat );
#ifdef IRIX
  vsprintf( xDbg_sStackDesc, iFormat, args );
#else
  vsnprintf( xDbg_sStackDesc, xDbg_knMaxDescLength, iFormat, args );
#endif
  va_end( args );
#endif
}

void xDbg_SetCurrentNote ( const char* iFormat, ... ) {

#ifdef Solaris
  strncpy( xDbg_sCurNoteDesc, iFormat, xDbg_knMaxDescLength );
#else
  va_list args;

  va_start( args, iFormat );
#ifdef IRIX
  vsprintf( xDbg_sCurNoteDesc, iFormat, args );
#else
  vsnprintf( xDbg_sCurNoteDesc, xDbg_knMaxDescLength, iFormat, args );
#endif
  va_end( args );
#endif
}

char* xDbg_GetCurrentNote () {
  
  return xDbg_sCurNoteDesc;
}

