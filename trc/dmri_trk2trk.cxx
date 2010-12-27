/**
 * @file  dmri_trk2trk.c
 * @brief Apply affine and non-linear warp to streamlines in .trk file
 *
 * Apply affine and non-linear warp to streamlines in .trk file
 */
/*
 * Original Author: Anastasia Yendiki
 * CVS Revision Info:
 *    $Author: ayendiki $
 *    $Date: 2010/12/27 17:49:17 $
 *    $Revision: 1.3 $
 *
 * Copyright (C) 2010
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

#include "vial.h"	// Needs to be included first because of CVS libs
#include "TrackIO.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
double round(double x);
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <float.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "error.h"
#include "diag.h"
#include "mri.h"
#include "fio.h"
#include "version.h"
#include "cmdargs.h"
#include "timer.h"

using namespace std;

static int  parse_commandline(int argc, char **argv);
static void check_options(void);
static void print_usage(void) ;
static void usage_exit(void);
static void print_help(void) ;
static void print_version(void) ;
static void dump_options(FILE *fp);

int debug = 0, checkoptsonly = 0;

int main(int argc, char *argv[]) ;

static char vcid[] = "";
const char *Progname = "dmri_track";

int nin = 0, nout = 0, nvol = 0;
char *inFile[100], *outFile[100], *outVolFile[100],
     *inRefFile = NULL, *outRefFile = NULL,
     *affineXfmFile = NULL, *nonlinXfmFile = NULL;

struct utsname uts;
char *cmdline, cwd[2000];

struct timeb cputimer;

/*--------------------------------------------------*/
int main(int argc, char **argv) {
  int nargs, cputime;
  vector<float> point(3);
  MRI *inref = 0, *outref = 0, *outvol = 0;
  AffineReg affinereg;
#ifndef NO_CVS_UP_IN_HERE
  NonlinReg nonlinreg;
#endif

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, vcid, "$Name:  $");
  if (nargs && argc - nargs == 1) exit (0);
  argc -= nargs;
  cmdline = argv2cmdline(argc,argv);
  uname(&uts);
  getcwd(cwd, 2000);

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

  // Read reference volumes
  inref = MRIread(inRefFile);
  outref = MRIread(outRefFile);

  if (nvol > 0)
    outvol = MRIclone(outref, NULL);

  // Read transform files
#ifndef NO_CVS_UP_IN_HERE
  if (nonlinXfmFile) {
    if (affineXfmFile)
      affinereg.ReadXfm(affineXfmFile, inref, 0);
    nonlinreg.ReadXfm(nonlinXfmFile, outref);
  }
#endif
  if (affineXfmFile)
    affinereg.ReadXfm(affineXfmFile, inref, outref);

  for (int itrk = 0; itrk < nout; itrk++) {
    int npts;
    CTrackReader trkreader;
    CTrackWriter trkwriter;
    TRACK_HEADER trkheadin, trkheadout;

    printf("Processing .trk file %d of %d...\n", itrk+1, nout);
    TimerStart(&cputimer);

    // Open input .trk file
    if (!trkreader.Open(inFile[itrk], &trkheadin)) {
      cout << "ERROR: Cannot open input " << inFile[itrk] << endl;
      cout << trkreader.GetLastErrorMessage() << endl;
      exit(1);
    }

    if (nout > 0) {
      // Set output .trk header
      trkheadout = trkheadin;
      trkheadout.voxel_size[0] = outref->xsize;
      trkheadout.voxel_size[1] = outref->ysize;
      trkheadout.voxel_size[2] = outref->zsize;

      // Open output .trk file
      if (!trkwriter.Initialize(outFile[itrk], trkheadout)) {
        cout << "ERROR: Cannot open output " << outFile[itrk] << endl;
        cout << trkwriter.GetLastErrorMessage() << endl;
        exit(1);
      }
    }

    if (nvol > 0)
      MRIclear(outvol);

    while (trkreader.GetNextPointCount(&npts)) {
      float *iraw, *rawpts = new float[npts*3];

      // Read a streamline from input file
      trkreader.GetNextTrackData(npts, rawpts);

      iraw = rawpts;
      for (int ipt = npts; ipt > 0; ipt--) {
        // Divide by input voxel size to get voxel coords
        for (int k = 0; k < 3; k++) {
          point[k] = *iraw / trkheadin.voxel_size[k];
          iraw++;
        }

        // Apply affine transform
        if (!affinereg.IsEmpty())
          affinereg.ApplyXfm(point, point.begin());

#ifndef NO_CVS_UP_IN_HERE
        // Apply nonlinear transform
        if (!nonlinreg.IsEmpty())
          nonlinreg.ApplyXfm(point, point.begin());
#endif

        // Write transformed point to volume
        if (nvol > 0) {
          int ix = (int) round(point[0]),
              iy = (int) round(point[1]),
              iz = (int) round(point[2]);

          if (ix < 0)			ix = 0;
          if (ix >= outvol->width)	ix = outvol->width-1;
          if (iy < 0)			iy = 0;
          if (iy >= outvol->height)	iy = outvol->height-1;
          if (iz < 0)			iz = 0;
          if (iz >= outvol->depth)	iz = outvol->depth-1;

          MRIsetVoxVal(outvol, ix, iy, iz, 0,
                       MRIgetVoxVal(outvol, ix, iy, iz, 0) + 1);
        }

        // Multiply back by output voxel size
        iraw -= 3;
        for (int k = 0; k < 3; k++) {
          *iraw = point[k] * trkheadout.voxel_size[k];
          iraw++;
        }
      }

      // Write transformed streamline to .trk file
      if (nout > 0)
        trkwriter.WriteNextTrack(npts, rawpts);

      delete[] rawpts;
    }

    if (nout > 0)
      trkwriter.Close();

    if (nvol > 0)
      MRIwrite(outvol, outVolFile[itrk]);

    cputime = TimerStop(&cputimer);
    printf("Done in %g sec.\n", cputime/1000.0);
  }

  printf("dmri_trk2trk done\n");
  return(0);
  exit(0);
}

/* --------------------------------------------- */
static int parse_commandline(int argc, char **argv) {
  int  nargc, nargsused;
  char **pargv, *option;

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
    else if (!strcmp(option, "--in")) {
      if (nargc < 1) CMDargNErr(option,1);
      nargsused = 0;
      while (strncmp(pargv[nargsused], "--", 2)) {
        inFile[nin] = fio_fullpath(pargv[nargsused]);
        nargsused++;
        nin++;
      }
    } 
    else if (!strcmp(option, "--out")) {
      if (nargc < 1) CMDargNErr(option,1);
      nargsused = 0;
      while (strncmp(pargv[nargsused], "--", 2)) {
        outFile[nout] = fio_fullpath(pargv[nargsused]);
        nargsused++;
        nout++;
      }
    } 
    else if (!strcmp(option, "--outvol")) {
      if (nargc < 1) CMDargNErr(option,1);
      nargsused = 0;
      while (strncmp(pargv[nargsused], "--", 2)) {
        outVolFile[nvol] = fio_fullpath(pargv[nargsused]);
        nargsused++;
        nvol++;
      }
    } 
    else if (!strcmp(option, "--inref")) {
      if (nargc < 1) CMDargNErr(option,1);
      inRefFile = fio_fullpath(pargv[0]);
      nargsused = 1;
    } 
    else if (!strcmp(option, "--outref")) {
      if (nargc < 1) CMDargNErr(option,1);
      outRefFile = fio_fullpath(pargv[0]);
      nargsused = 1;
    } 
    else if (!strcmp(option, "--reg")) {
      if (nargc < 1) CMDargNErr(option,1);
      affineXfmFile = fio_fullpath(pargv[0]);
      nargsused = 1;
    }
    else if (!strcmp(option, "--regnl")) {
      if (nargc < 1) CMDargNErr(option,1);
      nonlinXfmFile = fio_fullpath(pargv[0]);
      nargsused = 1;
    }
    else {
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

/* --------------------------------------------- */
static void print_usage(void) 
{
  printf("\n");
  printf("USAGE: ./dmri_trk2trk\n");
  printf("\n");
  printf("Basic inputs\n");
  printf("   --in  <file> [...]: input .trk file(s)\n");
  printf("   --out <file> [...]: output .trk file(s), as many as inputs\n");
  printf("   --outvol <file> [...]: output volume(s), as many as inputs\n");
  printf("   --inref  <file>: input reference volume\n");
  printf("   --outref <file>: output reference volume\n");
  printf("   --reg   <file>: affine registration (.mat), applied first\n");
  printf("   --regnl <file>: nonlinear registration (.tm3d), applied second\n");
  printf("\n");
  printf("Other options\n");
  printf("   --debug:     turn on debugging\n");
  printf("   --checkopts: don't run anything, just check options and exit\n");
  printf("   --help:      print out information on how to use this program\n");
  printf("   --version:   print out version and exit\n");
  printf("\n");
}

/* --------------------------------------------- */
static void print_help(void) {
  print_usage() ;
  printf("\n");
  printf("...\n");
  printf("\n");
  exit(1) ;
}

/* ------------------------------------------------------ */
static void usage_exit(void) {
  print_usage() ;
  exit(1) ;
}

/* --------------------------------------------- */
static void print_version(void) {
  printf("%s\n", vcid) ;
  exit(1) ;
}

/* --------------------------------------------- */
static void check_options(void) {
  if(nin == 0) {
    printf("ERROR: must specify input .trk file(s)\n");
    exit(1);
  }
  if(nout == 0 && nvol == 0) {
    printf("ERROR: must specify output .trk or volume file(s)\n");
    exit(1);
  }
  if(nout > 0 && nout != nin) {
    printf("ERROR: must specify as many output .trk files as input files\n");
    exit(1);
  }
  if(nvol > 0 && nvol != nin) {
    printf("ERROR: must specify as many output volumes as input .trk files\n");
    exit(1);
  }
  if(!inRefFile) {
    printf("ERROR: must specify input reference volume\n");
    exit(1);
  }
  if(!outRefFile) {
    printf("ERROR: must specify output reference volume\n");
    exit(1);
  }
  return;
}

/* --------------------------------------------- */
static void dump_options(FILE *fp) {
  fprintf(fp,"\n");
  fprintf(fp,"%s\n",vcid);
  fprintf(fp,"cwd %s\n",cwd);
  fprintf(fp,"cmdline %s\n",cmdline);
  fprintf(fp,"sysname  %s\n",uts.sysname);
  fprintf(fp,"hostname %s\n",uts.nodename);
  fprintf(fp,"machine  %s\n",uts.machine);
  fprintf(fp,"user     %s\n",VERuser());

  fprintf(fp, "Input files:");
  for (int k = 0; k < nin; k++)
    fprintf(fp, " %s", inFile[k]);
  fprintf(fp, "\n");
  if (nout > 0) {
    fprintf(fp, "Output files:");
    for (int k = 0; k < nout; k++)
      fprintf(fp, " %s", outFile[k]);
    fprintf(fp, "\n");
  }
  if (nvol > 0) {
    fprintf(fp, "Output volumes:");
    for (int k = 0; k < nvol; k++)
      fprintf(fp, " %s", outVolFile[k]);
    fprintf(fp, "\n");
  }
  fprintf(fp, "Input reference: %s\n", inRefFile);
  fprintf(fp, "Output reference: %s\n", outRefFile);
  if (affineXfmFile)
    fprintf(fp, "Affine registration: %s\n", affineXfmFile);
  if (nonlinXfmFile)
    fprintf(fp, "Nonlinear registration: %s\n", nonlinXfmFile);

  return;
}

