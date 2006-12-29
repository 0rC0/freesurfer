/**
 * @file  kinput.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:08:59 $
 *    $Revision: 1.3 $
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


#ifndef KINPUT_H
#define KINPUT_H

#include "image.h"


#define IN_SIZE             3
#define NHALF_IN            ((IN_SIZE-1)/2)
#define HALF_IN             (IN_SIZE/2)
#define NPYR                2
#define TROW                15
#define TCOL                15
#define MAX_SCALES          8
#define SIGMA_SCALE_FACTOR  1.5f

#define USE_PYRAMID  0

/* this structure will be used to write data into a .bp file */
typedef struct
{
  int     nscales ;
  int     input_size ;
  float   sigmas[MAX_SCALES] ;
  char    pc_fname[75] ;
  char    coef_fname[75] ;
  int     abs_gradient ;
  float   sigma_scale_factor ;
}
KINPUT_PARMS ;

typedef struct
{
#if USE_PYRAMID
  HIPSPyramid  *xpyr ;
  HIPSPyramid  *ypyr ;
  IMAGE    *xImage ;
  IMAGE    *yImage ;
#else
  IMAGE    *xInputs ;   /* sequence of nscales images */
  IMAGE    *yInputs ;
  IMAGE    *gImages[MAX_SCALES] ;
#endif
  float        *inputs ;
  int          nscales ;
  int          ninputs ;
  KINPUT_PARMS parms ;
}
KINPUT ;


KINPUT *KinputAlloc(int rows, int cols, int scales, int input_size,
                    float sigma, float sigma_scale_factor, int abs_gradient) ;
int    KinputFree(KINPUT **pkinput) ;
int    KinputInit(KINPUT *kinput, IMAGE *image) ;
int    KinputVector(KINPUT *kinput, int x, int y) ;


#define SIGMA   0.5f

#endif
