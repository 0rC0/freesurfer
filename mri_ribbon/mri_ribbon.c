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

int main(int argc, char *argv[])
{
  char *inner_mris_fname,*outer_mris_fname,*input_mri_pref,*output_mri_pref;
  MRI *mri,*mri_src;
  MRI_SURFACE *inner_mris,*outer_mris;
  int nargs;

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, "$Id: mri_ribbon.c,v 1.5 2003/09/05 04:45:37 kteich Exp $", "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

  /* Set command-line parameters */
  if (argc!=5) {
    printf("Usage: mri_ribbon inner_surface_fname outer_surface_fname input_volume_pref output_volume_pref\n");
    exit(1);
  }
  Progname=argv[0];
  inner_mris_fname=argv[1];
  outer_mris_fname=argv[2];
  input_mri_pref=argv[3];
  output_mri_pref=argv[4];

  /* Read surface information from inner surface file */
  printf("Reading surface file %s.\n",inner_mris_fname);
  inner_mris=MRISread(inner_mris_fname);
  if (!inner_mris) {
    printf("Could not read surface file %s.\n",inner_mris_fname);
    exit(1);
  }

  /* Read surface information from outer surface file */
  printf("Reading surface file %s.\n",outer_mris_fname);
  outer_mris=MRISread(outer_mris_fname);
  if (!outer_mris) {
    printf("Could not read surface file %s.\n",outer_mris_fname);
    exit(1);
  }

  /* Read example volume from file */
  printf("Reading MRI volume directory %s.\n",input_mri_pref);
  mri_src=MRIread(input_mri_pref);
  if (!mri_src) {
    printf("Could not read MRI volume directory %s.\n",input_mri_pref);
    exit(1);
  }

  /* Extract ribbon */
  printf("Extracting ribbon.\n");
  mri=MRISribbon(inner_mris,outer_mris,mri_src,NULL);

  /* Save MRI volume to directory */
  printf("Writing volume file %s.\n",output_mri_pref);
  MRIwrite(mri,output_mri_pref);

  MRIfree(&mri);
  MRISfree(&inner_mris);
  MRISfree(&outer_mris);

  return 0;
}

