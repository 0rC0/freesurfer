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

static char vcid[] = "$Id: mris_add_template.c,v 1.2 1998/01/24 15:53:45 fischl Exp $";

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;

char *Progname ;

static char curvature_fname[100] = "" ;

#define IMAGES_PER_SURFACE   3   /* mean, variance, and dof */
#define SURFACES             2   /* smoothwm and inflated */
#define PARAM_IMAGES         (IMAGES_PER_SURFACE*SURFACES)

static int nbrs = 2 ;
static int navgs = 0 ;
static float scale = 1 ;

int
main(int argc, char *argv[])
{
  char         **av, surf_fname[100], *template_fname, *out_fname, *surf_dir,
               *hemi ;
  int          ac, nargs ;
  MRI_SURFACE  *mris ;
  MRI_SP       *mrisp, *mrisp_template ;

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

  surf_dir = argv[1] ;
  hemi = argv[2] ;
  out_fname = template_fname = argv[3] ;
  if (argc > 4)
    out_fname = argv[4] ;

  sprintf(surf_fname, "%s/%s.%s", surf_dir, hemi, SPHERE_NAME) ;
  fprintf(stderr, "reading new surface %s...\n", surf_fname) ;
  mris = MRISread(surf_fname) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, surf_fname) ;
  MRISsaveVertexPositions(mris, ORIGINAL_VERTICES) ;

  if (!FileExists(template_fname))  /* first time - create it */
  {
    fprintf(stderr, "creating new parameterization...\n") ;
    mrisp_template = MRISPalloc(scale, PARAM_IMAGES); 
  }
  else
  {
    fprintf(stderr, "reading template parameterization from %s...\n",
            template_fname) ;
    mrisp_template = MRISPread(template_fname) ;
    if (!mrisp_template)
      ErrorExit(ERROR_NOFILE, "%s: could not open template file %s",
                Progname, template_fname) ;
  }
  /*
    first read in inflated surface and use it to build the first template
    set.
    */
  sprintf(surf_fname, "%s/%s.%s", surf_dir, hemi, INFLATED_NAME) ;
  if (MRISreadVertexPositions(mris, surf_fname) != NO_ERROR)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, surf_fname) ;

  MRISsetNeighborhoodSize(mris, nbrs) ;
  MRIScomputeMetricProperties(mris) ;
  MRIScomputeSecondFundamentalForm(mris) ;
  MRISuseMeanCurvature(mris) ;
  MRISaverageCurvatures(mris, navgs) ;
  MRISrestoreVertexPositions(mris, ORIGINAL_VERTICES) ;
  MRISnormalizeCurvature(mris) ;
  fprintf(stderr, "computing parameterization for surface %s...\n",surf_fname);
  mrisp = MRIStoParameterization(mris, NULL, scale, 0) ;
  MRISPcombine(mrisp, mrisp_template, 0) ;
  MRISPfree(&mrisp) ;

  /*
    now do the same thing with the smoothwm curvatures.
    */
  sprintf(surf_fname, "%s/%s.%s", surf_dir, hemi, SMOOTH_NAME) ;
  if (MRISreadVertexPositions(mris, surf_fname) != NO_ERROR)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, surf_fname) ;
  MRIScomputeMetricProperties(mris) ;
  if (curvature_fname[0])
    MRISreadCurvatureFile(mris, curvature_fname) ;
  else
  {
    MRIScomputeSecondFundamentalForm(mris) ;
    MRISuseMeanCurvature(mris) ;
    MRISaverageCurvatures(mris, navgs) ;
  }
  MRISrestoreVertexPositions(mris, ORIGINAL_VERTICES) ;
  fprintf(stderr, "computing parameterization for surface %s...\n",surf_fname);
  MRISnormalizeCurvature(mris) ;
  mrisp = MRIStoParameterization(mris, NULL, scale, 0) ;
  MRISPcombine(mrisp, mrisp_template, 3) ;

  fprintf(stderr, "writing updated template to %s...\n", out_fname) ;
  MRISPwrite(mrisp_template, out_fname) ;

  MRISPfree(&mrisp) ;
  MRISPfree(&mrisp_template) ;
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
  else if (!stricmp(option, "nbrs"))
  {
    nbrs = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "using neighborhood size = %d\n", nbrs) ;
  }
  else switch (toupper(*option))
  {
  case 'S':
    scale = atof(argv[2]) ;
    fprintf(stderr, "scaling parameterization by %2.1f\n", scale) ;
    nargs = 1 ;
    break ;
  case 'A':
    navgs = atoi(argv[2]) ;
    fprintf(stderr, "averaging curvature patterns %d times.\n", navgs) ;
    nargs = 1 ;
    break ;
  case 'C':
    strcpy(curvature_fname, argv[2]) ;
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
          "usage: %s [options] <surface dir> <hemisphere> <average surface>\n",
          Progname) ;
}

static void
print_help(void)
{
  print_usage() ;
  fprintf(stderr, 
       "\nThis program will add a template into an average surface.\n");
  fprintf(stderr, "\nvalid options are:\n\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}

