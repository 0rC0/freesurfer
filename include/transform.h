#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "matrix.h"

typedef struct
{
  float      x0 ;            /* center of transform */
  float      y0 ;
  float      z0 ;
  float      sigma ;         /* spread of transform */
  MATRIX     *m_L ;          /* transform matrix */
  MATRIX     *m_dL ;         /* gradient of fuctional wrt transform matrix */
  MATRIX     *m_last_dL ;    /* last time step for momentum */
} LINEAR_TRANSFORM, LT ;

typedef struct
{
  int               num_xforms ;      /* number linear transforms */
  LINEAR_TRANSFORM  *xforms ;         /* transforms */
  LINEAR_TRANSFORM  *inv_xforms ;     /* inverse transforms */
  int               type ;
} LINEAR_TRANSFORM_ARRAY, LTA ;

#include "mri.h"
typedef struct
{
  int        type ;
  MRI        *mri_xn ;
  MRI        *mri_yn ;
  MRI        *mri_zn ;
  void       *xform ;
} TRANSFORM ;

int      LTAfree(LTA **plta) ;
LTA      *LTAreadInVoxelCoords(char *fname, MRI *mri) ;
LTA      *LTAread(char *fname) ;
LTA      *LTAreadTal(char *fname) ;
int      LTAwrite(LTA *lta, char *fname) ;
LTA      *LTAalloc(int nxforms, MRI *mri) ;
int      LTAdivide(LTA *lta, MRI *mri) ;
MRI      *LTAtransform(MRI *mri_src, MRI *mri_dst, LTA *lta) ;
MATRIX   *LTAtransformAtPoint(LTA *lta, float x, float y,float z,MATRIX *m_L);
int      LTAworldToWorld(LTA *lta, float x, float y, float z,
                         float *px, float *py, float *pz);
int      LTAinverseWorldToWorld(LTA *lta, float x, float y, float z,
                         float *px, float *py, float *pz);
VECTOR   *LTAtransformPoint(LTA *lta, VECTOR *v_X, VECTOR *v_Y) ;
VECTOR   *LTAinverseTransformPoint(LTA *lta, VECTOR *v_X, VECTOR *v_Y) ;
double   LTAtransformPointAndGetWtotal(LTA *lta, VECTOR *v_X, VECTOR *v_Y) ;
MATRIX   *LTAinverseTransformAtPoint(LTA *lta, float x, float y, float z, 
                                     MATRIX *m_L) ;
MATRIX   *LTAworldTransformAtPoint(LTA *lta, float x, float y,float z,
                                   MATRIX *m_L);
int      LTAtoVoxelCoords(LTA *lta, MRI *mri) ;

#define LINEAR_VOX_TO_VOX       0
#define LINEAR_VOXEL_TO_VOXEL   LINEAR_VOX_TO_VOX
#define LINEAR_RAS_TO_RAS       1
#define TRANSFORM_ARRAY_TYPE    10
#define MORPH_3D_TYPE           11
#define MNI_TRANSFORM_TYPE      12
#define MATLAB_ASCII_TYPE       13

int      TransformFileNameType(char *fname) ;
int      LTAvoxelToRasXform(LTA *lta, MRI *mri_src, MRI *mri_dst) ;
int      LTAvoxelToRasXform(LTA *lta, MRI *mri_src, MRI *mri_dst) ;
int      LTAvoxelTransformToCoronalRasTransform(LTA *lta) ;

int FixMNITal(float  xmni, float  ymni, float  zmni,
        float *xtal, float *ytal, float *ztal);


TRANSFORM *TransformRead(char *fname) ;
int       TransformFree(TRANSFORM **ptrans) ;
int       TransformSample(TRANSFORM *transform, int xv, int yv, int zv, float *px, float *py, float *pz) ;
int       TransformSampleInverse(TRANSFORM *transform, int xv, int yv, int zv, 
                                 float *px, float *py, float *pz) ;
int       TransformSampleInverseVoxel(TRANSFORM *transform, int width, int height, int depth,
                                      int xv, int yv, int zv, 
                                      int *px, int *py, int *pz) ;
TRANSFORM *TransformAlloc(int type, MRI *mri) ;
int       TransformInvert(TRANSFORM *transform, MRI *mri) ;
MRI       *TransformApply(TRANSFORM *transform, MRI *mri_src, MRI *mri_dst) ;
MRI       *TransformApplyInverse(TRANSFORM *transform, MRI *mri_src, MRI *mri_dst) ;

#endif
