/* fsgdf.h - header for freesurfer group descriptor file format */

#ifndef FSGDF_INC
#define FSGDF_INC

#include "matrix.h"
#include "mri.h"

#ifdef X
#undef X
#endif

#define FSGDF_NCLASSES_MAX 100
#define FSGDF_NVARS_MAX    100
#define FSGDF_NINPUTS_MAX  100

typedef struct {
  int version;
  char title[200];
  char measname[200];
  char regsubj[200];
  char datafile[1000];
  int  nclasses;
  char classlabel[FSGDF_NCLASSES_MAX][50]; /* [class][length]*/
  char classmarker[FSGDF_NCLASSES_MAX][50];  /* [class][length]*/  
  char classcolor[FSGDF_NCLASSES_MAX][50]; /* [class][length]*/  
  int  nvariables;
  char varlabel[FSGDF_NVARS_MAX][50]; /* [class][length]*/
  char defvarlabel[50]; /* default variable */
  int  ninputs;
  char subjid[FSGDF_NINPUTS_MAX][100];
  int  subjclassno[FSGDF_NINPUTS_MAX];
  float varvals[FSGDF_NINPUTS_MAX][FSGDF_NVARS_MAX];
  MRI *data;
} GROUPDESCRIPTOR, FSGD;

FSGD *gdfAlloc(int version);
int   gdfFree(FSGD **ppgd);
FSGD *gdfRead(char *gdfname);
int   gdfPrint(FILE *fp, FSGD *gd);
FSGD *gdfRead(char *gdfname);
MATRIX *gdfMatrixDOSS(FSGD *gd, MATRIX *X);
MATRIX *gdfMatrixDODS(FSGD *gd, MATRIX *X);

int gdfGetTitle(FSGD *gd, char *title);
int gdfGetMeasurementName(FSGD *gd, char *name);
int gdfGetSubjectName(FSGD *gd, char *name);
int gdfGetDataFileName(FSGD *gd, char *filename);
int gdfGetNumClasses(FSGD *gd, int *nclasses);
int gdfGetNthClassLabel(FSGD *gd, int nclass, char *label);
int gdfGetNthClassMarker(FSGD *gd, int nclass, char *marker);
int gdfGetNthClassColor(FSGD *gd, int nclass, char *color);
int gdfGetNumVariables(FSGD *gd, int *nvariables);
int gdfGetNthVariableLabel(FSGD *gd, int nvariable, char *label);
int gdfGetDefaultVariable(FSGD *gd, char *label);
int gdfGetDefaultVariableIndex(FSGD *gd, int *nvariable);
int gdfGetNumSubjects(FSGD *gd, int *nsubjects);
int gdfGetNthSubjectID(FSGD *gd, int nsubject, char *id);
int gdfGetNthSubjectClass(FSGD *gd, int nsubject, int *class);
int gdfGetNthSubjectNthValue(FSGD *gd, int nsubject, 
			     int nvariable, float *value);
int gdfGetNthSubjectMeasurement(FSGD *gd, int nsubject, 
				int x, int y, int z, float *value);

#endif //#ifndef FSGDF_INC


