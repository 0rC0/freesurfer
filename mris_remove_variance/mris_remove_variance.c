
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

static char vcid[] = "$Id: mris_remove_variance.c,v 1.1 1998/07/25 16:18:49 fischl Exp $";

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;
int MRISremoveValueVarianceFromCurvature(MRI_SURFACE *mris) ;

char *Progname ;

static int navgs = 0 ;

int
main(int argc, char *argv[])
{
  char         **av, *in_fname, *in_curv, *var_curv, *out_curv, fname[200],
               hemi[10], path[200], name[200],*cp ;
  int          ac, nargs ;
  MRI_SURFACE  *mris ;

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

  if (argc < 2)
    usage_exit() ;

  in_fname = argv[1] ;
  in_curv = argv[2] ;
  var_curv = argv[3] ;
  out_curv = argv[4] ;

  FileNamePath(in_fname, path) ;
  FileNameOnly(in_fname, name) ;
  cp = strchr(name, '.') ;
  if (!cp)
    ErrorExit(ERROR_BADPARM, "%s: could not scan hemisphere from '%s'",
              Progname, fname) ;
  strncpy(hemi, cp-2, 2) ;
  hemi[2] = 0 ;

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "reading surface file %s...\n", in_fname) ;
  mris = MRISread(in_fname) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, in_fname) ;

  MRISreadCurvatureFile(mris, var_curv) ;
  MRIScopyCurvatureToValues(mris) ;
  MRISreadCurvatureFile(mris, in_curv) ;
  MRISremoveValueVarianceFromCurvature(mris) ;
  MRISaverageCurvatures(mris, navgs) ;
  MRISwriteCurvature(mris, out_curv) ;
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
  int  nargs = 0 ;
  char *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    print_help() ;
  else if (!stricmp(option, "-version"))
    print_version() ;
  else switch (toupper(*option))
  {
  case 'A':
    navgs = atoi(argv[2]) ;
    fprintf(stderr, "averaging curvature patterns %d times.\n", navgs) ;
    nargs = 1 ;
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
  fprintf(stderr, "usage: %s [options] <input surface file> <curv. file>\n"
          "\t<curv. file to remove> <output curvature file>.\n", Progname) ;
}

static void
print_help(void)
{
  print_usage() ;
  fprintf(stderr, 
      "\nThis program will remove the linear component of the variance \n"
          "\taccounted for by one curvature vector from another.\n") ;
  fprintf(stderr, "\nvalid options are:\n\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}

int
MRISremoveValueVarianceFromCurvature(MRI_SURFACE *mris)
{
  int      vno ;
  VERTEX   *v ;
  double   len, dot ;
  float    min_curv, max_curv, mean_curv ;

  /* first build normalize vector in vertex->val field */

  /* measure length of val vector */
  for (len = 0.0, vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    len += v->val*v->val ;
  }

  if (FZERO(len))
    return(NO_ERROR) ;

  len = sqrt(len) ;

  /* normalize it and compute dot product with curvature */
  for (dot = 0.0, vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    v->val /= len ;
    dot += v->curv * v->val ;
  }

  /* now subtract the the scaled val vector from the curvature vector */
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    v->curv -= dot * v->val ;
  }

  /* recompute min, max and mean curvature */
  min_curv = 10000.0f ; max_curv = -min_curv ; mean_curv = 0.0f ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    mean_curv += v->curv ;
    if (v->curv < min_curv)
      min_curv = v->curv ;
    if (v->curv > max_curv)
      max_curv = v->curv ;
  }

  mris->min_curv = min_curv ; mris->max_curv = max_curv ;
  mean_curv /= (float)mris->nvertices ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, 
            "after variance removal curvature %2.1f --> %2.1f, mean %2.1f.\n",
            min_curv, max_curv, mean_curv) ;
  return(NO_ERROR) ;
}

