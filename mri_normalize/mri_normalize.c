#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "timer.h"
#include "proto.h"
#include "mrinorm.h"
#include "mri_conform.h"

int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;
static void  usage_exit(void) ;

static int conform = 0 ;

char *Progname ;

static MRI_NORM_INFO  mni ;
static int verbose = 1 ;
static int num_3d_iter = 2 ;

static float intensity_above = 20 ;
static float intensity_below = 10 ;

static char *control_point_fname ;

static char *control_volume_fname = NULL ;
static char *bias_volume_fname = NULL ;

static int no1d = 0 ;

int
main(int argc, char *argv[])
{
  char   **av ;
  int    ac, nargs, n ;
  MRI    *mri_src, *mri_dst = NULL ;
  char   *in_fname, *out_fname ;
  int          msec, minutes, seconds ;
  struct timeb start ;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  mni.max_gradient = MAX_GRADIENT ;
  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 2)
    usage_exit() ;

  if (argc < 1)
    ErrorExit(ERROR_BADPARM, "%s: no input name specified", Progname) ;
  in_fname = argv[1] ;

  if (argc < 2)
    ErrorExit(ERROR_BADPARM, "%s: no output name specified", Progname) ;
  out_fname = argv[2] ;

  if (verbose)
    fprintf(stderr, "reading from %s...\n", in_fname) ;
  mri_src = MRIread(in_fname) ;
  if (!mri_src)
    ErrorExit(ERROR_NO_FILE, "%s: could not open source file %s", 
              Progname, in_fname) ;
#if 0
  if ((mri_src->type != MRI_UCHAR) ||
      (!(mri_src->xsize == 1 && mri_src->ysize == 1 && mri_src->zsize == 1)))
#else
    if (conform || mri_src->type != MRI_UCHAR)
#endif
  {
    MRI  *mri_tmp ;

    fprintf(stderr, 
            "downsampling to 8 bits and scaling to isotropic voxels...\n") ;
    mri_tmp = MRIconform(mri_src) ;
    mri_src = mri_tmp ;
  }

  if (verbose)
    fprintf(stderr, "normalizing image...\n") ;
  TimerStart(&start) ;
  if (!no1d)
  {
    MRInormInit(mri_src, &mni, 0, 0, 0, 0, 0.0f) ;
    mri_dst = MRInormalize(mri_src, NULL, &mni) ;
    if (!mri_dst)
      ErrorExit(ERROR_BADPARM, "%s: normalization failed", Progname) ;
  }
  else
  {
    mri_dst = MRIcopy(mri_src, NULL) ;
    if (!mri_dst)
      ErrorExit(ERROR_BADPARM, "%s: could not allocate volume", Progname) ;
  }

  if (control_point_fname)
    MRI3dUseFileControlPoints(mri_dst, control_point_fname) ;
  if (control_volume_fname)
    MRI3dWriteControlPoints(control_volume_fname) ;
  if (bias_volume_fname)
    MRI3dWriteBias(bias_volume_fname) ;
  for (n = 0 ; n < num_3d_iter ; n++)
  {
    fprintf(stderr, "3d normalization pass %d of %d\n", n+1, num_3d_iter) ;
    MRI3dNormalize(mri_dst, NULL, DEFAULT_DESIRED_WHITE_MATTER_VALUE, mri_dst,
                   intensity_above, intensity_below,
                   control_point_fname != NULL && !n && no1d);
  }

  if (verbose)
    fprintf(stderr, "writing output to %s\n", out_fname) ;
  MRIwrite(mri_dst, out_fname) ;
  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;
  fprintf(stderr, "3D bias adjustment took %d minutes and %d seconds.\n", 
          minutes, seconds) ;
  exit(0) ;
  return(0) ;
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
  if (!stricmp(option, "no1d"))
  {
    no1d = 1 ;
    fprintf(stderr, "disabling 1d normalization...\n") ;
  }
  else if (!stricmp(option, "conform"))
  {
    conform = 1 ;
    fprintf(stderr, "interpolating and embedding volume to be 256^3...\n") ;
  }
  else switch (toupper(*option))
  {
  case 'W':
    control_volume_fname = argv[2] ;
    bias_volume_fname = argv[3] ;
    nargs = 2 ;
    fprintf(stderr, "writing ctrl pts to   %s\n", control_volume_fname) ;
    fprintf(stderr, "writing bias field to %s\n", bias_volume_fname) ;
    break ;
  case 'F':
    control_point_fname = argv[2] ;
    nargs = 1 ;
    fprintf(stderr, "using control points from file %s...\n", 
           control_point_fname) ;
    break ;
  case 'A':
    intensity_above = atof(argv[2]) ;
    fprintf(stderr, "using control point with intensity %2.1f above target.\n",
            intensity_above) ;
    nargs = 1 ;
    break ;
  case 'B':
    intensity_below = atof(argv[2]) ;
    fprintf(stderr, "using control point with intensity %2.1f below target.\n",
            intensity_below) ;
    nargs = 1 ;
    break ;
  case 'G':
    mni.max_gradient = atof(argv[2]) ;
    fprintf(stderr, "using max gradient = %2.3f\n", mni.max_gradient) ;
    nargs = 1 ;
    break ;
  case 'V':
    verbose = !verbose ;
    break ;
  case 'N':
    num_3d_iter = atoi(argv[2]) ;
    nargs = 1 ;
    fprintf(stderr, "performing 3d normalization %d times\n", num_3d_iter) ;
    break ;
  case '?':
  case 'U':
    printf("usage: %s [input directory] [output directory]\n", argv[0]) ;
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
  fprintf(stderr, "usage: %s [options] <input volume> <output volume>\n", 
          Progname) ;
  fprintf(stderr, "\t-n <# of 3d normalization iterations>, default=5\n") ;
  fprintf(stderr, "\t-g <max intensity/mm gradient>, default=0.6\n") ;
  fprintf(stderr, "\t-s <sigma>, "
          "intensity interval pct for 3d, default=0.15\n") ;
  exit(0) ;
}

