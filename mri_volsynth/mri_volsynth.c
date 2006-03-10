/*
  Name:    mri_volsynth
  Author:  Douglas N. Greve 
  email:   analysis-bugs@nmr.mgh.harvard.edu
  Date:    2/27/02
  Purpose: Synthesize a volume.
  $Id: mri_volsynth.c,v 1.15.2.1 2006/03/10 20:37:37 greve Exp $
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "error.h"
#include "diag.h"
#include "proto.h"

#include "mri_identify.h"
#include "matrix.h"
#include "mri.h"
#include "MRIio_old.h"
#include "randomfields.h"

MRI *fMRIsqrt(MRI *mri, MRI *mrisqrt);

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void argnerr(char *option, int n);
static void dump_options(FILE *fp);
static int  singledash(char *flag);
static int  nth_is_arg(int nargc, char **argv, int nth);
static int checkfmt(char *fmt);
static int getfmtid(char *fname);
static int  isflag(char *flag);
//static int  stringmatch(char *str1, char *str2);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_volsynth.c,v 1.15.2.1 2006/03/10 20:37:37 greve Exp $";
char *Progname = NULL;

int debug = 0;

char *volid = NULL;
char *vol_type;
int   volfmtid;
char *volfmt = NULL;

char *tempid = NULL; // Template
char *temp_type;
int   tempfmtid;
char *tempfmt = NULL;

int dim[4];
float res[4];
float cras[4];
float cdircos[3], rdircos[3], sdircos[3];
char *pdfname = "gaussian";
char *precision=NULL; /* not used yet */
MRI *mri, *mrism, *mritemp, *mri2;
long seed = -1; /* < 0 for auto */
char *seedfile = NULL;
float fwhm = 0, gstd = 0, gmnnorm = 1;
int nframes = -1;
int delta_crsf[4];
int delta_crsf_speced = 0;
double delta_value = 1, delta_off_value = 0;
double gausmean=0, gausstd=1;
RFS *rfs;
int rescale = 0;
int numdof =  2;
int dendof = 20;

/*---------------------------------------------------------------*/
int main(int argc, char **argv)
{
  double voxradius;

  Progname = argv[0] ;
  argc --;
  argv++;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  /* assign default geometry */
  cdircos[0] = 1.0;
  cdircos[1] = 0.0;
  cdircos[2] = 0.0;
  rdircos[0] = 0.0;
  rdircos[1] = 1.0;
  rdircos[2] = 0.0;
  sdircos[0] = 0.0;
  sdircos[1] = 0.0;
  sdircos[2] = 1.0;
  res[0] = 1.0;
  res[1] = 1.0;
  res[2] = 1.0;
  cras[0] = 0.0;
  cras[1] = 0.0;
  cras[2] = 0.0;
  res[3] = 2.0; /* TR */

  if(argc == 0) usage_exit();

  parse_commandline(argc, argv);
  check_options();
  dump_options(stdout);

  if(tempid != NULL){
    printf("INFO: reading template header\n");
    mritemp = MRIreadHeader(tempid,tempfmtid);
    if(mritemp == NULL){
      printf("ERROR: reading %s header\n",tempid);
      exit(1);
    }
    dim[0] = mritemp->width;
    dim[1] = mritemp->height;
    dim[2] = mritemp->depth;
    if(nframes > 0) dim[3] = nframes;
    else            dim[3] = mritemp->nframes;
  }

  printf("Synthesizing\n");
  srand48(seed);
  if(strcmp(pdfname,"gaussian")==0)
    mri = MRIrandn(dim[0], dim[1], dim[2], dim[3], gausmean, gausstd, NULL);
  else if(strcmp(pdfname,"uniform")==0)
    mri = MRIdrand48(dim[0], dim[1], dim[2], dim[3], 0, 1, NULL);
  else if(strcmp(pdfname,"const")==0)
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 1, NULL);
  else if(strcmp(pdfname,"sphere")==0){
    voxradius = sqrt( pow(dim[0]/2.0,2)+pow(dim[1]/2.0,2)+pow(dim[2]/2.0,2) )/2.0;
    printf("voxradius = %lf\n",voxradius);
    mri = MRIsphereMask(dim[0], dim[1], dim[2], dim[3], 
			dim[0]/2.0, dim[1]/2.0, dim[2]/2.0,
			voxradius, 1, NULL);
  }
  else if(strcmp(pdfname,"delta")==0){
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], delta_off_value, NULL);
    if(delta_crsf_speced == 0){
      delta_crsf[0] = dim[0]/2;
      delta_crsf[1] = dim[1]/2;
      delta_crsf[2] = dim[2]/2;
      delta_crsf[3] = dim[3]/2;
    }
    printf("delta set to %g at %d %d %d %d\n",delta_value,delta_crsf[0],
	   delta_crsf[1],delta_crsf[2],delta_crsf[3]);
    MRIFseq_vox(mri,delta_crsf[0],delta_crsf[1],delta_crsf[2],delta_crsf[3]) = delta_value;
  }
  else if(strcmp(pdfname,"chi2")==0){
    rfs = RFspecInit(seed,NULL);
    rfs->name = strcpyalloc("chi2");
    rfs->params[0] = dendof;
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    printf("Synthesizing chi2 with dof=%d\n",dendof);
    RFsynth(mri,rfs,NULL);
  }
  else if(strcmp(pdfname,"z")==0){
    printf("Synthesizing z \n");
    rfs = RFspecInit(seed,NULL);
    rfs->name = strcpyalloc("gaussian");
    rfs->params[0] = 0; // mean
    rfs->params[1] = 1; // std
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri,rfs,NULL);
  }
  else if(strcmp(pdfname,"t")==0){
    printf("Synthesizing t with dof=%d\n",dendof);
    rfs = RFspecInit(seed,NULL);
    rfs->name = strcpyalloc("t");
    rfs->params[0] = dendof;
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri,rfs,NULL);
  }
  else if(strcmp(pdfname,"tr")==0){
    printf("Synthesizing t with dof=%d as ratio of z/sqrt(chi2)\n",dendof);
    rfs = RFspecInit(seed,NULL);
    // numerator
    rfs->name = strcpyalloc("gaussian");
    rfs->params[0] = 0; // mean
    rfs->params[1] = 1; // std
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri,rfs,NULL);
    // denominator
    rfs->name = strcpyalloc("chi2");
    rfs->params[0] = dendof;
    mri2 = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri2,rfs,NULL);
    fMRIsqrt(mri2,mri2); // sqrt of chi2
    mri = MRIdivide(mri,mri2,mri);
    MRIscalarMul(mri, mri, sqrt(dendof)) ;
    MRIfree(&mri2);
  }
  else if(strcmp(pdfname,"F")==0){
    printf("Synthesizing F with num=%d den=%d\n",numdof,dendof);
    rfs = RFspecInit(seed,NULL);
    rfs->name = strcpyalloc("F");
    rfs->params[0] = numdof;
    rfs->params[1] = dendof;
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri,rfs,NULL);
  }
  else if(strcmp(pdfname,"Fr")==0){
    printf("Synthesizing F with num=%d den=%d as ratio of two chi2\n",numdof,dendof);
    rfs = RFspecInit(seed,NULL);
    rfs->name = strcpyalloc("chi2");
    // numerator
    rfs->params[0] = numdof;
    mri = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri,rfs,NULL);
    // denominator
    rfs->params[0] = dendof;
    mri2 = MRIconst(dim[0], dim[1], dim[2], dim[3], 0, NULL);
    RFsynth(mri2,rfs,NULL);
    mri = MRIdivide(mri,mri2,mri);
    MRIscalarMul(mri, mri, (double)dendof/numdof) ;
    MRIfree(&mri2);
  }
  else {
    printf("ERROR: pdf %s unrecognized, must be gaussian, uniform, const, or delta\n",
	   pdfname);
    exit(1);
  }

  if(tempid != NULL){
    MRIcopyHeader(mritemp,mri);
    mri->type = MRI_FLOAT;
    if(nframes > 0) mri->nframes = nframes;
  }
  else {
    mri->xsize = res[0];
    mri->ysize = res[1];
    mri->zsize = res[2];
    mri->tr    = res[3];
    mri->x_r = cdircos[0];
    mri->x_a = cdircos[1];
    mri->x_s = cdircos[2];
    mri->y_r = rdircos[0];
    mri->y_a = rdircos[1];
    mri->y_s = rdircos[2];
    mri->z_r = sdircos[0];
    mri->z_a = sdircos[1];
    mri->z_s = sdircos[2];
    mri->c_r = cras[0];
    mri->c_a = cras[1];
    mri->c_s = cras[2];
  }

  if(gstd > 0){
    printf("Smoothing\n");
    MRIgaussianSmooth(mri, gstd, gmnnorm, mri); /* gmnnorm = 1 = normalize */
    if(rescale){
      printf("Rescaling\n");
      if(strcmp(pdfname,"z")==0)     RFrescale(mri,rfs,NULL,mri);
      if(strcmp(pdfname,"chi2")==0)  RFrescale(mri,rfs,NULL,mri);
      if(strcmp(pdfname,"t")==0)     RFrescale(mri,rfs,NULL,mri);
      if(strcmp(pdfname,"tr")==0)    RFrescale(mri,rfs,NULL,mri);
      if(strcmp(pdfname,"F")==0)     RFrescale(mri,rfs,NULL,mri);
      if(strcmp(pdfname,"Fr")==0)    RFrescale(mri,rfs,NULL,mri);
    }
  }

  printf("Saving\n");
  MRIwriteAnyFormat(mri,volid,volfmt,-1,NULL);
  //if(volfmtid == NULL) MRIwrite(mri,volid);
  //else MRIwriteType(mri,volid,);

  return(0);
}
/* ------------------------------------------------------------------ */
static int parse_commandline(int argc, char **argv)
{
  int  i, nargc , nargsused;
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
    else if (!strcasecmp(option, "--nogmnnorm")) gmnnorm = 0;
    else if (!strcasecmp(option, "--rescale")) rescale=1;
    else if (!strcasecmp(option, "--norescale")) rescale=0;

    else if (!strcmp(option, "--vol")){
      if(nargc < 1) argnerr(option,1);
      volid = pargv[0];
      nargsused = 1;
      if(nth_is_arg(nargc, pargv, 1)){
	volfmt = pargv[1]; nargsused ++;
	volfmtid = checkfmt(volfmt);
      }
      else volfmtid = getfmtid(volid);
    }
    else if (!strcmp(option, "--temp")){
      if(nargc < 1) argnerr(option,1);
      tempid = pargv[0];
      nargsused = 1;
      if(nth_is_arg(nargc, pargv, 1)){
	tempfmt = pargv[1]; nargsused ++;
	tempfmtid = checkfmt(tempfmt);
      }
      else tempfmtid = getfmtid(tempid);
    }
    else if ( !strcmp(option, "--dim") ) {
      if(nargc < 4) argnerr(option,4);
      for(i=0;i<4;i++) sscanf(pargv[i],"%d",&dim[i]);
      nargsused = 4;
    }
    else if ( !strcmp(option, "--nframes") ) {
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&nframes);
      nargsused = 1;
    }
    else if ( !strcmp(option, "--res") ) {
      if(nargc < 4) argnerr(option,4);
      for(i=0;i<4;i++) sscanf(pargv[i],"%f",&res[i]);
      nargsused = 4;
    }
    else if ( !strcmp(option, "--c_ras") ) {
      if(nargc < 3) argnerr(option,3);
      for(i=0;i<3;i++) sscanf(pargv[i],"%f",&cras[i]);
      nargsused = 3;
    }
    else if ( !strcmp(option, "--cdircos") ) {
      if(nargc < 3) argnerr(option,3);
      for(i=0;i<3;i++) sscanf(pargv[i],"%f",&cdircos[i]);
      nargsused = 3;
    }
    else if ( !strcmp(option, "--rdircos") ) {
      if(nargc < 3) argnerr(option,3);
      for(i=0;i<3;i++) sscanf(pargv[i],"%f",&rdircos[i]);
      nargsused = 3;
    }
    else if ( !strcmp(option, "--sdircos") ) {
      if(nargc < 3) argnerr(option,3);
      for(i=0;i<3;i++) sscanf(pargv[i],"%f",&sdircos[i]);
      nargsused = 3;
    }
    else if ( !strcmp(option, "--fwhm") ) {
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%f",&fwhm);
      gstd = fwhm/sqrt(log(256.0));
      nargsused = 1;
    }
    else if (!strcmp(option, "--precision")){
      if(nargc < 1) argnerr(option,1);
      precision = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--seed")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%ld",&seed);
      nargsused = 1;
    }
    else if (!strcmp(option, "--seedfile")){
      if(nargc < 1) argnerr(option,1);
      seedfile = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--pdf")){
      if(nargc < 1) argnerr(option,1);
      pdfname = pargv[0];
      nargsused = 1;
    }
    else if (!strcmp(option, "--dof-num")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&numdof);
      nargsused = 1;
    }
    else if (!strcmp(option, "--dof-den") || !strcmp(option, "--dof")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%d",&dendof);
      nargsused = 1;
    }
    else if (!strcmp(option, "--gmean")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%lf",&gausmean);
      nargsused = 1;
    }
    else if (!strcmp(option, "--gstd")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%lf",&gausstd);
      nargsused = 1;
    }
    else if (!strcmp(option, "--delta-crsf")){
      if(nargc < 4) argnerr(option,4);
      sscanf(pargv[0],"%d",&delta_crsf[0]);
      sscanf(pargv[1],"%d",&delta_crsf[1]);
      sscanf(pargv[2],"%d",&delta_crsf[2]);
      sscanf(pargv[3],"%d",&delta_crsf[3]);
      delta_crsf_speced = 1;
      nargsused = 4;
    }
    else if (!strcmp(option, "--delta-val")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%lf",&delta_value);
      nargsused = 1;
    }
    else if (!strcmp(option, "--delta-val-off")){
      if(nargc < 1) argnerr(option,1);
      sscanf(pargv[0],"%lf",&delta_off_value);
      nargsused = 1;
    }
    else{
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if(singledash(option))
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
  printf("   --vol volid <fmt> : output volume path id and format\n");
  printf("\n");
  printf(" Get geometry from template\n");
  printf("   --temp templateid <fmt>\n");
  printf("   --nframes nframes : override template\n");  
  printf("\n");
  printf(" Specify geometry explicitly\n");
  printf("   --dim nc nr ns nf  (required)\n");
  printf("   --res dc dr ds df\n");
  printf("   --cdircos x y z\n");
  printf("   --rdircos x y z\n");
  printf("   --sdircos x y z\n");
  printf("   --c_ras   c_r c_a c_s\n");
  printf("   --precision precision : eg, float\n");
  printf("\n");
  printf(" Value distribution flags\n");
  printf("   --seed seed (default is time-based auto)\n");
  printf("   --seedfile fname : write seed value to this file\n");
  printf("   --pdf pdfname : <gaussian>, uniform, const, delta, sphere, z, t, F, chi2\n");
  printf("   --gmean mean : use mean for gaussian (def is 0)\n");
  printf("   --gstd  std  : use std for gaussian standard dev (def is 1)\n");
  printf("   --delta-crsf col row slice frame : 0-based\n");
  printf("   --delta-val val : set delta value to val. Default is 1.\n");
  printf("   --delta-val-off offval : set delta background value to offval. Default is 0.\n");
  printf("   --dof dof : dof for t and chi2 \n");
  printf("   --dof-num numdof : numerator dof for F\n");
  printf("   --dof-den dendof : denomenator dof for F\n");
  printf("   --rescale : rescale z, t, F, or chi2 after smoothing\n");
  printf("\n");
  printf(" Other arguments\n");
  printf("   --fwhm fwhmmm : smooth by FWHM mm\n");
  printf("\n");
}
/* --------------------------------------------- */
static void print_help(void)
{
  print_usage() ;
  printf("Synthesizes a volume with the given geometry and pdf. Default pdf \n");
  printf("is gaussian (mean 0, std 1). If uniform is chosen, then the min\n");
  printf("is 0 and the max is 1. If const is chosen, then all voxels are set\n");
  printf("to 1. If delta, the middle voxel is set to 1, the rest to 0 unless\n");
  printf("the actual location is chosen with --delta-crsf. If sphere, then\n");
  printf("a spherical binary mask centered in the volume with radius half\n");
  printf("the field of view.\n");
  printf("\n");

  exit(1) ;
}
/* --------------------------------------------- */
static void print_version(void)
{
  printf("%s\n", vcid) ;
  exit(1) ;
}
/* --------------------------------------------- */
static void argnerr(char *option, int n)
{
  if(n==1)
    fprintf(stderr,"ERROR: %s flag needs %d argument\n",option,n);
  else
    fprintf(stderr,"ERROR: %s flag needs %d arguments\n",option,n);
  exit(-1);
}
/* --------------------------------------------- */
static void check_options(void)
{
  struct timeval tv;
  FILE *fp;

  if(volid == NULL){
    printf("A volume path must be supplied\n");
    exit(1);
  }
  if(seed < 0){
    gettimeofday(&tv, NULL);
    seed = tv.tv_sec + tv.tv_usec;
  }
  if(seedfile != NULL){
    fp = fopen(seedfile,"w");
    if(fp == NULL){
      printf("ERROR: cannot open seed file %s\n",seedfile);
      exit(1);
    }
    fprintf(fp,"%ld\n",seed);
    fclose(fp);
  }

  return;
}
/* --------------------------------------------- */
static void dump_options(FILE *fp)
{
  fprintf(fp,"volid    %s\n",volid);
  fprintf(fp,"volfmt   %s\n",volfmt);
  if(tempid == 0){
    fprintf(fp,"dim      %3d %3d %3d %3d\n",
	    dim[0],dim[1],dim[2],dim[3]);
    fprintf(fp,"res      %6.4f %6.4f %6.4f %6.4f\n",
	    res[0],res[1],res[2],res[3]);
    fprintf(fp,"c_ras  %6.4f %6.4f %6.4f\n",
	    cras[0], cras[1], cras[2]);
    fprintf(fp,"col   dircos  %6.4f %6.4f %6.4f\n",
	    cdircos[0],cdircos[1],cdircos[2]);
    fprintf(fp,"row   dircos  %6.4f %6.4f %6.4f\n",
	    rdircos[0],rdircos[1],rdircos[2]);
    fprintf(fp,"slice dircos  %6.4f %6.4f %6.4f\n",
	    sdircos[0],sdircos[1],sdircos[2]);
  }
  else{
    fprintf(fp,"Using %s as template\n",tempid);
    if(nframes > 0) fprintf(fp,"nframes = %d\n",nframes);
  }
  fprintf(fp,"fwhm = %g, gstd  = %g\n",fwhm,gstd);
  //fprintf(fp,"precision %s\n",precision);
  fprintf(fp,"seed %ld\n",seed);
  fprintf(fp,"pdf   %s\n",pdfname);
  printf("Diagnostic Level %d\n",Gdiag_no);

  return;
}
/*---------------------------------------------------------------*/
static int singledash(char *flag)
{
  int len;
  len = strlen(flag);
  if(len < 2) return(0);
  if(flag[0] == '-' && flag[1] != '-') return(1);
  return(0);
}
/*---------------------------------------------------------------*/
static int nth_is_arg(int nargc, char **argv, int nth)
{
  /* Checks that nth arg exists and is not a flag */
  /* nth is 0-based */
  /* check that there are enough args for nth to exist */
  if(nargc <= nth) return(0); 
  /* check whether the nth arg is a flag */
  if(isflag(argv[nth])) return(0);
  return(1);
}
/*------------------------------------------------------------*/
static int getfmtid(char *fname)
{
  int fmtid;
  fmtid = mri_identify(fname);
  if(fmtid == MRI_VOLUME_TYPE_UNKNOWN){
    printf("ERROR: cannot determine format of %s\n",fname);
    exit(1);
  }
  return(fmtid);
}

/*------------------------------------------------------------*/
static int checkfmt(char *fmt)
{
  int fmtid;

  if(fmt == NULL) return(MRI_VOLUME_TYPE_UNKNOWN);
  fmtid = string_to_type(fmt);
  if(fmtid == MRI_VOLUME_TYPE_UNKNOWN){
    printf("ERROR: format string %s unrecognized\n",fmt);
    exit(1);
  }
  return(fmtid);
}
/*---------------------------------------------------------------*/
static int isflag(char *flag)
{
  int len;
  len = strlen(flag);
  if(len < 2) return(0);

  if(flag[0] == '-' && flag[1] == '-') return(1);
  return(0);
}

/*---------------------------------------------------------------*/
MRI *fMRIsqrt(MRI *mri, MRI *mrisqrt)
{
  int c,r,s,f;
  double val;

  if(mrisqrt == NULL) mrisqrt = MRIcopy(mri,NULL);
  if(mrisqrt == NULL) return(NULL);

  for(c=0; c < mri->width; c++){
    for(r=0; r < mri->height; r++){
      for(s=0; s < mri->depth; s++){
	for(f=0; f < mri->nframes; f++){
	  val = MRIgetVoxVal(mri,c,r,s,f);
	  if(val < 0.0) val = 0;
	  else          val = sqrt(val);
	  MRIsetVoxVal(mri,c,r,s,f,val);
	}
      }
    }
  }
  return(mrisqrt);
}
