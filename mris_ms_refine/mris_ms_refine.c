
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "timer.h"
#include "mrisurf.h"
#include "mri.h"
#include "macros.h"
#include "mrimorph.h"
#include "mrinorm.h"
#include "mri_conform.h"
#include "cvector.h"
#include "histo.h"

static char vcid[] = "$Id: mris_ms_refine.c,v 1.3 2002/01/23 20:20:36 fischl Exp $";

int main(int argc, char *argv[]) ;

#define MAX_FLASH_VOLUMES   50
#define ORIG_EXPANSION_DIST 1.0  /* average thickness of the cortex in mm (approx) */
#define MAX_SAMPLES         1000
static int MAX_VNO =        5000000 ;
#define EPSILON             0.25

#define IS_CSF(T1, PD)  (!((PD > MIN_PD_CSF) && (T1 > MIN_T1_CSF && T1 < MAX_T1_CSF)))
#define IS_WM(T1,PD)   ((T1 >= MIN_WM_T1) && (T1 <= MAX_WM_T1) && (PD <= MAX_WM_PD) && (PD >= MIN_WM_PD))
#define WM_PARM_DIST(T1,PD)   (T1 > MAX_WM_T1 ? (T1-MAX_WM_T1) : T1 < MIN_WM_T1 ? MIN_WM_T1-T1 : 0)

static int   plot_stuff = 0 ;
#if 0
static float histo_sigma = 1.0f ;
#endif
static int fix_T1 = 0 ;
static int  build_lookup_table(double tr, double flip_angle, 
                               double min_T1, double max_T1, double step) ;

static int    compute_T1_PD(Real *image_vals, MRI **mri_flash, int nvolumes, 
                            double *pT1, double *pPD);
#if 0
static int    compute_T1_PD_slow(Real *image_vals, MRI **mri_flash, int nvolumes, 
                                 double *pT1, double *pPD);
#endif
static double lookup_flash_value(double TR, double flip_angle, double PD, double T1) ;
#if 0
static float subsample_dist = 10.0 ;
#endif

#define MIN_T1          10
#define MAX_T1          10000
#define T1_STEP_SIZE    5

#define MIN_WM_T1  500
#define MAX_WM_T1  950
#define MIN_GM_T1  900
#define MAX_GM_T1  1400
#define MIN_WM_PD  550
#define MIN_GM_PD  700
#define MAX_WM_PD  1300
#define MIN_CSF_T1 1800
#define MIN_CSF_PD 1500

static int sample_type = SAMPLE_NEAREST ;
static double MIN_T1_CSF = 500.0, MAX_T1_CSF = MIN_CSF_T1 /*1400.0*/ ;
static double MIN_PD_CSF = 600 ;

typedef struct
{
  float *cv_wm_T1 ;
  float *cv_wm_PD ;
  float *cv_gm_T1 ;
  float *cv_gm_PD ;
  float *cv_csf_T1 ;
  float *cv_csf_PD ;
  float *cv_inward_dists ;
  float *cv_outward_dists ;
  MRI   **mri_flash ;
  int   nvolumes ;
  int   *nearest_pial_vertices ;
  int   *nearest_white_vertices ;
  double current_sigma ;
  double dstep ;
  double max_inward_dist ;
  double max_outward_dist ;
  int    nvertices ;   /* # of vertices in surface on previous invocation */
} EXTRA_PARMS ;

#define BRIGHT_LABEL         130
#define BRIGHT_BORDER_LABEL  100
static double  DSTEP = 0.25 ;
static double MAX_DSTEP = 0.5 ;   /* max sampling distance */
#define MAX_DIST  3               /* max distance to sample in and out */

#define DEFORM_WHITE         0
#define DEFORM_PIAL          1

/*static int init_lookup_table(MRI *mri) ;*/
static double compute_sse(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
static double compute_optimal_parameters(MRI_SURFACE *mris, int vno, EXTRA_PARMS *ep, 
                                         double *pwhite_delta, double *pial_delta) ;
static double compute_optimal_vertex_positions(MRI_SURFACE *mris, int vno, 
                                               EXTRA_PARMS *ep, double *pwhite_delta, 
                                               double *ppial_delta) ;
static double image_forward_model(double TR, double flip_angle, double dist, 
                                  double thickness, double T1_wm, double PD_wm, 
                                  double T1_gm, double PD_gm, double T1_csf, double PD_csf) ;
static double vertex_error(MRI_SURFACE *mris, int vno, EXTRA_PARMS *ep, double *prms) ;
static int write_map(MRI_SURFACE *mris, float *cv, char *name, int suffix, char *output_suffix) ;
static int write_maps(MRI_SURFACE *mris, EXTRA_PARMS *ep, int suffix, char *output_suffix) ;
static int smooth_maps(MRI_SURFACE *mris, EXTRA_PARMS *ep, int smooth_parms) ;
static int smooth_map(MRI_SURFACE *mris, float *cv, int navgs) ;
#if 1
static int soap_bubble_maps(MRI_SURFACE *mris, EXTRA_PARMS *ep, int smooth_parms) ;
static int soap_bubble_map(MRI_SURFACE *mris, float *cv, int navgs) ;
#endif
static int compute_maximal_distances(MRI_SURFACE *mris, float sigma, MRI **mri_flash, 
                                     int nvolumes, float *cv_inward_dists, 
                                     float *cv_outward_dists, int *nearest_pial_vertices,
                                     int *nearest_white_vertices, double dstep,
                                     double max_inward_dist, double max_outward_dist) ;
static int find_nearest_pial_vertices(MRI_SURFACE *mris, int *nearest_pial_vertices,
                                      int  *nearest_white_vertices) ;
static int find_nearest_white_vertices(MRI_SURFACE *mris, int *nearest_white_vertices) ;
static int sample_parameter_maps(MRI_SURFACE *mris, MRI *mri_T1, MRI *mri_PD,
                                 float *cv_wm_T1, float *cv_wm_PD,
                                 float *cv_gm_T1, float *cv_gm_PD,
                                 float *cv_csf_T1, float *cv_csf_PD,
                                 float *cv_inward_dists, float *cv_outward_dists,
                                 EXTRA_PARMS *ep, int fix_T1, INTEGRATION_PARMS *parms,
                                 int start_vno) ;
#if 0
static int sample_parameter_map(MRI_SURFACE *mris, MRI *mri, MRI *mri_res,
                                float *cv_parm, float *cv_dists, float dir, 
                                int which_vertices,
                                char *name, HISTOGRAM *prior_histo, 
                                double dstep,
                                double max_dist) ;
#endif

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;
static int  mrisFindMiddleOfGray(MRI_SURFACE *mris) ;
MRI *MRIfindBrightNonWM(MRI *mri_T1, MRI *mri_wm) ;

static char brain_name[STRLEN] = "brain" ;
#if 0
static double dM_dT1(double flip_angle, double TR, double PD, double T1) ;
static double dM_dPD(double flip_angle, double TR, double PD, double T1) ;
#endif
static double FLASHforwardModel(double flip_angle, double TR, double PD, 
                                double T1) ;
static double FLASHforwardModelLookup(double flip_angle, double TR, double PD, 
                                double T1) ;

static double compute_vertex_sse(EXTRA_PARMS *ep, Real image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES], 
                                 int max_j,
                                 double white_dist, double cortical_dist, double T1_wm, double PD_wm, 
                                 double T1_gm, double PD_gm, double T1_csf, double PD_csf, int debug) ;
char *Progname ;
static char *gSdir = NULL ;

static int graymid = 0 ;
static int curvature_avgs = 10 ;
static int create = 1 ;
static int smoothwm = 0 ;
static int white_only = 0 ;

static int apply_median_filter = 0 ;

static int nbhd_size = 5 ;

static INTEGRATION_PARMS  parms ;
#define BASE_DT_SCALE    1.0
static float base_dt_scale = BASE_DT_SCALE ;

static int add = 0 ;

static int orig_flag = 0 ;
static char *start_white_name = NULL ;
static char *start_pial_name = NULL ;
static int smooth_parms = 10 ;
static int smooth = 0 ;
static int vavgs = 0 ;
static int nwhite = 25 /*5*/ ;
static int ngray = 30 /*45*/ ;

static int nbrs = 2 ;
static int write_vals = 0 ;

static char *suffix = "" ;
static char *output_suffix = "ms" ;
static char *xform_fname = NULL ;

static char pial_name[STRLEN] = "pial" ;
static char white_matter_name[STRLEN] = WHITE_MATTER_NAME ;
static char orig_name[STRLEN] = ORIG_NAME ;

static int lh_label = LH_LABEL ;
static int rh_label = RH_LABEL ;
#if 0
static int max_averages = 0 ;
static int min_averages = 0 ;
static float sigma = 0.0f ;
#else
static int max_averages = 8 ;
static int min_averages = 1 ;
static float sigma = 1.0f ;
#endif
static float max_thickness = 5.0 ;

static char *map_dir = "parameter_maps" ;


static int ms_errfunc_gradient(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
static double ms_errfunc_sse(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;
static int ms_errfunc_timestep(MRI_SURFACE *mris, INTEGRATION_PARMS *parms);
static double ms_errfunc_rms(MRI_SURFACE *mris, INTEGRATION_PARMS *parms) ;

static float *cv_inward_dists, *cv_outward_dists ;
static float *cv_wm_T1, *cv_wm_PD, *cv_gm_T1, *cv_gm_PD, *cv_csf_T1, *cv_csf_PD ;
static MRI *mri_T1 = NULL, *mri_PD = NULL ;

int
main(int argc, char *argv[])
{
  char          **av, *hemi, *sname, sdir[STRLEN], *cp, fname[STRLEN], mdir[STRLEN],
                *xform_fname ;
  int           ac, nargs, i, /*label_val, replace_val,*/ msec, n_averages, nvolumes,
                index, j, k, start_t, niter ;
  MRI_SURFACE   *mris ;
  MRI           *mri_template = NULL,
    /* *mri_filled, *mri_labeled ,*/ *mri_flash[MAX_FLASH_VOLUMES] ;
  float         max_len ;
  double        current_sigma, sse = 0.0, last_sse = 0.0 ;
  struct timeb  then ;
  LTA           *lta ;
  EXTRA_PARMS   ep ;

  Gdiag |= DIAG_SHOW ;
  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  memset(&parms, 0, sizeof(parms)) ;
  parms.projection = NO_PROJECTION ;
  parms.tol = 1e-3 ;
  parms.dt = 0.25f ;
  parms.base_dt = BASE_DT_SCALE*parms.dt ;
  parms.l_spring = 0.0f ; parms.l_curv = 1.0 ; parms.l_intensity = 0.0 ;
  parms.l_tspring = 1.0f ; parms.l_nspring = 0.1f ;
  parms.l_repulse = 1 /* was 1 */ ; parms.l_surf_repulse = 5 ;
  parms.l_external = 1 ;

#if 0
  parms.l_spring = 0.0f ; parms.l_curv = 0.0 ; parms.l_intensity = 0.0 ;
  parms.l_tspring = 0.0f ; parms.l_nspring = 0.0f ;
  parms.l_repulse = 0.0 ;
  parms.dt = 1.0f ;
#endif

  parms.niterations = 0 ;
  parms.write_iterations = 0 /*WRITE_ITERATIONS */;
  parms.integration_type = INTEGRATE_MOMENTUM ;
  parms.momentum = 0.0 /*0.8*/ ;
  /*  parms.integration_type = INTEGRATE_LINE_MINIMIZE ;*/
  parms.user_parms = (void *)&ep ;

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

  /* set default parameters for white and gray matter surfaces */
  parms.niterations = nwhite ;
  if (parms.momentum < 0.0)
    parms.momentum = 0.0 /*0.75*/ ;

  gMRISexternalGradient = ms_errfunc_gradient ;
  gMRISexternalSSE = ms_errfunc_sse ;
  gMRISexternalRMS = ms_errfunc_rms ;
  gMRISexternalTimestep = ms_errfunc_timestep ;

  TimerStart(&then) ;
  sname = argv[1] ;
  hemi = argv[2] ;
  if (gSdir)
    strcpy(sdir, gSdir) ;
  else
  {
    cp = getenv("SUBJECTS_DIR") ;
    if (!cp)
      ErrorExit(ERROR_BADPARM, 
                "%s: SUBJECTS_DIR not defined in environment.\n", Progname) ;
    strcpy(sdir, cp) ;
  }

  xform_fname = argv[3] ;
  sprintf(fname, "%s/%s/mri/flash/%s/%s", sdir, sname, map_dir, xform_fname) ;
  lta = LTAread(fname) ;
  if (!lta)
    ErrorExit(ERROR_NOFILE, "%s: could not read FLASH transform from %s...\n", 
              Progname, fname) ;

  cp = getenv("MRI_DIR") ;
  if (!cp)
    ErrorExit(ERROR_BADPARM, 
              "%s: MRI_DIR not defined in environment.\n", Progname) ;
  strcpy(mdir, cp) ;


#define FLASH_START 4
  nvolumes = argc - FLASH_START ;
  printf("reading %d flash volumes...\n", nvolumes) ;
  for (i = FLASH_START ; i < argc ; i++)
  {
    MRI *mri ;
    
    index = i-FLASH_START ;
    sprintf(fname, "%s/%s/mri/flash/%s/%s", sdir, sname, map_dir, argv[i]) ;
    printf("reading FLASH volume %s...\n", fname) ;
    mri = mri_flash[index] = MRIread(fname) ;
    if (!mri_flash[index])
      ErrorExit(ERROR_NOFILE, "%s: could not read FLASH volume from %s...\n", 
                Progname, fname) ;
    /*    init_lookup_table(mri_flash[index]) ;*/

    if (!mri_template)
    {
      mri = MRIcopy(mri_flash[index], NULL) ;
      mri_template = MRIconform(mri) ; mri = mri_flash[index] ;
    }

    mri_flash[index] = MRIresample(mri, mri_template, RESAMPLE_INTERPOLATE) ;
    mri_flash[index]->tr = mri->tr ;
    mri_flash[index]->flip_angle = mri->flip_angle ;
    mri_flash[index]->te = mri->te ;
    mri_flash[index]->ti = mri->ti ;
    MRIfree(&mri) ; mri = mri_flash[index] ;
    printf("TR = %2.1f msec, flip angle = %2.0f degrees, TE = %2.1f msec\n",
           mri->tr, DEGREES(mri->flip_angle), mri->te) ;
    build_lookup_table(mri->tr, mri->flip_angle, MIN_T1, MAX_T1, T1_STEP_SIZE) ;
  }

  index = i-FLASH_START ;

  if (mri_T1 || mri_PD)
  {
    MRI    *mri_tmp ;

    if (mri_T1)
    {
      mri_tmp = MRIresample(mri_T1, mri_template, RESAMPLE_INTERPOLATE) ;
      MRIfree(&mri_T1) ; mri_T1 = mri_tmp ;
    }
    if (mri_PD)
    {
      mri_tmp = MRIresample(mri_PD, mri_template, RESAMPLE_INTERPOLATE) ;
      MRIfree(&mri_PD) ; mri_PD = mri_tmp ;
    }
  }
  MRIfree(&mri_template) ;

#if 0
  sprintf(fname, "%s/%s/mri/filled", sdir, sname) ;
  printf("reading volume %s...\n", fname) ;
  mri_filled = MRIread(fname) ;
  if (!mri_filled)
    ErrorExit(ERROR_NOFILE, "%s: could not read input volume %s",
              Progname, fname) ;
  if (!stricmp(hemi, "lh"))
  { label_val = lh_label ; replace_val = rh_label ; }
  else
  { label_val = rh_label ; replace_val = lh_label ; }

  sprintf(fname, "%s/%s/mri/%s", sdir, sname, T1_name) ;
  printf("reading volume %s...\n", fname) ;

  /* remove other hemi */
  MRIdilateLabel(mri_filled, mri_filled, replace_val, 1) ;
  if (replace_val == RH_LABEL)
  {
    MRIdilateLabel(mri_filled, mri_filled, RH_LABEL2, 1) ;
    for (i = 0 ; i < nvolumes ; i++)
      MRImask(mri_flash[i], mri_filled, mri_flash[i], RH_LABEL2,0) ;
  }
  
  /*  MRImask(mri_T1, mri_filled, mri_T1, replace_val,0) ;*/
  MRIfree(&mri_filled) ;

  sprintf(fname, "%s/%s/mri/wm", sdir, sname) ;
  printf("reading volume %s...\n", fname) ;
  mri_wm = MRIread(fname) ;
  if (!mri_wm)
    ErrorExit(ERROR_NOFILE, "%s: could not read input volume %s",
              Progname, fname) ;
  mri_labeled = MRIfindBrightNonWM(mri_T1, mri_wm) ;

  MRIfree(&mri_wm) ;
#endif
  if (orig_flag)
  {
    sprintf(fname, "%s/%s/surf/%s.%s%s", sdir, sname, hemi, orig_name, suffix) ;
    printf("reading orig surface position from %s...\n", fname) ;
  }
  else
  {
    if (start_pial_name)
      sprintf(fname, "%s/%s/surf/%s.%s%s", sdir, sname, hemi, 
              start_pial_name, suffix) ;
    else
      sprintf(fname, "%s/%s/surf/%s.%s%s", sdir, sname, hemi, pial_name, 
              suffix) ;
  }
  if (add)
    mris = MRISreadOverAlloc(fname, 1.5) ;
  else
    mris = MRISreadOverAlloc(fname, 1.0) ;
  if (!mris)
    ErrorExit(ERROR_NOFILE, "%s: could not read surface file %s",
              Progname, fname) ;
  if (add)
    printf("surface read - max vertices %d\n", mris->max_vertices) ;

  MRISallocExtraGradients(mris) ;

  sprintf(parms.base_name, "%s%s", output_suffix, suffix) ;
  if (orig_flag)
  {
    MRISaverageVertexPositions(mris, 2) ;  /* so normals will be reasonable */
    MRISsaveVertexPositions(mris, ORIGINAL_VERTICES) ;
#if 0
    MRIScomputeMetricProperties(mris) ;
    MRISexpandSurface(mris, ORIG_EXPANSION_DIST, &parms) ;
#endif
    MRISsaveVertexPositions(mris, PIAL_VERTICES) ;
    parms.start_t = 0 ;
  }
  else
  {
    MRISsaveVertexPositions(mris, PIAL_VERTICES) ;
    if (start_white_name)
    {
      if (MRISreadOriginalProperties(mris, start_white_name) != NO_ERROR)
        ErrorExit(ERROR_NOFILE, 
                  "%s: could not read gray/white surface from %s",
                  Progname, start_white_name) ;
    }
    else
    {
      if (MRISreadOriginalProperties(mris, white_matter_name) != NO_ERROR)
        ErrorExit(ERROR_NOFILE, 
                  "%s: could not read gray/white surface from %s",
                  Progname, white_matter_name) ;
    }
  }

  if (smooth)
  {
    printf("smoothing surface for %d iterations...\n", smooth) ;
    MRISaverageVertexPositions(mris, smooth) ;
  }
    
  if (nbrs > 1)
    MRISsetNeighborhoodSize(mris, nbrs) ;

  MRIScomputeMetricProperties(mris) ;    /* recompute surface normals */

  if (add)
  {
    printf("adding vertices to initial tessellation...\n") ;
    for (max_len = 1.5*8 ; max_len > 1 ; max_len /= 2)
      while (MRISdivideLongEdges(mris, max_len) > 0)
      {}
  }

  ep.nvertices = mris->nvertices ;
  ep.dstep = MAX_DSTEP ;
  if (orig_flag)
    ep.max_outward_dist = 3*MAX_DIST ;
  else
    ep.max_outward_dist = MAX_DIST ;
  ep.max_inward_dist = MAX_DIST ;
  ep.mri_flash = mri_flash ; ep.nvolumes = nvolumes ;
  ep.cv_inward_dists = cv_inward_dists = cvector_alloc(mris->max_vertices) ;
  ep.cv_outward_dists = cv_outward_dists = cvector_alloc(mris->max_vertices) ;
  ep.cv_wm_T1 = cv_wm_T1 = cvector_alloc(mris->max_vertices) ;
  ep.cv_wm_PD = cv_wm_PD = cvector_alloc(mris->max_vertices) ;
  ep.cv_gm_T1 = cv_gm_T1 = cvector_alloc(mris->max_vertices) ;
  ep.cv_gm_PD = cv_gm_PD = cvector_alloc(mris->max_vertices) ;
  ep.cv_csf_T1 = cv_csf_T1 = cvector_alloc(mris->max_vertices) ;
  ep.cv_csf_PD = cv_csf_PD = cvector_alloc(mris->max_vertices) ;
  ep.nearest_pial_vertices = (int *)calloc(mris->max_vertices, sizeof(int)) ;
  ep.nearest_white_vertices = (int *)calloc(mris->max_vertices, sizeof(int)) ;
  if (!cv_inward_dists || !cv_outward_dists || !cv_wm_T1 || !cv_wm_PD
      || !cv_gm_T1 || !cv_gm_PD || !cv_csf_T1 || !cv_csf_PD || !ep.nearest_pial_vertices
      || !ep.nearest_white_vertices)
    ErrorExit(ERROR_NOMEMORY, "%s: could allocate %d len cvector arrays",
              Progname, mris->nvertices) ;
  current_sigma = sigma ;
  for (n_averages = max_averages, i = 0 ; 
       n_averages >= min_averages ; 
       n_averages /= 2, current_sigma /= 2, i++)
  {
    ep.current_sigma = current_sigma ;
    printf("computing inner and outer bounds on error functional, avgs=%d,sigma=%2.2f,dstep=%2.3f,"
           "max_dist=%2.2f:%2.2f...\n",
           n_averages, current_sigma, ep.dstep, ep.max_inward_dist, ep.max_outward_dist) ;
    {
      for (j = 0 ; j < mris->nvertices ; j++)
        ep.nearest_white_vertices[j] = -1 ;
    }

    k = 0 ;
    do
    {
      start_t = parms.start_t ;
      if (k++ > 2)   /* should be user settable, but.... */
        break ;
      find_nearest_pial_vertices(mris, ep.nearest_pial_vertices, ep.nearest_white_vertices);
      find_nearest_white_vertices(mris, ep.nearest_white_vertices) ;
      compute_maximal_distances(mris, current_sigma, mri_flash, nvolumes, 
                                cv_inward_dists, cv_outward_dists,
                                ep.nearest_pial_vertices, ep.nearest_white_vertices,
                                ep.dstep, ep.max_inward_dist, ep.max_outward_dist) ;
#if 0
      if (orig_flag && start_t == 0)
      {
        for (i = 0 ; i < mris->nvertices ; i++)
          cv_outward_dists[i] = 4.0 ;   /* first time through allow large-scale search */
      }
#endif
      printf("estimating tissue parameters for gray/white/csf...\n") ;
      sample_parameter_maps(mris, mri_T1, mri_PD, cv_wm_T1, cv_wm_PD,
                            cv_gm_T1, cv_gm_PD, cv_csf_T1, cv_csf_PD,
                            cv_inward_dists, cv_outward_dists, &ep, fix_T1, &parms, 0) ;
      fix_T1 = 0 ; /* only on 1st time through */
      smooth_maps(mris, &ep, smooth_parms) ;
      if (Gdiag & DIAG_WRITE)
        write_maps(mris, &ep, n_averages, output_suffix) ;
      if (Gdiag_no >= 0 && Gdiag_no < mris->nvertices)
      {
        printf("v %d: wm = (%2.0f, %2.0f), gm = (%2.0f, %2.0f), csf = (%2.0f, %2.0f)\n",
               Gdiag_no,ep.cv_wm_T1[Gdiag_no], ep.cv_wm_PD[Gdiag_no], 
               ep.cv_gm_T1[Gdiag_no], ep.cv_gm_PD[Gdiag_no],
               ep.cv_csf_T1[Gdiag_no], ep.cv_csf_PD[Gdiag_no]);
        DiagBreak() ;
      }
      
      parms.sigma = current_sigma ;
      parms.n_averages = n_averages ; 
      MRISprintTessellationStats(mris, stderr) ;

      if (FZERO(last_sse))  /* has to be done after maximal distances and parameter sampling */
      {
        last_sse = sse = compute_sse(mris, &parms) ;
        printf("outer loop: initial sse = %2.3f\n", sse/(double)mris->nvertices) ;
      }
      else
        last_sse = sse ;

      if (write_vals)
      {
        sprintf(fname, "./%s-white%2.2f.w", hemi, current_sigma) ;
        MRISwriteValues(mris, fname) ;
      }
      MRISpositionSurfaces(mris, mri_flash, nvolumes, &parms);
      if (add)
      {
        for (max_len = 1.5*8 ; max_len > 1 ; max_len /= 2)
          while (MRISdivideLongEdges(mris, max_len) > 0)
          {}
      }
      sse = compute_sse(mris, &parms) ;
      printf("outer loop iteration %d: sse = %2.3f (last=%2.3f)\n", 
             k, sse/(double)mris->nvertices, last_sse/(double)mris->nvertices);
      if (parms.fp)
        fprintf(parms.fp, 
                "outer loop iteration %d: sse = %2.3f (last=%2.3f), dstep=%2.3f, max_dist=(%2.2f:%2.2f)\n", 
                k, sse/(double)mris->nvertices, 
                last_sse/(double)mris->nvertices, ep.dstep, ep.max_inward_dist, ep.max_outward_dist) ;
      niter = parms.start_t - start_t ;
      break ;
    } while (sse < (last_sse - last_sse * (parms.tol/niter))) ;
    if (!n_averages)
      break ;
#if 0
    ep.dstep /= 2 ; 
    if (ep.dstep < 0.2)
      ep.dstep = 0.2 ;  /* no point in sampling finer (and takes forever) */
#endif
    ep.max_outward_dist = MAX_DIST ;
  }

  sprintf(fname, "%s/%s/surf/%s.%s%s%s", sdir, sname,hemi,white_matter_name,
          output_suffix,suffix);
  printf("writing white matter surface to %s...\n", fname) ;
  MRISaverageVertexPositions(mris, smoothwm) ;
  MRISwrite(mris, fname) ;
#if 0
  if (smoothwm > 0)
  {
    MRISaverageVertexPositions(mris, smoothwm) ;
    sprintf(fname, "%s/%s/surf/%s.%s%s", sdir, sname,hemi,SMOOTH_NAME,
            suffix);
    fprintf(stderr,"writing smoothed white matter surface to %s...\n",fname);
    MRISwrite(mris, fname) ;
  }
#endif
  
  if (create)   /* write out curvature and area files */
  {
    MRIScomputeMetricProperties(mris) ;
    MRIScomputeSecondFundamentalForm(mris) ;
    MRISuseMeanCurvature(mris) ;
    MRISaverageCurvatures(mris, curvature_avgs) ;
    sprintf(fname, "%s.curv%s%s",
            mris->hemisphere == LEFT_HEMISPHERE?"lh":"rh", output_suffix,
            suffix);
    printf("writing smoothed curvature to %s\n", fname) ;
    MRISwriteCurvature(mris, fname) ;
    sprintf(fname, "%s.area%s",
            mris->hemisphere == LEFT_HEMISPHERE?"lh":"rh", suffix);
#if 0
    printf("writing smoothed area to %s\n", fname) ;
    MRISwriteArea(mris, fname) ;
#endif
    MRISprintTessellationStats(mris, stderr) ;
  }
  
  sample_parameter_maps(mris, mri_T1, mri_PD, cv_wm_T1, cv_wm_PD,
                        cv_gm_T1, cv_gm_PD, cv_csf_T1, cv_csf_PD,
                        cv_inward_dists, cv_outward_dists, &ep, 0, &parms, 0) ;
  smooth_maps(mris, &ep, smooth_parms) ;

  write_maps(mris, &ep, -1, output_suffix) ;
  msec = TimerStop(&then) ; 
  fprintf(stderr,"positioning took %2.1f minutes\n", (float)msec/(60*1000.0f));
  MRISrestoreVertexPositions(mris, ORIGINAL_VERTICES) ;
  sprintf(fname, "%s/%s/surf/%s.%s%s%s", sdir, sname, hemi, 
          white_matter_name, output_suffix, suffix) ;
  printf("writing final white matter position to %s...\n", fname) ;
  MRISwrite(mris, fname) ;
  MRISrestoreVertexPositions(mris, PIAL_VERTICES) ;
  sprintf(fname, "%s/%s/surf/%s.%s%s%s", sdir, sname, hemi, 
          pial_name, output_suffix, suffix) ;
  printf("writing final pial surface position to %s...\n", fname) ;
  MRISwrite(mris, fname) ;


  /*  if (!(parms.flags & IPFLAG_NO_SELF_INT_TEST))*/
  {
    printf("measuring cortical thickness...\n") ;
    MRISmeasureCorticalThickness(mris, nbhd_size, max_thickness) ;
    printf(
            "writing cortical thickness estimate to 'thickness' file.\n") ;
    sprintf(fname, "thickness%s%s", output_suffix, suffix) ;
    MRISwriteCurvature(mris, fname) ;

    /* at this point, the v->curv slots contain the cortical surface. Now
       move the white matter surface out by 1/2 the thickness as an estimate
       of layer IV.
    */
    if (graymid)
    {
      MRISsaveVertexPositions(mris, TMP_VERTICES) ;
      mrisFindMiddleOfGray(mris) ;
      sprintf(fname, "%s/%s/surf/%s.%s%s", sdir, sname, hemi, GRAYMID_NAME,
              suffix) ;
      printf("writing layer IV surface to %s...\n", fname) ;
      MRISwrite(mris, fname) ;
      MRISrestoreVertexPositions(mris, TMP_VERTICES) ;
    }
  }
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
    printf( "using neighborhood size = %d\n", nbrs) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "sample"))
  {
    if (stricmp(argv[2], "nearest") == 0)
    {
      sample_type = SAMPLE_NEAREST ;
      printf("using nearest nbr sampling\n") ;
    }
    else if (stricmp(argv[2], "trilinear") == 0)
    {
      sample_type = SAMPLE_TRILINEAR ;
      printf("using trilinear interpolation\n") ;
    }
    else
      ErrorExit(ERROR_BADPARM,  "unknown sampling type %s\n", argv[2]) ;

    nargs = 1 ;
  }
  else if (!stricmp(option, "fix_T1"))
  {
    fix_T1 = 1 ;
    printf( "fixing T1 values of gray and white matter\n");
  }
  else if (!stricmp(option, "orig"))
  {
    orig_flag = 1 ;
    printf( "using orig surface as initial surface placement\n") ;
  }
  else if (!stricmp(option, "start"))
  {
    start_white_name = argv[2] ;
    start_pial_name = argv[3] ;
    printf( "using %s and %s surfaces for initial placement\n", 
            start_white_name, start_pial_name) ;
    nargs = 2 ;
  }
  else if (!stricmp(option, "maxv"))
  {
    MAX_VNO = atoi(argv[2]) ;
    printf( "limiting  calculations to 1st %d vertices\n", MAX_VNO) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "dstep"))
  {
    MAX_DSTEP = (double)atof(argv[2]) ;
    printf( "sampling volume every %2.2f mm\n", DSTEP) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "T1"))
  {
    printf("reading T1 parameter map from %s...\n", argv[2]) ;
    mri_T1 = MRIread(argv[2]) ;
    if (!mri_T1)
      ErrorExit(ERROR_NOFILE, "%s: could not read T1 volume from %s",
                Progname, argv[2]) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "TOL"))
  {
    parms.tol = atof(argv[2]) ;
    printf("using integration tol %e\n", parms.tol) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "PD"))
  {
    printf("reading PD parameter map from %s...\n", argv[2]) ;
    mri_PD = MRIread(argv[2]) ;
    
    if (!mri_PD)
      ErrorExit(ERROR_NOFILE, "%s: could not read PD volume from %s",
                Progname, argv[2]) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "brain"))
  {
    strcpy(brain_name, argv[2]) ;
    printf("using %s as brain volume...\n", brain_name) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "SDIR"))
  {
    gSdir = argv[2] ;
    printf("using %s as SUBJECTS_DIR...\n", gSdir) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "median"))
  {
    apply_median_filter = 1 ;
  }
  else if (!stricmp(option, "map_dir"))
  {
    map_dir = argv[2] ;
    nargs = 1 ;
    printf("reading parameter maps and residuals from %s...\n", map_dir) ;
  }
  else if (!stricmp(option, "graymid"))
  {
    graymid = 1 ;
    printf("generating graymid surface...\n") ;
  }
  else if (!strcmp(option, "rval"))
  {
    rh_label = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr,"using %d as fill val for right hemisphere.\n", rh_label);
  }
  else if (!strcmp(option, "nbhd_size"))
  {
    nbhd_size = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr,"using %d size nbhd for thickness calculation.\n", 
            nbhd_size);
  }
  else if (!strcmp(option, "lval"))
  {
    lh_label = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr,"using %d as fill val for left hemisphere.\n", lh_label);
  }
  else if (!stricmp(option, "whiteonly"))
  {
    white_only = 1 ;
    printf( "only generating white matter surface\n") ;
  }
  else if (!stricmp(option, "pial"))
  {
    strcpy(pial_name, argv[2]) ;
    printf( "writing pial surface to file named %s\n", pial_name) ;
  }
  else if (!stricmp(option, "write_vals"))
  {
    write_vals = 1 ;
    printf( "writing gray and white surface targets to w files\n") ;
  }
  else if (!stricmp(option, "name"))
  {
    strcpy(parms.base_name, argv[2]) ;
    nargs = 1 ;
    printf("base name = %s\n", parms.base_name) ;
  }
  else if (!stricmp(option, "dt"))
  {
    parms.dt = atof(argv[2]) ;
    parms.base_dt = base_dt_scale*parms.dt ;
    parms.integration_type = INTEGRATE_MOMENTUM ;
    printf( "using dt = %2.1e\n", parms.dt) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "spring"))
  {
    parms.l_spring = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_spring = %2.3f\n", parms.l_spring) ;
  }
  else if (!stricmp(option, "repulse"))
  {
    parms.l_repulse = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_repulse = %2.3f\n", parms.l_repulse) ;
  }
  else if (!stricmp(option, "grad"))
  {
    parms.l_grad = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_grad = %2.3f\n", parms.l_grad) ;
  }
  else if (!stricmp(option, "external"))
  {
    parms.l_external = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_external = %2.3f\n", parms.l_external) ;
  }
  else if (!stricmp(option, "tspring"))
  {
    parms.l_tspring = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_tspring = %2.3f\n", parms.l_tspring) ;
  }
  else if (!stricmp(option, "nspring"))
  {
    parms.l_nspring = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_nspring = %2.3f\n", parms.l_nspring) ;
  }
  else if (!stricmp(option, "curv"))
  {
    parms.l_curv = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_curv = %2.3f\n", parms.l_curv) ;
  }
  else if (!stricmp(option, "smooth"))
  {
    smooth = atoi(argv[2]) ;
    nargs = 1 ;
    printf("smoothing for %d iterations\n", smooth) ;
  }
  else if (!stricmp(option, "smooth_parms"))
  {
    smooth_parms = atoi(argv[2]) ;
    nargs = 1 ;
    printf("smoothing parameter maps for %d iterations\n", smooth_parms) ;
  }
  else if (!stricmp(option, "output"))
  {
    output_suffix = argv[2] ;
    nargs = 1 ;
    printf("appending %s to output names...\n", output_suffix) ;
  }
  else if (!stricmp(option, "vavgs"))
  {
    vavgs = atoi(argv[2]) ;
    nargs = 1 ;
    printf("smoothing values for %d iterations\n", vavgs) ;
  }
  else if (!stricmp(option, "white"))
  {
    strcpy(white_matter_name, argv[2]) ;
    nargs = 1 ;
    printf("using %s as white matter name...\n", white_matter_name) ;
  }
  else if (!stricmp(option, "intensity"))
  {
    parms.l_intensity = atof(argv[2]) ;
    nargs = 1 ;
    printf("l_intensity = %2.3f\n", parms.l_intensity) ;
  }
  else if (!stricmp(option, "lm"))
  {
    parms.integration_type = INTEGRATE_LINE_MINIMIZE ;
    printf("integrating with line minimization\n") ;
  }
  else if (!stricmp(option, "nwhite"))
  {
    nwhite = atoi(argv[2]) ;
    nargs = 1 ;
    printf(
           "integrating gray/white surface positioning for %d time steps\n",
           nwhite) ;
  }
  else if (!stricmp(option, "smoothwm"))
  {
    smoothwm = atoi(argv[2]) ;
    printf("writing smoothed (%d iterations) wm surface\n",
            smoothwm) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "ngray"))
  {
    ngray = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr,"integrating pial surface positioning for %d time steps\n",
            ngray) ;
  }
  else if (!stricmp(option, "sigma"))
  {
    sigma = atof(argv[2]) ;
    printf( "smoothing volume with Gaussian sigma = %2.1f\n", 
            sigma) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "add"))
  {
    add = 1 ;
    printf("adding vertices to tessellation during deformation.\n");
    parms.flags |= IPFLAG_ADD_VERTICES ;
  }
  else switch (toupper(*option))
  {
  case 'S':
    suffix = argv[2] ;
    printf("using %s as suffix\n", suffix) ;
    nargs = 1 ;
    break ;
  case '?':
  case 'U':
    print_usage() ;
    exit(1) ;
    break ;
  case 'T':
    xform_fname = argv[2] ;
    nargs = 1;
    printf("applying ventricular xform %s\n", xform_fname);
    break ;
  case 'O':
    strcpy(orig_name, argv[2]) ;
    nargs = 1 ;
    printf("reading original vertex positions from %s\n", orig_name);
    break ;
  case 'Q':
    parms.flags |= IPFLAG_NO_SELF_INT_TEST ;
    printf(
            "doing quick (no self-intersection) surface positioning.\n") ;
    break ;
  case 'P':
    plot_stuff = 1 ;
    printf("plotting intensity profiles...\n") ;
    break ;
  case 'A':
    max_averages = atoi(argv[2]) ;
    printf("using max_averages = %d\n", max_averages) ;
    nargs = 1 ;
    if (isdigit(*argv[3]))
    {
      min_averages = atoi(argv[3]) ;
      printf("using min_averages = %d\n", min_averages) ;
      nargs++ ;
    }
    break ;
  case 'M':
    parms.integration_type = INTEGRATE_MOMENTUM ;
    parms.momentum = atof(argv[2]) ;
    nargs = 1 ;
    printf("momentum = %2.2f\n", parms.momentum) ;
    break ;
  case 'R':
    parms.l_surf_repulse = atof(argv[2]) ;
    printf("l_surf_repulse = %2.3f\n", parms.l_surf_repulse) ;
    nargs = 1 ;
    break ;
  case 'B':
    base_dt_scale = atof(argv[2]) ;
    parms.base_dt = base_dt_scale*parms.dt ;
    nargs = 1;
    break ;
  case 'V':
    Gdiag_no = atoi(argv[2]) ;
    printf("printing diagnostic info for vertex %d\n", Gdiag_no) ;
    nargs = 1 ;
    break ;
  case 'C':
    create = !create ;
    printf("%screating area and curvature files for wm surface...\n",
            create ? "" : "not ") ;
    break ;
  case 'W':
    sscanf(argv[2], "%d", &parms.write_iterations) ;
    nargs = 1 ;
    printf("write iterations = %d\n", parms.write_iterations) ;
    Gdiag |= DIAG_WRITE ;
    break ;
  case 'N':
    sscanf(argv[2], "%d", &parms.niterations) ;
    nargs = 1 ;
    printf("niterations = %d\n", parms.niterations) ;
    break ;
  default:
    printf("unknown option %s\n", argv[1]) ;
    print_help() ;
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
  printf("usage: %s [options] <subject name> <hemisphere> <xform> <flash 1> <flash 2> .. <residuals>\n", 
          Progname) ;
}

static void
print_help(void)
{
  print_usage() ;
  printf(
       "\nThis program positions the tessellation of the cortical surface\n"
          "at the white matter surface, then the gray matter surface\n"
          "and generate surface files for these surfaces as well as a\n"
          "'curvature' file for the cortical thickness, and a surface file\n"
          "which approximates layer IV of the cortical sheet.\n");
  printf("\nvalid options are:\n\n") ;
  printf(
          "-q    omit self-intersection and only generate "
          "gray/white surface.\n") ;
  printf(
          "-c    create curvature and area files from white matter surface\n"
          );
  printf(
          "-a <avgs>   average curvature values <avgs> times (default=10)\n");
  printf(
          "-whiteonly  only generate white matter surface\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  printf("%s\n", vcid) ;
  exit(1) ;
}

static int
mrisFindMiddleOfGray(MRI_SURFACE *mris)
{
  int     vno ;
  VERTEX  *v ;
  float   nx, ny, nz, thickness ;

  MRISaverageCurvatures(mris, 3) ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    nx = v->nx ; ny = v->ny ; nz = v->nz ;
    thickness = 0.5 * v->curv ;
    v->x = v->origx + thickness * nx ;
    v->y = v->origy + thickness * ny ;
    v->z = v->origz + thickness * nz ;
  }
  return(NO_ERROR) ;
}

#define MAX_DEFORM_DIST  3
static int
ms_errfunc_gradient(MRI_SURFACE *mris, INTEGRATION_PARMS *parms)
{
  double      nx, ny, nz, lambda, white_delta, pial_delta, len ;
  int         vno, white_vno, pial_vno ;
  VERTEX      *v, *v_pial, *v_white ;
  EXTRA_PARMS *ep ;

  ep = (EXTRA_PARMS *)parms->user_parms ;
  lambda = parms->l_external ;
#if 0
  compute_maximal_distances(mris, ep->current_sigma, ep->mri_flash, ep->nvolumes, 
                            ep->cv_inward_dists, ep->cv_outward_dists,
                            ep->nearest_pial_vertices, ep->nearest_white_vertices, 
                            ep->dstep, ep->max_inward_dist, ep->max_outward_dist) ;
#endif
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
#if 0
    if ((vno+1) % (mris->nvertices/10) == 0)
      printf("%d ", vno) ;
#endif
    if (vno > MAX_VNO)
      break ;
    v = &mris->vertices[vno] ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    if (v->ripflag)
      continue ;

    compute_optimal_vertex_positions(mris, vno, ep, &white_delta, &pial_delta) ;

    white_vno = vno ;
#if 0
    pial_vno = ep->nearest_pial_vertices[vno] ;
#else
    pial_vno = vno ;
#endif


    v_white = &mris->vertices[white_vno] ;
    v_pial = &mris->vertices[pial_vno] ;
    nx = v_pial->pialx - v_white->origx ;
    ny = v_pial->pialy - v_white->origy ;
    nz = v_pial->pialz - v_white->origz ;
    len = sqrt(nx*nx + ny*ny + nz*nz) ; 
    if (FZERO(len))
    {
      nx = v_white->nx ; ny = v_white->ny ; nz = v_white->nz ; 
      len = sqrt(nx*nx + ny*ny + nz*nz) ; 
      if (FZERO(len))
        len = 0.01 ;
    }
    nx /= len ; ny /= len ; nz /= len ;
    if (Gdiag_no == white_vno || Gdiag_no == pial_vno)
    {
      printf("v %d,%d: delta = %2.2f, %2.2f along n = (%2.2f, %2.2f, %2.2f)\n",
             white_vno, pial_vno, lambda*white_delta, lambda*pial_delta, nx, ny, nz) ;
      printf("      current location   = (%2.2f, %2.2f, %2.2f) --> (%2.2f, %2.2f, %2.2f)\n",
             v_white->origx, v_white->origy, v_white->origz,
             v_pial->pialx, v_pial->pialy, v_pial->pialz);
      printf("      predicted location = (%2.2f, %2.2f, %2.2f) --> (%2.2f, %2.2f, %2.2f)\n",
             v_white->origx+lambda * white_delta * nx, 
             v_white->origy+lambda * white_delta * ny,
             v_white->origz+lambda * white_delta * nz,
             v_pial->pialx+lambda * pial_delta * nx, 
             v_pial->pialy+lambda * pial_delta * ny,
             v_pial->pialz+lambda * pial_delta * nz) ;
             
    }
    v_white->dx += lambda * white_delta * nx ;
    v_white->dy += lambda * white_delta * ny ;
    v_white->dz += lambda * white_delta * nz ;

    mris->dx2[pial_vno] += lambda * pial_delta * nx ;
    mris->dy2[pial_vno] += lambda * pial_delta * ny ;
    mris->dz2[pial_vno] += lambda * pial_delta * nz ;
    if (white_vno == Gdiag_no)
      printf("v %d white intensity   :  (%2.3f, %2.3f, %2.3f)\n",
             vno, lambda * white_delta * nx, lambda * white_delta * ny, 
             lambda * white_delta * nz) ;
    if (pial_vno == Gdiag_no)
      printf("v %d pial intensity    :  (%2.3f, %2.3f, %2.3f)\n",
             vno, lambda * pial_delta * nx, lambda * pial_delta * ny, 
             lambda * pial_delta * nz) ;
  }

#if 0
  printf("\n") ;
#endif
  return(NO_ERROR) ;
}
static int
ms_errfunc_timestep(MRI_SURFACE *mris, INTEGRATION_PARMS *parms)
{
  EXTRA_PARMS *ep ;

  ep = (EXTRA_PARMS *)parms->user_parms ;
  compute_maximal_distances(mris, ep->current_sigma, ep->mri_flash, 
                            ep->nvolumes, 
                            ep->cv_inward_dists, ep->cv_outward_dists,
                            ep->nearest_pial_vertices, ep->
                            nearest_white_vertices, ep->dstep, ep->max_inward_dist, ep->max_outward_dist) ;
  if (ep->nvertices < mris->nvertices)
  {
    int vno ;
    printf("resampling parameters for %d new vertices...\n", mris->nvertices-ep->nvertices) ;
    for (vno = 0 ; vno < ep->nvertices ; vno++)
      mris->vertices[vno].marked = 1 ;  /* T1s and PDs are up to date */
    sample_parameter_maps(mris, mri_T1, mri_PD, ep->cv_wm_T1, ep->cv_wm_PD,
                          ep->cv_gm_T1, ep->cv_gm_PD, ep->cv_csf_T1, ep->cv_csf_PD,
                          ep->cv_inward_dists, ep->cv_outward_dists, ep, fix_T1, parms, 
                          ep->nvertices) ;
    ep->nvertices = mris->nvertices ;
    
  }
  return(NO_ERROR) ;
}

static double last_sse[300000];
static double
ms_errfunc_sse(MRI_SURFACE *mris, INTEGRATION_PARMS *parms)
{
  double      sse, total_sse ;
  int         vno ;
  VERTEX      *v ;
  EXTRA_PARMS *ep ;

  ep = (EXTRA_PARMS *)parms->user_parms ;
#if 0
  compute_maximal_distances(mris, ep->current_sigma, ep->mri_flash, ep->nvolumes, 
                            ep->cv_inward_dists, ep->cv_outward_dists,
                            ep->nearest_pial_vertices, ep->nearest_white_vertices,ep->dstep,
                            ep->max_inward_dist, ep->max_outward_dist);
#endif 
  total_sse = 0.0 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    if (vno > MAX_VNO)
      break ;
    if (v->ripflag)
      continue ;
    sse = vertex_error(mris, vno, ep, NULL) ;
    if (!finite(sse))
      DiagBreak() ;
    total_sse += sse ;
    if (Gdiag_no == vno)
      printf("v %d: sse = %2.2f\n", vno, sse) ;
    if (sse > last_sse[vno] && !FZERO(last_sse[vno]))
      DiagBreak() ;
    last_sse[vno] = sse ;
  }

  return(total_sse) ;
}

static double
ms_errfunc_rms(MRI_SURFACE *mris, INTEGRATION_PARMS *parms)
{
  double      rms, rms_total ;
  int         vno ;
  VERTEX      *v ;
  EXTRA_PARMS *ep ;

  ep = (EXTRA_PARMS *)parms->user_parms ;
  rms_total = 0.0 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    if (vno > MAX_VNO)
      break ;
    if (v->ripflag)
      continue ;
    vertex_error(mris, vno, ep, &rms) ;
    rms_total += rms ;
    if (!finite(rms))
      DiagBreak() ;
    if (Gdiag_no == vno)
      printf("v %d: rms = %2.3f\n", vno, rms) ;
  }

  return(rms_total/(double)vno) ;
}


static double
vertex_error(MRI_SURFACE *mris, int vno, EXTRA_PARMS *ep, double *prms)
{
  VERTEX  *v_white, *v_pial ;
  double  dist, dx, dy, dz, cortical_dist, sigma,
          T1_wm, T1_gm, T1_csf, PD_wm, PD_gm, PD_csf, sse, inward_dist, outward_dist ;
  Real    xp, yp, zp, xw, yw, zw, x, y, z,
           image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES] ;
  MRI     *mri ;
  int     i, j, max_j ;

  v_white = &mris->vertices[vno] ;
#if 0
  v_pial = &mris->vertices[ep->nearest_pial_vertices[vno]] ;
#else
  v_pial = v_white ;
#endif

  sigma = ep->current_sigma ;
  sse = 0.0 ;
  T1_wm = ep->cv_wm_T1[vno] ; PD_wm = ep->cv_wm_PD[vno] ;
  T1_gm = ep->cv_gm_T1[vno] ; PD_gm = ep->cv_gm_PD[vno] ;
  T1_csf = ep->cv_csf_T1[vno] ; PD_csf = ep->cv_csf_PD[vno] ;
  inward_dist = ep->cv_inward_dists[vno] ;
  outward_dist = ep->cv_outward_dists[vno] ;
  mri = ep->mri_flash[0] ;
  MRIworldToVoxel(mri, v_white->origx, v_white->origy, v_white->origz, 
                  &xw, &yw, &zw) ;
  MRIworldToVoxel(mri, v_pial->pialx, v_pial->pialy, v_pial->pialz,
                  &xp, &yp, &zp) ;

  dx = xp-xw ; dy = yp-yw ; dz = zp-zw ; 
  cortical_dist = sqrt(dx*dx + dy*dy + dz*dz) ;
  if (FZERO(cortical_dist))
  {
    cortical_dist = ep->dstep ;
    MRIworldToVoxel(mri, 
                    v_pial->pialx+v_pial->nx, 
                    v_pial->pialy+v_pial->nx, 
                    v_pial->pialz+v_pial->nz,
                    &xp, &yp, &zp) ;

    dx = xp-xw ; dy = yp-yw ; dz = zp-zw ; 
    dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    if (!FZERO(dist))
    { dx /= dist ; dy /= dist ; dz /= dist ; }
  }
  else
  {
    dx /= cortical_dist ; dy /= cortical_dist ; dz /= cortical_dist ; 
  }
  
  max_j = (int)((inward_dist + cortical_dist + outward_dist) / ep->dstep) ;


  for (i = 0 ; i < ep->nvolumes ; i++)
  {
    mri = ep->mri_flash[i] ;
    for (j = 0, dist = -inward_dist ; dist <= cortical_dist+outward_dist ; dist += ep->dstep, j++)
    {
      x = xw + dist * dx ; y = yw + dist * dy ; z = zw + dist * dz ;
#if 0
      MRIsampleVolumeDirectionScale(mri, x, y, z, dx, dy, dz, &image_vals[i][j], sigma) ;
#else
      MRIsampleVolumeType(mri, x, y, z, &image_vals[i][j], sample_type) ;
#endif
      if (!finite(image_vals[i][j]))
        DiagBreak() ;
    }

    if ((plot_stuff == 1 && Gdiag_no == vno) ||
        plot_stuff > 1)
    {
      FILE *fp ;
      char fname[STRLEN] ;
      
      sprintf(fname, "sse_vol%d.plt", i) ;
      fp = fopen(fname, "w") ;
      for (j = 0, dist = -inward_dist ; dist <= cortical_dist+outward_dist ; dist += ep->dstep, j++)
        fprintf(fp, "%d %f %f\n", j, dist, image_vals[i][j]) ;
      fclose(fp) ;
    }
  }
  sse = compute_vertex_sse(ep, image_vals, max_j, inward_dist, cortical_dist, 
                           T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf, plot_stuff && vno == Gdiag_no) ;
  if (prms)
    *prms = sqrt(sse) ;

  return(sse) ;
}


#if 0
#define MAX_TABLES     10
static float  TRs[MAX_TABLES] ;
static double *exp_minus_TR_div_T1[MAX_TABLES] ;

static int
init_lookup_table(MRI *mri)
{
  int    i ;
  double TR = mri->tr ;
  double T1 ;

  for (i = 0 ; i < MAX_TABLES ; i++)
    if ((NULL != exp_minus_TR_div_T1[i]) && FEQUAL(TRs[i], TR))
      return(NO_ERROR) ;   /* table alread created */

  for (i = 0 ; i < MAX_TABLES ; i++)
    if (NULL == exp_minus_TR_div_T1[i])
      break ;
  if (i >= MAX_TABLES)
    return(NO_ERROR) ;    /* can't fit it */

  TRs[i] = TR ;
  exp_minus_TR_div_T1[i] = (double *)calloc(MAX_T1, sizeof(double)) ;
  if (!exp_minus_TR_div_T1[i])
    ErrorExit(ERROR_NOMEMORY, "init_lookup_table(%2.1f) - could not alloc table",
              TR) ;

  for (T1 = 0.0 ; T1 < MAX_T1 ; T1 += 1.0)
    exp_minus_TR_div_T1[i][(int)T1] = exp(-TR/T1) ;

  printf("creating lookup table for TR=%2.1f ms\n", (float)TR) ;

  return(NO_ERROR) ;
}

double *
find_lookup_table(double TR)
{
  int i ;

  for (i = 0 ; i < MAX_TABLES ; i++)
    if ((NULL != exp_minus_TR_div_T1[i]) && FEQUAL(TRs[i], TR))
      return(exp_minus_TR_div_T1[i]) ;

  return(NULL) ;
}
#endif

static double
FLASHforwardModel(double flip_angle, double TR, double PD, double T1)
{
  static double  CFA = 1, SFA = 0 ;
  static double last_flip_angle = -1 ;
  double  E1, FLASH ;

  if (!DZERO(flip_angle-last_flip_angle))
  {
    CFA = cos(flip_angle) ; SFA = sin(flip_angle) ;
    last_flip_angle = flip_angle ;
  }

  E1 = exp(-TR/T1) ;
      
  FLASH = PD * SFA ;
  if (!DZERO(T1))
    FLASH *= (1-E1)/(1-CFA*E1);
  return(FLASH) ;
}

#if 1

static double
FLASHforwardModelLookup(double flip_angle, double TR, double PD, double T1)
{
  double FLASH ;

  FLASH = lookup_flash_value(TR, flip_angle, PD, T1) ;
  return(FLASH) ;
}
#else
static double
FLASHforwardModelLookup(double flip_angle, double TR, double PD, double T1)
{
  double FLASH, E1, *table ;
  static double  CFA = 1, SFA = 0 ;
  static double last_flip_angle = -1 ;

  if (!DZERO(flip_angle-last_flip_angle))
  {
    CFA = cos(flip_angle) ; SFA = sin(flip_angle) ;
    last_flip_angle = flip_angle ;
  }

  table = find_lookup_table(TR) ;
  if (NULL == table)
    E1 = exp(-TR/T1) ;
  else
    E1 = table[nint(T1)] ;
      
  FLASH = PD * SFA ;
  if (!DZERO(T1))
    FLASH *= (1-E1)/(1-CFA*E1);
  return(FLASH) ;
}
#endif

#if 0
static double
dM_dT1(double flip_angle, double TR, double PD, double T1)
{
  double  dT1, E1 ;
  static double  CFA, SFA2_3, CFA2 ;


  CFA = cos(flip_angle) ;
  SFA2_3 = sin(flip_angle/2) ;
  SFA2_3 = SFA2_3*SFA2_3*SFA2_3 ;
  CFA2 = cos(flip_angle/2) ;
  E1 = exp(TR/T1) ;

  dT1 = -4*E1*PD*TR*CFA2*SFA2_3/ (T1*T1*pow(-E1 + CFA,2)) ;

  return(dT1) ;
}

static double
dM_dPD(double flip_angle, double TR, double PD, double T1)
{
  double  dPD, E1 ;
  static double  CFA, SFA ;

  CFA = cos(flip_angle) ;
  SFA = sin(flip_angle) ;
  E1 = exp(TR/T1) ;
  dPD = (-1 + E1)*SFA/(E1 - CFA) ;

  return(dPD) ;
}
#endif

static int
compute_maximal_distances(MRI_SURFACE *mris, float sigma, MRI **mri_flash, int nvolumes, 
                          float *cv_inward_dists, float *cv_outward_dists,
                          int *nearest_pial_vertices, int *nearest_white_vertices,
                          double dstep, double max_inward_dist, double max_outward_dist)
{
  int    vno, i, found_csf, j, max_j = 0, found_wm, wm_dist, csf_dist, min_j ;
  double T1, PD ;
  VERTEX *v_white, *v_pial ;
  MRI    *mri ;
  float  nx, ny, nz, min_inward_dist, min_outward_dist, dist,
         min_parm_dist, parm_dist ;
  Real   xw, yw, zw, xp, yp, zp, xo, yo, zo, cortical_dist,
         image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES] ;
  Real   mean_csf[MAX_FLASH_VOLUMES], mean_wm[MAX_FLASH_VOLUMES] ; ;


  nx = ny = nz = 0 ;   /* remove compiler warning */
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v_white = &mris->vertices[vno] ;
#if 0
    v_pial = &mris->vertices[nearest_pial_vertices[vno]] ;
#else
    v_pial = v_white ;
#endif
    if (vno == Gdiag_no)
      DiagBreak() ;
    if (vno > MAX_VNO)
      break ;
    
    min_inward_dist = max_inward_dist ;
    max_j = (max_inward_dist-dstep) / dstep ;
    for (i = 0 ; i < nvolumes ; i++)
    {
      mri = mri_flash[i] ;
      MRIworldToVoxel(mri, v_white->origx, v_white->origy, v_white->origz, &xw, &yw, &zw) ;
      MRIworldToVoxel(mri, v_pial->pialx, v_pial->pialy, v_pial->pialz, &xp, &yp, &zp) ;
      nx = xp - xw ; ny = yp - yw ; nz = zp - zw ; 
      dist = sqrt(nx*nx + ny*ny + nz*nz) ; 
      if (FZERO(dist))
      {
        MRIworldToVoxel(mri, 
                        v_pial->pialx+v_white->nx, 
                        v_pial->pialy+v_white->ny, 
                        v_pial->pialz+v_white->nz, &xp, &yp, &zp) ;
        nx = xp - xw ; ny = yp - yw ; nz = zp - zw ; 
        dist = sqrt(nx*nx + ny*ny + nz*nz) ; 
      }
      if (dist < 0.0001) dist = 0.001 ;
      nx /= dist ; ny /= dist ; nz /= dist ;

      /* first search inwards */
      for (j = 0, dist = dstep ; j <= max_j ; dist += dstep, j++)
      {
        xo = xw-dist*nx ; yo = yw-dist*ny ; zo = zw-dist*nz ;
        MRIsampleVolumeType(mri, xo, yo, zo, &image_vals[i][j], sample_type) ;
      }
      dist -= dstep ; dist = MAX(dist, dstep) ;
      if (min_inward_dist > dist)
        min_inward_dist = dist ;
    }
    /* search to see if T1/PD pairs are reasonable for CSF */
    wm_dist = 0 ;
    for (j = found_wm = 0 ; j <= max_j ; j++)
    {
      for (i = 0 ; i < nvolumes ; i++)
        mean_wm[i] = image_vals[i][j] ;
      compute_T1_PD(mean_wm, mri_flash, nvolumes, &T1, &PD) ;
      if (found_wm)
      {
        if (!IS_WM(T1,PD))
          break ;
#if 0
        if (++wm_dist >= 5)
          break ;
#endif
      }
      else if (IS_WM(T1,PD))
        found_wm = 1 ;
    }
    if (found_wm == 0)
    {
      wm_dist = 0 ; min_parm_dist = MAX_T1*MAX_T1 ; min_j = 0 ;
      for (j = 0 ; j <= max_j ; j++)
      {
        for (i = 0 ; i < nvolumes ; i++)
          mean_wm[i] = image_vals[i][j] ;
        compute_T1_PD(mean_wm, mri_flash, nvolumes, &T1, &PD) ;
        parm_dist = WM_PARM_DIST(T1,PD) ;
        if (parm_dist < min_parm_dist)
        {
          min_parm_dist = parm_dist ;
          min_j = j ;
        }
      }
      j = min_j+1 ;
    }

    min_inward_dist = ((float)j)*dstep ;
    cv_inward_dists[vno] = min_inward_dist ;
    if (vno == Gdiag_no)
      printf("v %d: inward_dist = %2.2f, X=(%2.1f, %2.1f, %2.1f)\n", 
             vno, min_inward_dist,v_white->origx,v_white->origy,v_white->origz);
  }

  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v_pial = &mris->vertices[vno] ;
    if (vno == Gdiag_no)
      DiagBreak() ;
#if 0
    v_white = &mris->vertices[nearest_white_vertices[vno]] ;
#else
    v_white = v_pial ;
#endif

    found_csf = 0 ;
    min_outward_dist = max_outward_dist ;
    max_j = (max_outward_dist-dstep) / dstep ;
    for (i = 0 ; i < nvolumes ; i++)
    {
      mri = mri_flash[i] ;
      MRIworldToVoxel(mri, v_white->origx, v_white->origy, v_white->origz, &xw, &yw, &zw) ;
      MRIworldToVoxel(mri, v_pial->pialx, v_pial->pialy, v_pial->pialz, &xp, &yp, &zp) ;
      nx = xp - xw ; ny = yp - yw ; nz = zp - zw ; 
      cortical_dist = dist = sqrt(nx*nx + ny*ny + nz*nz) ; 
      if (FZERO(dist))
      {
        MRIworldToVoxel(mri, 
                        v_pial->pialx+v_white->nx, 
                        v_pial->pialy+v_white->ny, 
                        v_pial->pialz+v_white->nz, &xp, &yp, &zp) ;
        nx = xp - xw ; ny = yp - yw ; nz = zp - zw ; 
        dist = sqrt(nx*nx + ny*ny + nz*nz) ; 
        xp = xw ; yp = yw ; zp = zw ;   /* true surface position */
      }

      if (dist < 0.0001) dist = 0.001 ;
      nx /= dist ; ny /= dist ; nz /= dist ;

      /* search outwards - match sampling of compute_optimal_parameters */
      dist = (int)(cortical_dist/dstep)*dstep + dstep ;
      for (j = 0 ; j <= max_j ; dist += dstep, j++)
      {
        xo = xw+dist*nx ; yo = yw+dist*ny ; zo = zw+dist*nz ;
        MRIsampleVolumeType(mri, xo, yo, zo, &image_vals[i][j], sample_type) ;
      }
      dist -= dstep ; dist = MAX(dstep, dist) ;
      if (min_outward_dist > dist)
        min_outward_dist = dist ;
    }

    /* search to see if T1/PD pairs are reasonable for CSF */
    csf_dist = 0 ;
    for (j = found_csf = 0 ; j <= max_j ; j++)
    {
      for (i = 0 ; i < nvolumes ; i++)
        mean_csf[i] = image_vals[i][j] ;
      compute_T1_PD(mean_csf, mri_flash, nvolumes, &T1, &PD) ;
      if (found_csf)
      {
        if (!IS_CSF(T1,PD))
          break ;
        if (++csf_dist >= 5)
          break ;
      }
      else if (IS_CSF(T1,PD))
        found_csf = 1 ;
    }
    if (found_csf)
      min_outward_dist = ((float)j)*dstep ;
    else
      min_outward_dist = dstep ;
    cv_outward_dists[vno] = min_outward_dist ; /* j=0 is dist=dstep */
    if (vno == Gdiag_no)
      printf("v %d: outward_dist = %2.2f, X=(%2.1f, %2.1f, %2.1f), N=(%2.1f, %2.1f, %2.1f)\n", 
             vno, min_outward_dist, v_pial->pialx, v_pial->pialy, v_pial->pialz, nx, ny, nz);
  }

  return(NO_ERROR) ;
}
#define MIN_CSF   2000
#define CSF_T1    3000
#define WM_T1     750
#define GM_T1     1050

#define WM_T1_STD 100
#define GM_T1_STD 100

#define CSF_PD    1500
#define WM_PD     700
#define GM_PD     1050

#define BIN_SIZE 20
#define MAX_PARM (BIN_SIZE*MAX_BINS)

static int
sample_parameter_maps(MRI_SURFACE *mris, MRI *mri_T1, MRI *mri_PD, 
                      float *cv_wm_T1, float *cv_wm_PD,
                      float *cv_gm_T1, float *cv_gm_PD,
                      float *cv_csf_T1, float *cv_csf_PD,
                      float *cv_inward_dists, float *cv_outward_dists,
                      EXTRA_PARMS *ep, int fix_T1, INTEGRATION_PARMS *parms, 
                      int start_vno)
{
  VERTEX    *v ;
  int       vno, nholes =0/*, bno*/ ;
  double    /*inward_dist, outward_dist,*/ dstep,
             best_white_delta = 0, best_pial_delta = 0, sse ;

#if 1

  if (fix_T1)
  {
    int vno ;
    
    for (vno = 0 ; vno < mris->nvertices ; vno++)
    {
      cv_gm_T1[vno] = GM_T1 ; cv_wm_T1[vno] = WM_T1 ;
    }
  }

#if 0
  /* compute csf parameters */
  sample_parameter_map(mris, mri_T1, mri_res, cv_csf_T1, cv_outward_dists, 1, 
                       PIAL_VERTICES, "csf T1", NULL, ep->dstep, ep->max_dist);
  sample_parameter_map(mris, mri_PD, mri_res, cv_csf_PD, cv_outward_dists, 1, 
                       PIAL_VERTICES, "csf PD", NULL, ep->dstep, ep->max_dist);
#endif
#if 0
  MRISclearFixedValFlags(mris) ;
  vno = MRISsubsampleDist(mris, subsample_dist) ; 
  printf("computing optimal parameters for %d vertices...\n", vno) ;
  dstep = ep->dstep ; ep->dstep = 0.25 ;
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (FEQUAL(v->val, 1))
      v->fixedval = TRUE ;
  }
#endif

  for (nholes = 0, vno = start_vno ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    if (((vno+1) % (mris->nvertices/25)) == 0)
    {
      printf("%2.0f%% ", 100.0f*(float)(vno+1)/(float)(mris->nvertices)) ;
      fflush(stdout) ;
    }
#if 0
    if (v->fixedval != TRUE )  /* not in subsampled surface */
      continue ;
#endif


    dstep = ep->dstep ; ep->dstep = 0.5 ;
    sse = compute_optimal_parameters(mris, vno, ep, &best_white_delta, 
                                     &best_pial_delta) ;
    if (sse < 0)
    {
      v->marked = 0 ;
      nholes++ ;
    }
    else
      v->marked = 1 ;
    ep->dstep = dstep ;
    if (vno == Gdiag_no)
    {
      printf("v %d: wm = (%2.0f, %2.0f), gm = (%2.0f, %2.0f), "
             "csf = (%2.0f, %2.0f), "
             "deltas = (%2.2f, %2.2f)\n",
             vno,ep->cv_wm_T1[vno], ep->cv_wm_PD[vno], ep->cv_gm_T1[vno], 
             ep->cv_gm_PD[vno], ep->cv_csf_T1[vno], 
             ep->cv_csf_PD[vno], 
             best_white_delta, best_pial_delta);
      DiagBreak() ;
    }
  }
  printf("\nperforming soap bubble smoothing to fill %d holes...\n",nholes) ;
  soap_bubble_maps(mris, ep, 25) ;  /* fill holes in parameter maps */
  MRISclearMarks(mris) ;
#if 0
  ep->dstep = dstep ;
  MRIScopyFixedValFlagsToMarks(mris) ;
  soap_bubble_maps(mris, ep, 10*subsample_dist*subsample_dist) ;
  MRISclearMarks(mris) ;
#endif

#else
  HISTOGRAM *h_prior, *h_tmp ;

  h_prior = HISTOalloc(MAX_BINS) ;
  h_tmp = HISTOalloc(MAX_BINS) ;
  if (mri_T1)
  {
    if (fix_T1)
    {
      int vno ;

      for (vno = 0 ; vno < mris->nvertices ; vno++)
      {
        cv_gm_T1[vno] = GM_T1 ; cv_wm_T1[vno] = WM_T1 ;
      }
    }
    else
    {
      for (bno = 0 ; bno < MAX_BINS ; bno++)
        h_tmp->bins[bno] = BIN_SIZE*bno+0.5*BIN_SIZE ;
      
      for (bno = 0 ; bno < MAX_BINS ; bno++)
        if ((h_tmp->bins[bno] > WM_T1-WM_T1_STD) && (h_tmp->bins[bno] < WM_T1+WM_T1_STD))
          h_tmp->counts[bno] = 1 ;
      h_prior = HISTOsmooth(h_tmp, NULL, WM_T1_STD/BIN_SIZE) ;
      sample_parameter_map(mris, mri_T1, mri_res, cv_wm_T1, cv_inward_dists, -1, 
                           ORIGINAL_VERTICES, "wm T1", h_prior, ep->dstep, ep->max_dist);
      HISTOclearCounts(h_tmp, h_tmp) ;
      for (bno = 0 ; bno < MAX_BINS ; bno++)
        if ((h_tmp->bins[bno] > GM_T1-GM_T1_STD) && (h_tmp->bins[bno] < GM_T1+GM_T1_STD))
          h_tmp->counts[bno] = 1 ;
      HISTOsmooth(h_tmp, h_prior, GM_T1_STD/BIN_SIZE) ;
      sample_parameter_map(mris, mri_T1, mri_res, cv_gm_T1, NULL, 1, 
                           ORIGINAL_VERTICES, "gm T1", h_prior, ep->dstep, ep->max_dist);
    }
    sample_parameter_map(mris, mri_T1, mri_res, cv_csf_T1, cv_outward_dists, 1, 
                         PIAL_VERTICES, "csf T1", NULL, ep->dstep, ep->max_dist);
  }
  else
  {
    for (vno = 0 ; vno < mris->nvertices ; vno++)
    {
      v = &mris->vertices[vno] ;
      if (v->ripflag)
        continue ;
      cv_gm_T1[vno] = GM_T1 ;
      cv_wm_T1[vno] = WM_T1 ;
      cv_csf_T1[vno] = CSF_T1 ;
    }
  }
  if (mri_PD)
  {
    sample_parameter_map(mris, mri_PD, mri_res, cv_wm_PD, cv_inward_dists, -1, 
                         ORIGINAL_VERTICES, "wm PD", NULL, ep->dstep, ep->max_dist);
    sample_parameter_map(mris, mri_PD, mri_res, cv_gm_PD, NULL, 1, 
                         ORIGINAL_VERTICES, "gm PD", NULL, ep->dstep, ep->max_dist);
    sample_parameter_map(mris, mri_PD, mri_res, cv_csf_PD, cv_outward_dists, 1, 
                         PIAL_VERTICES, "csf PD", NULL, ep->dstep, ep->max_dist);
  }
  else
  {
    for (vno = 0 ; vno < mris->nvertices ; vno++)
    {
      v = &mris->vertices[vno] ;
      if (v->ripflag)
        continue ;
      cv_gm_PD[vno] = GM_PD ;
      cv_wm_PD[vno] = WM_PD ;
      cv_csf_PD[vno] = CSF_PD ;
    }
  }


  HISTOfree(&h_tmp) ; HISTOfree(&h_prior) ;
#endif
  return(NO_ERROR) ;
}

#if 1
static int
soap_bubble_maps(MRI_SURFACE *mris, EXTRA_PARMS *ep, int smooth_parms)
{
  soap_bubble_map(mris, ep->cv_wm_T1, smooth_parms) ;
  soap_bubble_map(mris, ep->cv_wm_PD, smooth_parms) ;
  soap_bubble_map(mris, ep->cv_gm_T1, smooth_parms) ;
  soap_bubble_map(mris, ep->cv_gm_PD, smooth_parms) ;
#if 0
  soap_bubble_map(mris, ep->cv_csf_T1, smooth_parms) ;
  soap_bubble_map(mris, ep->cv_csf_PD, smooth_parms) ;
#endif
  return(NO_ERROR) ;
}


static int
soap_bubble_map(MRI_SURFACE *mris, float *cv, int navgs)
{
  MRISimportValVector(mris, cv) ;
  MRISsoapBubbleVals(mris, navgs) ;
  MRISexportValVector(mris, cv) ;
  return(NO_ERROR) ;
}
#endif

static int
smooth_map(MRI_SURFACE *mris, float *cv, int navgs)
{
  MRISimportCurvatureVector(mris, cv) ;
  MRISaverageCurvatures(mris, navgs) ;
  MRISextractCurvatureVector(mris, cv) ;
  return(NO_ERROR) ;
}

static int
write_map(MRI_SURFACE *mris, float *cv, char *name, int suffix, char *output_suffix)
{
  char fname[STRLEN] ;

  if (suffix >= 0)
    sprintf(fname, "%s.%s_%d%s", 
            mris->hemisphere == LEFT_HEMISPHERE ? "lh" : "rh", name, suffix, output_suffix) ;
  else
    sprintf(fname, "%s.%s%s", 
            mris->hemisphere == LEFT_HEMISPHERE ? "lh" : "rh", name, output_suffix) ;
  if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    printf("writing vector to curvature file %s...\n",fname) ;
  MRISimportCurvatureVector(mris, cv) ;
  MRISwriteCurvature(mris, fname) ;
  return(NO_ERROR) ;
}

static int
smooth_maps(MRI_SURFACE *mris, EXTRA_PARMS *ep, int smooth_parms)
{
  smooth_map(mris, ep->cv_wm_T1, smooth_parms) ;
  smooth_map(mris, ep->cv_wm_PD, smooth_parms) ;
  smooth_map(mris, ep->cv_gm_T1, smooth_parms) ;
  smooth_map(mris, ep->cv_gm_PD, smooth_parms) ;
#if 1
  smooth_map(mris, ep->cv_csf_T1, smooth_parms) ;
  smooth_map(mris, ep->cv_csf_PD, smooth_parms) ;
#endif
  return(NO_ERROR) ;
}

static int
write_maps(MRI_SURFACE *mris, EXTRA_PARMS *ep, int suffix, char *output_suffix) 
{
  write_map(mris, ep->cv_inward_dists, "inward_dists", suffix, output_suffix) ;
  write_map(mris, ep->cv_outward_dists, "outward_dists", suffix, output_suffix) ;
  write_map(mris, ep->cv_wm_T1, "wm_T1", suffix, output_suffix) ;
  write_map(mris, ep->cv_wm_PD, "wm_PD", suffix, output_suffix) ;
  write_map(mris, ep->cv_gm_T1, "gm_T1", suffix, output_suffix) ;
  write_map(mris, ep->cv_gm_PD, "gm_PD", suffix, output_suffix) ;
  write_map(mris, ep->cv_csf_T1, "csf_T1", suffix, output_suffix) ;
  write_map(mris, ep->cv_csf_PD, "csf_PD", suffix, output_suffix) ;
  return(NO_ERROR) ;
}

#define HALF_VOXEL_SIZE 0.0005

static double 
image_forward_model(double TR, double flip_angle, double dist, double thickness, 
                    double T1_wm, double PD_wm, double T1_gm, double PD_gm, 
                    double T1_csf, double PD_csf)
{
  static double   predicted_white, predicted_gray, predicted_csf, last_TR=0, 
                  last_flip_angle=0, last_T1_wm, last_PD_wm, last_T1_gm, last_PD_gm, last_T1_csf,
                  last_PD_csf;
  double predicted_val, wt ;

  if (!FZERO(last_TR - TR) || !FZERO(last_flip_angle - flip_angle) ||
      !FZERO(last_T1_wm-T1_wm) || !FZERO(last_PD_wm-PD_wm) ||
      !FZERO(last_T1_gm-T1_gm) || !FZERO(last_PD_gm-PD_gm) ||
      !FZERO(last_T1_csf-T1_csf) || !FZERO(last_PD_csf-PD_csf))
  {
    last_flip_angle = flip_angle ; last_TR = TR ;
    last_T1_wm = T1_wm ; last_PD_wm = PD_wm ; 
    last_T1_gm = T1_gm ; last_PD_gm = PD_gm ; 
    last_T1_csf = T1_csf ; last_PD_csf = PD_csf ; 

    predicted_white = FLASHforwardModelLookup(flip_angle, TR, PD_wm, T1_wm) ;
    predicted_gray = FLASHforwardModelLookup(flip_angle, TR, PD_gm, T1_gm) ;
    predicted_csf = FLASHforwardModelLookup(flip_angle, TR, PD_csf, T1_csf) ;
  }

  if (dist < -HALF_VOXEL_SIZE)   /* in white matter */
    return(predicted_white) ;
  else if (dist > thickness+HALF_VOXEL_SIZE)
    return(predicted_csf) ;
  else if (dist > HALF_VOXEL_SIZE && dist < thickness-HALF_VOXEL_SIZE)
    return(predicted_gray) ;
  else if (dist <= HALF_VOXEL_SIZE)   /* mixure of white and gray */
  {
    wt = (HALF_VOXEL_SIZE - dist) / HALF_VOXEL_SIZE ;
    predicted_val = wt*predicted_white + (1.0-wt)*predicted_gray ;
  }
  else  /* mixture of gray and csf */
  {
    dist -= thickness ;
    wt = (HALF_VOXEL_SIZE - dist) / HALF_VOXEL_SIZE ;
    predicted_val = wt*predicted_gray + (1.0-wt)*predicted_csf ;
  }

  return(predicted_val) ;
}

static int
find_nearest_pial_vertices(MRI_SURFACE *mris, int *nearest_pial_vertices, 
                           int *nearest_white_vertices)
{
#if 1
  int   vno ;

  for (vno = 0 ; vno < mris->max_vertices ; vno++)
    nearest_pial_vertices[vno] = nearest_white_vertices[vno] = vno ;
#else
  int     vno, n, vlist[100000], vtotal, ns, i, vnum, nbr_count[100], min_n ;
  VERTEX  *v, *vn, *vn2 ;
  float   dx, dy, dz, dist, min_dist, nx, ny, nz, dot ;

  memset(nbr_count, 0, 100*sizeof(int)) ;

  /* pial vertex positions are gray matter, orig are white matter */
  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    nx = v->nx ; ny = v->ny ; nz = v->nz ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    dx = v->pialx - v->origx ; dy = v->pialy - v->origy ; dz = v->pialz - v->origz ; 
    min_dist = sqrt(dx*dx + dy*dy + dz*dz) ; min_n = 0 ;
    v->marked = 1 ; vtotal = 1 ; vlist[0] = vno ;
    nearest_pial_vertices[vno] = vno ;
    min_n = 0 ;
    for (ns = 1 ; ns <= nbhd_size ; ns++)
    {
      vnum = 0 ;  /* will be # of new neighbors added to list */
      for (i = 0 ; i < vtotal ; i++)
      {
        vn = &mris->vertices[vlist[i]] ;
        if (vn->ripflag)
          continue ;
        if (vn->marked && vn->marked < ns-1)
          continue ;
        for (n = 0 ; n < vn->vnum ; n++)
        {
          vn2 = &mris->vertices[vn->v[n]] ;
          if (vn2->ripflag || vn2->marked)  /* already processed */
            continue ;
          vlist[vtotal+vnum++] = vn->v[n] ;
          vn2->marked = ns ;
          dx = vn2->pialx-v->origx ; dy = vn2->pialy-v->origy ; dz = vn2->pialz-v->origz ;
          dot = dx*nx + dy*ny + dz*nz ;
          if (dot < 0) /* must be outwards from surface */
            continue ;
          dot = vn2->nx*nx + vn2->ny*ny + vn2->nz*nz ;
          if (dot < 0) /* must be outwards from surface */
            continue ;
          dist = sqrt(dx*dx + dy*dy + dz*dz) ;
          if (dist < min_dist)
          {
            min_n = ns ;
            min_dist = dist ;
            nearest_pial_vertices[vno] = vn->v[n] ;
            if (min_n == nbhd_size && DIAG_VERBOSE_ON)
              fprintf(stdout, "%d --> %d = %2.3f\n",
                      vno,vn->v[n], dist) ;
          }
        }
      }
      vtotal += vnum ;
    }

    nearest_white_vertices[nearest_pial_vertices[vno]] = vno ;
    nbr_count[min_n]++ ;
    for (n = 0 ; n < vtotal ; n++)
    {
      vn = &mris->vertices[vlist[n]] ;
      if (vn->ripflag)
        continue ;
      vn->marked = 0 ;
    }
  }


  for (n = 0 ; n <= nbhd_size ; n++)
    printf("%d vertices at %d distance\n", nbr_count[n], n) ;
#endif
  return(NO_ERROR) ;
}
static int
find_nearest_white_vertices(MRI_SURFACE *mris, int *nearest_white_vertices)
{
  int     vno, n, vlist[100000], vtotal, ns, i, vnum, nbr_count[100], min_n, nfilled ;
  VERTEX  *v, *vn, *vn2 ;
  float   dx, dy, dz, dist, min_dist, nx, ny, nz, dot ;

  memset(nbr_count, 0, 100*sizeof(int)) ;

  /* pial vertex positions are gray matter, orig are white matter */
  for (nfilled = 0, vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (nearest_white_vertices[vno] >= 0)
      continue ;
    nfilled++ ;
    nx = v->nx ; ny = v->ny ; nz = v->nz ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    dx = v->origx - v->pialx ; dy = v->origy - v->pialy ; dz = v->origz - v->pialz ; 
    min_dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    nearest_white_vertices[vno] = vno ; min_n = 0 ;
    v->marked = 1 ; vtotal = 1 ; vlist[0] = vno ;
    min_n = 0 ;
    for (ns = 1 ; ns <= nbhd_size ; ns++)
    {
      vnum = 0 ;  /* will be # of new neighbors added to list */
      for (i = 0 ; i < vtotal ; i++)
      {
        vn = &mris->vertices[vlist[i]] ;
        if (vn->ripflag)
          continue ;
        if (vn->marked && vn->marked < ns-1)
          continue ;
        for (n = 0 ; n < vn->vnum ; n++)
        {
          vn2 = &mris->vertices[vn->v[n]] ;
          if (vn2->ripflag || vn2->marked)  /* already processed */
            continue ;
          vlist[vtotal+vnum++] = vn->v[n] ;
          vn2->marked = ns ;
          dx = vn2->origx-v->pialx ; dy = vn2->origy-v->pialy ; dz = vn2->origz-v->pialz ;
          dot = dx*nx + dy*ny + dz*nz ;
          if (dot < 0) /* must be outwards from surface */
            continue ;
          dot = vn2->nx*nx + vn2->ny*ny + vn2->nz*nz ;
          if (dot < 0) /* must be outwards from surface */
            continue ;
          dist = sqrt(dx*dx + dy*dy + dz*dz) ;
          if (dist < min_dist)
          {
            min_n = ns ;
            min_dist = dist ;
            nearest_white_vertices[vno] = vn->v[n] ;
            if (min_n == nbhd_size && DIAG_VERBOSE_ON)
              fprintf(stdout, "%d --> %d = %2.3f\n",
                      vno,vn->v[n], dist) ;
          }
        }
      }
      vtotal += vnum ;
    }

    nbr_count[min_n]++ ;
    for (n = 0 ; n < vtotal ; n++)
    {
      vn = &mris->vertices[vlist[n]] ;
      if (vn->ripflag)
        continue ;
      vn->marked = 0 ;
    }
  }

#if 0
  printf("%d holes filled in nearest wm vertex finding...\n", nfilled) ;
  for (n = 0 ; n <= nbhd_size ; n++)
    printf("%d vertices at %d distance\n", nbr_count[n], n) ;
#endif
  return(NO_ERROR) ;
}

static double
compute_optimal_parameters(MRI_SURFACE *mris, int vno, 
                           EXTRA_PARMS *ep, double *pwhite_delta, 
                           double *ppial_delta)
{
  double       predicted_val, dx, dy, dz,
               dist, inward_dist, outward_dist, cortical_dist, PD_wm, PD_gm, PD_csf,
               T1_wm, T1_gm, T1_csf, white_dist, pial_dist, sse, best_sse,
               best_white_dist, best_pial_dist, orig_cortical_dist, total_dist,
               orig_white_index, orig_pial_index, sigma, best_T1_wm, best_PD_wm,
               best_T1_gm, best_PD_gm, best_T1_csf, best_PD_csf, T1, PD  ;
  int          i, j, white_vno, pial_vno, max_j, best_white_index, csf_len,
               best_pial_index, pial_index, white_index, max_white_index, best_csf_len;
  VERTEX       *v_white, *v_pial ;
  MRI          *mri ;
  Real         image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES], xw, yw, zw, xp, yp, zp, 
               x, y, z,
               best_image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES],
               mean_white[MAX_FLASH_VOLUMES], mean_gray[MAX_FLASH_VOLUMES], 
               mean_csf[MAX_FLASH_VOLUMES];

  if (vno == Gdiag_no)
    DiagBreak() ;

  white_vno = vno ;
#if 0
  pial_vno = ep->nearest_pial_vertices[vno] ;
#else
  pial_vno = vno ;
#endif
  v_white = &mris->vertices[white_vno] ;
  v_pial = &mris->vertices[pial_vno] ;
#if 1
  inward_dist = ep->cv_inward_dists[white_vno] ;
  outward_dist = ep->cv_outward_dists[white_vno] ;
#else
  inward_dist = ep->max_inward_dist ;
  outward_dist = ep->max_outward_dist ;
#endif
  sigma = ep->current_sigma ;

  MRIworldToVoxel(ep->mri_flash[0], v_white->origx, v_white->origy,v_white->origz,
                  &xw,&yw,&zw);
  MRIworldToVoxel(ep->mri_flash[0], v_pial->pialx, v_pial->pialy,v_pial->pialz,
                  &xp,&yp,&zp);
  dx = xp - xw ; dy = yp - yw ; dz = zp - zw ; 
    orig_cortical_dist = cortical_dist = sqrt(dx*dx + dy*dy + dz*dz) ;
  if (FZERO(cortical_dist))
  {
    dx = v_pial->nx ; dy = v_pial->ny ; dz = v_pial->nz ;
    MRIworldToVoxel(ep->mri_flash[0], 
                    v_pial->pialx+v_pial->nx, 
                    v_pial->pialy+v_pial->ny,
                    v_pial->pialz+v_pial->nz,
                    &xp,&yp,&zp);
    dx = xp - xw ; dy = yp - yw ; dz = zp - zw ; 
    dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    if (FZERO(dist))
      dist = 1.0 ;
    dx /= dist ; dy /= dist ; dz /= dist ;
    xp = xw ; yp = yw ; zp = zw ;   /* true surface position */
  }
  else
  {
    /* surface normal in volume coords */
    dx /= cortical_dist ; dy /= cortical_dist ; dz /= cortical_dist ;  
  }

  T1_csf = ep->cv_csf_T1[white_vno] ; PD_csf = ep->cv_csf_PD[white_vno] ;

  /* fill image_vals[][] array */
  orig_white_index = inward_dist / ep->dstep ;
  orig_pial_index = orig_white_index + cortical_dist / ep->dstep ;
  max_j = (int)((inward_dist + cortical_dist + outward_dist) / ep->dstep) ;
  for (i = 0 ; i < ep->nvolumes ; i++)
  {
    mri = ep->mri_flash[i] ;
    for (j = 0, dist = -inward_dist ; dist <= cortical_dist+outward_dist ; 
         dist += ep->dstep, j++)
    {
      x = xw + dist * dx ; y = yw + dist * dy ; z = zw + dist * dz ;
#if 0
      MRIsampleVolumeDirectionScale(mri, x, y, z, dx, dy, dz, &image_vals[i][j], sigma) ;
#else
      MRIsampleVolumeType(mri, x, y, z, &image_vals[i][j], sample_type) ;
      if (j > orig_pial_index)
        DiagBreak() ;
      if (!finite(image_vals[i][j]))
        DiagBreak() ;
#endif
    }

    if ((plot_stuff == 1 && Gdiag_no == vno) ||
        plot_stuff > 1)
    {
      FILE *fp ;
      char fname[STRLEN] ;
      
      sprintf(fname, "vol%d.plt", i) ;
      fp = fopen(fname, "w") ;
      for (j = 0, dist = -inward_dist ; dist <= cortical_dist+outward_dist ; 
           dist += ep->dstep, j++)
        fprintf(fp, "%d %f %f\n", j, dist, image_vals[i][j]) ;
      fclose(fp) ;
    }
  }
  if ((plot_stuff == 1 && Gdiag_no == vno) ||
      plot_stuff > 1)
  {
    FILE *fp ;
    
    fp = fopen("orig_white.plt", "w") ;
    fprintf(fp, "%f %f %f\n%f %f %f\n",
            orig_white_index, 0.0, 0.0, orig_white_index, 0.0, 120.0) ;
    fclose(fp) ;
    fp = fopen("orig_pial.plt", "w") ;
    fprintf(fp, "%f %f %f\n%f %f %f\n",
            orig_pial_index, cortical_dist, 0.0, orig_pial_index, cortical_dist, 120.0) ;
    fclose(fp) ;
  }


  total_dist = outward_dist + orig_cortical_dist + inward_dist ;
  best_white_index = orig_white_index ; best_pial_index = orig_pial_index ;
  best_csf_len = outward_dist*ep->dstep ;
  best_white_dist = orig_white_index*ep->dstep; best_pial_dist = orig_pial_index*ep->dstep;
  max_white_index = nint((inward_dist + orig_cortical_dist/2)/ep->dstep) ;
  cortical_dist = best_pial_dist - best_white_dist ;
  best_sse = -1 ;
  best_T1_wm = WM_T1 ; best_PD_wm = WM_PD ;
  best_T1_gm = GM_T1 ; best_PD_gm = GM_PD ;
  best_T1_csf = CSF_T1 ; best_PD_csf = CSF_PD ;
  
  for (white_index = 0 ; white_index <= max_white_index ; white_index++)
  {
    white_dist = white_index * ep->dstep ;
    for (pial_index = white_index + nint(1.0/ep->dstep) ; pial_index <= max_j ;pial_index++)
    {
      /*
        for this pair of white/pial offsets, compute the mean wm,gm,csf vals
        and the T1/PD pairs that match them. Then compute the sse of the
        total and see if it is better than the best previous set of parms.
      */
      pial_dist = pial_index * ep->dstep ;
      cortical_dist = pial_dist - white_dist ;
      csf_len = 0 ;
      
      /* compute mean wm for these positions */
      if (white_index < 1)   /* just sample 1mm inwards */
      {
        for (i = 0 ; i < ep->nvolumes ; i++)
          mean_white[i] = image_vals[i][white_index] ;
        compute_T1_PD(mean_white, ep->mri_flash, ep->nvolumes, &T1_wm, &PD_wm);
      }
      else  /* average inwards up to 1/2 mm from white position */
      {
        for (T1_wm = PD_wm = 0.0, j = 0 ; j < white_index ; j++)
        {
          for (i = 0 ; i < ep->nvolumes ; i++)
            mean_white[i] = image_vals[i][j] ;
          compute_T1_PD(mean_white, ep->mri_flash, ep->nvolumes, &T1, &PD);
          if (T1 < MIN_WM_T1)
            T1 = MIN_WM_T1 ;
          if (T1 > MAX_WM_T1)
            T1 = MAX_WM_T1 ;
          if (PD < MIN_WM_PD)
            PD = MIN_WM_PD ;
          T1_wm += T1 ; PD_wm += PD ;
        }
        
        PD_wm /= (double)white_index ; T1_wm /= (double)white_index ;
      }
      
      /* compute mean gm for these positions */
      if (pial_index - white_index <= 1)
      {
        j = nint((inward_dist + white_dist + 0.5*cortical_dist)/ep->dstep);
        for (i = 0 ; i < ep->nvolumes ; i++)
          mean_gray[i] = image_vals[i][j] ;
        compute_T1_PD(mean_gray, ep->mri_flash, ep->nvolumes, &T1_gm, &PD_gm) ;
      }
      else
      {
        for (T1_gm = PD_gm = 0.0, j = white_index+1 ; j < pial_index ; j++)
        {
          for (i = 0 ; i < ep->nvolumes ; i++)
            mean_gray[i] = image_vals[i][j] ;
          compute_T1_PD(mean_gray, ep->mri_flash, ep->nvolumes, &T1, &PD) ;
          if (T1 < MIN_GM_T1)
            T1 = MIN_GM_T1 ;
          if (T1 > MAX_GM_T1)
            T1 = MAX_GM_T1 ;
          if (PD < MIN_GM_PD)
            PD = MIN_GM_PD ;
          T1_gm += T1 ; PD_gm += PD ;
        }
        T1_gm /= (double)(pial_index-white_index-1) ;
        PD_gm /= (double)(pial_index-white_index-1) ;
      }
      /* compute mean csf for these positions */
      if (max_j - pial_index <= 1)
      {
        for (i = 0 ; i < ep->nvolumes ; i++)
          mean_csf[i] = image_vals[i][max_j] ;
        compute_T1_PD(mean_csf, ep->mri_flash, ep->nvolumes,&T1_csf,&PD_csf);
      }
      else
      {
        for (T1_csf = PD_csf = 0.0, j = pial_index+1 ; j <= max_j ; j++)
        {
          for (i = 0 ; i < ep->nvolumes ; i++)
            mean_csf[i] = image_vals[i][j] ;
          compute_T1_PD(mean_csf, ep->mri_flash, ep->nvolumes,&T1,&PD);
          T1_csf += T1 ; PD_csf += PD ;
        }
        T1_csf /= (double)(max_j - pial_index) ;
        PD_csf /= (double)(max_j - pial_index) ;
      }
    
    
      /* do some bounds checking */
      if (PD_wm < MIN_WM_PD)
        PD_wm = MIN_WM_PD ;
      if (T1_wm < MIN_WM_T1)
        T1_wm = MIN_WM_T1 ;
      if (T1_wm > MAX_WM_T1)
        T1_wm = MAX_WM_T1 ;
      if (T1_gm < MIN_GM_T1)
        T1_gm = MIN_GM_T1 ;
      if (T1_gm > MAX_GM_T1)
        T1_gm = MAX_GM_T1 ;
      if (T1_csf < MIN_CSF_T1)
        T1_csf = MIN_CSF_T1 ;
      if (PD_gm < MIN_GM_PD)
        PD_gm = MIN_GM_PD ;
      sse = compute_vertex_sse(ep, image_vals, max_j, white_dist, 
                               cortical_dist,
                               T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf, 0) ;
      
      /* do some bounds checking */
      if (T1_wm < MIN_WM_T1 || T1_wm > MAX_WM_T1 ||
          T1_gm < MIN_GM_T1 || T1_gm > MAX_GM_T1)
        continue ;
      
      if (!finite(sse))
        ErrorPrintf(ERROR_BADPARM, "sse not finite at v %d", vno) ;
      
      if (sse < best_sse || best_sse < 0)
      {
        best_csf_len = csf_len ;
        best_white_index = white_index ; best_pial_index = pial_index ;
        best_pial_dist = pial_dist ; best_white_dist = white_dist ;
        best_sse = sse ;
        best_T1_wm = T1_wm ;   best_PD_wm = PD_wm ;
        best_T1_gm = T1_gm ;   best_PD_gm = PD_gm ;
        best_T1_csf = T1_csf ; best_PD_csf = PD_csf ;
        mris->vertices[vno].marked = 1 ;
        for (i = 0 ; i < ep->nvolumes ; i++)
        {
          for (j = 0 ; j <= max_j ; j++)
            best_image_vals[i][j] = image_vals[i][j] ;
        }
      }
    }
  }


  if ((plot_stuff == 1 && Gdiag_no == vno) ||
      plot_stuff > 1)
  {
    FILE *fp ;
    char fname[STRLEN] ;
    
    cortical_dist = best_pial_dist - best_white_dist ;
    for (i = 0 ; i < ep->nvolumes ; i++)
    {
      sprintf(fname, "vol%d_best.plt", i) ;
      fp = fopen(fname, "w") ;
      mri = ep->mri_flash[i] ;
      for (j = 0 ; j <= max_j ; j++)
      {
        dist = (double)j*ep->dstep-best_white_dist ;
        predicted_val = 
          image_forward_model(mri->tr, mri->flip_angle, dist, cortical_dist,
                              best_T1_wm, best_PD_wm, best_T1_gm, best_PD_gm, best_T1_csf, 
                              best_PD_csf) ;
        fprintf(fp, "%d %f %f\n", j, dist, predicted_val) ;
      }
      fclose(fp) ;
    }
  }

  if ((plot_stuff == 1 && Gdiag_no == vno) ||
      plot_stuff > 1)
  {
    FILE *fp ;
    
    fp = fopen("best_white.plt", "w") ;
    fprintf(fp, "%d %f %f\n%d %f %f\n",
            best_white_index, 0.0, 0.0, best_white_index, 0.0, 120.0) ;
    fclose(fp) ;
    fp = fopen("best_pial.plt", "w") ;
    fprintf(fp, "%d %f %f\n%d %f %f\n",
            best_pial_index, cortical_dist, 0.0, best_pial_index, cortical_dist, 120.0) ;
    fclose(fp) ;
  }


  *pwhite_delta = best_white_dist - (orig_white_index * ep->dstep) ;
  *ppial_delta =  best_pial_dist  - (orig_pial_index  * ep->dstep) ;
  if (*pwhite_delta < 0.1 || *ppial_delta > 0.1)
    DiagBreak() ;
  if (vno == Gdiag_no)
  {
    double pwhite[MAX_FLASH_VOLUMES], pgray[MAX_FLASH_VOLUMES], pcsf[MAX_FLASH_VOLUMES];

    printf("\n") ;
    for (i = 0 ; i < ep->nvolumes ; i++)
    {
      mri = ep->mri_flash[i] ;
      pwhite[i] = 
        image_forward_model(mri->tr, mri->flip_angle, -1, cortical_dist,
                            best_T1_wm, best_PD_wm, best_T1_gm, best_PD_gm, 
                            best_T1_csf, best_PD_csf) ;
      pgray[i] = 
        image_forward_model(mri->tr, mri->flip_angle, cortical_dist/2, 
                            cortical_dist,
                            best_T1_wm, best_PD_wm, best_T1_gm, best_PD_gm, 
                            best_T1_csf, best_PD_csf) ;
      pcsf[i] = 
        image_forward_model(mri->tr, mri->flip_angle, cortical_dist+1, 
                            cortical_dist, best_T1_wm, best_PD_wm, 
                            best_T1_gm, best_PD_gm, best_T1_csf, best_PD_csf) ;
      printf("\tpredicted image intensities %d: (%2.0f, %2.0f, %2.0f)\n",
             i, pwhite[i], pgray[i], pcsf[i]) ;
    }
    
    printf("v %d: best_white_delta = %2.2f, best_pial_delta = %2.2f, best_sse = %2.2f\n",
           vno, *pwhite_delta, *ppial_delta, best_sse) ;
    printf("      inward_dist = %2.2f, outward_dist = %2.2f, cortical_dist = %2.2f\n",
           inward_dist, outward_dist, orig_cortical_dist) ;
    cortical_dist = best_pial_dist - best_white_dist ;
    sse = compute_vertex_sse(ep, image_vals, max_j, best_white_dist, 
                             cortical_dist, best_T1_wm, best_PD_wm, 
                             best_T1_gm, best_PD_gm, best_T1_csf, 
                             best_PD_csf, plot_stuff) ;
    if (!finite(sse))
      ErrorPrintf(ERROR_BADPARM, "sse not finite at v %d", vno) ;
    
  }
  if (best_T1_wm < MIN_WM_T1)
    best_T1_wm = MIN_WM_T1 ;
  if (best_T1_wm > MAX_WM_T1)
    best_T1_wm = MAX_WM_T1 ;
  if (best_T1_gm < MIN_GM_T1)
    best_T1_gm = MIN_GM_T1 ;
  if (best_T1_gm > MAX_GM_T1)
    best_T1_gm = MAX_GM_T1 ;
#if 0
  if (best_sse > 0)
    ep->cv_outward_dists[vno] = 
      ep->dstep*((float)(best_pial_index+csf_len) - orig_pial_index) ;
#endif
    
  ep->cv_wm_T1[vno] = best_T1_wm ; ep->cv_wm_PD[vno] = best_PD_wm ;
  ep->cv_gm_T1[vno] = best_T1_gm ; ep->cv_gm_PD[vno] = best_PD_gm ;
  ep->cv_csf_T1[vno] = best_T1_csf ; ep->cv_csf_PD[vno] = best_PD_csf ;
  return(best_sse) ;
}


static double 
compute_vertex_sse(EXTRA_PARMS *ep, Real image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES], int max_j,
                   double white_dist, double cortical_dist, double T1_wm, double PD_wm, 
                   double T1_gm, double PD_gm, double T1_csf, double PD_csf, int debug)
{
  double sse, dist, predicted_val, error ;
  MRI    *mri ;
  int    i, j ;
  static int callno = 0 ;
  FILE   *fp ;

  
  sse = 0.0 ;
  if (debug)
  {
    char fname[1000] ;
    sprintf(fname, "sse%d.dat", callno) ;
    fp = fopen(fname, "w") ;
    fprintf(fp, "call %d: T1 = (%2.0f, %2.0f, %2.0f), PD = (%2.0f, %2.0f, %2.0f)\n",
            callno, T1_wm, T1_gm, T1_csf, PD_wm, PD_gm, PD_csf) ;
    fprintf(fp, "max_j = %d, white_dist = %2.2f, cortical_dist = %2.2f\n",
            max_j, white_dist, cortical_dist) ;
    printf("call %d: T1 = (%2.0f, %2.0f, %2.0f), PD = (%2.0f, %2.0f, %2.0f)\n",
           callno, T1_wm, T1_gm, T1_csf, PD_wm, PD_gm, PD_csf) ;
    printf("max_j = %d, white_dist = %2.2f, cortical_dist = %2.2f\n",
           max_j, white_dist, cortical_dist) ;
    callno++ ;
  }
  else
    fp = NULL ;
  for (i = 0 ; i < ep->nvolumes ; i++)
  {
    mri = ep->mri_flash[i] ;
    for (j = 0 ; j <= max_j ; j++)
    {
      dist = (double)j*ep->dstep-white_dist ;
      predicted_val = 
        image_forward_model(mri->tr, mri->flip_angle, dist, cortical_dist,
                            T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
      error = (image_vals[i][j] - predicted_val) ;
      sse += (error*error) ;
      if (!finite(sse))
      {
        printf("sse not finite predicted_val=%2.1f, "
               "tissue parms=(%2.0f,%2.0f,%2.0f|%2.0f, %2.0f, %2.0f)\n",
               predicted_val, T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
        DiagBreak() ;
      }
      if (debug)
        fprintf(fp, "%f %f %f %f %d %d %f\n",
                dist, image_vals[i][j], predicted_val, error, i, j, sse) ;
    }
  }
  sse /= ((max_j+1)*ep->nvolumes) ; /* so it matches vertex_error */
  if (debug)
  {
    fprintf(fp, "sse = %2.3f\n", sse) ;
    printf("sse = %2.3f\n", sse) ;
    fclose(fp) ;
  }
  return(sse) ;
}




#if 0
static int
sample_parameter_map(MRI_SURFACE *mris, MRI *mri, MRI *mri_res, 
                     float *cv_parm, float *cv_dists, float dir, 
                     int which_vertices, char *name, HISTOGRAM *h_prior, 
                     double dstep, double max_sampling_dist)
{
  VERTEX    *v ;
  int       vno, bpeak, bsmooth_peak, bno ;
  float     dist, max_dist, len, dist1, dist2 ;
  Real      dx, dy, dz, res, parm_sample, total_wt, wt, x0, y0, z0, x1, y1, z1,
             parm, xs, ys, zs, xe, ye, ze, tx1, ty1, tz1, tx2, ty2, tz2 ;
  HISTOGRAM *histo, *hsmooth ;

  histo = HISTOalloc(MAX_BINS) ;
  hsmooth = HISTOalloc(MAX_BINS) ;
  for (bno = 0 ; bno < MAX_BINS ; bno++)
    histo->bins[bno] = BIN_SIZE*bno+0.5*BIN_SIZE ;

  if (!mri)
    return(NO_ERROR) ;

  for (vno = 0 ; vno < mris->nvertices ; vno++)
  {
    v = &mris->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (vno == Gdiag_no)
      DiagBreak() ;

    switch (which_vertices)
    {
    default:
      ErrorReturn(ERROR_BADPARM,
                  (ERROR_BADPARM, "sample_parameter_map: %d bad vertex set", which_vertices));
      break ;
    case PIAL_VERTICES:
      xs = v->pialx ; ys = v->pialy ; zs = v->pialz ;
      break ;
    case ORIGINAL_VERTICES:
      xs = v->origx ; ys = v->origy ; zs = v->origz ;
      break ;
    }

    dx = v->pialx - v->origx ; dy = v->pialy - v->origy ; dz = v->pialz - v->origz ;
    if (cv_dists)
      max_dist = cv_dists[vno] ;
    else  /* no distance specified - assume it is cortical thickness */
      max_dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    if (h_prior)
      max_dist += 2 ;

    if (FZERO(max_dist))
      max_dist = dstep ;

    dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    if (FZERO(dist))
    {
      dx = v->nx ; dy = v->ny ; dz = v->nz ;
      dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    }
    if (FZERO(dist))
      dist = dstep ;
    dx /= dist ; dy /= dist ; dz /= dist ;
    xe = xs + max_dist*dir*dx ; ye = ys + max_dist*dir*dy ; ze = zs + max_dist*dir*dz ;

    MRIworldToVoxel(mri, xs, ys, zs, &xs, &ys, &zs) ;
    MRIworldToVoxel(mri, xe, ye, ze, &xe, &ye, &ze) ;

    dx = xe-xs ; dy = ye-ys ; dz = ze-zs ; 
    max_dist = sqrt(dx*dx + dy*dy + dz*dz) ;  /* in units of voxels now */
    if (FZERO(max_dist))
      max_dist = dstep ;
    dx /= max_dist ; dy /= max_dist ; dz /= max_dist ; 

    /* avoid boundaries of region */
    xs += dstep*dx ; ys += dstep*dy ; zs += dstep*dz ; 
    max_dist = max_dist - 1.5*dstep ;   /* so we will sample the last point */
    if (max_dist <= dstep)
      max_dist = 1.5*dstep ;

    /*  now compute tangent vectors */
    tx2 = dy ; ty2 = dz ; tz2 = dx ;  /* any independent vector */
    tx1 = dy*tz2 - dz*ty2 ;           /* cross product */
    ty1 = dz*tx2 - dx*tz2 ;
    tz1 = dx*ty2 - dy*tx2 ;
    len = sqrt(tx1*tx1 + ty1*ty1 + tz1*tz1) ;
    if (FZERO(len))
      len = 1 ;
    tx1 /= len ; ty1 /= len ; tz1 /= len ;

    tx2 = dy*tz1 - dz*ty1 ;           /* cross product */
    ty2 = dz*tx1 - dx*tz1 ;
    tz2 = dx*ty1 - dy*tx1 ;
    len = sqrt(tx2*tx2 + ty2*ty2 + tz2*tz2) ;
    if (FZERO(len))
      len = 1 ;
    tx2 /= len ; ty2 /= len ; tz2 /= len ;

    HISTOclearCounts(histo, histo) ;
    total_wt = 0 ; parm = 0 ;
    for (dist = 0 ; dist <= max_dist ; dist += dstep) /* along surface normal (sort of) */
    {
      x0 = xs + dist*dx ; y0 = ys + dist*dy ; z0 = zs + dist*dz ; 

      /* sample in plane */
      for (dist1 = -.5 ; dist1 <= 0.5 ; dist1 += dstep)
      {
        for (dist2 = -.5 ; dist2 <= 0.5 ; dist2 += dstep)
        {
          x1 = x0 + dist1*tx1 + dist2*tx2 ;
          y1 = y0 + dist1*ty1 + dist2*ty2 ;
          z1 = z0 + dist1*tz1 + dist2*tz2 ;

          MRIsampleVolumeType(mri, x1, y1, z1, &parm_sample, sample_type) ;
          MRIsampleVolumeType(mri_res, x1, y1, z1, &res, sample_type) ;
          wt = 1 / (res + EPSILON) ;   /* avoid dividing by 0 */
          total_wt += wt ;
          parm += (parm_sample*wt) ;
          bno = (int)(parm_sample/BIN_SIZE) ;
          if (bno >= MAX_BINS)
            bno = MAX_BINS-1 ;
          else if (bno < 0)
            bno = 0 ;
          histo->counts[bno]++ ;
        }
      }
    }
    HISTOsmooth(histo, hsmooth, histo_sigma) ;
    if (h_prior)
    {
      if (Gdiag_no == vno)
      {
        HISTOplot(hsmooth, "hsmooth_noprior.plt") ;
        bsmooth_peak = HISTOfindHighestPeakInRegion(hsmooth, 0, MAX_BINS) ;
        printf("before application of prior, peak at %2.1f (%d)\n",
               hsmooth->bins[bsmooth_peak], bsmooth_peak) ;
      }
      HISTOmul(histo, h_prior, hsmooth) ;
    }

    bsmooth_peak = HISTOfindHighestPeakInRegion(hsmooth, 0, MAX_BINS) ;
    parm /= total_wt ;
#if 0
    cv_parm[vno] = parm ;
#else
    cv_parm[vno] =  hsmooth->bins[bsmooth_peak] ;
#endif
    if (vno == Gdiag_no)
    {
      bpeak = HISTOfindHighestPeakInRegion(histo, 0, MAX_BINS) ;
      printf("v %d: %s = %2.0f (max_dist = %2.2f) (%2.1f,%2.1f,%2.1f) --> (%2.1f,%2.1f,%2.1f)\n",
             vno, name, cv_parm[vno], max_dist, xs, ys, zs, xe, ye, ze) ;
      printf("    : max peak %2.0f (%d), smooth %2.0f (%d), weighted mean %2.1f\n",
             histo->bins[bpeak], bpeak,
             hsmooth->bins[bsmooth_peak], bsmooth_peak, parm) ;
      HISTOplot(histo, "histo.plt") ;
      HISTOplot(hsmooth, "hsmooth.plt") ;
    }
    if (!finite(parm))
      ErrorPrintf(ERROR_BADPARM, "sample_parameter_map(%s): vno %d, parm = %2.2f, total_wt = %2.2f\n",
                  name, vno, parm, total_wt) ;
  }

  HISTOfree(&histo) ; HISTOfree(&hsmooth) ;
  return(NO_ERROR) ;
}
#endif

static double
compute_sse(MRI_SURFACE *mris, INTEGRATION_PARMS *parms)
{
  double sse ;

  MRISsaveVertexPositions(mris, TMP_VERTICES) ;

  MRISrestoreVertexPositions(mris, ORIGINAL_VERTICES) ;
  MRIScomputeMetricProperties(mris) ;
  sse = MRIScomputeSSE(mris, parms) ;

  MRISrestoreVertexPositions(mris, PIAL_VERTICES) ;
  MRIScomputeMetricProperties(mris) ;
  sse += MRIScomputeSSE(mris, parms) ;

  MRISrestoreVertexPositions(mris, TMP_VERTICES) ;
  return(sse) ;
}


typedef struct
{
  double *flash ;    /* forward model f(T1,TR,alpha) */
  double TR ;
  double alpha ;
  double step_size ; /* T1 increment between table elements */
  double min_T1 ;
  int    size ;      /* number of elements in flash and norm */
} FLASH_LOOKUP_TABLE, FLT ;

static FLT *find_lookup_table(double TR, double flip_angle) ;
static FLT lookup_tables[MAX_FLASH_VOLUMES] ;
static double *norms = NULL ;
static int ntables = 0 ;

static int
build_lookup_table(double tr, double flip_angle, 
                   double min_T1, double max_T1, double step)
{
  FLT    *flt ;
  int    i ;
  double T1 ;

  flt = find_lookup_table(tr, flip_angle) ;
  if (flt != NULL)
    return(NO_ERROR) ;  /* already created one */

  if (ntables >= MAX_FLASH_VOLUMES)
    ErrorExit(ERROR_NOMEMORY, "%s: MAX_FLASH_VOLUMES %d exceeded",
              Progname, MAX_FLASH_VOLUMES) ;


  flt = &lookup_tables[ntables++] ;
  flt->TR = tr ; flt->alpha = flip_angle ; flt->step_size = step ;

  flt->size = (int)((max_T1 - min_T1) / step)+1 ;
  flt->flash = (double *)calloc(flt->size, sizeof(*flt->flash)) ;
  if (!flt->flash)
    ErrorExit(ERROR_NOMEMORY, "%s: could not allocated %dth lookup table",
              Progname, ntables) ;

  for (i = 0, T1 = MIN_T1 ; T1 <= MAX_T1 ; i++, T1 += step)
    flt->flash[i] = FLASHforwardModel(flip_angle, tr, 1.0, T1) ;
  return(NO_ERROR) ;
}

#define T1_TO_INDEX(T1) (nint((T1 - MIN_T1) / T1_STEP_SIZE))

static double
lookup_flash_value(double TR, double flip_angle, double PD, double T1)
{
  int   index ;
  FLT   *flt ;
  double FLASH ;

  flt = find_lookup_table(TR, flip_angle) ;
  if (!flt)
    return(FLASHforwardModel(flip_angle, TR, PD, T1)) ;

  index = T1_TO_INDEX(T1) ;
  if (index < 0)
    index = 0 ;
  if (index >= flt->size)
    index = flt->size-1 ;
  FLASH = PD*flt->flash[index] ;
  return(FLASH) ;
}

static FLT *
find_lookup_table(double TR, double flip_angle)
{
  int   i ;

  for (i = 0 ; i < ntables ; i++)
    if (FEQUAL(lookup_tables[i].TR, TR) && FEQUAL(lookup_tables[i].alpha, flip_angle))
      break ;

  if (i >= ntables)
    return(NULL) ;
  return(&lookup_tables[i]) ;
}
static double
compute_optimal_vertex_positions(MRI_SURFACE *mris, int vno, EXTRA_PARMS *ep, 
                           double *pwhite_delta, double *ppial_delta)
{
  double       predicted_val, dx, dy, dz,
               dist, inward_dist, outward_dist, cortical_dist, PD_wm, PD_gm, PD_csf,
               T1_wm, T1_gm, T1_csf, white_dist, pial_dist, sse, best_sse,
               best_white_dist, best_pial_dist, orig_cortical_dist, total_dist,
               orig_white_index, orig_pial_index, sigma, pial_delta, wm_delta,
               gw_border, pial_border ;
  int          i, j, white_vno, pial_vno, max_j, best_white_index,
               best_pial_index, pial_index, white_index, max_white_index, nwm, npial  ;
  VERTEX       *v_white, *v_pial ;
  MRI          *mri ;
  Real         image_vals[MAX_FLASH_VOLUMES][MAX_SAMPLES], xw, yw, zw, xp, yp, zp, x, y, z;
  double       pwhite[MAX_FLASH_VOLUMES], pgray[MAX_FLASH_VOLUMES], pcsf[MAX_FLASH_VOLUMES];

  white_vno = vno ;
#if 0
  pial_vno = ep->nearest_pial_vertices[vno] ;
#else
  pial_vno = vno ;
#endif
  v_white = &mris->vertices[white_vno] ;
  v_pial = &mris->vertices[pial_vno] ;
#if 1
  inward_dist = ep->cv_inward_dists[white_vno] ;
  outward_dist = ep->cv_outward_dists[white_vno] ;
#else
  inward_dist = ep->max_dist ;
  outward_dist = ep->max_dist ;
#endif
  sigma = ep->current_sigma ;

  dx = v_pial->pialx - v_white->origx ;
  dy = v_pial->pialy - v_white->origy ;
  dz = v_pial->pialz - v_white->origz ;
  orig_cortical_dist = cortical_dist = sqrt(dx*dx + dy*dy + dz*dz) ;
  MRIworldToVoxel(ep->mri_flash[0], v_white->origx, v_white->origy,v_white->origz,
                  &xw,&yw,&zw);
  MRIworldToVoxel(ep->mri_flash[0], v_pial->pialx, v_pial->pialy,v_pial->pialz,
                  &xp,&yp,&zp);
  if (FZERO(cortical_dist))
  {
    dx = v_white->nx ; dy = v_white->ny ; dz = v_white->nz ;
    orig_cortical_dist = cortical_dist = sqrt(dx*dx + dy*dy + dz*dz) ;
    MRIworldToVoxel(ep->mri_flash[0], 
                    v_pial->pialx+v_pial->nx, 
                    v_pial->pialy+v_pial->ny,
                    v_pial->pialz+v_pial->nz,
                    &xp,&yp,&zp);
  }

  if (FZERO(cortical_dist))
    cortical_dist = ep->dstep ;
  dx /= cortical_dist ; dy /= cortical_dist ; dz /= cortical_dist ;
  
  dx = xp - xw ; dy = yp - yw ; dz = zp - zw ; 
  dist = sqrt(dx*dx + dy*dy + dz*dz) ;
  if (FZERO(dist))
    dist = ep->dstep ;
  dx /= dist ; dy /= dist ; dz /= dist ;  /* surface normal in volume coords */

  T1_wm = ep->cv_wm_T1[white_vno] ; PD_wm = ep->cv_wm_PD[white_vno] ;
  T1_gm = ep->cv_gm_T1[white_vno] ; PD_gm = ep->cv_gm_PD[white_vno] ;
  T1_csf = ep->cv_csf_T1[white_vno] ; PD_csf = ep->cv_csf_PD[white_vno] ;

  orig_white_index = inward_dist / ep->dstep ;
  orig_pial_index = orig_white_index + cortical_dist / ep->dstep ;
  max_j = (int)((inward_dist + cortical_dist + outward_dist) / ep->dstep) ;
  for (i = 0 ; i < ep->nvolumes ; i++)
  {
    mri = ep->mri_flash[i] ;
    for (j = 0, dist = -inward_dist ; dist <= cortical_dist+outward_dist ; 
         dist += ep->dstep, j++)
    {
      x = xw + dist * dx ; y = yw + dist * dy ; z = zw + dist * dz ;
#if 0
      MRIsampleVolumeDirectionScale(mri, x, y, z, dx, dy, dz, &image_vals[i][j], sigma) ;
#else
      MRIsampleVolumeType(mri, x, y, z, &image_vals[i][j], sample_type) ;
#endif
    }

    if ((plot_stuff == 1 && Gdiag_no == vno) ||
        plot_stuff > 1)
    {
      FILE *fp ;
      char fname[STRLEN] ;
      
      sprintf(fname, "vol%d.plt", i) ;
      fp = fopen(fname, "w") ;
      for (j = 0, dist = -inward_dist ; dist <= cortical_dist+outward_dist ; 
           dist += ep->dstep, j++)
        fprintf(fp, "%d %f %f\n", j, dist, image_vals[i][j]) ;
      fclose(fp) ;
    }
  }
  if ((plot_stuff == 1 && Gdiag_no == vno) ||
      plot_stuff > 1)
  {
    FILE *fp ;
    
    fp = fopen("orig_white.plt", "w") ;
    fprintf(fp, "%f %f %f\n%f %f %f\n",
            orig_white_index, 0.0, 0.0, orig_white_index, 0.0, 120.0) ;
    fclose(fp) ;
    fp = fopen("orig_pial.plt", "w") ;
    fprintf(fp, "%f %f %f\n%f %f %f\n",
            orig_pial_index, cortical_dist, 0.0, orig_pial_index, cortical_dist, 120.0) ;
    fclose(fp) ;
  }


  total_dist = outward_dist + orig_cortical_dist + inward_dist ;
  best_white_index = orig_white_index ; best_pial_index = orig_pial_index ;
  best_white_dist = orig_white_index*ep->dstep; best_pial_dist = orig_pial_index*ep->dstep;
  max_white_index = nint((inward_dist + orig_cortical_dist/2)/ep->dstep) ;
  cortical_dist = best_pial_dist - best_white_dist ;
  best_sse = compute_vertex_sse(ep, image_vals, max_j, best_white_dist, cortical_dist, 
                                T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf, 0) ;
  for (white_index = 0 ; white_index <= max_white_index ; white_index++)
  {
    white_dist = white_index * ep->dstep ;
    for (pial_index = white_index + nint(1.0/ep->dstep) ; pial_index <= max_j ;pial_index++)
    {
      pial_dist = pial_index * ep->dstep ;
      cortical_dist = pial_dist - white_dist ;
      sse = compute_vertex_sse(ep, image_vals, max_j, white_dist, cortical_dist, 
                               T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf, 0) ;
      if (sse < best_sse)
      {
        best_white_index = white_index ;
        best_pial_index = pial_index ;
        best_pial_dist = pial_dist ;
        best_white_dist = white_dist ;
        best_sse = sse ;
      }
    }
  }

  if ((plot_stuff == 1 && Gdiag_no == vno) ||
      plot_stuff > 1)
  {
    FILE *fp ;
    char fname[STRLEN] ;
    
    cortical_dist = best_pial_dist - best_white_dist ;
    for (i = 0 ; i < ep->nvolumes ; i++)
    {
      sprintf(fname, "vol%d_best.plt", i) ;
      fp = fopen(fname, "w") ;
      mri = ep->mri_flash[i] ;
      for (j = 0 ; j <= max_j ; j++)
      {
        dist = (double)j*ep->dstep-best_white_dist ;
        predicted_val = 
          image_forward_model(mri->tr, mri->flip_angle, dist, cortical_dist,
                              T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
        fprintf(fp, "%d %f %f\n", j, dist, predicted_val) ;
      }
      fclose(fp) ;
    }
  }

  if ((plot_stuff == 1 && Gdiag_no == vno) ||
      plot_stuff > 1)
  {
    FILE *fp ;
    
    fp = fopen("best_white.plt", "w") ;
    fprintf(fp, "%d %f %f\n%d %f %f\n",
            best_white_index, 0.0, 0.0, best_white_index, 0.0, 120.0) ;
    fclose(fp) ;
    fp = fopen("best_pial.plt", "w") ;
    fprintf(fp, "%d %f %f\n%d %f %f\n",
            best_pial_index, cortical_dist, 0.0, best_pial_index, cortical_dist, 120.0) ;
    fclose(fp) ;
  }


  /* get subsample accuracy with linear fit */
  for (nwm = npial = 0, pial_delta = wm_delta = 0.0, i = 0 ; i < ep->nvolumes ; i++)
  {
    double I0, I1, Idist ;

    mri = ep->mri_flash[i] ;
    pwhite[i] = 
      image_forward_model(mri->tr, mri->flip_angle, -1, cortical_dist,
                          T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
    pgray[i] = 
      image_forward_model(mri->tr, mri->flip_angle, cortical_dist/2, cortical_dist,
                          T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
    pcsf[i] = 
      image_forward_model(mri->tr, mri->flip_angle, cortical_dist+1, cortical_dist,
                          T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
    gw_border = (pwhite[i] + pgray[i])/2 ;
    pial_border = (pcsf[i] + pgray[i])/2 ;

    I0 = image_vals[i][best_white_index] ;
    if (best_white_index > 0)
    {
      I1 = image_vals[i][best_white_index-1] ;
      Idist = I1-I0 ;
      if (fabs(gw_border-I0) < fabs(Idist) &&
          fabs(gw_border-I1) < fabs(Idist))
      {    /* predicted value is between best and previous */
        wm_delta -= (gw_border - I0) / Idist ;
        nwm++ ;
      }
    }

    if (best_white_index < max_j)
    {
      I1 = image_vals[i][best_white_index+1] ;
      Idist = I1-I0 ;

      if (fabs(gw_border-I0) < fabs(Idist) &&
          fabs(gw_border-I1) < fabs(Idist))
      {    /* predicted value is between best and previous */
        wm_delta += (gw_border - I0)  / Idist ;
        nwm++ ;
      }
    }

    I0 = image_vals[i][best_pial_index] ;
    if (best_pial_index > 0)
    {
      I1 = image_vals[i][best_pial_index-1] ;
      Idist = I1-I0 ;
      if (fabs(pial_border-I0) < fabs(Idist) &&
          fabs(pial_border-I1) < fabs(Idist))
      {    /* predicted value is between best and previous */
        pial_delta -= (pial_border - I0) / Idist ;
        npial++ ;
      }
    }

    if (best_pial_index < max_j)
    {
      I1 = image_vals[i][best_pial_index+1] ;
      Idist = I1-I0 ;

      if (fabs(pial_border-I0) < fabs(Idist) &&
          fabs(pial_border-I1) < fabs(Idist))
      {    /* predicted value is between best and previous */
        pial_delta += (pial_border - I0) / Idist ;
        npial++ ;
      }
    }
  }
  
  if (nwm)
    wm_delta /= (float)nwm ;
  if (npial)
    pial_delta /= (float)npial ;
#if 0
  *pwhite_delta = best_white_dist - (orig_white_index * ep->dstep) + wm_delta*ep->dstep ;
  *ppial_delta =  best_pial_dist  - (orig_pial_index  * ep->dstep) + pial_delta*ep->dstep;
#else
  *pwhite_delta = best_white_dist - (orig_white_index * ep->dstep) ;
  *ppial_delta =  best_pial_dist  - (orig_pial_index  * ep->dstep) ;
#endif
  if (*pwhite_delta < 0.1 || *ppial_delta > 0.1)
    DiagBreak() ;
  if (vno == Gdiag_no)
  {
    double pwhite[MAX_FLASH_VOLUMES], pgray[MAX_FLASH_VOLUMES], pcsf[MAX_FLASH_VOLUMES];

    for (i = 0 ; i < ep->nvolumes ; i++)
    {
      mri = ep->mri_flash[i] ;
      pwhite[i] = 
        image_forward_model(mri->tr, mri->flip_angle, -1, cortical_dist,
                            T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
      pgray[i] = 
        image_forward_model(mri->tr, mri->flip_angle, cortical_dist/2, cortical_dist,
                            T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
      pcsf[i] = 
        image_forward_model(mri->tr, mri->flip_angle, cortical_dist+1, cortical_dist,
                            T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf) ;
      printf("\tpredicted image intensities %d: (%2.0f, %2.0f, %2.0f)\n",
             i, pwhite[i], pgray[i], pcsf[i]) ;
    }

    printf("v %d: best_white_delta = %2.2f, best_pial_delta = %2.2f, best_sse = %2.2f\n",
           vno, *pwhite_delta, *ppial_delta, best_sse) ;
    printf("      inward_dist = %2.2f, outward_dist = %2.2f, cortical_dist = %2.2f\n",
           inward_dist, outward_dist, orig_cortical_dist) ;
    cortical_dist = best_pial_dist - best_white_dist ;
    sse = compute_vertex_sse(ep, image_vals, max_j, best_white_dist, cortical_dist, 
                               T1_wm, PD_wm, T1_gm, PD_gm, T1_csf, PD_csf, plot_stuff) ;
    
  }
  return(best_sse) ;
}

#if 0
static int
compute_T1_PD_slow(Real *image_vals, MRI **mri_flash, int nvolumes, 
                   double *pT1, double *pPD)
{
  double    sse, best_sse, best_T1, best_PD, pred, PD, T1, error ;
  int       i ;
  MRI       *mri ;

  if (plot_stuff <= 2)   /* disable it for now */
    return(NO_ERROR) ;


  best_T1 = best_PD = 0 ; best_sse = -1 ;
  for (PD = 100.0 ; PD < 1700 ; PD += T1_STEP_SIZE)
  {
    for (T1 = MIN_T1 ; T1 <= MAX_T1 ; T1 += T1_STEP_SIZE)
    {
      for (sse = 0.0, i = 0 ; i < nvolumes ; i++)
      {
        mri = mri_flash[i] ;
        pred = FLASHforwardModel(mri->flip_angle, mri->tr, PD, T1) ;
        error = (pred - image_vals[i]) ;
        sse += (error*error) ;
      }
      if (sse < best_sse || best_sse < 0)
      {
        best_sse = sse ; best_T1 = T1 ; best_PD = PD ;
      }
    }
  }
    
  *pT1 = best_T1 ; *pPD = best_PD ;
  return(NO_ERROR) ;
}
#endif
static int
compute_T1_PD(Real *image_vals, MRI **mri_flash, int nvolumes, 
              double *pT1, double *pPD)
{
  double    best_T1, best_PD, norm_im, norm_pred, sse, best_sse, T1,
            pred_vals[MAX_FLASH_VOLUMES], error, upper_T1, lower_T1, mid_T1,
            upper_sse, lower_sse, mid_sse, upper_norm, mid_norm, lower_norm, range ;
  int       i, j, upper_j, lower_j, mid_j, niter ;
  MRI       *mri ;

  if (!norms)
  {
    norms = (double *)calloc(nint((MAX_T1-MIN_T1)/T1_STEP_SIZE)+1, sizeof(double)) ;
    if (!norms)
      ErrorExit(ERROR_NOMEMORY, "%s: could not allocate norm table", Progname) ;
    for (j = 0, T1 = MIN_T1 ; T1 < MAX_T1 ; T1 += T1_STEP_SIZE, j++)
    {
      for (norm_pred = 0.0, i = 0 ; i < nvolumes ; i++)
      {
        mri = mri_flash[i] ;
        pred_vals[i] = FLASHforwardModelLookup(mri->flip_angle, mri->tr, 1.0, T1) ;
        norm_pred += (pred_vals[i]*pred_vals[i]) ;
      }
      norms[j] = sqrt(norm_pred) ;
      if (FZERO(norms[j]))
      {
        printf("norms[%d] is zero!\n", j) ;
        DiagBreak() ;
        exit(0) ;
      }
    }
  }

  for (norm_im = i = 0 ; i < nvolumes ; i++)
    norm_im += (image_vals[i]*image_vals[i]) ;
  norm_im = sqrt(norm_im) ;   /* length of image vector */
  if (FZERO(norm_im))
  {
    *pT1 = MIN_T1 ;
    *pPD = MIN_T1 ;
    return(ERROR_BADPARM) ;
  }
  for (i = 0 ; i < nvolumes ; i++)
    image_vals[i] /= norm_im ;   /* normalize them */

  mid_T1 = (MAX_T1 - MIN_T1)/2 ;   mid_j = T1_TO_INDEX(mid_T1) ;
  range = (MAX_T1 - MIN_T1) / 2 ;

  /* compute sse for mid T1 */
  mid_norm = norms[mid_j] ;
  for (mid_sse = 0.0, i = 0 ; i < nvolumes ; i++)
  {
    mri = mri_flash[i] ;
    pred_vals[i] = FLASHforwardModelLookup(mri->flip_angle, mri->tr, 1.0, mid_T1) ;
    pred_vals[i] /= mid_norm ; /* normalize them */
    error = (pred_vals[i] - image_vals[i]) ;
    mid_sse += (error * error) ;
  }

  best_T1 = mid_T1 ; best_PD = norm_im / mid_norm ; best_sse = mid_sse ;
  niter = 0 ;
  if (FZERO(mid_norm))
  {
    printf("mid norm=0 at %d (%2.1f)\n", mid_j, mid_T1) ;
    DiagBreak() ;
    exit(0) ;
  }
  do
  {
    upper_T1 = mid_T1 + 0.5*range ;
    lower_T1 = mid_T1 - 0.5*range ;
    if (upper_T1 > MAX_T1)
      upper_T1 = MAX_T1 ;
    if (lower_T1 < MIN_T1)
      lower_T1 = MIN_T1 ;
    upper_j = T1_TO_INDEX(upper_T1) ; lower_j = T1_TO_INDEX(lower_T1) ;
    upper_norm = norms[upper_j] ; lower_norm = norms[lower_j] ;
    if (FZERO(upper_norm))
    {
      printf("upper norm=0 at %d (%2.1f)\n", upper_j, upper_T1) ;
      DiagBreak() ;
      exit(0) ;
    }
    if (FZERO(lower_norm))
    {
      printf("lower norm=0 at %d (%2.1f)\n", lower_j, lower_T1) ;
      DiagBreak() ;
      exit(0) ;
    }
    for (lower_sse = upper_sse = 0.0, i = 0 ; i < nvolumes ; i++)
    {
      mri = mri_flash[i] ;

      pred_vals[i] = FLASHforwardModelLookup(mri->flip_angle, mri->tr, 1.0, upper_T1) ;
      pred_vals[i] /= upper_norm ; /* normalize them */
      error = (pred_vals[i] - image_vals[i]) ;
      upper_sse += (error * error) ;

      pred_vals[i] = FLASHforwardModelLookup(mri->flip_angle, mri->tr, 1.0, lower_T1) ;
      pred_vals[i] /= lower_norm ; /* normalize them */
      error = (pred_vals[i] - image_vals[i]) ;
      lower_sse += (error * error) ;
    }
    
    if (lower_sse <= mid_sse && lower_sse <= upper_sse) /* make lower new mid */
    {
      mid_sse = lower_sse ; mid_j = lower_j ; mid_norm = lower_norm ; mid_T1 = lower_T1 ;
      best_T1 = lower_T1 ; best_PD = norm_im / lower_norm ; best_sse = lower_sse ;
    }
    else if (upper_sse < mid_sse)  /* make upper new mid */
    {
      mid_sse = upper_sse ; mid_j = upper_j ; mid_norm = upper_norm ; mid_T1 = upper_T1 ;
      best_T1 = upper_T1 ; best_PD = norm_im / upper_norm ; best_sse = upper_sse ;
    }
    if (!finite(best_PD))
    {
      printf("best_PD is not finite at %d (%2.1f)\n", mid_j, mid_T1) ;
      DiagBreak() ;
      exit(0) ;
    }
    range /= 2 ;
    niter++ ;
  } while (upper_j - lower_j > 3) ;

  for (i = 0 ; i < nvolumes ; i++)
    image_vals[i] *= norm_im ;   /* restore them */

  for (sse = 0.0, i = 0 ; i < nvolumes ; i++)
  {
    mri = mri_flash[i] ;
    pred_vals[i] = FLASHforwardModelLookup(mri->flip_angle, mri->tr, best_PD, best_T1) ;
    error = (pred_vals[i] - image_vals[i]) ;
    sse += (error * error) ;
  }

  *pT1 = best_T1 ; *pPD = best_PD ;
  return(NO_ERROR) ;
}


