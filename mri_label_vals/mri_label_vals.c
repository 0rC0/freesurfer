#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "utils.h"
#include "timer.h"
#include "version.h"
#include "label.h"

int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;

char *Progname ;
static char *log_fname = NULL ;
static void usage_exit(int code) ;

static int quiet = 0 ;
static int all_flag = 0 ;
int cras =0; // 0 is false.  1 is true

int
main(int argc, char *argv[])
{
  char   **av, *label_name, *vol_name, *out_name ;
  int    ac, nargs ;
  int    msec, minutes, seconds,  i  ;
	LABEL  *area ;
  struct timeb start ;
  MRI    *mri ;

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, "$Id: mri_label_vals.c,v 1.6 2003/09/15 19:27:10 tosa Exp $", "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

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

  if (argc < 2)
    usage_exit(1) ;

  vol_name = argv[1] ;
  label_name = argv[2]  ;
  out_name = argv[3] ;

  mri = MRIread(vol_name) ;
  if (!mri)
    ErrorExit(ERROR_NOFILE, "%s: could not read volume from %s",Progname, vol_name) ;
  area = LabelRead(NULL, label_name) ;
  if (!area)
    ErrorExit(ERROR_NOFILE, "%s: could not read label from %s",Progname, label_name) ;

  if (cras == 1)
    printf("using the label coordinates to be c_(r,a,s) != 0.\n");

  for (i = 0 ;  i  < area->n_points ; i++)
  {
    Real  xw, yw, zw, xv, yv, zv, val;

    xw =  area->lv[i].x ;
    yw =  area->lv[i].y ;
    zw =  area->lv[i].z ;
    if (cras == 1)
      MRIworldToVoxel(mri, xw, yw,  zw, &xv, &yv, &zv) ;
    else
      MRIsurfaceRASToVoxel(mri, xw, yw, zw, &xv, &yv, &zv);
    MRIsampleVolumeType(mri, xv,  yv, zv, &val, SAMPLE_NEAREST);
    if (val < .000001)
    {  
      val *= 1000000;
      printf("%f*0.000001\n", val);
    }
    else
      printf("%f\n", val);
  }

  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;

  if (DIAG_VERBOSE_ON)
    fprintf(stderr, "label value extractiong took %d minutes and %d seconds.\n", 
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
  if (strcmp("cras", option) == 0)
    cras = 1;
  else
  {
    switch (toupper(*option))
    {
    case 'Q':
      quiet = 1 ;
      break ;
    case 'A':
      all_flag = 1 ;
      break ;
    case 'L':
      log_fname = argv[2] ;
      nargs = 1 ;
      fprintf(stderr, "logging results to %s\n", log_fname) ;
      break ;
    case 'U':
      usage_exit(0) ;
      break ;
    default:
      fprintf(stderr, "unknown option %s\n", argv[1]) ;
      exit(1) ;
      break ;
    }
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
  printf("usage: %s [options] <volume> <label file>\n",  Progname) ;
  printf("where optins are\n");
  printf("   -cras   label created in the coordinates where c_(r,a,s) != 0\n");
  printf("             if it did not work, try using this option.\n");
  // printf("   -q      quiet\n");
  // printf("   -a      all\n");
  // printf("   -l file log results to a file.\n");
  printf("   -u      print this help.\n");
  exit(code) ;
}
