/*
 *       FILE NAME:   mrihisto.c
 *
 *       DESCRIPTION: utilities for MRI  data structure histograms
 *
 *       AUTHOR:      Bruce Fischl
 *       DATE:        1/8/97
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

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/

#define DEBUG_POINT(x,y,z)  (((x) == 15)&&((y)==6)&&((z)==15))

/*-----------------------------------------------------
                    STATIC DATA
-------------------------------------------------------*/

/*-----------------------------------------------------
                    STATIC PROTOTYPES
-------------------------------------------------------*/
static HISTOGRAM *mriHistogramRegion(MRI *mri, int nbins, HISTOGRAM *histo,
                                     MRI_REGION *region);
/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIapplyHistogram(MRI *mri_src, MRI *mri_dst, HISTOGRAM *histo)
{
  int       width, height, depth, x, y, z ;
  BUFTYPE   *psrc, *pdst, sval, dval ;

  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  width = mri_src->width ;
  height = mri_src->height ;
  depth = mri_src->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pdst = &MRIvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        sval = *psrc++ ;
        if (sval >= histo->nbins)
          sval = histo->nbins - 1 ;
        dval = histo->counts[sval] ;
        *pdst++ = dval ;
      }
    }
  }
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
HISTOGRAM *
MRIhistogramRegion(MRI *mri, int nbins, HISTOGRAM *histo, MRI_REGION *region)
{
  int               overlap ;
  float             fmin, fmax, bin_size ;
  BUFTYPE           bmin, bmax ;
  static MRI        *mri_prev = NULL ;
  static HISTOGRAM  h_prev ;
  static MRI_REGION reg_prev ;

#if 0
  if (mri->type != MRI_UCHAR)
    ErrorReturn(NULL, 
                (ERROR_UNSUPPORTED,"MRIhistogramRegion: must by type UCHAR"));
#endif

  fmin = MRIvalRange(mri, &fmin, &fmax) ;
  bmin = (BUFTYPE)fmin ; bmax = (BUFTYPE)fmax ;
  if (!nbins)
    nbins = bmax - bmin + 1 ;

  if (!histo)
    histo = HISTOalloc(nbins) ;
  else
    histo->nbins = nbins ;

  HISTOclear(histo, histo) ;
  bin_size = (fmax - fmin + 1) / (float)nbins ;

  if (!mri_prev)   /* first invocation, initialize state machine */
  {
    HISTOclear(&h_prev, &h_prev) ;
    REGIONclear(&reg_prev) ;
  }

/*
   note that the overlap only works with subsequent windows advancing only 
   in the x direction.
 */
  /* check to see if regions overlap */
  overlap = ((mri == mri_prev) &&
             (region->x > reg_prev.x) && 
             (region->y == reg_prev.y) &&
             (region->z == reg_prev.z)) ;

  if (overlap)   /* take advantage of overlapping regions */
  {
    MRI_REGION  reg_left, reg_right ;
    HISTOGRAM   histo_left, histo_right ;

    HISTOclear(&histo_left, &histo_left) ;
    HISTOclear(&histo_right, &histo_right) ;
    histo_left.nbins = histo_right.nbins = 256 ;
    REGIONcopy(&reg_prev, &reg_left) ;
    reg_left.dx = region->x - reg_left.x ;
    mriHistogramRegion(mri, 0, &histo_left, &reg_left) ;
    REGIONcopy(&reg_prev, &reg_right) ;
    reg_right.x = reg_prev.x + reg_prev.dx ;
    reg_right.dx = region->x + region->dx - reg_right.x ;
    mriHistogramRegion(mri, 0, &histo_right, &reg_right) ;

    HISTOsubtract(&h_prev, &histo_left, histo) ;
    HISTOadd(histo, &histo_right, histo) ;
  }
  else
    mriHistogramRegion(mri, nbins, histo, region) ;
  
  mri_prev = mri ;
  HISTOcopy(histo, &h_prev) ;
  REGIONcopy(region, &reg_prev) ;
  return(histo) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static HISTOGRAM *
mriHistogramRegion(MRI *mri, int nbins, HISTOGRAM *histo, MRI_REGION *region)
{
  int        width, height, depth, x, y, z, bin_no, x0, y0, z0 ;
  float      fmin, fmax, bin_size ;
  BUFTYPE    val, *psrc, bmin, bmax ;
  short      *spsrc ;

#if 0
  if (mri->type != MRI_UCHAR)
    ErrorReturn(NULL, 
                (ERROR_UNSUPPORTED,"mriHistogramRegion: must by type UCHAR"));
#endif

  if (mri->type == MRI_UCHAR)
  {
    fmin = 0 ; fmax = 255 ;
  }
  else
    fmin = MRIvalRange(mri, &fmin, &fmax) ;
  bmin = (BUFTYPE)fmin ; bmax = (BUFTYPE)fmax ;
  if (!nbins)
    nbins = bmax - bmin + 1 ;

  if (!histo)
    histo = HISTOalloc(nbins) ;
  else
    histo->nbins = nbins ;

  HISTOclear(histo, histo) ;

  bin_size = (fmax - fmin + 1) / (float)nbins ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  width = region->x + region->dx ;
  if (width > mri->width)
    width = mri->width ;
  height = region->y + region->dy ;
  if (height > mri->height)
    height = mri->height ;
  depth = region->z + region->dz ;
  if (depth > mri->depth)
    depth = mri->depth ;
  x0 = region->x ;
  if (x0 < 0)
    x0 = 0 ;
  y0 = region->y ;
  if (y0 < 0)
    y0 = 0 ;
  z0 = region->z ;
  if (z0 < 0)
    z0 = 0 ;

  for (bin_no = 0 ; bin_no < nbins ; bin_no++)
    histo->bins[bin_no] = (bin_no+1)*bin_size ;

  switch (mri->type)
  {
  case MRI_UCHAR:
    for (z = z0 ; z < depth ; z++)
    {
      for (y = y0 ; y < height ; y++)
      {
        psrc = &MRIvox(mri, x0, y, z) ;
        for (x = x0 ; x < width ; x++)
        {
          val = *psrc++ ;
          bin_no = (int)((float)(val - bmin) / (float)bin_size) ;
          histo->counts[bin_no]++ ;
        }
      }
    }
    break ;
  case MRI_SHORT:
    for (z = z0 ; z < depth ; z++)
    {
      for (y = y0 ; y < height ; y++)
      {
        spsrc = &MRISvox(mri, x0, y, z) ;
        for (x = x0 ; x < width ; x++)
        {
          bin_no = (int)((float)(*spsrc++ - bmin) / (float)bin_size) ;
          histo->counts[bin_no]++ ;
        }
      }
    }
    break ;
  default:
    ErrorReturn(NULL, 
                (ERROR_UNSUPPORTED, 
                 "mriHistogramRegion: unsupported mri type %d",
                 mri->type)) ;
    break ;
  }

    
  return(histo) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIhistoEqualizeRegion(MRI *mri_src, MRI *mri_dst, int low,MRI_REGION *region)
{
  HISTOGRAM  histo_eq ;

  MRIgetEqualizeHistoRegion(mri_src, &histo_eq, low, region, 1) ;
  mri_dst = MRIapplyHistogramToRegion(mri_src, mri_dst, &histo_eq, region) ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIapplyHistogramToRegion(MRI *mri_src, MRI *mri_dst, 
                          HISTOGRAM *histo, MRI_REGION *region)
{
  int       width, height, depth, x, y, z, x0, y0, z0 ;
  BUFTYPE   *psrc, *pdst, sval, dval ;

  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  width = region->x + region->dx ;
  if (width > mri_src->width)
    width = mri_src->width ;
  height = region->y + region->dy ;
  if (height > mri_src->height)
    height = mri_src->height ;
  depth = region->z + region->dz ;
  if (depth > mri_src->depth)
    depth = mri_src->depth ;
  x0 = region->x ;
  if (x0 < 0)
    x0 = 0 ;
  y0 = region->y ;
  if (y0 < 0)
    y0 = 0 ;
  z0 = region->z ;
  if (z0 < 0)
    z0 = 0 ;

  for (z = z0 ; z < depth ; z++)
  {
    for (y = y0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, x0, y, z) ;
      pdst = &MRIvox(mri_dst, x0, y, z) ;
      for (x = x0 ; x < width ; x++)
      {
        sval = *psrc++ ;
        if (sval >= histo->nbins)
          sval = histo->nbins - 1 ;
        dval = histo->counts[sval] ;
        *pdst++ = dval ;
      }
    }
  }
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
HISTOGRAM *
MRIgetEqualizeHistoRegion(MRI *mri, HISTOGRAM *histo_eq, int low, 
                          MRI_REGION *region, int norm)
{
  int       i, total, total_pix, *pc, *pdst, nbins ;
  HISTOGRAM histo ;


  if (mri->type != MRI_UCHAR)
    ErrorReturn(NULL,
                (ERROR_UNSUPPORTED, 
                 "MRIgetEqualizeHistoRegion: unsupported type %d",mri->type));
  MRIhistogramRegion(mri, 0, &histo, region) ;
  nbins = histo.nbins ;
  if (!histo_eq)
    histo_eq = HISTOalloc(nbins) ;
  else
  {
    histo_eq->nbins = nbins ;
    HISTOclear(histo_eq, histo_eq) ;
  }

  for (pc = &histo.counts[0], total_pix = 0, i = low ; i < nbins ; i++)
    total_pix += *pc++ ;

  if (total_pix) 
  {
    pc = &histo.counts[0] ;
    pdst = &histo_eq->counts[0] ;

    for (total = 0, i = low ; i < nbins ; i++)
    {
      total += *pc++ ;
      if (norm)
        *pdst++ = nint(255.0f * (float)total / (float)total_pix) ;
      else
        *pdst++ = total ;
    }
  }

  return(histo_eq) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
HISTOGRAM *
MRIgetEqualizeHisto(MRI *mri, HISTOGRAM *histo_eq, int low, int norm)
{
  int       i, total, total_pix ;
  HISTOGRAM *histo ;

  if (mri->type != MRI_UCHAR)
    ErrorReturn(NULL,
                (ERROR_UNSUPPORTED, "MRIgetEqualizeHisto: unsupported type %d",
                 mri->type)) ;
  histo = MRIhistogram(mri, 0) ;
  if (!histo_eq)
    histo_eq = HISTOalloc(histo->nbins) ;

  for (total_pix = 0, i = low ; i < histo->nbins ; i++)
    total_pix += histo->counts[i] ;

  for (total = 0, i = low ; i < histo->nbins ; i++)
  {
    total += histo->counts[i] ;
    if (norm)
      histo_eq->counts[i] = nint(255.0f * (float)total / (float)total_pix) ;
    else
      histo_eq->counts[i] = total ;
  }

  HISTOfree(&histo) ;
  return(histo_eq) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIhistoEqualize(MRI *mri_src, MRI *mri_dst, int low)
{
  HISTOGRAM  histo_eq ;

  MRIgetEqualizeHisto(mri_src, &histo_eq, low, 1) ;
  mri_dst = MRIapplyHistogram(mri_src, mri_dst, &histo_eq) ;
/*  MRIcrunch(mri_dst, mri_dst) ;*/
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIcrunch(MRI *mri_src, MRI *mri_dst)
{
  HISTOGRAM  *histo ;
  int        b, deleted ;

  if (mri_src->type != MRI_UCHAR)
    ErrorReturn(NULL,
                (ERROR_UNSUPPORTED, "MRIcrunch: unsupported type %d",
                 mri_src->type)) ;
  histo = MRIhistogram(mri_src, 0) ;

  deleted = 0 ;
  for (b = 0 ; b < histo->nbins ; b++)
  {
    if (!histo->counts[b])
      deleted++ ;
    histo->counts[b] = b-deleted ;
  }

  histo->nbins -= deleted ;
  mri_dst = MRIapplyHistogram(mri_src, mri_dst, histo) ;
  HISTOfree(&histo) ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
HISTOGRAM *
MRIhistogram(MRI *mri, int nbins)
{
  int        width, height, depth, x, y, z, bin_no ;
  HISTOGRAM  *histo ;
  float      fmin, fmax, bin_size ;
  BUFTYPE    val, *psrc, bmin, bmax ;
float fval;

  if (mri->type != MRI_UCHAR && mri->type != MRI_FLOAT)
    ErrorReturn(NULL, (ERROR_UNSUPPORTED,"MRIhistogram: must by type UCHAR or FLOAT"));

  fmin = MRIvalRange(mri, &fmin, &fmax) ;
  bmin = (BUFTYPE)fmin ; bmax = (BUFTYPE)fmax ;
  if (!nbins)
    nbins = bmax - bmin + 1 ;

  histo = HISTOalloc(nbins) ;

  bin_size = (fmax - fmin + 1) / (float)nbins ;
  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  for (bin_no = 0 ; bin_no < nbins ; bin_no++)
    histo->bins[bin_no] = (bin_no+1)*bin_size ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
  switch (mri->type)
  {
  case MRI_UCHAR:
/* 0 -> x */
          psrc = &MRIvox(mri, x, y, z) ;
          val = *psrc++ ;  
          bin_no = (int)((float)(val - bmin) / (float)bin_size) ;
          histo->counts[bin_no]++ ;
    break ;

  case MRI_SHORT:
    break ;

  case MRI_FLOAT:
    fval = MRIFvox(mri, x, y, z);
    bin_no = (int)((fval - fmin) / (float)bin_size);
    histo->counts[bin_no]++;
    break;

  default:
    break ;
        }
      }
    }
  }
  return(histo) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Remove small inconsistincies in the labelling of a volume.
------------------------------------------------------*/
#define MIN_VOXELS_PCT       0.15   /* peak must average 15% of max count */
#define PEAK_SEPARATION      11     /* min width of peak */
#define VALLEY_WIDTH         3
#define GRAY_LOW             65     /* lowest interesting gray matter */

#define SMOOTH_SIGMA         4.0f

#define X_DB2                 98
#define Y_DB2                 147
#define Z_DB2                 93

#define X_DB1                 106
#define Y_DB1                 158
#define Z_DB1                 137

#define X_DB                  157
#define Y_DB                  153
#define Z_DB                  128

MRI *
MRIhistoSegment(MRI *mri_src, MRI *mri_labeled, int wm_low, int wm_hi,
                int gray_hi, int wsize, float sigma)
{
  int        width, height, depth, x, y, z, whalf, in_val, label, nvox, 
             valley, wm_peak, gray_peak, thresh, nlabeled ;
  BUFTYPE    *pdst, *psrc ;
  MRI_REGION region ;
  HISTOGRAM  *histo, *hsmooth ;

  if (sigma < 0)
    sigma = SMOOTH_SIGMA ;

  thresh = (gray_hi + wm_low) / 2 ;
  histo = hsmooth = NULL ;
  width = mri_src->width ;
  height = mri_src->height ;
  depth = mri_src->depth ;
  if (!mri_labeled)
    mri_labeled = MRIclone(mri_src, NULL) ;

  whalf = wsize/2 ;
  region.dx = region.dy = region.dz = wsize ;

  if ((Gdiag & DIAG_WRITE) && DIAG_VERBOSE_ON)
  {
    MRIwrite(mri_labeled, "label.mnc") ;
    MRIwrite(mri_src, "src.mnc") ;
  }
  for (nlabeled = nvox = z = 0 ; z < depth ; z++)
  {
    DiagShowPctDone((float)z / (float)(depth-1), 5) ;
    for (y = 0 ; y < height ; y++)
    {
      pdst = &MRIvox(mri_labeled, 0, y, z) ;
      psrc = &MRIvox(mri_src, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == X_DB && y == Y_DB && z == Z_DB)
          DiagBreak() ;

        label = *pdst ;
        in_val = *psrc++ ;
        if (x == X_DB && y == Y_DB && z == Z_DB)
          DiagBreak() ;
        if (label != MRI_AMBIGUOUS)
        {
          pdst++ ;
          continue ;
        }
        nvox++ ;
        region.x = x-whalf ; region.y = y-whalf ; region.z = z-whalf ;
        histo = mriHistogramRegion(mri_src, 0, histo, &region) ;
        /*        HISTOclearBins(histo, histo, in_val-1, in_val+1) ;*/
        hsmooth = HISTOsmooth(histo, hsmooth, sigma) ;
#if 0
        HISTOclearBins(histo, histo, 0, GRAY_LOW-5) ;
        HISTOclearBins(histo, histo, wm_hi+5, 255) ;
#endif
        wm_peak = 
          HISTOfindLastPeakInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT, 
                                    wm_low/*GRAY_LOW*/, 
                                    wm_hi-PEAK_SEPARATION/2-2);
        gray_peak = 
          HISTOfindLastPeakInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT, 
                                    GRAY_LOW+PEAK_SEPARATION/2+2,
                                    wm_peak-PEAK_SEPARATION+1) ;
        while (gray_peak > gray_hi)  /* white matter is bimodal */
        {
          wm_peak = gray_peak ;
          gray_peak = 
            HISTOfindLastPeakInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT,
                                      GRAY_LOW+PEAK_SEPARATION/2+2,
                                      wm_peak-PEAK_SEPARATION+1) ;
        }

        if ((wm_peak < 0) || (gray_peak < 0))  /* unimodal - take best guess */
          valley = -1 /*thresh*/ ;
        else   /* bimodal, find min between peaks and use it as descriminant */
          valley = HISTOfindValley(hsmooth, VALLEY_WIDTH,
                                   gray_peak+VALLEY_WIDTH-1,
                                   wm_peak-VALLEY_WIDTH+1);
        if (valley > gray_hi)  /* can't be proper descriminant */
          valley = -1 ;
#if 0
        if (x == X_DB && y == Y_DB && z == Z_DB)
        {
          FILE *fp = fopen("histo.dat", "w") ;
          fprintf(fp, "histogram at (%d, %d, %d) = %d\n", x, y, z, in_val) ;
          fprintf(fp, "wm peak = %d, gray peak = %d, valley = %d\n",
                  wm_peak, gray_peak, valley) ;
          HISTOdump(histo, fp) ;
          fprintf(fp, "smooth histo:\n") ;
          HISTOdump(hsmooth, fp) ;
          fclose(fp) ;
          HISTOplot(hsmooth, "hsmooth.plt") ;
          HISTOplot(histo, "histo.plt") ;
        }
        if (x == X_DB1 && y == Y_DB1 && z == Z_DB1)
        {
          FILE *fp = fopen("histo1.dat", "w") ;
          fprintf(fp, "histogram at (%d, %d, %d) = %d\n", x, y, z, in_val) ;
          fprintf(fp, "wm peak = %d, gray peak = %d, valley = %d\n",
                  wm_peak, gray_peak, valley) ;
          HISTOdump(histo, fp) ;
          fprintf(fp, "smooth histo:\n") ;
          HISTOdump(hsmooth, fp) ;
          fclose(fp) ;
          HISTOplot(hsmooth, "hsmooth1.plt") ;
          HISTOplot(histo, "histo1.plt") ;
        }
#endif
#if 0
        if (valley < 0)
          valley = (wm_peak + gray_peak) / 2 ;  /* assume equal sigmas */
#endif
        if (valley < 0 || abs(in_val-valley)<=2)
          *pdst++ = MRI_AMBIGUOUS ;
        else if (valley >= gray_hi)
          *pdst++ = MRI_AMBIGUOUS ;
        else if (in_val >= valley)
        {
          nlabeled++ ;
          *pdst++ = MRI_WHITE ;
        }
        else
        {
          nlabeled++ ;
          *pdst++ = MRI_NOT_WHITE ;
        }
      }
    }
  }
  if (Gdiag & DIAG_SHOW)
  {
    fprintf(stderr, "              %8d voxels processed (%2.2f%%)\n",
            nvox, 100.0f*(float)nvox / (float)(width*height*depth)) ;
    fprintf(stderr, "              %8d voxels labeled (%2.2f%%)\n",
            nlabeled, 100.0f*(float)nlabeled / (float)(width*height*depth)) ;
  }
  HISTOfree(&histo) ;
  HISTOfree(&hsmooth) ;
  return(mri_labeled) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Remove small inconsistincies in the labelling of a volume.
------------------------------------------------------*/
MRI *
MRIhistoSegmentVoxel(MRI *mri_src, MRI *mri_labeled, int wm_low, int wm_hi,
                int gray_hi, int wsize, int x, int y, int z,
                     HISTOGRAM *histo, HISTOGRAM *hsmooth, float sigma)
{
  int        whalf, in_val, valley, wm_peak, gray_peak, thresh/*, npeaks,
             peaks[300]*/ ;
  MRI_REGION region ;
  float      sig ;

  thresh = (gray_hi + wm_low) / 2 ;

  whalf = wsize/2 ;
  region.dx = region.dy = region.dz = wsize ;

  in_val = MRIvox(mri_src, x, y, z) ;
  if (x == X_DB && y == Y_DB && z == Z_DB)
    DiagBreak() ;

  region.x = x-whalf ; region.y = y-whalf ; region.z = z-whalf ;
  histo = mriHistogramRegion(mri_src, 0, histo, &region) ;
  hsmooth = HISTOsmooth(histo, hsmooth, sigma) ;
  /*        HISTOclearBins(histo, histo, in_val-1, in_val+1) ;*/

#if 1
  sig = sigma ;
#else
  sig = 0.0f ;
  do
  {
    hsmooth = HISTOsmooth(histo, hsmooth, sigma) ;
    npeaks = 
      HISTOcountPeaksInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT, 
                                peaks, 5,
                                GRAY_LOW, wm_hi-PEAK_SEPARATION/2-2);
    if (npeaks != 2)
    {
      sig += 0.5f ;
      if (sig > 8.0f)
      {
        fprintf(stderr, "could not find two peaks\n") ;
        return(NULL) ;
      }
    }
  } while (npeaks != 2) ;
#endif

  HISTOclearBins(histo, histo, 0, GRAY_LOW-5) ;
  HISTOclearBins(histo, histo, wm_hi+5, 255) ;
  wm_peak = 
    HISTOfindLastPeakInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT, 
                              GRAY_LOW, wm_hi-PEAK_SEPARATION/2-2);
  gray_peak = 
    HISTOfindLastPeakInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT, 
                              GRAY_LOW+PEAK_SEPARATION/2+2,
                              wm_peak-PEAK_SEPARATION+1) ;
  while (gray_peak > gray_hi)  /* white matter is bimodal */
  {
    wm_peak = gray_peak ;
    gray_peak = 
      HISTOfindLastPeakInRegion(hsmooth, PEAK_SEPARATION, MIN_VOXELS_PCT,
                                GRAY_LOW+PEAK_SEPARATION/2+2,
                                wm_peak-PEAK_SEPARATION+1) ;
  }
  
  if ((wm_peak < 0) || (gray_peak < 0))  /* unimodal - take best guess */
    valley = -1 ; /* was thresh */
  else   /* bimodal, find min between peaks and use it as descriminant */
    valley = HISTOfindValley(hsmooth, VALLEY_WIDTH,gray_peak,wm_peak);

  {
    FILE *fp = fopen("histo.dat", "w") ;
    fprintf(fp, "histogram at (%d, %d, %d) = %d\n", x, y, z, in_val) ;
    fprintf(fp, "wm peak = %d, gray peak = %d, valley = %d, sigma = %2.2f\n",
            wm_peak, gray_peak, valley, sig) ;
    printf("histogram at (%d, %d, %d) = %d\n", x, y, z, in_val) ;
    printf("wm peak = %d, gray peak = %d, valley = %d\n",
            wm_peak, gray_peak, valley) ;
    HISTOdump(histo, fp) ;
    fprintf(fp, "smooth histo:\n") ;
    HISTOdump(hsmooth, fp) ;
    fclose(fp) ;
    HISTOplot(hsmooth, "hsmooth.plt") ;
    HISTOplot(histo, "histo.plt") ;
  }
  if (valley < 0)
    valley = -1 ; /*(wm_peak + gray_peak) / 2 ;*/  /* assume equal sigmas */
  if (valley < 0)
    MRIvox(mri_labeled, x, y, z) = MRI_AMBIGUOUS ;
  else if (in_val >= valley)
    MRIvox(mri_labeled, x, y, z) = MRI_WHITE ;
  else
    MRIvox(mri_labeled, x, y, z) = MRI_NOT_WHITE ;
  return(mri_labeled) ;
}
