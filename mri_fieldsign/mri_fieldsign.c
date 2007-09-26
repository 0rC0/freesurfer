/**
 * @file  mri_fieldsign
 * @brief Computes retinotopic field sign
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: Douglas N. Greve
 * CVS Revision Info:
 *    $Author: greve $
 *    $Date: 2007/09/26 15:55:29 $
 *    $Revision: 1.1 $
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



/*!
\file mri_fieldsign.c
\brief Computes retinotopy field sign
\author Douglas Greve

*/


// $Id: mri_fieldsign.c,v 1.1 2007/09/26 15:55:29 greve Exp $

/*
  BEGINHELP

  ENDHELP
*/

/*
  BEGINUSAGE

  ENDUSAGE
*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
double round(double x);
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "macros.h"
#include "error.h"
#include "diag.h"
#include "utils.h"
#include "mrisurf.h"
#include "mrisutils.h"
#include "mri.h"
#include "mri2.h"
#include "fio.h"
#include "version.h"
#include "cmdargs.h"
#include "fsenv.h"
#include "retinotopy.h"

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);
MRI *SFA2MRI(MRI *eccen, MRI *polar);

int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_fieldsign.c,v 1.1 2007/09/26 15:55:29 greve Exp $";
char *Progname = NULL;
char *cmdline, cwd[2000];
int debug=0;
int checkoptsonly=0;
struct utsname uts;

char *FieldSignFile = NULL;
char *EccenSFAFile=NULL,*PolarSFAFile=NULL;
int DoSFA = 0;
char *subject, *hemi, *SUBJECTS_DIR;
char *PatchFile = NULL;
double fwhm = -1;
int nsmooth = -1;
char tmpstr[2000];

/*---------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  int nargs, err;
  MRIS *surf;
  MRI *eccensfa, *polarsfa, *mri, *mri2;

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
  if (argc == 0) usage_exit();
  parse_commandline(argc, argv);
  check_options();
  if (checkoptsonly) return(0);
  dump_options(stdout);

  SUBJECTS_DIR = getenv("SUBJECTS_DIR");
  if (SUBJECTS_DIR == NULL) {
    printf("ERROR: SUBJECTS_DIR not defined in environment\n");
    exit(1);
  }

  // Load the surface
  sprintf(tmpstr,"%s/%s/surf/%s.sphere",SUBJECTS_DIR,subject,hemi);
  printf("Reading %s\n",tmpstr);
  surf = MRISread(tmpstr);
  if(!surf) exit(1);

  // Load the patch
  if(PatchFile){
    sprintf(tmpstr,"%s/%s/surf/%s.%s",SUBJECTS_DIR,subject,hemi,PatchFile);
    printf("Reading %s\n",tmpstr);
    err = MRISreadPatchNoRemove(surf, tmpstr) ;
    if(err) exit(1);
  }

  if(DoSFA){
    eccensfa = MRIread(EccenSFAFile);
    if(eccensfa == NULL) exit(1);
    polarsfa = MRIread(PolarSFAFile);
    if(polarsfa == NULL) exit(1);
    mri = SFA2MRI(eccensfa, polarsfa);
    MRIfree(&eccensfa);
    MRIfree(&polarsfa);
  }

  if(fwhm > 0) {
    nsmooth = MRISfwhm2nitersSubj(fwhm,subject,hemi,"white");
    if(nsmooth == -1) exit(1);
    printf("Approximating gaussian smoothing of target with fwhm = %lf,\n"
           "with %d iterations of nearest-neighbor smoothing\n",
           fwhm,nsmooth);
  }

  if(nsmooth > 0){
    printf("Smoothing %d steps\n",nsmooth);
    mri2 = MRISsmoothMRI(surf, mri, nsmooth, NULL, NULL);
    MRIfree(&mri);
    mri = mri2;
  }

  MRIScopyMRI(surf, mri, 0, "val");    // eccen real
  MRIScopyMRI(surf, mri, 1, "val2");   // eccen imag
  MRIScopyMRI(surf, mri, 2, "valbak"); // polar real
  MRIScopyMRI(surf, mri, 3, "val2bak");// polar imag

  RETcompute_angles(surf);
  RETcompute_fieldsign(surf);

  mri2 = MRIcopyMRIS(NULL, surf, 0, "fieldsign");
  MRIwrite(mri2,FieldSignFile);

  return 0;
}
/*---------------------------------------------------------*/
static int parse_commandline(int argc, char **argv) {
  int  nargc , nargsused;
  char **pargv, *option ;

  if (argc < 1) usage_exit();

  nargc = argc;
  pargv = argv;
  while (nargc > 0) {

    option = pargv[0];
    if (debug) printf("%d %s\n",nargc,option);
    nargc -= 1;
    pargv += 1;

    nargsused = 0;

    if (!strcasecmp(option, "--help"))  print_help() ;
    else if (!strcasecmp(option, "--version")) print_version() ;
    else if (!strcasecmp(option, "--debug"))   debug = 1;
    else if (!strcasecmp(option, "--checkopts"))   checkoptsonly = 1;
    else if (!strcasecmp(option, "--nocheckopts")) checkoptsonly = 0;
    else if (!strcasecmp(option, "--occip")) PatchFile = "occip.patch.flat";

    else if (!strcasecmp(option, "--eccen-sfa")) {
      if (nargc < 1) CMDargNErr(option,1);
      EccenSFAFile = pargv[0];
      if(!fio_FileExistsReadable(EccenSFAFile)){
	printf("ERROR: cannot find %s\n",EccenSFAFile);
	exit(1);
      }
      DoSFA = 1;
      nargsused = 1;
    } else if (!strcasecmp(option, "--polar-sfa")) {
      if (nargc < 1) CMDargNErr(option,1);
      PolarSFAFile = pargv[0];
      if(!fio_FileExistsReadable(PolarSFAFile)){
	printf("ERROR: cannot find %s\n",PolarSFAFile);
	exit(1);
      }
      DoSFA = 1;
      nargsused = 1;
    } else if (!strcasecmp(option, "--s")) {
      if (nargc < 1) CMDargNErr(option,1);
      subject = pargv[0];
      nargsused = 1;
    } else if (!strcasecmp(option, "--hemi")) {
      if (nargc < 1) CMDargNErr(option,1);
      hemi = pargv[0];
      nargsused = 1;
    } else if (!strcasecmp(option, "--patch")) {
      if (nargc < 1) CMDargNErr(option,1);
      PatchFile = pargv[0];
      nargsused = 1;
    } else if (!strcasecmp(option, "--fwhm")) {
      if (nargc < 1) CMDargNErr(option,1);
      scanf(pargv[0],"%lf",&fwhm);
      nargsused = 1;
    } else if (!strcasecmp(option, "--nsmooth")) {
      if (nargc < 1) CMDargNErr(option,1);
      scanf(pargv[0],"%d",&nsmooth);
      nargsused = 1;
    } else if (!strcmp(option, "--sd")) {
      if (nargc < 1) CMDargNErr(option,1);
      FSENVsetSUBJECTS_DIR(pargv[0]);
      nargsused = 1;
    } else if (!strcmp(option, "--fs")) {
      if (nargc < 1) CMDargNErr(option,1);
      FieldSignFile = pargv[0];
      nargsused = 1;
    } else {
      fprintf(stderr,"ERROR: Option %s unknown\n",option);
      if (CMDsingleDash(option))
        fprintf(stderr,"       Did you really mean -%s ?\n",option);
      exit(-1);
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/*---------------------------------------------------------*/
static void usage_exit(void) {
print_usage() ;
  exit(1) ;
}
/*------------------------------------------------*/
static void print_usage(void) {
  printf("%s \n",Progname) ;
  printf("   --fs fieldsignfile : output\n");
  printf("\n");
  printf("   --eccen-sfa sfafile : eccen selfreqavg file \n");
  printf("   --polar-sfa sfafile : polar selfreqavg file \n");
  printf("   --s subject \n");
  printf("   --hemi hemi \n");
  printf("   --patch patchfile : without hemi \n");
  printf("   --occip : patchfile = occip.patch.flat\n");
  printf("   --fwhm fwhm_mm\n");
  printf("   --nsmooth nsmoothsteps\n");
  printf("   --\n");
  printf("\n");
  printf("   --debug     turn on debugging\n");
  printf("   --checkopts don't run anything, just check options and exit\n");
  printf("   --help      print out information on how to use this program\n");
  printf("   --version   print out version and exit\n");
  printf("\n");
  printf("%s\n", vcid) ;
  printf("\n");
}
/*--------------------------------------------------*/
static void print_help(void) {
  print_usage() ;
  printf("WARNING: this program is not yet tested!\n");
  exit(1) ;
}
/*--------------------------------------------------*/
static void print_version(void) {
  printf("%s\n", vcid) ;
  exit(1) ;
}
/*--------------------------------------------------*/
static void check_options(void) 
{
  if(!DoSFA){
    printf("Need SFA files\n");
    exit(1);
  }
  if(FieldSignFile == NULL){
    printf("Need output field sign file\n");
    exit(1);
  }
  if(subject == NULL){
    printf("Need subject\n");
    exit(1);
  }
  if(hemi == NULL){
    printf("Need hemi\n");
    exit(1);
  }
  if(fwhm > 0 && nsmooth > 0){
    printf("Cannot --fwhm and --nsmooth\n");
    exit(1);
  }
  return;
}
/*--------------------------------------------------*/
static void dump_options(FILE *fp) {
  fprintf(fp,"\n");
  fprintf(fp,"%s\n",vcid);
  fprintf(fp,"cwd %s\n",cwd);
  fprintf(fp,"cmdline %s\n",cmdline);
  fprintf(fp,"sysname  %s\n",uts.sysname);
  fprintf(fp,"hostname %s\n",uts.nodename);
  fprintf(fp,"machine  %s\n",uts.machine);
  fprintf(fp,"user     %s\n",VERuser());
  fprintf(fp,"eccen-sfa  %s\n",EccenSFAFile);
  fprintf(fp,"polar-sfa  %s\n",PolarSFAFile);
  fprintf(fp,"patch     %s\n",PatchFile);
  fprintf(fp,"subject     %s\n",subject);
  fprintf(fp,"hemi        %s\n",hemi);
  fprintf(fp,"fwhm        %lf\n",fwhm);
  fprintf(fp,"nsmooth     %d\n",nsmooth);
  fprintf(fp,"fieldsign   %s\n",FieldSignFile);
  return;
}
/*--------------------------------------------------------------------
  SFA2MRI(MRI *eccen, MRI *polar) - pack two SFAs int a single MRI. An
  SFA is the output of selfreqavg. Must be sampled to the surface. 
  ----------------------------------------------------------------*/
MRI *SFA2MRI(MRI *eccen, MRI *polar)
{
  MRI *mri;
  int c,r,s;
  double v;

  mri = MRIallocSequence(eccen->width, eccen->height, eccen->depth, 
			 MRI_FLOAT, 4);
  MRIcopyHeader(eccen,mri);

  for(c=0; c < eccen->width; c++){
    for(r=0; r < eccen->height; r++){
      for(s=0; s < eccen->depth; s++){

	v = MRIgetVoxVal(eccen,c,r,s,1); // eccen-real
	MRIsetVoxVal(mri,c,r,s,0, v);

	v = MRIgetVoxVal(eccen,c,r,s,2); // eccen-imag
	MRIsetVoxVal(mri,c,r,s,1, v);

	v = MRIgetVoxVal(polar,c,r,s,1); // polar-real
	MRIsetVoxVal(mri,c,r,s,2, v);

	v = MRIgetVoxVal(polar,c,r,s,2); // polar-imag
	MRIsetVoxVal(mri,c,r,s,3, v);

      }
    }
  }

  return(mri);
}
