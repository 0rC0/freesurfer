#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "mrisurf.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "utils.h"
#include "timer.h"
#include "gcsa.h"
#include "transform.h"
#include "annotation.h"
#include "version.h"

int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;
static int postprocess(GCSA *gcsa, MRI_SURFACE *mris) ;

char *Progname ;
static void usage_exit(int code) ;

static char *read_fname = NULL ;
static int nbrs = 2 ;
static int filter = 10 ;
static char *orig_name = "smoothwm" ;

#if 0
static int normalize_flag = 0 ;
static int navgs = 5 ;
static char *curv_name = "curv" ;
static char *thickness_name = "thickness" ;
static char *sulc_name = "sulc" ;
#endif

static char subjects_dir[STRLEN] ;
extern char *gcsa_write_fname ;
extern int gcsa_write_iterations ;

static int novar = 0 ;
static int refine = 0;

int
main(int argc, char *argv[])
{
  char         **av, fname[STRLEN], *out_fname, *subject_name, *cp,*hemi,
               *canon_surf_name ;
  int          ac, nargs, i ;
  int          msec, minutes, seconds ;
  struct timeb start ;
  MRI_SURFACE  *mris ;
  GCSA         *gcsa ;

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, "$Id: mris_ca_label.c,v 1.11 2005/08/31 22:56:38 xhan Exp $", "$Name:  $");
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

  if (!strlen(subjects_dir)) /* hasn't been set on command line */
  {
    cp = getenv("SUBJECTS_DIR") ;
    if (!cp)
      ErrorExit(ERROR_BADPARM, "%s: SUBJECTS_DIR not defined in environment", 
                Progname);
    strcpy(subjects_dir, cp) ;
  }
  if (argc < 5)
    usage_exit(1) ;

  subject_name = argv[1] ; hemi = argv[2] ; canon_surf_name = argv[3] ; 
  out_fname = argv[5] ;

	printf("reading atlas from %s...\n", argv[4]) ;
  gcsa = GCSAread(argv[4]) ;
  if (!gcsa)
    ErrorExit(ERROR_NOFILE, "%s: could not read classifier from %s",
              Progname, argv[4]) ;

  sprintf(fname, "%s/%s/surf/%s.%s", subjects_dir,subject_name,hemi,orig_name);
  if (DIAG_VERBOSE_ON)
    printf("reading surface from %s...\n", fname) ;
  mris = MRISread(fname) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s for %s",
              Progname, fname, subject_name) ;
  MRISsetNeighborhoodSize(mris, nbrs) ;
  mris->ct = gcsa->ct ; /* hack so that color table will get written into annot file */

  // read colortable from the gcsa if not already done
  if(gcsa->ct != NULL)
    read_named_annotation_table(gcsa->ct->fname);

  MRIScomputeSecondFundamentalForm(mris) ;
  MRISsaveVertexPositions(mris, ORIGINAL_VERTICES) ;
  if (MRISreadCanonicalCoordinates(mris, canon_surf_name) != NO_ERROR)
    ErrorExit(ERROR_NOFILE, "%s: could not read spherical coordinate system from %s for %s",
              Progname, canon_surf_name, subject_name) ;
#if 1
  for (i = gcsa->ninputs-1 ; i >= 0 ; i--)
  {
    printf("input %d: %s, flags %0x, avgs %d, name %s\n",
           i, gcsa->inputs[i].type == GCSA_INPUT_CURV_FILE ? 
           "CURVATURE FILE" : "MEAN CURVATURE", 
           gcsa->inputs[i].flags, 
           gcsa->inputs[i].navgs, gcsa->inputs[i].fname) ;
    switch (gcsa->inputs[i].type)
    {
    case GCSA_INPUT_CURV_FILE:
      if (MRISreadCurvature(mris, gcsa->inputs[i].fname) != NO_ERROR)
        ErrorExit(ERROR_NOFILE, "%s: could not read curv file %s for %s",
                  Progname, gcsa->inputs[i].fname, subject_name) ;
      break ;
    case GCSA_INPUT_CURVATURE:
      MRISuseMeanCurvature(mris) ;
      MRISaverageCurvatures(mris, gcsa->inputs[i].navgs) ;
      break ;
    }
    if (gcsa->inputs[i].flags & GCSA_NORMALIZE)
      MRISnormalizeCurvature(mris) ;
    MRIScopyCurvatureToValues(mris) ;
    if (i == 2)
      MRIScopyCurvatureToImagValues(mris) ;
    else if (i == 1)
      MRIScopyValToVal2(mris) ;
  }
#else
  if (gcsa->ninputs > 2)
  {
    if (MRISreadCurvature(mris, thickness_name) != NO_ERROR)
      ErrorExit(ERROR_NOFILE, "%s: could not read curv file %s for %s",
                Progname, thickness_name, subject_name) ;
    MRIScopyCurvatureToImagValues(mris) ;
  }
  if (gcsa->ninputs > 1 || sulconly)
  {
    if (MRISreadCurvature(mris, sulc_name) != NO_ERROR)
      ErrorExit(ERROR_NOFILE, "%s: could not read curv file %s for %s",
                Progname, curv_name, subject_name) ;
    MRIScopyCurvatureToValues(mris) ;
    MRIScopyValToVal2(mris) ;
  }

  if (!sulconly)
  {
#if 0
    if (MRISreadCurvature(mris, curv_name) != NO_ERROR)
      ErrorExit(ERROR_NOFILE, "%s: could not read curv file %s for %s",
                Progname, sulc_name, subject_name) ;
#else
    MRISuseMeanCurvature(mris) ;
    MRISaverageCurvatures(mris, navgs) ;
#endif
    MRIScopyCurvatureToValues(mris) ;
  }
#endif


  if (novar)
    GCSAsetCovariancesToIdentity(gcsa) ;

  MRISrestoreVertexPositions(mris, CANONICAL_VERTICES) ;
  MRISprojectOntoSphere(mris, mris, DEFAULT_RADIUS) ;
  MRISsaveVertexPositions(mris, CANONICAL_VERTICES) ;

  if (!read_fname)
  {
    printf("labeling surface...\n") ;
    GCSAlabel(gcsa, mris) ;
		if (Gdiag_no >= 0)
			printf("vertex %d: label %s\n", Gdiag_no, annotation_to_name(mris->vertices[Gdiag_no].annotation, NULL)) ;
    printf("relabeling using gibbs priors...\n") ;
    GCSAreclassifyUsingGibbsPriors(gcsa, mris) ;
		if (Gdiag_no >= 0)
			printf("vertex %d: label %s\n", Gdiag_no, annotation_to_name(mris->vertices[Gdiag_no].annotation, NULL)) ;
    postprocess(gcsa, mris) ;
		if (Gdiag_no >= 0)
			printf("vertex %d: label %s\n", Gdiag_no, annotation_to_name(mris->vertices[Gdiag_no].annotation, NULL)) ;
    if (gcsa_write_iterations != 0)
    {
      char fname[STRLEN] ;
      sprintf(fname, "%s_post.annot", gcsa_write_fname) ;
      printf("writing snapshot to %s...\n", fname) ;
      MRISwriteAnnotation(mris, fname) ;
    }
  }
  else{
    
    MRISreadAnnotation(mris, read_fname) ;
    if(refine != 0){
      GCSAreclassifyUsingGibbsPriors(gcsa, mris) ;
      if (Gdiag_no >= 0)
	printf("vertex %d: label %s\n", Gdiag_no, annotation_to_name(mris->vertices[Gdiag_no].annotation, NULL)) ;
      postprocess(gcsa, mris) ;
      if (Gdiag_no >= 0)
	printf("vertex %d: label %s\n", Gdiag_no, annotation_to_name(mris->vertices[Gdiag_no].annotation, NULL)) ;
      if (gcsa_write_iterations != 0)
	{
	  char fname[STRLEN] ;
	  sprintf(fname, "%s_post.annot", gcsa_write_fname) ;
	  printf("writing snapshot to %s...\n", fname) ;
	  MRISwriteAnnotation(mris, fname) ;
	}
      
    }
  }
  
  MRISmodeFilterAnnotations(mris, filter) ;
  if (Gdiag_no >= 0)
    printf("vertex %d: label %s\n", Gdiag_no, annotation_to_name(mris->vertices[Gdiag_no].annotation, NULL)) ;
  
  printf("writing output to %s...\n", out_fname) ;
  if (MRISwriteAnnotation(mris, out_fname) != NO_ERROR)
    ErrorExit(ERROR_NOFILE, "%s: could not write annot file %s for %s",
	      Progname, out_fname, subject_name) ;
  
  MRISfree(&mris) ;
  GCSAfree(&gcsa) ;
  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;
  printf("classification took %d minutes and %d seconds.\n", minutes, seconds) ;
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
  if (!stricmp(option, "SDIR"))
  {
    strcpy(subjects_dir, argv[2]) ;
    nargs = 1 ;
    printf("using %s as subjects directory\n", subjects_dir) ;
  }
  else if (!stricmp(option, "ORIG"))
  {
    orig_name = argv[2] ;
    nargs = 1 ;
    printf("using %s as original surface\n", orig_name) ;
  }
  else if (!stricmp(option, "LONG"))
  {
    refine = 1 ;
    printf("will refine the initial labeling read-in from -R \n") ;
  }
  else if (!stricmp(option, "NOVAR"))
  {
    novar = 1 ;
    printf("setting all covariance matrices to the identity...\n") ;
  }
#if 0
  else if (!stricmp(option, "NORM"))
  {
    normalize_flag = 1 ;
    printf("normalizing sulc after reading...\n") ;
  }
  else if (!stricmp(option, "SULC"))
  {
    sulconly = 1 ;
    printf("using sulc as only input....\n") ;
  }
#endif
  else if (!stricmp(option, "nbrs"))
  {
    nbrs = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "using neighborhood size=%d\n", nbrs) ;
  }
  else switch (toupper(*option))
  {
#if 0
  case 'A':
    navgs = atoi(argv[2]) ;
    nargs = 1 ;
    break ;
#endif
  case 'F':
    filter = atoi(argv[2]) ;
    nargs = 1 ;
    printf("applying mode filter %d times before writing...\n", filter) ;
    break ;
	case 'T':
		if (read_named_annotation_table(argv[2]) != NO_ERROR)
			ErrorExit(ERROR_BADFILE, "%s: could not read annotation file %s...\n", Progname, argv[2]) ;
		nargs = 1 ;
		break ;
  case 'V':
    Gdiag_no = atoi(argv[2]) ;
    nargs = 1 ;
		printf("printing diagnostic information about vertex %d\n", Gdiag_no) ;
    break ;
  case 'W':
    gcsa_write_iterations = atoi(argv[2]) ;
    gcsa_write_fname = argv[3] ;
    nargs = 2 ;
    printf("writing out snapshots of gibbs process every %d iterations to %s\n"
           ,gcsa_write_iterations, gcsa_write_fname) ;
    break ;
  case 'R':
    read_fname = argv[2] ;
    nargs = 1 ;
    printf("reading precomputed parcellation from %s...\n", read_fname) ;
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
  printf("usage: %s [options] <subject> <hemi> <canon surf> "
         "<classifier> <output file>\n", Progname) ;
  exit(code) ;
}
static int
postprocess(GCSA *gcsa, MRI_SURFACE *mris)
{
  LABEL       **larray, *area ;
  int         nlabels, i, j, annotation, n, nchanged, niter = 0, deleted ;
  double      max_area, label_area ;

#define MAX_ITER 5

  do
  {
    deleted = nchanged  = 0 ;
    MRISsegmentAnnotated(mris, &larray, &nlabels, 0) ;
    /*    printf("%d total segments in Gibbs annotation\n", nlabels) ;*/
    MRISclearMarks(mris) ;
    for (i = 0 ; i < nlabels ; i++)
    {
      area = larray[i] ;
      if (!area)   /* already processed */
        continue ;
      annotation = mris->vertices[area->lv[0].vno].annotation ;
      
      /* find label with this annotation with max area */
      max_area = LabelArea(area, mris) ;
      for (n = 1, j = i+1 ; j < nlabels ; j++)
      {
        if (!larray[j])
          continue ;
        if (annotation != mris->vertices[larray[j]->lv[0].vno].annotation)
          continue ;
        n++ ;
        label_area = LabelArea(larray[j], mris) ;
        if (label_area > max_area)
          max_area = label_area ;
      }
#if 0
      printf("%03d: annotation %s (%d): %d segments, max area %2.1f\n", 
             niter, annotation_to_name(annotation, NULL), 
             annotation, n, max_area) ;
#endif
      for (j = i ; j < nlabels ; j++)
      {
        if (!larray[j])
          continue ;
        if (annotation != mris->vertices[larray[j]->lv[0].vno].annotation)
          continue ;
        
#define MIN_AREA_PCT   0.1
        label_area = LabelArea(larray[j], mris) ;
        if (label_area < MIN_AREA_PCT*max_area)
        {
          /*          printf("relabeling annot %2.1f mm area...\n", label_area) ;*/
          nchanged += GCSAreclassifyLabel(gcsa, mris, larray[j]) ;
          deleted++ ;
        }
        LabelFree(&larray[j]) ;
      }
    }
    free(larray) ;
    printf("%03d: %d total segments, %d labels (%d vertices) changed\n",
           niter, nlabels, deleted, nchanged) ;
  } while (nchanged > 0 && niter++ < MAX_ITER) ;
  return(NO_ERROR) ;
}

