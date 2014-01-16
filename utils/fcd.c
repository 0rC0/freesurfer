/**
 * @file  fcd.c
 * @brief I/O and analysis algorithms for FCDs (focal cortical dysplasias)
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: Bruce Fischl
 * CVS Revision Info:
 *    $Author: fischl $
 *    $Date: 2014/01/16 23:25:33 $
 *    $Revision: 1.2 $
 *
 * Copyright (C) 2002-2007,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */

#include "fcd.h"
#include "error.h"
#include "diag.h"
#include "cma.h"
#include "mrisegment.h"

static int
most_frequent_label(MRI *mri_seg, MRI_SEGMENT *mseg)
{
  int  label_counts[MAX_CMA_LABELS], i, max_count, max_label, label ;

  memset(label_counts, 0, sizeof(label_counts)) ;
  for (i = max_count = max_label = 0 ; i < mseg->nvoxels ; i++)
  {
    label = MRIgetVoxVal(mri_seg, mseg->voxels[i].x, mseg->voxels[i].y, mseg->voxels[i].z, 0) ;
    label_counts[label]++ ;
  }
  for (i = max_count = max_label = 0 ; i < MAX_CMA_LABELS ; i++)
  {
    if (label_counts[i] > max_count)
    {
      max_count = label_counts[i] ;
      max_label = i ;
    }
  }
  return(max_label) ;
}


FCD_DATA   *
FCDloadData(char *sdir, char *subject)
{
  FCD_DATA *fcd ;
  char     fname[STRLEN] ;

  fcd = (FCD_DATA *)calloc(1, sizeof(FCD_DATA)) ;

  sprintf(fname, "%s/%s/surf/lh.white", sdir, subject) ;
  fcd->mris_lh = MRISread(fname) ;
  if (fcd->mris_lh == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;
  MRISsaveVertexPositions(fcd->mris_lh, WHITE_VERTICES) ;

  sprintf(fname, "%s/%s/surf/rh.white", sdir, subject) ;
  fcd->mris_rh = MRISread(fname) ;
  if (fcd->mris_rh == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;
  MRISsaveVertexPositions(fcd->mris_rh, WHITE_VERTICES) ;

  sprintf(fname, "%s/%s/mri/aparc+aseg.mgz", sdir, subject) ;
  fcd->mri_aseg = MRIread(fname) ;
  if (fcd->mri_aseg == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;

  sprintf(fname, "%s/%s/mri/norm.mgz", sdir, subject) ;
  fcd->mri_norm = MRIread(fname) ;
  if (fcd->mri_norm == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;

  fcd->mri_thickness_increase = MRIclone(fcd->mri_aseg, NULL) ;
  fcd->mri_thickness_decrease = MRIclone(fcd->mri_aseg, NULL) ;

  sprintf(fname, "%s/%s/surf/lh.rh.thickness.smooth0.mgz", sdir, subject) ;
  fcd->rh_thickness_on_lh = MRIread(fname) ;
  if (fcd->mri_aseg == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;

  sprintf(fname, "%s/%s/surf/rh.thickness.mgz", sdir, subject) ;
  fcd->rh_thickness_on_rh = MRIread(fname) ;
  if (fcd->rh_thickness_on_rh == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;


  sprintf(fname, "%s/%s/surf/lh.thickness.mgz", sdir, subject) ;
  fcd->lh_thickness_on_lh = MRIread(fname) ;
  if (fcd->lh_thickness_on_lh == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;

  sprintf(fname, "%s/%s/surf/rh.lh.thickness.mgz", sdir, subject) ;
  fcd->lh_thickness_on_rh = MRIread(fname) ;
  if (fcd->lh_thickness_on_rh == NULL)
    ErrorExit(ERROR_NOFILE, "FCDloadData: couldn't load %s", fname) ;

  
  return(fcd) ;
}

int
FCDcomputeThicknessLabels(FCD_DATA *fcd, double thickness_thresh, double sigma, int size_thresh) 
{
  MRI    *mri_lh, *mri_rh, *mri_lh_diff, *mri_rh_diff ;
  int    niter, vno, s ;
  VERTEX *v ;
  double xv, yv, zv ;
  float  val;
  MRI_SEGMENTATION *mriseg ;

  niter = SIGMA_TO_SURFACE_SMOOTH_STEPS(sigma) ;

  // do LH
  mri_lh = MRIclone(fcd->lh_thickness_on_lh, NULL) ;
  mri_rh = MRIclone(fcd->lh_thickness_on_lh, NULL) ;

  MRISwriteFrameToValues(fcd->mris_lh, fcd->lh_thickness_on_lh, 0) ;
  MRISaverageVals(fcd->mris_lh, niter) ;
  MRISreadFrameFromValues(fcd->mris_lh, mri_lh, 0) ;

  MRISwriteFrameToValues(fcd->mris_lh, fcd->rh_thickness_on_lh, 0) ;
  MRISaverageVals(fcd->mris_lh, niter) ;
  MRISreadFrameFromValues(fcd->mris_lh, mri_rh, 0) ;
  mri_lh_diff = MRIsubtract(mri_lh, mri_rh, NULL) ;  // lh minus rh on lh
  MRIfree(&mri_lh); MRIfree(&mri_rh) ;

  // do RH
  mri_lh = MRIclone(fcd->lh_thickness_on_rh, NULL) ;
  mri_rh = MRIclone(fcd->lh_thickness_on_rh, NULL) ;

  MRISwriteFrameToValues(fcd->mris_rh, fcd->lh_thickness_on_rh, 0) ;
  MRISaverageVals(fcd->mris_rh, niter) ;
  MRISreadFrameFromValues(fcd->mris_rh, mri_lh, 0) ;

  MRISwriteFrameToValues(fcd->mris_rh, fcd->rh_thickness_on_rh, 0) ;
  MRISaverageVals(fcd->mris_rh, niter) ;
  MRISreadFrameFromValues(fcd->mris_rh, mri_rh, 0) ;
  mri_rh_diff = MRIsubtract(mri_lh, mri_rh, NULL) ;  // lh minus rh on rh
  MRIfree(&mri_lh); MRIfree(&mri_rh) ;

  MRIclear(fcd->mri_thickness_increase) ;
  MRIclear(fcd->mri_thickness_decrease) ;
  for (vno = 0 ; vno < fcd->mris_lh->nvertices ; vno++)
  {
    v = &fcd->mris_lh->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    val = MRIgetVoxVal(mri_lh_diff, vno, 0, 0, 0) ;
    MRISwhiteVertexToVoxel(fcd->mris_lh, v, fcd->mri_thickness_increase, &xv, &yv, &zv) ;
    if (val >= thickness_thresh)
      MRIsetVoxVal(fcd->mri_thickness_increase, nint(xv), nint(yv), nint(zv), 0, val) ;
    else if (-val >= thickness_thresh)
      MRIsetVoxVal(fcd->mri_thickness_decrease, nint(xv), nint(yv), nint(zv), 0, val) ;
  }

  for (vno = 0 ; vno < fcd->mris_rh->nvertices ; vno++)
  {
    v = &fcd->mris_rh->vertices[vno] ;
    if (v->ripflag)
      continue ;
    if (vno == Gdiag_no)
      DiagBreak() ;
    val = MRIgetVoxVal(mri_rh_diff, vno, 0, 0, 0) ;
    MRISwhiteVertexToVoxel(fcd->mris_rh, v, fcd->mri_thickness_increase, &xv, &yv, &zv) ;
    if (val >= thickness_thresh)
      MRIsetVoxVal(fcd->mri_thickness_increase, nint(xv), nint(yv), nint(zv), 0, val) ;
    else if (-val >= thickness_thresh)
      MRIsetVoxVal(fcd->mri_thickness_decrease, nint(xv), nint(yv), nint(zv), 0, val) ;
  }

  mriseg = MRIsegment(fcd->mri_thickness_increase, thickness_thresh, 1e10) ;
  MRIremoveSmallSegments(mriseg, size_thresh) ;
  printf("segmenting volume at threshold %2.1f yields %d segments\n", thickness_thresh, mriseg->nsegments) ;

  fcd->nlabels = mriseg->nsegments ;
  for (s = 0 ; s < mriseg->nsegments ; s++)
  {
    int label ;

    fcd->labels[s] = MRIsegmentToLabel(mriseg, fcd->mri_thickness_increase, s) ;
    label = most_frequent_label(fcd->mri_aseg, &mriseg->segments[s]) ;
    sprintf(fcd->label_names[s], cma_label_to_name(label)) ;
  }
  MRIfree(&mri_lh_diff) ; MRIfree(&mri_rh_diff) ;
  MRIsegmentFree(&mriseg) ;

  return(fcd->nlabels) ;
}


