// $Id: dti.h,v 1.7 2006/11/03 05:41:12 greve Exp $

#ifndef DTI_INC
#define DTI_INC

typedef struct {
  float bValue;
  int nAcq;
  int nB0;
  int nDir;
  char *GradFile;
  MATRIX *GradDir;
  MATRIX *GradDirNorm;
  MATRIX *B;
} DTI;

const char *DTIsrcVersion(void);
int DTIparamsFromSiemensAscii(char *fname, float *bValue, 
			      int *nAcq, int *nDir, int *nB0);
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

MRI *DTItensor2ADC(MRI *tensor, MRI *mask, MRI *adc);
int DTIsortEV(float *EigVals, MATRIX *EigVecs);
int DTIfslBValFile(DTI *dti, char *bvalfname);
int DTIfslBVecFile(DTI *dti, char *bvecfname);

#endif //#ifndef FSENV_INC
