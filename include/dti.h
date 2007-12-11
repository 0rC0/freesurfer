/**
 * @file  dti.h
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: greve $
 *    $Date: 2007/12/11 08:20:05 $
 *    $Revision: 1.15 $
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


// $Id: dti.h,v 1.15 2007/12/11 08:20:05 greve Exp $

#ifndef DTI_INC
#define DTI_INC

#include "mri.h"

typedef struct
{
  MATRIX *bValue;
  MATRIX *GradDir;
  MATRIX *GradDirNorm;
  MATRIX *B;
  // These strictly relate to Siemens/MGH DTI
  int nB0;
  int nDir;
  char *GradFile;
}
DTI;

const char *DTIsrcVersion(void);
int DTIfree(DTI **pdti);
int DTIparamsFromSiemensAscii(char *fname, float *bValue,int *nDir, int *nB0);
int DTIloadGradients(DTI *dti, char *GradFile);
DTI *DTIstructFromSiemensAscii(char *fname);
int DTInormGradDir(DTI *dti);
int DTIdesignMatrix(DTI *dti);
MRI *DTIbeta2Tensor(MRI *beta, MRI *mask, MRI *tensor);
MRI *DTIbeta2LowB(MRI *beta, MRI *mask, MRI *lowb);
MRI *DTIsynthDWI(MATRIX *X, MRI *beta, MRI *mask, MRI *synth);

int DTItensor2Eig(MRI *tensor, MRI *mask,   MRI **evals,
                  MRI **evec1, MRI **evec2, MRI **evec3);
MRI *DTIeigvals2FA(MRI *evals, MRI *mask, MRI *FA);
MRI *DTIeigvals2RA(MRI *evals, MRI *mask, MRI *RA);
MRI *DTIeigvals2VR(MRI *evals, MRI *mask, MRI *VR);
MRI *DTIradialDiffusivity(MRI *evals, MRI *mask, MRI *RD);

MRI *DTItensor2ADC(MRI *tensor, MRI *mask, MRI *adc);
int DTIsortEV(float *EigVals, MATRIX *EigVecs);
int DTIfslBValFile(DTI *dti, char *bvalfname);
int DTIfslBVecFile(DTI *dti, char *bvecfname);
MRI *DTIivc(MRI *evec, MRI *mask, MRI *ivc);
MATRIX *DTIloadBValues(char *bvalfile);
MATRIX *DTIloadBVectors(char *bvecfile);
int DTIwriteBVectors(MATRIX *bvecs,char *bvecfile);
int DTIwriteBValues(MATRIX *bvals, char *bvalfile);
DTI *DTIstructFromBFiles(char *bvalfile, char *bvecfile);
int DTIparsePulseSeqName(char *pulseseq, double *bValue, int *nthDirection);
int DTIisFSLBVec(char *fname);

#endif //#ifndef FSENV_INC
