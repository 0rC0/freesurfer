/**
 * @file  mri_partial_ribbon.c
 * @brief REPLACE_WITH_ONE_LINE_SHORT_DESCRIPTION
 *
 * REPLACE_WITH_LONG_DESCRIPTION_OR_REFERENCE
 */
/*
 * Original Author: REPLACE_WITH_FULL_NAME_OF_CREATING_AUTHOR 
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2006/12/29 02:09:08 $
 *    $Revision: 1.9 $
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


//////////////////////////////////////////////////////////////
//   mri_partial_ribbon
//
// originally written by Andre van der Kouwe
//
// Warning: Do not edit the following four lines.  CVS maintains them.
// Revision Author: $Author: nicks $
// Revision Date  : $Date: 2006/12/29 02:09:08 $
// Revision       : $Revision: 1.9 $
//
////////////////////////////////////////////////////////////////////
char *MRI_PARTIAL_RIBBON_VERSION = "$Revision: 1.9 $";
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mri.h"
#include "mrisurf.h"
#include "macros.h"
#include "version.h"

#define IMGSIZE 256

char *Progname;

void usage() {
  printf("Usage: mri_partial_ribbon innerSurf_lh outerSurf_lh innerSurf_rh outerSurf_rh input_volume output_volume cma_mask\n");
}


int main(int argc, char *argv[]) {
  char *inner_mris_fname_lh,*outer_mris_fname_lh,*inner_mris_fname_rh,*outer_mris_fname_rh,*input_mri_pref,*output_mri_pref,*mask_mri_pref;
  MRI *mri,*mri_src,*mri_mask;
  MRI_SURFACE *inner_mris_lh,*outer_mris_lh,*inner_mris_rh,*outer_mris_rh;
  int nargs;

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, "$Id: mri_partial_ribbon.c,v 1.9 2006/12/29 02:09:08 nicks Exp $", "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

  /* Set command-line parameters */
  if (argc < 7) {
    usage();
    exit(-1);
  }
  Progname=argv[0];

  inner_mris_fname_lh=argv[1]; // lh.white
  outer_mris_fname_lh=argv[2]; // lh.pial
  inner_mris_fname_rh=argv[3]; // rh.white
  outer_mris_fname_rh=argv[4]; // rh.pial
  input_mri_pref=argv[5];      // volume
  output_mri_pref=argv[6];     // output
  if (argc==8)
    mask_mri_pref=argv[7];     // cma mask
  else
    mask_mri_pref=NULL;

  /* Read surface information from left inner surface file */
  printf("Reading left inner surface file %s.\n",inner_mris_fname_lh);
  inner_mris_lh=MRISread(inner_mris_fname_lh);
  if (!inner_mris_lh) {
    fprintf(stderr,"Could not read surface file %s.\n",inner_mris_fname_lh);
    exit(1);
  }

  /* Read surface information from left outer surface file */
  printf("Reading left outer surface file %s.\n",outer_mris_fname_lh);
  outer_mris_lh=MRISread(outer_mris_fname_lh);
  if (!outer_mris_lh) {
    fprintf(stderr,"Could not read surface file %s.\n",outer_mris_fname_lh);
    exit(1);
  }

  /* Read surface information from right inner surface file */
  printf("Reading right inner surface file %s.\n",inner_mris_fname_rh);
  inner_mris_rh=MRISread(inner_mris_fname_rh);
  if (!inner_mris_rh) {
    fprintf(stderr,"Could not read surface file %s.\n",inner_mris_fname_rh);
    exit(1);
  }

  /* Read surface information from right outer surface file */
  printf("Reading right outer surface file %s.\n",outer_mris_fname_rh);
  outer_mris_rh=MRISread(outer_mris_fname_rh);
  if (!outer_mris_rh) {
    fprintf(stderr,"Could not read surface file %s.\n",outer_mris_fname_rh);
    exit(1);
  }

  /* Read example volume from file */
  printf("Reading MRI volume directory %s.\n",input_mri_pref);
  mri_src=MRIread(input_mri_pref);
  if (!mri_src) {
    fprintf(stderr,"Could not read MRI volume directory %s.\n",input_mri_pref);
    exit(1);
  }

  /* Read example volume from file */
  if (argc==8) {
    printf("Reading MRI volume directory %s.\n",mask_mri_pref);
    mri_mask=MRIread(mask_mri_pref);
    if (!mri_mask) {
      fprintf(stderr,"Could not read MRI volume directory %s.\n",mask_mri_pref);
      exit(1);
    }
  } else
    mri_mask=NULL;

  /* Extract ribbon */
  printf("Extracting ribbon.\n");
  mri=MRISpartialribbon(inner_mris_lh,outer_mris_lh,inner_mris_rh,outer_mris_rh,mri_src,NULL,mri_mask);

  /* Save MRI volume to directory */
  printf("Writing volume file %s.\n",output_mri_pref);
  MRIwrite(mri,output_mri_pref);

  MRIfree(&mri);
  MRIfree(&mri_src);
  if (mri_mask)
    MRIfree(&mri_mask);
  MRISfree(&inner_mris_lh);
  MRISfree(&outer_mris_lh);
  MRISfree(&inner_mris_rh);
  MRISfree(&outer_mris_rh);

  return 0;
}

