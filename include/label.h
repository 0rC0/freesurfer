/**
 * @file  label.h
 * @brief include file for ROI utilies.
 *
 * structures, macros, constants and prototypes for the manipulation, creation and 
 * I/O for labels (lists of vertices and/or voxels)
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: fischl $
 *    $Date: 2008/04/10 15:25:26 $
 *    $Revision: 1.41 $
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


#ifndef LABEL_H
#define LABEL_H

#include "minc_volume_io.h"
#include "matrix.h"

typedef struct
{
  int           vno ;
  float         x ;
  float         y ;
  float         z ;
  unsigned char deleted ;
  float         stat ;     /* statistic (might not be used) */
}
LABEL_VERTEX, LV ;

typedef struct
{
  int    max_points ;         /* # of points allocated */
  int    n_points ;           /* # of points in area */
  char   name[100] ;          /* name of label file */
  char   subject_name[100] ;  /* name of subject */
  LV     *lv ;                /* labeled vertices */
  General_transform transform ;   /* the next two are from this struct */
  Transform         *linear_transform ;
  Transform         *inverse_linear_transform ;
  char   space[100];          /* space description of the coords */
}
LABEL ;

#include "mrisurf.h" // MRI_SURFACE, MRIS

int     LabelIsCompletelyUnassigned(LABEL *area, int *unassigned);
int     LabelFillUnassignedVertices(MRI_SURFACE *mris,
                                    LABEL *area,
                                    int coords);
int     LabelFree(LABEL **parea) ;
int     LabelDump(FILE *fp, LABEL *area) ;
LABEL   *LabelRead(char *subject_name, char *label_name) ;
int     LabelWrite(LABEL *area, char *fname) ;
int     LabelToCanonical(LABEL *area, MRI_SURFACE *mris) ;
int     LabelThreshold(LABEL *area, float thresh) ;
int     LabelMarkWithThreshold(LABEL *area, MRI_SURFACE *mris, float thresh);
int     LabelMarkSurface(LABEL *area, MRI_SURFACE *mris) ;
int     LabelToOriginal(LABEL *area, MRI_SURFACE *mris) ;
int     LabelToWhite(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFromCanonical(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFromTalairach(LABEL *area, MRI_SURFACE *mris) ;
int     LabelToFlat(LABEL *area, MRI_SURFACE *mris) ;
int     LabelRipRestOfSurface(LABEL *area, MRI_SURFACE *mris) ;
int     LabelRipRestOfSurfaceWithThreshold(LABEL *area,
    MRI_SURFACE *mris,
    float thresh) ;
int     LabelRemoveOverlap(LABEL *area1, LABEL *area2) ;
int     LabelRemoveDuplicates(LABEL *area) ;
int     LabelHasVertex(int vtxno, LABEL *lb);
LABEL   *LabelAlloc(int max_points, char *subject_name, char *label_name) ;
int     LabelRealloc(LABEL *lb, int max_points);
int     LabelCurvFill(LABEL *area, int *vertex_list, int nvertices,
                      int max_vertices, MRI_SURFACE *mris) ;
int     LabelFillMarked(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFillAnnotated(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFillAll(LABEL *area, int *vertex_list, int nvertices,
                     int max_vertices, MRI_SURFACE *mris) ;
int     LabelTalairachTransform(LABEL *area, MRI_SURFACE *mris) ;
int     LabelSphericalTransform(LABEL *area, MRI_SURFACE *mris) ;
MATRIX  *LabelCovarianceMatrix(LABEL *area, MATRIX *mat) ;
LABEL   *LabelCombine(LABEL *area, LABEL *area_dst) ;

LABEL   *LabelTranslate(LABEL *area,
                        LABEL *area_offset,
                        float dx, float dy, float dz) ;
LABEL   *LabelCopy(LABEL *asrc, LABEL *adst) ;
LABEL   *LabelCombine(LABEL *area, LABEL *adst) ;
double  LabelArea(LABEL *area, MRI_SURFACE *mris) ;
double  LabelVariance(LABEL *area, double ux, double uy, double uz) ;
int     LabelMean(LABEL *area, double *px, double *py, double *pz) ;
int     LabelMark(LABEL *area, MRI_SURFACE *mris) ;
int     LabelMarkUndeleted(LABEL *area, MRI_SURFACE *mris) ;
int     LabelMarkStats(LABEL *area, MRI_SURFACE *mris) ;
int     LabelUnmark(LABEL *area, MRI_SURFACE *mris) ;
LABEL   *LabelFromMarkedSurface(MRI_SURFACE *mris) ;
int     LabelNormalizeStats(LABEL *area, float norm) ;
LABEL   *MaskSurfLabel(LABEL *lbl,
                       MRI *SurfMask,
                       float thresh, char *masksign, int frame);

int     LabelErode(LABEL *area, MRI_SURFACE *mris, int num_times);
int     LabelDilate(LABEL *area, MRI_SURFACE *mris, int num_times);

int   LabelSetStat(LABEL *area, float stat) ;
int   LabelCopyStatsToSurface(LABEL *area, MRI_SURFACE *mris, int which) ;
LABEL *LabelFillHoles(LABEL *area_src, MRI_SURFACE *mris, int coords) ;
LABEL *LabelFillHolesWithOrig(LABEL *area_src, MRI_SURFACE *mris) ;
LABEL *LabelfromASeg(MRI *aseg, int segcode);

MATRIX *LabelFitXYZ(LABEL *label, int order);
LABEL *LabelBoundary(LABEL *label, MRIS *surf);
int VertexIsInLabel(int vtxno, LABEL *label);
LABEL *LabelInFOV(MRI_SURFACE *mris, MRI *mri, float pad) ;
int   LabelUnassign(LABEL *area) ;
LABEL *MRISlabelInvert(MRIS *surf, LABEL *label);

#include "mrishash.h"
LABEL   *LabelSphericalCombine(MRI_SURFACE *mris, LABEL *area,
                               MRIS_HASH_TABLE *mht,
                               MRI_SURFACE *mris_dst, LABEL *area_dst);

#define LabelClone(a)  LabelAlloc(a->max_points,a->subject_name,a->name)

#endif
