#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrisurf.h"
#include "mri.h"
#include "macros.h"

static char vcid[] = "$Id: mris_jacobian.c,v 1.1 2002/05/01 17:26:28 fischl Exp $";

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;
static int  compute_area_ratios(MRI_SURFACE *mris, int noscale) ;
static int  log_ratios(MRI_SURFACE *mris) ;

static int log_flag = 0 ;
static int noscale = 0 ;

char *Progname ;

int
main(int argc, char *argv[])
{
  char         **av, *orig_surf, *mapped_surf, *out_fname;
  int          ac, nargs ;
  MRI_SURFACE  *mris ;

  Gdiag = DIAG_SHOW ;
  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;


  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 4)
    usage_exit() ;

  orig_surf = argv[1] ;
  mapped_surf = argv[2] ;
  out_fname = argv[3] ;

  fprintf(stderr, "reading surface from %s...\n", orig_surf) ;
  mris = MRISread(orig_surf) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, orig_surf) ;
  MRIScomputeMetricProperties(mris) ;
  MRISstoreMetricProperties(mris) ;
  MRISsaveVertexPositions(mris, ORIGINAL_VERTICES) ;
  if (MRISreadVertexPositions(mris, mapped_surf) != NO_ERROR)
    ErrorExit(ERROR_BADPARM, "%s: could not read target surface %s",Progname,mapped_surf) ;
  MRIScomputeMetricProperties(mris) ;

  compute_area_ratios(mris, noscale) ;  /* will put results in v->curv */
  if (log_flag)
    log_ratios(mris) ;
  MRISwriteCurvature(mris, out_fname) ;


  MRISfree(&mris) ;
  exit(0) ;
  return(0) ;  /* for ansi */
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int    nargs = 0 ;
  char   *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    print_help() ;
  else if (!stricmp(option, "-version"))
    print_version() ;
  else if (!stricmp(option, "log"))
  {
    log_flag = 1 ;
    fprintf(stderr, "computing log of jacobian...\n") ;
  }
  else switch (toupper(*option))
  {
  case 'N':
    noscale = 1 ;
    printf("not scaling by total surface area\n") ;
    break ;
  case 'V':
    Gdiag_no = atoi(argv[2]) ;
    nargs = 1 ;
    break ;
  case '?':
  case 'U':
    print_usage() ;
    exit(1) ;
    break ;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}

static void
usage_exit(void)
{
  print_usage() ;
  exit(1) ;
}

static void
print_usage(void)
{
  fprintf(stderr, 
       "usage: %s [options] <original surface> <mapped surface> <jacobian file name>\n",
          Progname) ;
}

static void
print_help(void)
{
  print_usage() ;


  fprintf(stderr, 
          "\nThis program computes the jacobian of a surface mapping\n") ;
  fprintf(stderr, "\nvalid options are:\n\n") ;
  fprintf(stderr, "\t-log:\tcompute and write out log of jacobian\n") ;
  fprintf(stderr, "\t-noscale:\tdon't scale jacobian by total surface areas\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}

static int
compute_area_ratios(MRI_SURFACE *mris, int noscale)
{
  VERTEX  *v ;
  int     vno ;
  float   area_scale ;

  if (noscale)
    area_scale = 1 ;
  else
    area_scale = mris->total_area / mris->orig_area  ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;

#define SMALL 0.00001
    if (v->origarea < SMALL)
      v->origarea = SMALL ;
    v->curv = v->area / (v->origarea*area_scale) ;
  }

  return(NO_ERROR) ;
}

static int
log_ratios(MRI_SURFACE *mris)
{
  VERTEX  *v ;
  int     vno ;
  float   area_scale ;

  area_scale = mris->total_area / mris->orig_area  ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;

    v->curv = log10(v->curv) ;
  }

  return(NO_ERROR) ;
}
