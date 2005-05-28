#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "macros.h"
#include "error.h"
#include "diag.h"
#include "mri.h"
#include "proto.h"
#include "gca.h"
#include "gcamorph.h"
#include "cma.h"
#include "transform.h"
#include "version.h"

#define UNIT_VOLUME 128

static char vcid[] = "$Id: mri_make_density_map.c,v 1.2 2005/05/28 18:52:48 fischl Exp $";

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;

char *Progname ;
static char *out_like_fname = NULL ;

static int nreductions = 0 ;
static char *xform_fname = NULL ;
static LTA  *lta = NULL;
static TRANSFORM *transform = NULL ;
static float sigma = 0 ;

int
main(int argc, char *argv[])
{
  char        **av, *seg_fname, *intensity_fname, *out_fname, *gcam_fname ;
  int         ac, nargs, i, label ;
  MRI         *mri_seg, *mri_intensity, *mri_out = NULL, *mri_kernel, *mri_smoothed ;
	GCA_MORPH   *gcam ;
	

  /* rkt: check for and handle version tag */
  nargs = handle_version_option (argc, argv, "$Id: mri_make_density_map.c,v 1.2 2005/05/28 18:52:48 fischl Exp $", "$Name:  $");
  if (nargs && argc - nargs == 1)
    exit (0);
  argc -= nargs;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 6)
    usage_exit() ;

  seg_fname = argv[1] ;
	intensity_fname = argv[2] ;
	gcam_fname = argv[3] ;
  out_fname = argv[argc-1] ;

  printf("reading segmentation volume from %s\n", seg_fname) ;
  mri_seg = MRIread(seg_fname) ;
  if (!mri_seg)
    ErrorExit(ERROR_NOFILE, "%s: could not read segmentation volume %s", Progname, 
              seg_fname) ;

  printf("reading intensity volume from %s\n", intensity_fname) ;
  mri_intensity = MRIread(intensity_fname) ;
  if (!mri_intensity)
    ErrorExit(ERROR_NOFILE, "%s: could not read intensity volume %s", Progname, 
              intensity_fname) ;

	gcam = GCAMread(gcam_fname) ;
	if (gcam == NULL)
		ErrorExit(ERROR_NOFILE, "%s: could not read gcam from %s", Progname, gcam_fname) ;

  for (i = 4 ; i < argc-1 ; i++)
  {
    label = atoi(argv[i]) ;
    printf("extracting label %d (%s)\n", label, cma_label_to_name(label)) ;
		mri_out = GCAMextract_density_map(mri_seg, mri_intensity, gcam, label, mri_out) ;
		/*    extract_labeled_image(mri_in, transform, label, mri_out) ;*/
  }
	if (!FZERO(sigma))
	{
		printf("smoothing extracted volume...\n") ;
		mri_kernel = MRIgaussian1d(sigma, 10*sigma) ;
		mri_smoothed = MRIconvolveGaussian(mri_out, NULL, mri_kernel) ;
		MRIfree(&mri_out) ; mri_out = mri_smoothed ;
	}
	/* removed for gcc3.3
	 * vsprintf(out_fname, out_fname, (va_list) &label) ;
	 */
	while (nreductions-- > 0)
	{
		MRI *mri_tmp ;

		mri_tmp = MRIreduce(mri_out, NULL) ;
		MRIfree(&mri_out) ; mri_out = mri_tmp ;
	}
	printf("writing output to %s\n", out_fname) ;
	MRIwrite(mri_out, out_fname) ;
  

  exit(0) ;
  return(0) ;  /* for ansi */
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;
  
  option = argv[1] + 1 ;            /* past '-' */
  if (!stricmp(option, "-help"))
    print_help() ;
  else if (!stricmp(option, "-version"))
    print_version() ;
  else if (!stricmp(option, "DEBUG_VOXEL"))
  {
    Gx = atoi(argv[2]) ;
    Gy = atoi(argv[3]) ;
    Gz = atoi(argv[4]) ;
    nargs = 3 ;
    printf("debugging voxel (%d, %d, %d)\n", Gx,Gy,Gz) ;
  }
  else if (!stricmp(option, "out_like") || !stricmp(option, "ol"))
  {
    out_like_fname = argv[2] ;
    nargs = 1 ;
    printf("shaping output to be like %s...\n", out_like_fname) ;
  }
  else switch (toupper(*option))
  {
	case 'R':
		nreductions = atoi(argv[2]) ;
		printf("reducing density maps %d times...\n", nreductions);
		nargs = 1 ;
		break ;
  case 'S':
    sigma = atof(argv[2]) ;
    printf("applying sigma=%2.1f smoothing kernel after extraction...\n",sigma) ;
    nargs = 1 ;
    break ;
  case 'T':
    xform_fname = argv[2] ;
    printf("reading and applying transform %s...\n", xform_fname) ;
    nargs = 1 ;
    transform = TransformRead(xform_fname) ;
    if (!transform)
      ErrorExit(ERROR_NOFILE, "%s: could not read transform from %s", 
                Progname, xform_fname) ;

		if (transform->type != MORPH_3D_TYPE)
			lta = (LTA *)(transform->xform) ;
    break ;
  case 'V':
    Gdiag_no = atoi(argv[2]) ;
    nargs = 1 ;
    break ;
  case '?':
  case 'U':
    print_usage() ;
    exit(1) ;
    break ;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}

static void
usage_exit(void)
{
  print_usage() ;
  print_help() ;
  exit(1) ;
}

static void
print_usage(void)
{
  fprintf(stderr, 
          "usage: %s [options] <input volume> <label 1> <label 2> ... <output name>\n",
          Progname) ;
  fprintf(stderr, "where options are:\n") ;
  fprintf(stderr, 
          "\t-s <sigma>\tapply a Gaussian smoothing kernel\n"
          "\t-r <n>\tapply a Gaussian reduction n times\n"
          "\t-t <xform file>\tapply the transform in <xform file> to extracted volume\n");
}

static void
print_help(void)
{
  fprintf(stderr, 
          "\nThis program will extract a set of labeled voxels from an image\n") ;
  exit(1) ;
}

static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}


