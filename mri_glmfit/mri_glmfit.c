// mri_glmfit.c

// Save config in output dir
// Links to source data
// Cleanup

// p-to-z
// Rewrite MatrixReadTxt to ignore # and % and empty lines
// Auto-det/read matlab4 matrices
// Profiling

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
double round(double x);
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "macros.h"
#include "utils.h"
#include "mrisurf.h"
#include "mrisutils.h"
#include "error.h"
#include "diag.h"
#include "mri.h"
#include "mri2.h"
#include "fio.h"
#include "version.h"
#include "label.h"
#include "matrix.h"
#include "annotation.h"
#include "fmriutils.h"
#include "cmdargs.h"
#include "fsglm.h"
#include "gsl/gsl_cdf.h"

/*---------------------------------------------------------*/
typedef struct{
  MRI *y;            // Input data
  MATRIX *Xg;        // Global regressor matrix
  int npvr;          // Number of per-voxel regressors
  MRI *pvr[50];      // Per-voxel regressors (local)
  float DOF;         // DOF
  int Xcols;         // X->cols + npvr
  MRI *w;            // Per-voxel, per-input weight
  MRI *mask;         // Only proc within mask
  int n_ill_cond;    // Number of ill-conditioned voxels

  MRI *iXtX;         // inv(X'X), where X is global and pv (needed?)
  MRI *beta;         // beta = inv(X'X)*X'*y
  MRI *yhat;         // yhat = X*beta
  int yhatsave;      // Flag to save yhat
  MRI *eres;         // eres = y - yhat
  MRI *rvar;         // rvar = sum(eres.^2)/DOF;

  int ncontrasts;    // Number of contrasts
  char *cname[100];  // Contrast names
  MATRIX *C[100];    // Contrast matrices
  MATRIX *Ct[100];   // transposes Contrast matrices
  MRI *gamma[100];   // gamma = C*beta
  MRI *F[100];       // F = gamma'*iXtX*gamma/(rvar*J)
  MRI *sig[100];     // sig = significance of the F
} GLMPV;
/*---------------------------------------------------------*/

int MRIglmpvFit(GLMPV *glmpv);
MATRIX *MRItoMatrix(MRI *mri, int c, int r, int s, 
		    int Mrows, int Mcols, MATRIX *M);
MATRIX *MRItoSymMatrix(MRI *mri, int c, int r, int s, MATRIX *M);
int MRIfromMatrix(MRI *mri, int c, int r, int s, MATRIX *M);
int MRIfromSymMatrix(MRI *mri, int c, int r, int s, MATRIX *M);
MRI *MRInormWeights(MRI *w, int sqrtFlag, int invFlag, MRI *mask, MRI *wn);

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_glmfit.c,v 1.12 2005/09/18 05:11:48 greve Exp $";
char *Progname = NULL;

char *yFile = NULL, *XFile=NULL, *betaFile=NULL, *rvarFile=NULL;
char *yhatFile=NULL, *eresFile=NULL, *wFile=NULL, *maskFile;
char *GLMDir=NULL;
char *pvrFiles[50];
int synth = 0;
int yhatSave=0;

MRI *mritmp=NULL, *sig=NULL;

int debug = 0, checkoptsonly = 0;
char tmpstr[2000];

MATRIX *H=NULL, *Xt=NULL, *XtX=NULL, *iXtX=NULL, *Q=NULL, *R=NULL;
MATRIX *M;
int nContrasts=0;
char *CFile[100];
int err,c,r,s;

int npvr=0;
GLMPV *glmpv;

/*--------------------------------------------------*/
int main(int argc, char **argv)
{
  int nargs,n;
  struct utsname uts;
  char *cmdline, cwd[2000];

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, vcid, "$Name:  $");
  if (nargs && argc - nargs == 1) exit (0);
  argc -= nargs;
  cmdline = argv2cmdline(argc,argv);
  uname(&uts);
  getcwd(cwd,2000);

  Progname = argv[0] ;
  argc --;
  argv++;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  if(argc == 0) usage_exit();

  parse_commandline(argc, argv);
  check_options();
  if(checkoptsonly) return(0);

  printf("\n");
  printf("%s\n",vcid);
  printf("cwd %s\n",cwd);
  printf("cmdline %s\n",cmdline);
  printf("sysname  %s\n",uts.sysname);
  printf("hostname %s\n",uts.nodename);
  printf("machine  %s\n",uts.machine);
  dump_options(stdout);

  if(GLMDir != NULL){
    printf("Creating output directory %s\n",GLMDir);
    err = mkdir(GLMDir,(mode_t)-1);
    if(err != 0 && errno != EEXIST){
      printf("ERROR: creating directory %s\n",GLMDir);
      perror(NULL);    
      return(1);
    }
  }

  glmpv = (GLMPV *) calloc(sizeof(GLMPV),1);
  glmpv->npvr = npvr;
  glmpv->ncontrasts = nContrasts;
  if(yhatFile != NULL) glmpv->yhatsave = 1;
  else                 glmpv->yhatsave = 0;

  //-----------------------------------------------------
  glmpv->Xg = MatrixReadTxt(XFile, NULL);
  if(glmpv->Xg==NULL){
    printf("ERROR: loading X %s\n",XFile);
    exit(1);
  }

  // Total number of columns in X, including PVRs
  glmpv->Xcols = glmpv->Xg->cols + npvr;

  // Compute and check DOF -------------------------------------
  glmpv->DOF = glmpv->Xg->rows - glmpv->Xcols;
  if(glmpv->DOF < 1){
    printf("ERROR: DOF = %g\n",glmpv->DOF);
    exit(1);
  }

  // Load the contrasts -----------------------------------------------
  if(nContrasts > 0){
    for(n=0; n < nContrasts; n++){
      glmpv->C[n] = MatrixReadTxt(CFile[n], NULL);
      if(glmpv->C[n] == NULL){
	printf("ERROR: loading C %s\n",CFile[n]);
	exit(1);
      }
      if(glmpv->C[n]->cols != glmpv->Xcols){
	printf("ERROR: dimension mismatch between X and contrast %s",CFile[n]);
	printf("       X has %d cols, C has %d cols\n",
	       glmpv->Xcols,glmpv->C[n]->cols);
	exit(1);
      }
      glmpv->cname[n] = fio_basename(CFile[n],".mat");
    }
  }

  // Load/synth input  -----------------------------------------------
  if(synth == 0){
    printf("Loading y from %s\n",yFile);
    glmpv->y = MRIread(yFile);
    if(glmpv->y == NULL){
      printf("ERROR: loading y %s\n",yFile);
      exit(1);
    }
  }
  else{
    mritmp = MRIreadHeader(yFile,MRI_VOLUME_TYPE_UNKNOWN);
    if(mritmp == NULL){
      printf("ERROR: reading header for y %s\n",yFile);
      exit(1);
    }
    printf("Synthesizing y with white noise\n");
    glmpv->y =  MRIrandn(mritmp->width, mritmp->depth, mritmp->height, 
			 mritmp->nframes,0,1,NULL);
    MRIfree(&mritmp);
  }

  // Check some dimensions -----------------------------------
  if(glmpv->y->nframes != glmpv->Xg->rows){
    printf("ERROR: dimension mismatch between y and X.\n");
    printf("  y has %d inputs, X has %d rows.\n",
	   glmpv->y->nframes,glmpv->Xg->rows);
    exit(1);
  }

  // Load the mask file ----------------------------------
  if(maskFile != NULL){
    glmpv->mask = MRIread(maskFile);
    if(glmpv->mask  == NULL){
      printf("ERROR: reading mask file %s\n",maskFile);
      exit(1);
    }
  }
  else glmpv->mask = NULL;

  // Load the weight file ----------------------------------
  if(wFile != NULL){
    glmpv->w = MRIread(wFile);
    if(glmpv->w  == NULL){
      printf("ERROR: reading weight file %s\n",wFile);
      exit(1);
    }
    MRInormWeights(glmpv->w, 1, 1, glmpv->mask, glmpv->w);
    MRIwrite(glmpv->w,"wn.mgh");
  }
  else glmpv->w = NULL;

  // Load PVRs -----------------------------------
  if(glmpv->npvr > 0){
    for(n=0; n < glmpv->npvr; n++){
      glmpv->pvr[n] = MRIread(pvrFiles[n]);
      if(glmpv->pvr[n] == NULL) exit(1);
    }
  }

  // PVR Analysis  -----------------------------------
  if(glmpv->npvr > 0 || wFile != NULL){
    printf("Starting PVR Fit\n");
    MRIglmpvFit(glmpv);
    MRIwrite(glmpv->beta,betaFile);
    MRIwrite(glmpv->rvar,rvarFile);
    if(glmpv->yhatsave) MRIwrite(glmpv->yhat,yhatFile);
    if(eresFile) MRIwrite(glmpv->eres,eresFile);    

    for(n=0; n < glmpv->ncontrasts; n++){

      // Create output directory for contrast
      sprintf(tmpstr,"%s/%s",GLMDir,glmpv->cname[n]);
      mkdir(tmpstr,(mode_t)-1);
      printf("%s\n",glmpv->cname[n]);

      // Save gamma and F
      sprintf(tmpstr,"%s/%s/gamma.mgh",GLMDir,glmpv->cname[n]);
      MRIwrite(glmpv->gamma[n],tmpstr);
      sprintf(tmpstr,"%s/%s/F.mgh",GLMDir,glmpv->cname[n]);
      MRIwrite(glmpv->F[n],tmpstr);

      // Compute and Save p-values
      sig = fMRIsigF(glmpv->F[n], glmpv->DOF, glmpv->C[n]->rows, NULL);
      sprintf(tmpstr,"%s/%s/p.mgh",GLMDir,glmpv->cname[n]);
      MRIwrite(sig,tmpstr);

      // Compute and Save -log10 p-values
      MRIlog10(sig,sig,1);
      sprintf(tmpstr,"%s/%s/sig.mgh",GLMDir,glmpv->cname[n]);
      MRIwrite(sig,tmpstr);

      MRIfree(&sig);
    }

    printf("mri_glmfit done\n");
    return(0); exit(0);

  }

  // Only gets here if there are no PVRs -----------------------------------
  Xt = MatrixTranspose(glmpv->Xg,NULL);
  XtX = MatrixMultiply(Xt,glmpv->Xg,NULL);
  iXtX = MatrixInverse(XtX,NULL);
  if(iXtX==NULL){
    printf("ERROR: could not compute psuedo inverse of X\n");
    exit(1);
  }
  /* Q is the matrix that when multiplied by y gives beta */
  Q = MatrixMultiply(iXtX,Xt,NULL);
  /* H is the matrix that when multiplied by y gives the signal estimate */
  H = MatrixMultiply(glmpv->Xg,Q,NULL);
  /* R is the matrix that when multiplied by y gives the residual error */
  R = MatrixSubtract(MatrixIdentity(glmpv->y->nframes,NULL),H,NULL);

  //-----------------------------------------------------
  printf("Computing beta\n");
  glmpv->beta = fMRImatrixMultiply(glmpv->y, Q, NULL);
  MRIwrite(glmpv->beta,betaFile);

  printf("Computing residuals\n");
  glmpv->eres = fMRImatrixMultiply(glmpv->y, R, NULL);
  if(eresFile) MRIwrite(glmpv->eres,eresFile);

  printf("Computing residual variance\n");
  glmpv->rvar = fMRIvariance(glmpv->eres,glmpv->DOF,0,NULL);
  MRIwrite(glmpv->rvar,rvarFile);

  if(glmpv->yhatsave){ 
    printf("Computing signal estimate\n");
    glmpv->yhat = fMRImatrixMultiply(glmpv->y, H, NULL);
    MRIwrite(glmpv->yhat,yhatFile);
  }

  if(nContrasts == 0){
    printf("mri_glmfit done\n");
    return(0);
  }

  //---------------------------------------------------
  for(n=0; n < nContrasts; n++){

    printf("%s\n",glmpv->cname[n]);
    sprintf(tmpstr,"%s/%s",GLMDir,glmpv->cname[n]);
    mkdir(tmpstr,(mode_t)-1);

    glmpv->gamma[n] = fMRImatrixMultiply(glmpv->beta,glmpv->C[n],NULL);
    sprintf(tmpstr,"%s/%s/gamma.mgh",GLMDir,glmpv->cname[n]);
    MRIwrite(glmpv->gamma[n],tmpstr);

    glmpv->F[n] = fMRIcomputeF(glmpv->gamma[n], glmpv->Xg, glmpv->C[n], 
			       glmpv->rvar, NULL);
    sprintf(tmpstr,"%s/%s/F.mgh",GLMDir,glmpv->cname[n]);
    MRIwrite(glmpv->F[n],tmpstr);

    sig = fMRIsigF(glmpv->F[n], glmpv->DOF, glmpv->C[n]->rows, NULL);
    MRIlog10(sig,sig,1);
    sprintf(tmpstr,"%s/%s/sig.mgh",GLMDir,glmpv->cname[n]);
    MRIwrite(sig,tmpstr);

    MRIfree(&sig);
  }

  printf("mri_glmfit done\n");
  return(0);
}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv)
{
  int  nargc , nargsused, msec, niters;
  char **pargv, *option ;

  if(argc < 1) usage_exit();

  nargc   = argc;
  pargv = argv;
  while(nargc > 0){

    option = pargv[0];
    if(debug) printf("%d %s\n",nargc,option);
    nargc -= 1;
    pargv += 1;

    nargsused = 0;

    if (!strcasecmp(option, "--help"))  print_help() ;
    else if (!strcasecmp(option, "--version")) print_version() ;
    else if (!strcasecmp(option, "--debug"))   debug = 1;
    else if (!strcasecmp(option, "--checkopts"))   checkoptsonly = 1;
    else if (!strcasecmp(option, "--nocheckopts")) checkoptsonly = 0;
    else if (!strcasecmp(option, "--synth"))   synth = 1;
    else if (!strcasecmp(option, "--yhatsave")) yhatSave = 1;

    else if (!strcasecmp(option, "--profile")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&niters);
      printf("Starting GLM profile over %d iterations\n",niters);
      msec = GLMprofile(200, 20, 5, niters);
      nargsused = 1;
      exit(0);
    }
    else if (!strcmp(option, "--y")){
      if(nargc < 1) CMDargNErr(option,1);
      yFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--mask")){
      if(nargc < 1) CMDargNErr(option,1);
      maskFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--w")){
      if(nargc < 1) CMDargNErr(option,1);
      wFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--X")){
      if(nargc < 1) CMDargNErr(option,1);
      XFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--pvr")){
      if(nargc < 1) CMDargNErr(option,1);
      pvrFiles[npvr] = pargv[0];
      npvr++;
      nargsused = 1;
    }
    else if (!strcmp(option, "--glmdir")){
      if(nargc < 1) CMDargNErr(option,1);
      GLMDir = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--beta")){
      if(nargc < 1) CMDargNErr(option,1);
      betaFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--rvar")){
      if(nargc < 1) CMDargNErr(option,1);
      rvarFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--yhat")){
      if(nargc < 1) CMDargNErr(option,1);
      yhatFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--eres")){
      if(nargc < 1) CMDargNErr(option,1);
      eresFile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--C")){
      if(nargc < 1) CMDargNErr(option,1);
      CFile[nContrasts] = pargv[0];
      nContrasts++;
      nargsused = 1;
    }
    else{
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if(CMDsingleDash(option))
	fprintf(stderr,"       Did you really mean -%s ?\n",option);
      exit(-1);
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/* ------------------------------------------------------ */
static void usage_exit(void)
{
  print_usage() ;
  exit(1) ;
}
/* --------------------------------------------- */
static void print_usage(void)
{
  printf("USAGE: %s \n",Progname) ;
  printf("\n");
  printf("   --y input volume \n");
  printf("   --X design matrix file\n");
  printf("   --glmdir dir : save outputs to dir\n");
  printf("   --C contrast1.mat <--C contrast2.mat ...>\n");
  printf("\n");
  printf("   --beta regression coeffient volume\n");
  printf("   --rvar residual variance\n");
  printf("   --yhat input estimate\n");
  printf("   --eres residual\n");
  printf("\n");
  printf("   --synth : replace input with gaussian \n");
  printf("\n");
  printf("   --debug     turn on debugging\n");
  printf("   --checkopts don't run anything, just check options and exit\n");
  printf("   --help      print out information on how to use this program\n");
  printf("   --version   print out version and exit\n");
  printf("\n");
  printf("%s\n", vcid) ;
  printf("\n");
}
/* --------------------------------------------- */
static void print_help(void)
{
  print_usage() ;
  printf("WARNING: this program is not yet tested!\n");
  exit(1) ;
}
/* --------------------------------------------- */
static void print_version(void)
{
  printf("%s\n", vcid) ;
  exit(1) ;
}
/* --------------------------------------------- */
static void check_options(void)
{
  if(yFile == NULL){
    printf("ERROR: must specify input y file\n");
    exit(1);
  }
  if(XFile == NULL){
    printf("ERROR: must specify an input X file\n");
    exit(1);
  }
  if(GLMDir != NULL){
    sprintf(tmpstr,"%s/beta.mgh",GLMDir);
    betaFile = strcpyalloc(tmpstr);
    sprintf(tmpstr,"%s/rvar.mgh",GLMDir);
    rvarFile = strcpyalloc(tmpstr);
    sprintf(tmpstr,"%s/eres.mgh",GLMDir);
    eresFile = strcpyalloc(tmpstr);
    if(yhatSave){
      sprintf(tmpstr,"%s/yhat.mgh",GLMDir);
      yhatFile = strcpyalloc(tmpstr);
    }
  }
  else{
    if(betaFile == NULL){
      printf("ERROR: must specify an output dir or beta file\n");
      exit(1);
    }
    if(rvarFile == NULL){
      printf("ERROR: must specify an output dir or rvar file\n");
      exit(1);
    }
  }

  return;
}

/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  fprintf(fp,"y    %s\n",yFile);
  fprintf(fp,"X    %s\n",XFile);
  fprintf(fp,"beta %s\n",betaFile);
  fprintf(fp,"rvar %s\n",rvarFile);
  if(yhatFile) fprintf(fp,"yhat %s\n",yhatFile);
  if(eresFile) fprintf(fp,"eres %s\n",eresFile);

  return;
}
/*---------------------------------------------------------------*/
MATRIX *MRItoMatrix(MRI *mri, int c, int r, int s, 
		    int Mrows, int Mcols, MATRIX *M)
{
  int mr, mc, f;

  if(M==NULL) M = MatrixAlloc(Mrows,Mcols,MATRIX_REAL);
  else{
    if(M->rows != Mrows || M->cols != Mcols){
      printf("ERROR: Matrix dim mismatch\n");
    }
  }

  if(mri->nframes != Mrows*Mcols){
    printf("ERROR: MRItoMatrix: MRI frames = %d, does not equal\n",
	   mri->nframes);
    printf("       matrix dim = %dx%d = %d",Mrows,Mcols,Mrows*Mcols);
    return(NULL);
  }

  f = 0;
  for(mr=1; mr <= Mrows; mr++){
    for(mc=1; mc <= Mcols; mc++){
      M->rptr[mr][mc] = MRIgetVoxVal(mri,c,r,s,f);
      f++;
    }
  }
  return(M);
}

/*---------------------------------------------------------------*/
MATRIX *MRItoSymMatrix(MRI *mri, int c, int r, int s, MATRIX *M)
{
  int mr, mc, f, Msize, nframesexp;

  if(M==NULL){
    Msize = (int)(round( (sqrt(8.0*mri->nframes + 1.0) - 1.0 )/2.0 ));
    printf("Msize = %d\n",Msize);
    M = MatrixAlloc(Msize,Msize,MATRIX_REAL);
  }

  nframesexp = M->rows*(M->rows+1)/2;
  if(mri->nframes != nframesexp){
    printf("ERROR: MRItoSymMatrix: MRI frames = %d, does not support sym\n",
	   mri->nframes);
    return(NULL);
  }

  f = 0;
  for(mr=1; mr <= M->rows; mr++){
    for(mc=mr; mc <= M->cols; mc++){
      M->rptr[mr][mc] = MRIgetVoxVal(mri,c,r,s,f);
      M->rptr[mc][mr] = MRIgetVoxVal(mri,c,r,s,f);
      f++;
    }
  }
  return(M);
}

/*---------------------------------------------------------------*/
int MRIfromMatrix(MRI *mri, int c, int r, int s, MATRIX *M)
{
  int mr,mc,f;

  if(mri->nframes != M->rows*M->cols){
    printf("ERROR: MRIfromMatrix: MRI frames = %d, does not equal\n",
	   mri->nframes);
    printf("       matrix dim = %dx%d = %d",M->rows,M->cols,M->rows*M->cols);
    return(1);
  }

  f = 0;
  for(mr=1; mr <= M->rows; mr++){
    for(mc=1; mc <= M->cols; mc++){
      MRIsetVoxVal(mri,c,r,s,f,M->rptr[mr][mc]);
      f++;
    }
  }
  return(0);
}

/*---------------------------------------------------------------*/
int MRIfromSymMatrix(MRI *mri, int c, int r, int s, MATRIX *M)
{
  int mr,mc,f, nframesexp;

  nframesexp = M->rows*(M->rows+1)/2;
  if(mri->nframes != nframesexp){
    printf("ERROR: MRIfromSumMatrix: MRI frames = %d, does not equal\n",
	   mri->nframes);
    printf("       matrix dim = %dx%d = %d",M->rows,M->cols,M->rows*M->cols);
    return(1);
  }

  f = 0;
  for(mr=1; mr <= M->rows; mr++){
    for(mc=mr; mc <= M->cols; mc++){
      MRIsetVoxVal(mri,c,r,s,f,M->rptr[mr][mc]);
      f++;
    }
  }
  return(0);
}

/*---------------------------------------------------------------*/
int MRIglmpvFit(GLMPV *glmpv)
{
  int c,r,s,n,f,nc,nr,ns;
  MATRIX *X0, *X=NULL, *Xt=NULL, *XtX=NULL, *iXtX=NULL, *y, *Xty=NULL;
  MATRIX *beta=NULL,*yhat=NULL,*eres=NULL, *Mtmp;
  MATRIX *C[50],*Ct[50];
  MATRIX *gam=NULL, *gamt=NULL, *CiXtX=NULL,*CiXtXCt=NULL;
  MATRIX *iCiXtXCt=NULL,*gtiCiXtXCt=NULL, *gtiCiXtXCtg=NULL;
  double rvar,F,m,v;

  glmpv->DOF = glmpv->Xg->rows - (glmpv->Xg->cols + glmpv->npvr);

  X0 = MatrixAlloc(glmpv->Xg->rows, glmpv->Xg->cols + glmpv->npvr, MATRIX_REAL);
  y = MatrixAlloc(glmpv->Xg->rows, 1, MATRIX_REAL);

  nc = glmpv->y->width;
  nr = glmpv->y->height;
  ns = glmpv->y->depth;
  glmpv->beta = MRIallocSequence(nc, nr, ns, MRI_FLOAT, X0->cols) ;
  glmpv->eres = MRIallocSequence(nc, nr, ns, MRI_FLOAT, glmpv->y->nframes) ;
  glmpv->rvar = MRIallocSequence(nc, nr, ns, MRI_FLOAT, 1);

  if(glmpv->yhatsave)
    glmpv->yhat = MRIallocSequence(nc, nr, ns, MRI_FLOAT, glmpv->y->nframes) ;

  for(n = 0; n < glmpv->ncontrasts; n++){
    glmpv->gamma[n] = MRIallocSequence(nc,nr,ns,MRI_FLOAT, glmpv->C[n]->rows);
    glmpv->F[n] = MRIallocSequence(nc, nr, ns,MRI_FLOAT, 1);
    C[n] = glmpv->C[n];
    Ct[n] = MatrixTranspose(C[n],NULL);
  }

  // pre-load X0
  for(f = 1; f <= X0->rows; f++){
    for(n = 1; n <= glmpv->Xg->cols; n++){
      X0->rptr[f][n] = glmpv->Xg->rptr[f][n];
    }
  }

  //--------------------------------------------
  glmpv->n_ill_cond = 0;
  for(c=0; c < nc; c++){
    printf("%d \n",c);
    for(r=0; r < nr; r++){
      for(s=0; s< ns; s++){

	// Check the mask -----------
	if(glmpv->mask != NULL){
	  m = MRIgetVoxVal(glmpv->mask,c,r,s,0);
	  if(m < 0.5) continue;
	}

	// Load y and the per-vox reg --------------------------
	for(f = 1; f <= X0->rows; f++){
	  y->rptr[f][1] = MRIgetVoxVal(glmpv->y,c,r,s,f-1);
	  for(n = 1; n <= glmpv->npvr; n++){
	    X0->rptr[f][n+glmpv->Xg->cols] = 
	      MRIgetVoxVal(glmpv->pvr[n-1],c,r,s,f-1);
	  }
	}

	// Weight X and y
	if(glmpv->w != NULL){
	  X = MatrixCopy(X0,X);
	  for(f = 1; f <= X->rows; f++){
	    v = MRIgetVoxVal(glmpv->w,c,r,s,f-1);	    
	    y->rptr[f][1] *= v;
	    for(n = 1; n <= X->cols; n++) X->rptr[f][n] *= v;
	  }
	}
	else X = X0;

	// Compute intermediate matrices
	Xt   = MatrixTranspose(X,Xt);
	XtX  = MatrixMultiply(Xt,X,XtX);
	Mtmp = MatrixInverse(XtX,iXtX);
	if(Mtmp == NULL){
	  MatrixPrint(stdout,X);
	  exit(1);
	  glmpv->n_ill_cond++;
	  continue;
	}
	iXtX = Mtmp;

	Xty  = MatrixMultiply(Xt,y,Xty);

	// Now do the actual parameter estmation
	beta = MatrixMultiply(iXtX,Xty,beta);

	// Compute residual variance
	yhat = MatrixMultiply(X,beta,yhat);
	eres = MatrixSubtract(y, yhat, eres);
	rvar = 0;
	for(f = 1; f <= eres->rows; f++)
	  rvar += (eres->rptr[f][1] * eres->rptr[f][1]);
	rvar /= glmpv->DOF;

	// Pack data back into MRI
	MRIsetVoxVal(glmpv->rvar,c,r,s,0,rvar);
	MRIfromMatrix(glmpv->beta, c, r, s, beta);
	MRIfromMatrix(glmpv->eres, c, r, s, eres);
	if(glmpv->yhatsave)
	  MRIfromMatrix(glmpv->yhat, c, r, s, yhat);

	// Now do contrasts
	for(n = 0; n < glmpv->ncontrasts; n++){
	  gam         = MatrixMultiply(C[n],beta,gam);
	  gamt        = MatrixTranspose(gam,gamt);
	  CiXtX       = MatrixMultiply(C[n],iXtX,CiXtX);
	  CiXtXCt     = MatrixMultiply(CiXtX,Ct[n],CiXtXCt);
	  iCiXtXCt    = MatrixInverse(CiXtXCt,iCiXtXCt);
	  gtiCiXtXCt  = MatrixMultiply(gamt,iCiXtXCt,gtiCiXtXCt);
	  gtiCiXtXCtg = MatrixMultiply(gtiCiXtXCt,gam,gtiCiXtXCtg);
	  F           = gtiCiXtXCtg->rptr[1][1]/(rvar/C[n]->rows);
	  MRIfromMatrix(glmpv->gamma[n], c, r, s, gam);
	  MRIsetVoxVal(glmpv->F[n],c,r,s,0,F);
	}

      }
    }
  }

  MatrixFree(&X0);    MatrixFree(&Xt);   MatrixFree(&XtX);
  MatrixFree(&iXtX);  MatrixFree(&y);    MatrixFree(&Xty); 
  MatrixFree(&beta);  MatrixFree(&yhat); MatrixFree(&eres); 
  MatrixFree(&gam);   MatrixFree(&gamt); MatrixFree(&CiXtX); 
  MatrixFree(&CiXtXCt);    MatrixFree(&iCiXtXCt); 
  MatrixFree(&gtiCiXtXCt); MatrixFree(&gtiCiXtXCtg); 
  for(n = 0; n < glmpv->ncontrasts; n++) MatrixFree(&Ct[n]); 
  if(glmpv->w != NULL) MatrixFree(&X); 

  printf("n_ill_cond = %d\n",glmpv->n_ill_cond);
  return(0);
}

/*------------------------------------------------------------------------
  MRInormWeights() - rescales each voxel so that the sum across all
  frames equals nframes. If sqrtFlag=1, then computes the sqrt(w)
  before normalzing.  If invFlag=1, then computes the 1/w before
  normalzing.  The sqrt and inv can be used if the weights are
  variances for WLMS. Can be done in-place. If mask, then ignores
  voxels where mask<0.5. Weights must be >  0.
  *------------------------------------------------------*/
MRI *MRInormWeights(MRI *w, int sqrtFlag, int invFlag, MRI *mask, MRI *wn)
{
  int c,r,s,f;
  double v, vsum, m;

  //-------------------------------------------
  if(wn == NULL){
    wn = MRIallocSequence(w->width,w->height,w->depth,
			  MRI_FLOAT,w->nframes);
    if(wn == NULL){
      printf("ERROR: MRInormWeights(): could not alloc weights\n");
      return(NULL);
    }
    MRIcopyHeader(wn,w);
  }

  //-------------------------------------------
  for(c=0; c < w->width; c++){
    for(r=0; r < w->height; r++){
      for(s=0; s < w->depth; s++){

	if(mask != NULL){
	  m = MRIgetVoxVal(mask,c,r,s,0);
	  if(m < 0.5) continue;
	}

	// First go through and compute the sum
	vsum = 0;
	for(f = 0; f < w->nframes; f++){
	  v = MRIgetVoxVal(w,c,r,s,f);
	  if(v <= 0){
	    printf("ERROR: MRInormWeights: value less than or eq to 0.\n");
	    printf("  c=%d, r=%d, s=%d, v=%g\n",c,r,s,v);
	    // Should do a free here, I guess
	    return(NULL);
	  }
	  if(sqrtFlag) v = sqrt(v);
	  if(invFlag)  v = 1/v;
	  vsum += v;
	}

	// So that the sum = nframes
	vsum /= w->nframes; 

	// Now rescale
	for(f = 0; f < w->nframes; f++){
	  v = MRIgetVoxVal(w,c,r,s,f);
	  if(sqrtFlag) v = sqrt(v);
	  if(invFlag)  v = 1/v;
	  v = v/vsum;
	  MRIsetVoxVal(wn,c,r,s,f,v);
	}
      }
    }
  }

  return(wn);
}


