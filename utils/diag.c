/*
 *       FILE NAME:   diag.c
 *
 *       DESCRIPTION: diagnostic routines
 *
 *       AUTHOR:      Bruce Fischl
 *       DATE:        2/5/96
 *
*/

/*-----------------------------------------------------
                    INCLUDE FILES
-------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>

#include "image.h"
#include "diag.h"
#include "windiag.h"
#include "proto.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/

/*-----------------------------------------------------
                    GLOBAL VARIABLES
-------------------------------------------------------*/

unsigned long  Gdiag = 0 ;

/*-----------------------------------------------------
                     STATIC DATA
-------------------------------------------------------*/

static char diag_fname[100] = "diag.log" ;
static int (*diag_vprintf)(const char *fmt, va_list args) = vprintf ;
static int (*diag_vfprintf)(FILE *fp, const char *fmt, va_list args) =vfprintf;

/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/

/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        diag bits.

------------------------------------------------------------------------*/
unsigned long
DiagInit(char *fname, 
                  int (*vfprint)(FILE *fp, const char *fmt, va_list args),
                  int (*vprint)(const char *fmt, va_list args))
{
  char *cp ;

  if (fname)
    strcpy(diag_fname, fname) ;
  if (vfprint)
    diag_vfprintf = vfprint ;
  if (vprint)
    diag_vprintf = vprint ;

  cp = getenv("diag") ;

  if (!cp) cp = getenv("DIAG") ;
  if (cp) 
  {
    sscanf(cp, "0x%lx", &Gdiag) ;
    fprintf(stderr, "diagnostics set to 0x%lx\n", Gdiag) ;
  }

#if 0
  if (getenv("logging") != NULL)
    DebugOpenLogFile("trace.log") ;
  
  if (getenv("flushing") != NULL)
    flushing = 1 ;
#endif
  return(Gdiag) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
DiagShowImage(unsigned long diag_bits, int win, int which, IMAGE *I, 
              char *fmt, ...)
{
#if 0
  char    name[100] ;
  va_list args ;
  
  if (diag_bits && !(diag_bits & Gdiag))
    return(-1) ;

  va_start(args, fmt) ;
/*  fmt = va_arg(args, char *) ;*/
  vsprintf(name, fmt, args) ;

  if (win == DIAG_NEW_WINDOW)    /* create a new window for this image */
  {
  }

  WinShowImage(win, I, which) ;
  WinSetName(win, which, name) ;
  if (diag_bits & DIAG_WAIT)     /* wait for a keystroke before continuing */
    fgetc(stdin) ;    
#endif
  return(win) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
DiagDrawBox(unsigned long diag_bits, int win, int row0, int col, 
                 int rows, int cols, int color)
{
   int i;
   i=win; i=row0; i=col; i=rows; i=cols; i=color; /* prevent warning (dng) */

  if (diag_bits && !(diag_bits & Gdiag))
    return(-1) ;

  if (diag_bits & DIAG_WAIT)     /* wait for a keystroke before continuing */
    fgetc(stdin) ;    

  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
DiagCloseWindow(unsigned long diag_bits, int win)
{
  if (diag_bits && !(diag_bits & Gdiag))
    return(-1) ;

  win = win + (int)diag_bits ;  /* to remove warning */
  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
DiagCreateWindow(unsigned long diag_bits, int wrows, int wcols,
                 int rows,int cols)
{
  int win = -1 ;
#if 0
   int i;
   i=wcols; 

  if (diag_bits && !(diag_bits & Gdiag))
    return(-1) ;

/*
  create a set of rows x cols windows, each one of which is wrows x wcols
  in size.
*/
#if 0
  win = WinAlloc("window", 50,50, wcols,wrows) ;
  WinShow(win) ;
#else
  win = WinCreate("window", 1, wrows, wcols, rows, cols) ;
#endif
#endif
  return(win) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
DiagFprintf(unsigned long diag_bits, char *fmt, ...)
{
  static int first = 1 ;
  va_list args ;
  FILE    *fp ;
  int     len ;
  
  if (diag_bits && !(diag_bits & Gdiag))
    return(-1) ;

  if (first)
    fp = fopen(diag_fname, "w") ;
  else
    fp = fopen(diag_fname, "a+") ;
  first = 0 ;
  va_start(args, fmt) ;
  len = vfprintf(fp, fmt, args) ;
  fclose(fp) ;
  return(len) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
DiagPrintf(unsigned long diag_bits, char *fmt, ...)
{
  va_list args ;
  
  if (diag_bits && !(diag_bits & Gdiag))
    return(-1) ;

  va_start(args, fmt) ;
  return((*diag_vprintf)(fmt, args)) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          dummy for break points in debugger
------------------------------------------------------*/
void
DiagBreak(void)
{
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          dummy for break points in debugger
------------------------------------------------------*/
void
DiagHeartbeat(float pct_done)
{
  static float old_pct = -10.0f ;

  if ((old_pct < 0.0f) || (old_pct > pct_done) || (pct_done < 0.00001f))
    fprintf(stderr, "\n") ;
  old_pct = pct_done ;
  if (Gdiag & DIAG_HEARTBEAT)
    fprintf(stderr, "\r%2.1f%% finished     ",100.0f*pct_done);
  if (pct_done >= 0.999f)
    fprintf(stderr, "\n") ;
}
