#ifndef LABEL_H
#define LABEL_H

#include "volume_io.h"

typedef struct
{
  int           vno ;
  float         x ;
  float         y ;
  float         z ;
  unsigned char deleted ;
  float         stat ;     /* statistic (might not be used) */
} LABEL_VERTEX, LV ;

typedef struct
{
  int    max_points ;         /* # of points allocated */
  int    n_points ;           /* # of points in area */
  char   name[100] ;          /* name of label file */
  char   subject_name[100] ;  /* name of subject */
  LV     *lv ;                /* labelled vertices */
  General_transform transform ;   /* the next two are from this struct */
  Transform         *linear_transform ;
  Transform         *inverse_linear_transform ;
} LABEL ;


#include "mrisurf.h"

int     LabelFillUnassignedVertices(MRI_SURFACE *mris, LABEL *area);
int     LabelFree(LABEL **parea) ;
int     LabelDump(FILE *fp, LABEL *area) ;
LABEL   *LabelRead(char *subject_name, char *label_name) ;
int     LabelWrite(LABEL *area, char *fname) ;
int     LabelToCanonical(LABEL *area, MRI_SURFACE *mris) ;
int     LabelMarkSurface(LABEL *area, MRI_SURFACE *mris) ;
int     LabelToOriginal(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFromCanonical(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFromTalairach(LABEL *area, MRI_SURFACE *mris) ;
int     LabelToFlat(LABEL *area, MRI_SURFACE *mris) ;
int     LabelRipRestOfSurface(LABEL *area, MRI_SURFACE *mris) ;
int     LabelRemoveOverlap(LABEL *area1, LABEL *area2) ;
int     LabelRemoveDuplicates(LABEL *area) ;
LABEL   *LabelAlloc(int max_points, char *subject_name, char *label_name) ;
int     LabelCurvFill(LABEL *area, int *vertex_list, int nvertices, 
                    int max_vertices, MRI_SURFACE *mris) ;
int     LabelFillMarked(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFillAll(LABEL *area, int *vertex_list, int nvertices, 
                    int max_vertices, MRI_SURFACE *mris) ;
int     LabelTalairachTransform(LABEL *area, MRI_SURFACE *mris) ;
int     LabelSphericalTransform(LABEL *area, MRI_SURFACE *mris) ;
MATRIX  *LabelCovarianceMatrix(LABEL *area, MATRIX *mat) ;
LABEL   *LabelCombine(LABEL *area, LABEL *area_dst) ;
LABEL   *LabelCopy(LABEL *asrc, LABEL *adst) ;
LABEL   *LabelCombine(LABEL *area, LABEL *adst) ;
double  LabelArea(LABEL *area, MRI_SURFACE *mris) ;
double  LabelVariance(LABEL *area, double ux, double uy, double uz) ;
int     LabelMean(LABEL *area, double *px, double *py, double *pz) ;
int     LabelMark(LABEL *area, MRI_SURFACE *mris) ;
LABEL   *LabelFromMarkedSurfaces(MRI_SURFACE *mris) ;
int     LabelUnmark(LABEL *area, MRI_SURFACE *mris) ;
LABEL   *LabelFromMarkedSurface(MRI_SURFACE *mris) ;

#define LabelClone(a)  LabelAlloc(a->max_points,a->subject_name,a->name)

#endif
