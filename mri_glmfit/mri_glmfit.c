// mri_glmfit.c

// FSGD not quite ready
// Save some sort of config in output dir.

// Permute X
// Leave out one
// Copies or Links to source data

// Check to make sure no two contrast names are the same
// Check to make sure no two contrast mtxs are the same
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
#include "pdf.h"
#include "fsgdf.h"
#include "timer.h"
#include "matfile.h"

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_glmfit.c,v 1.27 2005/09/28 00:00:43 greve Exp $";
char *Progname = NULL;

char *yFile = NULL, *XFile=NULL, *betaFile=NULL, *rvarFile=NULL;
char *yhatFile=NULL, *eresFile=NULL, *wFile=NULL, *maskFile;
char *condFile=NULL;
char *GLMDir=NULL;
char *pvrFiles[50];
int synth = 0;
int yhatSave=0;
int condSave=0;

MRI *mritmp=NULL, *sig=NULL;

int debug = 0, checkoptsonly = 0;
char tmpstr[2000];

int nContrasts=0;
char *CFile[100];
int err,c,r,s;
int SynthSeed = -1;
double Xcond;

int npvr=0;
MRIGLM *mriglm=NULL, *mriglmtmp=NULL;

char voxdumpdir[1000];
int voxdump[3];
int voxdumpflag = 0;

char *fsgdfile = NULL;
FSGD *fsgd=NULL;
char  *gd2mtx_method = "none";

int nSelfReg = 0;
int crsSelfReg[100][3];
char *SUBJECTS_DIR;
int cmax, rmax, smax;
double Fmax, sigmax;

int pcaSave=0;
int npca = -1;
MATRIX *Upca=NULL,*Spca=NULL;
MRI *Vpca=NULL;

/*--------------------------------------------------*/
int main(int argc, char **argv)
{
  int nargs,n;
  struct utsname uts;
  char *cmdline, cwd[2000];
  struct timeb  mytimer;
  int msecFitTime;
  MATRIX *wvect=NULL, *Mtmp=NULL, *Xselfreg=NULL, *Xnorm=NULL;
  FILE *fp;

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
  mriglm = (MRIGLM *) calloc(sizeof(MRIGLM),1);
  mriglm->glm = GLMalloc();

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

  if(synth){
    if(SynthSeed < 0) SynthSeed = PDFtodSeed();
    printf("SynthSeed = %d\n",SynthSeed);
  }

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

  mriglm->npvr = npvr;
  mriglm->yhatsave = yhatSave;
  mriglm->condsave = condSave;

  // X ---------------------------------------------------------
  //Load global X------------------------------------------------

  if(XFile != NULL){  
    mriglm->Xg = MatrixReadTxt(XFile, NULL);
    if(mriglm->Xg==NULL){
      printf("ERROR: loading X %s\n",XFile);
      exit(1);
    }
  }
  else{
    mriglm->Xg = gdfMatrix(fsgd,gd2mtx_method,NULL);
    if(mriglm->Xg==NULL) exit(1);
  }

  // Check the condition of the global matrix -----------------
  Xnorm = MatrixNormalizeCol(mriglm->Xg,NULL);
  Xcond = MatrixNSConditionNumber(Xnorm);
  printf("Matrix condition is %g\n",Xcond);
  if(Xcond > 10000){
    printf("ERROR: matrix is ill-conditioned or badly scaled, condno = %g\n",
	   Xcond);
    MatrixPrint(stdout,mriglm->Xg);
    exit(1);
  }
  // Load Per-Voxel Regressors -----------------------------------
  if(mriglm->npvr > 0){
    for(n=0; n < mriglm->npvr; n++){
      mriglm->pvr[n] = MRIread(pvrFiles[n]);
      if(mriglm->pvr[n] == NULL) exit(1);
      if(mriglm->pvr[n]->nframes != mriglm->Xg->rows){
	printf("ERROR: dimension mismatch between pvr and X.\n");
	printf("  pvr has %d frames, X has %d rows.\n",
	       mriglm->pvr[n]->nframes,mriglm->Xg->rows);
	printf("PVR %d %s\n",n,pvrFiles[n]);
	exit(1);
      }
    }
  }

  // --------------------------------------------------------------
  // Load or synthesize input--------------------------------------
  if(synth == 0){
    printf("Loading y from %s\n",yFile);
    mriglm->y = MRIread(yFile);
    if(mriglm->y == NULL){
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
    mriglm->y =  MRIrandn(mritmp->width, mritmp->height, mritmp->depth, 
			 mritmp->nframes,0,1,NULL);
    MRIfree(&mritmp);
  }
  // Check number of frames ----------------------------------
  if(mriglm->y->nframes != mriglm->Xg->rows){
    printf("ERROR: dimension mismatch between y and X.\n");
    printf("  y has %d inputs, X has %d rows.\n",
	   mriglm->y->nframes,mriglm->Xg->rows);
    exit(1);
  }
  // Load the weight file ----------------------------------
  if(wFile != NULL){
    mriglm->w = MRIread(wFile);
    if(mriglm->w  == NULL){
      printf("ERROR: reading weight file %s\n",wFile);
      exit(1);
    }
    // Check number of frames
    if(mriglm->y->nframes != mriglm->w->nframes){
      printf("ERROR: dimension mismatch between y and w.\n");
      printf("  y has %d frames, w has %d frames.\n",
	     mriglm->y->nframes,mriglm->w->nframes);
      exit(1);
    }
    // Invert, Sqrt, and Normalize the weights
    mritmp = MRInormWeights(mriglm->w, 1, 1, mriglm->mask, mriglm->w);
    if(mritmp==NULL) exit(1);
    sprintf(tmpstr,"%s/wn.mgh",GLMDir);
    MRIwrite(mriglm->w,tmpstr);
  }
  else mriglm->w = NULL;

  // Handle self-regressors
  if(nSelfReg > 0){
    for(n=0; n<nSelfReg; n++){
      c = crsSelfReg[n][0];
      r = crsSelfReg[n][1];
      s = crsSelfReg[n][2];
      printf("Self regressor %d     %d %d %d\n",n,c,r,s);
      if(c < 0 || c >= mriglm->y->width || 
	 r < 0 || r >= mriglm->y->height ||
	 s < 0 || s >= mriglm->y->depth){
	printf("ERROR: %d self regressor is out of the volume (%d,%d,%d)\n",
	       n,c,r,s);
	exit(1);
      }
      MRIglmLoadVox(mriglm,c,r,s);
      GLMxMatrices(mriglm->glm);
      GLMfit(mriglm->glm);
      GLMdump("selfreg",mriglm->glm);
      Mtmp = MatrixHorCat(Xselfreg,mriglm->glm->eres,NULL);
      if(n > 0) MatrixFree(&Xselfreg);
      Xselfreg = Mtmp;
    }

    // Need new mriglm, so alloc and copy the old one
    mriglmtmp = (MRIGLM *) calloc(sizeof(MRIGLM),1);
    mriglmtmp->glm = GLMalloc();
    mriglmtmp->y = mriglm->y;
    mriglmtmp->w = mriglm->w;
    mriglmtmp->Xg = MatrixHorCat(mriglm->Xg,Xselfreg,NULL);
    for(n=0; n < mriglm->npvr; n++) mriglmtmp->pvr[n] = mriglmtmp->pvr[n];
    //MRIglmFree(&mriglm);
    mriglm = mriglmtmp;
  }
  MRIglmNRegTot(mriglm);

  // Load the contrast matrices ---------------------------------
  mriglm->glm->ncontrasts = nContrasts;
  if(nContrasts > 0){
    for(n=0; n < nContrasts; n++){
      mriglm->glm->C[n] = MatrixReadTxt(CFile[n], NULL);
      if(mriglm->glm->C[n] == NULL){
	printf("ERROR: loading C %s\n",CFile[n]);
	exit(1);
      }
      if(mriglm->glm->C[n]->cols != mriglm->nregtot){
	printf("ERROR: dimension mismatch between X and contrast %s",CFile[n]);
	printf("       X has %d cols, C has %d cols\n",
	       mriglm->nregtot,mriglm->glm->C[n]->cols);
	exit(1);
      }
      // Get its name
      mriglm->glm->Cname[n] = fio_basename(CFile[n],".mat");
      // Should check to make sure no two are the same
    }
  }

  // At this point, y, X, and C have been loaded, now pre-alloc
  GLMallocX(mriglm->glm,mriglm->y->nframes,mriglm->nregtot);
  GLMallocY(mriglm->glm);

  // Check DOF
  GLMdof(mriglm->glm);
  printf("DOF = %g\n",mriglm->glm->dof);
  if(mriglm->glm->dof < 1){
    printf("ERROR: DOF = %g\n",mriglm->glm->dof);
    exit(1);
  }

  // Compute Contrast-related matrices
  GLMcMatrices(mriglm->glm);

  // Load the mask file ----------------------------------
  if(maskFile != NULL){
    mriglm->mask = MRIread(maskFile);
    if(mriglm->mask  == NULL){
      printf("ERROR: reading mask file %s\n",maskFile);
      exit(1);
    }
  }
  else mriglm->mask = NULL;


  // Dump a voxel
  if(voxdumpflag){
    sprintf(voxdumpdir,"%s/voxdump-%d-%d-%d",GLMDir,
	    voxdump[0],voxdump[1],voxdump[2]);
    printf("Dumping voxel %d %d %d to %s\n",
	   voxdump[0],voxdump[1],voxdump[2],voxdumpdir);
    MRIglmLoadVox(mriglm,voxdump[0],voxdump[1],voxdump[2]);
    GLMxMatrices(mriglm->glm);
    GLMfit(mriglm->glm);
    GLMtest(mriglm->glm);
    GLMdump(voxdumpdir,mriglm->glm);
    if(mriglm->w){
      wvect = MRItoVector(mriglm->w,voxdump[0],voxdump[1],voxdump[2],NULL);
      sprintf(tmpstr,"%s/w.dat",voxdumpdir);
      MatrixWriteTxt(tmpstr,wvect);
    }
    exit(0);
  }

  if(pcaSave){
    if(npca < 0) npca = mriglm->y->nframes;
    if(npca > mriglm->y->nframes){
      printf("ERROR: npca = %d, max can be %d\n",npca,mriglm->y->nframes);
      exit(1);
    }
  }

  // Now do the estimation and testing
  TimerStart(&mytimer) ;
  printf("Starting fit\n");
  MRIglmFit(mriglm);
  msecFitTime = TimerStop(&mytimer) ;
  printf("Fit completed in %g minutes\n",msecFitTime/(1000*60.0));

  // Save estimation results
  printf("Writing results\n");
  MRIwrite(mriglm->beta,betaFile);
  MRIwrite(mriglm->rvar,rvarFile);
  if(mriglm->yhatsave) MRIwrite(mriglm->yhat,yhatFile);
  if(mriglm->condsave) MRIwrite(mriglm->cond,condFile);
  if(eresFile) MRIwrite(mriglm->eres,eresFile);    

  sprintf(tmpstr,"%s/Xg.dat",GLMDir);
  MatrixWriteTxt(tmpstr, mriglm->Xg);
  
  // Save the contrast results
  for(n=0; n < mriglm->glm->ncontrasts; n++){
    printf("  %s\n",mriglm->glm->Cname[n]);
    
    // Create output directory for contrast
    sprintf(tmpstr,"%s/%s",GLMDir,mriglm->glm->Cname[n]);
    mkdir(tmpstr,(mode_t)-1);
    
    // Dump contrast matrix
    sprintf(tmpstr,"%s/%s/C.dat",GLMDir,mriglm->glm->Cname[n]);
    MatrixWriteTxt(tmpstr, mriglm->glm->C[n]);

    // Save gamma 
    sprintf(tmpstr,"%s/%s/gamma.mgh",GLMDir,mriglm->glm->Cname[n]);
    MRIwrite(mriglm->gamma[n],tmpstr);

    // Save F
    sprintf(tmpstr,"%s/%s/F.mgh",GLMDir,mriglm->glm->Cname[n]);
    MRIwrite(mriglm->F[n],tmpstr);
    
    // Compute and Save -log10 p-values
    sig=MRIlog10(mriglm->p[n],sig,1);

    // If it is t-test (ie, one row) then apply the sign
    if(mriglm->glm->C[n]->rows == 1) MRIsetSign(sig,mriglm->gamma[n],0);

    // Write out the sig
    sprintf(tmpstr,"%s/%s/sig.mgh",GLMDir,mriglm->glm->Cname[n]);
    MRIwrite(sig,tmpstr);

    // Find and save the max sig
    sigmax = MRIframeMax(sig,0,mriglm->mask,1,&cmax,&rmax,&smax);
    Fmax = MRIgetVoxVal(mriglm->F[n],cmax,rmax,smax,0);
    printf("    maxvox sig=%g  F=%g  at  index %d %d %d    seed=%d\n",
	   sigmax,Fmax,cmax,rmax,smax,SynthSeed);

    sprintf(tmpstr,"%s/%s/maxvox.dat",GLMDir,mriglm->glm->Cname[n]);
    fp = fopen(tmpstr,"w");
    fprintf(fp,"%e  %e    %d %d %d     %d\n",
	    sigmax,Fmax,cmax,rmax,smax,SynthSeed);
    fclose(fp);
    
    MRIfree(&sig);
  }

  // --------- Save FSGDF stuff --------------------------------
  if(fsgd != NULL){
    strcpy(fsgd->measname,"external");

    sprintf(fsgd->datafile,"%s",yFile);

    sprintf(tmpstr,"%s/fsgd.X.mat",GLMDir);
    MatlabWrite(mriglm->Xg,tmpstr,"X");
    sprintf(fsgd->DesignMatFile,"fsgd.X.mat");

    sprintf(tmpstr,"%s/y.fsgd",GLMDir);
    fp = fopen(tmpstr,"w");
    gdfPrintHeader(fp,fsgd);
    fprintf(fp,"Creator          %s\n",Progname);
    fprintf(fp,"SUBJECTS_DIR     %s\n",SUBJECTS_DIR);
    fprintf(fp,"SynthSeed        %d\n",SynthSeed);
    fclose(fp);
  }

  // Compute and save PCA
  if(pcaSave){
    printf("Computing PCA (%d)\n",npca);
    sprintf(tmpstr,"%s/eres-pca",GLMDir);
    mkdir(tmpstr,(mode_t)-1);
    err=MRIpca(mriglm->eres, &Upca, &Spca, &Vpca, mriglm->mask);
    if(err) exit(1);
    sprintf(tmpstr,"%s/eres-pca/v.mgh",GLMDir);
    MRIwrite(Vpca,tmpstr);
    sprintf(tmpstr,"%s/eres-pca/u.mat",GLMDir);
    MatrixWriteTxt(tmpstr, Upca);
    sprintf(tmpstr,"%s/eres-pca/sdiag.mat",GLMDir);
    MatrixWriteTxt(tmpstr, Spca);
    sprintf(tmpstr,"%s/eres-pca/stats.dat",GLMDir);
    WritePCAStats(tmpstr,Spca);
  }

  printf("mri_glmfit done\n");
  return(0); 
  exit(0);

}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/

/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv)
{
  int  nargc , nargsused, msec, niters;
  char **pargv, *option ;
  double rvartmp;

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
    else if (!strcasecmp(option, "--save-yhat")) yhatSave = 1;
    else if (!strcasecmp(option, "--save-cond")) condSave = 1;

    else if (!strcasecmp(option, "--seed")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&SynthSeed);
      srand48(SynthSeed);
      nargsused = 1;
    }
    else if (!strcasecmp(option, "--voxdump")){
      if(nargc < 3) CMDargNErr(option,3);
      sscanf(pargv[0],"%d",&voxdump[0]);
      sscanf(pargv[1],"%d",&voxdump[1]);
      sscanf(pargv[2],"%d",&voxdump[2]);
      voxdumpflag = 1;
      nargsused = 3;
    }
    else if (!strcasecmp(option, "--selfreg")){
      if(nargc < 3) CMDargNErr(option,3);
      sscanf(pargv[0],"%d",&crsSelfReg[nSelfReg][0]);
      sscanf(pargv[1],"%d",&crsSelfReg[nSelfReg][1]);
      sscanf(pargv[2],"%d",&crsSelfReg[nSelfReg][2]);
      nSelfReg++;
      nargsused = 3;
    }
    else if (!strcasecmp(option, "--profile")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&niters);
      if(SynthSeed < 0) SynthSeed = PDFtodSeed();
      srand48(SynthSeed);
      printf("Starting GLM profile over %d iterations. Seed=%d\n",
	     niters,SynthSeed);
      msec = GLMprofile(200, 20, 5, niters);
      nargsused = 1;
      exit(0);
    }
    else if (!strcasecmp(option, "--resynthtest")){
      if(nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%d",&niters);
      if(SynthSeed < 0) SynthSeed = PDFtodSeed();
      srand48(SynthSeed);
      printf("Starting GLM resynth test over %d iterations. Seed=%d\n",
	     niters,SynthSeed);
      err = GLMresynthTest(niters, &rvartmp);
      if(err){
	printf("Failed. rvar = %g\n",rvartmp);
	exit(1);
      }
      printf("Passed. rvarmax = %g\n",rvartmp);
      exit(0);
      nargsused = 1;
    }
    else if (!strcmp(option, "--y")){
      if(nargc < 1) CMDargNErr(option,1);
      yFile = fio_fullpath(pargv[0]);
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
    else if (!strcmp(option, "--pca")){
      if(nargc > 0 && !CMDnthIsArg(nargc, pargv, 0)){
	sscanf(pargv[0],"%d",&niters);
	nargsused = 1;
      }
      pcaSave = 1;
    }
    else if ( !strcmp(option, "--fsgd") ){
      if(nargc < 1) CMDargNErr(option,1);
      fsgdfile = pargv[0];
      nargsused = 1;
      fsgd = gdfRead(fsgdfile,0);
      if(fsgd==NULL) exit(1);
      if(CMDnthIsArg(nargc, pargv, 1)){
	gd2mtx_method = pargv[1]; nargsused ++;
	if(gdfCheckMatrixMethod(gd2mtx_method)) exit(1);
      }
      else gd2mtx_method = "dods";
      printf("INFO: gd2mtx_method is %s\n",gd2mtx_method);
      strcpy(fsgd->DesignMatMethod,gd2mtx_method);
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
  printf("\n");
  printf("   --X design matrix file\n");
  printf("   --fsgd FSGDF <gd2mtx>\n");
  printf("   --pvr pvr1 <--prv pvr2 ...>\n");
  printf("   --selfreg c r s\n");
  printf("\n");
  printf("   --w weight volume\n");
  printf("   --mask mask volume\n");
  printf("   --C contrast1.mat <--C contrast2.mat ...>\n");
  printf("\n");
  printf("   --glmdir dir : save outputs to dir\n");
  printf("\n");
  printf("   --save-yhat \n");
  printf("   --save-cond \n");
  printf("\n");
  printf("   --synth : replace input with gaussian \n");
  printf("   --seed seed \n");
  printf("\n");
  printf("   --voxdump c r s \n");
  printf("   --resynthtest niters \n");
  printf("   --profile     niters \n");
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
  if(XFile == NULL && fsgdfile == NULL){
    printf("ERROR: must specify an input X file or fsgd file\n");
    exit(1);
  }
  if(XFile && fsgdfile ){
    printf("ERROR: cannot specify both X file and fsgd file\n");
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
    if(condSave){
      sprintf(tmpstr,"%s/cond.mgh",GLMDir);
      condFile = strcpyalloc(tmpstr);
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

  if(SUBJECTS_DIR == NULL){
    SUBJECTS_DIR = getenv("SUBJECTS_DIR");
    if(SUBJECTS_DIR==NULL){
      fprintf(stderr,"ERROR: SUBJECTS_DIR not defined in environment\n");
      exit(1);
    }
  }

  return;
}

/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  int n;

  fprintf(fp,"y    %s\n",yFile);
  if(XFile)     fprintf(fp,"X    %s\n",XFile);
  if(fsgdfile)  fprintf(fp,"FSGD File %s\n",fsgdfile);
  fprintf(fp,"beta %s\n",betaFile);
  fprintf(fp,"rvar %s\n",rvarFile);
  if(maskFile) fprintf(fp,"mask %s\n",maskFile);
  if(yhatFile) fprintf(fp,"yhat %s\n",yhatFile);
  if(eresFile) fprintf(fp,"eres %s\n",eresFile);
  if(condFile) fprintf(fp,"cond %s\n",condFile);

  for(n=0; n < nSelfReg; n++){
    printf("SelfRegressor %d  %4d %4d %4d\n",n+1,
	   crsSelfReg[n][0],crsSelfReg[n][1],crsSelfReg[n][2]);
  }

  return;
}


