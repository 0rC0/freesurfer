#ifndef xDebug_H
#define xDebug_H

#include <stdio.h>
#include "xTypes.h"

#ifdef XDEBUG_NO_CODE
#define kDebugging          0
#else
#define kDebugging          1 
#endif

#define xDebug_Nothing 0
#define xDebug_Print   1
#define xDebug_File    2

/* prototypes */
void xDbg_Init ( char* isFileName );
void xDbg_ShutDown ();
void xDbg_PrintStatus ();
void xDbg_RegisterSegfaultHandler ( void(*f)(int) );
void xDbg_PushStack ( char* isTitle, char* isNote );
void xDbg_PopStack ();
char* xDbg_GetCurrentFunction ();
void xDbg_PrintStack ();
void xDbg_SegfaultHandler ( int );
void xDbg_Segfault ();
void xDbg_Printf ( const char* iFormat, ... );
void xDbg_SetStackDesc ( const char* iFormat, ... );
void xDbg_SetCurrentNote ( const char* iFormat, ... );
char* xDbg_GetCurrentNote ();

/* typedefs */
typedef tBoolean xDbg_tDebuggingState;

/* constants */
#define xDbg_knMaxStackDepth 50
#define xDbg_knMaxDescLength 512

/* vars, included here because we refrence them in the macros, so they
   need to be global */
extern xDbg_tDebuggingState xDbg_gbOutput;
extern FILE*                xDbg_gStream;
extern int                  xDbg_gType;
extern char*                xDbg_gsRequest;
extern char                 xDbg_sStackDesc[xDbg_knMaxDescLength];
extern char                 xDbg_sCurNoteDesc[xDbg_knMaxDescLength];
extern int                  xDbg_gLineNumberOfError;

/* if we're generating debugging code... */
#if kDebugging

/* start up and take down debugging. the argument to Init should be the
   program name. */
#define InitDebugging(sProgName) xDbg_Init(sProgName)
#define DeleteDebugging          xDbg_ShutDown()

/* register a handler to get called at a segfault */   
#define DebugRegisterSegfaultHandler(s) xDbg_RegisterSegfaultHandler(s)

/* control whether or not output is sent to the chosen target. */
#define DisableDebuggingOutput   xDbg_gbOutput = FALSE
#define EnableDebuggingOutput    if ( xDbg_gStream != xDebug_Nothing) \
                                      xDbg_gbOutput = TRUE

/* to be used to get the current state and restore it later so it can be 
   explicitly enabled or disabled in a function */
#define GetDebuggingState(x)     *x = xDbg_gbOutput
#define SetDebuggingState(x)     xDbg_gbOutput = x

/* to bed used as a test */
#define IsDebugging              (xDbg_gbOutput == TRUE)

#define DebugCode
#define EndDebugCode

/* print a message. args should be a format string and then the parameters,
   enclosed in parens. i.e.
           DebugPrint( ("%02d: Hello %s\n", 1, "world") ); */
#define DebugPrint(ARGS)         xDbg_Printf ARGS

#define Here(n)                  DebugPrint( ("--> here %d\n", n) )

/* sets the current note. it will not be output unless the program crasshes.
   use it to store the current 'state' of the program, so you can see what
   it was doing when it crahses. */
#define DebugNote(ARGS)          xDbg_SetCurrentNote ARGS

/* returns pointers to the current function and current note */
#define DebugGetNote             xDbg_GetCurrentNote()
#define DebugGetFunction         xDbg_GetCurrentFunction()

/* prints the stack */
#define DebugPrintStack          xDbg_PrintStack()

/* use at the beginning and ending of a function. the string will be written
   to a stack which will be printed if the program crashes. use the exit
   macro at the end of a function to pop the last string off the stack. */
#define DebugEnterFunction(ARGS) \
                   xDbg_PushStack( xDbg_sStackDesc, xDbg_sCurNoteDesc ); \
                   xDbg_SetStackDesc ARGS ; \
                   xDbg_SetCurrentNote((""));

#define DebugExitFunction        xDbg_PopStack()

/* use these only if you are going to catch them with the DebugCatch macros
   below. the first just takes a test. the second will set var to errorCode
   if the test is true. the third throws without a test. */
#define DebugAssertThrow(test) \
                                 do { \
                                    if( !(test) ) { \
                                        xDbg_gLineNumberOfError = __LINE__; \
                                        goto error; \
                                    } \
                                 } while(0)

#define DebugAssertThrowX(test,var,errorCode) \
                                 do { \
                                    if( !(test) ) { \
                                        xDbg_gLineNumberOfError = __LINE__; \
                                        var = errorCode; \
                                        goto error; \
                                    } \
                                 } while(0)

#define DebugThrowX(var,errorCode) \
                                 do { \
                                    xDbg_gLineNumberOfError = __LINE__; \
                                    var = errorCode; \
                                    goto error; \
                                 } while(0)

/* Throw without going through the error reporting code. */
#define DebugAssertQuietThrow(test) \
                                 do { \
                                    if( !(test) ) { \
                                        goto cleanup; \
                                    } \
                                 } while(0)

/* start and end the 'catch block' */
#define DebugCatch               goto cleanup; \
                                 error: \
                                 do {} while(0)
#define EndDebugCatch            cleanup: \
                                 do {} while(0)

/* if errorCode != kNoError, the function will be called, passing the
   errorCode. it should return a char* string with the error message. */
#define DebugCatchError(errorCode,kNoError,errorStringFunc) \
     do { \
     DebugPrint( ("Error in %s (line %d)\n\twhile %s\n", \
                  xDbg_GetCurrentFunction(), xDbg_gLineNumberOfError, \
                  xDbg_sCurNoteDesc) ); \
     if( (errorCode) != (kNoError) ) { \
        if( NULL != errorStringFunc ) { \
           DebugPrint( ("\tError %d: %s\n", \
                        errorCode, errorStringFunc(errorCode)) ); \
        } else { \
           DebugPrint( ("\tError %d\n", errorCode) ); \
        } \
     } else { \
        DebugPrint( ("\tNo error code.\n") ); \
     } \
     } while(0) 

#define DebugGotoCleanup         goto cleanup

#else

       /* define all macros so they do nothing */
#define InitDebugging(s)   
#define DeleteDebugging
#define DebugRegisterSegfaultHandler(s)
#define DisableDebuggingOutput  
#define EnableDebuggingOutput
#define DebugCode
#define EndDebugCode
#define DebugPrint(ARGS)
#define Here(n)
#define GetDebuggingState(x)
#define SetDebuggingState(x)
#define IsDebugging
#define DebugNote(ARGS)
#define DebugEnterFunction(ARGS)
#define DebugExitFunction    
#define DebugAssertThrow(test)
#define DebugAssertThrowX(test,var,errorCode) 
#define DebugCatchError(errorCode,kNoError,errorStringFunc)

       /* have to define them here such that the cleanup label is here
    but that nothing else happens. */
#define DebugCatch               goto cleanup
#define EndDebugCatch            cleanup: \
                                 do {} while(0)
#define DebugGotoCleanup         goto cleanup

#endif

#endif
