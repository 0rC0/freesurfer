#ifndef MRICLASS_H
#define MRICLASS_H

#include "classify.h"
#include "backprop.h"
#include "gclass.h"
#include "artmap.h"
#include "mri.h"

#define GAUSSIAN_NCLASSES      4
#define BACKGROUND             0
#define GREY_MATTER            1
#define WHITE_MATTER           2
#define BRIGHT_MATTER          3
#define LO_LIM                 70
#define HI_LIM                 150
#define DEFINITELY_BACKGROUND  50

#define MAX_INPUTS             10

/* bitfield of feature types */
#define FEATURE_INTENSITY      0x00001
#define FEATURE_ZSCORE3        0x00002
#define FEATURE_ZSCORE5        0x00004
#define FEATURE_DIRECTION      0x00010
#define FEATURE_MEAN3          0x00020
#define FEATURE_MEAN5          0x00040
#define FEATURE_CPOLV_MEAN3    0x00080
#define FEATURE_CPOLV_MEAN5    0x00100
#define FEATURE_CPOLV_MEDIAN3  0x00200
#define FEATURE_CPOLV_MEDIAN5  0x00400
#define FEATURE_CPOLV          (FEATURE_CPOLV_MEAN3 | FEATURE_CPOLV_MEAN5 | \
                                FEATURE_CPOLV_MEDIAN3 | FEATURE_CPOLV_MEDIAN5)
#define MAX_FEATURE            FEATURE_CPOLV_MEDIAN5

#define MAX_ROUNDS             5

typedef union
{
  BACKPROP   *bp ;
  ARTMAP     *artmap ;
  GCLASSIFY  *gc ;
} CL_UNION ;

typedef struct
{
  int         features[MAX_ROUNDS] ;  /* bit field of above features */
  int         type[MAX_ROUNDS] ;      /* what kind of classifier are we using */
  int         ninputs[MAX_ROUNDS] ;   /* # of inputs to the classifier */
  int         nrounds ;               /* # of times to apply classifier */
  void        *parms ;                /* classifier specific parameters */
  CL_UNION    classifier[MAX_ROUNDS] ;/* pointer to appropriate classifier */
  MRI         *mri_priors ;           /* prior probabilities */
  char        prior_fname[100] ;
} MRI_CLASSIFIER, MRIC ;

MRIC   *MRICalloc(int nrounds, int types[], int features[], void *parms) ;
int    MRICfree(MRIC **pmri) ;
int    MRICtrain(MRIC *mric, char *file_name, char *prior_fname) ;
MRIC   *MRICread(char *fname) ;
int    MRICwrite(MRIC *mric, char *fname) ;
MRI    *MRICclassify(MRIC *mric, MRI *mri_src, 
                     MRI *mri_dst, float conf,MRI *mri_probs,MRI *mri_classes);
int    MRICcomputeInputs(MRI *mri, int x,int y,int z,float *obs, int features);
MRI    *MRICbuildTargetImage(MRI *mri_src, MRI *mri_target, MRI *mri_wm,
                             int lo_lim, int hi_lim) ;
MRI    *MRICupdatePriors(MRI *mri_target, MRI *mri_priors, int scale) ;
int    MRInormalizePriors(MRI *mri_priors) ;
int    MRICupdateStatistics(MRIC *mric, int round, MRI *mri_src, 
                            MRI *mri_target, MRI_REGION *box) ;
int    MRICcomputeStatistics(MRIC *mric, int round) ;
char   *MRICclassName(MRIC *mric, int round, int classno) ;

#endif
