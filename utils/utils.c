/*
   @(#)utils.c  1.12
   10/16/95
*/
/*------------------------------------------------------------------------
      File Name: utils.c

         Author: Bruce Fischl

        Created: Jan. 1994

    Description: miscellaneous utility functions

------------------------------------------------------------------------*/

#ifdef _HOME_
#define FZERO(d)  (fabs(d) < 0.0000001)
#endif

/*------------------------------------------------------------------------
                              HEADERS
------------------------------------------------------------------------*/



#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>

#include <time.h> /* msvc (dng) */

#include "nr.h"
#include "const.h"
#include "utils.h"
#include "proto.h"
#include "error.h"
#include "image.h"
#include "macros.h"

/*------------------------------------------------------------------------
                            CONSTANTS
------------------------------------------------------------------------*/


/*------------------------------------------------------------------------
                            STRUCTURES
------------------------------------------------------------------------*/


/*------------------------------------------------------------------------
                            GLOBAL DATA
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
                            STATIC DATA
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
                            STATIC PROTOTYPES
------------------------------------------------------------------------*/


double drand48(void) ;

/*------------------------------------------------------------------------
                              FUNCTIONS
------------------------------------------------------------------------*/
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
double
randomNumber(double low, double hi)
{
  double val, range ;
  static long idum = -1L ;

  if (low > hi)
  {
    val = low ;
    low = hi ;
    hi = val ;
  }

  if (idum <= 0L)     /* change seed from run to run */
    idum = -1L * (long)(abs((int)time(NULL))) ; 

  range = hi - low ;
  val = ran1(&idum) * range + low ;
  
  if ((val < low) || (val > hi))
    ErrorPrintf(ERROR_BADPARM, "randomNumber(%2.1f, %2.1f) - %2.1f\n",
               (float)low, (float)hi, (float)val) ;

   return(val) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
double
normAngle(double angle)
{
   while (angle > PI)
      angle -= 2.0 * PI ;

   while (angle < -PI)
      angle += 2.0 * PI ;

   return(angle) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
double
calcDeltaPhi(double target_phi, double robot_phi)
{
   double delta_phi ;

   if (target_phi < 0.0)
      target_phi += 2.0 * PI ;
   else if (target_phi > 2.0 * PI)
      target_phi -= 2.0 * PI ;
   
   if (robot_phi < 0.0)
      robot_phi += 2.0 * PI ;
   else if (robot_phi > 2.0 * PI)
      robot_phi -= 2.0 * PI ;
   
   delta_phi = (target_phi - robot_phi) ;

   
   while (delta_phi > PI)
      delta_phi -= 2.0 * PI ;

   while (delta_phi < -PI)
      delta_phi += 2.0 * PI ;

   return(delta_phi) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
double
latan2(double y, double x)
{
   int oerr ;
   double val ;
   
   oerr = errno ;
   val = atan2(y, x) ;
   if (FZERO(x) && FZERO(y))
      val = 0.0 ;
   if (val < -PI)
      val += 2.0 * PI ;
   if (val > PI)
      val -= 2.0 * PI ;

   if (oerr != errno)
      ErrorPrintf(ERROR_BADPARM,
                  "error %d, y %f, x %f\n", errno, (float)y, (float)x) ;
   return(val) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
int
QuadEqual(double a1, double a2)
{
  a1 = normAngle(a1) ;
  a2 = normAngle(a2) ;
  if (fabs(a1 - a2) < RADIANS(90.0))
    return(1) ;
  else
    return(0) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
void
fComplementCode(double *pdIn, double *pdOut, int iLen)
{
  int     i, iFullLen ;
  double  d ;

  iFullLen = iLen * 2 ;
  for (i = 0 ; i < iLen ; i++, pdOut++)
  {
    d = *pdIn++ ;
/*    *pdOut = d ;*/
    pdOut[iLen] = 1.0 - d ;
  }
}

#ifdef _HOME_
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
double
drand48(void)
{
  int  r ;

  r = rand() ;
  return((double)r / (double)RAND_MAX) ;
}
#endif
#ifndef _HOME_
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
char *
fgetl(char *s, int n, FILE *fp)
{
  char *cp ;

  do
  {
    cp = fgets(s, n, fp) ;
    if (!cp)
      return(NULL) ;

    while (isspace(*cp))
      cp++ ;

  } while (((*cp) == '#') || ((*cp) == '\n') || ((*cp) == 0)) ;

  return(cp) ;
}
#endif
/*
 * IntSqrt(n) -- integer square root
 *
 * Algorithm due to Isaac Newton, takes log(n)/2 iterations.
 */
int 
IntSqrt(int n)
{
  register int approx, prev;

  if (n == 0)
    return 0;

  if (n < 4)
    return 1;

  approx = n/2; /* 1st approx */

  do 
  {
    /*
     * f(x) = x**2 - n
     *
     * nextx = x - f(x)/f'(x)
     *   = x - (x**2-n)/(2*x) x**2 may overflow
     *   = x - (x - n/x)/2  but this will *not*
     */

    prev = approx;
    approx = prev - (prev - n / prev)/2;

  } while (approx != prev);

  return(approx) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
char *
StrUpper(char *str)
{
  char *cp ;

  for (cp = str ; *cp ; cp++)
    *cp = (char)toupper(*cp) ;

  return(str) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
  
    Return Values:
        nothing.
------------------------------------------------------------------------*/
char *
StrLower(char *str)
{
  char *cp ;

  for (cp = str ; *cp ; cp++)
    *cp = (char)tolower(*cp) ;

  return(str) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
         extract just the file name (no path) from a string.

    Return Values:
        nothing.
------------------------------------------------------------------------*/
char *
FileName(char *full_name)
{
  char *fname, *colon, *at ;

  fname = strrchr(full_name, '/') ;
  if (!fname)
    fname = full_name ;
  else
    fname++ ;   /* skip '/' */

  if (*fname == '@')
    fname++ ;

  colon = strrchr(fname, ':') ;
  if (colon)
    *colon = 0 ;
  at = strrchr(fname, '@') ;
  if (at)
    *at = 0 ;

  return(fname) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
        determine whether a given file exists or not

    Return Values:
        1 if the file exists, 0 otherwise
------------------------------------------------------------------------*/
int
FileExists(char *fname)
{
  FILE *fp ;

  fp = fopen(fname, "r") ;
  if (fp)
    fclose(fp) ;

  return(fp != NULL) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
         determine the file type from the name

    Return Values:

------------------------------------------------------------------------*/
int
FileType(char *fname)
{
  char *dot, buf[100], *colon ;

  if (*fname == '@')
    return(LIST_FILE) ;

  strcpy(buf, fname) ;
  dot = strrchr(buf, '@') ;
  colon = strchr(buf, ':') ;
  if (colon)
    *colon = 0 ;  /* don't consider : part of extension */
  if (!dot)
    dot = strrchr(buf, '.') ;

  if (dot)
  {
    dot++ ;
    StrUpper(buf) ;
    if (!strcmp(dot, "MAT"))
      return(MATLAB_FILE) ;
    else if (!strcmp(dot, "HIPL")||!strcmp(dot, "HIPS") || !strcmp(dot,"HIP"))
      return(HIPS_FILE) ;
    else if (!strcmp(dot, "LST"))
      return(LIST_FILE) ;
  }
  return(UNKNOWN_FILE) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
         if a number is specified return it, otherwise return -1

    Return Values:

------------------------------------------------------------------------*/
int
FileNumber(char *fname)
{
  char buf[100], *colon ;
  int  num ;

  strcpy(buf, fname) ;
  colon = strchr(buf, ':') ;
  if (colon)
    sscanf(colon+1, "%d", &num) ;
  else
    num = -1 ;
  return(num) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
          determine the number of separate entries in a file

      Return Values:

------------------------------------------------------------------------*/
int
FileNumberOfEntries(char *fname)
{
  int  type, num, nentries ;
  FILE *fp ;
  char buf[100], line[200], *cp ;

  strcpy(buf, fname) ;   /* we will modify fname, don't ruin callers copy */
  fname = buf ;
  
  num = FileNumber(fname) ;
  if (num == -1)
  {
    type = FileType(fname) ;
    switch (type)
    {
    case LIST_FILE:
      fp = fopen(FileName(fname), "rb") ;
      if (!fp)
        ErrorReturn(-1, (ERROR_NO_FILE, 
                         "FileNumberOfEntries: could not open %s",
                         FileName(fname))) ;
      cp = fgetl(line, 199, fp) ;
      nentries = 0 ;
      while (cp)
      {
        sscanf(cp, "%s", buf) ;
        num = FileNumberOfEntries(buf) ;
        nentries += num ;
        cp = fgetl(line, 199, fp) ;
      }
      fclose(fp) ;
      
      break ;
    case HIPS_FILE:
      nentries = ImageNumFrames(fname) ;
      break ;
    case MATLAB_FILE:
    default:
      nentries = 1 ;
      break ;
    }
  }
  else
    nentries = 1 ;

  return(nentries) ;
}
/*------------------------------------------------------------------------
       Parameters:

      Description:
         extract the file name including path, removing additional
         modifiers such as '@' and ':'

    Return Values:
        nothing.
------------------------------------------------------------------------*/
char *
FileFullName(char *full_name)
{
  char *fname, *colon, *at ;

  fname = full_name ;
  if (*fname == '@')
    fname++ ;

  colon = strrchr(fname, ':') ;
  if (colon)
    *colon = 0 ;
  at = strrchr(fname, '@') ;
  if (at)
    *at = 0 ;

  return(fname) ;
}
