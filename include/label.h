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

int     LabelFree(LABEL **parea) ;
int     LabelDump(FILE *fp, LABEL *area) ;
LABEL   *LabelRead(char *subject_name, char *label_name) ;
int     LabelWrite(LABEL *area, char *fname) ;
int     LabelToCanonical(LABEL *area, MRI_SURFACE *mris) ;
int     LabelFromCanonical(LABEL *area, MRI_SURFACE *mris) ;
int     LabelToFlat(LABEL *area, MRI_SURFACE *mris) ;
int     LabelRipRestOfSurface(LABEL *area, MRI_SURFACE *mris) ;
int     LabelRemoveOverlap(LABEL *area1, LABEL *area2) ;
LABEL   *LabelAlloc(int max_points, char *subject_name, char *label_name) ;
int     LabelCurvFill(LABEL *area, int *vertex_list, int nvertices, 
                    int max_vertices, MRI_SURFACE *mris) ;
int     LabelTalairachTransform(LABEL *area) ;
int     LabelSphericalTransform(LABEL *area, MRI_SURFACE *mris) ;
MATRIX  *LabelCovarianceMatrix(LABEL *area, MATRIX *mat) ;
LABEL   *LabelCombine(LABEL *area, LABEL *area_dst) ;
LABEL   *LabelCopy(LABEL *asrc, LABEL *adst) ;
LABEL   *LabelCombine(LABEL *area, LABEL *adst) ;

#define LabelClone(a)  LabelAlloc(a->max_points,a->subject_name,a->name)

#endif
