#define SCALE_INVARIANT           1
#define USE_ORIGINAL_PROPERTIES   0
/*
 *       FILE NAME:   mrimorph.c
 *
 *       DESCRIPTION: utilities for 3d morph of one volume into another
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

#include "mrimorph.h"
#include "mri.h"
#include "mrinorm.h"
#include "histo.h"
#include "diag.h"
#include "error.h"
#include "macros.h"
#include "fio.h"
#include "proto.h"
#include "region.h"

#define EXP_K            10.0
#define IMAGE_SIZE       400
#define BACKGROUND_VAL   10

static MRI    *find_midline(MRI *mri_src, MRI *mri_thresh, float *px) ;
static int    find_spinal_fusion(MRI *mri_thresh, float *px, float *py,
                                 float *pz) ;
static int    mriLinearAlignPyramidLevel(MRI *mri_in, MRI *mri_ref, MP *parms);
static int    mri3DAlignPyramidLevel(MRI *mri_in, MRI *mri_ref, 
                                     MRI *mri_ref_blur, MP *parms,
                                     MORPH_3D *m3d);
static double mriIntensityRMS(MRI *mri_in, MRI *mri_ref, LTA *lta, 
                              double l_intensity) ;
static int    mriWriteImageViews(MRI *mri, char *base_name, int target_size) ;
static int    mriWriteImageView(MRI *mri, char *base_name, int target_size, 
                             int view, int slice) ;
static int    writeSnapshot(MRI *mri, MORPH_PARMS *parms, int n) ;
static int    write3DSnapshot(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms, 
                              MORPH_3D *m3d, int n) ;
static int    ltaGradientStep(MRI *mri_in, MRI *mri_ref, LTA *lta, double dt,
                              double momentum);
static int    openLogFile(MORPH_PARMS *parms) ;
static int    logIntegration(MORPH_PARMS *parms, int n, double rms) ;
static int    log3DIntegration(MORPH_PARMS *parms, MORPH_3D *m3d,
                               int n, double dt, double rms,
                               double intensity_rms, double distance_rms,
                               double area_rms) ;
static int     finitep(float f) ;
static int     mriNormalizeStds(MRI *mri) ;

#define MAX_LABELS   1000
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
HISTOGRAM *
MRIhorizontalHistogram(MRI *mri, int thresh_low, int thresh_hi)
{
  HISTOGRAM  *histo ;
  int        x, y, z, width, height, depth, npix, val ;
  
  histo = HISTOalloc(mri->height) ;

  width = mri->width ; height = mri->height ; depth = mri->depth ;

  for (y = 0 ; y < height ; y++)
  {
    if (y == Gdiag_no)
      DiagBreak() ;
    for (npix = z = 0 ; z < depth ; z++)
    {
      for (x = 0 ; x < width ; x++)
      {
        val =  MRIvox(mri, x, y, z) ;
        if (val >= thresh_low && val <= thresh_hi)
          npix++ ;
      }
    }
    histo->counts[y] = npix ;
    histo->bins[y] = y ;
  }
  return(histo) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
HISTOGRAM *
MRIhorizontalBoundingBoxHistogram(MRI *mri, int thresh)
{
  HISTOGRAM  *histo ;
  int        x, y, z, width, height, depth, xmin, xmax, zmin, zmax ;
  
  histo = HISTOalloc(mri->height) ;

  width = mri->width ; height = mri->height ; depth = mri->depth ;

  histo->nbins = 0 ;
  for (y = 0 ; y < height ; y++)
  {
    zmin = depth ; xmin = width ; zmax = 0 ; xmax = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (x = 0 ; x < width ; x++)
      {
        if (MRIvox(mri, x, y, z) >= thresh)
        {
          if (x < xmin)
            xmin = x ;
          if (z < zmin)
            zmin = z ;
          if (x > xmax)
            xmax = x ;
          if (z > zmax)
            zmax = z ;
        }
      }
    }
    if (zmin == depth)
      zmin = -1 ;
    histo->counts[y] = zmin ;
    if (histo->counts[y] >= 0 && y >= histo->nbins)
      histo->nbins = y ;
    histo->bins[y] = y ;
  }
  return(histo) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIcountAboveThreshold(MRI *mri, int thresh)
{
  int     x, y, z, width, height, depth, count ;
  BUFTYPE *psrc ;
  
  width = mri->width ; height = mri->height ; depth = mri->depth ;

  for (count = y = 0 ; y < height ; y++)
  {
    for (z = 0 ; z < depth ; z++)
    {
      psrc = &MRIvox(mri, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (*psrc++ >= thresh)
          count++ ;
      }
    }
  }
  return(count) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MRI *
MRIlabel(MRI *mri_src, MRI *mri_dst, int *pnlabels)
{
  int     l, x, y, z, width, height, depth, *pxi, *pyi, *pzi, xi, yi, zi,
          total_on, total_labeled, labeled, xk, yk, zk ;
  BUFTYPE *psrc, *plabel ;

  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  total_on = MRIcountAboveThreshold(mri_src, 1) ;
  /*  fprintf(stderr, "labeling %d voxels.\n", total_on) ;*/
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ;
  pxi = mri_src->xi ; pyi = mri_src->yi ; pzi = mri_src->zi ; 
  total_labeled = 0 ;
  for (l = 1 ; total_labeled < total_on ; l++)
  {
    /* first find an unlabeled 'on' voxel and label it to start a new seed */
    for (labeled = z = 0 ; !labeled && z < depth ; z++)
    {
      for (y = 0 ; !labeled && y < height ; y++)
      {
        psrc = &MRIvox(mri_src, 0, y, z) ;
        plabel = &MRIvox(mri_dst, 0, y, z) ;
        for (x = 0 ; !labeled && x < width ; x++)
        {
          if (*psrc++ && !*plabel)  /* unlabeled on voxel */
          {
            *plabel = l ;
            labeled = 1 ;
            total_labeled++ ;
          }
          else
            plabel++ ;
        }
      }
    }
    do
    {
      labeled = 0 ;
      for (z = 0 ; z < depth ; z++)
      {
        for (y = 0 ; y < height ; y++)
        {
          plabel = &MRIvox(mri_dst, 0, y, z) ;
          for (x = 0 ; x < width ; x++)
          {
            if (*plabel++ == l)  /* current label */
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
                    if (MRIvox(mri_src,xi,yi,zi) && !MRIvox(mri_dst,xi,yi,zi))
                    {
                      MRIvox(mri_dst,xi,yi,zi) = l ;
                      total_labeled++ ;
                      labeled++ ;
                    }
                  }
                }
              }
            }
          }
        }
      }
    } while (labeled > 0) ;
    /*    fprintf(stderr, "\r%d labels found     ", l) ;*/
  }
  /*  fprintf(stderr, "\n") ;*/
  *pnlabels = l ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIlabelBoundingBoxes(MRI *mri, MRI_REGION *bboxes,int nlabels)
{
  int     l, x, y, z, width, height, depth, x0, x1, y0, y1, z0, z1 ;
  BUFTYPE *plabel ;

  width = mri->width ; height = mri->height ; depth = mri->depth ;
  for (l = 1 ; l < nlabels ; l++)
  {
    x0 = width ; y0 = height ; z0 = depth ; x1 = y1 = z1 = 0 ;
    for (z = 0 ; z < depth ; z++)
    {
      for (y = 0 ; y < height ; y++)
      {
        plabel = &MRIvox(mri, 0, y, z) ;
        for (x = 0 ; x < width ; x++)
        {
          if (*plabel++ == l)
          {
            if (x < x0)
              x0 = x ;
            if (x > x1)
              x1 = x ;
            if (y < y0)
              y0 = y ;
            if (y > y1)
              y1 = y ;
            if (z < z0)
              z0 = z ;
            if (z > z1)
              z1 = z ;
          }
        }
      }
    }
    x0 *= mri->thick ; y0 *= mri->thick ; z0 *= mri->thick ;
    x1 *= mri->thick ; y1 *= mri->thick ; z1 *= mri->thick ;
    bboxes[l].x = x0 ; bboxes[l].dx = x1 - x0 + 1 ;
    bboxes[l].y = y0 ; bboxes[l].dy = y1 - y0 + 1 ;
    bboxes[l].z = z0 ; bboxes[l].dz = z1 - z0 + 1 ;
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIeraseOtherLabels(MRI *mri_src, MRI *mri_dst, int label)
{
  int     x, y, z, width, height, depth ;
  BUFTYPE *psrc, *pdst ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pdst = &MRIvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (*psrc++ != label)
          *pdst++ = 0 ;
        else
          *pdst++ = label ;
      }
    }
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIeraseLabel(MRI *mri_src, MRI *mri_dst, int label)
{
  int     x, y, z, width, height, depth ;
  BUFTYPE *psrc, *pdst, l ;

  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri_src, 0, y, z) ;
      pdst = &MRIvox(mri_dst, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        l = *psrc++ ;
        if (l == label)
          *pdst++ = 0 ;
        else
          *pdst++ = l ;
      }
    }
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIfindHorizontalLabelLimits(MRI *mri, int label, int *xmins, int *xmaxs)
{
  int     x, y, z, width, height, depth, ymin ;
  BUFTYPE *psrc ;

  width = mri->width ; height = mri->height ; depth = mri->depth ;

  for (ymin = -1, y = 0 ; y < height ; y++)
  {
    if (ymin >= 0)
    {
      xmins[y-ymin] = width ;
      xmaxs[y-ymin] = -1 ;
    }
    for (z = 0 ; z < depth ; z++)
    {
      psrc = &MRIvox(mri, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (*psrc++ == label)
        {
          if (ymin < 0)
          {
            xmins[0] = xmaxs[0] = x ;
            ymin = y ;
          }
          if (x > xmaxs[y-ymin])
            xmaxs[y-ymin] = x ;
          if (x < xmins[y-ymin])
            xmins[y-ymin] = x ;
        }
      }
    }
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define MIN_SPINAL_ASPECT .75    /* will eliminate corpus callosum */

/* provide some very broad bounds on where it can be and what size it is */
#define MIN_SPINAL_X       80
#define MAX_SPINAL_X      160
#define MIN_SPINAL_Y       85
#define MAX_SPINAL_Y      260   /* can extend to bottom of image */
#define MIN_SPINAL_AREA   190

#define M3D_VERSION       1

MRI *
MRIremoveNeck(MRI *mri_src,MRI *mri_dst,int thresh_low,int thresh_hi,MP *parms,
              int dir)
{
  MRI        *mri_label, *mri_thresh, *mri_midline, *mri_rot ;
  int        nlabels, l, max_dy, spinal_label, xmins[256], xmaxs[256], y ;
  MRI_REGION bboxes[MAX_LABELS], *spinal_bbox, in_bbox ;
  float      n, len, avg_dy, x0, y0, z0, xmid, avg_dz, aspect,
             areas[MAX_LABELS], thick ;
  MATRIX     *m_rot, *m_trans, *m_L, *m_inv ;
  VECTOR     *v ;
  double     xa ;
  static int callno = 0 ;

  v = VectorAlloc(4, MATRIX_REAL) ; 
  thick = mri_src->thick ;
  callno++ ;   /* for diagnostics */
  MRIboundingBox(mri_src, 80, &in_bbox) ;

  /* first find a sagittal slice at the midline */
  mri_thresh = MRIthresholdRangeInto(mri_src, NULL, thresh_low, thresh_hi) ;
  MRIopen(mri_thresh, mri_thresh) ;
  mri_midline = find_midline(mri_src, mri_thresh, &xmid) ;
  if (!mri_dst)
    mri_dst = MRIcopy(mri_src, NULL) ;

  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
  {
    char fname[100] ;
    sprintf(fname, "midline%d.mgh", callno) ;
    MRIwrite(mri_midline, fname) ;
  }
  mri_label = MRIlabel(mri_midline, NULL, &nlabels) ;
  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
  {
    char fname[100] ;
    sprintf(fname, "label%d.mgh", callno) ;
    MRIwrite(mri_label, fname) ;
  }

  /* find the label with the biggest y extent, it will be spinal cord */
  /* note that bboxes are in mm which may NOT be the same as voxels */
  MRIlabelBoundingBoxes(mri_label, bboxes, nlabels) ;
  MRIlabelAreas(mri_label, areas, nlabels) ;  
  spinal_label = -1 ;
  for (l = 1, max_dy = 0 ; l < nlabels ; l++)
  {
    aspect = (float)bboxes[l].dy / (float)bboxes[l].dx ;
    if (aspect > MIN_SPINAL_ASPECT && 
        bboxes[l].x >= MIN_SPINAL_X &&
        bboxes[l].x+bboxes[l].dx-1 <= MAX_SPINAL_X &&
        bboxes[l].y >= MIN_SPINAL_Y &&
        bboxes[l].y+bboxes[l].dy-1 <= MAX_SPINAL_Y &&
        areas[l] >= MIN_SPINAL_AREA &&
        bboxes[l].dy >= max_dy)
    {
      /* candidate for spinal chord */
      spinal_label = l ;
      max_dy = bboxes[l].dy ;
    }
  }
  
  if (spinal_label < 0)
    ErrorReturn(NULL, 
                (ERROR_BADPARM, "MRIremoveNeck: could not find spinal chord"));

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"spinal label %d, area = %2.0f, "
            "box = (%d, %d) --> (%d, %d)\n",
            spinal_label, areas[spinal_label],
            bboxes[spinal_label].x, bboxes[spinal_label].y,
            bboxes[spinal_label].x+bboxes[spinal_label].dx-1,
            bboxes[spinal_label].y+bboxes[spinal_label].dy-1) ;
  MRIeraseOtherLabels(mri_label, mri_label, spinal_label) ;
  MRIfindHorizontalLabelLimits(mri_label, spinal_label, xmins, xmaxs) ;
  spinal_bbox = &bboxes[spinal_label] ;

  /* calculate avg dz, skip 1st 6 mm or so which typically contain the pons */
  for (y = 6/thick, n = avg_dz = 0.0f ; y < spinal_bbox->dy/thick-1 ; y++)
  {
    avg_dz += xmins[y] - xmins[y-1] ;
    n++ ;
  }
  avg_dz /= n ; len = sqrt(1 + avg_dz*avg_dz) ; 
  avg_dz /= len ; avg_dy = 1.0f / len ;

  /* now build a rotation matrix which will orient the brain so that
     the spinal cord is vertical.
  */
  x0 = (float)(spinal_bbox->x + spinal_bbox->dx-1)/2 ;
  y0 = (float)(spinal_bbox->y + spinal_bbox->dy-1)/2 ;
  z0 = (float)(spinal_bbox->z + spinal_bbox->dz-1)/2 ;
  xa = acos(avg_dy) ; 
  if (avg_dz > 0.0f) 
    xa *= -1.0f ;
  m_trans = MatrixIdentity(4, NULL) ;
  *MATRIX_RELT(m_trans, 1, 4) = -x0 * mri_src->thick ;
  *MATRIX_RELT(m_trans, 2, 4) = -y0 * mri_src->thick ;
  *MATRIX_RELT(m_trans, 3, 4) = -z0 * mri_src->thick ;
  m_rot = MatrixAllocRotation(4, xa, X_ROTATION) ;
  m_L = MatrixMultiply(m_rot, m_trans, NULL) ;
  *MATRIX_RELT(m_trans, 1, 4) = x0 * mri_src->thick ;
  *MATRIX_RELT(m_trans, 2, 4) = y0 * mri_src->thick ;
  *MATRIX_RELT(m_trans, 3, 4) = z0 * mri_src->thick ;
  MatrixMultiply(m_trans, m_L, m_L) ;

  mri_rot = MRIlinearTransform(mri_src, NULL, m_L) ;
  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
  {
    char fname[100] ;
    sprintf(fname, "rot%d.mgh", callno) ;
    MRIwrite(mri_rot, fname) ;
  }
  MRIthresholdRangeInto(mri_rot, mri_thresh, thresh_low, thresh_hi) ;
  MRIopen(mri_thresh, mri_thresh) ;
  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
  {
    char fname[100] ;
    sprintf(fname, "thresh%d.mgh", callno) ;
    MRIwrite(mri_thresh, fname) ;
  }
  MRIboundingBox(mri_rot, 80, &in_bbox) ;
  
  /* find coordinates for cutting plane in rotated system */
  find_spinal_fusion(mri_thresh, &x0, &y0, &z0) ;
  if (!parms)  /* 1 cm below for morph. HACK - should be fixed */
    y0 += 10.0f/thick ;

  /* transform them into original voxel coordinates */
  VECTOR_ELT(v,4) = 1.0/mri_thresh->thick ;
  V3_X(v) = x0 ; V3_Y(v) = y0 ; V3_Z(v) = z0 ; 
  m_inv = MatrixSVDInverse(m_L, NULL) ;
  MatrixMultiply(m_inv, v, v) ;
  x0 = V3_X(v) ; y0 = V3_Y(v) ; z0 = V3_Z(v) ;

#if 0
  /* now erase the neck */
  x0 = (float)xmid ; 
  z0 = ((float)in_bbox.z+(float)in_bbox.dz/2.0f) / thick ;
  if (!parms)  /* 1 cm below for morph. HACK - should be fixed */
    y0 += 10.0f/thick ;
#endif
  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
  {
    char fname[100] ;
    sprintf(fname, "unerased%d.mgh", callno) ;
    fprintf(stderr, "writing volume before erasure to %s\n", fname) ;
    MRIwrite(mri_dst, fname) ;
  }
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, 
            "erasing neck around (%2.1f, %2.1f, %2.1f), "
            "n = (%2.2f,%2.2f, %2.2f)\n",
            thick*x0, thick*y0, thick*z0, 0.0f, avg_dy, avg_dz) ;

  /* plane may be inclined, so have to continue off end of image */
  for (y = y0 ; y < mri_dst->height+25 ; y++)
    MRIerasePlane(mri_dst, x0, (float)y, z0, 0.0f, avg_dy, avg_dz, 0) ;

  if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
  {
    char fname[100] ;
    sprintf(fname, "erased%d.mgh", callno) ;
    fprintf(stderr, "writing volume after erasure to %s\n", fname) ;
    MRIwrite(mri_dst, fname) ;
  }

  if (parms)
  {
    MATRIX *parms_m_L = parms->lta->xforms[0].m_L ;

    /*
      dir > 0 means that this is the template image, so that the 
      transformation is forward while dir < 0 means that this is an
      input image, and the transformation is inverted.
    */
    if (dir > 0)
      MatrixMultiply(parms_m_L, m_L, parms_m_L);
    else
      MatrixMultiply(parms_m_L, m_inv, parms_m_L);
  }
  MatrixFree(&m_trans) ; MatrixFree(&m_rot) ; MatrixFree(&m_L) ;
  VectorFree(&v) ;
  MRIfree(&mri_label) ; MRIfree(&mri_thresh) ; MRIfree(&mri_rot) ;
  MatrixFree(&m_inv) ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#ifdef WSIZE
#undef WSIZE
#endif
#define WSIZE   21
#define WHALF   ((WSIZE-1)/2)
static MRI *
find_midline(MRI *mri_src, MRI *mri_thresh, float *px)
{
  MRI         *mri_dst ;
  MRI_REGION  bbox ;
  int         count, min_count, x, x0, xmid ;

  MRIboundingBox(mri_src, 80, &bbox) ;

  xmid = x0 = bbox.x + bbox.dx/2 ; 
  min_count = mri_thresh->width*mri_thresh->height;
  for (mri_dst = NULL, x = x0-WHALF ; x <= x0+WHALF ; x++)
  {
    mri_dst = MRIextractPlane(mri_thresh, mri_dst, MRI_SAGITTAL, x) ;
    count = MRIcountAboveThreshold(mri_dst, 2) ;
    if (count < min_count)
    {
      min_count = count ;
      xmid = x ;
      /*      fprintf(stderr, "new min %d found at %d\n", min_count, xmid) ;*/
    }
  }

  MRIextractPlane(mri_thresh, mri_dst, MRI_SAGITTAL, xmid) ;
  *px = (float)xmid ;
  return(mri_dst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           Compute the centroid of a label (in mm coordinates)
------------------------------------------------------*/
int
MRIlabelCentroid(MRI *mri, int target_l, float *px, float *py, float *pz)
{
  int     l, x, y, z, width, height, depth, nvox ;
  BUFTYPE *plabel ;
  float   thick, x0, y0, z0 ;

  thick = mri->thick ;
  width = mri->width ; height = mri->height ; depth = mri->depth ;
  for (x0 = y0 = z0 = 0.0f, nvox = z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      plabel = &MRIvox(mri, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        l = *plabel++ ;
        if (l == target_l)
        {
          x0 += (float)x*thick ; y0 += (float)y*thick ; z0 += (float)z*thick ;
          nvox++ ;
        }
      }
    }
  }

  if (nvox)
  { *px = x0/nvox ; *py = y0/nvox ; *pz = z0/nvox ; }
  else
   *px = *py = *pz = 0.0f ;

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute the area of all labels (in mm^3)
------------------------------------------------------*/
int
MRIlabelAreas(MRI *mri, float *areas, int nlabels) 
{
  int     l, x, y, z, width, height, depth ;
  BUFTYPE *plabel ;
  float   voxel_area ;

  voxel_area = mri->xsize * mri->ysize * mri->zsize ;
  width = mri->width ; height = mri->height ; depth = mri->depth ;
  for (l = 0 ; l < nlabels ; l++)
    areas[l] = 0 ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      plabel = &MRIvox(mri, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        l = *plabel++ ;
        areas[l] += voxel_area ;
      }
    }
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#undef MIN_SPINAL_X     
#undef MAX_SPINAL_X
#undef MIN_SPINAL_Y
#undef MAX_SPINAL_Y
#undef MIN_SPINAL_ASPECT
#undef MAX_SPINAL_ASPECT
#undef MIN_SPINAL_AREA

#define MIN_SPINAL_AREA          600.0f
#define MIN_SPINAL_AREA_RATIO    3.0f
#define MIN_SPINAL_ASPECT        .5
#define MAX_SPINAL_ASPECT       1.5

/* bounds on spatial extent */
#define MIN_SPINAL_X       60
#define MAX_SPINAL_X      180
#define MIN_SPINAL_Y       50
#define MAX_SPINAL_Y      170

/* bounds on centroid position */
#define MAX_SPINAL_WIDTH  80
#define MIN_SPINAL_CX     (128-MAX_SPINAL_WIDTH/2)
#define MAX_SPINAL_CX     (128+MAX_SPINAL_WIDTH/2)


static int
find_spinal_fusion(MRI *mri_thresh, float *px, float *py, float *pz)
{
  MRI    *mri_label, *mri_slice ;
  float  areas[MAX_LABELS], spinal_area, aspect, corner_dist, thick,x0,y0,z0 ;
  int    y, found, nlabels, l, spinal_l = 0 ;
  MRI_REGION bboxes[MAX_LABELS], *bbox ;

  thick = mri_thresh->thick ;

  /*
    search from the bottom of the MRI upwards looking for the point at
    which the two branches of the spinal chord fuse together. This will
    be detectable using a bunch of hacky model-based parameters - where 
    it is, when it appears (detected by increase in area), no other big stuff
    near it, in approximately the right place, with reasonable aspect 
    ratio. Note that all these parameters are very loose.
  */
  for (found = 0, y = mri_thresh->height-1 ; y >= 0 && !found ; y--)
  {
    if (y < 160)
      DiagBreak() ;
    mri_slice = MRIextractPlane(mri_thresh, NULL, MRI_HORIZONTAL, y) ;
    mri_label = MRIlabel(mri_slice, NULL, &nlabels) ;

    /* first erase labels that are not in possible regions */
    MRIlabelBoundingBoxes(mri_label, bboxes, nlabels) ;
    for (l = 1 ; l < nlabels ; l++)
    {
      if ((bboxes[l].x < MIN_SPINAL_X) ||
          (bboxes[l].x+bboxes[l].dx-1 > MAX_SPINAL_X) ||
          (bboxes[l].y < MIN_SPINAL_Y) ||
          (bboxes[l].y+bboxes[l].dy-1 > MAX_SPINAL_Y))
        MRIeraseLabel(mri_label, mri_label, l) ;
      else
      {
        MRIlabelCentroid(mri_label, l, &x0, &y0, &z0) ;
        if (x0 < MIN_SPINAL_CX || x0 > MAX_SPINAL_CX)
          MRIeraseLabel(mri_label, mri_label, l) ;
      }
    }
    MRIlabelAreas(mri_label, areas, nlabels) ;  /* compute remaining areas */
    for (spinal_area = 0, l = 1 ; !found && l < nlabels ; l++)
    {
      aspect = (float)bboxes[l].dy / bboxes[l].dx ;
      if ((aspect < MIN_SPINAL_ASPECT) || (aspect > MAX_SPINAL_ASPECT))
        continue ;
      if (areas[l] >= spinal_area) /* find remaining label with biggest area */
      {
        spinal_l = l ;
        spinal_area = areas[l] ;
      }
    }

    /* make sure it is much bigger than anything else close to it */
    if (spinal_area > MIN_SPINAL_AREA)
    {
#if 0
      for (l = 1 ; l < nlabels ; l++)
        fprintf(stderr, "slice %d, areas[%d] = %2.1f\n", y, l, areas[l]) ;
#endif
      for (found = l = 1 ; found && l < nlabels ; l++)
      {
        if ((l == spinal_l) || FZERO(areas[l]))
          continue ;
        corner_dist = REGIONminCornerDistance(&bboxes[l], &bboxes[spinal_l]) ;
        if (corner_dist < 25 && (spinal_area / areas[l])<MIN_SPINAL_AREA_RATIO)
          found = 0 ;
      }
    }
    if (found && Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
    {
      char fname[100] ;
      sprintf(fname, "spinal_label%d.mgh", y) ;
      MRIwrite(mri_label, fname) ;
    }
    if (found)
      DiagBreak() ;
    MRIfree(&mri_label) ; MRIfree(&mri_slice) ;
    if (found)  /* don't decrement y again */
      break ;
  }

  /* convert bboxes to voxel coordinates */
  bbox = &bboxes[spinal_l] ;
  bbox->x /= thick ; bbox->dx /= thick ; bbox->y /= thick ; bbox->dy /= thick ;

  /* pick midpoint of label at appropriate slice */
  *px = (float)(bbox->x + bbox->dx/2) ;
  *py = (float)y ;
  *pz = (float)(bbox->y + bbox->dy/2) ;
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "spinal fusion found at slice %d (%2.0f,%2.0f,%2.0f)\n", 
            y, *px, *py, *pz) ;
  return(y) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIinitTranslation(MRI *mri_in, MRI *mri_ref, MATRIX *m_L)
{
  MATRIX *m_translation ;
  float  in_means[4], ref_means[4] ;
  double dx, dy, dz ;

  m_translation = MatrixIdentity(4, NULL) ;
  MRIfindCenterOfBrain(mri_in, in_means, in_means+1, in_means+2) ;
  MRIfindCenterOfBrain(mri_ref, ref_means, ref_means+1, ref_means+2) ;
  dx = (double)(ref_means[0] - in_means[0]) * mri_in->thick ;
  dy = (double)(ref_means[1] - in_means[1]) * mri_in->thick ;
  dz = (double)(ref_means[2] - in_means[2]) * mri_in->thick ;
  if (Gdiag & DIAG_SHOW)
  {
    fprintf(stderr, "centering template around (%d,%d,%d) and input around"
            " (%d,%d,%d)\n", 
            (int)ref_means[0], (int)ref_means[1], (int)ref_means[2],
            (int)in_means[0], (int)in_means[1], (int)in_means[2]) ;
    fprintf(stderr, "initial translation = (%2.0f, %2.0f, %2.0f).\n",dx,dy,dz);
  }
  *MATRIX_RELT(m_translation, 1, 4) = dx ;
  *MATRIX_RELT(m_translation, 2, 4) = dy ;
  *MATRIX_RELT(m_translation, 3, 4) = dz ;
  MatrixMultiply(m_translation, m_L, m_L) ;
  MatrixFree(&m_translation) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define MAX_DX   1.2
#define MAX_DY   1.2
#define MAX_DZ   1.2
#define MIN_DX   (1.0/MAX_DX)
#define MIN_DY   (1.0/MAX_DY)
#define MIN_DZ   (1.0/MAX_DZ)

int
MRIinitScaling(MRI *mri_in, MRI *mri_ref, MATRIX *m_L)
{
  MATRIX      *m_scaling ;
  float       sx, sy, sz, dx, dy, dz ;
  MRI_REGION  in_bbox, ref_bbox ;

  m_scaling = MatrixIdentity(4, NULL) ;

  MRIboundingBox(mri_in, 120, &in_bbox) ;
  MRIboundingBox(mri_ref, 120, &ref_bbox) ;
  sx = (float)ref_bbox.dx / (float)in_bbox.dx ;
  sy = (float)ref_bbox.dy / (float)in_bbox.dy ;
  sz = (float)ref_bbox.dz / (float)in_bbox.dz ;
  dx = (ref_bbox.x+ref_bbox.dx-1)/2 - (in_bbox.x+in_bbox.dx-1)/2 ;
  dy = (ref_bbox.y+ref_bbox.dy-1)/2 - (in_bbox.y+in_bbox.dy-1)/2 ;
  dz = (ref_bbox.z+ref_bbox.dz-1)/2 - (in_bbox.z+in_bbox.dz-1)/2 ;
  
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "initial scaling: (%2.2f, %2.2f, %2.2f) <-- "
            "(%d/%d,%d/%d,%d/%d)\n", 
            sx,sy,sz, ref_bbox.dx, in_bbox.dx, ref_bbox.dy, in_bbox.dy, 
            ref_bbox.dz, in_bbox.dz) ;
  if (sx > MAX_DX || sx < MIN_DX) sx = 1.0 ;
  if (sy > MAX_DY || sy < MIN_DY) sy = 1.0 ;
  if (sz > MAX_DZ || sz < MIN_DZ) sz = 1.0 ;
  *MATRIX_RELT(m_scaling, 1, 1) = sx ;
  *MATRIX_RELT(m_scaling, 2, 2) = sy ;
  *MATRIX_RELT(m_scaling, 3, 3) = sz ;

#if 0
  *MATRIX_RELT(m_L, 1, 4) = dx ;
  *MATRIX_RELT(m_L, 2, 4) = dy ;
  *MATRIX_RELT(m_L, 3, 4) = dz ;
#endif
  MatrixMultiply(m_scaling, m_L, m_L) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define MAX_LEVELS 10
#define MIN_PYR_WIDTH 16
#define USE_PYRAMID 1
int
MRIlinearAlign(MRI *mri_in, MRI *mri_ref, MP *parms)
{
#if USE_PYRAMID
  int    nlevels, max_levels, i ;
  MRI    *mri_in_pyramid[MAX_LEVELS], *mri_ref_pyramid[MAX_LEVELS] ;
#endif
  double rms ;
  char   base_name[100] ;
  MRI    *mri_tmp ;

  mriNormalizeStds(mri_ref) ;
  if (DZERO(parms->dt))
    parms->dt = 1e-6 ;

  if (parms->lta->num_xforms == 1)  /* one (global) transformation */
  {
    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    {
      rms = mriIntensityRMS(mri_in, mri_ref, parms->lta, 1.0f) ;
      fprintf(stderr, "before initial alignment rms = %2.3f\n", rms) ;
    }
    mri_tmp = LTAtransform(mri_in, NULL, parms->lta) ;
    MRIinitTranslation(mri_tmp, mri_ref, parms->lta->xforms[0].m_L) ;
    MRIinitScaling(mri_tmp, mri_ref, parms->lta->xforms[0].m_L) ;
    LTAtransform(mri_in, mri_tmp, parms->lta) ;
    MRIinitTranslation(mri_tmp, mri_ref, parms->lta->xforms[0].m_L) ;
    MRIfree(&mri_tmp) ;

    if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
    {
      rms = mriIntensityRMS(mri_in, mri_ref, parms->lta, 1.0f) ;
      fprintf(stderr, "after initial alignment  rms = %2.3f\n", rms) ;
    }
  }

  strcpy(base_name, parms->base_name) ;
  openLogFile(parms) ;
#if !USE_PYRAMID
  {
    double dt, sigma ;
    MRI     *mri_in_blur = NULL, *mri_ref_blur = NULL, *mri_kernel;

  dt = parms->dt ;
  for (sigma = 4.0f ; sigma >= 1.0f ; sigma /= 2.0f)
  {
    parms->dt = sigma > 1 ? dt * sigma : dt ;
    /*    sprintf(parms->base_name, "level%d_", nint(sigma)) ;*/
    if (sigma > 0.0f)
    {
      mri_kernel = MRIgaussian1d(sigma, 17) ;
      fprintf(stderr, "blurring volumes with sigma = %2.1f...", sigma) ;
      mri_in_blur = MRIconvolveGaussian(mri_in, mri_in_blur, mri_kernel) ;
      mri_ref_blur = 
        MRIconvolveGaussianMeanAndStdByte(mri_ref, mri_ref_blur, mri_kernel) ;
      fprintf(stderr, "done.\n") ;
      MRIfree(&mri_kernel) ;
      mriLinearAlignPyramidLevel(mri_in_blur, mri_ref_blur, parms) ;
    }
    else
      mriLinearAlignPyramidLevel(mri_in, mri_ref, parms) ;
  }
  parms->dt = dt ;
  MRIfree(&mri_in_blur) ; MRIfree(&mri_ref_blur) ;
  }
#else
  if (parms->lta->num_xforms == 1)
    max_levels = MAX_LEVELS ;
  else
    max_levels = 2 ;

  /* build Gaussian pyramid */
  mri_in_pyramid[0] = mri_in ; mri_ref_pyramid[0] = mri_ref ;
  for (nlevels = 1 ; nlevels < max_levels ; nlevels++)
  {
    if (mri_in_pyramid[nlevels-1]->width <= MIN_PYR_WIDTH)
      break ;
    mri_in_pyramid[nlevels] = MRIreduceByte(mri_in_pyramid[nlevels-1], NULL) ;
    mri_ref_pyramid[nlevels] = 
      MRIreduceMeanAndStdByte(mri_ref_pyramid[nlevels-1],NULL);
  }

  for (i = nlevels-1 ; i >= 1 ; i--)
  {
    /*    sprintf(parms->base_name, "level%d_", i) ;*/
    fprintf(stderr, "aligning pyramid level %d.\n", i) ;
    if ((Gdiag & DIAG_WRITE) && parms->log_fp)
      fprintf(parms->log_fp, "aligning pyramid level %d.\n", i) ;
    mriLinearAlignPyramidLevel(mri_in_pyramid[i], mri_ref_pyramid[i], parms) ;
  }

  /* free Gaussian pyramid */
  for (i = 1 ; i < nlevels ; i++)
  {
    MRIfree(&mri_in_pyramid[i]) ;
    MRIfree(&mri_ref_pyramid[i]) ;
  }
#endif
  strcpy(parms->base_name, base_name) ;
  if (parms->log_fp)
     fclose(parms->log_fp) ;

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
int
MRIfindMeans(MRI *mri, float *means)
{
  int      width, height, depth, x, y, z, val, npoints, mx, my, mz ;
  BUFTYPE  *psrc ;

  width = mri->width ;
  height = mri->height ;
  depth = mri->depth ;

  mx = my = mz = npoints = 0 ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      psrc = &MRIvox(mri, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        val = *psrc++ ;
        if (val > 30)
        {
          npoints++ ;
          mx += x ;
          my += y ;
          mz += z ;
        }
      }
    }
  }

  if (npoints)
  {
    means[0] = (float)mx / npoints ; 
    means[1] = (float)my / npoints ; 
    means[2] = (float)mz / npoints ;
  }
  else
    means[0] = means[1] = means[2] = 0.0f ;

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
int
MRIfindCenterOfBrain(MRI *mri, float *px0, float *py0, float *pz0)
{
#if 1
  int   x, y, z, width, height, depth ;
  float x0, y0, z0, total, val ;

  width = mri->width ; height = mri->height ; depth = mri->depth ; 
  x0 = y0 = z0 = 0.0f ; total = 0.0f ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        val = (float)MRIvox(mri, x, y, z) ;
        if (val > 20)   /* don't let differences in bg bias result */
        {
          x0 += (float)x*val ; y0 += (float)y*val ; z0 += (float)z*val ;
          total += val ;
        }
      }
    }
  }
  if (total > 0)
  {
    *px0 = x0 / (float)total ;
    *py0 = y0 / (float)total ;
    *pz0 = z0 / (float)total ;
  }
  else
  {
    *px0 = width/2 ; 
    *py0 = height/2 ;
    *pz0 = depth/2;
  }
#else
  MRI_REGION box ;

  MRIboundingBox(mri, 140, &box) ; 
  *px0 = box.x + box.dx/2 ;
  *py0 = box.y + 75/mri->ysize ;
  *pz0 = box.z + box.dz/2 ;
#endif
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/

static int
mriLinearAlignPyramidLevel(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms)
{
  double     rms, old_rms, dt ;
  int        k, n, nsmall = 0 ;
  MATRIX     *m_L ;
  static int ncalls = 0 ;
  char       fname[100], base_name[100] ;

  /*  MRIeraseBorders(mri_in, 1) ; MRIeraseBorders(mri_ref, 1) ;*/
  mri_ref->mean = MRImeanFrame(mri_ref, 1) ;
  fprintf(stderr, "mean std = %2.2f\n", mri_ref->mean) ;
  if (DZERO(mri_ref->mean))
    mri_ref->mean = 1 ;

  /* clear momentum from prior level */
  for (k = 0 ; k < parms->lta->num_xforms ; k++)
    MatrixClear(parms->lta->xforms[k].m_last_dL) ;

  strcpy(base_name, parms->base_name) ;
  sprintf(parms->base_name, "level%d_%s", ncalls, base_name) ;

  ncalls++ ;   /* for diagnostics */
  dt = parms->dt * mri_in->thick /** mri_in->thick*/ ;
  if (mri_in->thick <= 4.0)
    dt /= 2.0 ;

  m_L = parms->lta->xforms[0].m_L ;
  old_rms = rms = mriIntensityRMS(mri_in, mri_ref, parms->lta, 1.0f) ;
  fprintf(stderr, 
          "000: rms = %2.3f, t = [%2.2f, %2.2f, %2.2f], dt = %2.2e, "
          "thick=%2.0f\n", 
          rms, m_L->rptr[1][4], m_L->rptr[2][4], m_L->rptr[3][4], dt,
          mri_in->thick) ;

  if ((Gdiag & DIAG_WRITE) && parms->log_fp)
  {
    fprintf(parms->log_fp, 
            "\n000: rms = %2.3f, t = [%2.2f, %2.2f, %2.2f], dt = %2.2e\n", 
            rms, m_L->rptr[1][4], m_L->rptr[2][4], m_L->rptr[3][4], dt) ;
  }
  if ((Gdiag & DIAG_WRITE) && (parms->write_iterations > 0) &&!parms->start_t)
  {
    writeSnapshot(parms->mri_in, parms, 0) ;

    if (ncalls == 1)
    {
      sprintf(fname, "%sref.mgh", base_name) ;
      fprintf(stderr, "writing reference volume to %s...\n", fname) ;
      MRIwrite(parms->mri_ref, fname) ;
      sprintf(fname, "%sref", base_name) ;
      fprintf(stderr, "writing reference views to %s...\n", fname) ;
      mriWriteImageViews(parms->mri_ref, fname, IMAGE_SIZE) ;
    }
  }
  for (n = 0 ; n < parms->niterations ; n++)
  {
    ltaGradientStep(mri_in, mri_ref, parms->lta, dt, parms->momentum);
    rms = mriIntensityRMS(mri_in, mri_ref, parms->lta, 1.0f) ;
    fprintf(stderr, "%3.3d: rms = %2.3f, t = [%2.2f, %2.2f, %2.2f]\n", 
            n+1, rms, m_L->rptr[1][4], m_L->rptr[2][4], m_L->rptr[3][4]) ;

    if ((Gdiag & DIAG_WRITE) &&
        (parms->write_iterations > 0) && 
        (((n+1) % parms->write_iterations) == 0))
      writeSnapshot(parms->mri_in, parms, n+1) ;
    logIntegration(parms, n, rms) ;
    if (FZERO(rms) || (((old_rms - rms) / rms) < parms->tol))
      nsmall++ ;
    else
      if (nsmall > 0)
        nsmall = 0 ;

    /* more than 2 small steps or error increased - integration asymptoted */
    if (nsmall > 2 || old_rms < rms)
      break ;
    old_rms = rms ;
  }
  fprintf(stderr, "\n") ;

  if ((Gdiag & DIAG_WRITE) && parms->log_fp)
    fprintf(parms->log_fp, "\n") ;
  strcpy(parms->base_name, base_name) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute the rms intensity error between a reference image and
          an input image after a linear transformation of the input
          coordinate system.
------------------------------------------------------*/
static double
mriIntensityRMS(MRI *mri_in, MRI *mri_ref, LTA *lta, double l_intensity)
{
  int    x1, x2, x3, width, height, depth ;
  VECTOR *v_X, *v_Y ;  /* original and transformed coordinate systems */
  Real   val, y1, y2, y3 ;
  double sse = 0.0f, delta = 0.0, std, mean_std ;

  mean_std = mri_ref->mean ;
  width = mri_in->width ; height = mri_in->height ; depth = mri_in->depth ;

  v_X  = VectorAlloc(4, MATRIX_REAL) ;  /* input (src) coordinates */
  v_Y  = VectorAlloc(4, MATRIX_REAL) ;  /* transformed (dst) coordinates */

  for (x3 = 0 ; x3 < depth ; x3++)
  {
    v_X->rptr[4][1] = 1.0f / mri_in->thick ;
    v_X->rptr[3][1] = x3 ;
    for (x2 = 0 ; x2 < height ; x2++)
    {
      v_X->rptr[2][1] = x2 ;
      for (x1 = 0 ; x1 < width ; x1++)
      {
        if (x1 == width/2 && x2 == height/2 && x3 == depth/2)
          DiagBreak() ;

        v_X->rptr[1][1] = x1 ;
        LTAtransformPoint(lta, v_X, v_Y) ;
        
        y1 = (Real)v_Y->rptr[1][1] ;
        y2 = (Real)v_Y->rptr[2][1] ;
        y3 = (Real)v_Y->rptr[3][1] ;

        if (y1 > -1 && y1 < width &&
            y2 > -1 && y2 < height &&
            y3 > -1 && y3 < depth)
        {
          MRIsampleVolume(mri_ref, y1, y2, y3, &val);
          if (mri_ref->nframes > 1)
            MRIsampleVolumeFrame(mri_ref, y1, y2, y3, 1, &std);
          else
            std = mean_std ;
          std /= mean_std ;
          if (DZERO(std))
            std = 1.0 ;
          delta = (val - (double)MRIvox(mri_in,x1,x2,x3)) / std ;
        }
        else   /* out of bounds, assume in val is 0 */
          delta = (0.0 - (double)MRIvox(mri_in,x1,x2,x3)) / mean_std ;
        if (fabs(delta) > 20)
          DiagBreak() ;
        sse += delta * delta * mri_in->thick ;
        if (sse > 2000)
          DiagBreak() ;
        if (!finite(sse))
          DiagBreak() ;
      }
    }
  }

  MatrixFree(&v_X) ; MatrixFree(&v_Y) ;

  sse = sqrt(sse / ((double)(width*height*depth))) ;
  return(sse) ;   /* rms */
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
static int
writeSnapshot(MRI *mri, MORPH_PARMS *parms, int n)
{
  MRI   *mri_tmp ;
  char  fname[200] ;

  if (!(Gdiag & DIAG_WRITE))
    return(NO_ERROR) ;

#if 0
  mri_tmp = MRIlinearTransform(mri, NULL, parms->lta->xforms[0].m_L) ;
#else
  mri_tmp = LTAtransform(mri, NULL, parms->lta) ;
#endif
  if ((n % 50*parms->write_iterations) == 0)
  {
    sprintf(fname, "%s%3.3d.mgh", parms->base_name, n) ;
    if (Gdiag & DIAG_SHOW)
      fprintf(stderr, "writing snapshot to %s...\n", fname) ;
    MRIwrite(mri_tmp, fname) ;
  }
  sprintf(fname, "%s%3.3d", parms->base_name, n) ;
  mriWriteImageViews(mri_tmp, fname, IMAGE_SIZE) ;
#if 0
  mriWriteImageView(mri_tmp, fname, IMAGE_SIZE, MRI_CORONAL, -1) ; 
  mriWriteImageView(mri_tmp, fname, IMAGE_SIZE, MRI_SAGITTAL, -1) ; 
  mriWriteImageView(mri_tmp, fname, IMAGE_SIZE, MRI_HORIZONTAL, -1) ; 
#endif
  MRIfree(&mri_tmp) ;

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
ltaGradientStep(MRI *mri_in, MRI *mri_ref, LTA *lta, double dt,double momentum)
{
  int    width, height, depth ;
  VECTOR *v_X, *v_Y, *v_Yk ; /* original and transformed coordinate systems */
  MATRIX *m_tmp, *m_L, *m_dT_X_T ;
  VECTOR *v_dT, *v_X_T ;      /* gradient of mri_ref */
  Real   val, x1, x2, x3, y1, y2, y3 /*, len*/ ;
  double delta = 0.0, n, std, sigma, dx, dy, dz, wtotal, dsq, w_k_p, mean_std ;
  int    only_translation, k ;
  LT     *lt ;
  MRI_REGION bbox ;

  mean_std = mri_ref->mean ;
  MRIboundingBox(mri_in, 5, &bbox) ;

  only_translation = (getenv("ONLY_TRANSLATION") != NULL) ;

  m_L = lta->xforms[0].m_L ;
  width = mri_in->width ; height = mri_in->height ; depth = mri_in->depth ;
  m_tmp = MatrixAlloc(4, 4, MATRIX_REAL) ;
  m_dT_X_T = MatrixAlloc(4, 4, MATRIX_REAL) ;
  v_X = VectorAlloc(4, MATRIX_REAL) ;    /* input (src) coordinates */
  v_Y = VectorAlloc(4, MATRIX_REAL) ;    /* transformed (dst) coordinates */
  v_Yk = VectorAlloc(4, MATRIX_REAL) ;   /* transformed (dst) coordinates */
  v_X_T = RVectorAlloc(4, MATRIX_REAL) ; /* transpose of input coords */
  v_dT = VectorAlloc(4, MATRIX_REAL) ;   /* gradient of target image */

  v_X->rptr[4][1] = 1.0f / mri_in->thick ;
  for (k = 0 ; k < lta->num_xforms ; k++)
    MatrixClear(lta->xforms[k].m_dL) ;

  for (n = 0.0, x3 = bbox.z ; x3 < bbox.z+bbox.dz ; x3++)
  {
    V3_Z(v_X) = x3 ;
    for (x2 = bbox.y ; x2 < bbox.y+bbox.dy ; x2++)
    {
      V3_Y(v_X) = x2 ;
      for (x1 = bbox.x ; x1 < bbox.x+bbox.dx ; x1++)
      {
        V3_X(v_X) = x1 ;
        MatrixTranspose(v_X, v_X_T) ;

        /* first compute weight normalization factor */
        
        wtotal = LTAtransformPointAndGetWtotal(lta, v_X, v_Y) ;

        y1 = V3_X(v_Y) ; y2 = V3_Y(v_Y) ; y3 = V3_Z(v_Y) ;

        if (y1 > -1 && y1 < width &&
            y2 > -1 && y2 < height &&
            y3 > -1 && y3 < depth)
        {
          MRIsampleVolume(mri_ref, y1, y2, y3, &val);
          if (mri_ref->nframes > 1)
            MRIsampleVolumeFrame(mri_ref, y1, y2, y3, 1, &std);
          else
            std = mean_std ;
          MRIsampleVolumeGradient(mri_ref, y1, y2, y3, &dx, &dy, &dz) ;
        }
        else
        {
          std = mean_std ; val = 0.0f ;
          dx = dy = dz = 0.0f ;
        }
        std /= mean_std ;
        if (DZERO(std))
          std = 1.0 ;
        delta = (val-(double)MRIvox(mri_in,nint(x1),nint(x2),nint(x3)))/std;

        if (only_translation)
        { RV3_X(v_X_T) = RV3_Y(v_X_T) = RV3_Z(v_X_T) = 0 ; }
        V3_X(v_dT) = dx ; V3_Y(v_dT) = dy ; V3_Z(v_dT) = dz ; 
        MatrixMultiply(v_dT, v_X_T, m_dT_X_T) ;

        for (k = 0 ; k < lta->num_xforms ; k++)
        {
          lt = &lta->xforms[k] ;
          sigma = lt->sigma ;
          dx = lt->x0-x1*mri_in->thick ; 
          dy = lt->y0-x2*mri_in->thick ; 
          dz = lt->z0-x3*mri_in->thick ; 
          dsq = dx*dx+dy*dy+dz*dz ; 
          w_k_p = exp(-dsq/(2*sigma*sigma)); 
          w_k_p /= wtotal ;
          MatrixScalarMul(m_dT_X_T, w_k_p*mri_in->thick*delta, m_tmp) ;
          MatrixCheck(m_tmp) ;
          MatrixAdd(m_tmp, lt->m_dL, lt->m_dL) ;
          MatrixCheck(lt->m_dL) ;
#if 0
          if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
          {
            fprintf(stderr, "m_tmp ") ;
            MatrixAsciiWriteInto(stderr, m_tmp) ;
            fprintf(stderr, "m_dL ") ;
            MatrixAsciiWriteInto(stderr, m_dL) ;
          }
#endif
          n++ ;
        }
      }
    }
  }

  if (FZERO(n))
    DiagBreak() ;
  else
  {
    dt *= (lta->num_xforms/n) ;
    for (k = 0 ; k < lta->num_xforms ; k++)
    {
      /* take step in negative gradient direction */
      lt = &lta->xforms[k] ;
      MatrixScalarMul(lt->m_dL, -dt, lt->m_dL) ;
      MatrixScalarMul(lt->m_last_dL, momentum, lt->m_last_dL) ;
      MatrixAdd(lt->m_last_dL, lt->m_dL, lt->m_dL) ;
      MatrixAdd(lt->m_dL, lt->m_L, lt->m_L) ; 
      MatrixCopy(lt->m_dL, lt->m_last_dL) ;
      MatrixCheck(lt->m_L) ;
    }
  }
  MatrixFree(&m_dT_X_T) ;
  MatrixFree(&v_X) ;
  MatrixFree(&v_Y) ;
  MatrixFree(&v_X_T) ;
  MatrixFree(&v_dT) ;
  MatrixFree(&m_tmp) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
openLogFile(MORPH_PARMS *parms)
{
  char fname[100] ;

  if ((Gdiag & DIAG_WRITE) && (parms->log_fp == NULL))
  {
    sprintf(fname, "%s.log", parms->base_name) ;
    parms->log_fp = fopen(fname, "w") ;
    fprintf(parms->log_fp, 
            "dt=%2.2e, momentum=%2.2f, tol=%2.2e, niteratons=%d\n",
            parms->dt, parms->momentum, parms->tol, parms->niterations) ;
    if (!FZERO(parms->l_intensity))
      fprintf(parms->log_fp, "l_intensity = %2.4f\n", parms->l_intensity) ;
    if (!FZERO(parms->l_dist))
      fprintf(parms->log_fp, "l_dist = %2.4f\n", parms->l_dist) ;
    if (!FZERO(parms->l_area))
      fprintf(parms->log_fp, "l_area = %2.4f\n", parms->l_area) ;
    if (!FZERO(parms->l_nlarea))
      fprintf(parms->log_fp, "l_nlarea = %2.4f\n", parms->l_nlarea) ;
    if (!FZERO(parms->sigma))
      fprintf(parms->log_fp, "sigma   = %2.4f\n", parms->sigma) ;
    if (!FZERO(parms->exp_k))
      fprintf(parms->log_fp, "exp k   = %2.1f\n", parms->exp_k) ;
    fflush(parms->log_fp) ;
  }
  fprintf(stderr, "dt = %2.2e, momentum=%2.2f, tol=%2.2e\n",
          parms->dt, parms->momentum, parms->tol) ;
  if (!FZERO(parms->l_intensity))
    fprintf(stderr, "l_intensity = %2.4f\n", parms->l_intensity) ;
  if (!FZERO(parms->l_dist))
    fprintf(stderr, "l_dist = %2.4f\n", parms->l_dist) ;
  if (!FZERO(parms->l_area))
    fprintf(stderr, "l_area = %2.4f\n", parms->l_area) ;
  if (!FZERO(parms->l_nlarea))
    fprintf(stderr, "l_nlarea = %2.4f\n", parms->l_nlarea) ;
    if (!FZERO(parms->sigma))
      fprintf(stderr, "sigma   = %2.4f\n", parms->sigma) ;
  fflush(stderr) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
logIntegration(MORPH_PARMS *parms, int n, double rms)
{
  if (Gdiag & DIAG_WRITE)
  {
    fprintf(parms->log_fp, "%3.3d: rms = %2.3f\n", n+1, rms) ;
    fflush(parms->log_fp) ;
  }
  return(NO_ERROR) ;
}
/*--------------------------------------------------------------
                                3D Morph
----------------------------------------------------------------*/

static double   mri3DSSE(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms,
                         MORPH_3D *m3d, 
                         double *pintensity_rms,
                         double *pdistance_rms,
                         double *parea_rms
                         ) ;
static double   mri3DCorrelationSSE(MRI *mri_in, MRI *mri_ref, MORPH_3D *m3d);
static double   mri3DDistanceSSE(MRI *mri_in, MRI *mri_ref, MORPH_3D *m3d) ;
static double   mri3DAreaSSE(MRI *mri_in, MRI *mri_ref, MORPH_3D *m3d) ;
static double   mri3DNonlinearAreaSSE(MRI *mri_in,MRI *mri_ref,MORPH_3D *m3d,
                                      MORPH_PARMS *parms);
static int      mri3DCorrelationTerm(MRI *mri_in, MRI *mri_ref,
                                     MRI *mri_ref_blur, double l_intensity, 
                                     int xin, int yin, int zin,
                                     Real xref, Real yref, Real zref,
                                     double *pdx, double *pdy, double *pdz) ;
static int      mri3DDistanceTerm(MORPH_3D *m3d, double l_distance, 
                                  int i, int j, int k,
                                   double *pdx, double *pdy, double *pdz) ;
static int      mri3DNonlinearAreaTerm(MORPH_3D *m3d, double l_nlarea,int i, 
                                       int j, int k, double *pdx, double *pdy,
                                       double *pdz, MORPH_PARMS *parms);
static int      mri3DAreaTerm(MORPH_3D *m3d,  double l_area, int i, int j, 
                              int k, double *pdx, double *pdy, double *pdz) ;
static MORPH_3D *mri3DInit(MRI *mri_in, MRI *mri_ref,
                           MORPH_PARMS *parms, float scale) ;
static MORPH_3D *mri3Dalloc(int width, int height, int depth, float spacing) ;
static int      mri3DallocProperties(MORPH_3D *m3d) ;
static int      mri3DInitDistances(MORPH_3D *m3d) ;
static int      mri3DInitAreas(MORPH_3D *m3d) ;
static double   mri3DIntegrationStep(MRI *mri_in, MRI *mri_ref, 
                                     MRI *mri_ref_blur, MORPH_PARMS *parms, 
                                     MORPH_3D *m3d, double dt) ;
static int      mri3DcomputeMetricProperties(MORPH_3D *m3d) ;
static int      mri3DclearGradient(MORPH_3D *m3d) ;
static int      mri3DapplyGradient(MORPH_3D *m3d, double dt);
static double   mri3DscaleDeltaT(MORPH_3D *m3d, double dt, double max_grad);
static int      MRIsample3Dmorph(MORPH_3D *m3d, float x, float y, float z, 
                                 float *pxd, float *pyd, float *pzd);
static MORPH_3D *mri3DscaleUp2(MORPH_3D *m3d_in, MORPH_3D *m3d_out,
                               MORPH_PARMS *parms) ;
#if 0
static MORPH_3D *mri3DscaleDown2(MORPH_3D *m3d_in, MORPH_3D *m3d_out,
                                 MORPH_PARMS *parms) ;
#endif

/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute SSE for entire energy functional, and 
          return rms error for the individual terms as well.
------------------------------------------------------*/
static double
mri3DSSE(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms, MORPH_3D *m3d,
         double *pintensity_rms, double *pdistance_rms, double *parea_rms)
{
  double   sse, intensity_sse, area_sse, distance_sse, nvox, nlarea_sse ;

  intensity_sse = mri3DCorrelationSSE(mri_in, mri_ref, m3d);
  area_sse = mri3DAreaSSE(mri_in, mri_ref, m3d);
  nlarea_sse = mri3DNonlinearAreaSSE(mri_in, mri_ref, m3d, parms);
  distance_sse = mri3DDistanceSSE(mri_in, mri_ref, m3d);

  nvox = m3d->width * m3d->height * m3d->depth ;
  *pintensity_rms = sqrt(intensity_sse / nvox) ;
  *pdistance_rms = sqrt(distance_sse / nvox) ;
  *parea_rms = sqrt(area_sse / nvox) ;
  sse = 
    parms->l_intensity * intensity_sse + 
    parms->l_area * area_sse + 
    parms->l_nlarea * nlarea_sse + 
    parms->l_dist * distance_sse ;
  return(sse) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute correlation SSE
------------------------------------------------------*/
static double
mri3DCorrelationSSE(MRI *mri_in, MRI *mri_ref,MORPH_3D *m3d)
{
  double sse ;
  int        width, height, depth, x, y, z, xsi, ysi, zsi, *pxi, *pyi, *pzi ;
  MORPH_NODE *mn ;
  float      x1, x2, x3, scale ;
  Real       ref_val, in_val, delta, xd, yd, zd, ref_std, mean_std ;

  mean_std = mri_ref->mean ;
#if 0
  mri_in = m3d->mri_in ; mri_ref = m3d->mri_ref ;  /* non-blurred versions */
#endif
  width = mri_in->width ; height = mri_in->height ; depth = mri_in->depth ; 
  scale = m3d->node_spacing / mri_in->thick  ;
  pxi = mri_in->xi ; pyi = mri_in->yi ; pzi = mri_in->zi ; 
  for (sse = 0.0f, x = 0 ; x < m3d->width ; x++)
  {
    x1 = x * scale ; /* convert to voxel space of mri_in */  
    xsi = pxi[nint(x1)] ;
    for (y = 0 ; y < m3d->height ; y++)
    {
      x2 = y * scale ; ysi = pyi[nint(x2)] ;    /* voxel coords of mri_in */
      for (z = 0 ; z < m3d->depth ; z++)
      {
        x3 = z * scale ; zsi = pzi[nint(x3)] ;  /* voxel coords of mri_in */
        mn = &m3d->nodes[x][y][z] ;        /* find out where this voxel went */
        xd = mn->x / mri_ref->thick ; 
        yd = mn->y / mri_ref->thick ; 
        zd = mn->z / mri_ref->thick ; 
        if (xd > -1 && yd > -1 && zd > 0 &&
            xd < width && yd < height && zd < depth)
        {
          MRIsampleVolume(mri_ref, xd, yd, zd, &ref_val);
          MRIsampleVolumeFrame(mri_ref, xd, yd, zd, 1, &ref_std);
          ref_std /= mean_std ;
        }
        else
        { ref_std = mean_std ; ref_val = 0.0 ; }
        if (DZERO(ref_std))
          ref_std = 1.0 ;   /* don't let morph be driven by background */
        in_val = (double)MRIvox(mri_in,xsi,ysi,zsi) ;
        delta = (in_val - ref_val) / ref_std ;
        sse += delta * delta * mri_in->thick ;  /* delta^2 dx */
        if (!finitep(sse))
          DiagBreak() ;
      }
    }
  }

  return(sse) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute SSE for distance term.
------------------------------------------------------*/
static double   
mri3DDistanceSSE(MRI *mri_in, MRI *mri_ref, MORPH_3D *m3d)
{
  int        i, j, k, width, height, depth, n ;
  MORPH_NODE *mn, *mn_nbr ;
  MNP        *mnp ;
  double     dist, xd, yd, zd, delta, sse, node_spacing ;

  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  node_spacing = m3d->node_spacing ;
  for (sse = 0.0, i = 0 ; i < width ; i++)
  {
    for (j = 0 ; j < height ; j++)
    {
      for (k = 0 ; k < depth ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        for (n = 0 ; n < NEIGHBORS ; n++)
        {
          switch (n)
          {
          default:
          case 0:      /* i-1 */
            if (i > 0)
              mn_nbr = &m3d->nodes[i-1][j][k] ;
            else
              mn_nbr = NULL ;
            break ;
          case 1:      /* i+1 */
            if (i < width-1)
              mn_nbr = &m3d->nodes[i+1][j][k] ;
            else
              mn_nbr = NULL ;
            break ;
          case 2:      /* j-1 */
            if (j > 0)
              mn_nbr = &m3d->nodes[i][j-1][k] ;
            else
              mn_nbr = NULL ;
            break ;
          case 3:      /* j+1 */
            if (j < height-1)
              mn_nbr = &m3d->nodes[i][j+1][k] ;
            else
              mn_nbr = NULL ;
            break ;
          case 4:      /* k-1 */
            if (k > 0)
              mn_nbr = &m3d->nodes[i][j][k-1] ;
            else
              mn_nbr = NULL ;
            break ;
          case 5:      /* k+1 */
            if (k < depth-1)
              mn_nbr = &m3d->nodes[i][j][k+1] ;
            else
              mn_nbr = NULL ;
            break ;
          }
          if (mn_nbr)
          {
            xd = mn->x - mn_nbr->x ;
            yd = mn->y - mn_nbr->y ;
            zd = mn->z - mn_nbr->z ;
            dist = sqrt(xd*xd + yd*yd + zd*zd) ;
            delta = (dist - mnp->orig_dist[n]) ;
#if SCALE_INVARIANT
            delta /= node_spacing ;
#endif
            sse += delta * delta * mri_in->thick ;
          }
        }
      }
    }
  }

  return(sse) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute SSE for area term.
------------------------------------------------------*/
static double
mri3DNonlinearAreaSSE(MRI *mri_in, MRI *mri_ref, MORPH_3D *m3d, 
                      MORPH_PARMS *parms)
{
  double     sse, delta, n3, ratio ;
  int        i, j, k, width, height, depth ;
  MORPH_NODE *mn ;
  MNP        *mnp ;

  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  m3d->neg = 0 ;
#if SCALE_INVARIANT
  n3 = m3d->node_spacing * m3d->node_spacing * m3d->node_spacing ;
#else
  n3 = 1.0f ;
#endif
  for (sse = 0.0, i = 0 ; i < width-1 ; i++)
  {
    for (j = 0 ; j < height-1 ; j++)
    {
      for (k = 0 ; k < depth-1 ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        delta = (mnp->area - mnp->orig_area) / n3 ;
        /* scale up the area coefficient if the area of the current node is
           close to 0 or already negative */
        if (!FZERO(mnp->orig_area))
          ratio = mnp->area / mnp->orig_area ;
        else
        {
          ratio = 0 ;
          fprintf(stderr, "orig area = 0 at (%d, %d, %d)!!!\n", i, j, k) ;
        }
#if 0
        delta = (1.0 / parms->exp_k) * log(1.0+exp(-parms->exp_k*ratio));
#else
        delta = log(1+exp(parms->exp_k*ratio)) / parms->exp_k - ratio ;
#endif

        sse += delta * mri_in->thick ;
        if (!finitep(delta) || !finitep(sse))
          DiagBreak() ;
      }
    }
  }

  return(sse) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute SSE for area term.
------------------------------------------------------*/
static double   
mri3DAreaSSE(MRI *mri_in, MRI *mri_ref, MORPH_3D *m3d)
{
  double     sse, delta, n3 ;
  int        i, j, k, width, height, depth ;
  MORPH_NODE *mn ;
  MNP        *mnp ;

  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  m3d->neg = 0 ;
#if SCALE_INVARIANT
  n3 = m3d->node_spacing * m3d->node_spacing * m3d->node_spacing ;
#else
  n3 = 1.0f ;
#endif
  for (sse = 0.0, i = 0 ; i < width-1 ; i++)
  {
    for (j = 0 ; j < height-1 ; j++)
    {
      for (k = 0 ; k < depth-1 ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        delta = (mnp->area - mnp->orig_area) / n3 ;
        sse += delta * delta * mri_in->thick ;
        if (!finitep(delta) || !finitep(sse))
          DiagBreak() ;
        if (mnp->area <= 0)
          m3d->neg++ ;
      }
    }
  }

  return(sse) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Allocate and initialize 3D properties
------------------------------------------------------*/
static int
mri3DallocProperties(MORPH_3D *m3d)
{
  int        x, y, width, height, depth ;
  
  width = m3d->width ; height = m3d->height ; depth = m3d->depth ;

  m3d->pnodes = (MORPH_NODE_PROPERTIES ***)
    calloc(width, sizeof(MORPH_NODE_PROPERTIES **)) ;
  if (!m3d->pnodes)
    ErrorExit(ERROR_NOMEMORY, 
              "mri3DallocProperties: could not allocate node width") ;
  for (x = 0 ; x < width ; x++)
  {
    m3d->pnodes[x] = (MORPH_NODE_PROPERTIES **)
      calloc(height, sizeof(MORPH_NODE_PROPERTIES *)) ;
    if (!m3d->pnodes)
      ErrorExit(ERROR_NOMEMORY, 
                "mri3DallocProperties: could not allocate node height %d", x) ;
    for (y = 0 ; y < height ; y++)
    {
      m3d->pnodes[x][y] = (MORPH_NODE_PROPERTIES *)
        calloc(depth, sizeof(MORPH_NODE_PROPERTIES)) ;
      if (!m3d->pnodes)
        ErrorExit(ERROR_NOMEMORY, 
                  "mri3DallocProperties: could not allocate node depth %d,%d",
                  x,y) ;
    }
  }
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Allocate and initialize a 3D morph structure.
------------------------------------------------------*/
static MORPH_3D *
mri3Dalloc(int width, int height, int depth, float node_spacing)
{
  MORPH_3D   *m3d ;
  int        x, y ;
  
  m3d = (MORPH_3D *)calloc(1, sizeof(MORPH_3D)) ;
  if (!m3d)
    ErrorExit(ERROR_NOMEMORY, "mri3Dalloc: could not allocate m3d") ;

  m3d->node_spacing = node_spacing ;
  m3d->width = width ; m3d->height = height ; m3d->depth = depth ; 
#if 0
  m3d->mri_in = mri_in ; m3d->mri_ref = mri_ref ; 
  m3d->m_L = parms->lta->xforms[0].m_L ;
#endif

  m3d->nodes = (MORPH_NODE ***)calloc(width, sizeof(MORPH_NODE **)) ;
  if (!m3d->nodes)
    ErrorExit(ERROR_NOMEMORY, "mri3Dalloc: could not allocate node width") ;
  for (x = 0 ; x < width ; x++)
  {
    m3d->nodes[x] = (MORPH_NODE **)calloc(height, sizeof(MORPH_NODE *)) ;
    if (!m3d->nodes)
      ErrorExit(ERROR_NOMEMORY, 
                "mri3Dalloc: could not allocate node height %d", x) ;
    for (y = 0 ; y < height ; y++)
    {
      m3d->nodes[x][y] = (MORPH_NODE *)calloc(depth, sizeof(MORPH_NODE)) ;
      if (!m3d->nodes)
        ErrorExit(ERROR_NOMEMORY, 
                  "mri3Dalloc: could not allocate node depth %d,%d", x,y) ;
    }
  }

  return(m3d) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Allocate and initialize a 3D morph structure.
------------------------------------------------------*/
static MORPH_3D *
mri3DInit(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms, float size)
{
  MORPH_3D   *m3d ;
  int        width, height, depth, x, y, i, j, k ;
  float      scale ;
  VECTOR     *v_X, *v_Y ;
  MORPH_NODE *mn ;
  MNP        *mnp ;
  
  v_X = VectorAlloc(4, MATRIX_REAL) ;
  v_Y = VectorAlloc(4, MATRIX_REAL) ;
  scale = mri_in->thick / size  ;
  width = mri_in->width*scale ; 
  height = mri_in->height*scale ; 
  depth = mri_in->depth*scale ; 
  m3d = (MORPH_3D *)calloc(1, sizeof(MORPH_3D)) ;
  if (!m3d)
    ErrorExit(ERROR_NOMEMORY, "mri3DInit: could not allocate m3d") ;

  m3d->node_spacing = size ;
  m3d->width = width ; m3d->height = height ; m3d->depth = depth ; 
  m3d->mri_in = mri_in ; m3d->mri_ref = mri_ref ; m3d->lta = parms->lta ;

  m3d->nodes = (MORPH_NODE ***)calloc(width, sizeof(MORPH_NODE **)) ;
  if (!m3d->nodes)
    ErrorExit(ERROR_NOMEMORY, "mri3DInit: could not allocate node width") ;
  for (x = 0 ; x < width ; x++)
  {
    m3d->nodes[x] = (MORPH_NODE **)calloc(height, sizeof(MORPH_NODE *)) ;
    if (!m3d->nodes)
      ErrorExit(ERROR_NOMEMORY, 
                "mri3DInit: could not allocate node height %d", x) ;
    for (y = 0 ; y < height ; y++)
    {
      m3d->nodes[x][y] = (MORPH_NODE *)calloc(depth, sizeof(MORPH_NODE)) ;
      if (!m3d->nodes)
        ErrorExit(ERROR_NOMEMORY, 
                  "mri3DInit: could not allocate node depth %d,%d", x,y) ;
    }
  }

  if (!m3d->pnodes)
    mri3DallocProperties(m3d) ;

  /*parms->lta->xforms[0].m_L = MatrixIdentity(4, NULL) ;*/

  /* now initialize node positions */
  v_X->rptr[4][1] = 1.0f /*/ mri_in->thick*/ ;
  for (k = 0 ; k < depth ; k++)
  {
    V3_Z(v_X) = (float)k * m3d->node_spacing ;
    for (j = 0 ; j < height ; j++)
    {
      V3_Y(v_X) = (float)j * m3d->node_spacing ;
      for (i = 0 ; i < width ; i++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        V3_X(v_X) = (float)i * m3d->node_spacing ;
        LTAtransformPoint(parms->lta, v_X, v_Y) ;
#if !USE_ORIGINAL_PROPERTIES
        mnp->ox = mn->x = V3_X(v_Y) ; 
        mnp->oy = mn->y = V3_Y(v_Y) ; 
        mnp->oz = mn->z = V3_Z(v_Y) ; 
#else
        mnp->ox = V3_X(v_X) ; 
        mnp->oy = V3_Y(v_X) ; 
        mnp->oz = V3_Z(v_X) ; 
        mn->x = V3_X(v_Y) ; 
        mn->y = V3_Y(v_Y) ; 
        mn->z = V3_Z(v_Y) ; 
#endif
      }
    }
  }

  mri3DInitDistances(m3d) ;
  mri3DInitAreas(m3d) ;
  VectorFree(&v_X) ; VectorFree(&v_Y) ; 
  return(m3d) ;
}
#if 0
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRI3Dinit(MORPH_3D *m3d)
{
  int        width, height, depth, x, y, i, j, k ;
  float      scale ;
  VECTOR     *v_X, *v_Y ;
  MORPH_NODE *mn ;
  MNP        *mnp ;
  
  v_X = VectorAlloc(4, MATRIX_REAL) ;
  v_Y = VectorAlloc(4, MATRIX_REAL) ;
  width = m3d->width ; height = m3d->height ;  depth = m3d->depth ; 

  /* now initialize node positions */
  v_X->rptr[4][1] = 1.0f ;
  for (k = 0 ; k < depth ; k++)
  {
    V3_Z(v_X) = (float)k * m3d->node_spacing ;
    for (j = 0 ; j < height ; j++)
    {
      V3_Y(v_X) = (float)j * m3d->node_spacing ;
      for (i = 0 ; i < width ; i++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        V3_X(v_X) = (float)i * m3d->node_spacing ;
#if !USE_ORIGINAL_PROPERTIES
        LTAtransformPoint(parms->lta, v_X, v_Y) ;
#else
        MatrixCopy(v_X, v_Y) ;
#endif
        mnp->ox = V3_X(v_Y) ; 
        mnp->oy = V3_Y(v_Y) ; 
        mnp->oz = V3_Z(v_Y) ; 
      }
    }
  }

  mri3DInitDistances(m3d) ;
  mri3DInitAreas(m3d) ;
  VectorFree(&v_X) ; VectorFree(&v_Y) ; 
  return(NO_ERROR) ;
}
#endif
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define MIN_3D_PYR_WIDTH 32
#if 1
MORPH_3D *
MRI3Dmorph(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms)
{
  int      nlevels, i, max_levels ;
  MRI      *mri_in_pyramid[MAX_LEVELS], *mri_ref_pyramid[MAX_LEVELS] ;
  MORPH_3D *m3d_tmp, *m3d ;
  char     base_name[100] ;
  double   dt ;

  mriNormalizeStds(mri_ref) ;
  if (parms->levels >= 0)
    max_levels = parms->levels ;
  else
    max_levels = MAX_LEVELS ;
  MRIremoveNeck(mri_in, mri_in, 90, 120, NULL, 0) ;
  MRIremoveNeck(mri_ref, mri_ref, 90, 120, NULL, 0) ;

  openLogFile(parms) ;

  /* build Gaussian pyramid */
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "building Gaussian pyramid...") ;
  mri_in_pyramid[0] = mri_in ; mri_ref_pyramid[0] = mri_ref ;
  for (nlevels = 1 ; nlevels < max_levels ; nlevels++)
  {
    if (mri_in_pyramid[nlevels-1]->width <= MIN_3D_PYR_WIDTH)
      break ;
    mri_in_pyramid[nlevels] = MRIreduceByte(mri_in_pyramid[nlevels-1], NULL) ;
    mri_ref_pyramid[nlevels] = 
      MRIreduceMeanAndStdByte(mri_ref_pyramid[nlevels-1], NULL);
  }
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "done.\n") ;

  /* now morph each level, and use the resulting morph as the input to
     the next (finer) level.
  */
#define MIN_LEVEL 0
#define MAX_LEVEL nlevels-1
  strcpy(base_name, parms->base_name) ;
  m3d = 
    mri3DInit(mri_in_pyramid[MAX_LEVEL], mri_ref_pyramid[MAX_LEVEL], 
                  parms, mri_in_pyramid[MAX_LEVEL]->thick) ;
  m3d->mri_in = mri_in ; m3d->mri_ref = mri_ref ;
  dt = parms->dt ;
  for (i = MAX_LEVEL ; i >= MIN_LEVEL ; i--)
  {
    /*    parms->dt = dt / mri_in_pyramid[i]->thick ;*/
#if !SCALE_INVARIANT
#if 0
    parms->dt = dt / (mri_in_pyramid[i]->thick*mri_in_pyramid[i]->thick) ;
#else
    parms->dt = dt / (mri_in_pyramid[i]->thick) ;
#endif
#endif
    /*    parms->l_intensity = l_intensity * (sigma*sigma+1.0f) ;*/
    sprintf(parms->base_name, "%s_level%d_", base_name, i) ;
    sprintf(parms->base_name, "%s_", base_name) ;
    fprintf(stderr, "aligning pyramid level %d.\n", i) ;
    if ((Gdiag & DIAG_WRITE) && parms->log_fp)
      fprintf(parms->log_fp, "aligning pyramid level %d.\n", i) ;
    mri3DAlignPyramidLevel(mri_in_pyramid[i], mri_ref_pyramid[i], 
                           mri_ref_pyramid[i], parms, m3d) ;

    if (i > MIN_LEVEL)
    {
      MRIfree(&mri_in_pyramid[i]) ;
      MRIfree(&mri_ref_pyramid[i]) ;
      m3d_tmp = mri3DscaleUp2(m3d, NULL, parms) ;
      /*      MRI3DmorphFree(&m3d) ;  done inside function now */
      m3d = m3d_tmp ;
      m3d->mri_in = mri_in ; m3d->mri_ref = mri_ref ;
    }
  }
  strcpy(parms->base_name, base_name) ;

  /* free Gaussian pyramid */
#if 0
  for (i = 1 ; i < nlevels ; i++)
  {
    MRIfree(&mri_in_pyramid[i]) ;
    MRIfree(&mri_ref_pyramid[i]) ;
  }
#endif
  if (parms->log_fp)
     fclose(parms->log_fp) ;

  return(m3d) ;
}
#else
MORPH_3D *
MRI3Dmorph(MRI *mri_in, MRI *mri_ref, MORPH_PARMS *parms)
{
  MRI      *mri_in_blur, *mri_ref_blur, *mri_kernel ;
  char     base_name[100] ;
  float    sigma ;
  double   l_intensity ;
  MORPH_3D *m3d ;

  MRIremoveNeck(mri_in, mri_in, 90, 120, NULL, 0) ;
  MRIremoveNeck(mri_ref, mri_ref, 90, 120, NULL, 0) ;

  openLogFile(parms) ;

  /* now morph each level, and use the resulting morph as the input to
     the next (finer) level.
  */
#define MIN_LEVEL 0.0f
#define MAX_SIGMA (parms->sigma)
  strcpy(base_name, parms->base_name) ;
  m3d = mri3DInit(mri_in, mri_ref, parms, mri_in->thick) ;
  l_intensity = parms->l_intensity ;
  for (sigma = MAX_SIGMA ; sigma >= 0.0f ; sigma -= 1.0f)
  {
    /*    parms->l_intensity = l_intensity * (sigma*sigma+1.0f) ;*/
    if (Gdiag & DIAG_SHOW)
      fprintf(stderr,"aligning images with sigma %2.1f, l_intensity = %2.3f\n",
              sigma, parms->l_intensity) ;
    if ((Gdiag & DIAG_WRITE) && parms->log_fp)
      fprintf(parms->log_fp,
              "aligning images with sigma %2.1f, l_intensity = %2.3f\n",
              sigma, parms->l_intensity) ;
    sprintf(parms->base_name, "%s_level%d_", base_name, nint(sigma)) ;
    if (sigma > 0.0f)
    { 
      mri_kernel = MRIgaussian1d(sigma, 13) ;
      mri_ref_blur = 
        MRIconvolveGaussianMeanAndStdByte(mri_ref, NULL, mri_kernel) ;
      mri_in_blur = MRIconvolveGaussian(mri_in, NULL, mri_kernel) ;
      mri3DAlignPyramidLevel(mri_in_blur,mri_ref_blur,mri_ref_blur,parms,m3d);
      MRIfree(&mri_kernel) ; MRIfree(&mri_in_blur) ; MRIfree(&mri_ref_blur) ;
    }
    else
    {
      mri_kernel = MRIgaussian1d(0.5, 13) ;
      mri_ref_blur = 
        MRIconvolveGaussianMeanAndStdByte(mri_ref, NULL, mri_kernel) ;
      MRIfree(&mri_kernel) ;
      mri3DAlignPyramidLevel(mri_in, mri_ref, mri_ref, parms, m3d) ;
      MRIfree(&mri_ref_blur) ;
    }
  }
  strcpy(parms->base_name, base_name) ;
  if (parms->log_fp)
     fclose(parms->log_fp) ;

  return(m3d) ;
}
#endif
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          For each point in the morph, compute the point to which it
          maps. Then use a linear interpolation scheme to build the
          morphed image.

          This should really use floating point volumes, or at
          the very least short volumes, but unfortunatly I
          can't fit them in memory, so it all has to be
          done in unsigned char volumes which forces things
          to be scaled up and then back down in various places.
------------------------------------------------------*/
MRI *
MRIapply3DMorph(MRI *mri_in, MORPH_3D *m3d, MRI *mri_morphed)
{
  int        width, height, depth, x, y, z,
             xm1, ym1, zm1, xp1, yp1, zp1 ;
  float      xd, yd, zd, dx, dy, dz, thick ;
  Real       weight, orig_val, val ;
  MRI        *mri_weights, *mri_ctrl, *mri_s_morphed ;

  width = mri_in->width ; height = mri_in->height ; depth = mri_in->depth ; 
  thick = mri_in->thick ;
#define SCALE_FACTOR 25.0f  /* so we can use byte images for float values */

  mri_weights = MRIalloc(width, height, depth, MRI_UCHAR) ;
  mri_s_morphed = MRIalloc(width, height, depth, MRI_SHORT) ;
  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {

        /* compute voxel coordinates of this morph point */
        MRIsample3Dmorph(m3d, (float)x*thick, (float)y*thick, (float)z*thick, 
                         &xd, &yd, &zd) ;
        xd /= thick ; yd /= thick ; zd /= thick ;  /* voxel coords */
#if 0
        MRIsampleVolume(mri_in, xd, yd, zd, &orig_val);
#else
        orig_val = MRIvox(mri_in, x, y, z) ;
#endif
        if (orig_val > 40)
          DiagBreak() ;

        /* now use trilinear interpolation */
        xm1 = (int)floor(xd) ; ym1 = (int)floor(yd) ; zm1 = (int)floor(zd) ; 
        xp1 = xm1 + 1 ; yp1 = ym1 + 1 ; zp1 = zm1 + 1 ;

        /* make sure they are within bounds */
        xm1 = mri_in->xi[xm1] ; ym1 = mri_in->xi[ym1] ; zm1 = mri_in->xi[zm1] ;
        xp1 = mri_in->xi[xp1] ; yp1 = mri_in->xi[yp1] ; zp1 = mri_in->xi[zp1] ;

        dx = xd - xm1 ; dy = yd - ym1 ; dz = zd - zm1 ;

        /* now compute contribution to each of 8 nearest voxels */
        weight = (1-dx) * (1-dy) * (1-dz) ;
        if (xm1 == 121 && ym1 == 172 && zm1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xm1,ym1,zm1) += weight * orig_val ;
        MRIvox(mri_weights,xm1,ym1,zm1) += nint(weight * SCALE_FACTOR) ;

        weight = (dx) * (1-dy) * (1-dz) ;
        if (xm1 == 121 && ym1 == 172 && zm1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xp1,ym1,zm1) += weight * orig_val ;
        MRIvox(mri_weights,xp1,ym1,zm1) += nint(weight * SCALE_FACTOR) ;
        
        weight = (1-dx) * (dy) * (1-dz) ;
        if (xm1 == 121 && yp1 == 172 && zm1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xm1,yp1,zm1) += weight * orig_val ;
        MRIvox(mri_weights,xm1,yp1,zm1) += nint(weight * SCALE_FACTOR) ;
        
        weight = (1-dx) * (1-dy) * (dz) ;
        if (xm1 == 121 && ym1 == 172 && zp1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xm1,ym1,zp1) += weight * orig_val ;
        MRIvox(mri_weights,xm1,ym1,zp1) += nint(weight * SCALE_FACTOR) ;
        
        weight = (dx) * (dy) * (1-dz) ;
        if (xp1 == 121 && yp1 == 172 && zm1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xp1,yp1,zm1) += weight * orig_val ;
        MRIvox(mri_weights,xp1,yp1,zm1) += nint(weight * SCALE_FACTOR) ;
        
        weight = (dx) * (1-dy) * (dz) ;
        if (xp1 == 121 && ym1 == 172 && zp1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xp1,ym1,zp1) += weight * orig_val ;
        MRIvox(mri_weights,xp1,ym1,zp1) += nint(weight * SCALE_FACTOR) ;
        
        weight = (1-dx) * (dy) * (dz) ;
        if (xm1 == 121 && yp1 == 172 && zp1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xm1,yp1,zp1) += weight * orig_val ;
        MRIvox(mri_weights,xm1,yp1,zp1) += nint(weight * SCALE_FACTOR) ;
        
        weight = (dx) * (dy) * (dz) ;
        if (xp1 == 121 && yp1 == 172 && zp1 == 127)
          DiagBreak() ;
        MRISvox(mri_s_morphed,xp1,yp1,zp1) += weight * orig_val ;
        MRIvox(mri_weights,xp1,yp1,zp1) += nint(weight * SCALE_FACTOR) ;
      }
    }
  }

  /* now normalize weights and values */
  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {
        if (x == 121 && y == 172 && z == 127)
          DiagBreak() ;
        weight = (float)MRIvox(mri_weights,x,y,z) / SCALE_FACTOR ;
        if (!FZERO(weight))
        {
          val = (Real)MRISvox(mri_s_morphed,x,y,z) / weight ;
          if (val > 255.0)
            val = 255.0 ;
          MRISvox(mri_s_morphed,x,y,z) = (short)nint(val) ;
        }
      }
    }
  }

  /* copy from short image to BUFTYPE one */
  if (!mri_morphed)
    mri_morphed = MRIclone(mri_in, NULL) ;
  else
    MRIclear(mri_morphed) ;
  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {
        MRIvox(mri_morphed,x,y,z) = (BUFTYPE)MRISvox(mri_s_morphed,x,y,z) ;
      }
    }
  }

  MRIfree(&mri_s_morphed) ;

  /* run soap bubble to fill in remaining holes */
#if 0
  mri_ctrl = MRIclone(mri_in, NULL) ;
#else
  mri_ctrl = mri_weights ;
#endif
  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {
        if (x == 121 && y == 172 && z == 127)
          DiagBreak() ;
        weight = (float)MRIvox(mri_weights,x,y,z) / SCALE_FACTOR ;
        if (weight > .1)
          MRIvox(mri_ctrl, x, y, z) = 1 ;
        else
          MRIvox(mri_ctrl, x, y, z) = 0 ;
      }
    }
  }

#if 0
  MRIfree(&mri_weights) ;
#endif
  MRIbuildVoronoiDiagram(mri_morphed, mri_ctrl, mri_morphed) ;
  MRIsoapBubble(mri_morphed, mri_ctrl, mri_morphed, 5) ;

  MRIfree(&mri_ctrl) ;
  return(mri_morphed) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Apply the previously computed morph which takes
          mri_in into mri_ref space, and use it to generate a
          warping of mri_ref into mri_in space. This is much
          simpler than the inverse mapping as the sampling is
          uniform in this direction.
------------------------------------------------------*/
MRI *
MRIapplyInverse3DMorph(MRI *mri_ref,MORPH_3D *m3d,MRI *mri_morphed)
{
  int        width, height, depth, x, y, z ;
  float      xd, yd, zd ;
  Real       val ;

  width = mri_ref->width ; height = mri_ref->height ; depth = mri_ref->depth ; 
  if (!mri_morphed)
    mri_morphed = MRIclone(mri_ref, NULL) ;
  for (x = 0 ; x < width ; x++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (z = 0 ; z < depth ; z++)
      {
        MRIsample3Dmorph(m3d, (float)x*mri_ref->thick, 
                         (float)y*mri_ref->thick, (float)z*mri_ref->thick, 
                         &xd, &yd, &zd) ;
        xd /= mri_ref->thick ; yd /= mri_ref->thick ; zd /= mri_ref->thick ; 
        if (xd > -1 && yd > -1 && zd > 0 &&
            xd < width && yd < height && zd < depth)
          MRIsampleVolume(mri_ref, xd, yd, zd, &val);
        else
          val = 0.0 ;
        MRIvox(mri_morphed,x,y,z) = val ;
      }
    }
  }
  return(mri_morphed) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
MRIsample3DmorphOrig(MORPH_3D *m3d, float x, float y, float z, 
                     float *pxd, float *pyd, float *pzd)
{
  int   xm, xp, ym, yp, zm, zp, width, height, depth ;
  float xmd, ymd, zmd, xpd, ypd, zpd ;  /* d's are distances */

  /* convert to 3d morph index space */
  x /= m3d->node_spacing ; y /= m3d->node_spacing ; z /= m3d->node_spacing ;
  width = m3d->width ; height = m3d->height ; depth = m3d->depth ;
  if (x >= width)
    x = width - 1.0 ;
  if (y >= height)
    y = height - 1.0 ;
  if (z >= depth)
    z = depth - 1.0 ;
  if (x < 0.0)
    x = 0.0 ;
  if (y < 0.0)
    y = 0.0 ;
  if (z < 0.0)
    z = 0.0 ;

  xm = MAX((int)x, 0) ;
  xp = MIN(width-1, xm+1) ;
  ym = MAX((int)y, 0) ;
  yp = MIN(height-1, ym+1) ;
  zm = MAX((int)z, 0) ;
  zp = MIN(depth-1, zm+1) ;

  xmd = x - (float)xm ;
  ymd = y - (float)ym ;
  zmd = z - (float)zm ;
  xpd = (1.0f - xmd) ;
  ypd = (1.0f - ymd) ;
  zpd = (1.0f - zmd) ;

  *pxd =
    xpd * ypd * zpd * m3d->pnodes[xm][ym][zm].ox +
    xpd * ypd * zmd * m3d->pnodes[xm][ym][zp].ox +
    xpd * ymd * zpd * m3d->pnodes[xm][yp][zm].ox +
    xpd * ymd * zmd * m3d->pnodes[xm][yp][zp].ox +
    xmd * ypd * zpd * m3d->pnodes[xp][ym][zm].ox +
    xmd * ypd * zmd * m3d->pnodes[xp][ym][zp].ox +
    xmd * ymd * zpd * m3d->pnodes[xp][yp][zm].ox +
    xmd * ymd * zmd * m3d->pnodes[xp][yp][zp].ox ;
  *pyd =
    xpd * ypd * zpd * m3d->pnodes[xm][ym][zm].oy +
    xpd * ypd * zmd * m3d->pnodes[xm][ym][zp].oy +
    xpd * ymd * zpd * m3d->pnodes[xm][yp][zm].oy +
    xpd * ymd * zmd * m3d->pnodes[xm][yp][zp].oy +
    xmd * ypd * zpd * m3d->pnodes[xp][ym][zm].oy +
    xmd * ypd * zmd * m3d->pnodes[xp][ym][zp].oy +
    xmd * ymd * zpd * m3d->pnodes[xp][yp][zm].oy +
    xmd * ymd * zmd * m3d->pnodes[xp][yp][zp].oy ;
  *pzd =
    xpd * ypd * zpd * m3d->pnodes[xm][ym][zm].oz +
    xpd * ypd * zmd * m3d->pnodes[xm][ym][zp].oz +
    xpd * ymd * zpd * m3d->pnodes[xm][yp][zm].oz +
    xpd * ymd * zmd * m3d->pnodes[xm][yp][zp].oz +
    xmd * ypd * zpd * m3d->pnodes[xp][ym][zm].oz +
    xmd * ypd * zmd * m3d->pnodes[xp][ym][zp].oz +
    xmd * ymd * zpd * m3d->pnodes[xp][yp][zm].oz +
    xmd * ymd * zmd * m3d->pnodes[xp][yp][zp].oz ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
static int
MRIsample3Dmorph(MORPH_3D *m3d, float x, float y, float z, 
                 float *pxd, float *pyd, float *pzd)
{
  int   xm, xp, ym, yp, zm, zp, width, height, depth ;
  float xmd, ymd, zmd, xpd, ypd, zpd ;  /* d's are distances */

  /* convert to 3d morph index space */
  x /= m3d->node_spacing ; y /= m3d->node_spacing ; z /= m3d->node_spacing ;
  width = m3d->width ; height = m3d->height ; depth = m3d->depth ;
  if (x >= width)
    x = width - 1.0 ;
  if (y >= height)
    y = height - 1.0 ;
  if (z >= depth)
    z = depth - 1.0 ;
  if (x < 0.0)
    x = 0.0 ;
  if (y < 0.0)
    y = 0.0 ;
  if (z < 0.0)
    z = 0.0 ;

  xm = MAX((int)x, 0) ;
  xp = MIN(width-1, xm+1) ;
  ym = MAX((int)y, 0) ;
  yp = MIN(height-1, ym+1) ;
  zm = MAX((int)z, 0) ;
  zp = MIN(depth-1, zm+1) ;

  xmd = x - (float)xm ;
  ymd = y - (float)ym ;
  zmd = z - (float)zm ;
  xpd = (1.0f - xmd) ;
  ypd = (1.0f - ymd) ;
  zpd = (1.0f - zmd) ;

  *pxd =
    xpd * ypd * zpd * m3d->nodes[xm][ym][zm].x +
    xpd * ypd * zmd * m3d->nodes[xm][ym][zp].x +
    xpd * ymd * zpd * m3d->nodes[xm][yp][zm].x +
    xpd * ymd * zmd * m3d->nodes[xm][yp][zp].x +
    xmd * ypd * zpd * m3d->nodes[xp][ym][zm].x +
    xmd * ypd * zmd * m3d->nodes[xp][ym][zp].x +
    xmd * ymd * zpd * m3d->nodes[xp][yp][zm].x +
    xmd * ymd * zmd * m3d->nodes[xp][yp][zp].x ;
  *pyd =
    xpd * ypd * zpd * m3d->nodes[xm][ym][zm].y +
    xpd * ypd * zmd * m3d->nodes[xm][ym][zp].y +
    xpd * ymd * zpd * m3d->nodes[xm][yp][zm].y +
    xpd * ymd * zmd * m3d->nodes[xm][yp][zp].y +
    xmd * ypd * zpd * m3d->nodes[xp][ym][zm].y +
    xmd * ypd * zmd * m3d->nodes[xp][ym][zp].y +
    xmd * ymd * zpd * m3d->nodes[xp][yp][zm].y +
    xmd * ymd * zmd * m3d->nodes[xp][yp][zp].y ;
  *pzd =
    xpd * ypd * zpd * m3d->nodes[xm][ym][zm].z +
    xpd * ypd * zmd * m3d->nodes[xm][ym][zp].z +
    xpd * ymd * zpd * m3d->nodes[xm][yp][zm].z +
    xpd * ymd * zmd * m3d->nodes[xm][yp][zp].z +
    xmd * ypd * zpd * m3d->nodes[xp][ym][zm].z +
    xmd * ypd * zmd * m3d->nodes[xp][ym][zp].z +
    xmd * ymd * zpd * m3d->nodes[xp][yp][zm].z +
    xmd * ymd * zmd * m3d->nodes[xp][yp][zp].z ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Free the properties field of a 3d morph structure
------------------------------------------------------*/
int
MRI3DmorphFreeProperties(MORPH_3D *m3d)
{
  int   x, y ;

  if (m3d->pnodes)
  {
    for (x = 0 ; x < m3d->width ; x++)
    {
      for (y = 0 ; y < m3d->height ; y++)
        free(m3d->pnodes[x][y]) ;
      free(m3d->pnodes[x]) ;
    }
    free(m3d->pnodes) ;
    m3d->pnodes = NULL ;
  }
  
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Free a 3d morph structure and set the pointer to NULL.
------------------------------------------------------*/
int
MRI3DmorphFree(MORPH_3D **pm3d)
{
  MORPH_3D *m3d = *pm3d ;
  int      x, y ;

  *pm3d = NULL ;
  if (!m3d)
    ErrorReturn(ERROR_BADPARM,(ERROR_BADPARM,"MRI3DmorphFree: NULL pointer!"));


  for (x = 0 ; x < m3d->width ; x++)
  {
    for (y = 0 ; y < m3d->height ; y++)
      free(m3d->nodes[x][y]) ;
    free(m3d->nodes[x]) ;
  }
  free(m3d->nodes) ;

  if (m3d->pnodes)
  {
    for (x = 0 ; x < m3d->width ; x++)
    {
      for (y = 0 ; y < m3d->height ; y++)
        free(m3d->pnodes[x][y]) ;
      free(m3d->pnodes[x]) ;
    }
    free(m3d->pnodes) ;
  }
  free(m3d) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute the directional derivative of the correlation term
          at the input image position xin,yin,zin which currently
          corresponds to the reference image position xref,yref,zref.
------------------------------------------------------*/
static int
mri3DCorrelationTerm(MRI *mri_in, MRI *mri_ref, MRI *mri_ref_blur,
                     double l_intensity, 
                     int xin, int yin, int zin, Real xref, Real yref, 
                     Real zref, double *pdx, double *pdy, double *pdz)
{
  Real   ref_val, in_val, delta, Tdx, Tdy, Tdz, len, ref_std, mean_std ;

  mean_std = mri_ref->mean ;
  if (FZERO(l_intensity))
  {   
    *pdx = *pdy = *pdz = 0.0 ;
    return(NO_ERROR) ;
  }
  if (xref > -1 && yref > -1 && zref > 0 && 
      xref < mri_ref->width && yref < mri_ref->height && zref < mri_ref->depth)
  {
    MRIsampleVolume(mri_ref, xref, yref, zref, &ref_val);
    MRIsampleVolumeFrame(mri_ref, xref, yref, zref, 1, &ref_std);
    if (DZERO(ref_std))
      ref_std = 1.0 ;
  }
  else
  { ref_std = mean_std ; ref_val = 0.0 ; }
  ref_std /= mean_std ;
  in_val = (double)MRIvox(mri_in,xin,yin,zin) ;
  delta = -l_intensity * (ref_val - in_val) / ref_std ;  /* -delta */
  MRIsampleVolumeGradient(mri_ref_blur, xref, yref, zref, &Tdx, &Tdy, &Tdz) ;
  len = sqrt(Tdx*Tdx + Tdy*Tdy + Tdz*Tdz) ;
  if (!FZERO(len))
  { Tdx /= len ; Tdy /= len ; Tdz /= len ; }
  *pdx = Tdx * delta ; *pdy = Tdy * delta ; *pdz = Tdz * delta ; 
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute the directional derivative of the distance term
          w.r.t the i,j,kth node.
------------------------------------------------------*/
static int
mri3DDistanceTerm(MORPH_3D *m3d, double l_distance, int i, int j, int k,
                  double *pdx, double *pdy, double *pdz)
{
  int         n ;
  double      dx, dy, dz, dist, delta, xgrad, ygrad, zgrad ;
  MORPH_NODE  *mn, *mn_nbr ;
  MNP         *mnp ;

  if (FZERO(l_distance))
  {
    *pdx = *pdy = *pdz = 0.0 ;
    return(NO_ERROR) ;
  }

  l_distance /= (float)NEIGHBORS ;
  mn = &m3d->nodes[i][j][k] ;
  mnp = &m3d->pnodes[i][j][k] ;
  for (xgrad = ygrad = zgrad = 0.0, n = 0 ; n < NEIGHBORS ; n++)
  {
    switch (n)
    {
    default:
    case 0:      /* i-1 */
      if (i > 0)
        mn_nbr = &m3d->nodes[i-1][j][k] ;
      else
        mn_nbr = NULL ;
      break ;
    case 1:      /* i+1 */
      if (i < m3d->width-1)
        mn_nbr = &m3d->nodes[i+1][j][k] ;
      else
        mn_nbr = NULL ;
      break ;
    case 2:      /* j-1 */
      if (j > 0)
        mn_nbr = &m3d->nodes[i][j-1][k] ;
      else
        mn_nbr = NULL ;
      break ;
    case 3:      /* j+1 */
      if (j < m3d->height-1)
        mn_nbr = &m3d->nodes[i][j+1][k] ;
      else
        mn_nbr = NULL ;
      break ;
    case 4:      /* k-1 */
      if (k > 0)
        mn_nbr = &m3d->nodes[i][j][k-1] ;
      else
        mn_nbr = NULL ;
      break ;
    case 5:      /* k+1 */
      if (k < m3d->depth-1)
        mn_nbr = &m3d->nodes[i][j][k+1] ;
      else
        mn_nbr = NULL ;
      break ;
    }
    if (mn_nbr)
    {
      dx = mn_nbr->x - mn->x ; dy = mn_nbr->y - mn->y ; dz = mn_nbr->z-mn->z;
      dist = sqrt(dx*dx + dy*dy + dz*dz) ;
      delta = dist - mnp->orig_dist[n] ;
      if (!FZERO(dist))   /* make it a unit vector */
      { dx /= dist ; dx /= dist ; dz /= dist ; }
#if SCALE_INVARIANT
      delta /= m3d->node_spacing ;
#endif
      xgrad += delta * dx ; ygrad += delta * dy ; zgrad += delta * dz ; 
      if (!finitep(xgrad) || !finitep(ygrad) || !finitep(zgrad) ||
          (fabs(dx) > 1e5) || (fabs(dy) > 1e5) || (fabs(dz) > 1e5))
        DiagBreak() ;
    }
  }
  
  *pdx = l_distance * xgrad ; 
  *pdy = l_distance * ygrad ; 
  *pdz = l_distance * zgrad ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          Compute the directional derivative of the area term
          w.r.t the i,j,kth node.
------------------------------------------------------*/
#define AREA_NEIGHBORS 4
static int
mri3DAreaTerm(MORPH_3D *m3d, double l_area, int i, int j, int k,
              double *pdx, double *pdy, double *pdz)
{
  MORPH_NODE     *mn, *mni, *mnj, *mnk ;
  MNP            *mnp ;
  float          delta, node_spacing, n3 ;
  int            n ;
  static VECTOR  *v_a = NULL, *v_b, *v_c, *v_c_x_b, *v_b_x_a,*v_a_x_c,*v_grad,
                 *v_tmp ;

  if ((FZERO(l_area)) || 
      (i >= m3d->width-1 || j >= m3d->height-1 || k >= m3d->depth-1) ||
      i <= 0 || j <= 0 || k <= 0)
  {
    *pdx = *pdy = *pdz = 0.0 ;
    return(NO_ERROR) ;
  }

  node_spacing = m3d->node_spacing ; 
#if SCALE_INVARIANT
  n3 = node_spacing * node_spacing * node_spacing ; 
#else
  n3 = 1.0f ;
#endif
  if (!v_a)   /* initialize */
  {
    v_a = VectorAlloc(3, MATRIX_REAL) ;
    v_b = VectorAlloc(3, MATRIX_REAL) ;
    v_c = VectorAlloc(3, MATRIX_REAL) ;
    v_grad = VectorAlloc(3, MATRIX_REAL) ;
    v_c_x_b = VectorAlloc(3, MATRIX_REAL) ;
    v_b_x_a = VectorAlloc(3, MATRIX_REAL) ;
    v_a_x_c = VectorAlloc(3, MATRIX_REAL) ;
    v_tmp = VectorAlloc(3, MATRIX_REAL) ;
  }

  for (n = 0 ; n < AREA_NEIGHBORS ; n++)
  {
    /* assign mn pointers to appropriate nodes */
    switch (n)
    {
    default:
    case 0:    /* first do central node */
      mn = &m3d->nodes[i][j][k] ;
      mnp = &m3d->pnodes[i][j][k] ;
      mni = &m3d->nodes[i+1][j][k] ;
      mnj = &m3d->nodes[i][j+1][k] ;
      mnk = &m3d->nodes[i][j][k+1] ;
      break ;
    case 1:       /*  i-1 */
      mn = &m3d->nodes[i-1][j][k] ;
      mnp = &m3d->pnodes[i-1][j][k] ;
      mni = &m3d->nodes[i][j][k] ;
      mnj = &m3d->nodes[i-1][j+1][k] ;
      mnk = &m3d->nodes[i-1][j][k+1] ;
      break ;
    case 2:       /* j-1 */
      mn = &m3d->nodes[i][j-1][k] ;
      mnp = &m3d->pnodes[i][j-1][k] ;
      mni = &m3d->nodes[i+1][j-1][k] ;
      mnj = &m3d->nodes[i][j][k] ;
      mnk = &m3d->nodes[i][j-1][k+1] ;
      break ;
    case 3:      /* k-1 */
      mn = &m3d->nodes[i][j][k-1] ;
      mnp = &m3d->pnodes[i][j][k-1] ;
      mni = &m3d->nodes[i+1][j][k-1] ;
      mnj = &m3d->nodes[i][j+1][k-1] ;
      mnk = &m3d->nodes[i][j][k] ;
      break ;
    }

    /* compute cross products and area delta */
    V3_LOAD(v_a, mni->x - mn->x, mni->y - mn->y, mni->z - mn->z) ;
    V3_LOAD(v_b, mnj->x - mn->x, mnj->y - mn->y, mnj->z - mn->z) ;
    V3_LOAD(v_c, mnk->x - mn->x, mnk->y - mn->y, mnk->z - mn->z) ;
    delta = (mnp->orig_area - mnp->area) / n3 ;
    if (mnp->area < 0)
      DiagBreak() ;
    if (delta > 100000)
      DiagBreak() ;
    
    /* compute cross-products and add the appropriate 
       (i.e. scaled by area difference) cross-products to the gradient */
    switch (n)
    {
    default:
    case 0:    /* first do central node */
      V3_CROSS_PRODUCT(v_c, v_b, v_c_x_b) ;
      V3_CROSS_PRODUCT(v_b, v_a, v_b_x_a) ;
      V3_CROSS_PRODUCT(v_a, v_c, v_a_x_c) ;
      V3_ADD(v_c_x_b, v_b_x_a, v_tmp) ;
      V3_ADD(v_a_x_c, v_tmp, v_tmp) ;
      V3_SCALAR_MUL(v_tmp, delta, v_grad) ;
      break ;
    case 1:       /*  i-1 */
      V3_CROSS_PRODUCT(v_c, v_b, v_c_x_b) ;
      V3_SCALAR_MUL(v_c_x_b, -delta, v_tmp) ;
      V3_ADD(v_tmp, v_grad, v_grad) ;
      break ;
    case 2:      /* j-1 */
      V3_CROSS_PRODUCT(v_a, v_c, v_a_x_c) ;
      V3_SCALAR_MUL(v_a_x_c, -delta, v_tmp) ;
      V3_ADD(v_tmp, v_grad, v_grad) ;
      break ;
    case 3:      /* k-1 */
      V3_CROSS_PRODUCT(v_b, v_a, v_b_x_a) ;
      V3_SCALAR_MUL(v_b_x_a, -delta, v_tmp) ;
      V3_ADD(v_tmp, v_grad, v_grad) ;
      break ;
    }
  }
  l_area /= (float)AREA_NEIGHBORS ;
  *pdx = l_area * V3_X(v_grad) ;
  *pdy = l_area * V3_Y(v_grad) ;
  *pdz = l_area * V3_Z(v_grad) ;
  return(NO_ERROR) ;
}
static int
mri3DNonlinearAreaTerm(MORPH_3D *m3d, double l_nlarea, int i, int j, int k, 
                       double *pdx, double *pdy, double *pdz, 
                       MORPH_PARMS *parms)
{
  MORPH_NODE     *mn, *mni, *mnj, *mnk ;
  MNP            *mnp ;
  float          delta, delta_scale, ratio, node_spacing, n3 ;
  int            n ;
  static VECTOR  *v_a = NULL, *v_b, *v_c, *v_c_x_b, *v_b_x_a,*v_a_x_c,*v_grad,
                 *v_tmp ;

  if ((FZERO(l_nlarea)) || 
      (i >= m3d->width-1 || j >= m3d->height-1 || k >= m3d->depth-1) ||
      i <= 0 || j <= 0 || k <= 0)
  {
    *pdx = *pdy = *pdz = 0.0 ;
    return(NO_ERROR) ;
  }

  node_spacing = m3d->node_spacing ; 
#if SCALE_INVARIANT
  n3 = node_spacing * node_spacing * node_spacing ; 
#else
  n3 = 1.0f ;
#endif
  if (!v_a)   /* initialize */
  {
    v_a = VectorAlloc(3, MATRIX_REAL) ;
    v_b = VectorAlloc(3, MATRIX_REAL) ;
    v_c = VectorAlloc(3, MATRIX_REAL) ;
    v_grad = VectorAlloc(3, MATRIX_REAL) ;
    v_c_x_b = VectorAlloc(3, MATRIX_REAL) ;
    v_b_x_a = VectorAlloc(3, MATRIX_REAL) ;
    v_a_x_c = VectorAlloc(3, MATRIX_REAL) ;
    v_tmp = VectorAlloc(3, MATRIX_REAL) ;
  }

  for (n = 0 ; n < AREA_NEIGHBORS ; n++)
  {
    /* assign mn pointers to appropriate nodes */
    switch (n)
    {
    default:
    case 0:    /* first do central node */
      mn = &m3d->nodes[i][j][k] ;
      mnp = &m3d->pnodes[i][j][k] ;
      mni = &m3d->nodes[i+1][j][k] ;
      mnj = &m3d->nodes[i][j+1][k] ;
      mnk = &m3d->nodes[i][j][k+1] ;
      break ;
    case 1:       /*  i-1 */
      mn = &m3d->nodes[i-1][j][k] ;
      mnp = &m3d->pnodes[i-1][j][k] ;
      mni = &m3d->nodes[i][j][k] ;
      mnj = &m3d->nodes[i-1][j+1][k] ;
      mnk = &m3d->nodes[i-1][j][k+1] ;
      break ;
    case 2:       /* j-1 */
      mn = &m3d->nodes[i][j-1][k] ;
      mnp = &m3d->pnodes[i][j-1][k] ;
      mni = &m3d->nodes[i+1][j-1][k] ;
      mnj = &m3d->nodes[i][j][k] ;
      mnk = &m3d->nodes[i][j-1][k+1] ;
      break ;
    case 3:      /* k-1 */
      mn = &m3d->nodes[i][j][k-1] ;
      mnp = &m3d->pnodes[i][j][k-1] ;
      mni = &m3d->nodes[i+1][j][k-1] ;
      mnj = &m3d->nodes[i][j+1][k-1] ;
      mnk = &m3d->nodes[i][j][k] ;
      break ;
    }

    /* compute cross products and area delta */
    V3_LOAD(v_a, mni->x - mn->x, mni->y - mn->y, mni->z - mn->z) ;
    V3_LOAD(v_b, mnj->x - mn->x, mnj->y - mn->y, mnj->z - mn->z) ;
    V3_LOAD(v_c, mnk->x - mn->x, mnk->y - mn->y, mnk->z - mn->z) ;
    delta = (mnp->orig_area - mnp->area) / n3 ;
#if 0
    ratio = delta_scale = 1 ;
#else
#define MAX_AREA_SCALE (l_nlarea)
    /* scale up the area coefficient if the area of the current node is
       close to 0 or already negative */
    if (!FZERO(mnp->orig_area))
      ratio = mnp->area / mnp->orig_area ;
    else
    {
      ratio = 0 ;
      fprintf(stderr, "orig area = 0 at (%d, %d, %d)!!!\n", i, j, k) ;
    }
    if (mnp->area < 0)
      DiagBreak() ;
    /*delta = mnp->area / mnp->orig_area ;delta = 100*pow(0.001, abs(10*x));*/
#if 0
    delta_scale = l_nlarea * (1 - (1/(1.0+exp(-parms->exp_k*ratio)))) ;
#else
    delta_scale = l_nlarea/(1.0+exp(parms->exp_k*ratio)) ;
#endif
    if (delta_scale > 10000)
      DiagBreak() ;
#endif
    delta *= delta_scale ;
    if (delta > 100000)
      DiagBreak() ;
    if (!finitep(delta_scale))
      DiagBreak() ;
    
    
    /* compute cross-products and add the appropriate 
       (i.e. scaled by area difference) cross-products to the gradient */
    switch (n)
    {
    default:
    case 0:    /* first do central node */
      V3_CROSS_PRODUCT(v_c, v_b, v_c_x_b) ;
      V3_CROSS_PRODUCT(v_b, v_a, v_b_x_a) ;
      V3_CROSS_PRODUCT(v_a, v_c, v_a_x_c) ;
      V3_ADD(v_c_x_b, v_b_x_a, v_tmp) ;
      V3_ADD(v_a_x_c, v_tmp, v_tmp) ;
      V3_SCALAR_MUL(v_tmp, delta, v_grad) ;
      break ;
    case 1:       /*  i-1 */
      V3_CROSS_PRODUCT(v_c, v_b, v_c_x_b) ;
      V3_SCALAR_MUL(v_c_x_b, -delta, v_tmp) ;
      V3_ADD(v_tmp, v_grad, v_grad) ;
      break ;
    case 2:      /* j-1 */
      V3_CROSS_PRODUCT(v_a, v_c, v_a_x_c) ;
      V3_SCALAR_MUL(v_a_x_c, -delta, v_tmp) ;
      V3_ADD(v_tmp, v_grad, v_grad) ;
      break ;
    case 3:      /* k-1 */
      V3_CROSS_PRODUCT(v_b, v_a, v_b_x_a) ;
      V3_SCALAR_MUL(v_b_x_a, -delta, v_tmp) ;
      V3_ADD(v_tmp, v_grad, v_grad) ;
      break ;
    }
  }
  *pdx = V3_X(v_grad) ;
  *pdy = V3_Y(v_grad) ;
  *pdz = V3_Z(v_grad) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define AVG_LEN  10
static int
mri3DAlignPyramidLevel(MRI *mri_in, MRI *mri_ref, MRI *mri_ref_blur, 
                       MP *parms, MORPH_3D *m3d)
{
  double  dt, sse, rms, nvox, intensity_rms, distance_rms, area_rms, d,
          old_rms, last_avg, current_avg ;
  int     n, last_neg ;

  mri_ref->mean = MRImeanFrame(mri_ref, 1) ;
  if (DZERO(mri_ref->mean))
  {
    /*    ErrorPrintf(ERROR_BADPARM, "AlignPyramidLevel: mean std is 0!!") ;*/
    mri_ref->mean = 1.0 ;
  }

#if 0
  dt = parms->dt * mri_in->thick /** mri_in->thick*/ ;
#else
  dt = parms->dt ;
#endif
  nvox = m3d->width * m3d->height * m3d->depth ;
  mri3DcomputeMetricProperties(m3d) ;
  sse = mri3DSSE(mri_in, mri_ref, parms, m3d,
                 &intensity_rms, &distance_rms, &area_rms) ; 
  old_rms = rms = sqrt(sse / nvox) ;
  old_rms += parms->tol*rms ;
  log3DIntegration(parms, m3d,0,dt,rms, intensity_rms, distance_rms,area_rms);

  last_neg = m3d->neg ;
  if ((Gdiag & DIAG_WRITE) && (parms->write_iterations > 0) && !parms->start_t)
  {
    /*    char fname[100] ;*/
    write3DSnapshot(m3d->mri_in, m3d->mri_ref, parms, m3d, 0) ;

#if 0
    sprintf(fname, "%sref.mgh", parms->base_name) ;
    MRIwrite(mri_ref, fname) ;
#endif
  }

  last_avg = old_rms ;
  current_avg = rms ;
  for (n = parms->start_t ; n < parms->start_t+parms->niterations ; n++)
  {
#if 0
    if (((n+1)%5) == 0)
    {
      if (((n+1)%15) == 0)
        parms->l_area = 0.0 ;
      else
        parms->l_area = 0.1 ;
    }
#endif
    d = mri3DIntegrationStep(mri_in, mri_ref, mri_ref_blur, parms, m3d, dt) ;
    mri3DcomputeMetricProperties(m3d) ;
    sse = mri3DSSE(mri_in, mri_ref, parms, m3d,
                   &intensity_rms, &distance_rms, &area_rms) ; 
    rms = sqrt(sse / nvox) ;
    if ((Gdiag & DIAG_WRITE) &&
        (parms->write_iterations > 0) && 
        (((n+1) % parms->write_iterations) == 0))
      write3DSnapshot(m3d->mri_in, m3d->mri_ref, parms, m3d, n+1) ;
    log3DIntegration(parms,m3d,n+1,d,rms,intensity_rms,distance_rms,area_rms);
    if  ((((n+1)-parms->start_t) % AVG_LEN) == 0)
    {
      current_avg /= AVG_LEN ;
      fprintf(stderr, "last avg=%2.2f, current avg = %2.2f, ratio=%2.3f\n",
              last_avg, current_avg, (last_avg - current_avg) / current_avg);
      if ((last_avg - current_avg) / current_avg < parms->tol) 
        break ;
      last_avg = current_avg ;
      current_avg = rms ;
    }
    else
      current_avg += rms ;
  }

  if (n < parms->niterations+parms->start_t)
    n++ ;   /* broke in while, n will be one too small (never incremented) */
  parms->start_t = n ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static double
mri3DIntegrationStep(MRI *mri_in, MRI *mri_ref, MRI *mri_ref_blur,
                     MORPH_PARMS *parms, MORPH_3D *m3d, double dt)
{
  double     dx, dy, dz, xgrad, ygrad, zgrad ;
  int        width, height, depth, x, y, z, xsi, ysi, zsi, *pxi, *pyi, *pzi ;
  MORPH_NODE *mn ;
  MNP        *mnp ;
  Real       xin, yin, zin, scale, xref, yref, zref ;

  mri3DclearGradient(m3d) ;
  width = mri_in->width ; height = mri_in->height ; depth = mri_in->depth ; 
  scale = m3d->node_spacing / mri_in->thick  ;
  pxi = mri_in->xi ; pyi = mri_in->yi ; pzi = mri_in->zi ; 
  for (x = 0 ; x < m3d->width ; x++)
  {
    xin = x * scale ; /* convert to voxel space of mri_in */  
    xsi = pxi[nint(xin)] ;
    for (y = 0 ; y < m3d->height ; y++)
    {
      yin = y * scale ; ysi = pyi[nint(yin)] ;    /* voxel coords of mri_in */
      for (z = 0 ; z < m3d->depth ; z++)
      {
        xgrad = ygrad = zgrad = 0.0 ;
        zin = z * scale ; zsi = pzi[nint(zin)] ;  /* voxel coords of mri_in */
        mn = &m3d->nodes[x][y][z] ;        /* find out where this voxel went */
        mnp = &m3d->pnodes[x][y][z] ;       /* find out where this voxel went */
        xref = mn->x / mri_ref->thick ;
        yref = mn->y / mri_ref->thick ; 
        zref = mn->z / mri_ref->thick ; 
        mri3DCorrelationTerm(mri_in, mri_ref, mri_ref_blur,parms->l_intensity,
                             xsi, ysi, zsi, xref, yref, zref, &dx, &dy, &dz) ;
        if (!finitep(dz) || !finitep(dy) || !finitep(dx))
          DiagBreak() ;
        xgrad += dx ; ygrad += dy ; zgrad += dz ;
        if (!finitep(xgrad) || !finitep(ygrad) || !finitep(zgrad))
          DiagBreak() ;
        mri3DDistanceTerm(m3d, parms->l_dist, x, y, z, &dx, &dy, &dz) ;
        xgrad += dx ; ygrad += dy ; zgrad += dz ;
        if (!finitep(dz) || !finitep(dy) || !finitep(dx) ||
            !finitep(xgrad) || !finitep(ygrad) || !finitep(zgrad))
          DiagBreak() ;

        mri3DAreaTerm(m3d, parms->l_area, x, y, z, &dx,&dy,&dz);
        mri3DNonlinearAreaTerm(m3d, parms->l_nlarea, x, y, z, &dx, &dy, &dz,
                               parms) ;
        xgrad += dx ; ygrad += dy ; zgrad += dz ;
        if (!finitep(dz) || !finitep(dy) || !finitep(dx) ||
            !finitep(xgrad) || !finitep(ygrad) || !finitep(zgrad))
          DiagBreak() ;

        mnp->dx = xgrad ; mnp->dy = ygrad ; mnp->dz = zgrad ;
        if (!finitep(mnp->dx) || !finitep(mnp->dy) || !finitep(mnp->dz))
          DiagBreak() ;
      }
    }
  }

#define MAX_GRAD (m3d->node_spacing / 8.0f)
  dt = mri3DscaleDeltaT(m3d, dt, MAX_GRAD) ;
  mri3DapplyGradient(m3d, dt) ;
  return(dt) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
mri3DInitDistances(MORPH_3D *m3d)
{
  int        i, j, k, width, height, depth, n, ndists ;
  MORPH_NODE *mn ;
  MNP        *mnp, *mnp_nbr ;
  double     dist, xd, yd, zd, avg_dist ;

  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  for (ndists = avg_dist = 0.0, i = 0 ; i < width ; i++)
  {
    for (j = 0 ; j < height ; j++)
    {
      for (k = 0 ; k < depth ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;        
        mnp = &m3d->pnodes[i][j][k] ;
        for (n = 0 ; n < NEIGHBORS ; n++)
        {
          switch (n)
          {
          default:
          case 0:      /* i-1 */
            if (i > 0)
              mnp_nbr = &m3d->pnodes[i-1][j][k] ;
            else
              mnp_nbr = NULL ;
            break ;
          case 1:      /* i+1 */
            if (i < width-1)
              mnp_nbr = &m3d->pnodes[i+1][j][k] ;
            else
              mnp_nbr = NULL ;
            break ;
          case 2:      /* j-1 */
            if (j > 0)
              mnp_nbr = &m3d->pnodes[i][j-1][k] ;
            else
              mnp_nbr = NULL ;
            break ;
          case 3:      /* j+1 */
            if (j < height-1)
              mnp_nbr = &m3d->pnodes[i][j+1][k] ;
            else
              mnp_nbr = NULL ;
            break ;
          case 4:      /* k-1 */
            if (k > 0)
              mnp_nbr = &m3d->pnodes[i][j][k-1] ;
            else
              mnp_nbr = NULL ;
            break ;
          case 5:      /* k+1 */
            if (k < depth-1)
              mnp_nbr = &m3d->pnodes[i][j][k+1] ;
            else
              mnp_nbr = NULL ;
            break ;
          }
          if (mnp_nbr)
          {
            xd = mnp->ox - mnp_nbr->ox ; 
            yd = mnp->oy - mnp_nbr->oy ; 
            zd = mnp->oz - mnp_nbr->oz ;
            dist = sqrt(xd*xd + yd*yd + zd*zd) ;
            if (!finitep(dist))
              DiagBreak() ;
            mnp->orig_dist[n] = dist ;
            avg_dist += mnp->orig_dist[n] ; ndists++ ;
          }
        }
      }
    }
  }
  if (Gdiag & DIAG_SHOW)
  {
    avg_dist /= (double)ndists ;
    fprintf(stderr, "average node spacing = %2.3f\n", avg_dist) ;
  }
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
mri3DInitAreas(MORPH_3D *m3d)
{
  int        i, j, k, width, height, depth ;
  MNP        *mnp, *mnpi, *mnpj, *mnpk ;
  VECTOR     *v_a, *v_b, *v_c ;
  double     avg_area, min_area, max_area ;

  v_a = VectorAlloc(3, MATRIX_REAL) ;
  v_b = VectorAlloc(3, MATRIX_REAL) ;
  v_c = VectorAlloc(3, MATRIX_REAL) ;
  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  min_area = 100000.0 ; max_area = -1.0 ;
  for (avg_area = 0.0, i = 0 ; i < width-1 ; i++)
  {
    for (j = 0 ; j < height-1 ; j++)
    {
      for (k = 0 ; k < depth-1 ; k++)
      {
        mnp = &m3d->pnodes[i][j][k] ;
        mnpi = &m3d->pnodes[i+1][j][k] ;
        mnpj = &m3d->pnodes[i][j+1][k] ;
        mnpk = &m3d->pnodes[i][j][k+1] ;
        V3_LOAD(v_a, mnpi->ox - mnp->ox, mnpi->oy - mnp->oy, mnpi->oz-mnp->oz);
        V3_LOAD(v_b, mnpj->ox - mnp->ox, mnpj->oy - mnp->oy, mnpj->oz-mnp->oz);
        V3_LOAD(v_c, mnpk->ox - mnp->ox, mnpk->oy - mnp->oy, mnpk->oz-mnp->oz);
        mnp->orig_area = VectorTripleProduct(v_b, v_c, v_a) ;
        if (!finitep(mnp->orig_area) || FZERO(mnp->orig_area))
          DiagBreak() ;
        avg_area += mnp->orig_area ;
        if (mnp->orig_area > max_area)
          max_area = mnp->orig_area ;
        if (mnp->orig_area < min_area)
          min_area = mnp->orig_area ;
      }
    }
  }

  if (Gdiag & DIAG_SHOW)
  {
    avg_area /= (double)((m3d->width-1)*(m3d->height-1)*(m3d->depth-1)) ;
    fprintf(stderr, "average node area    = %2.3f [%2.3f --> %2.3f]\n", 
            avg_area, min_area, max_area) ;
  }

  VectorFree(&v_a) ; VectorFree(&v_b) ; VectorFree(&v_c) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
log3DIntegration(MORPH_PARMS *parms, MORPH_3D *m3d,int n,double dt,double rms, 
                 double intensity_rms, double distance_rms, double area_rms)
{
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "%3.3d: dt = %2.4f, "
            "rms = %2.3f (%2.2f, %2.2f, %2.2f), neg = %d\n", 
            n, dt, rms,
            intensity_rms, distance_rms, area_rms, m3d->neg) ;
  if (Gdiag & DIAG_WRITE)
  {
    fprintf(parms->log_fp, 
            "%3.3d: dt = %2.4f, "
            "rms = %2.3f (%2.2f, %2.2f, %2.2f), neg=%d\n", 
            n, dt, rms, intensity_rms, distance_rms, area_rms, m3d->neg) ;
    fflush(parms->log_fp) ;
  }
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
#include "image.h"
static int
write3DSnapshot(MRI *mri_in,MRI *mri_ref,MORPH_PARMS *parms,
                MORPH_3D *m3d, int n)
{
  MRI   *mri_tmp ;
  char  fname[200] ;

  if (!(Gdiag & DIAG_WRITE))
    return(NO_ERROR) ;

  if (!n)
  {
    sprintf(fname, "in_%s", parms->base_name) ;
    mriWriteImageViews(mri_in, fname, IMAGE_SIZE) ;
#if 0
    sprintf(fname, "in_%s.mgh", parms->base_name) ;
    fprintf(stderr, "writing volume to %s...\n", fname) ;
    MRIwrite(mri_in, fname) ;
#endif
  }

  mri_tmp = MRIapplyInverse3DMorph(mri_ref, m3d, NULL) ;
  sprintf(fname, "%s%3.3d", parms->base_name, n) ;
  mriWriteImageViews(mri_tmp, fname, IMAGE_SIZE) ;
  if (((n) % (5*parms->write_iterations)) == 0)
  {
    sprintf(fname, "%s%3.3d.mgh", parms->base_name, n) ;
    fprintf(stderr, "writing volume to %s...\n", fname) ;
    MRIwrite(mri_tmp, fname) ;
    MRIfree(&mri_tmp) ;
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
static int
mriWriteImageViews(MRI *mri, char *base_name, int target_size)
{
  if (Gdiag & DIAG_SHOW)
    fprintf(stderr, "writing image views to ???_%s.hipl...\n", base_name) ;
  mriWriteImageView(mri, base_name, target_size, MRI_CORONAL, -1) ; 
  mriWriteImageView(mri, base_name, target_size, MRI_SAGITTAL, -1) ;
  mriWriteImageView(mri, base_name, target_size, MRI_HORIZONTAL, -1) ; 
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
static int
mriWriteImageView(MRI *mri, char *base_name, int target_size, int view, 
                  int slice)
{
  char  fname[200], *prefix ;
  IMAGE *I ;
  float scale ;

  switch (view)
  {
  default:
  case MRI_CORONAL:    prefix = "cor" ; break ;
  case MRI_SAGITTAL:   prefix = "sag" ; break ;
  case MRI_HORIZONTAL: prefix = "hor" ; break ;
  }

  if (slice < 0)    /* pick middle slice */
  {
    switch (view)
    {
    default:
    case MRI_CORONAL:    slice = mri->depth/2; break ;
    case MRI_SAGITTAL:   slice = mri->width/2 ; break ;
    case MRI_HORIZONTAL: slice = mri->height/2 ; break ;
    }
  }
  I = MRItoImageView(mri, NULL, slice, view, 0) ;
  scale = (float)target_size / (float)I->rows ;
  if (!FEQUAL(scale, 1.0f))
  {
    IMAGE *Itmp ;

    Itmp = ImageRescale(I, NULL, scale) ;
    ImageFree(&I) ;
    I = Itmp ;
  }
  sprintf(fname, "%s_%s.hipl", prefix, base_name) ;
  ImageWrite(I, fname) ;
  ImageFree(&I) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
mri3DcomputeMetricProperties(MORPH_3D *m3d)
{
  double     area ;
  int        i, j, k, width, height, depth ;
  MORPH_NODE *mn, *mni, *mnj, *mnk ;
  MNP        *mnp ;
  VECTOR     *v_a, *v_b, *v_c ;

  v_a = VectorAlloc(3, MATRIX_REAL) ;
  v_b = VectorAlloc(3, MATRIX_REAL) ;
  v_c = VectorAlloc(3, MATRIX_REAL) ;
  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  m3d->neg = 0 ;
  for (i = 0 ; i < width-1 ; i++)
  {
    for (j = 0 ; j < height-1 ; j++)
    {
      for (k = 0 ; k < depth-1 ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        mni = &m3d->nodes[i+1][j][k] ;
        mnj = &m3d->nodes[i][j+1][k] ;
        mnk = &m3d->nodes[i][j][k+1] ;
        V3_LOAD(v_a, mni->x - mn->x, mni->y - mn->y, mni->z - mn->z) ;
        V3_LOAD(v_b, mnj->x - mn->x, mnj->y - mn->y, mnj->z - mn->z) ;
        V3_LOAD(v_c, mnk->x - mn->x, mnk->y - mn->y, mnk->z - mn->z) ;
        area = mnp->area = VectorTripleProduct(v_b, v_c, v_a) ;
        if (area <= 0)
          m3d->neg++ ;
      }
    }
  }

  VectorFree(&v_a) ; VectorFree(&v_b) ; VectorFree(&v_c) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
mri3DclearGradient(MORPH_3D *m3d)
{
  int        i, j, k, width, height, depth ;
  MNP        *mnp ;

  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  for (i = 0 ; i < width ; i++)
  {
    for (j = 0 ; j < height ; j++)
    {
      for (k = 0 ; k < depth ; k++)
      {
        mnp = &m3d->pnodes[i][j][k] ;
        mnp->dx = mnp->dy = mnp->dz = 0.0f ;
      }
    }
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define DI  42
#define DJ  90
#define DK  104

static int
mri3DapplyGradient(MORPH_3D *m3d, double dt)
{
  int        i, j, k, width, height, depth ;
  MORPH_NODE *mn ;
  MNP        *mnp ;
  double     dx, dy, dz ;

  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  for (i = 0 ; i < width ; i++)
  {
    for (j = 0 ; j < height ; j++)
    {
      for (k = 0 ; k < depth ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        dx = mnp->dx ; dy = mnp->dy ; dz = mnp->dz ;
        mn->x += dx * dt ; mn->y += dy * dt ; mn->z += dz * dt ;
      }
    }
  }

  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static int
finitep(float f)
{
  return(1) ;
  if (!finite(f))
    return(0) ;
  if (fabs(f) > 1e5)
    return(0) ;
  return(1) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
static double
mri3DscaleDeltaT(MORPH_3D *m3d, double dt, double max_len)
{
  int        i, j, k, width, height, depth, maxi, maxj, maxk ;
  MORPH_NODE *mn ;
  MNP        *mnp ;
  double     len, dx, dy, dz, max_delta, new_dt ;

  maxi = maxj = maxk = -1 ;
  width = m3d->width ; height = m3d->height ; depth = m3d->depth ; 
  for (max_delta = 0.0f, i = 0 ; i < width ; i++)
  {
    for (j = 0 ; j < height ; j++)
    {
      for (k = 0 ; k < depth ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mnp = &m3d->pnodes[i][j][k] ;
        dx = mnp->dx ; dy = mnp->dy ; dz = mnp->dz ;
        len = sqrt(dx*dx + dy*dy + dz*dz) ;
        if (len > max_delta)
        {
          maxi = i ; maxj = j ; maxk = k ;
          max_delta = len ;
        }
      }
    }
  }

  if (Gdiag & DIAG_SHOW)
    fprintf(stderr,"max delta %2.3f at (%d,%d,%d)\n",max_delta,maxi,maxj,maxk);
  if (max_delta * dt > max_len)
  {
    new_dt = max_len / max_delta ;
    if (new_dt < dt)
      dt = new_dt ;
  }
  return(dt) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define UNUSED_SPACE_SIZE  256

int
MRI3Dwrite(MORPH_3D *m3d, char *fname)
{
  FILE        *fp ;
  int         i, j, k ;
  MORPH_NODE  *mn ;
  char        buf[UNUSED_SPACE_SIZE+1] ;

  fp = fopen(fname, "wb") ;
  if (!fp)
    ErrorReturn(ERROR_NOFILE, 
                (ERROR_NOFILE, "mris3Dwrite: could not open file %s", fname)) ;

  if (fwriteInt(M3D_MAGIC, fp) != 1)
    ErrorReturn(ERROR_BADFILE, (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
  if (fwriteInt(M3D_VERSION, fp) != 1)
    ErrorReturn(ERROR_BADFILE, (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
  if (fwriteInt(m3d->width, fp) != 1)
    ErrorReturn(ERROR_BADFILE, (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
  if (fwriteInt(m3d->height, fp) != 1)
    ErrorReturn(ERROR_BADFILE, (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
  if (fwriteInt(m3d->depth, fp) != 1)
    ErrorReturn(ERROR_BADFILE, (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
  if (fwriteFloat(m3d->node_spacing, fp) != 1)
    ErrorReturn(ERROR_BADFILE, (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;

  /* so stuff can be added to the header in the future */
  memset(buf, 0, UNUSED_SPACE_SIZE*sizeof(char)) ;
  fwrite(buf, sizeof(char), UNUSED_SPACE_SIZE, fp) ;
  for (i = 0 ; i < m3d->width ; i++)
  {
    for (j = 0 ; j < m3d->height ; j++)
    {
      for (k = 0 ; k < m3d->depth ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        if (fwriteFloat(mn->x, fp) != 1)
          ErrorReturn(ERROR_BADFILE, 
                      (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
        if (fwriteFloat(mn->y, fp) != 1)
          ErrorReturn(ERROR_BADFILE, 
                      (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
        if (fwriteFloat(mn->z, fp) != 1)
          ErrorReturn(ERROR_BADFILE, 
                      (ERROR_BADFILE, "mri3Dwrite: fwrite failed")) ;
      }
    }
  }


  fclose(fp) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           Read a 3d morph from a file, and initialize it,
           not allocating the properties field to conserve memory.
------------------------------------------------------*/
MORPH_3D  *
MRI3DreadSmall(char *fname)
{
  MORPH_3D *m3d ;
  FILE        *fp ;
  int         i, j, k, width, height, depth, magic, version ;
  float       node_spacing ;
  MORPH_NODE  *mn ;
  char        buf[UNUSED_SPACE_SIZE+1] ;

  fp = fopen(fname, "rb") ;
  if (!fp)
    ErrorReturn(NULL, 
                (ERROR_NOFILE, 
                 "MRI3DreadSmall: could not open file %s", fname)) ;

  magic = freadInt(fp) ;
  if (magic != M3D_MAGIC)
  {
    fclose(fp) ;
    ErrorReturn(NULL, 
                (ERROR_BADFILE, "MRI3DreadSmall: file %s not a 3d morph file", 
                 fname)) ;
  }
  version = freadInt(fp) ;
  width = freadInt(fp) ;
  height = freadInt(fp) ;
  depth = freadInt(fp) ;
  node_spacing = freadFloat(fp) ;

  /* so stuff can be added to the header in the future */
  fread(buf, sizeof(char), UNUSED_SPACE_SIZE, fp) ;

  m3d = mri3Dalloc(width, height, depth, node_spacing) ;
  for (i = 0 ; i < m3d->width ; i++)
  {
    for (j = 0 ; j < m3d->height ; j++)
    {
      for (k = 0 ; k < m3d->depth ; k++)
      {
        mn = &m3d->nodes[i][j][k] ;
        mn->x = freadFloat(fp);
        mn->y = freadFloat(fp) ;
        mn->z = freadFloat(fp) ;
      }
    }
  }

  fclose(fp) ;
  return(m3d) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           Read a 3d morph from a file, and initialize it.
------------------------------------------------------*/
MORPH_3D *
MRI3Dread(char *fname)
{
  MORPH_3D *m3d ;

  m3d = MRI3DreadSmall(fname) ;
  if (!m3d)
    return(NULL) ;

  mri3DallocProperties(m3d) ;
  mri3DcomputeMetricProperties(m3d) ;
  return(m3d) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           Read a 3d morph from a file, and initialize it.
------------------------------------------------------*/
#if 0
static MORPH_3D *
mri3DscaleUp2(MORPH_3D *m3d_in, MORPH_3D *m3d_out, MORPH_PARMS *parms)
{
  int         width, height, depth, i, j, k, alloced = 0 ;
  MORPH_NODE  *mn ;
  float       x, y, z, node_spacing ;
  VECTOR     *v_X, *v_Y ;
#if 0
  float      dx, dy, dz ;
  int        n ;
#endif
  v_X = VectorAlloc(4, MATRIX_REAL) ;
  v_Y = VectorAlloc(4, MATRIX_REAL) ;

  width  = m3d_in->width *2-1 ; 
  height = m3d_in->height*2-1 ; 
  depth  = m3d_in->depth *2-1 ;
  node_spacing = m3d_in->node_spacing/2.0 ;
  if (!m3d_out)
  {
    alloced = 1 ;
    m3d_out = mri3Dalloc(width, height, depth, node_spacing) ;
  }

  for (i = 0 ; i < width ; i++)
  {
    x = V3_X(v_X) = (float)i * node_spacing ;
    for (j = 0 ; j < height ; j++)
    {
      y = V3_Y(v_X) = (float)j * node_spacing ;
      for (k = 0 ; k < depth ; k++)
      {
        z = V3_Z(v_X) = (float)k * node_spacing ;
        mn = &m3d_out->nodes[i][j][k] ;
        MRIsample3Dmorph(m3d_in, x, y, z, &mn->x, &mn->y, &mn->z) ;
        if (alloced)
        {
          LTAtransformPoint(parms->lta, v_X, v_Y) ;
          mn->ox = V3_X(v_Y) ; mn->oy = V3_Y(v_Y) ; mn->oz = V3_Z(v_Y) ; 
        }

#if 0
        /* handle boundary conditions */
        dx = dy = dz = 0.0f ; n = 0 ;
        if (i == width-1)
        {
          dx += m3d_out->nodes[i-1][j][k].x - m3d_out->nodes[i-2][j][k].x ;
          dy += m3d_out->nodes[i-1][j][k].y - m3d_out->nodes[i-2][j][k].y ;
          dz += m3d_out->nodes[i-1][j][k].z - m3d_out->nodes[i-2][j][k].z ;
          n++ ;
        }
        if (j == height-1)
        {
          dx += m3d_out->nodes[i][j-1][k].x - m3d_out->nodes[i][j-2][k].x ;
          dy += m3d_out->nodes[i][j-1][k].y - m3d_out->nodes[i][j-2][k].y ;
          dz += m3d_out->nodes[i][j-1][k].z - m3d_out->nodes[i][j-2][k].z ;
          n++ ;
        }
        if (k == depth-1)
        {
          dx += m3d_out->nodes[i][j][k-1].x - m3d_out->nodes[i][j][k-2].x ;
          dy += m3d_out->nodes[i][j][k-1].y - m3d_out->nodes[i][j][k-2].y ;
          dz += m3d_out->nodes[i][j][k-1].z - m3d_out->nodes[i][j][k-2].z ;
          n++ ;
        }
        if (n > 0)  /* on one of the outside borders */
        {
          dx /= (float)n ; dy /= (float)n ; dz /= (float)n ; 
          mn->x += dx ; mn->y += dy ; mn->z += dz ; 
        }
#endif
        if (!finitep(mn->x) || !finitep(mn->y) || !finitep(mn->z) || 
            !finitep(mn->ox) || !finitep(mn->oy) || !finitep(mn->oz))
          DiagBreak() ;
      }
    }
  }
  if (alloced)
  {
    mri3DallocProperties(m3d_out) ;
    mri3DInitDistances(m3d_out) ;
    mri3DInitAreas(m3d_out) ;
  }
  return(m3d_out) ;
}
#else
static MORPH_3D *
mri3DscaleUp2(MORPH_3D *m3d_in, MORPH_3D *m3d_out, MORPH_PARMS *parms)
{
  int         width, height, depth, i, j, k, alloced = 0 ;
  MORPH_NODE  *mn, *mn0, *mn1 ;
  MNP         *mnp ;
  float       x, y, z, node_spacing ;
  VECTOR     *v_X, *v_Y ;
  float      dx, dy, dz ;
#if 0
  int        n ;
#endif
  v_X = VectorAlloc(4, MATRIX_REAL) ;
  v_Y = VectorAlloc(4, MATRIX_REAL) ;

  width  = m3d_in->width *2-1 ; 
  height = m3d_in->height*2-1 ; 
  depth  = m3d_in->depth *2-1 ;
  node_spacing = m3d_in->node_spacing/2.0 ;
  if (!m3d_out)
  {
    alloced = 1 ;
    m3d_out = mri3Dalloc(width, height, depth, node_spacing) ;
  }
  m3d_out->lta = m3d_in->lta ;

  width = m3d_in->width ; height = m3d_in->height ; depth = m3d_in->depth ; 
  for (i = 0 ; i < width ; i++)
  {
    for (j = 0 ; j < height ; j++)
    {
      for (k = 0 ; k < depth ; k++)
      {
        mn = &m3d_out->nodes[i*2][j*2][k*2] ;
        mn0 = &m3d_in->nodes[i][j][k] ;
        mn->x = mn0->x ; mn->y = mn0->y ; mn->z = mn0->z ;
        if (i < width-1)
        {
          mn = &m3d_out->nodes[i*2+1][j*2][k*2] ;
          mn1 = &m3d_in->nodes[i+1][j][k] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
        if (j < height-1)
        {
          mn = &m3d_out->nodes[i*2][j*2+1][k*2] ;
          mn1 = &m3d_in->nodes[i][j+1][k] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
        if (k < depth-1)
        {
          mn = &m3d_out->nodes[i*2][j*2][k*2+1] ;
          mn1 = &m3d_in->nodes[i][j][k+1] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
        if (i < width-1 && j < height-1)
        {
          mn = &m3d_out->nodes[i*2+1][j*2+1][k*2] ;
          mn1 = &m3d_in->nodes[i+1][j+1][k] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
        if (i < width-1 && k < depth-1)
        {
          mn = &m3d_out->nodes[i*2+1][j*2][k*2+1] ;
          mn1 = &m3d_in->nodes[i+1][j][k+1] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
        if (j < height-1 && k < depth-1)
        {
          mn = &m3d_out->nodes[i*2][j*2+1][k*2+1] ;
          mn1 = &m3d_in->nodes[i][j+1][k+1] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
        if (i < width-1 && j < height-1 && k < depth-1)
        {
          mn = &m3d_out->nodes[i*2+1][j*2+1][k*2+1] ;
          mn1 = &m3d_in->nodes[i+1][j+1][k+1] ;
          dx = mn1->x - mn0->x ; dy = mn1->y - mn0->y ; dz = mn1->z - mn0->z ; 
          mn->x = mn0->x+dx/2 ;
          mn->y = mn0->y+dy/2 ; 
          mn->z = mn0->z+dz/2 ; 
        }
      }
    }
  }


  width  = m3d_in->width *2-1 ; 
  height = m3d_in->height*2-1 ; 
  depth  = m3d_in->depth *2-1 ;
  MRI3DmorphFree(&m3d_in) ;

  mri3DallocProperties(m3d_out) ;
  for (i = 0 ; i < width ; i++)
  {
    x = V3_X(v_X) = (float)i * node_spacing ;
    for (j = 0 ; j < height ; j++)
    {
      y = V3_Y(v_X) = (float)j * node_spacing ;
      for (k = 0 ; k < depth ; k++)
      {
        z = V3_Z(v_X) = (float)k * node_spacing ;
        mn  = &m3d_out->nodes[i][j][k] ;
        mnp = &m3d_out->pnodes[i][j][k] ;
        /*        MRIsample3Dmorph(m3d_in, x, y, z, &mn->x, &mn->y, &mn->z) ;*/
        if (alloced)
        {
#if !USE_ORIGINAL_PROPTERIES
          LTAtransformPoint(parms->lta, v_X, v_Y) ;
#else
          MatrixCopy(v_X, v_Y) ;
#endif
          mnp->ox = V3_X(v_Y) ; mnp->oy = V3_Y(v_Y) ; mnp->oz = V3_Z(v_Y) ; 
        }

        if (!finitep(mn->x) || !finitep(mn->y) || !finitep(mn->z) || 
            !finitep(mnp->ox) || !finitep(mnp->oy) || !finitep(mnp->oz))
          DiagBreak() ;
      }
    }
  }

  mri3DcomputeMetricProperties(m3d_out) ;
  if (alloced)
  {
    mri3DInitDistances(m3d_out) ;
    mri3DInitAreas(m3d_out) ;
  }
  return(m3d_out) ;
}
#endif
#if 0
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           Read a 3d morph from a file, and initialize it.
------------------------------------------------------*/
static MORPH_3D *
mri3DscaleDown2(MORPH_3D *m3d_in, MORPH_3D *m3d_out, MORPH_PARMS *parms)
{
  int         width, height, depth, i, j, k, alloced = 0 ;
  MORPH_NODE  *mn ;
  float       x, y, z, node_spacing ;
  VECTOR     *v_X, *v_Y ;

  v_X = VectorAlloc(4, MATRIX_REAL) ;
  v_Y = VectorAlloc(4, MATRIX_REAL) ;
  width  = m3d_in->width /2 ; 
  height = m3d_in->height/2 ; 
  depth  = m3d_in->depth /2 ;
  node_spacing = m3d_in->node_spacing*2.0 ;
  if (!m3d_out)
  {
    alloced = 1 ;
    m3d_out = mri3Dalloc(width, height, depth, node_spacing) ;
  }

  v_X->rptr[4][1] = 1.0f ;
  for (i = 0 ; i < width ; i++)
  {
    x = V3_X(v_X) = (float)i * node_spacing ;
    for (j = 0 ; j < height ; j++)
    {
      y = V3_Y(v_X) = (float)j * node_spacing ;
      for (k = 0 ; k < depth ; k++)
      {
        z = V3_Z(v_X) = (float)k * node_spacing ;
        LTAtransformPoint(parms->lta, v_X, v_Y) ;
        mn = &m3d_out->nodes[i][j][k] ;
        MRIsample3Dmorph(m3d_in, x, y, z, &mn->x, &mn->y, &mn->z) ;
        if (alloced)
          MRIsample3DmorphOrig(m3d_in, x, y, z, &mn->ox, &mn->oy, &mn->oz) ;
      }
    }
  }
  if (alloced)
  {
    mri3DInitDistances(m3d_out) ;
    mri3DInitAreas(m3d_out) ;
  }
  return(m3d_out) ;
}
#endif
#if 0
static float
mri3DcomputeOrigCoords(MORPH_3D *m3d, int i, int j, int k, 
                       float *pox, float *poy, float *poz)
{
  static VECTOR  *v_X, *v_Y = NULL ;

  if (v_Y == NULL)
  {
    v_X = VectorAlloc(4, MATRIX_REAL) ;
    v_Y = VectorAlloc(4, MATRIX_REAL) ;
  }
  v_X->rptr[4][1] = 1.0f /*/ mri_in->thick*/ ;
  V3_Z(v_X) = (float)k * m3d->node_spacing ;
  V3_Y(v_X) = (float)j * m3d->node_spacing ;
  V3_X(v_X) = (float)i * m3d->node_spacing ;
  LTAtransformPoint(m3d->lta, v_X, v_Y) ;

  *pox = V3_X(v_Y) ; *poy = V3_Y(v_Y) ; *poz = V3_Z(v_Y) ; 
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
static float
mri3DcomputeOrigArea(MORPH_3D *m3d, int i, int j, int k)
{
  static VECTOR  *v_a = NULL, *v_b, *v_c ;
  float          mn_ox, mn_oy, mn_oz, mni_ox, mni_oy, mni_oz, 
                 mnj_ox, mnj_oy, mnj_oz, mnk_ox, mnk_oy, mnk_oz, area ;

  if (v_a == NULL)
  {
    v_a = VectorAlloc(3, MATRIX_REAL) ;
    v_b = VectorAlloc(3, MATRIX_REAL) ;
    v_c = VectorAlloc(3, MATRIX_REAL) ;
  }

  mri3DcomputeOrigCoords(m3d, i, j, k, &mn_ox, &mn_oy, &mn_oz) ;
  mri3DcomputeOrigCoords(m3d, i+1, j, k, &mni_ox, &mni_oy, &mni_oz) ;
  mri3DcomputeOrigCoords(m3d, i, j+1, k, &mnj_ox, &mnj_oy, &mnj_oz) ;
  mri3DcomputeOrigCoords(m3d, i, j, k+1, &mnk_ox, &mnk_oy, &mnk_oz) ;
  V3_LOAD(v_a, mni_ox - mn_ox, mni_oy - mn_oy, mni_oz - mn_oz) ;
  V3_LOAD(v_b, mnj_ox - mn_ox, mnj_oy - mn_oy, mnj_oz - mn_oz) ;
  V3_LOAD(v_c, mnk_ox - mn_ox, mnk_oy - mn_oy, mnk_oz - mn_oz) ;
  area = VectorTripleProduct(v_b, v_c, v_a) ;

  return(area) ;
}
#endif
int
MRIeraseBorders(MRI *mri, int width)
{
  int i ;

  for (i = 0 ; i < width ; i++)
  {
    MRIerasePlane(mri, mri->width/2, mri->height/2, i, 0, 0, 1, 0) ;
    MRIerasePlane(mri, mri->width/2, i, mri->depth/2, 0, 1, 0, 0) ;
    MRIerasePlane(mri, i, mri->height/2, mri->depth/2, 1, 0, 0, 0) ;
    MRIerasePlane(mri, mri->width/2, mri->height/2, mri->depth-1-i,0,0, 1, 0) ;
    MRIerasePlane(mri, mri->width/2, mri->height-1-i,mri->depth/2,0, 1, 0, 0) ;
    MRIerasePlane(mri, mri->width-1-i,mri->height/2,mri->depth/2, 1, 0, 0, 0) ;
  }
  return(NO_ERROR) ;
}

static int
mriNormalizeStds(MRI *mri)
{
  int      x, y, z, width, height, depth, std, val ;
  BUFTYPE  *pstd, *pval ;
  double   mean_std ;

  if (mri->nframes < 2)
    return(NO_ERROR) ;

  mean_std = MRImeanFrame(mri, 1) ;
  if (FZERO(mean_std))
  {
    mri->mean = 0.0 ;
    return(NO_ERROR) ;
  }
  width = mri->width ; height = mri->height ; depth = mri->depth ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pval = &MRIvox(mri, 0, y, z) ;
      pstd = &MRIseq_vox(mri, 0, y, z, 1) ;
      for (x = 0 ; x < width ; x++)
      {
        val = *pval++ ;
        if (val < BACKGROUND_VAL)
        {
          std = *pstd ;
          if (std < mean_std)
            std = mean_std ;
          *pstd++ = std ;
        }
        else
          pstd++ ;
      }
    }
  }
  mri->mean = MRImeanFrame(mri, 1) ;
  return(NO_ERROR) ;
}
