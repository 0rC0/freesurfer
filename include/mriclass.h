#ifndef MRICLASS_H
#define MRICLASS_H

#include "classify.h"
#include "backprop.h"
#include "gclass.h"
#include "artmap.h"
#include "box.h"
#include "mri.h"

typedef struct
{
  int         type ;        /* what kind of classifier are we using */
  int         ninputs ;     /* # of inputs to the classifier */
  void        *parms ;      /* classifier specific parameters */
  GCLASSIFY   *gc ;         /* if type == CLASSIFIER_GAUSSIAN */
  BACKPROP    *bp ;         /* if type == CLASSIFIER_BACKPROP */
  ARTMAP      *artmap ;     /* if type == CLASSIFIER_ARTMAP */
} MRI_CLASSIFIER, MRIC ;

MRIC   *MRICalloc(int type, int ninputs, void *parms) ;
int    MRICfree(MRIC **pmri) ;
int    MRICtrain(MRIC *mric, char *file_name) ;
/*int    MRICclassify(MRIC *mric, MRI *mri_src, float *pprob) ;*/
MRIC   *MRICread(char *fname) ;
int    MRICwrite(MRIC *mric, char *fname) ;
MRI    *MRICclassify(MRIC *mric, MRI *mri_src, MRI *mri_dst, float conf, 
                     MRI *mri_probs, MRI *mri_classes) ;
int    MRICupdateMeans(MRIC *mric, MRI *mri_src, MRI *mri_target, BOX *bbox) ;
int    MRICcomputeMeans(MRIC *mric) ;  
int    MRICupdateCovariances(MRIC *mric, MRI *mri_src, MRI *mri_target, 
                             BOX *bbox) ;
int    MRICcomputeCovariances(MRIC *mric) ;
int    MRICcomputeInputs(MRI *mri,float src, int x,int y,int z,float *obs,
                         int ninputs) ;

#endif
