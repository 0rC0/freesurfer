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
MRIhistoNormalize(MRI *mri_src, MRI *mri_norm, MRI *mri_template, int low)
{
  HISTOGRAM  h_fwd_eq, h_template_eq, h_norm ;

  HISTOclear(&h_fwd_eq, &h_fwd_eq) ;
  HISTOclear(&h_template_eq, &h_template_eq) ;
  HISTOclear(&h_norm, &h_norm) ;
  MRIgetEqualizeHisto(mri_src, &h_fwd_eq, low, 0) ;
  MRIgetEqualizeHisto(mri_template, &h_template_eq, low, 0) ;
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
              z0_tal, error ;
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
  error = MRItalairachToVoxel(mri, 0.0, 0.0, 0.0, &x0, &y0, &z0) ;
  x0_tal = nint(x0) ; y0_tal = nint(y0) ; z0_tal = nint(z0) ;
  if (error != NO_ERROR)
    ErrorReturn(error, 
          (error, "MRInormComputeWindows: could not find Talairach origin"));

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
    if (Gdiag & DIAG_SHOW)
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
      outputs[i-deleted] = (float)peak ;
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
    dy = inputs[slice] - inputs[old_slice] ;
    grad = fabs(dI / dy) ;
    deleted[slice] =  (grad > max_gradient) ;
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
    dy = inputs[slice] - inputs[old_slice] ;
    grad = fabs(dI / dy) ;
    deleted[slice] =  (grad > max_gradient) ;
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
