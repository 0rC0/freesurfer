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
#include "gca.h"
#include "transform.h"
#include "cma.h"

static int distance_to_label(MRI *mri_labeled, int label, int x, 
                             int y, int z, int dx, int dy, 
                             int dz, int max_dist) ;
static int preprocess(MRI *mri_in, MRI *mri_labeled, GCA *gca, LTA *lta, 
                      MRI *mri_fixed) ;
static int edit_hippocampus(MRI *mri_in, MRI *mri_labeled, GCA *gca, LTA *lta, 
                      MRI *mri_fixed) ;
static int MRIcountNbhdLabels(MRI *mri, int x, int y, int z, int label) ;
int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;

char *Progname ;
static void usage_exit(int code) ;

static int filter = 0 ;
#if 0
static float thresh = 0.5 ;
#endif
static int read_flag = 0 ;
static char *read_fname =  NULL ;

static char *wm_fname = NULL ;
static int fixed_flag = 0 ;
static char *heq_fname = NULL ;
static int max_iter = 200 ;
static int mle_niter = 4 ;
static int no_gibbs = 0 ;
static int anneal = 0 ;
static char *mri_fname = NULL ;
static int hippocampus_flag = 1 ;

#define CMA_PARCELLATION  0
static int parcellation_type = CMA_PARCELLATION ;
static MRI *insert_wm_segmentation(MRI *mri_labeled, MRI *mri_wm, 
                                  int parcellation_type, int fixed_flag,
                                   GCA *gca, LTA *lta) ;

extern char *gca_write_fname ;
extern int gca_write_iterations ;

static int expand_flag = TRUE ;

int
main(int argc, char *argv[])
{
  char   **av ;
  int    ac, nargs ;
  char   *in_fname, *out_fname,  *gca_fname, *xform_fname ;
  MRI    *mri_in, *mri_labeled, *mri_fixed = NULL ;
  int          msec, minutes, seconds ;
  struct timeb start ;
  GCA     *gca ;
  LTA     *lta ;

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

  if (argc < 5)
    usage_exit(1) ;
  in_fname = argv[1] ;
  xform_fname = argv[2];
  gca_fname = argv[3] ;
  out_fname = argv[4] ;

  printf("reading input volume from %s...\n", in_fname) ;
  mri_in = MRIread(in_fname) ;
  if (!mri_in)
    ErrorExit(ERROR_NOFILE, "%s: could not read input MR volume from %s",
              Progname, in_fname) ;

  if (filter)
  {
    MRI *mri_dir, /**mri_grad,*/ *mri_kernel, *mri_smooth ;

    mri_kernel = MRIgaussian1d(1, 15) ;
    mri_smooth = MRIconvolveGaussian(mri_in, NULL, mri_kernel) ;
    MRIfree(&mri_kernel) ;
#if 0
    mri_grad = MRIsobel(mri_smooth, NULL, NULL) ;
    mri_dir = MRIdirectionMapUchar(mri_grad, NULL, 5) ;
    MRIfree(&mri_grad) ; 
    MRIwrite(mri_dir, "dir.mgh") ;
#else
    mri_dir = MRIgradientDir2ndDerivative(mri_in, NULL, 5) ;
    MRIscaleAndMultiply(mri_in, 128.0, mri_dir, mri_in) ;
    MRIwrite(mri_dir, "lap.mgh") ;
    MRIwrite(mri_in, "filtered.mgh") ;
#endif
    MRIfree(&mri_dir) ; MRIfree(&mri_smooth) ;
    exit(1) ;
  }

  /*  fprintf(stderr, "mri_in read: xform %s\n", mri_in->transform_fname) ;*/
  printf("reading classifier array from %s...\n", gca_fname) ;
  gca = GCAread(gca_fname) ;
  if (!gca)
    ErrorExit(ERROR_NOFILE, "%s: could not read classifier array from %s",
              Progname, gca_fname) ;

  if (mri_fname)
  {
    GCAbuildMostLikelyVolume(gca, mri_in) ;
    MRIwrite(mri_in, mri_fname) ;
    exit(0) ;
  }
  if (stricmp(xform_fname, "none"))
  {
    lta = LTAread(xform_fname) ;
    if (!lta)
      ErrorExit(ERROR_NOFILE, "%s: could not open transform", xform_fname) ;
  }
  else
    lta = LTAalloc(1, NULL) ;

  if (heq_fname)
  {
    MRI *mri_eq ;

    mri_eq = MRIread(heq_fname) ;
    if (!mri_eq)
      ErrorExit(ERROR_NOFILE, 
                "%s: could not read histogram equalization volume %s", 
                Progname, heq_fname) ;
    MRIhistoEqualize(mri_in, mri_eq, mri_in, 30, 170) ;
    MRIfree(&mri_eq) ;
    if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    {
      fprintf(stderr, "writing equalized volume to %s...\n", "heq.mgh") ;
      MRIwrite(mri_in, "heq.mgh") ;
    }
  }

  if (read_flag)
  {
    mri_labeled = MRIread(read_fname) ;
    if (!mri_labeled)
      ErrorExit(ERROR_NOFILE, "%s: could not read parcellation from %s",
                Progname, out_fname) ;
  }
  else
  {
    printf("labeling volume...\n") ;
    mri_labeled = GCAlabel(mri_in, gca, NULL, lta) ;
    if (wm_fname)
    {
      MRI *mri_wm ;
      
      mri_wm = MRIread(wm_fname) ;
      if (!mri_wm)
        ErrorExit(ERROR_NOFILE, "%s: could not read wm segmentation from %s",
                  Progname, wm_fname) ;
      mri_fixed = insert_wm_segmentation(mri_labeled,mri_wm,parcellation_type,
                                         fixed_flag, gca, lta);
      if (DIAG_VERBOSE_ON)
      {
        fprintf(stderr, "writing patched labeling to %s...\n", out_fname) ;
        MRIwrite(mri_labeled, out_fname) ;
      }
      MRIfree(&mri_wm) ;
    }
    else
      mri_fixed = MRIclone(mri_labeled, NULL) ;
    
    if (gca_write_iterations != 0)
    {
      char fname[STRLEN] ;
      sprintf(fname, "%s_pre.mgh", gca_write_fname) ;
      printf("writing snapshot to %s...\n", fname) ;
      MRIwrite(mri_labeled, fname) ;
    }
    preprocess(mri_in, mri_labeled, gca, lta, mri_fixed) ;
    if (fixed_flag == 0)
      MRIfree(&mri_fixed) ;
    if (!no_gibbs)
    {
      if (anneal)
        GCAanneal(mri_in, gca, mri_labeled, lta, max_iter) ;
      else
        GCAreclassifyUsingGibbsPriors(mri_in, gca, mri_labeled, lta, max_iter,
                                      mri_fixed);
    }
  }
  GCAmaxLikelihoodBorders(gca, mri_in, mri_labeled, mri_labeled,lta,mle_niter,
                          5.0);
  if (expand_flag)
  {
    GCAexpandVentricle(gca, mri_in, mri_labeled, mri_labeled, lta,   
                       Left_Lateral_Ventricle) ;
    GCAexpandVentricle(gca, mri_in, mri_labeled, mri_labeled, lta,   
                       Left_Inf_Lat_Vent) ;
    GCAexpandVentricle(gca, mri_in, mri_labeled, mri_labeled, lta,   
                       Right_Lateral_Ventricle) ;
    GCAexpandVentricle(gca, mri_in, mri_labeled, mri_labeled, lta,   
                       Right_Inf_Lat_Vent) ;
  }

  if (gca_write_iterations != 0)
  {
    char fname[STRLEN] ;
    sprintf(fname, "%s_post.mgh", gca_write_fname) ;
    printf("writing snapshot to %s...\n", fname) ;
    MRIwrite(mri_labeled, fname) ;
  }
  GCAconstrainLabelTopology(gca, mri_in, mri_labeled, mri_labeled, lta) ;
  GCAfree(&gca) ; MRIfree(&mri_in) ;
#if 0
  if (filter)
  {
    MRI *mri_tmp ;

    printf("filtering labeled volume...\n") ;
    mri_tmp = MRIthreshModeFilter(mri_labeled, NULL, filter, thresh) ;
    MRIfree(&mri_labeled) ;
    mri_labeled = mri_tmp ;
  }
#endif

  printf("writing labeled volume to %s...\n", out_fname) ;
  MRIwrite(mri_labeled, out_fname) ;

  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;
  printf("auto-labeling took %d minutes and %d seconds.\n", 
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
  if (!stricmp(option, "NOGIBBS"))
  {
    no_gibbs = 1 ;
    printf("disabling gibbs priors...\n") ;
  }
  else if (!stricmp(option, "WM"))
  {
    wm_fname = argv[2] ;
    nargs = 1 ;
    printf("inserting white matter segmentation from %s...\n", wm_fname) ;
  }
  else if (!stricmp(option, "NITER"))
  {
    mle_niter = atoi(argv[2]) ;
    nargs = 1 ;
    printf("applying max likelihood for %d iterations...\n", mle_niter) ;
  }
  else if (!stricmp(option, "nohippo"))
  {
    hippocampus_flag = 0 ;
    printf("disabling auto-editting of hippocampus\n") ;
  }
  else if (!stricmp(option, "FWM"))
  {
    fixed_flag = 1 ;
    wm_fname = argv[2] ;
    nargs = 1 ;
    printf("inserting fixed white matter segmentation from %s...\n",wm_fname);
  }
  else if (!stricmp(option, "MRI"))
  {
    mri_fname = argv[2] ;
    nargs = 1 ;
    printf("building most likely MR volume and writing to %s...\n", mri_fname);
  }
  else if (!stricmp(option, "HEQ"))
  {
    heq_fname = argv[2] ;
    nargs = 1 ;
    printf("reading template for histogram equalization from %s...\n", 
           heq_fname) ;
  }
  else switch (toupper(*option))
  {
  case 'R':
    read_flag = 1 ;
    read_fname = argv[2] ;
    nargs = 1 ;
    break ;
  case 'A':
    anneal = 1 ;
    fprintf(stderr, "using simulated annealing to find optimum\n") ;
    break ;
  case 'W':
    gca_write_iterations = atoi(argv[2]) ;
    gca_write_fname = argv[3] ;
    nargs = 2 ;
    printf("writing out snapshots of gibbs process every %d iterations to %s\n",
           gca_write_iterations, gca_write_fname) ;
    break ;
  case 'E':
    expand_flag = atoi(argv[2]) ;
    nargs = 1 ;
    break ;
  case 'N':
    max_iter = atoi(argv[2]) ;
    nargs = 1 ;
    printf("setting max iterations to %d...\n", max_iter) ;
    break ;
  case 'F':
#if 0
    filter = atoi(argv[2]) ;
    thresh = atof(argv[3]) ;
    nargs = 2 ;
    printf("applying thresholded (%2.2f) mode filter %d times to output of "
           "labelling\n",thresh,filter);
#else
    filter = 1 ;
#endif
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
  printf("usage: %s [options] <input volume> <xform> <gca file>"
         " <output volume>\n", Progname) ;
  exit(code) ;
}


static int cma_editable_labels[] = 
{
  Left_Hippocampus,
  Right_Hippocampus,
  Left_Cerebral_Cortex,
  Left_Cerebral_White_Matter,
  Right_Cerebral_Cortex,
  Right_Cerebral_White_Matter,
#if 1
  Left_Amygdala,
  Right_Amygdala
#endif
} ;
#define NEDITABLE_LABELS sizeof(cma_editable_labels) / sizeof(cma_editable_labels[0])

static MRI *
insert_wm_segmentation(MRI *mri_labeled, MRI *mri_wm, int parcellation_type, 
                       int fixed_flag, GCA *gca,LTA *lta)
{
  int      x, y, z, width, depth, height, change_label[1000], n, label,
           nchanged, rh, lh, xn, yn, zn ;
  MRI      *mri_fixed ;
  GCA_NODE *gcan ;
  double   wm_prior ;

  mri_fixed = MRIclone(mri_wm, NULL) ;

  memset(change_label, 0, sizeof(change_label)) ;
  for (n = 0 ; n < NEDITABLE_LABELS ; n++)
    change_label[cma_editable_labels[n]] = 1 ;
  width = mri_wm->width ; height = mri_wm->height ; depth = mri_wm->depth ;

  for (nchanged = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        if (x == 103 && y == 121 && z == 122)
          DiagBreak() ;
        if (MRIvox(mri_wm, x, y, z) < WM_MIN_VAL || MRIvox(mri_wm,x,y,z)>=200)
          continue ;
        label = MRIvox(mri_labeled, x, y, z) ;
        if (!change_label[label])
          continue ;
        
        GCAsourceVoxelToNode(gca, mri_labeled, lta, x, y, z, &xn, &yn, &zn) ;
        gcan = &gca->nodes[xn][yn][zn] ;
        wm_prior = 0.0 ;
        for (n = 0 ; n < gcan->nlabels ; n++)
          if (gcan->labels[n] == Right_Cerebral_White_Matter ||
              gcan->labels[n] == Left_Cerebral_White_Matter)
            wm_prior = gcan->gcs[n].prior ;
#define PRIOR_THRESH 0.1
#define FIXED_PRIOR  0.5
        if (wm_prior < PRIOR_THRESH)
          continue ;
        lh = MRIcountNbhdLabels(mri_labeled, x, y, z, 
                                Left_Cerebral_White_Matter) ;
        lh += MRIcountNbhdLabels(mri_labeled, x, y, z, 
                                Left_Cerebral_Cortex) ;
        rh = MRIcountNbhdLabels(mri_labeled, x, y, z, 
                                Right_Cerebral_White_Matter) ;
        rh += MRIcountNbhdLabels(mri_labeled, x, y, z, 
                                Right_Cerebral_Cortex) ;
        if (rh > lh)
          label = Right_Cerebral_White_Matter ;
        else
          label = Left_Cerebral_White_Matter ;
        if (label != MRIvox(mri_labeled, x, y, z))
          nchanged++ ;
        MRIvox(mri_labeled, x, y, z) = label ;
#if 0
        if (MRIvox(mri_wm, x, y, z) < 140)
#endif
          MRIvox(mri_fixed, x, y, z) = fixed_flag ;
      }
    }
  }

  printf("%d labels changed to wm\n", nchanged) ;
  return(mri_fixed) ;
}

static int
MRIcountNbhdLabels(MRI *mri, int x, int y, int z, int label)
{
  int     total, xi, yi, zi, xk, yk, zk ;

  for (total = 0, zk = -1 ; zk <= 1 ; zk++)
  {
    zi = mri->zi[z+zk] ;
    for (yk = -1 ; yk <= 1 ; yk++)
    {
      yi = mri->yi[y+yk] ;
      for (xk = -1 ; xk <= 1 ; xk++)
      {
        xi = mri->xi[x+xk] ;
        if (MRIvox(mri, xi, yi, zi) == label)
          total++ ;
      }
    }
  }

  return(total) ;
}


static int cma_expandable_labels[] = 
{
#if 0
  Left_Putamen,
  Right_Putamen,
  Left_Amygdala,
  Right_Amygdala,
  Left_Caudate,
  Right_Caudate,
#endif
  Left_Pallidum,
  Right_Pallidum
#if 0
  Left_Lateral_Ventricle,
  Left_Inf_Lat_Vent,
  Right_Lateral_Ventricle,
  Right_Inf_Lat_Vent
#endif
} ;
#define NEXPANDABLE_LABELS sizeof(cma_expandable_labels) / sizeof(cma_expandable_labels[0])

static int
preprocess(MRI *mri_in, MRI *mri_labeled, GCA *gca, LTA *lta,
           MRI *mri_fixed)
{
  int i ;

  
  if (hippocampus_flag)
    edit_hippocampus(mri_in, mri_labeled, gca, lta, mri_fixed) ;
  if (expand_flag) 
  {
    for (i = 0 ; i < NEXPANDABLE_LABELS ; i++)
      GCAexpandLabelIntoWM(gca, mri_in, mri_labeled, mri_labeled, lta,
                           mri_fixed, cma_expandable_labels[i]) ;
#if 0
    GCAexpandVentricleIntoWM(gca, mri_in, mri_labeled, mri_labeled, lta,
                             mri_fixed,Left_Lateral_Ventricle) ;
    GCAexpandVentricleIntoWM(gca, mri_in, mri_labeled, mri_labeled, lta,
                             mri_fixed,Left_Inf_Lat_Vent) ;
    GCAexpandVentricleIntoWM(gca, mri_in, mri_labeled, mri_labeled, lta,
                             mri_fixed,Right_Lateral_Ventricle) ;
    GCAexpandVentricleIntoWM(gca, mri_in, mri_labeled, mri_labeled, lta,
                             mri_fixed, Right_Inf_Lat_Vent) ;
#endif
  }
#if 0
  GCAmaxLikelihoodBorders(gca, mri_in, mri_labeled, mri_labeled, lta,2,2.0) ;
#endif
  return(NO_ERROR) ;
}

static int
distance_to_label(MRI *mri_labeled, int label, int x, int y, int z, int dx, 
                  int dy, int dz, int max_dist)
{
  int   xi, yi, zi, d ;

  for (d = 1 ; d <= max_dist ; d++)
  {
    xi = x + d * dx ; yi = y + d * dy ; zi = z + d * dz ;
    xi = mri_labeled->xi[xi] ; 
    yi = mri_labeled->yi[yi] ; 
    zi = mri_labeled->zi[zi];
    if (MRIvox(mri_labeled, xi, yi, zi) == label)
      break ;
  }

  return(d) ;
}
static int
edit_hippocampus(MRI *mri_in, MRI *mri_labeled, GCA *gca, LTA *lta, 
                 MRI *mri_fixed)
{
  int   width, height, depth, x, y, z, label, val, nchanged, dleft, 
        dright, dpos, dant, dup, ddown, dup_hippo, ddown_gray, i ;
  MRI   *mri_tmp ;
  float phippo, pwm ;

  mri_tmp = MRIcopy(mri_labeled, NULL) ;

  width = mri_in->width ; height = mri_in->height ; depth = mri_in->depth ;

  for (nchanged = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        if (x == 96 && y == 123 && z == 132)  /* wm should be pallidum */
          DiagBreak() ;
        label = MRIvox(mri_labeled, x, y, z) ;
        val = MRIvox(mri_in, x, y, z) ;

        if (label == Left_Hippocampus)
        {
          pwm = GCAlabelProbability(mri_in, gca, lta, x, y, z, 
                                    Left_Cerebral_White_Matter) ;
          phippo = GCAlabelProbability(mri_in, gca, lta, x, y, z, 
                                       Left_Hippocampus) ;
          if (phippo > 2*pwm)
            continue ;  /* don't let it change */

          dleft = distance_to_label(mri_labeled, Left_Cerebral_White_Matter,
                                    x,y,z,-1,0,0,3);
          dright = distance_to_label(mri_labeled, Left_Cerebral_White_Matter,
                                     x,y,z,1,0,0,3);
          ddown = distance_to_label(mri_labeled,
                                  Left_Cerebral_White_Matter,x,y,z,0,1,0,2);
          dup = distance_to_label(mri_labeled,
                                   Left_Cerebral_White_Matter,x,y,z,0,-1,0,2);
          dpos = distance_to_label(mri_labeled,
                                   Left_Cerebral_White_Matter,x,y,z,0,0,-1,3);
          dant = distance_to_label(mri_labeled,
                                   Left_Cerebral_White_Matter,x,y,z,0,0,1,3);

          if ((dpos <= 2 && dant <= 2) ||
              (dleft <= 2 && dright <= 2) ||
              (dup <= 1 && (dleft == 1 || dright == 1)))
          {
            nchanged++ ;
            MRIvox(mri_tmp, x, y, z) = Left_Cerebral_White_Matter ;
          }
        }
        else if (label == Right_Hippocampus)
        {
          pwm = GCAlabelProbability(mri_in, gca, lta, x, y, z, 
                                    Right_Cerebral_White_Matter) ;
          phippo = GCAlabelProbability(mri_in, gca, lta, x, y, z, 
                                       Right_Hippocampus) ;
          if (phippo > 2*pwm)
            continue ;  /* don't let it change */

          dleft = distance_to_label(mri_labeled, Right_Cerebral_White_Matter,
                                    x,y,z,-1,0,0,3);
          dright = distance_to_label(mri_labeled, Right_Cerebral_White_Matter,
                                     x,y,z,1,0,0,3);
          ddown = distance_to_label(mri_labeled,
                                  Right_Cerebral_White_Matter,x,y,z,0,1,0,2);
          dup = distance_to_label(mri_labeled,
                                   Right_Cerebral_White_Matter,x,y,z,0,-1,0,2);
          dpos = distance_to_label(mri_labeled,
                                   Right_Cerebral_White_Matter,x,y,z,0,0,-1,3);
          dant = distance_to_label(mri_labeled,
                                   Right_Cerebral_White_Matter,x,y,z,0,0,1,3);
          if ((dpos <= 2 && dant <= 2) ||
              (dleft <= 2 && dright <= 2) ||
              (dup <= 1 && (dleft == 1 || dright == 1)))
          {
            nchanged++ ;
            MRIvox(mri_tmp, x, y, z) = Right_Cerebral_White_Matter ;
          }
        }
      }
    }
  }

  for (i = 0 ; i < 2 ; i++)
  {
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          if (x == 96 && y == 123 && z == 132)  
            DiagBreak() ;
          label = MRIvox(mri_tmp, x, y, z) ;
          val = MRIvox(mri_in, x, y, z) ;
          
          if (label == Left_Hippocampus)
          {
            ddown = distance_to_label(mri_tmp, Left_Cerebral_White_Matter,
                                      x,y,z,0,1,0,2);
            dup = distance_to_label(mri_tmp,
                                    Left_Cerebral_White_Matter,x,y,z,0,-1,0,3);
            
            ddown_gray = distance_to_label(mri_tmp,Left_Cerebral_Cortex,
                                           x,y,z,0,1,0,2);
            dup_hippo = distance_to_label(mri_tmp,
                                          Left_Hippocampus,x,y,z,0,-1,0,3);
            
            /* closer to superior white matter than hp and gray below */
            if (((dup < dup_hippo) && ddown > ddown_gray))
            {
              nchanged++ ;
              MRIvox(mri_tmp, x, y, z) = Left_Cerebral_Cortex ;
            }
          }
          else if (label == Right_Hippocampus)
          {
            ddown = distance_to_label(mri_tmp,Right_Cerebral_White_Matter,
                                      x,y,z,0,1,0,3);
            dup = distance_to_label(mri_tmp,Right_Cerebral_White_Matter,
                                    x,y,z,0,-1,0,3);
            
            ddown_gray = distance_to_label(mri_tmp,
                                           Right_Cerebral_Cortex,
                                           x,y,z,0,1,0,2);
            dup_hippo = distance_to_label(mri_tmp,
                                          Right_Hippocampus,x,y,z,0,-1,0,3);
            
            /* closer to superior white matter than hp and gray below */
            if (((dup < dup_hippo) && ddown > ddown_gray))
            {
              nchanged++ ;
              MRIvox(mri_tmp, x, y, z) = Right_Cerebral_Cortex ;
            }
          }
        }
      }
    }
  }

  MRIcopy(mri_tmp, mri_labeled) ;
  MRIfree(&mri_tmp) ;
  printf("%d hippocampal voxels changed.\n", nchanged) ;
  return(NO_ERROR) ;
}
