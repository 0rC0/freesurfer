/*
 *       FILE NAME:   error.c
 *
 *       DESCRIPTION: error handling routines
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <hips_error.h>

#include "error.h"
#include "hips.h"
#include "proto.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/

#define ERROR_FNAME  "error.log"

/*-----------------------------------------------------
                    STATIC DATA
-------------------------------------------------------*/

static char error_fname[100] = ERROR_FNAME ;
static int (*error_vprintf)(const char *fmt, va_list args) = vprintf ;
static int (*error_vfprintf)(FILE *fp,const char *fmt,va_list args) = vfprintf;
static void (*error_exit)(int ecode) = (void *)(int)exit ;

/*-----------------------------------------------------
                      GLOBAL DATA
-------------------------------------------------------*/

int Gerror = NO_ERROR ;

/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/

/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ErrorInit(char *fname, 
                  int (*vfprint)(FILE *fp, const char *fmt, va_list args),
                  int (*vprint)(const char *fmt, va_list args))
{
  if (fname)
    strcpy(error_fname, fname) ;
  if (vfprint)
    error_vfprintf = vfprint ;
  if (vprint)
    error_vprintf = vprint ;

  unlink(error_fname) ; /* start with a fresh log file */

  /* probably should be some info into log file like date/user etc... */
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
void
ErrorExit(int ecode, char *fmt, ...)
{
  va_list  args ;

  Gerror = ecode ;
  va_start(args, fmt) ;
  vfprintf(stderr, fmt, args) ;
  fprintf(stderr, "\n") ;
  if (errno)
    perror(NULL) ;
  if (hipserrno)
    perr(ecode, "Hips error:") ;

  (*error_exit)(ecode) ;
}

/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ErrorPrintf(int ecode, char *fmt, ...)
{
  va_list  args ;
  FILE     *fp ;

  Gerror = ecode ;
  va_start(args, fmt) ;
  (*error_vfprintf)(stderr, fmt, args) ;
  (*error_vfprintf)(stderr, "\n", NULL) ;
  if (errno)
    perror(NULL) ;
  if (hipserrno)
    perr(ecode, "Hips error:") ;

  fp = fopen(ERROR_FNAME, "a") ;
  (*error_vfprintf)(fp, fmt, args) ;
  (*error_vfprintf)(fp, "\n", NULL) ;
  fclose(fp) ;     /* close file to flush changes */
  
  return(ecode) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ErrorSetExitFunc(void (*exit_func)(int ecode))
{
  error_exit = exit_func ;
  return(1) ;
}

