/*
 *       FILE NAME:   mrinorm.c
 *
 *       DESCRIPTION: utilities for normalizing MRI intensity values
 *
 *       AUTHOR:      Bruce Fischl
 *       DATE:        4/9/97
 *
*/

/*-----------------------------------------------------
                    INCLUDE FILES
-------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <memory.h>

#include "error.h"
#include "proto.h"
#include "mri.h"
#include "macros.h"
#include "diag.h"
#include "volume_io.h"
#include "filter.h"
#include "box.h"
#include "region.h"
#include "nr.h"
#include "mrinorm.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/


/*-----------------------------------------------------
                    STATIC PROTOTYPES
-------------------------------------------------------*/

static MRI *mriSplineNormalizeShort(MRI *mri_src,MRI *mri_dst, 
                                    MRI **pmri_field, float *inputs,
                                    float *outputs, int npoints) ;
static MRI *mriMarkUnmarkedNeighbors(MRI *mri_src, MRI *mri_marked,
                                     MRI *mri_dst, int mark, int nbr_mark) ;
static int mriRemoveOutliers(MRI *mri, int min_nbrs) ;
#if 0
static MRI *mriDownsampleCtrl2(MRI *mri_src, MRI *mri_dst) ;
#endif
static MRI *mriSoapBubbleFloat(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,
                               int niter) ;
static MRI *mriSoapBubbleShort(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,
                               int niter) ;
static MRI *mriSoapBubbleExpandFloat(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,
                               int niter) ;
static MRI *mriBuildVoronoiDiagramFloat(MRI *mri_src, MRI *mri_ctrl,
                                        MRI *mri_dst);
static MRI *mriBuildVoronoiDiagramShort(MRI *mri_src, MRI *mri_ctrl,
                                        MRI *mri_dst);

static int num_control_points = 0 ;
static int *xctrl ;
static int *yctrl ;
static int *zctrl ;

static char *control_volume_fname = NULL ;
static char *bias_volume_fname = NULL ;

/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           Given a set of control points, calculate the
           1 dimensional splines which fit them and apply it
           to an image.
------------------------------------------------------*/
#define BIAS_IMAGE_WIDTH  25

MRI *
MRIsplineNormalize(MRI *mri_src,MRI *mri_dst, MRI **pmri_field,
                   float *inputs,float *outputs, int npoints)
{
  int       width, height, depth, x, y, z, i, dval ;
  BUFTYPE   *psrc, *pdst, sval, *pfield = NULL ;
  float     outputs_2[MAX_SPLINE_POINTS], frac ;
  MRI       *mri_field = NULL ;
  double    d ;
  char      *cp ;

  if (mri_src->type == MRI_SHORT)
    return(mriSplineNormalizeShort(mri_src, mri_dst, pmri_field, inputs, 
                                   outputs, npoints)) ;
  cp = getenv("RAN") ;
  if (cp)
    d = atof(cp) ;
  else
    d = 0.0 ;

  if (pmri_field)
  {
    mri_field = *pmri_field ;
    if (!mri_field)
      *pmri_field = mri_field = 
        MRIalloc(BIAS_IMAGE_WIDTH, mri_src->height, 1, MRI_UCHAR) ;
  }

  if (npoints > MAX_SPLINE_POINTS)
    npoints = MAX_SPLINE_POINTS ;
  spline(inputs, outputs, npoints, 0.0f, 0.0f, outputs_2) ;
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  width = mri_src->width ;
  height = mri_src->height ;
  depth = mri_src->depth ;

  for (y = 0 ; y < height ; y++)
  {
    if (pmri_field)
      pfield = &MRIvox(mri_field, 0, y, 0) ;
    splint(inputs, outputs, outputs_2, npoints, (float)y, &frac) ;
    if (pmri_field)
      for (i = 0 ; i < BIAS_IMAGE_WIDTH ; i++)
        *pfield++ = nint(110.0f/frac) ;

    for (z = 0 ; z < depth ; z++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pdst = &MRIvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        sval = *psrc++ ;
        dval = nint((float)sval * frac + randomNumber(0.0,d)) ;
        if (dval > 255)
          dval = 255 ;
        else if (dval < 0)
          dval = 0 ;
        *pdst++ = (BUFTYPE)dval ;
      }
    }
  }
  return(mri_dst) ;
}
static MRI *
mriSplineNormalizeShort(MRI *mri_src,MRI *mri_dst, MRI **pmri_field,
                   float *inputs,float *outputs, int npoints)
{
  int       width, height, depth, x, y, z, i, dval ;
  short     *psrc, *pdst, sval ;
  char      *pfield = NULL ;
  float     outputs_2[MAX_SPLINE_POINTS], frac ;
  MRI       *mri_field = NULL ;
  double    d ;
  char      *cp ;

  cp = getenv("RAN") ;
  if (cp)
    d = atof(cp) ;
  else
    d = 0.0 ;

  if (pmri_field)
  {
    mri_field = *pmri_field ;
    if (!mri_field)
      *pmri_field = mri_field = 
        MRIalloc(BIAS_IMAGE_WIDTH, mri_src->height, 1, MRI_UCHAR) ;
  }

  if (npoints > MAX_SPLINE_POINTS)
    npoints = MAX_SPLINE_POINTS ;
  spline(inputs, outputs, npoints, 0.0f, 0.0f, outputs_2) ;
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  width = mri_src->width ;
  height = mri_src->height ;
  depth = mri_src->depth ;

  for (y = 0 ; y < height ; y++)
  {
    if (pmri_field)
      pfield = &MRIvox(mri_field, 0, y, 0) ;
    splint(inputs, outputs, outputs_2, npoints, (float)y, &frac) ;
    if (pmri_field)
      for (i = 0 ; i < BIAS_IMAGE_WIDTH ; i++)
        *pfield++ = nint(110.0f/frac) ;

    for (z = 0 ; z < depth ; z++)
    {
      psrc = &MRISvox(mri_src, 0, y, z) ;
      pdst = &MRISvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        sval = *psrc++ ;
        dval = nint((float)sval * frac + randomNumber(0.0,d)) ;
        if (dval > 255)
          dval = 255 ;
        else if (dval < 0)
          dval = 0 ;
        *pdst++ = (short)dval ;
      }
    }
  }
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          perform an adaptive histogram normalization. For each
          wsize x wsize region in the image, for the histogram of
          the hsize x hsize region around it (hsize >> wsize) and
          around the corresponding region in mri_template, and adjust
------------------------------------------------------*/
MRI *
MRIadaptiveHistoNormalize(MRI *mri_src, MRI *mri_norm, MRI *mri_template, 
                          int wsize, int hsize, int low)
{
  int        width, height, depth, woff ;
  MRI_REGION wreg, h_src_reg, h_tmp_reg, h_clip_reg ;

  /* offset the left edge of histo region w.r.t the windowed region */
  woff = (wsize - hsize) / 2 ;  

  /* align the two regions so that they have a common center */
  wreg.dx = wreg.dy = wreg.dz = wsize ;
  h_src_reg.dx = h_src_reg.dy = h_src_reg.dz = hsize ;
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ;

  h_src_reg.z = woff ;
  for (wreg.z = 0 ; wreg.z < depth ; wreg.z += wsize, h_src_reg.z += wsize)
  {
    h_src_reg.y = woff ;
    for (wreg.y = 0 ; wreg.y < height ; wreg.y += wsize, h_src_reg.y += wsize)
    {
      h_src_reg.x = woff ;
      for (wreg.x = 0 ; wreg.x < width ; wreg.x += wsize, h_src_reg.x += wsize)
      {
        MRIclipRegion(mri_src, &h_src_reg, &h_clip_reg) ;
        MRItransformRegion(mri_src, mri_template, &h_clip_reg, &h_tmp_reg) ;
        if (Gdiag & DIAG_SHOW)
#if 1
          fprintf(stderr, "\rnormalizing (%d, %d, %d) --> (%d, %d, %d)       ",
                  wreg.x, wreg.y, wreg.z, wreg.x+wreg.dx-1, wreg.y+wreg.dy-1,
                  wreg.z+wreg.dz-1) ;
#else
          fprintf(stderr, "\rnormalizing (%d, %d, %d) --> (%d, %d, %d)       ",
                  h_tmp_reg.x, h_tmp_reg.y, h_tmp_reg.z, 
                  h_tmp_reg.x+h_tmp_reg.dx-1, h_tmp_reg.y+h_tmp_reg.dy-1, 
                  h_tmp_reg.z+h_tmp_reg.dz-1) ;
#endif
#if 0
        mri_norm = MRIhistoNormalizeRegion(mri_src, mri_norm, mri_template, 
                                           low, &wreg, &h_src_reg, &h_tmp_reg);
#else
        mri_norm = MRIhistoNormalizeRegion(mri_src, mri_norm, mri_template, 
                                           low, &wreg,&h_clip_reg,&h_clip_reg);
#endif
      }
    } 
  }

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, " done.\n") ;

  return(mri_norm) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIhistoNormalizeRegion(MRI *mri_src, MRI *mri_norm, MRI *mri_template, 
                        int low, MRI_REGION *wreg, MRI_REGION *h_src_reg,
                        MRI_REGION *h_tmp_reg)
{
  HISTOGRAM  h_fwd_eq, h_template_eq, h_norm ;

  MRIgetEqualizeHistoRegion(mri_src, &h_fwd_eq, low, h_src_reg, 0) ;
  MRIgetEqualizeHistoRegion(mri_template, &h_template_eq, low, h_tmp_reg, 0);
  HISTOcomposeInvert(&h_fwd_eq, &h_template_eq, &h_norm) ;
  mri_norm = MRIapplyHistogramToRegion(mri_src, mri_norm, &h_norm, wreg) ;
  if (Gdiag & DIAG_WRITE)
  {
    FILE      *fp ; 
    HISTOGRAM h ;

    fp = fopen("histo.dat", "w") ;
    MRIhistogramRegion(mri_src, 0, &h, h_src_reg) ;
    fprintf(fp, "src histo\n") ;
    HISTOdump(&h, fp) ;
    fprintf(fp, "src eq\n") ;
    HISTOdump(&h_fwd_eq, fp) ;
    fprintf(fp, "template eq\n") ;
    HISTOdump(&h_template_eq, fp) ;
    fprintf(fp, "composite mapping\n") ;
    HISTOdump(&h_norm, fp) ;
  }

  return(mri_norm) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIhistoNormalize(MRI *mri_src, MRI *mri_norm, MRI *mri_template, int low,
                  int high)
{
  HISTOGRAM  h_fwd_eq, h_template_eq, h_norm ;

  HISTOclear(&h_fwd_eq, &h_fwd_eq) ;
  HISTOclear(&h_template_eq, &h_template_eq) ;
  HISTOclear(&h_norm, &h_norm) ;
  MRIgetEqualizeHisto(mri_src, &h_fwd_eq, low, high, 0) ;
  MRIgetEqualizeHisto(mri_template, &h_template_eq, low, high, 0) ;
  HISTOcomposeInvert(&h_fwd_eq, &h_template_eq, &h_norm) ;
  mri_norm = MRIapplyHistogram(mri_src, mri_norm, &h_norm) ;

  if (Gdiag & DIAG_WRITE)
  {
    FILE       *fp ;

    fp = fopen("histo.dat", "w") ;
    fprintf(fp, "src eq\n") ;
    HISTOdump(&h_fwd_eq, fp) ;
    fprintf(fp, "template eq\n") ;
    HISTOdump(&h_template_eq, fp) ;
    fprintf(fp, "composite mapping\n") ;
    HISTOdump(&h_norm, fp) ;
    fclose(fp) ;
  }

  return(mri_norm) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define WINDOW_WIDTH      120  /* in millimeters */

int
MRInormInit(MRI *mri, MNI *mni, int windows_above_t0,int windows_below_t0,
            int wsize, int desired_wm_value, float smooth_sigma)
{
  MRI_REGION  *reg ;
  int         i, x, y, z, dx, dy, dz, nup, z_offset, nwindows, x0_tal, y0_tal,
              z0_tal ;
  float       size_mod ;
  Real        x0, y0, z0 ;

  if (wsize <= 0)
    wsize = DEFAULT_WINDOW_SIZE ;

  if (!desired_wm_value)
    desired_wm_value = mni->desired_wm_value = 
      DEFAULT_DESIRED_WHITE_MATTER_VALUE ;
  else
    mni->desired_wm_value = desired_wm_value ;
  if (FZERO(smooth_sigma))
    smooth_sigma = mni->smooth_sigma = DEFAULT_SMOOTH_SIGMA ;
  else
    mni->smooth_sigma = smooth_sigma ;
  if (mri->inverse_linear_transform)
  {
    if (MRItalairachToVoxel(mri, 0.0, 0.0, 0.0, &x0, &y0, &z0) != NO_ERROR)
      ErrorReturn(Gerror, 
                  (Gerror, 
                   "MRInormComputeWindows: could not find Talairach origin"));
    x0_tal = nint(x0) ; y0_tal = nint(y0) ; z0_tal = nint(z0) ;
  }
  else  /* no Talairach information available */
  {
    MRI_REGION  bbox ;

#if 0
    MRIboundingBoxNbhd(mri, 50, 5, &bbox) ;
#else
    MRIfindApproximateSkullBoundingBox(mri, 50, &bbox) ;
#endif
    /* place x and z at center of bounding box */
    x0_tal = bbox.x + bbox.dx/2.0 ; z0_tal = bbox.z + bbox.dz/2.0 ;

    /* due to neck variability, place y 7.5 cm down from top of skull */
    y0_tal = bbox.y + 75.0f/mri->ysize ;
    
  }

  if (windows_above_t0 > 0)
    mni->windows_above_t0 = windows_above_t0 ;
  else
    windows_above_t0 = mni->windows_above_t0 = DEFAULT_WINDOWS_ABOVE_T0 ;

  if (windows_below_t0 > 0)
    mni->windows_below_t0 = windows_below_t0 ;
  else
    windows_below_t0 = mni->windows_below_t0 = DEFAULT_WINDOWS_BELOW_T0 ;
  nwindows = mni->windows_above_t0 + mni->windows_below_t0 ;

  if (Gdiag & DIAG_SHOW)
  {
    fprintf(stderr, "MRInormInit:\n") ;
    fprintf(stderr, "Talairach origin at (%d, %d, %d)\n",
            x0_tal, y0_tal, z0_tal) ;
    fprintf(stderr, "wsize %d, windows %d above, %d below\n", 
            wsize, mni->windows_above_t0, mni->windows_below_t0) ;
  }

  x = 0 ;
  dx = mri->width ;
  z = 0 ;
  dz = mri->depth ;
  y = y0_tal - nint((float)wsize*OVERLAP) * windows_above_t0 ;
  dy = wsize ;
  for (i = 0 ; i < nwindows ; i++)
  {
    reg = &mni->regions[i] ;
    reg->x = x ;   reg->y = y ;   reg->z = z ;
    if (y < y0_tal)  /* head gets smaller as we get further up */
    {
      nup = windows_above_t0 - i ;
      size_mod = pow(SIZE_MOD, (double)nup) ;
    }
    else
      size_mod = 1.0f ;

    dx = nint(size_mod * WINDOW_WIDTH) ;
    z_offset = nint((float)dx * Z_OFFSET_SCALE) ;
    dz = nint(size_mod * WINDOW_WIDTH) ;
    reg->dx = dx ; 
    reg->x = x0_tal - dx/2 ;
    reg->z = z0_tal - (dz/2+z_offset) ;
    reg->dz = dz + z_offset ;
    reg->dy = dy ; 
    reg->y = y ;
    y += nint((float)wsize*OVERLAP) ;
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, "window %d: (%d, %d, %d) -> (%d, %d, %d)\n",
              i, reg->x, reg->y, reg->z, reg->dx, reg->dy, reg->dz) ;
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define TOO_BRIGHT   225
int
MRInormFillHistograms(MRI *mri, MNI *mni)
{
  int         i, nwindows ;

  nwindows = mni->windows_above_t0 + mni->windows_below_t0 ;
  for (i = 0 ; i < nwindows ; i++)
  {
    MRIhistogramRegion(mri, HISTO_BINS, mni->histograms+i, mni->regions+i) ;
    HISTOclearBins(mni->histograms+i,mni->histograms+i,0,BACKGROUND_INTENSITY);
    HISTOclearBins(mni->histograms+i,mni->histograms+i,TOO_BRIGHT, 255);
  }
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int 
MRInormFindPeaks(MNI *mni, float *inputs, float *outputs)
{
  int        i, peak, deleted, nwindows, npeaks ;
  HISTOGRAM  *hsmooth = NULL ;
  MRI_REGION *reg ;

  nwindows = mni->windows_above_t0 + mni->windows_below_t0 ;
  for (deleted = i = 0 ; i < nwindows ; i++)
  {
    reg = &mni->regions[i] ;
    hsmooth = HISTOsmooth(&mni->histograms[i], hsmooth, mni->smooth_sigma) ;
    peak = HISTOfindLastPeak(hsmooth, HISTO_WINDOW_SIZE, MIN_HISTO_PCT) ;
    if (peak < 0)
      deleted++ ;
    else
    {
      inputs[i-deleted] = (float)reg->y + (float)reg->dy/ 2.0f ;
#if 0
      outputs[i-deleted] = mni->desired_wm_value / (float)peak ;
#else
      outputs[i-deleted] = (float)mni->histograms[i].bins[peak] ;
#endif
    }
  }

  npeaks = nwindows - deleted ;
  npeaks = MRInormCheckPeaks(mni, inputs, outputs, npeaks) ;
  for (i = 0 ; i < npeaks ; i++)
    outputs[i] = (float)mni->desired_wm_value / outputs[i] ;

  return(npeaks) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          the variation in the magnetic field should be relatively
          slow. Use this to do a consistency check on the peaks, removing
          any outliers.

          Note that the algorithm only deletes the furthest outlier on
          each iteration. This is because you can get a patch of bad
          peaks, the middle of which looks fine by the local slope
          test. However, the outside of the patch will be eroded at
          each iteration, and the avg and sigma estimates will improve.
          Of course, this is slower, but since the number of peaks is
          small (20 or so), time isn't a concern.

          Also, I use both forwards and backwards derivatives to try
          and arbitrate which of two neighboring peaks is bad (the bad
          one is more likely to have both fwd and bkwd derivatives far
          from the mean).
------------------------------------------------------*/
int
MRInormCheckPeaks(MNI *mni, float *inputs, float *outputs, int npeaks)
{
  int        starting_slice, slice, deleted[MAX_SPLINE_POINTS], old_slice,n ;
  float      Iup, I, Idown, dI, dy, grad, max_gradient ;


  /* rule of thumb - at least a third of the coefficients must be valid */
  if (npeaks < (mni->windows_above_t0+mni->windows_below_t0)/3) 
    return(0) ;

  if (FZERO(mni->max_gradient))
    max_gradient = MAX_GRADIENT ;
  else
    max_gradient = mni->max_gradient ;

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "max gradient %2.3f\n", mni->max_gradient) ;
  if (Gdiag & DIAG_SHOW)
    for (slice = 0 ; slice < npeaks ; slice++)
      fprintf(stderr, "%d: %2.0f --> %2.0f\n",
              slice,inputs[slice],outputs[slice]) ;

/* 
   first find a good starting slice to anchor the checking by taking the
   median peak value of three slices around the center of the brain.
*/
  starting_slice = mni->windows_above_t0-STARTING_SLICE ;
  Iup = outputs[starting_slice-SLICE_OFFSET] ;
  I = outputs[starting_slice] ;
  Idown = outputs[starting_slice+SLICE_OFFSET] ;

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, 
            "testing slices %d-%d=%d (%2.0f), %d (%2.0f), and %d (%2.0f)\n",
            mni->windows_above_t0,STARTING_SLICE, starting_slice, I,
            starting_slice+SLICE_OFFSET, Idown, starting_slice-SLICE_OFFSET,
            Iup) ;

  if ((I >= MIN(Iup, Idown)) && (I <= MAX(Iup, Idown)))
    starting_slice = starting_slice ;
  else if ((Iup >= MIN(I, Idown)) && (Iup <= MAX(I, Idown)))
    starting_slice = starting_slice - SLICE_OFFSET ;
  else
    starting_slice = starting_slice + SLICE_OFFSET ;

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "starting slice %d --> %2.0f\n", 
            starting_slice, outputs[starting_slice]) ;

  /* search forward and fill in arrays */
  old_slice = starting_slice ;
  for (slice = starting_slice+1 ; slice < npeaks ; slice++)
  {
    dI = outputs[slice] - outputs[old_slice] ;
#if 0
    dy = inputs[slice] - inputs[old_slice] ;
#else
    dy = inputs[slice] - inputs[slice-1] ;
#endif
    grad = fabs(dI / dy) ;
    deleted[slice] =  
      ((grad > max_gradient) || 
       ((slice - old_slice) > MAX_SKIPPED));
    if (!deleted[slice])
      old_slice = slice ;
    else if (Gdiag & DIAG_SHOW)
      fprintf(stderr,"deleting peak[%d]=%2.0f, grad = %2.0f / %2.0f = %2.1f\n",
              slice, outputs[slice],dI, dy, grad) ;
  }

  /* now search backwards and fill stuff in */
  old_slice = starting_slice ;
  for (slice = starting_slice-1 ; slice >= 0 ; slice--)
  {
    dI = outputs[old_slice] - outputs[slice] ;
#if 0
    dy = inputs[slice] - inputs[old_slice] ;
#else
    dy = inputs[slice+1] - inputs[slice] ;
#endif
    grad = fabs(dI / dy) ;
    deleted[slice] =  
      ((grad > max_gradient) ||
       ((old_slice - slice) > MAX_SKIPPED));
    if (!deleted[slice])
      old_slice = slice ;
    else if (Gdiag & DIAG_SHOW)
      fprintf(stderr,"deleting peak[%d]=%2.0f, grad = %2.0f / %2.0f = %2.1f\n",
              slice, outputs[slice], dI, dy, grad) ;
  }

  for (n = slice = 0 ; slice < npeaks ; slice++)
  {
    if (!deleted[slice])   /* passed consistency check */
    {
      outputs[n] = outputs[slice] ;
      inputs[n] = inputs[slice] ;
      n++ ;
    }
  }

  npeaks = n ;
  
  if (Gdiag & DIAG_SHOW)
    fflush(stderr) ;
  
  return(npeaks) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRInormalize(MRI *mri_src, MRI *mri_dst, MNI *mni)
{
  float    inputs[MAX_SPLINE_POINTS], outputs[MAX_SPLINE_POINTS] ;
  int      npeaks, dealloc = 0 ;

  if (!mni)   /* do local initialization */
  {
    dealloc = 1 ;
    mni = (MNI *)calloc(1, sizeof(MNI)) ;
    MRInormInit(mri_src, mni, 0, 0, 0, 0, 0.0f) ;
  }

  MRInormFillHistograms(mri_src, mni) ;
  npeaks = MRInormFindPeaks(mni, inputs, outputs) ;
  if (npeaks == 0)
    ErrorReturn(NULL,
                (ERROR_BADPARM,"MRInormalize: could not find any valid peaks"));
  mri_dst = MRIsplineNormalize(mri_src, mri_dst,NULL,inputs,outputs,npeaks);

  if (Gdiag & DIAG_SHOW)
  {
    int i ;

    fprintf(stderr, "normalization found %d peaks:\n", npeaks) ;
    for (i = 0 ; i < npeaks ; i++)
      fprintf(stderr, "%d: %2.1f --> %2.3f\n", i, inputs[i], outputs[i]) ;
  }
  if (dealloc)
    free(mni) ;

  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define SCALE  2

MRI *
MRInormFindControlPoints(MRI *mri_src, int wm_target, float intensity_above, 
                         float intensity_below, MRI *mri_ctrl)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, *pxi, *pyi, 
          *pzi, ctrl, nctrl, nfilled, too_low, total_filled, val0, val, n;
  BUFTYPE low_thresh, hi_thresh ;

  if (!wm_target)
    wm_target = DEFAULT_DESIRED_WHITE_MATTER_VALUE ;
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_ctrl)
    mri_ctrl = MRIalloc(width, height, depth, MRI_UCHAR) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;
  /*
    find points which are close to wm_target, and in relatively
    homogenous regions.
  */
  low_thresh = wm_target-intensity_below; 
  hi_thresh =  wm_target+intensity_above;
  for (nctrl = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        switch (mri_src->type)
        {
        case MRI_UCHAR: val0 = MRIvox(mri_src, x, y, z) ; break ;
        case MRI_SHORT: val0 = MRISvox(mri_src, x, y, z) ; break ;
        default:
          ErrorReturn(NULL, (ERROR_UNSUPPORTED, 
                             "MRInormFindControlPoints: unsupported"
                             " src format %d", mri_src->type)) ;
        }
        if (val0 >= low_thresh && val0 <= hi_thresh)
        {
#define WSIZE   5
#define WHALF  ((WSIZE-1)/2)
          ctrl = 128 ;
          for (zk = -WHALF ; ctrl && zk <= WHALF ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -WHALF ; ctrl && yk <= WHALF ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -WHALF ; ctrl && xk <= WHALF ; xk++)
              {
                xi = pxi[x+xk] ;
                switch (mri_src->type)
                {
                default:
                case MRI_UCHAR: val = MRIvox(mri_src, xi, yi, zi) ; break ;
                case MRI_SHORT: val = MRISvox(mri_src, xi, yi, zi) ; break ;
                }
                if (val > hi_thresh || val < low_thresh)
                  ctrl = 0 ;   /* not homogeneous enough */
              }
            }
          }
        }
        else
          ctrl = 0 ;
        if (ctrl)
          nctrl++ ;
        MRIvox(mri_ctrl, x, y, z) = ctrl ;
      }
    }
  }
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%d 5x5x5 control points found\n", nctrl) ;

  /*  add all voxels that neighbor a control point and are in a 3x3x3
      neighborhood that has unambiguous intensities 
      (should push things out close to the border).
  */
  total_filled = 0 ;
  do
  {
    nfilled = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          switch (mri_src->type)
          {
          default:
          case MRI_UCHAR: val0 = MRIvox(mri_src, x, y, z) ; break ;
          case MRI_SHORT: val0 = MRISvox(mri_src, x, y, z) ; break ;
          }
          ctrl = MRIvox(mri_ctrl, x, y, z) ;
          too_low = 0 ;
          if (val0 >= low_thresh && val0 <= hi_thresh && !ctrl)
          {
            if (x == 167 && y == 127 && z == 126)
              DiagBreak() ;
            for (zk = -1 ; zk <= 1 && !too_low ; zk++)
            {
              zi = pzi[z+zk] ;
              for (yk = -1 ; yk <= 1 && !too_low ; yk++)
              {
                yi = pyi[y+yk] ;
                for (xk = -1 ; xk <= 1 && !too_low ; xk++)
                {
                  /*
                    check for any 27-connected neighbor that is a control
                    point and has a small intensity difference with the
                    current point.
                  */
                  xi = pxi[x+xk] ;
                  switch (mri_src->type)
                  {
                  default:
                  case MRI_UCHAR: val = MRIvox(mri_src, xi, yi, zi) ; break ;
                  case MRI_SHORT: val = MRISvox(mri_src, xi, yi, zi) ; break ;
                  }
                  if (val > hi_thresh || val < low_thresh)
                  {
                    too_low = 1 ; ctrl = 0 ;
                  }
                  if ((abs(xk) + abs(yk) + abs(zk)) > 1)
                    continue ;  /* only allow 4 (6 in 3-d) connectivity */

                  /* now make sure that a 6-connected neighbor exists that
                     is currently a control point.
                  */
                  if (MRIvox(mri_ctrl, xi, yi, zi))
                    ctrl = 128 ;
                }
              }
            }
            MRIvox(mri_ctrl, x, y, z) = ctrl ;
            if (ctrl)
              nfilled++ ;
          }
        }
      }
    }
    total_filled += nfilled ;
  } while (nfilled > 0) ;
  nctrl += total_filled ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"%d contiguous 3x3x3 control points added\n",total_filled);


  /*  add all voxels that neighbor at least 3 control points and
      have and that lie in a 6-connected neighborhood of high
      intensities. (should push things out even close to the border).
  */
#if 0
  low_thresh = wm_target-intensity_below/2; 
  hi_thresh =  wm_target+intensity_above/2;
#endif
  total_filled = 0 ;
  do
  {
    nfilled = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          switch (mri_src->type)
          {
          default:
          case MRI_UCHAR: val0 = MRIvox(mri_src, x, y, z) ; break ;
          case MRI_SHORT: val0 = MRISvox(mri_src, x, y, z) ; break ;
          }
          ctrl = MRIvox(mri_ctrl, x, y, z) ;
          too_low = 0 ;
          if (val0 >= low_thresh && val0 <= hi_thresh && !ctrl)
          {
            if (x == 116 && y == 111 && z == 187)
              DiagBreak() ;
            n = 0 ;
            for (zk = -1 ; zk <= 1 && !too_low ; zk++)
            {
              zi = pzi[z+zk] ;
              for (yk = -1 ; yk <= 1 && !too_low ; yk++)
              {
                yi = pyi[y+yk] ;
                for (xk = -1 ; xk <= 1 && !too_low ; xk++)
                {
                  /*
                    check for any 27-connected neighbor that is a control
                    point and has a small intensity difference with the
                    current point.
                  */
                  xi = pxi[x+xk] ;
                  switch (mri_src->type)
                  {
                  default:
                  case MRI_UCHAR: val = MRIvox(mri_src, xi, yi, zi) ; break ;
                  case MRI_SHORT: val = MRISvox(mri_src, xi, yi, zi) ; break ;
                  }
                  if (MRIvox(mri_ctrl, xi, yi, zi))
                    n++ ;   /* count # of 27-connected control points */
                  if ((abs(xk) + abs(yk) + abs(zk)) > 1)
                    continue ;  /* only allow 4 (6 in 3-d) connectivity */
                  if (val > hi_thresh || val < low_thresh)
                    too_low = 1 ;
                }
              }
            }
#define MIN_CONTROL_POINTS   4
            if (n >= MIN_CONTROL_POINTS && !too_low)
            {
              ctrl = 1 ;
              MRIvox(mri_ctrl, x, y, z) = 128 ;
              nfilled++ ;
            }
          }
        }
      }
    }
    total_filled += nfilled ;
  } while (nfilled > 0) ;
  nctrl += total_filled ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"%d contiguous 6-connected control points added\n",
            total_filled);

#if 0
  low_thresh = wm_target-(2*intensity_below); 
  hi_thresh =  wm_target+intensity_above;
  total_filled = 0 ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        val0 = MRIvox(mri_src, x, y, z) ;
        if (val0 >= low_thresh && val0 <= hi_thresh)
        {
#undef WHALF
#undef WSIZE
#define WSIZE   9
#define WHALF  ((WSIZE-1)/2)
          ctrl = 128 ;
          for (zk = -WHALF ; ctrl && zk <= WHALF ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -WHALF ; ctrl && yk <= WHALF ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -WHALF ; ctrl && xk <= WHALF ; xk++)
              {
                xi = pxi[x+xk] ;
                val = MRIvox(mri_src, xi, yi, zi) ;
                if (val > hi_thresh || val < low_thresh)
                  ctrl = 0 ;   /* not homogeneous enough */
              }
            }
          }
        }
        else
          ctrl = 0 ;
        if (ctrl)
          total_filled++ ;
        MRIvox(mri_ctrl, x, y, z) = ctrl ;
      }
    }
  }
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%d %d mm homogenous control points found\n",
            total_filled,WSIZE);
  nctrl += total_filled ;
#endif

#if 0
  total_filled = 0 ;
  do
  {
    int   low_gradients ;
    float dist ;

    nfilled = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          val0 = MRIvox(mri_src, x, y, z) ;
          ctrl = MRIvox(mri_ctrl, x, y, z) ;
          low_gradients = 0 ;
          if (val0 >= low_thresh && val0 <= hi_thresh && !ctrl)
          {
            if (x == 167 && y == 127 && z == 126)
              DiagBreak() ;
            for (zk = -1 ; zk <= 1 ; zk++)
            {
              zi = pzi[z+zk] ;
              for (yk = -1 ; yk <= 1 ; yk++)
              {
                yi = pyi[y+yk] ;
                for (xk = -1 ; xk <= 1 ; xk++)
                {
                  if (!xk && !yk && !zk)
                    continue ;

                  /*
                    check for any 27-connected neighbor that is not
                    in the right intensity range.
                  */
                  xi = pxi[x+xk] ;
                  if (MRIvox(mri_ctrl, xi, yi, zi)) /* neighboring ctrl pt */
                  {
                    val = MRIvox(mri_src, xi, yi, zi) ;
                    dist = sqrt(xk*xk+yk*yk+zk*zk) ;
                    if (FZERO(dist))
                      dist = 1.0 ;
#define MIN_GRAD 1.0
                    if (fabs(val-val0)/dist < MIN_GRAD)
                      low_gradients++ ;
                  }
                  if ((abs(xk) + abs(yk) + abs(zk)) > 1)
                    continue ;  /* only allow 4 (6 in 3-d) connectivity */
                }
              }
            }
            if (low_gradients >= 9)
            {
              MRIvox(mri_ctrl, x, y, z) = 128 ;
              nfilled++ ;
            }
          }
        }
      }
    }
    total_filled += nfilled ;
  } while (nfilled > 0) ;
  nctrl += total_filled ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"%d contiguous low gradient control points added\n",
            total_filled);
#endif

#undef WSIZE
#undef WHALF
  mriRemoveOutliers(mri_ctrl, 2) ;

#if 1
  nctrl += MRInormAddFileControlPoints(mri_ctrl, 255) ;
#else
  /* read in control points from a file (if specified) */
  for (i = 0 ; i < num_control_points ; i++)
  {
    /*    if (!MRIvox(mri_ctrl, xctrl[i], yctrl[i], zctrl[i]))*/
    {
      MRIvox(mri_ctrl, xctrl[i], yctrl[i], zctrl[i]) = 255 ;
      nctrl++ ;
    }
  }
#endif

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%d control points found.\n", nctrl) ;
  return(mri_ctrl) ;
}
MRI *
MRInormGentlyFindControlPoints(MRI *mri_src, int wm_target, 
                               float intensity_above, 
                               float intensity_below, MRI *mri_ctrl)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, *pxi, *pyi, 
          *pzi, ctrl, nctrl, val0, val;
  BUFTYPE low_thresh, hi_thresh ;

  if (!wm_target)
    wm_target = DEFAULT_DESIRED_WHITE_MATTER_VALUE ;
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_ctrl)
    mri_ctrl = MRIalloc(width, height, depth, MRI_UCHAR) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;
  /*
    find points which are close to wm_target, and in relatively
    homogenous regions.
  */
  low_thresh = wm_target-intensity_below; 
  hi_thresh =  wm_target+intensity_above;
  for (nctrl = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        switch (mri_src->type)
        {
        case MRI_UCHAR: val0 = MRIvox(mri_src, x, y, z) ; break ;
        case MRI_SHORT: val0 = MRISvox(mri_src, x, y, z) ; break ;
        default:
          ErrorReturn(NULL, (ERROR_UNSUPPORTED, 
                             "MRInormFindControlPoints: unsupported"
                             " src format %d", mri_src->type)) ;
        }
        if (val0 >= low_thresh && val0 <= hi_thresh)
        {
#define WSIZE   5
#define WHALF  ((WSIZE-1)/2)
          ctrl = 128 ;
          for (zk = -WHALF ; ctrl && zk <= WHALF ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -WHALF ; ctrl && yk <= WHALF ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -WHALF ; ctrl && xk <= WHALF ; xk++)
              {
                xi = pxi[x+xk] ;
                switch (mri_src->type)
                {
                default:
                case MRI_UCHAR: val = MRIvox(mri_src, xi, yi, zi) ; break ;
                case MRI_SHORT: val = MRISvox(mri_src, xi, yi, zi) ; break ;
                }
                if (val > hi_thresh || val < low_thresh)
                  ctrl = 0 ;   /* not homogeneous enough */
              }
            }
          }
        }
        else
          ctrl = 0 ;
        if (ctrl)
          nctrl++ ;
        MRIvox(mri_ctrl, x, y, z) = ctrl ;
      }
    }
  }
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%d 5x5x5 control points found\n", nctrl) ;

#if 0
  /*  add all voxels that neighbor a control point and are in a 3x3x3
      neighborhood that has unambiguous intensities 
      (should push things out close to the border).
  */
  total_filled = 0 ;
  do
  {
    nfilled = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          switch (mri_src->type)
          {
          default:
          case MRI_UCHAR: val0 = MRIvox(mri_src, x, y, z) ; break ;
          case MRI_SHORT: val0 = MRISvox(mri_src, x, y, z) ; break ;
          }
          ctrl = MRIvox(mri_ctrl, x, y, z) ;
          too_low = 0 ;
          if (val0 >= low_thresh && val0 <= hi_thresh && !ctrl)
          {
            if (x == 167 && y == 127 && z == 126)
              DiagBreak() ;
            for (zk = -1 ; zk <= 1 && !too_low ; zk++)
            {
              zi = pzi[z+zk] ;
              for (yk = -1 ; yk <= 1 && !too_low ; yk++)
              {
                yi = pyi[y+yk] ;
                for (xk = -1 ; xk <= 1 && !too_low ; xk++)
                {
                  /*
                    check for any 27-connected neighbor that is a control
                    point and has a small intensity difference with the
                    current point.
                  */
                  xi = pxi[x+xk] ;
                  switch (mri_src->type)
                  {
                  default:
                  case MRI_UCHAR: val = MRIvox(mri_src, xi, yi, zi) ; break ;
                  case MRI_SHORT: val = MRISvox(mri_src, xi, yi, zi) ; break ;
                  }
                  if (val > hi_thresh || val < low_thresh)
                  {
                    too_low = 1 ; ctrl = 0 ;
                  }
                  if ((abs(xk) + abs(yk) + abs(zk)) > 1)
                    continue ;  /* only allow 4 (6 in 3-d) connectivity */

                  /* now make sure that a 6-connected neighbor exists that
                     is currently a control point.
                  */
                  if (MRIvox(mri_ctrl, xi, yi, zi))
                    ctrl = 128 ;
                }
              }
            }
            MRIvox(mri_ctrl, x, y, z) = ctrl ;
            if (ctrl)
              nfilled++ ;
          }
        }
      }
    }
    total_filled += nfilled ;
  } while (nfilled > 0) ;
  nctrl += total_filled ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"%d contiguous 3x3x3 control points added\n",total_filled);


  /*  add all voxels that neighbor at least 3 control points and
      have and that lie in a 6-connected neighborhood of high
      intensities. (should push things out even close to the border).
  */
#if 0
  low_thresh = wm_target-intensity_below/2; 
  hi_thresh =  wm_target+intensity_above/2;
#endif
  total_filled = 0 ;
  do
  {
    nfilled = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          switch (mri_src->type)
          {
          default:
          case MRI_UCHAR: val0 = MRIvox(mri_src, x, y, z) ; break ;
          case MRI_SHORT: val0 = MRISvox(mri_src, x, y, z) ; break ;
          }
          ctrl = MRIvox(mri_ctrl, x, y, z) ;
          too_low = 0 ;
          if (val0 >= low_thresh && val0 <= hi_thresh && !ctrl)
          {
            if (x == 116 && y == 111 && z == 187)
              DiagBreak() ;
            n = 0 ;
            for (zk = -1 ; zk <= 1 && !too_low ; zk++)
            {
              zi = pzi[z+zk] ;
              for (yk = -1 ; yk <= 1 && !too_low ; yk++)
              {
                yi = pyi[y+yk] ;
                for (xk = -1 ; xk <= 1 && !too_low ; xk++)
                {
                  /*
                    check for any 27-connected neighbor that is a control
                    point and has a small intensity difference with the
                    current point.
                  */
                  xi = pxi[x+xk] ;
                  switch (mri_src->type)
                  {
                  default:
                  case MRI_UCHAR: val = MRIvox(mri_src, xi, yi, zi) ; break ;
                  case MRI_SHORT: val = MRISvox(mri_src, xi, yi, zi) ; break ;
                  }
                  if (MRIvox(mri_ctrl, xi, yi, zi))
                    n++ ;   /* count # of 27-connected control points */
                  if ((abs(xk) + abs(yk) + abs(zk)) > 1)
                    continue ;  /* only allow 4 (6 in 3-d) connectivity */
                  if (val > hi_thresh || val < low_thresh)
                    too_low = 1 ;
                }
              }
            }
#define MIN_CONTROL_POINTS   4
            if (n >= MIN_CONTROL_POINTS && !too_low)
            {
              ctrl = 1 ;
              MRIvox(mri_ctrl, x, y, z) = 128 ;
              nfilled++ ;
            }
          }
        }
      }
    }
    total_filled += nfilled ;
  } while (nfilled > 0) ;
  nctrl += total_filled ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"%d contiguous 6-connected control points added\n",
            total_filled);

#if 0
  low_thresh = wm_target-(2*intensity_below); 
  hi_thresh =  wm_target+intensity_above;
  total_filled = 0 ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        val0 = MRIvox(mri_src, x, y, z) ;
        if (val0 >= low_thresh && val0 <= hi_thresh)
        {
#undef WHALF
#undef WSIZE
#define WSIZE   9
#define WHALF  ((WSIZE-1)/2)
          ctrl = 128 ;
          for (zk = -WHALF ; ctrl && zk <= WHALF ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -WHALF ; ctrl && yk <= WHALF ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -WHALF ; ctrl && xk <= WHALF ; xk++)
              {
                xi = pxi[x+xk] ;
                val = MRIvox(mri_src, xi, yi, zi) ;
                if (val > hi_thresh || val < low_thresh)
                  ctrl = 0 ;   /* not homogeneous enough */
              }
            }
          }
        }
        else
          ctrl = 0 ;
        if (ctrl)
          total_filled++ ;
        MRIvox(mri_ctrl, x, y, z) = ctrl ;
      }
    }
  }
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%d %d mm homogenous control points found\n",
            total_filled,WSIZE);
  nctrl += total_filled ;
#endif

#if 0
  total_filled = 0 ;
  do
  {
    int   low_gradients ;
    float dist ;

    nfilled = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        for (x = 0 ; x < width ; x++)
        {
          val0 = MRIvox(mri_src, x, y, z) ;
          ctrl = MRIvox(mri_ctrl, x, y, z) ;
          low_gradients = 0 ;
          if (val0 >= low_thresh && val0 <= hi_thresh && !ctrl)
          {
            if (x == 167 && y == 127 && z == 126)
              DiagBreak() ;
            for (zk = -1 ; zk <= 1 ; zk++)
            {
              zi = pzi[z+zk] ;
              for (yk = -1 ; yk <= 1 ; yk++)
              {
                yi = pyi[y+yk] ;
                for (xk = -1 ; xk <= 1 ; xk++)
                {
                  if (!xk && !yk && !zk)
                    continue ;

                  /*
                    check for any 27-connected neighbor that is not
                    in the right intensity range.
                  */
                  xi = pxi[x+xk] ;
                  if (MRIvox(mri_ctrl, xi, yi, zi)) /* neighboring ctrl pt */
                  {
                    val = MRIvox(mri_src, xi, yi, zi) ;
                    dist = sqrt(xk*xk+yk*yk+zk*zk) ;
                    if (FZERO(dist))
                      dist = 1.0 ;
#define MIN_GRAD 1.0
                    if (fabs(val-val0)/dist < MIN_GRAD)
                      low_gradients++ ;
                  }
                  if ((abs(xk) + abs(yk) + abs(zk)) > 1)
                    continue ;  /* only allow 4 (6 in 3-d) connectivity */
                }
              }
            }
            if (low_gradients >= 9)
            {
              MRIvox(mri_ctrl, x, y, z) = 128 ;
              nfilled++ ;
            }
          }
        }
      }
    }
    total_filled += nfilled ;
  } while (nfilled > 0) ;
  nctrl += total_filled ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"%d contiguous low gradient control points added\n",
            total_filled);
#endif
#endif

#undef WSIZE
#undef WHALF
  mriRemoveOutliers(mri_ctrl, 2) ;

#if 1
  nctrl += MRInormAddFileControlPoints(mri_ctrl, 255) ;
#else
  /* read in control points from a file (if specified) */
  for (i = 0 ; i < num_control_points ; i++)
  {
    /*    if (!MRIvox(mri_ctrl, xctrl[i], yctrl[i], zctrl[i]))*/
    {
      MRIvox(mri_ctrl, xctrl[i], yctrl[i], zctrl[i]) = 255 ;
      nctrl++ ;
    }
  }
#endif

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%d control points found.\n", nctrl) ;
  return(mri_ctrl) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#if 0
MRI *
MRIbuildBiasImage(MRI *mri_src, MRI *mri_ctrl, MRI *mri_bias)
{
  int     width, height, depth ;
  MRI     *mri_s_ctrl, *mri_s_bias, *mri_s_src, *mri_tmp ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 

  mri_s_ctrl = mriDownsampleCtrl2(mri_ctrl, NULL) ;
  mri_s_src = MRIreduceByte(mri_src, NULL) ;
  mri_s_bias = MRIclone(mri_s_src, NULL) ;
  MRIbuildVoronoiDiagram(mri_s_src, mri_s_ctrl, mri_s_bias) ;
  MRIsoapBubble(mri_s_bias, mri_s_ctrl, mri_s_bias, 25) ;
  MRIfree(&mri_s_ctrl) ; MRIfree(&mri_s_src) ; 
  mri_bias = MRIupsample2(mri_s_bias, mri_bias) ;

  MRIfree(&mri_s_bias) ;
  mri_tmp = MRImeanByte(mri_bias, NULL, 3) ;
  MRImeanByte(mri_tmp, mri_bias, 3) ;
  MRIfree(&mri_tmp) ;
  return(mri_bias) ;
}
#else
MRI *
MRIbuildBiasImage(MRI *mri_src, MRI *mri_ctrl, MRI *mri_bias)
{
  int     width, height, depth ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 

  mri_bias = MRIclone(mri_src, NULL) ;
  fprintf(stderr, "building Voronoi diagram...\n") ;
  MRIbuildVoronoiDiagram(mri_src, mri_ctrl, mri_bias) ;
  fprintf(stderr, "performing soap bubble smoothing...\n") ;
  MRIsoapBubble(mri_bias, mri_ctrl, mri_bias, 10) ;
  return(mri_bias) ;
}
#endif
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRI3dNormalize(MRI *mri_src, MRI *mri_bias, int wm_target, MRI *mri_norm,
               float intensity_above, float intensity_below, int only_file)
{
  int     width, height, depth, x, y, z, src, bias, free_bias ;
  BUFTYPE *psrc, *pbias, *pnorm ;
  float   norm ;

  if (!wm_target)
    wm_target = DEFAULT_DESIRED_WHITE_MATTER_VALUE ;
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_norm)
    mri_norm = MRIclone(mri_src, NULL) ;

  if (!mri_bias)
  {
    MRI  *mri_ctrl ;

    free_bias = 1 ;
    if (!only_file)
      mri_ctrl = MRInormFindControlPoints(mri_src, wm_target, intensity_above, 
                                          intensity_below, NULL);
    else
    {
      int nctrl ;

      mri_ctrl = MRIclone(mri_src, NULL) ;
      nctrl = MRInormAddFileControlPoints(mri_ctrl, 255) ;
      fprintf(stderr, "only using %d control points from file...\n", nctrl) ;
    }
      
    if (control_volume_fname)
    {
      fprintf(stderr, "writing control point volume to %s...\n",
              control_volume_fname) ;
      MRIwrite(mri_ctrl, control_volume_fname) ;
    }
    MRIbinarize(mri_ctrl, mri_ctrl, 1, CONTROL_NONE, CONTROL_MARKED) ;
    mri_bias = MRIbuildBiasImage(mri_src, mri_ctrl, NULL) ;
    if (bias_volume_fname)
    {
      fprintf(stderr, "writing bias field volume to %s...\n",
              bias_volume_fname) ;
      MRIwrite(mri_bias, bias_volume_fname) ;
    }
    if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    {
      static int pass = 0 ;
      char fname[500] ;

      fprintf(stderr, "writing out control and bias volumes...\n") ;
      sprintf(fname, "src%d.mgh", pass) ; MRIwrite(mri_src, fname) ;
      sprintf(fname, "ctrl%d.mgh", pass) ; MRIwrite(mri_ctrl, fname) ;
      sprintf(fname, "bias%d.mgh", pass) ; MRIwrite(mri_bias, fname) ;
      pass++ ;
    }
    MRIfree(&mri_ctrl) ;
  }
  else
    free_bias = 0 ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pbias = &MRIvox(mri_bias, 0, y, z) ;
      pnorm = &MRIvox(mri_norm, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        src = *psrc++ ; bias = *pbias++ ;
        if (!bias)   /* should never happen */
          norm = (float)src ;
        else
          norm = (float)src * (float)wm_target / (float)bias ;
        if (norm > 255.0f)
          norm = 255.0f ;
        *pnorm++ = nint(norm) ;
      }
    }
  }
  if (free_bias)
    MRIfree(&mri_bias) ;

  return(mri_norm) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRI3dGentleNormalize(MRI *mri_src, MRI *mri_bias, int wm_target, MRI *mri_norm,
                     float intensity_above,float intensity_below,int only_file)
{
  int     width, height, depth, x, y, z, src, bias, free_bias ;
  BUFTYPE *psrc, *pbias, *pnorm ;
  float   norm ;

  if (!wm_target)
    wm_target = DEFAULT_DESIRED_WHITE_MATTER_VALUE ;
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_norm)
    mri_norm = MRIclone(mri_src, NULL) ;

  if (!mri_bias)
  {
    MRI  *mri_ctrl ;

    free_bias = 1 ;
    if (!only_file)
      mri_ctrl = MRInormGentlyFindControlPoints(mri_src, wm_target, 
                                                intensity_above, 
                                                intensity_below, NULL);
    else
    {
      int nctrl ;

      mri_ctrl = MRIclone(mri_src, NULL) ;
      nctrl = MRInormAddFileControlPoints(mri_ctrl, 255) ;
      fprintf(stderr, "only using %d control points from file...\n", nctrl) ;
    }
      
    if (control_volume_fname)
    {
      fprintf(stderr, "writing control point volume to %s...\n",
              control_volume_fname) ;
      MRIwrite(mri_ctrl, control_volume_fname) ;
    }
    MRIbinarize(mri_ctrl, mri_ctrl, 1, CONTROL_NONE, CONTROL_MARKED) ;
    mri_bias = MRIbuildBiasImage(mri_src, mri_ctrl, NULL) ;
    if (bias_volume_fname)
    {
      fprintf(stderr, "writing bias field volume to %s...\n",
              bias_volume_fname) ;
      MRIwrite(mri_bias, bias_volume_fname) ;
    }
    if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    {
      static int pass = 0 ;
      char fname[500] ;

      fprintf(stderr, "writing out control and bias volumes...\n") ;
      sprintf(fname, "src%d.mgh", pass) ; MRIwrite(mri_src, fname) ;
      sprintf(fname, "ctrl%d.mgh", pass) ; MRIwrite(mri_ctrl, fname) ;
      sprintf(fname, "bias%d.mgh", pass) ; MRIwrite(mri_bias, fname) ;
      pass++ ;
    }
    MRIfree(&mri_ctrl) ;
  }
  else
    free_bias = 0 ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pbias = &MRIvox(mri_bias, 0, y, z) ;
      pnorm = &MRIvox(mri_norm, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        src = *psrc++ ; bias = *pbias++ ;
        if (!bias)   /* should never happen */
          norm = (float)src ;
        else
          norm = (float)src * (float)wm_target / (float)bias ;
        if (norm > 255.0f)
          norm = 255.0f ;
        *pnorm++ = nint(norm) ;
      }
    }
  }
  if (free_bias)
    MRIfree(&mri_bias) ;

  return(mri_norm) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static MRI *
mriBuildVoronoiDiagramFloat(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, 
          *pxi, *pyi, *pzi, nchanged, n, total ;
  BUFTYPE *pmarked, *pctrl, ctrl, mark ;
  float   *psrc, *pdst, src, val, mean ;
  MRI     *mri_marked ;
  float   scale ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  scale = mri_src->width / mri_dst->width ;
  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  /* initialize dst image */
  for (total = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIFvox(mri_src, 0, y, z) ;
      pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
      pdst = &MRIFvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == 141 && y == 68 && z == 127)
          DiagBreak() ;
        ctrl = *pctrl++ ;
        src = *psrc++ ;
        if (!ctrl)
          val = 0 ;
        else   /* find mean in region, and use it as bias field estimate */
        { 
          val = src ;
          total++ ; 
#if 0
          mean = 0.0f ; n = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                n++ ; mean += MRIFvox(mri_src, xi, yi, zi) ;
              }
            }
          }
          val = mean / (float)n ;
#else
          val = MRIFvox(mri_src, x, y, z) ; /* it's already reduced, don't avg*/
#endif
        } 
        *pdst++ = val ;
      }
    }
  }

  total = width*height*depth - total ;  /* total # of voxels to be processed */
  mri_marked = MRIcopy(mri_ctrl, NULL) ;

  /* now propagate values outwards */
  do
  {
    nchanged = 0 ;
    /*
      use mri_marked to keep track of the last set of voxels changed which
      are marked CONTROL_MARKED. The neighbors of these will be marked
      with CONTROL_TMP, and they are the only ones which need to be
      examined.
    */
    mriMarkUnmarkedNeighbors(mri_marked, mri_ctrl, mri_marked, CONTROL_MARKED, 
                             CONTROL_TMP);
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_MARKED, CONTROL_NONE) ;
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_TMP, CONTROL_MARKED) ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pmarked = &MRIvox(mri_marked, 0, y, z) ;
        pdst = &MRIFvox(mri_dst, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          mark = *pmarked++ ;
          if (mark != CONTROL_MARKED)  /* not a neighbor of a marked point */
          {
            pdst++ ; ;
            continue ;
          }

          /* now see if any neighbors are on and set this voxel to the
             average of the marked neighbors (if any) */
          mean = 0.0f ; n = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                if (MRIvox(mri_ctrl, xi, yi, zi))
                {
                  n++ ; mean += MRIFvox(mri_dst, xi, yi, zi) ;
                }
              }
            }
          }
          if (n > 0)  /* some neighbors were on */
          {
            MRIvox(mri_ctrl, x, y, z) = CONTROL_TMP ; /* it has a value */
            *pdst++ = mean / (float)n ;
            nchanged++ ;
          }
          else   /* should never happen anymore */
            pdst++ ;
        }
      }
    }
    total -= nchanged ;
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, 
              "Voronoi: %d voxels assigned, %d remaining.      \n", 
              nchanged, total) ;
  } while (nchanged > 0 && total > 0) ;

  if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    fprintf(stderr, "\n") ;
  MRIfree(&mri_marked) ;
  MRIreplaceValues(mri_ctrl, mri_ctrl, CONTROL_TMP, CONTROL_NONE) ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static MRI *
mriBuildVoronoiDiagramShort(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, 
          *pxi, *pyi, *pzi, nchanged, n, total, visited ;
  BUFTYPE *pmarked, *pctrl, ctrl, mark ;
  short   *psrc, *pdst ;
  float   src, val, mean ;
  MRI     *mri_marked ;
  float   scale ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  scale = mri_src->width / mri_dst->width ;
  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  /* initialize dst image */
  for (total = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRISvox(mri_src, 0, y, z) ;
      pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
      pdst = &MRISvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == Gx && y == Gy && z == Gz)
          DiagBreak() ;
        ctrl = *pctrl++ ;
        src = (float)*psrc++ ;
        if (!ctrl)
          val = 0 ;
        else   /* find mean in region, and use it as bias field estimate */
        { 
          val = src ;
          total++ ; 
          val = MRISvox(mri_src, x, y, z) ;/* it's already reduced, don't avg*/
        } 
        *pdst++ = val ;
      }
    }
  }

  total = width*height*depth - total ;  /* total # of voxels to be processed */
  mri_marked = MRIcopy(mri_ctrl, NULL) ;
  MRIreplaceValues(mri_marked, mri_marked, CONTROL_MARKED, CONTROL_NBR) ;

  /* now propagate values outwards */
  do
  {
    nchanged = 0 ;
    /*
      use mri_marked to keep track of the last set of voxels changed which
      are marked CONTROL_MARKED. The neighbors of these will be marked
      with CONTROL_TMP, and they are the only ones which need to be
      examined.
    */
#if 0
    MRIreplaceValues(mri_ctrl, mri_ctrl, CONTROL_HAS_VAL, CONTROL_TMP) ;
    mriMarkUnmarkedNeighbors(mri_marked, mri_ctrl, mri_marked, CONTROL_MARKED, 
                             CONTROL_TMP);
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_MARKED, CONTROL_NONE) ;
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_TMP, CONTROL_MARKED) ;
#else
    /* voxels that were CONTROL_TMP were filled on previous iteration, now they
       should be labeled as marked
    */
    /* only process nbrs of marked values (that aren't already marked) */
    mriMarkUnmarkedNeighbors(mri_marked, mri_marked, mri_marked, CONTROL_NBR, CONTROL_TMP);
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_NBR, CONTROL_MARKED) ;
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_TMP, CONTROL_NBR) ;
#endif

    /*
      everything in mri_marked=CONTROL_TMP is now a nbr of a point
      with a value.
    */
    for (visited = z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pmarked = &MRIvox(mri_marked, 0, y, z) ;
        pdst = &MRISvox(mri_dst, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          if (x == Gx && y == Gy && z == Gz)
            DiagBreak() ;
          mark = *pmarked++ ;
          if (mark != CONTROL_NBR)  /* not a neighbor of a marked point */
          {
            pdst++ ; ;
            continue ;
          }

          /* now see if any neighbors are on and set this voxel to the
             average of the marked neighbors (if any) */
          visited++ ;
          mean = 0.0f ; n = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                if (MRIvox(mri_marked, xi, yi, zi) == CONTROL_MARKED)
                {
                  n++ ; mean += (float)MRISvox(mri_dst, xi, yi, zi) ;
                }
              }
            }
          }
          if (n > 0)  /* some neighbors were on */
          {
            *pdst++ = mean / (float)n ;
            nchanged++ ;
          }
          else   /* should never happen anymore */
            pdst++ ;
        }
      }
    }
    total -= nchanged ;
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, 
              "Voronoi: %d voxels assigned, %d remaining, %d visited.      \n", 
              nchanged, total, visited) ;
  } while (nchanged > 0 && total > 0) ;

  if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    fprintf(stderr, "\n") ;
  MRIfree(&mri_marked) ;
  MRIreplaceValues(mri_ctrl, mri_ctrl, CONTROL_TMP, CONTROL_NONE) ;
  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    MRIwrite(mri_ctrl, "ctrl.mgh") ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIbuildVoronoiDiagram(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, 
          *pxi, *pyi, *pzi, nchanged, n, mean, total ;
  BUFTYPE *pmarked, *psrc, *pctrl, *pdst, ctrl, src, val, mark ;
  MRI     *mri_marked ;
  float   scale ;

  if (mri_src->type == MRI_FLOAT)
    return(mriBuildVoronoiDiagramFloat(mri_src, mri_ctrl, mri_dst));
  if (mri_src->type == MRI_SHORT)
    return(mriBuildVoronoiDiagramShort(mri_src, mri_ctrl, mri_dst));

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  scale = mri_src->width / mri_dst->width ;
  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  /* initialize dst image */
  for (total = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
      pdst = &MRIvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == 167 && y == 127 && z == 126)
          DiagBreak() ;
        ctrl = *pctrl++ ;
        src = *psrc++ ;
        if (!ctrl)
          val = 0 ;
        else   /* find mean in region, and use it as bias field estimate */
        { 
          val = src ;
          total++ ; 
#if 0
          mean = n = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                n++ ; mean += MRIvox(mri_src, xi, yi, zi) ;
              }
            }
          }
          val = nint((float)mean / (float)n) ;
#else
          val = MRIvox(mri_src, x, y, z) ; /* it's already reduced, don't avg*/
#endif
        } 
        *pdst++ = val ;
      }
    }
  }

  total = width*height*depth - total ;  /* total # of voxels to be processed */
  mri_marked = MRIcopy(mri_ctrl, NULL) ;

  /* now propagate values outwards */
  do
  {
    nchanged = 0 ;
    /*
      use mri_marked to keep track of the last set of voxels changed which
      are marked CONTROL_MARKED. The neighbors of these will be marked
      with CONTROL_TMP, and they are the only ones which need to be
      examined.
    */
    mriMarkUnmarkedNeighbors(mri_marked, mri_ctrl, mri_marked, CONTROL_MARKED, 
                             CONTROL_TMP);
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_MARKED, CONTROL_NONE) ;
    MRIreplaceValues(mri_marked, mri_marked, CONTROL_TMP, CONTROL_MARKED) ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pmarked = &MRIvox(mri_marked, 0, y, z) ;
        pdst = &MRIvox(mri_dst, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          if (x == 167 && y == 127 && z == 126)
            DiagBreak() ;
          mark = *pmarked++ ;
          if (mark != CONTROL_MARKED)  /* not a neighbor of a marked point */
          {
            pdst++ ; ;
            continue ;
          }

          /* now see if any neighbors are on and set this voxel to the
             average of the marked neighbors (if any) */
          mean = n = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                if (MRIvox(mri_ctrl, xi, yi, zi))
                {
                  n++ ; mean += MRIvox(mri_dst, xi, yi, zi) ;
                }
              }
            }
          }
          if (n > 0)  /* some neighbors were on */
          {
            MRIvox(mri_ctrl, x, y, z) = CONTROL_TMP ; /* it has a value */
            *pdst++ = nint((float)mean / (float)n) ;
            nchanged++ ;
          }
          else   /* should never happen anymore */
            pdst++ ;
        }
      }
    }
    total -= nchanged ;
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, 
              "Voronoi: %d voxels assigned, %d remaining.      \n", 
              nchanged, total) ;
  } while (nchanged > 0 && total > 0) ;

  if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    fprintf(stderr, "\n") ;
  MRIfree(&mri_marked) ;
  MRIreplaceValues(mri_ctrl, mri_ctrl, CONTROL_TMP, CONTROL_NONE) ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIsoapBubble(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,int niter)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, i,
          *pxi, *pyi, *pzi, mean ;
  BUFTYPE *pctrl, ctrl, *ptmp ;
  MRI     *mri_tmp ;
  

  if (mri_src->type == MRI_FLOAT)
    return(mriSoapBubbleFloat(mri_src, mri_ctrl, mri_dst, niter)) ;
  else if (mri_src->type == MRI_SHORT)
    return(mriSoapBubbleShort(mri_src, mri_ctrl, mri_dst, niter)) ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIcopy(mri_src, NULL) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  mri_tmp = MRIcopy(mri_dst, NULL) ;

  /* now propagate values outwards */
  for (i = 0 ; i < niter ; i++)
  {
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, "soap bubble iteration %d of %d\n", i+1, niter) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        ptmp = &MRIvox(mri_tmp, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl++ ;
          if (ctrl == CONTROL_MARKED)   /* marked point - don't change it */
          {
            ptmp++ ;
            continue ;
          }
          /* now set this voxel to the average of the marked neighbors */
          mean = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                mean += MRIvox(mri_dst, xi, yi, zi) ;
              }
            }
          }
          *ptmp++ = nint((float)mean / (3.0f*3.0f*3.0f)) ;
        }
      }
    }
    MRIcopy(mri_tmp, mri_dst) ;
  }

  MRIfree(&mri_tmp) ;

  /*MRIwrite(mri_dst, "soap.mnc") ;*/
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIsoapBubbleExpand(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,int niter)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, i,
          *pxi, *pyi, *pzi ;
  BUFTYPE *pctrl, ctrl, *ptmp ;
  MRI     *mri_tmp ;
  float   mean, nvox ;

  if (mri_src->type == MRI_FLOAT)
    return(mriSoapBubbleExpandFloat(mri_src, mri_ctrl, mri_dst, niter)) ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIcopy(mri_src, NULL) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  mri_tmp = MRIcopy(mri_dst, NULL) ;

  /* now propagate values outwards */
  for (i = 0 ; i < niter ; i++)
  {
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, "soap bubble expansion iteration %d of %d\n", i+1, niter) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        ptmp = &MRIvox(mri_tmp, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl++ ;
          if (ctrl == CONTROL_MARKED)   /* marked point - don't change it */
          {
            ptmp++ ;
            continue ;
          }
          /* now set this voxel to the average of the marked neighbors */
          mean = nvox = 0.0f ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                if (MRIvox(mri_ctrl, xi, yi, zi) == CONTROL_MARKED)
                {
                  mean += (float)MRIvox(mri_dst, xi, yi, zi) ;
                  nvox++ ;
                }
              }
            }
          }
          if (nvox)
          {
            *ptmp++ = (BUFTYPE)nint(mean / nvox) ;
            MRIvox(mri_ctrl, x, y, z) = CONTROL_TMP ;
          }
          else
            ptmp++ ;
        }
      }
    }
    MRIcopy(mri_tmp, mri_dst) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl ;
          if (ctrl == CONTROL_TMP)
            ctrl = CONTROL_MARKED ;
          *pctrl++ = ctrl ;
        }
      }
    }
  }

  MRIfree(&mri_tmp) ;

  /*MRIwrite(mri_dst, "soap.mnc") ;*/
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static MRI *
mriSoapBubbleFloat(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,int niter)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, i,
          *pxi, *pyi, *pzi ;
  BUFTYPE *pctrl, ctrl, mean ;
  float   *ptmp ;
  MRI     *mri_tmp ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIcopy(mri_src, NULL) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  mri_tmp = MRIcopy(mri_dst, NULL) ;

  /* now propagate values outwards */
  for (i = 0 ; i < niter ; i++)
  {
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, "soap bubble iteration %d of %d\n", i+1, niter) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        ptmp = &MRIFvox(mri_tmp, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl++ ;
          if (ctrl == CONTROL_MARKED)   /* marked point - don't change it */
          {
            ptmp++ ;
            continue ;
          }
          /* now set this voxel to the average of the marked neighbors */
          mean = 0.0f ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                mean += MRIFvox(mri_dst, xi, yi, zi) ;
              }
            }
          }
          *ptmp++ = (float)mean / (3.0f*3.0f*3.0f) ;
        }
      }
    }
    MRIcopy(mri_tmp, mri_dst) ;
  }

  MRIfree(&mri_tmp) ;

  /*MRIwrite(mri_dst, "soap.mnc") ;*/
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static MRI *
mriSoapBubbleShort(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,int niter)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, i,
          *pxi, *pyi, *pzi, num ;
  BUFTYPE *pctrl, ctrl ;
  short   *ptmp ;
  int     mean ;
  MRI     *mri_tmp ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIcopy(mri_src, NULL) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  mri_tmp = MRIcopy(mri_dst, NULL) ;
  
  for ( z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
      ptmp = &MRISvox(mri_tmp, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        ctrl = *pctrl++ ;
        if (ctrl == CONTROL_MARKED)
          continue ;
        num = mean = 0 ;
        for (zk = -1 ; zk <= 1 ; zk++)
        {
          zi = pzi[z+zk] ;
          for (yk = -1 ; yk <= 1 ; yk++)
          {
            yi = pyi[y+yk] ;
            for (xk = -1 ; xk <= 1 ; xk++)
            {
              xi = pxi[x+xk] ;
              if (MRIvox(mri_ctrl, xi, yi, zi) != CONTROL_MARKED)
                continue ;
              mean += (int)MRISvox(mri_dst, xi, yi, zi) ;
              num++ ;
            }
          }
        }
        if (num > 0)
          MRISvox(mri_dst, x, y, z) = (short)nint((float)mean / (float)num) ;
      }
    }
  }

  /* now propagate values outwards */
  for (i = 0 ; i < niter ; i++)
  {
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, "soap bubble iteration %d of %d\n", i+1, niter) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        ptmp = &MRISvox(mri_tmp, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl++ ;
          if (ctrl == CONTROL_MARKED)   /* marked point - don't change it */
          {
            ptmp++ ;
            continue ;
          }
          /* now set this voxel to the average of the marked neighbors */
          mean = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                mean += (int)MRISvox(mri_dst, xi, yi, zi) ;
              }
            }
          }
          *ptmp++ = (short)nint((float)mean / (3.0f*3.0f*3.0f)) ;
        }
      }
    }
    MRIcopy(mri_tmp, mri_dst) ;
  }

  MRIfree(&mri_tmp) ;

  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    MRIwrite(mri_dst, "soap.mgh") ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static MRI *
mriSoapBubbleExpandFloat(MRI *mri_src, MRI *mri_ctrl, MRI *mri_dst,int niter)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, i,
          *pxi, *pyi, *pzi ;
  BUFTYPE *pctrl, ctrl ;
  float   *ptmp, nvox, mean ;
  MRI     *mri_tmp ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIcopy(mri_src, NULL) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  mri_tmp = MRIcopy(mri_dst, NULL) ;

  /* now propagate values outwards */
  for (i = 0 ; i < niter ; i++)
  {
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
      fprintf(stderr, "soap bubble expansion iteration %d of %d\n", i+1, niter) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        ptmp = &MRIFvox(mri_tmp, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl++ ;
          if (ctrl == CONTROL_MARKED)   /* marked point - don't change it */
          {
            ptmp++ ;
            continue ;
          }
          /* now set this voxel to the average of the marked neighbors */
          nvox = mean = 0 ;
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                if (MRIvox(mri_ctrl, xi, yi, zi) == CONTROL_MARKED)
                {
                  mean += MRIFvox(mri_dst, xi, yi, zi) ;
                  nvox++ ;
                }
              }
            }
          }
          if (nvox)
          {
            *ptmp++ = (float)mean / nvox ;
            MRIvox(mri_ctrl, x, y, z) = CONTROL_TMP ;
          }
          else
            ptmp++ ;
        }
      }
    }
    MRIcopy(mri_tmp, mri_dst) ;
    for ( z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        pctrl = &MRIvox(mri_ctrl, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          ctrl = *pctrl ;
          if (ctrl == CONTROL_TMP)
            ctrl = CONTROL_MARKED ;
          *pctrl++ = ctrl ;
        }
      }
    }
  }

  MRIfree(&mri_tmp) ;

  /*MRIwrite(mri_dst, "soap.mnc") ;*/
  return(mri_dst) ;
}

static MRI *
mriMarkUnmarkedNeighbors(MRI *mri_src, MRI *mri_marked, 
                         MRI *mri_dst, int mark, int nbr_mark)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, *pxi, *pyi, 
          *pzi ;
  BUFTYPE *psrc, val ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        val = *psrc++ ;
        if (val == mark)   /* mark all neighbors */
        {
          for (zk = -1 ; zk <= 1 ; zk++)
          {
            zi = pzi[z+zk] ;
            for (yk = -1 ; yk <= 1 ; yk++)
            {
              yi = pyi[y+yk] ;
              for (xk = -1 ; xk <= 1 ; xk++)
              {
                xi = pxi[x+xk] ;
                if (MRIvox(mri_marked, xi, yi, zi) == CONTROL_NONE)
                  MRIvox(mri_dst, xi, yi, zi) = nbr_mark ;
              }
            }
          }
        }
      }
    }
  }
  return(mri_dst) ;
}

static int
mriRemoveOutliers(MRI *mri, int min_nbrs)
{
  int     width, height, depth, x, y, z, xk, yk, zk, xi, yi, zi, *pxi, *pyi, 
          *pzi, marked, nbrs, deleted ;
  BUFTYPE *pmarked ;

  width = mri->width ; height = mri->height ; depth = mri->depth ; 

  pxi = mri->xi ; pyi = mri->yi ; pzi = mri->zi ;

  for (deleted = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pmarked = &MRIvox(mri, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        marked = *pmarked ;
        if (!marked)
        {
          pmarked++ ;
          continue ;
        }

        /* check to see that at least min_nbrs nbrs are on */
        nbrs = -1 ;  /* so it doesn't count the central voxel */
        for (zk = -1 ; marked && zk <= 1 ; zk++)
        {
          zi = pzi[z+zk] ;
          for (yk = -1 ; marked && yk <= 1 ; yk++)
          {
            yi = pyi[y+yk] ;
            for (xk = -1 ; marked && xk <= 1 ; xk++)
            {
              xi = pxi[x+xk] ;
              if (MRIvox(mri, xi, yi, zi))
                nbrs++ ;
            }
          }
        }

        if (nbrs < min_nbrs)
        {
          deleted++ ;
          *pmarked++ = 0 ;
        }
        else
          pmarked++ ;
      }
    }
  }
  
  if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    fprintf(stderr, "%d control points deleted.\n", deleted) ;
  return(NO_ERROR) ;
}
#if 0
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          If any voxels are on in a 2x2x2 neighborhood, set the
          output value to it.
------------------------------------------------------*/
static MRI *
mriDownsampleCtrl2(MRI *mri_src, MRI *mri_dst)
{
  int     width, depth, height, x, y, z, x1, y1, z1 ;
  BUFTYPE *psrc, val ;
  
  if (mri_src->type != MRI_UCHAR)
    ErrorReturn(NULL, 
                (ERROR_UNSUPPORTED, "MRIdownsample2: source must be UCHAR"));

  width = mri_src->width/2 ;
  height = mri_src->height/2 ;
  depth = mri_src->depth/2 ;

  if (!mri_dst)
  {
    mri_dst = MRIalloc(width, height, depth, mri_src->type) ;
    MRIcopyHeader(mri_src, mri_dst) ;
  }

  MRIclear(mri_dst) ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        for (val = 0, z1 = 2*z ; !val && z1 <= 2*z+1 ; z1++)
        {
          for (y1 = 2*y ; !val && y1 <= 2*y+1 ; y1++)
          {
            psrc = &MRIvox(mri_src, 2*x, y1, z1) ;
              
            for (x1 = 2*x ; !val && x1 <= 2*x+1 ; x1++)
              val = *psrc++ ;
          }
        }
        MRIvox(mri_dst, x, y, z) = val ;
      }
    }
  }

  mri_dst->imnr0 = mri_src->imnr0 ;
  mri_dst->imnr1 = mri_src->imnr0 + mri_dst->depth - 1 ;
  mri_dst->xsize = mri_src->xsize*2 ;
  mri_dst->ysize = mri_src->ysize*2 ;
  mri_dst->zsize = mri_src->zsize*2 ;
  return(mri_dst) ;
}
#endif

int
MRI3dUseFileControlPoints(MRI *mri, char *fname)
{
  FILE *fp ;
  char *cp, line[STRLEN] ;
  int  i = 0 ;
  float xw, yw, zw ;
  int   xv, yv, zv ;

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "reading control points from %s...\n", fname) ;
  fp = fopen(fname, "r") ;
  if (!fp)
    ErrorReturn(ERROR_BADPARM, 
                (ERROR_BADPARM, "MRI3dUseFileControlPoints(%s): could not"
                 " open file", fname)) ;

  num_control_points = -1 ;
  do
  {
    cp = fgetl(line, 199, fp) ;
    num_control_points++ ;
    if (cp)
      i = sscanf(cp, "%f %f %f", &xw, &yw, &zw) ;
  } while (cp && i == 3) ;

  fprintf(stderr, "reading %d control points...\n", num_control_points) ;
  xctrl = (int *)calloc(num_control_points, sizeof(int)) ;
  yctrl = (int *)calloc(num_control_points, sizeof(int)) ;
  zctrl = (int *)calloc(num_control_points, sizeof(int)) ;
  if (!xctrl || !yctrl || !zctrl)
    ErrorExit(ERROR_NOMEMORY, 
              "MRI3dUseFileControlPoints: could not allocate %d-sized table",
              num_control_points) ;

  rewind(fp) ;
  for (i = 0 ; i < num_control_points ; i++)
  {
    cp = fgetl(line, 199, fp) ;
    sscanf(cp, "%f %f %f", &xw, &yw, &zw) ;
    MRIworldToVoxelIndex(mri, xw, yw, zw, &xv, &yv, &zv) ;
    xctrl[i] = xv ; yctrl[i] = yv ; zctrl[i] = zv ;
  }
  fclose(fp) ;
  return(NO_ERROR) ;
}

int
MRI3dWriteControlPoints(char *t_control_volume_fname)
{
  control_volume_fname = t_control_volume_fname ;
  return(NO_ERROR) ;
}
int
MRI3dWriteBias(char *t_bias_volume_fname)
{
  bias_volume_fname = t_bias_volume_fname ;
  return(NO_ERROR) ;
}

int
MRInormAddFileControlPoints(MRI *mri_ctrl, int value)
{
  int  i, nctrl ;

  /* read in control points from a file (if specified) */
  for (nctrl = i = 0 ; i < num_control_points ; i++)
  {
    /*    if (!MRIvox(mri_ctrl, xctrl[i], yctrl[i], zctrl[i]))*/
    {
      MRIvox(mri_ctrl, xctrl[i], yctrl[i], zctrl[i]) = value ;
      nctrl++ ;
    }
  }
  return(nctrl) ;
}
