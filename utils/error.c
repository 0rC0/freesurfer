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
static int (*error_vprintf)(char *fmt, va_list args) = vprintf ;
static int (*error_vfprintf)(FILE *fp, char *fmt, va_list args) = vfprintf ;

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
                  int (*vfprint)(FILE *fp, char *fmt, va_list args),
                  int (*vprint)(char *fmt, va_list args))
{
  if (fname)
    strcpy(error_fname, fname) ;
  if (vfprint)
    error_vfprintf = vfprint ;
  if (vprint)
    error_vprintf = vprint ;

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

  va_start(args, fmt) ;
  vfprintf(stderr, fmt, args) ;
  if (errno)
    perror(NULL) ;
  if (hipserrno)
    perr(ecode, "Hips error:") ;

  exit(ecode) ;
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

  va_start(args, fmt) ;
  (*error_vfprintf)(stderr, fmt, args) ;
  if (errno)
    perror(NULL) ;
  if (hipserrno)
    perr(ecode, "Hips error:") ;

  fp = fopen(ERROR_FNAME, "a") ;
  (*error_vfprintf)(fp, fmt, args) ;
  fclose(fp) ;     /* close file to flush changes */
  
  return(ecode) ;
}

