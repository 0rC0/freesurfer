
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
#include "version.h"

#define		STRBUF		65536
#define 	MAX_FILES 	1000
#define 	CE( x )		fprintf(stdout, ( x ))
#define 	START_i  	3

typedef enum _secondOrderType {
	e_Raw,			// "Raw" (native) curvature
	e_Gaussian, 		// Gaussian curvature
	e_Mean,			// Mean curvature
	e_Normal,		// Normalised curvature
	e_Scaled,		// "Raw" scaled curvature
	e_ScaledTrans		// Scaled and translated curvature
} e_secondOrderType;

static char vcid[] = 
	"$Id: mris_curvature_stats.c,v 1.6 2005/09/20 19:49:46 rudolph Exp $";

int 		main(int argc, char *argv[]) ;

static int  	get_option(int argc, char *argv[]) ;
static void 	usage_exit(void) ;
static void 	print_usage(void) ;
static void 	print_help(void) ;
static void 	print_version(void) ;
int  		comp(const void *p, const void *q);    // Compare p and q for qsort()
void		histogram_wrapper(
			MRIS*			amris,
			e_secondOrderType	aesot
		);
void		histogram_create(
			MRIS*		amris_curvature,
			float		af_minCurv,
			double		af_binSize,
			int		abins,
			float*		apf_histogram
		);
void		OFSP_create(
			char*		apch_prefix,
			char*		apch_suffix
		);
void 		secondOrderParams_print(
			MRIS*			amris,
			e_secondOrderType	aesot,
			int			ai
		);
void		outputFileNames_create(void);
void		outputFiles_open(void);
void		shut_down(void);

char*		Progname ;
char*		hemi;

static int 	navgs 			= 0 ;
static int 	normalize_flag 		= 0 ;
static int 	condition_no 		= 0 ;
static int 	stat_flag 		= 0 ;
static char*	label_name 		= NULL ;
static char*	output_fname 		= NULL ;
static char	surf_name[STRBUF];

// Additional global variables (prefixed by 'G') added by RP.
//	Flags are prefixed with Gb_ and are used to track
//	user spec'd command line flags.
static int 	Gb_minMaxShow		= 0;
static int	Gb_histogram		= 0;
static int	Gb_histogramPercent	= 0;
static int	Gb_binSizeOverride	= 0;
static double	Gf_binSize		= 0.;
static int	Gb_histStartOverride	= 0;
static float	Gf_histStart		= 0.;
static int	Gb_gaussianAndMean	= 0;
static int	Gb_output2File		= 0;
static int	Gb_scale		= 0;
static int	Gb_scaleMin		= 0;
static int	Gb_scaleMax		= 0;
static int 	G_nbrs 			= 2 ;
static int	G_bins			= 1;

// All possible output file name and suffixes
static char	Gpch_log[STRBUF];
static char	Gpch_logS[]		= "log";
static FILE*	GpFILE_log		= NULL;
static char	Gpch_allLog[STRBUF];		
static char	Gpch_allLogS[]		= "all.log";
static FILE*	GpFILE_allLog		= NULL;
static char	Gpch_rawHist[STRBUF];
static char	Gpch_rawHistS[]		= "raw.hist";
static FILE*	GpFILE_rawHist		= NULL;
static char	Gpch_normCurv[STRBUF];
static char	Gpch_normHist[STRBUF];
static char	Gpch_normHistS[]	= "norm.hist";
static FILE*	GpFILE_normHist		= NULL;
static char	Gpch_normCurv[STRBUF];
static char	Gpch_normCurvS[]	= "norm.curv";
static char	Gpch_KHist[STRBUF];
static char	Gpch_KHistS[]		= "K.hist";
static FILE*	GpFILE_KHist		= NULL;
static char	Gpch_KCurv[STRBUF];
static char	Gpch_KCurvS[]		= "K.curv";
static char	Gpch_HHist[STRBUF];
static char	Gpch_HHistS[]		= "H.hist";
static FILE*	GpFILE_HHist		= NULL;
static char	Gpch_HCurv[STRBUF];
static char	Gpch_HCurvS[]		= "H.curv";
static char	Gpch_scaledHist[STRBUF];
static char	Gpch_scaledHistS[]	= "scaled.hist";
static FILE*	GpFILE_scaledHist	= NULL;
static char	Gpch_scaledCurv[STRBUF];
static char	Gpch_scaledCurvS[]	= "scaled.curv";
		
// These are used for tabular output
const int	G_leftCols		= 20;
const int	G_rightCols		= 20;

// Mean / sigma tracking and scaling
static double 	Gpf_means[MAX_FILES] ;
static double	Gf_mean			= 0.;
static double	Gf_sigma		= 0.;
static double	Gf_n			= 0.;
static double	Gf_total		= 0.;
static double	Gf_total_sq		= 0.;	
static double	Gf_scaleFactor		= 1.;
static double	Gf_scaleMin		= 0.;
static double	Gf_scaleMax		= 0.;

int
main(int argc, char *argv[])
{
  char         	**av, fname[STRBUF], *sdir ;
  char         	*subject_name, *curv_fname ;
  int          	ac, nargs, i ;
  MRI_SURFACE  	*mris ;

  char		pch_text[65536];

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, 
	"$Id: mris_curvature_stats.c,v 1.6 2005/09/20 19:49:46 rudolph Exp $", "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  sdir = getenv("SUBJECTS_DIR") ;
  if (!sdir)
    ErrorExit(ERROR_BADPARM, "%s: no SUBJECTS_DIR in environment.\n",Progname);
  ac = argc ;
  av = argv ;
  strcpy(surf_name, "-x");
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }
  if(!strcmp(surf_name, "-x"))
    strcpy(surf_name, "orig") ;

  if (argc < 4)
    usage_exit() ;

  /* parameters are 
     1. subject name
     2. hemisphere
  */
  subject_name = argv[1] ; hemi = argv[2] ;
  sprintf(fname, "%s/%s/surf/%s.%s", sdir, subject_name, hemi, surf_name) ;
  printf("Reading surface from %s...\n", fname);
  mris = MRISread(fname) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, fname) ;

  outputFileNames_create();
  outputFiles_open();

  if (label_name)
  {
    LABEL *area ;
    area = LabelRead(subject_name, label_name) ;
    if (!area)
      ErrorExit(ERROR_NOFILE, "%s: could not read label file %s",
                Progname, label_name) ;
    LabelRipRestOfSurface(area, mris) ;
    LabelFree(&area) ;
  }
  if (label_name)
    fprintf(stdout, "%s: ", label_name) ;

  // Process all the command-line spec'd curvature files
  for (Gf_n= Gf_total_sq = Gf_total = 0.0, i = START_i ; i < argc ; i++)
  {
    curv_fname = argv[i] ;
    if (MRISreadCurvatureFile(mris, curv_fname) != NO_ERROR)
      ErrorExit(ERROR_BADFILE,"%s: could not read curvature file %s.\n",
                Progname, curv_fname);

    if(Gb_scale) {
	MRISscaleCurvature(mris, Gf_scaleFactor);
      	if(Gpch_scaledCurv) MRISwriteCurvature(mris, Gpch_scaledCurv);
    }

    if(Gb_scaleMin && Gb_scaleMax) {
	MRISscaleCurvatures(mris, Gf_scaleMin, Gf_scaleMax);
      	if(Gpch_scaledCurv) MRISwriteCurvature(mris, Gpch_scaledCurv);
    }

    if (normalize_flag)
      MRISnormalizeCurvature(mris);


    MRISaverageCurvatures(mris, navgs) ;
    Gf_mean = MRIScomputeAverageCurvature(mris, &Gf_sigma) ;
    sprintf(pch_text, "Raw Curvature (using '%s.%s'):", hemi, curv_fname);
    fprintf(stdout, "%-50s", pch_text);
    fprintf(stdout, "%20.4f +- %2.4f\n", Gf_mean, Gf_sigma) ;
    if(GpFILE_log) 
	fprintf(GpFILE_allLog, "mean/sigma = %20.4f +- %2.4f\n", Gf_mean, Gf_sigma);

    if(Gb_minMaxShow) {
	fprintf(stdout, 
	    "%*s%20.6f\n%*s%20.6f\n",
	    G_leftCols, "min = ", mris->min_curv,
	    G_leftCols, "min = ", mris->max_curv);
	if(GpFILE_allLog)
	    fprintf(GpFILE_allLog, "min = %f\nmax = %f\n", 
		mris->min_curv, mris->max_curv);
	if(Gb_histogram) histogram_wrapper(mris, e_Raw);
    }

    Gpf_means[i-START_i] = Gf_mean ;
    Gf_total += Gf_mean ; Gf_total_sq += Gf_mean*Gf_mean ;
    Gf_n+= 1.0 ;
  }

    // Should we calculate Gaussian and mean? This is a surface-based
    //	calculation, and this does not depend on the curvature processing
    //	loop.
    if(Gb_gaussianAndMean) {
	fprintf(stdout, 
	"\tCalculating second fundamental form for Gaussian, Mean...\n");

	/* Gaussian and Mean curvature calculations */
	MRISsetNeighborhoodSize(mris, G_nbrs);
	MRIScomputeSecondFundamentalForm(mris);
	secondOrderParams_print(mris, e_Gaussian, i);
      	if(Gpch_KCurv) MRISwriteCurvature(mris, Gpch_KCurv);
	secondOrderParams_print(mris, e_Mean,	  i);
      	if(Gpch_HCurv) MRISwriteCurvature(mris, Gpch_HCurv);
    }


  if (Gf_n> 1.8)
  {
    Gf_mean = Gf_total / Gf_n;
    Gf_sigma = sqrt(Gf_total_sq/Gf_n- Gf_mean*Gf_mean) ;
    fprintf(stdout, "\nMean across %d curvature files: %8.4e +- %8.4e\n",
            (int) Gf_n, Gf_mean, Gf_sigma) ;
  }
  //else
  //  Gf_mean = Gf_sigma = 0.0 ;
  

  MRISfree(&mris) ;
  if (output_fname)
  {
    // This code dumps only the *final* mean/sigma values to the summary
    //	log file. For cases where only 1 curvature file has been processed,
    //	the mean/sigma of this file is written. For multiple files, the
    //	mean/sigma over *all* of the files is written.
    if (label_name)
      fprintf(GpFILE_log, "%s: ", label_name) ;
    fprintf(GpFILE_log, "%8.4e +- %8.4e\n", Gf_mean, Gf_sigma) ;
  }
  shut_down();
  exit(0) ;
  return(0) ;  /* for ansi */
}

void secondOrderParams_print(
	MRIS*			amris,
	e_secondOrderType	aesot,
	int			ai
) {
    //
    // PRECONDITIONS
    //	o amris must have been prepared with a call to
    //	  MRIScomputeSecondFundamentalForm(...)
    //
    // POSTCONDITIONS
    // 	o Depending on <esot>, data is printed to stdout (and
    //	  output file, if spec'd).
    //

    char	pch_out[65536];
    char	pch_text[65536];
    float	f_max			= 0.;
    float	f_min			= 0.;

    switch(aesot) {
	case e_Gaussian:
    	    MRISuseGaussianCurvature(amris);
	    sprintf(pch_out, "Gaussian");
	    f_min	= amris->Kmin;
	    f_max	= amris->Kmax;
	break;
	case e_Mean:
	    MRISuseMeanCurvature(amris);
	    sprintf(pch_out, "Mean");
	    f_min	= amris->Hmin;
	    f_max	= amris->Hmax;
	break;
	case e_Raw:
	case e_Normal:
	case e_Scaled:
	case e_ScaledTrans:
	break;
    }
    
    Gf_mean = MRIScomputeAverageCurvature(amris, &Gf_sigma);
    sprintf(pch_text, "%s Curvature (using '%s.%s'):", 
			pch_out, hemi, surf_name);
    fprintf(stdout, "%-50s", pch_text);
    fprintf(stdout, "%20.4f +- %2.4f\n", Gf_mean, Gf_sigma);
    if(GpFILE_allLog)
	fprintf(GpFILE_allLog, "mean/sigma = %20.4f +- %2.4f\n", Gf_mean, Gf_sigma);
    if(Gb_minMaxShow) {
	fprintf(stdout, 
	     	"%*s%20.6f\n%*s%20.6f\n",
	G_leftCols, "min = ", f_min,
	G_leftCols, "max = ", f_max);
	if(GpFILE_allLog)
	    fprintf(GpFILE_allLog, "min = %f\nmax = %f\n",
			f_min, f_max);
    }

    if(Gb_histogram) histogram_wrapper(amris, aesot);
    Gpf_means[ai-START_i] 	= Gf_mean;
    Gf_total 			+= Gf_mean; 
    Gf_total_sq 		+= Gf_mean*Gf_mean ;
    Gf_n			+= 1.0;

}

int  
MRISscaleCurvature(
	MRI_SURFACE*	apmris, 
	float 		af_scale) {
    //
    // POSTCONDITIONS
    //	o Each curvature value in apmris is simply scaled by <af_scale>
    //

    VERTEX*	pvertex;
    int		vno;
    int		vtotal;
    double	f_mean;   

    for(f_mean = 0.0, vtotal = vno = 0 ; vno < apmris->nvertices ; vno++) {
      pvertex = &apmris->vertices[vno] ;
      if (pvertex->ripflag)
        continue ;
      vtotal++ ;
      f_mean += pvertex->curv ;
    }
    f_mean /= (double)vtotal ;

    for (vno = 0 ; vno < apmris->nvertices ; vno++) {
	pvertex = &apmris->vertices[vno] ;
      	if (pvertex->ripflag)
            continue;
      	pvertex->curv = (pvertex->curv - f_mean) * af_scale + f_mean ;
    }
    return(NO_ERROR);
}

void
OFSP_create(
	char*		apch_prefix,
	char*		apch_suffix
) {
    //
    // PRECONDITIONS
    //	o pch_prefix / pch_suffix must be large enough to contain
    //	  required text info.
    //
    // POSTCONDITIONS
    //  o pch_prefix is composed of:
    //		<output_fname>.<hemi>.<surface>.<pch_suffix>
    //

    sprintf(apch_prefix, "%s.%s.%s.%s",
	output_fname,
	hemi,
	surf_name,
	apch_suffix
	);    
}

void
histogram_wrapper(
    	MRIS*			amris,
	e_secondOrderType	aesot
) {
    //
    // PRECONDITIONS
    //	o amris_curvature must be valid and populated with curvature values
    //	  of correct type (i.e. normalised, Gaussian/Mean, scaled)
    //	o apf_histogram memory must be handled (i.e. created/freed) by the
    //	  caller.
    //
    // POSTCONDITIONS
    // 	o max/min_curvatures and binSize are checked for validity.
    //	o histogram memory is created / freed.
    //  o histogram_create() is called.
    //

    int		i;
    float*	pf_histogram;
    float	f_maxCurv	= amris->max_curv;
    float	f_minCurv	= amris->min_curv;
    double 	f_binSize	= 0.;
 
    if(f_maxCurv <= f_minCurv)
	ErrorExit(ERROR_SIZE, "%s: f_maxCurv < af_minCurv.",
			Progname);

    f_binSize		= ((double)f_maxCurv - (double)f_minCurv) / (double)G_bins;
    if(Gb_binSizeOverride)
	f_binSize	= Gf_binSize;

    if(Gb_histStartOverride)
	f_minCurv	= Gf_histStart;

    if(f_minCurv < amris->min_curv)
	f_minCurv	= amris->min_curv;

    if((f_minCurv+G_bins*f_binSize) > f_maxCurv)
	ErrorExit(ERROR_SIZE, "%s: Invalid <binSize> and <bins> combination",
			Progname);

    pf_histogram = calloc(G_bins, sizeof(float));
    histogram_create(	amris,
			f_minCurv,
			f_binSize,
			G_bins,
			pf_histogram);

    printf("%*s%*s%*s\n", G_leftCols, "bin start", 
			  G_leftCols, "bin end", 
			  G_leftCols, "count");
    for(i=0; i<G_bins; i++) {
	printf("%*.4f%*.4f%*.4f ",
		G_leftCols, (i*f_binSize)+f_minCurv, 
		G_leftCols, ((i+1)*f_binSize)+f_minCurv, 
		G_leftCols, pf_histogram[i]);
	printf("\n");
	switch(aesot) {
	    case e_Raw:
		if(GpFILE_rawHist!=NULL)
		fprintf(GpFILE_rawHist, "%f\t%f\t%f\n", 
				(i*f_binSize)+f_minCurv,
				((i+1)*f_binSize)+f_minCurv,
				pf_histogram[i]);
	    break;
	    case e_Gaussian:
		if(GpFILE_KHist!=NULL)
		fprintf(GpFILE_KHist,"%f\t%f\t%f\n", 
				(i*f_binSize)+f_minCurv,
				((i+1)*f_binSize)+f_minCurv,
				pf_histogram[i]);
	    break;
	    case e_Mean:
		if(GpFILE_HHist!=NULL)
		fprintf(GpFILE_HHist,"%f\t%f\t%f\n", 
				(i*f_binSize)+f_minCurv,
				((i+1)*f_binSize)+f_minCurv,
				pf_histogram[i]);
	    break;
	    case e_Normal:
		if(GpFILE_normHist!=NULL)
		fprintf(GpFILE_normHist, "%f\t%f\t%f\n", 
				(i*f_binSize)+f_minCurv,
				((i+1)*f_binSize)+f_minCurv,
				pf_histogram[i]);
	    break;
	    case e_ScaledTrans:
	    case e_Scaled:
		if(GpFILE_scaledHist!=NULL)
		fprintf(GpFILE_scaledHist,"%f\t%f\t%f\n", 
				(i*f_binSize)+f_minCurv,
				((i+1)*f_binSize)+f_minCurv,
				pf_histogram[i]);
	    break;
	}
    }
    free(pf_histogram);
}

void
histogram_create(
	MRIS*			amris_curvature,
	float			af_minCurv,
	double			af_binSize,
	int			abins,
	float*			apf_histogram
) {
    //
    // PRECONDITIONS
    //	o amris_curvature must be valid and populated with curvature values
    //	  of correct type (i.e. normalised, Gaussian/Mean, scaled)
    //	o apf_histogram memory must be handled (i.e. created/freed) by the
    //	  caller.
    //  o af_minCurv < af_maxCurv.
    //
    // POSTCONDITIONS
    //	o curvature values in amris_curvature will be sorted into abins.
    //
    // HISTORY
    // 12 September 2005
    //	o Initial design and coding.
    //

    //char*	pch_function	= "histogram_create()";

    int 	vno		= 0;
    float*	pf_curvature	= NULL;
    int		i		= 0;
    int		j		= 0;
    int		start		= 0;
    int		count		= 0;
    int		totalCount	= 0;
    int		nvertices	= 0;

    nvertices		= amris_curvature->nvertices;
    fprintf(stdout, "\n%*s = %f\n",  
		G_leftCols, "bin size", af_binSize);
    fprintf(stdout, "%*s = %d\n",  
		G_leftCols, "surface vertices", nvertices);
    pf_curvature	= calloc(nvertices, sizeof(float));
    
    for(vno=0; vno < nvertices; vno++) 
	pf_curvature[vno]	= amris_curvature->vertices[vno].curv;

    qsort(pf_curvature, nvertices, sizeof(float), comp);

    for(i=0; i<abins; i++) {
	count	= 0;
	for(j=start; j<nvertices; j++) {
	    if(pf_curvature[j] <= ((i+1)*af_binSize)+af_minCurv) {
		count++;
	        totalCount++;
	    }
	    if(pf_curvature[j] > ((i+1)*af_binSize)+af_minCurv) {
		start = j;
		break;
	    }
	}
	apf_histogram[i]	= count;
	if(Gb_histogramPercent)
	    apf_histogram[i] /= nvertices;
    }
    fprintf(stdout, "%*s = %d\n", 
		G_leftCols, "sorted vertices", totalCount);
    free(pf_curvature);
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int  		nargs 		= 0;
  char*		option;
  char*		pch_text;
  
  option = argv[1] + 1 ;            /* past '-' */

  switch (toupper(*option))
  {
  case 'O':
    output_fname 	= argv[2] ;
    Gb_output2File	= 1;
    nargs 		= 1 ;
    fprintf(stderr, "Outputting results using filestem '%s'...\n", output_fname) ;
    break ;
  case 'F':
    pch_text = argv[2];
    strcpy(surf_name, pch_text); 
    nargs = 1 ;
    fprintf(stderr, "Setting surface to '%s'...\n", surf_name);
    break;
  case 'L':
    label_name = argv[2] ;
    fprintf(stderr, "Using label '%s'...\n", label_name) ;
    nargs = 1 ;
    break ;
  case 'A':
    navgs = atoi(argv[2]) ;
    fprintf(stderr, "Averaging curvature %d times...\n", navgs);
    nargs = 1 ;
    break ;
  case 'C':
    Gb_scale		= 1;
    Gf_scaleFactor 	= atof(argv[2]) ;
    fprintf(stderr, "Setting raw scale factor to %f...\n", Gf_scaleFactor);
    nargs 		= 1 ;
    break ;
  case 'D':
    Gb_scaleMin		= 1;
    Gf_scaleMin		= atof(argv[2]);
    fprintf(stderr, "Setting scale min factor to %f...\n", Gf_scaleMin);
    nargs		= 1;
    break;
  case 'E':
    Gb_scaleMax		= 1;
    Gf_scaleMax		= atof(argv[2]);
    fprintf(stderr, "Setting scale max factor to %f...\n", Gf_scaleMax);
    nargs		= 1;
    break;
  case 'G':
    Gb_gaussianAndMean	= 1;
    break ;
  case 'S':   /* write out stats */
    stat_flag 		= 1 ;
    condition_no 	= atoi(argv[2]) ;
    nargs 		= 1 ;
    fprintf(stderr, "Setting out summary statistics as condition %d...\n",
            condition_no) ;
    break ;
  case 'H':   /* histogram */
    if(argc == 2)
	print_usage();
    Gb_histogram	= 1;
    Gb_histogramPercent	= 0;
    Gb_minMaxShow	= 1;
    G_bins 		= atoi(argv[2]);
    nargs 		= 1 ;
    fprintf(stderr, "Setting curvature histogram to %d bins...\n",
            G_bins);
    if(G_bins <=0 )
    	ErrorExit(ERROR_BADPARM, "%s: Invalid bin number.\n", Progname);
    break ;
  case 'P':   /* percentage histogram */
    Gb_histogram	= 1;
    Gb_histogramPercent	= 1;
    Gb_minMaxShow	= 1;
    G_bins 		= atoi(argv[2]);
    nargs 		= 1 ;
    fprintf(stderr, "Creating percentage curvature histogram with %d bins...\n",
            G_bins);
    break ;
  case 'B':   /* binSize override*/
    Gb_binSizeOverride	= 1;
    Gf_binSize 		= (double) atof(argv[2]);
    nargs 		= 1 ;
    fprintf(stderr, "Setting curvature histogram binSize to %f...\n",
            Gf_binSize);
    break;
  case 'I':   /* histogram start */
    Gb_histStartOverride	= 1;
    Gf_histStart 		= atof(argv[2]);
    nargs 			= 1 ;
    fprintf(stderr, "Setting histogram start point to %f...\n",
            Gf_histStart);
    break;
  case 'N':
    normalize_flag 	= 1 ;
    break ;
  case 'M':
    Gb_minMaxShow	= 1;
    break;
  case 'V':
    printf("Here!!\n\n");
    print_version() ;
    exit(1);
    break;
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

void
outputFileNames_create(void) {
    //
    // POSTCONDITIONS
    //	o All necessary (depending on user flags) file names are created.
    //
    OFSP_create(Gpch_log, 		Gpch_logS);
    // For backwards compatibility with original scheme...
    sprintf(Gpch_log, 			output_fname);	
    OFSP_create(Gpch_allLog, 		Gpch_allLogS);
    OFSP_create(Gpch_rawHist,		Gpch_rawHistS);
    OFSP_create(Gpch_normHist,		Gpch_normHistS);
    OFSP_create(Gpch_normCurv,		Gpch_normCurvS);
    OFSP_create(Gpch_KHist, 		Gpch_KHistS);
    OFSP_create(Gpch_KCurv, 		Gpch_KCurvS);
    OFSP_create(Gpch_HHist,		Gpch_HHistS);
    OFSP_create(Gpch_HCurv,		Gpch_HCurvS);
    OFSP_create(Gpch_scaledHist,	Gpch_scaledHistS);
    OFSP_create(Gpch_scaledCurv,	Gpch_scaledCurvS);
}

void
outputFiles_open(void) {
    //
    // POSTCONDITIONS
    //	o All necessary (depending on user flags) files are opened.
    //

    if(Gb_output2File) {	
	CE("The following files will be created / appended for this run:\n");
  	printf("%s\n", Gpch_log);
	if((GpFILE_log=fopen(Gpch_log, "a"))==NULL)
	    ErrorExit(ERROR_NOFILE, "%s: Could not open file '%s' for apending.\n",
			Progname, Gpch_log);
  	printf("%s\n", Gpch_allLog);
	if((GpFILE_allLog=fopen(Gpch_allLog, "w"))==NULL)
	    ErrorExit(ERROR_NOFILE, "%s: Could not open file '%s' for writing.\n",
			Progname, Gpch_allLog);
  	printf("%s\n", Gpch_rawHist);
	if((GpFILE_rawHist=fopen(Gpch_rawHist, "w"))==NULL)
	    ErrorExit(ERROR_NOFILE, "%s: Could not open file '%s' for writing.\n",
			Progname, Gpch_rawHist);
	if(normalize_flag) {
	    if(Gb_histogram) {
		printf("%s\n", Gpch_normHist);
	    	if((GpFILE_normHist=fopen(Gpch_normHist, "w"))==NULL)
	    		ErrorExit(ERROR_NOFILE, 
				"%s: Could not open file '%s' for writing.\n",
				Progname, Gpch_normHist);
	    }
	    printf("%s\n", Gpch_normCurv);
	}
	if(Gb_gaussianAndMean) {
	    if(Gb_histogram) {
	    	if((GpFILE_KHist=fopen(Gpch_KHist, "w"))==NULL) {
			printf("%s\n", Gpch_KHist);
	    		ErrorExit(ERROR_NOFILE, 
				"%s: Could not open file '%s' for writing.\n",
				Progname, Gpch_KHist);
		}
	    	if((GpFILE_HHist=fopen(Gpch_HHist, "w"))==NULL) {
			printf("%s\n", Gpch_HHist);
	    		ErrorExit(ERROR_NOFILE, 
				"%s: Could not open file '%s' for writing.\n",
				Progname, Gpch_HHist);
		}
	    }
	    printf("%s\n", Gpch_KCurv);
	    printf("%s\n", Gpch_HCurv);
        }
	if(Gb_scale) {
	    if(Gb_histogram) {
		printf("%s\n", Gpch_scaledHist);		
	    	if((GpFILE_scaledHist=fopen(Gpch_scaledHist, "w"))==NULL)
	    		ErrorExit(ERROR_NOFILE, 
				"%s: Could not open file '%s' for writing.\n",
				Progname, Gpch_scaledHist);
	    }
	    printf("%s\n", Gpch_scaledCurv);
	}	
  }
}

void
shut_down(void) {
    //
    // POSTCONDITIONS
    //	o Checks for any open files and closes them.
    //
  
    if(GpFILE_log)		fclose(GpFILE_log);
    if(GpFILE_allLog)		fclose(GpFILE_allLog);
    if(GpFILE_rawHist)		fclose(GpFILE_rawHist);
    if(GpFILE_normHist)		fclose(GpFILE_normHist);
    if(GpFILE_KHist)		fclose(GpFILE_KHist);
    if(GpFILE_HHist)		fclose(GpFILE_HHist);
    if(GpFILE_scaledHist)	fclose(GpFILE_scaledHist);
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
  print_help() ;
}

static void
print_help(void)
{
  char  pch_synopsis[65536];
  
  sprintf(pch_synopsis,"\n\
									\n\
NAME									\n\
									\n\
	mris_curvature_stats						\n\
									\n\
SYNOPSIS								\n\
									\n\
	mris_curvature_stats [options] \\				\n\
		<subjectName> <hemi> <curvFile1> [... <curvFileN]	\n\
									\n\
DESCRIPTION								\n\
									\n\
	'mris_curvature_stats' will primarily compute the mean and	\n\
	variances for a curvature file (or a set of curvature files).	\n\
	These files can be possibly constrained by a label.		\n\
									\n\
	Additionally, 'mris_curvature_stats' can report the max/min	\n\
	curvature values, and compute a simple histogram based on 	\n\
	these vales.							\n\
									\n\
	Curvatures can also be normalised and constrained to a given	\n\
	range before computation.					\n\
									\n\
	Gaussian and mean calculations on a surface structure can	\n\
	be performed.							\n\
									\n\
	Finally, all output to the console, as well as any new		\n\
	curvatures that result from the above calculations can be	\n\
	saved to a series of text and binary-curvature files.		\n\
									\n\
	Please see the following options for more details.		\n\
									\n\
OPTIONS									\n\
									\n\
    [-a <numberOfAverages]						\n\
									\n\
	Average the curvature <numberOfAverages> times.			\n\
									\n\
    [-o <outputFileStem>]						\n\
									\n\
	Save processing results to a series of files. This includes	\n\
	condensed text output, histogram files (MatLAB friendly)	\n\
	and curvature files.						\n\
									\n\
	The actual files that are saved depends on which additional	\n\
	calculation flags have been specified (i.e. normalisation,	\n\
	Gaussian / Mean, scaling).					\n\
									\n\
	In the case when a Gaussian/Mean calculation has been 		\n\
	performed, 'mris_curvature_stats' will act in a manner		\n\
	similar to 'mris_curvature -w'.	 Note though that though the	\n\
	name of the curvature files that are created are different, 	\n\
	the contents are identical to 'mris_curvature -w' created files.\n\
									\n\
	All possible files that can be saved are:			\n\
	(where OFSP = <outputFileStem>.<hemi>.<surface>)		\n\
									\n\
		<outputFileStem>	(log only a single		\n\
					 mean+-sigma. If several	\n\
					 curvature files have been	\n\
					 specified, log the mean+-sigma \n\
					 across all the curvatures.)	\n\
		<OFSP>.all.log		(full output, i.e the output of	\n\
					 curvature as it is processed)  \n\
		<OFSP>.raw.hist		(Raw histogram file. By 'raw'   \n\
					 is implied that the curvature  \n\
					 has not been further processed \n\
					 in any manner.)		\n\
		<OFSP>.norm.hist	(Normalised histogram file)	\n\
		<OFSP>.norm.curv 	(Normalised curv file)		\n\
		<OFSP>.K.hist		(Gaussian histogram file)	\n\
		<OFSP>.K.curv 		(Gaussian curv file)		\n\
		<OFSP>.H.hist		(Mean histogram file)		\n\
		<OFSP>.H.curv 		(Mean curv file)		\n\
		<OFSP>.scaled.hist	(Scaled histogram file)		\n\
		<OFSP>.scaled.curv	(Scaled curv file)		\n\
									\n\
	Note that curvature files are saved to $SUBJECTS_DIR/surf	\n\
	and *not* to the current working directory. Also note that	\n\
	the above names for curvature files are not compatible with	\n\
	the command-line interface of this program. Simply rename	\n\
	any created curvature files (see the examples section).		\n\
									\n\
    [-h <numberOfBins>] [-p <numberOfBins]				\n\
									\n\
	If specified, prepare a histogram over the range of curvature	\n\
	values, using <numberOfBins> buckets. These are dumped to	\n\
	stdout.								\n\
									\n\
	If '-p' is used, then the histogram is expressed as a 		\n\
	percentage.							\n\
									\n\
	The histogram behaviour can be further tuned with the 		\n\
	following:							\n\
									\n\
    [-b <binSize>] [-i <binStartCurvature]				\n\
									\n\
	These two arguments are only processed iff a '-h <numberOfBins>'\n\
	has also been specified. By default, <binSize> is defined as	\n\
									\n\
		(maxCurvature - minCurvature) / <numberOfBins>		\n\
									\n\
	The '-b' option allows the user to specify an arbitrary		\n\
	<binSize>. This is most useful when used in conjunction with	\n\
	the '-i <binStartCurvature>' option, which starts the histogram	\n\
	not at (minCurvature), but at <binStartCurvature>. So, if 	\n\
	a histogram reveals that most values seem confined to a very	\n\
	narrow range, the '-b' and '-i' allow the user to 'zoom in'	\n\
	to this range and expand.					\n\
									\n\
	If <binStartCurvature> < (minCurvature), then regardless	\n\
	of its current value, <binStartCurvature> = (minCurvature).	\n\
	Also, if (<binStartCurvature> + <binSize>*<numberOfBins> >)	\n\
	(maxCurvature), an error is raised and processing aborts.	\n\
									\n\
    [-h]								\n\
									\n\
	Show this help screen.						\n\
									\n\
    [-l <labelFileName>]						\n\
									\n\
	Constrain statistics to the region defined in <labelFileName>.	\n\
									\n\
    [-m]								\n\
									\n\
	Output min / max information for the processed curvature.	\n\
									\n\
    [-n]								\n\
									\n\
	Normalise the curvature before computation. Normalisation	\n\
	takes precedence over scaling, so if '-n' is specified		\n\
	in conjunction with '-c' or '-smin'/'-smax' it will		\n\
	override the effects of the scaling.				\n\
									\n\
	If specified in conjunction with '-o <outputFileStem>'		\n\
	will also create a curvature file with these values.		\n\
									\n\
    [-o <outputFileName>]						\n\
									\n\
	Output *text* results to <outputFileName>.			\n\
									\n\
    [-s <summaryCondition>]						\n\
									\n\
	Write out stats as <summaryCondition>.				\n\
									\n\
    [-d <minCurvature> -e <maxCurvature>]				\n\
									\n\
	Scale curvature values between <minCurvature> and 		\n\
	<maxCurvature>. If the minimum curvature is greater		\n\
	than the maximum curvature, or if either is 			\n\
	unspecified, these flags are ignored.				\n\
									\n\
	This scale computation takes precedence over '-c' scaling.	\n\
									\n\
	Note also that the final scaling bounds might not correspond	\n\
	to <minCurvature>... <maxCurvature> since values are scaled	\n\
	across this range so as to preserve the original mean profile.	\n\
									\n\
	If specified in conjunction with '-o <outputFileStem>'		\n\
	will also create a curvature file with these values.		\n\
									\n\
    [-c <factor>]							\n\
									\n\
	Scale curvature values with <factor>. The mean of the 		\n\
	original curvature is conserved (and the sigma increases)	\n\
	with <factor>.							\n\
									\n\
	If specified in conjunction with '-o <outputFileStem>'		\n\
	will also create a curvature file with these values.		\n\
									\n\
    [-version]								\n\
									\n\
	Print out version number.					\n\
									\n\
NOTES									\n\
									\n\
	It is important to note that some combinations of the command	\n\
	line parameters are somewhat meaningless, such as normalising	\n\
	a 'sulc' curvature file (since it's normalised by definition).	\n\
									\n\
EXAMPLES								\n\
 									\n\
    mris_curvature_stats 801_recon rh curv				\n\
									\n\
	For subject '801_recon', determine the mean and sigma for	\n\
	the curvature file on the right hemisphere.			\n\
									\n\
    mris_curvature_stats -m 801_recon rh curv				\n\
									\n\
	Same as above, but print the min/max curvature values		\n\
	across the surface.						\n\
									\n\
    mris_curvature_stats -h 20 -m 801_recon rh curv			\n\
									\n\
	Same as above, and also print a histogram of curvature 		\n\
	values over the min/max range, using 20 bins. By replacing	\n\
	the '-h' with '-p', print the histogram as a percentage.	\n\
									\n\
    mris_curvature_stats -h 20 -b 0.01 -i 0.1 -m 801_recon rh curv	\n\
									\n\
	Same as above, but this time constrain the histogram to the 20  \n\
	bins from -0.1 to 0.1, with a bin size of 0.01.			\n\
									\n\
	Note that the count / percentage values are taken across the    \n\
	total curvature range and not the constrained window defined 	\n\
	by the '-i' and '-b' arguments.					\n\
									\n\
    mris_curvature_stats -G -m 801_recon rh curv			\n\
									\n\
	Print the min/max curvatures for 'curv', and also calculate	\n\
	the Gaussian and Mean curvatures (also printing the min/max     \n\
	for these).							\n\
									\n\
    mris_curvature_stats -G -F smoothwm -m 801_recon rh curv		\n\
									\n\
	By default, 'mris_curvature_stats' reads the 'orig' surface	\n\
	for the passed subject. This is not generally the best surface	\n\
	for Gaussian determination. The '-F' uses the 'smoothwm' 	\n\
	surface, which is a better choice.				\n\
									\n\
    mris_curvature_stats -h 10 -G -F smoothwm -m 801_recon rh curv	\n\
									\n\
	Same as above, with the addition of a histogram for the 	\n\
	Gaussian and Mean curvatures as well.				\n\
									\n\
    mris_curvature_stats -h 10 -G -F smoothwm -m -o foo \\ 		\n\
	801_recon rh curv 						\n\
									\n\
	Generate several output text files that capture the min/max	\n\
	and histograms for each curvature processed. Also create	\n\
	new Gaussian and Mean curvature files.				\n\
									\n\
	In this case, the curvature files created are called:		\n\
									\n\
		foo.rh.smoothwm.K.curv					\n\
		foo.rh.smoothwm.H.curv					\n\
									\n\
	To process these exact files with 'mris_curvature_stats', you	\n\
	will need to rename them:					\n\
									\n\
		cd $SUBJECTS_DIR/801_recon/surf				\n\
		mv foo.rh.smoothwm.K.curv rh.smoothwm.K.curv		\n\
									\n\
	i.e. just strip away the first prefix. These can then be read	\n\
	with								\n\
									\n\
		mris_curvature_stats -m 801_recon rh smoothwm.K.curv	\n\
									\n\
");

  CE(pch_synopsis);
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}

int  comp(const void *p, const void *q)
{
  float *ptr1 = (float *)(p);
  float *ptr2 = (float *)(q);

  if (*ptr1 < *ptr2) return -1;
  else if (*ptr1 == *ptr2) return 0;
  else return 1;
}
