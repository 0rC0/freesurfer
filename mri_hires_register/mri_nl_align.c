//
// mri_nl_align.c
//
// written by Bruce Fischl
// Nov. 9th ,2000
// 
// Warning: Do not edit the following four lines.  CVS maintains them.
// Revision Author: $Author: fischl $
// Revision Date  : $Date: 2006/08/06 20:56:08 $
// Revision       : $Revision: 1.4 $
//
////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gcamorph.h"
#include "mri.h"
#include "matrix.h"
#include "proto.h"
#include "macros.h"
#include "error.h"
#include "timer.h"
#include "diag.h"
#include "mrimorph.h"
#include "utils.h"
#include "gca.h"
#include "cma.h"
#include "version.h"
#include "transform.h"
#include "nr_wrapper.h"
#include "fastmarching.h"
#include "voxlist.h"

#define NONMAX 0
#define PAD      10
#define PADVOX   1


static int apply_transform = 1 ;

static int write_snapshot(MRI *mri_target, MRI *mri_source, 
													MATRIX *m_vox_xform, GCA_MORPH_PARMS *parms, 
													int fno, int conform, char *fname) ;

static int regrid = 0 ;

static void  usage_exit(int ecode) ;
static int get_option(int argc, char *argv[]) ;


static char *source_intensity_fname = NULL ;
char *Progname ;

static int skip = 2 ;
static double distance = 1.0 ;


static TRANSFORM  *transform = NULL ;
static GCA_MORPH_PARMS mp ;

int
main(int argc, char *argv[])
{
	char         **av, *source_fname, *target_fname, *out_fname, fname[STRLEN] ;
  int          ac, nargs, new_transform = 0, pad ;
	MRI          *mri_target, *mri_source, *mri_tmp, *mri_orig_source ;
	MRI_REGION   box ;
  struct timeb start ;
  int          msec, minutes, seconds ;
	GCA_MORPH    *gcam ;
	MATRIX       *m_L/*, *m_I*/ ;
	LTA          *lta ;


	/* for nonlinear morph */
	mp.l_jacobian = 1 ;
	mp.l_distance = 0 ;
	mp.l_log_likelihood = .025 ;
	mp.dt = 0.005 ;
	mp.noneg = True ;
	mp.exp_k = 20 ;
	mp.diag_write_snapshots = 1 ;
	mp.momentum = 0.9 ;
	if (FZERO(mp.l_smoothness))
		mp.l_smoothness = 2 ;
	mp.sigma = 8 ;
	mp.relabel_avgs = -1 ;
	mp.navgs = 256 ;
	mp.levels = 6 ;
	mp.integration_type = GCAM_INTEGRATE_BOTH ;
	mp.nsmall = 1 ;
	mp.reset_avgs = -1 ;
	mp.npasses = 3 ;
	mp.regrid = regrid? True : False ;
	mp.tol = 0.1 ;
	mp.niterations = 1000 ;
	
  TimerStart(&start) ;
  setRandomSeed(-1L) ;
  DiagInit(NULL, NULL, NULL) ;
  ErrorInit(NULL, NULL, NULL) ;

	Progname = argv[0] ;
  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 4)
		usage_exit(1) ;

	source_fname = argv[1] ;
	target_fname = argv[2] ;
	out_fname = argv[3] ;
  FileNameOnly(out_fname, fname) ;
  FileNameRemoveExtension(fname, fname) ;
  strcpy(mp.base_name, fname) ;
	mri_source = MRIread(source_fname) ;
	if (!mri_source)
		ErrorExit(ERROR_NOFILE, "%s: could not read source label volume %s",
							Progname, source_fname) ;

	mri_target = MRIread(target_fname) ;
	if (!mri_target)
		ErrorExit(ERROR_NOFILE, "%s: could not read target label volume %s",
							Progname, target_fname) ;
	MRIboundingBox(mri_source, 0, &box) ;
	pad = (int)ceil(PADVOX * 
									MAX(mri_target->xsize,MAX(mri_target->ysize,mri_target->zsize)) / 
									MIN(mri_source->xsize,MIN(mri_source->ysize,mri_source->zsize))); 
	mri_tmp = MRIextractRegionAndPad(mri_source, NULL, &box, pad) ;
	printf("padding source with %d voxels...\n", pad) ;
	if (pad < 1)
		pad = 1 ;
	if ((Gdiag & DIAG_WRITE) && DIAG_VERBOSE_ON)
		MRIwrite(mri_tmp, "t.mgz") ;
	MRIfree(&mri_source) ;
	mri_source = mri_tmp ;
	mri_orig_source = MRIcopy(mri_source, NULL) ;
	MRImatchMeanIntensity(mri_source, mri_target, mri_source) ;

	mp.max_grad = 0.3*mri_source->xsize ;
	
	if (transform->type != MORPH_3D_TYPE)  // initializing m3d from a linear transform
	{
		new_transform = 1 ;
		lta = ((LTA *)(transform->xform)) ;
		if (lta->type != LINEAR_VOX_TO_VOX)
		{
			printf("converting ras xform to voxel xform\n") ;
			m_L = MRIrasXformToVoxelXform(mri_source, mri_target, lta->xforms[0].m_L, NULL) ;
			MatrixFree(&lta->xforms[0].m_L) ;
		}
		else
		{
			printf("using voxel xform\n") ;
			m_L = lta->xforms[0].m_L ;
		}
		if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
			write_snapshot(mri_target, mri_source, m_L, &mp, 0, 1, "linear_init");

#if 0		
		// transform RAS xform of both source to incorporate xform
		printf("voxel xform:\n") ;
		MatrixPrint(stdout, m_L) ;
		mri_tmp = MRITransformedCenteredMatrix(mri_source, mri_target, m_L) ;
		MRIfree(&mri_source) ;
		mri_source = mri_tmp ;
		
		if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
		{
			MRIwrite(mri_source, "s1.mgz") ;
		}
		m_I = MatrixIdentity(4, NULL) ;
		MRIrasXformToVoxelXform(mri_source, mri_target, m_I, m_L);
		MatrixFree(&m_I) ;
		
		/* make sure none of the labels are on the border */
		MRIboundingBox(mri_source, 0, &box) ;
		if (Gdiag & DIAG_WRITE && DIAG_VERBOSE)
			mri_tmp = MRIextractRegionAndPad(mri_source, NULL, &box, pad) ;
		MRIfree(&mri_source) ; mri_source = mri_tmp ;
		if (Gdiag & DIAG_WRITE && DIAG_VERBOSE_ON)
			MRIwrite(mri_source, "source_xformed.mgz") ;
#endif

		lta->xforms[0].m_L = m_L ;
		printf("initializing GCAM with vox->vox matrix:\n") ;
		MatrixPrint(stdout, m_L) ;
		gcam = GCAMcreateFromIntensityImage(mri_source, mri_target, transform) ;
	}
	else  /* use a previously create morph and integrate it some more */
	{
		printf("using previously create gcam...\n") ;
		gcam = (GCA_MORPH *)(transform->xform) ;
		GCAMrasToVox(gcam, mri_target) ;
	}
	if (gcam->width != mri_source->width ||
			gcam->height != mri_source->height ||
			gcam->depth != mri_source->depth)
		ErrorExit(ERROR_BADPARM, "%s: warning gcam (%d, %d, %d), doesn't match source vol (%d, %d, %d)",
							Progname, gcam->width, gcam->height, gcam->depth,
							mri_source->width, mri_source->height, mri_source->depth) ;
	
	mp.mri_diag = NULL ;
	mp.diag_morph_from_atlas = 0 ;
	mp.diag_volume = 0 ;
	
	mp.mri_diag = mri_target ;
	mp.diag_morph_from_atlas = 0 ;
	mp.diag_write_snapshots = 1 ;
	mp.diag_volume = GCAM_MEANS ;

	if (mp.write_iterations != 0)
	{
		char fname[STRLEN] ;
		MRI  *mri_gca ;
		
		sprintf(fname, "%s_target.mgz", mp.base_name) ;
		if (mp.diag_morph_from_atlas)
		{
			printf("writing target volume to %s...\n", fname) ;
			MRIwriteImageViews(mri_target, fname, IMAGE_SIZE) ;
			MRIwrite(mri_target, fname) ;
		}
		else
		{
			mri_gca = MRIclone(mri_source, NULL) ;
			GCAMbuildMostLikelyVolume(gcam, mri_gca) ;
			printf("writing target volume to %s...\n", fname) ;
			MRIwriteImageViews(mri_gca, fname, IMAGE_SIZE) ;
			MRIwrite(mri_gca, fname) ;
			MRIfree(&mri_gca) ;
		}
	}

	mp.mri = mri_target ;
	if (mp.regrid == True && new_transform == 0)
		GCAMregrid(gcam, mri_target, PAD, &mp, &mri_source) ;
	GCAMregister(gcam, mri_target, &mp) ; // atlas is source, morph target into register with it
	if (apply_transform)
	{
		MRI *mri_aligned ;
		char   fname[STRLEN] ;
		
		FileNameRemoveExtension(out_fname, fname) ;
		strcat(fname, ".mgz") ;
		mri_aligned = GCAMmorphFieldFromAtlas(gcam, mp.mri, GCAM_MEANS,0, 1) ;
		printf("writing transformed output volume to %s...\n", fname) ;
		MRIwrite(mri_aligned, fname) ;
		MRIfree(&mri_aligned) ;
	}
	printf("writing warp vector field to %s\n", out_fname) ;
	GCAMvoxToRas(gcam) ;
	GCAMwrite(gcam, out_fname) ;
	GCAMrasToVox(gcam, mri_target) ;

  msec = TimerStop(&start) ;
  seconds = nint((float)msec/1000.0f) ;
  minutes = seconds / 60 ;
  seconds = seconds % 60 ;
  printf("registration took %d minutes and %d seconds.\n", 
				 minutes, seconds) ;
	exit(0) ;
	return(0) ;
}

static int
get_option(int argc, char *argv[])
{
  int  nargs = 0 ;
  char *option ;

  option = argv[1] + 1 ;            /* past '-' */
  StrUpper(option) ;
  if (!stricmp(option, "debug_voxel"))
	{
		Gx = atoi(argv[2]) ;
		Gy = atoi(argv[3]) ;
		Gz = atoi(argv[4]) ;
		nargs = 3 ;
		printf("debugging voxel (%d, %d, %d)\n", Gx, Gy, Gz) ;
	}
  else if (!stricmp(option, "OPTIMAL"))
  {
    mp.integration_type = GCAM_INTEGRATE_OPTIMAL ;
    printf("using optimal time-step integration\n") ;
  }
  else if (!stricmp(option, "MOMENTUM") || !stricmp(option, "FIXED"))
  {
    mp.integration_type = GCAM_INTEGRATE_FIXED ;
    printf("using optimal time-step integration\n") ;
  }
  else if (!stricmp(option, "distance"))
  {
    distance = atof(argv[2]) ;
		nargs = 1 ;
    printf("expanding border by %2.1f mm every outer cycle\n", distance);
  }
  else if (!stricmp(option, "intensity") ||!stricmp(option, "ll"))
  {
    mp.l_log_likelihood = atof(argv[2]) ;
		nargs = 1 ;
    printf("setting l_log_likelihood = %2.1f\n", mp.l_log_likelihood );
  }
  else if (!stricmp(option, "noregrid"))
  {
		regrid = 0 ;
		mp.regrid = False ;
    printf("disabling regridding...\n") ;
  }
  else if (!stricmp(option, "regrid"))
  {
		regrid = 1 ;
		mp.regrid = True ;
    printf("enabling regridding...\n") ;
  }
  else if (!stricmp(option, "view"))
	{
		Gsx = atoi(argv[2]) ;
		Gsy = atoi(argv[3]) ;
		Gsz = atoi(argv[4]) ;
		nargs = 3 ;
		printf("viewing voxel (%d, %d, %d)\n", Gsx, Gsy, Gsz) ;
	}
  else if (!stricmp(option, "LEVELS"))
  {
    mp.levels = atoi(argv[2]) ;
    nargs = 1 ;
    printf("levels = %d\n", mp.levels) ;
  }
  else if (!stricmp(option, "area"))
  {
    mp.l_area = atof(argv[2]) ;
    nargs = 1 ;
    printf("using l_area=%2.3f\n", mp.l_area) ;
  }
  else if (!stricmp(option, "area_intensity"))
  {
    mp.l_area_intensity = atof(argv[2]) ;
    nargs = 1 ;
    printf("using l_area_intensity=%2.3f\n", mp.l_area_intensity) ;
  }
  else if (!stricmp(option, "tol"))
  {
    mp.tol = atof(argv[2]) ;
    nargs = 1 ;
    printf("using tol=%2.3f\n", mp.tol) ;
  }
  else if (!stricmp(option, "sigma"))
  {
    mp.sigma = atof(argv[2]) ;
    nargs = 1 ;
    printf("using sigma=%2.3f\n", mp.sigma) ;
  }
	else if (!stricmp(option, "rthresh"))
  {
		mp.ratio_thresh = atof(argv[2]) ;
    nargs = 1 ;
    printf("using compression ratio threshold = %2.3f...\n", mp.ratio_thresh) ;
  }
	else if (!stricmp(option, "dt"))
	{
		mp.dt = atof(argv[2]) ;
		nargs = 1 ;
		printf("using dt = %2.3f\n", mp.dt) ;
	}
	else if (!stricmp(option, "passes"))
	{
		mp.npasses = atoi(argv[2]) ;
		nargs = 1 ;
		printf("integrating in %d passes (default=3)\n", mp.npasses) ;
	}
	else if (!stricmp(option, "skip"))
	{
		skip = atoi(argv[2]);
		printf("skipping %d voxels in source data...\n", skip) ;
		nargs = 1 ;
	}
	else switch (*option)
	{
	case 'D':
		mp.l_distance = atof(argv[2]) ;
		nargs = 1 ;
		printf("using l_distance = %2.3f\n", mp.l_distance) ;
		break ;
  case 'M':
    mp.momentum = atof(argv[2]) ;
    nargs = 1 ;
    printf("momentum = %2.2f\n", mp.momentum) ;
    break ;
	case 'N':
		mp.niterations = atoi(argv[2]) ;
		nargs = 1 ;
		printf("using niterations = %d\n", mp.niterations) ;
		break ;
	case 'S':
		mp.l_smoothness = atof(argv[2]) ;
		nargs = 1 ;
		printf("using l_smoothness = %2.3f\n", mp.l_smoothness) ;
		break ;
	case 'T':
		printf("reading transform from %s...\n", argv[2]) ;
		transform = TransformRead(argv[2]) ;
		if (transform == NULL)
			ErrorExit(ERROR_NOFILE,"%s: could not read transform from %s\n",Progname,argv[2]);
		nargs = 1 ;
		break ;
	case 'I':
		source_intensity_fname = argv[2] ;
		nargs = 1 ;
		printf("reading intensity image from %s for debugging...\n", source_intensity_fname) ;
		break ;
	case 'B':
    mp.l_binary = atof(argv[2]) ;
    nargs = 1 ;
    printf("using l_binary=%2.3f\n", mp.l_binary) ;
    break ;
  case 'J':
    mp.l_jacobian = atof(argv[2]) ;
    nargs = 1 ;
    printf("using l_jacobian=%2.3f\n", mp.l_jacobian) ;
    break ;
  case 'A':
		mp.navgs = atoi(argv[2]) ;
    nargs = 1 ;
    printf("smoothing gradient with %d averages...\n", mp.navgs) ;
    break ;
  case 'K':
    mp.exp_k = atof(argv[2]) ;
    printf("setting exp_k to %2.2f (default=%2.2f)\n",
           mp.exp_k, EXP_K) ;
    nargs = 1 ;
    break ;
	case 'W':
		mp.write_iterations = atoi(argv[2]) ;
		Gdiag |= DIAG_WRITE ;
		nargs = 1 ;
		printf("setting write iterations = %d\n", mp.write_iterations) ;
		break ;
  case '?':
  case 'U':
    usage_exit(1);
    break ;
	default:
    printf("unknown option %s\n", argv[1]) ;
		usage_exit(1) ;
    break ;
	}
	return(nargs) ;
}

static void 
usage_exit(int ecode)
{
	printf(
				 "usage: %s <source> <destination> <output xform>\n",
				 Progname) ;
	exit(ecode) ;
}


static int
write_snapshot(MRI *mri_target, MRI *mri_source, MATRIX *m_vox_xform, 
							 GCA_MORPH_PARMS *parms, int fno, int conform, char *in_fname)
{
	MRI *mri_aligned ;
	char fname[STRLEN] ;

	if (Gdiag & DIAG_SHOW && DIAG_VERBOSE_ON)
	{
		printf("source->target vox->vox transform:\n") ;
		MatrixPrint(stdout, m_vox_xform) ;
	}
	if (conform || 1)
	{
		mri_aligned = MRIalloc(mri_target->width, mri_target->height,
													 mri_target->depth,mri_source->type);
		MRIcopyHeader(mri_target, mri_aligned) ;
		MRIlinearTransformInterp(mri_source, mri_aligned, m_vox_xform, SAMPLE_NEAREST);
	}
	else
	{
		mri_aligned = MRITransformedCenteredMatrix(mri_source, mri_target, m_vox_xform) ;
	}
	if (in_fname)
		sprintf(fname, "%s_%s", parms->base_name, in_fname) ;
	else
		sprintf(fname, "%s_%03d", parms->base_name, fno) ;
	MRIwriteImageViews(mri_aligned, fname, IMAGE_SIZE) ;
	if (in_fname)
		sprintf(fname, "%s_%s.mgz", parms->base_name, in_fname) ;
	else
		sprintf(fname, "%s_%03d.mgz", parms->base_name, fno) ;
	printf("writing snapshot to %s...\n", fname) ;
	MRIwrite(mri_aligned, fname) ;
	MRIfree(&mri_aligned) ;

	{
#if 0
		mri_aligned = MRIsrcTransformedCentered(mri_source, mri_target, m_vox_xform,
																						SAMPLE_NEAREST) ;
#else
		mri_aligned = MRITransformedCenteredMatrix(mri_source, mri_target, m_vox_xform) ;
#endif
		if (in_fname)
			sprintf(fname, "orig_%s_%s.mgz", parms->base_name, in_fname) ;
		else
			sprintf(fname, "orig_%s_%03d.mgz", parms->base_name, fno) ;
		printf("writing snapshot to %s...\n", fname) ;
		MRIwrite(mri_aligned, fname) ;
		MRIfree(&mri_aligned) ;
	}

#if 0
	if (mri_source_intensity)
	{
#if 0
		mri_aligned = MRIsrcTransformedCentered(mri_source_intensity, mri_target, m_vox_xform, SAMPLE_TRILINEAR) ;
#else
		mri_aligned = MRITransformedCenteredMatrix(mri_source_intensity, mri_target, m_vox_xform) ;
#endif
		if (in_fname)
			sprintf(fname, "intensity_%s_%s", parms->base_name, in_fname) ;
		else
			sprintf(fname, "intensity_%s_%03d", parms->base_name, fno) ;
		MRIwriteImageViews(mri_aligned, fname, IMAGE_SIZE) ;
		if (in_fname)
			sprintf(fname, "intensity_%s_%s.mgz", parms->base_name, in_fname) ;
		else
			sprintf(fname, "intensity_%s_%03d.mgz", parms->base_name, fno) ;
		printf("writing snapshot to %s...\n", fname) ;
		MRIwrite(mri_aligned, fname) ;
		MRIfree(&mri_aligned) ;
	}
#endif

	return(NO_ERROR) ;
}

