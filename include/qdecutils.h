// QDEC Query-Design-Estimation-Contrast
// $Id: qdecutils.h,v 1.8 2005/11/10 22:31:04 greve Exp $

#ifndef QDEC_INC
#define QDEC_INC

#include "matrix.h"

#ifdef X
#undef X
#endif

#define QDEC_NLEVELS_MAX 10

#define QDEC_FACTOR_TYPE_CONTINUOUS 1
#define QDEC_FACTOR_TYPE_DISCRETE   2

/*---------------------------------------------------------*/
typedef struct{
  char *name;
  int type;
  int nLevels;
  char LevelNames[QDEC_NLEVELS_MAX][100];
  int nInputs;
  double  *cValues; // continuous value for each input
  char   **dValues; // or discrete value for each input
} QDEC_FACTOR, QDECF;
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
typedef struct{
  int nContrasts;
  char **ContrastNames;
  char **ContrastQuestions;
  MATRIX **C;
} QDEC_CONTRAST, QDECC;
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
typedef struct{
  char *name;
  int nInputs;        // Number of subjects
  char **InputIdList; // Subject Ids
  int nFactors;
  char *measure;
  QDEC_FACTOR *Factors;
} QDEC_DESIGN, QDECD;
/*---------------------------------------------------------*/

/*---------------------------------------------------------*/
typedef struct{
  char *name;         // Design name
  char *measure;      // thickness, sulc, curv
  char *hemi;         // lh or rh
  char *df1;          // name of discrete factor 1
  char *df2;          // name of discrete factor 2
  char *cf1;          // name of continuous factor 1
  char *cf2;          // name of continuous factor 2
  double fwhm;        // approx fwhm
  int  nsmooth;       // number of smooth steps
} QDEC_DESIGN_GUI, QDECDGUI;
/*---------------------------------------------------------*/




const char *QDECsrcVersion(void);
int QDECisContinuousFactor(QDECF *F);
int QDECisDiscreteFactor(QDECF *F);
int QDECallocFactor(QDECF *F);
int QDECallocDesign(QDECD *D);
QDECD *QDECsynthDesign(int nInputs, int nFactors);
const char *QDECfactorTypeName(QDECF *F);
int QDECdumpDesign(FILE *fp, QDECD *D);
int QDECnClasses(QDECD *D);
int QDECnContinuousFactors(QDECD *D);
int QDECnDiscreteFactors(QDECD *D);
int QDECnRegressors(QDECD *D);
int QDECfsgdFile(char *fsgdf, QDECD *D);
char *QDEClevels2ClassName(QDECD *D, int *nthlevels);
char *QDECinputClassName(QDECD *D, int nthInput);
//const char *QDECcheckDesign(QDECD *D);
QDECD *QDECloadTable(char *tablebase);
char **QDECdiscreteFactorNames(QDECD *D, int *ndf);
char **QDECcontinuousFactorNames(QDECD *D, int *ncf);
int QDECfactorNumber(QDECD *D, char *FactorName);
QDECF *QDECcloneFactor(QDECF *F, QDECF *Fsrc);
QDECD *QDECextractDesign(QDECD *DD, QDECDGUI *dgui);
QDECC *QDECallocContrasts(int nContrasts);
int QDECnthContinuousFactor(QDECD *D, int nthcf);
int QDECnthDiscreteFactor(QDECD *D, int nthdf);
QDECC *QDECcontrasts(QDECD *D);
char *QDECsaveConfig(QDECD *D, QDECDGUI *dgui, char *qdecdir);

#endif
