
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

static char vcid[] = "$Id: mris_average_curvature.c,v 1.1 1998/06/16 21:13:49 fischl Exp $";

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;

char *Progname ;

static int normalize_flag = 0 ;

int
main(int argc, char *argv[])
{
  char         **av, *in_fname, *out_fname, *surf_name, fname[200], *sdir, *hemi ;
  int          ac, nargs, i ;
  MRI_SURFACE  *mris ;
  MRI_SP       *mrisp, *mrisp_total ;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  sdir = getenv("SUBJECTS_DIR") ;
  if (!sdir)
    ErrorExit(ERROR_BADPARM, "%s: no SUBJECTS_DIR in envoronment.\n", Progname);
  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 6)
    usage_exit() ;

  in_fname = argv[1] ;
  hemi = argv[2] ;
  surf_name = argv[3] ;
  out_fname = argv[argc-1] ;

  mrisp_total = MRISPalloc(1, 3) ;
  for (i = 4 ; i < argc-1 ; i++)
  {
    fprintf(stderr, "processing subject %s...\n", argv[i]) ;
    sprintf(fname, "%s/%s/surf/%s.%s", sdir, argv[i], hemi, surf_name) ;
    mris = MRISread(fname) ;
    if (!mris)
      ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, fname) ;
    if (MRISreadCurvatureFile(mris, in_fname) != NO_ERROR)
      ErrorExit(ERROR_BADFILE,"%s: could not read curvature file %s.\n",
                Progname, in_fname);
#if 0
    if (normalize_flag)
      MRISnormalizeCurvature(mris) ;
#endif
    mrisp = MRIStoParameterization(mris, NULL, 1, 0) ;
    MRISPcombine(mrisp, mrisp_total, 0) ;
    MRISPfree(&mrisp) ;
    if (i < argc-2)
      MRISfree(&mris) ;
  }
  
  MRISfromParameterization(mrisp_total, mris, 0) ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "writing blurred pattern to surface to %s\n", out_fname) ;
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
  int  nargs = 0 ;
  char *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    print_help() ;
  else if (!stricmp(option, "-version"))
    print_version() ;
  else switch (toupper(*option))
  {
  case '?':
  case 'U':
    print_usage() ;
    exit(1) ;
    break ;
  case 'N':
    normalize_flag = 1 ;
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
          "usage: %s [options] <input curv. file> <hemi> <surface> <subject> ... "
          " <output curv file >\n", Progname) ;
  fprintf(stderr, "the output curvature file will be painted onto the last "
          "subject name specified\non the command line.\n") ;
}

static void
print_help(void)
{
  print_usage() ;
  fprintf(stderr, 
       "\nThis program will add a template into an average surface.\n");
  fprintf(stderr, "\nvalid options are:\n\n") ;
  fprintf(stderr, "-n    normalize output curvatures.\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}

