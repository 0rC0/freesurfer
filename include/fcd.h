#ifndef FCD_H
#define FCD_H

#include "mri.h"
#include "mrisurf.h"
#include "label.h"
#include "utils.h"
#include "label.h"
#define MAX_FCD_LABELS 100

typedef struct
{
  MRI_SURFACE *mris_lh ;
  MRI_SURFACE *mris_rh ;
  MRI         *mri_aseg ;
  MRI         *mri_norm ;
  MRI         *mri_flair ;
  MRI         *mri_thickness_increase ;
  MRI         *mri_thickness_decrease ;
  MRI         *lh_thickness_on_lh ;  // thickness of left hemi mapped to left hemi
  MRI         *lh_thickness_on_rh ;  // thickness of left hemi mapped to right hemi
  MRI         *rh_thickness_on_lh ;  // thickness of right hemi mapped to right hemi
  MRI         *rh_thickness_on_rh ;  // thickness of right hemi mapped to right hemi
  double      thickness_threshold ;
  int         thickness_smooth_steps ;
  int         nlabels ;
  LABEL       *labels[MAX_FCD_LABELS] ;
  char        label_names[MAX_FCD_LABELS][STRLEN] ;
} FCD_DATA ;


FCD_DATA   *FCDloadData(char *sdir, char *subject);
int         FCDcomputeThicknessLabels(FCD_DATA *fcd, double thickness_thresh, double sigma, int size_thresh) ;
int         FCDfree(FCD_DATA **pfcd) ;

#endif
