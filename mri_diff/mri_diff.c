/**
 * @file  mri_diff.c
 * @brief Determines whether two volumes differ.
 *
 * The basic usage is something like:
 *
 *   mri_diff vol1 vol2
 *
 * It then prints to the terminal whether they differ or not.
 *
 * Volumes can differ in six ways:
 * 1. Dimension,               return status = 101
 * 2. Resolutions,             return status = 102
 * 3. Acquisition Parameters,  return status = 103
 * 4. Geometry,                return status = 104
 * 5. Precision,               return status = 105
 * 6. Pixel Data,              return status = 106
 */
/*
 * Original Author: Doug Greve
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2007/01/11 17:40:39 $
 *    $Revision: 1.17 $
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


// mri_voldiff v1 v2
//
/*
  BEGINHELP

  Determines whether two volumes differ. See below for what
  'differ' means.

  The basic usage is something like:

  mri_diff vol1 vol2

  It then prints to the terminal whether they differ or not.

  NOTE: stuff might get printed to the terminal regardless of whether
  the volumes are different or not, so you cannot just use the
  presence or absence of terminal output to determine whether they
  are different as you can with unix diff.

  There are three ways to determine whether they are different:
  1. Look for 'volumes differ' in the terminal output
  2. Program exits with status > 100
  3. Create a log file

  To create a log file, add --log yourlogfile any where in the
  command line, eg:

  mri_diff vol1 vol2 --log yourlogfile

  If yourlogfile exists, it will be immediately deleted when the
  program starts. If a difference is detected, yourlogfile will
  be created, and information about the difference will be
  printed to it. If there is no difference, yourlogfile will
  not exist when the program finishes.

  Volumes can differ in six ways:
  1. Dimension,               return status = 101
  2. Resolutions,             return status = 102
  3. Acquisition Parameters,  return status = 103
  4. Geometry,                return status = 104
  5. Precision,               return status = 105
  6. Pixel Data,              return status = 106

  Dimension is number of rows, cols, slices, and frames.
  Resolution is voxel size.
  Acqusition parameters are: flip angle, TR, TE, and TI.
  Geometry checks the vox2ras matrices for equality.
  Precision is int, float, short, etc.

  By default, all of these are checked, but some can be turned off
  with certain command-line flags:

  --notallow-res  : turns off resolution checking
  --notallow-acq  : turns off acquistion parameter checking
  --notallow-geo  : turns off geometry checking
  --notallow-prec : turns off precision checking
  --notallow-pix  : turns off pixel checking

  In addition, the minimum difference in pixel value required
  to be considered different can be controlled with --thresh.
  Eg, if two volumes differ by .00001, and you do not consider
  this to be significant, then --thresh .00002 will prevent
  that difference from being considered different. The default
  threshold is 0.

  --diff diffvol

  Saves difference image to diffvol.

  QUALITY ASSURANCE

  mri_diff can be used to check that two volumes where acquired in the
  same way with the --qa flag. This turns on Res, Acq, and Prec, and
  turns off Geo and Pix. Instead of checking geometry, it checks for the
  basic orientation of the volumes (eg, RAS, LPI, etc). The idea here is
  that the pixel data and exact geometry may be different, but other
  things should be the same.

  EXIT STATUS

  0   Volumes are not different and there were no errors
  1   Errors encountered. Volumes may or may not be different
  101 Volumes differ in dimension
  102 Volumes differ in resolution
  103 Volumes differ in acquisition parameters
  104 Volumes differ in geometry
  105 Volumes differ in precision
  106 Volumes differ in pixel data
  107 Volumes differ in orientation

  ENDHELP
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
#include "pdf.h"
#include "fsgdf.h"
#include "timer.h"
#include "matfile.h"
#include "volcluster.h"
#include "surfcluster.h"


static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);
int main(int argc, char *argv[]) ;

static char vcid[] = "$Id: mri_diff.c,v 1.17 2007/01/11 17:40:39 nicks Exp $";
char *Progname = NULL;
char *cmdline, cwd[2000];
int debug=0;
int checkoptsonly=0;
struct utsname uts;

char *InVol1File=NULL;
char *InVol2File=NULL;
char *subject, *hemi, *SUBJECTS_DIR;
double pixthresh=0, resthresh=0, geothresh=0;
char *DiffFile=NULL;
int DiffAbs=0;

MRI *InVol1=NULL, *InVol2=NULL, *DiffVol=NULL;
char *DiffVolFile=NULL;

int CheckResolution=1;
int CheckAcqParams=1;
int CheckPixVals=1;
int CheckGeo=1;
int CheckOrientation=1;
int CheckPrecision=1;
MATRIX *vox2ras1,*vox2ras2;
char Orient1[4], Orient2[4];

int ExitOnDiff = 1;
int ExitStatus = 0;

/*---------------------------------------------------------------*/
int main(int argc, char *argv[]) {
  int nargs, r, c, s, f;
  int rmax, cmax, smax, fmax;
  double diff,maxdiff;
  double val1, val2;
  FILE *fp=NULL;

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

  if (debug) dump_options(stdout);

  //  SUBJECTS_DIR = getenv("SUBJECTS_DIR");
  //  if(SUBJECTS_DIR == NULL){
  //    printf("ERROR: SUBJECTS_DIR not defined in environment\n");
  //    exit(1);
  //  }
  if (DiffFile) {
    if (fio_FileExistsReadable(DiffFile)) unlink(DiffFile);
    if (fio_FileExistsReadable(DiffFile)) {
      printf("ERROR: could not delete %s\n",DiffFile);
      exit(1);
    }
  }

  InVol1 = MRIread(InVol1File);
  if (InVol1==NULL) exit(1);

  InVol2 = MRIread(InVol2File);
  if (InVol2==NULL) exit(1);

  /*- Check dimension ---------------------------------*/
  if (InVol1->width   != InVol2->width  ||
      InVol1->height  != InVol2->height ||
      InVol1->depth   != InVol2->depth  ||
      InVol1->nframes != InVol2->nframes) {
    printf("Volumes differ in dimension\n");
    printf("v1dim %d %d %d %d\n",
           InVol1->width,InVol1->height,InVol1->depth,InVol1->nframes);
    printf("v2dim %d %d %d %d\n",
           InVol2->width,InVol2->height,InVol2->depth,InVol2->nframes);
    if (DiffFile) {
      fp = fopen(DiffFile,"w");
      if (fp==NULL) {
        printf("ERROR: could not open %s\n",DiffFile);
        exit(1);
      }
      dump_options(fp);
      fprintf(fp,"v1dim %d %d %d %d\n",
              InVol1->width,InVol1->height,InVol1->depth,InVol1->nframes);
      fprintf(fp,"v2dim %d %d %d %d\n",
              InVol2->width,InVol2->height,InVol2->depth,InVol2->nframes);
      fprintf(fp,"Volumes differ in dimension\n");
      fclose(fp);
    }
    // Must exit here
    exit(101);
  }

  //---------------------------------------------------
  if (CheckResolution) {
    if (fabs(InVol1->xsize - InVol2->xsize) > resthresh  ||
        fabs(InVol1->ysize - InVol2->ysize) > resthresh  ||
        fabs(InVol1->zsize - InVol2->zsize) > resthresh) {
      printf("Volumes differ in resolution\n");
      printf("v1res %f %f %f\n",
             InVol1->xsize,InVol1->ysize,InVol1->zsize);
      printf("v2res %f %f %f\n",
             InVol2->xsize,InVol2->ysize,InVol2->zsize);
      printf("diff %f %f %f\n",
             InVol1->xsize-InVol2->xsize,
             InVol1->ysize-InVol2->ysize,
             InVol1->zsize-InVol2->zsize);
      if (DiffFile) {
        fp = fopen(DiffFile,"w");
        if (fp==NULL) {
          printf("ERROR: could not open %s\n",DiffFile);
          exit(1);
        }
        dump_options(fp);
        fprintf(fp,"v1res %f %f %f\n",
                InVol1->xsize,InVol1->ysize,InVol1->zsize);
        fprintf(fp,"v2res %f %f %f\n",
                InVol2->xsize,InVol2->ysize,InVol2->zsize);
        fprintf(fp,"Volumes differ in resolution\n");
        fclose(fp);
      }
      ExitStatus = 102;
      if (ExitOnDiff) exit(ExitStatus);
    }
  }

  //---------------------------------------------------
  if (CheckAcqParams) {
    if (InVol1->flip_angle   != InVol2->flip_angle ||
        InVol1->tr   != InVol2->tr ||
        InVol1->te   != InVol2->te ||
        InVol1->ti   != InVol2->ti) {
      printf("Volumes differ in acquisition parameters\n");
      printf("v1acq fa=%f tr=%f te=%f ti=%f\n",
             InVol1->flip_angle,InVol1->tr,InVol1->te,InVol1->ti);
      printf("v2acq fa=%f tr=%f te=%f ti=%f\n",
             InVol2->flip_angle,InVol2->tr,InVol2->te,InVol2->ti);
      if (DiffFile) {
        fp = fopen(DiffFile,"w");
        if (fp==NULL) {
          printf("ERROR: could not open %s\n",DiffFile);
          exit(1);
        }
        dump_options(fp);
        fprintf(fp,"v1acq fa=%f tr=%f te=%f ti=%f\n",
                InVol1->flip_angle,InVol1->tr,InVol1->te,InVol1->ti);
        fprintf(fp,"v2acq fa=%f tr=%f te=%f ti=%f\n",
                InVol2->flip_angle,InVol2->tr,InVol2->te,InVol2->ti);
        fprintf(fp,"Volumes differ in acquisition parameters\n");
        fclose(fp);
      }
      ExitStatus = 103;
      if (ExitOnDiff) exit(103);
    }
  }

  //------------------------------------------------------
  if (CheckGeo) {
    vox2ras1 = MRIxfmCRS2XYZ(InVol1,0);
    vox2ras2 = MRIxfmCRS2XYZ(InVol2,0);
    for (r=1; r<=4; r++) {
      for (c=1; c<=4; c++) {
        val1 = vox2ras1->rptr[r][c];
        val2 = vox2ras2->rptr[r][c];
        diff = fabs(val1-val2);
        if (diff > geothresh) {
          printf("Volumes differ in geometry %d %d %lf\n",
                 r,c,diff);
          if (DiffFile) {
            fp = fopen(DiffFile,"w");
            if (fp==NULL) {
              printf("ERROR: could not open %s\n",DiffFile);
              exit(1);
            }
            dump_options(fp);
            fprintf(fp,"v1 vox2ras ----------\n");
            MatrixPrint(fp,vox2ras1);
            fprintf(fp,"v2 vox2ras ---------\n");
            MatrixPrint(fp,vox2ras2);

            fprintf(fp,"Volumes differ in geometry vox2ras r=%d c=%d %lf\n",
                    r,c,diff);
            fclose(fp);
          }
          ExitStatus = 104;
          if (ExitOnDiff) exit(104);
        }
      } // c
    } // r
  } // checkgeo

  //-------------------------------------------------------
  if (CheckPrecision) {
    if (InVol1->type != InVol2->type) {
      printf("Volumes differ in precision %d %d\n",
             InVol1->type,InVol2->type);
      if (DiffFile) {
        fp = fopen(DiffFile,"w");
        if (fp==NULL) {
          printf("ERROR: could not open %s\n",DiffFile);
          exit(1);
        }
        dump_options(fp);
        fprintf(fp,"Volumes differ in precision %d %d\n",
                InVol1->type,InVol2->type);

        fclose(fp);
      }
      ExitStatus = 105;
      if (ExitOnDiff) exit(105);
    }
  }

  //------------------------------------------------------
  // Compare pixel values
  if (CheckPixVals) {
    if (DiffVolFile) {
      DiffVol = MRIallocSequence(InVol1->width,InVol1->height,
                                 InVol1->depth,MRI_FLOAT,InVol1->nframes);
      MRIcopyHeader(InVol1,DiffVol);
    }
    c=r=s=f=0;
    val1 = MRIgetVoxVal(InVol1,c,r,s,f);
    val2 = MRIgetVoxVal(InVol2,c,r,s,f);
    maxdiff = fabs(val1-val2);
    cmax=rmax=smax=fmax=0;
    for (c=0; c < InVol1->width; c++) {
      for (r=0; r < InVol1->height; r++) {
        for (s=0; s < InVol1->depth; s++) {
          for (f=0; f < InVol1->nframes; f++) {
            val1 = MRIgetVoxVal(InVol1,c,r,s,f);
            val2 = MRIgetVoxVal(InVol2,c,r,s,f);
            if (! DiffAbs) diff = fabs(val1-val2);
            else diff = fabs(fabs(val1)-fabs(val2));
            if (DiffVolFile) MRIsetVoxVal(DiffVol,c,r,s,f,val1-val2);
            if (maxdiff < diff) {
              maxdiff = diff;
              cmax = c;
              rmax = r;
              smax = s;
              fmax = f;
            }
          }
        }
      }
    }
    if (debug) printf("maxdiff %f at %d %d %d %d\n",
                        maxdiff,cmax,rmax,smax,fmax);
    if (DiffVolFile) MRIwrite(DiffVol,DiffVolFile);

    if (maxdiff > pixthresh) {
      printf("Volumes differ in pixel data\n");
      printf("maxdiff %f at %d %d %d %d\n",
             maxdiff,cmax,rmax,smax,fmax);
      if (DiffFile) {
        fp = fopen(DiffFile,"w");
        if (fp==NULL) {
          printf("ERROR: could not open %s\n",DiffFile);
          exit(1);
        }
        dump_options(fp);
        fprintf(fp,"maxdiff %f at %d %d %d %d\n",maxdiff,cmax,rmax,smax,fmax);
        fprintf(fp,"Volumes differ in pixel value\n");
        fclose(fp);
      }
      ExitStatus = 106;
      if (ExitOnDiff) exit(106);
    }
  }

  //----------------------------------------------------------
  if (CheckOrientation) {
    MRIdircosToOrientationString(InVol1,Orient1);
    MRIdircosToOrientationString(InVol2,Orient2);
    if (strcmp(Orient1,Orient2)) {
      printf("Volumes differ in orientation %s %s\n",
             Orient1,Orient2);
      if (DiffFile) {
        fp = fopen(DiffFile,"w");
        if (fp==NULL) {
          printf("ERROR: could not open %s\n",DiffFile);
          exit(1);
        }
        dump_options(fp);
        fprintf(fp,"Volumes differ in orientation %s %s\n",
                Orient1,Orient2);
        fclose(fp);
      }
      ExitStatus = 107;
      if (ExitOnDiff) exit(107);
    }
  }

  exit(ExitStatus);
  return(ExitStatus);
}
/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv) {
  int  nargc , nargsused;
  char **pargv, *option ;

  if (argc < 1) usage_exit();

  nargc   = argc;
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
    else if (!strcasecmp(option, "--no-exit-on-diff")) ExitOnDiff = 0;
    else if (!strcasecmp(option, "--no-exit-on-error")) ExitOnDiff = 0;

    else if (!strcasecmp(option, "--notallow-res"))  CheckResolution = 0;
    else if (!strcasecmp(option, "--notallow-acq"))  CheckAcqParams = 0;
    else if (!strcasecmp(option, "--notallow-geo"))  CheckGeo = 0;
    else if (!strcasecmp(option, "--notallow-prec")) CheckPrecision = 0;
    else if (!strcasecmp(option, "--notallow-pix"))  CheckPixVals = 0;
    else if (!strcasecmp(option, "--notallow-ori"))  CheckOrientation = 0;
    else if (!strcasecmp(option, "--diffabs"))  DiffAbs = 1;
    else if (!strcasecmp(option, "--qa")) {
      CheckPixVals = 0;
      CheckGeo     = 0;
    } else if (!strcasecmp(option, "--pix-only")) {
      CheckPixVals = 1;
      CheckResolution = 0;
      CheckAcqParams = 0;
      CheckGeo     = 0;
      CheckPrecision = 0;
      CheckOrientation = 0;
    } else if (!strcasecmp(option, "--v1")) {
      if (nargc < 1) CMDargNErr(option,1);
      InVol1File = pargv[0];
      nargsused = 1;
    } else if (!strcasecmp(option, "--v2")) {
      if (nargc < 1) CMDargNErr(option,1);
      InVol2File = pargv[0];
      nargsused = 1;
    } else if (!strcasecmp(option, "--thresh")) {
      if (nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%lf",&pixthresh);
      nargsused = 1;
    } else if (!strcasecmp(option, "--res-thresh")) {
      if (nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%lf",&resthresh);
      CheckResolution=1;
      nargsused = 1;
    } else if (!strcasecmp(option, "--geo-thresh")) {
      if (nargc < 1) CMDargNErr(option,1);
      sscanf(pargv[0],"%lf",&geothresh);
      CheckResolution=1;
      nargsused = 1;
    } else if (!strcasecmp(option, "--diff-file") ||
               !strcasecmp(option, "--log")) {
      if (nargc < 1) CMDargNErr(option,1);
      DiffFile = pargv[0];
      nargsused = 1;
    } else if (!strcasecmp(option, "--diff")) {
      if (nargc < 1) CMDargNErr(option,1);
      DiffVolFile = pargv[0];
      nargsused = 1;
    } else {
      if (InVol1File == NULL)      InVol1File = option;
      else if (InVol2File == NULL) InVol2File = option;
      else {
        fprintf(stderr,"ERROR: Option %s unknown\n",option);
        if (CMDsingleDash(option))
          fprintf(stderr,"       Did you really mean -%s ?\n",option);
        exit(-1);
      }
    }
    nargc -= nargsused;
    pargv += nargsused;
  }
  return(0);
}
/* --------------------------------------------- */
static void print_version(void) {
  printf("%s\n", vcid) ;
  exit(1) ;
}
/* --------------------------------------------- */
static void check_options(void) {
  if (InVol1File==NULL) {
    printf("ERROR: need to spec volume 1\n");
    exit(1);
  }
  if (InVol2File==NULL) {
    printf("ERROR: need to spec volume 2\n");
    exit(1);
  }
  return;
}
/* ------------------------------------------------------ */
static void usage_exit(void) {
  print_usage() ;
  exit(1) ;
}
/* --------------------------------------------- */
static void dump_options(FILE *fp) {
  fprintf(fp,"\n");
  fprintf(fp,"%s\n",vcid);
  fprintf(fp,"%s\n",Progname);
  fprintf(fp,"FREESURFER_HOME %s\n",getenv("FREESURFER_HOME"));
  fprintf(fp,"cwd       %s\n",cwd);
  fprintf(fp,"cmdline   %s\n",cmdline);
  fprintf(fp,"timestamp %s\n",VERcurTimeStamp());
  fprintf(fp,"sysname   %s\n",uts.sysname);
  fprintf(fp,"hostname  %s\n",uts.nodename);
  fprintf(fp,"machine   %s\n",uts.machine);
  fprintf(fp,"user      %s\n",VERuser());
  fprintf(fp,"v1        %s\n",InVol1File);
  fprintf(fp,"v2        %s\n",InVol2File);
  fprintf(fp,"pixthresh %lf\n",pixthresh);
  fprintf(fp,"checkres  %d\n",CheckResolution);
  fprintf(fp,"checkacq  %d\n",CheckAcqParams);
  fprintf(fp,"checkpix  %d\n",CheckPixVals);
  fprintf(fp,"checkgeo  %d\n",CheckGeo);
  fprintf(fp,"checkori  %d\n",CheckOrientation);
  fprintf(fp,"checkprec %d\n",CheckPrecision);
  fprintf(fp,"diffabs   %d\n",DiffAbs);
  fprintf(fp,"logfile   %s\n",DiffFile);
  return;
}
/* --------------------------------------------- */
static void print_usage(void) {
  printf("USAGE: %s <options> vol1file vol2file <options> \n",Progname) ;
  printf("\n");
  //printf("   --v1 volfile1 : first  input volume \n");
  //printf("   --v2 volfile2 : second input volume \n");
  printf("\n");
  printf("   --notallow-res  : do not check for resolution diffs\n");
  printf("   --notallow-acq  : do not check for acq param diffs\n");
  printf("   --notallow-geo  : do not check for geometry diffs\n");
  printf("   --notallow-prec : do not check for precision diffs\n");
  printf("   --notallow-pix  : do not check for pixel diffs\n");
  printf("   --notallow-ori  : do not check for orientation diffs\n");
  printf("   --no-exit-on-diff : do not exit on diff "
         "(runs thru everything)\n");
  printf("\n");
  printf("   --qa         : check res, acq, precision, "
         "and orientation only\n");
  printf("   --pix-only   : only check pixel data\n");
  printf("   --diffabs    : take abs before computing diff\n");
  printf("\n");
  printf("   --thresh thresh : pix diffs must be greater than this \n");
  printf("   --log DiffFile : store diff info in this file. \n");
  printf("   --diff DiffVol : save difference image. \n");
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
static void print_help(void) {
  print_usage() ;

  printf("\n");
  printf("Determines whether two volumes differ. See below for what \n");
  printf("'differ' means. \n");
  printf("\n");
  printf("The basic usage is something like:\n");
  printf("\n");
  printf("mri_diff vol1 vol2\n");
  printf("\n");
  printf("It then prints to the terminal whether they differ or not.\n");
  printf("\n");
  printf("NOTE: stuff might get printed to the terminal regardless "
         "of whether\n");
  printf("the volumes are different or not, so you cannot just use the \n");
  printf("presence or absence of terminal output to determine whether they\n");
  printf("are different as you can with unix diff.\n");
  printf("\n");
  printf("There are three ways to determine whether they are different:\n");
  printf("1. Look for 'volumes differ' in the terminal output\n");
  printf("2. Program exits with status > 100\n");
  printf("3. Create a log file\n");
  printf("\n");
  printf("To create a log file, add --log yourlogfile any where in the\n");
  printf("command line, eg:\n");
  printf("\n");
  printf("mri_diff vol1 vol2 --log yourlogfile\n");
  printf("\n");
  printf("If yourlogfile exists, it will be immediately deleted when the \n");
  printf("program starts. If a difference is detected, yourlogfile will\n");
  printf("be created, and information about the difference will be\n");
  printf("printed to it. If there is no difference, yourlogfile will\n");
  printf("not exist when the program finishes.\n");
  printf("\n");
  printf("Volumes can differ in six ways:\n");
  printf("  1. Dimension,               return status = 101\n");
  printf("  2. Resolutions,             return status = 102\n");
  printf("  3. Acquisition Parameters,  return status = 103\n");
  printf("  4. Geometry,                return status = 104\n");
  printf("  5. Precision,               return status = 105\n");
  printf("  6. Pixel Data,              return status = 106\n");
  printf("\n");
  printf("Dimension is number of rows, cols, slices, and frames.\n");
  printf("Resolution is voxel size.\n");
  printf("Acqusition parameters are: flip angle, TR, TE, and TI.\n");
  printf("Geometry checks the vox2ras matrices for equality.\n");
  printf("Precision is int, float, short, etc.\n");
  printf("\n");
  printf("By default, all of these are checked, but some can be turned off\n");
  printf("with certain command-line flags:\n");
  printf("\n");
  printf("--notallow-res  : turns off resolution checking\n");
  printf("--notallow-acq  : turns off acquistion parameter checking\n");
  printf("--notallow-geo  : turns off geometry checking\n");
  printf("--notallow-prec : turns off precision checking\n");
  printf("--notallow-pix  : turns off pixel checking\n");
  printf("\n");
  printf("In addition, the minimum difference in pixel value required\n");
  printf("to be considered different can be controlled with --thresh.\n");
  printf("Eg, if two volumes differ by .00001, and you do not consider\n");
  printf("this to be significant, then --thresh .00002 will prevent\n");
  printf("that difference from being considered different. The default\n");
  printf("threshold is 0.\n");
  printf("\n");
  printf("--diff diffvol\n");
  printf("\n");
  printf("Saves difference image to diffvol.\n");
  printf("\n");
  printf("QUALITY ASSURANCE\n");
  printf("\n");
  printf("mri_diff can be used to check that two volumes where "
         "acquired in the\n");
  printf("same way with the --qa flag. This turns on Res, Acq, and "
         "Prec, and\n");
  printf("turns off Geo and Pix. Instead of checking geometry, it checks "
         "for the\n");
  printf("basic orientation of the volumes (eg, RAS, LPI, etc). The idea "
         "here is\n");
  printf("that the pixel data and exact geometry may be different, "
         "but other\n");
  printf("things should be the same.\n");
  printf("\n");
  printf("EXIT STATUS\n");
  printf("\n");
  printf("0   Volumes are not different and there were no errors\n");
  printf("1   Errors encounted. Volumes may or may not be different\n");
  printf("101 Volumes differ in dimension\n");
  printf("102 Volumes differ in resolution\n");
  printf("103 Volumes differ in acquisition parameters\n");
  printf("104 Volumes differ in geometry\n");
  printf("105 Volumes differ in precision\n");
  printf("106 Volumes differ in pixel data\n");
  printf("107 Volumes differ in orientation\n");
  printf("\n");

  exit(1) ;
}
