#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrimorph.h"
#include "mri_conform.h"
#include "utils.h"
#include "timer.h"

int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;

char *Progname ;
static void usage_exit(int code) ;

static int normalize_flag = 0 ;


int
main(int argc, char *argv[])
{
  char   **av ;
  int    ac, nargs ;
  int          msec, minutes, seconds, wrong, total, correct ;
  struct timeb start ;
  MRI    *mri1, *mri2, *mri_diff ;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  TimerStart(&start) ;

  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 3)
    usage_exit(1) ;


  mri1 = MRIread(argv[1]) ;
  if (!mri1)
    ErrorExit(ERROR_NOFILE, "%s: could not read volume from %s",Progname,
              argv[1]) ;
  mri2 = MRIread(argv[2]) ;
  if (!mri2)
    ErrorExit(ERROR_NOFILE, "%s: could not read volume from %s",Progname,
              argv[2]) ;
  mri_diff = MRIabsdiff(mri1, mri2, NULL) ;
  MRIbinarize(mri_diff, mri_diff, 1, 0, 1) ;
  wrong = MRIvoxelsInLabel(mri_diff, 1) ;
#if 0
  MRIbinarize(mri1, mri1, 1, 0, 1) ;
  total = MRIvoxelsInLabel(mri1, 1) ;
  MRIbinarize(mri2, mri2, 1, 0, 1) ;
  total += MRIvoxelsInLabel(mri1, 1) ;
  total /= 2 ;
#else
  total = mri1->width*mri1->height*mri1->depth ;
#endif

  correct = total-wrong ;
  printf("%d of %d voxels correctly labeled - %2.1f%%\n",
         correct, total, 100.0f*(float)correct/(float)total) ;

  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;

  if (DIAG_VERBOSE_ON)
    fprintf(stderr, "overlap calculation took %d minutes and %d seconds.\n", 
            minutes, seconds) ;

  exit(0) ;
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  switch (toupper(*option))
  {
  case 'N':
    normalize_flag = atoi(argv[2]) ;
    fprintf(stderr, "noise-sensitiviy normalization %s\n",
            normalize_flag ? "on" : "off") ;
    nargs = 1 ;
    break ;
  case '?':
  case 'U':
    usage_exit(0) ;
    break ;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
usage_exit(int code)
{
  printf("usage: %s [options] <inverse operator> <EEG/MEG data file>",
         Progname) ;
  printf(
         "\tf <f low> <f hi> - apply specified filter (not implemented yet)\n"
         );
  printf("\tn - noise-sensitivity normalize inverse (default=1)") ;
  exit(code) ;
}
