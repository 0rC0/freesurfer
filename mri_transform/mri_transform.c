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
#include "transform.h"

#define LINEAR_CORONAL_RAS_TO_CORONAL_RAS       21
//E/ should be in transform.h if it isn't already

static char vcid[] = "$Id: mri_transform.c,v 1.3 2002/11/20 23:36:51 ebeth Exp $";

//E/ For transformations: for case LINEAR_RAS_TO_RAS, we convert to
//vox2vox with MRIrasXformToVoxelXform() in mri.c; for case
//LINEAR_CORONAL_RAS_TO_CORONAL_RAS, this converts to vox2vox with
//MT_CoronalRasXformToVoxelXform(), below.  For LINEAR_VOX_TO_VOX, we
//don't know the output vox2ras tx, and just do the best we can
//(i.e. guess isotropic cor).

//E/ Eventually, maybe, this should be in more public use and live in mri.c:
static MATRIX *
MT_CoronalRasXformToVoxelXform(MRI *mri_in, MRI *mri_out,
				MATRIX *m_ras2ras, MATRIX *m_vox2vox);

int main(int argc, char *argv[]) ;

static int  get_option(int argc, char *argv[]) ;
static void usage_exit(void) ;
static void print_usage(void) ;
static void print_help(void) ;
static void print_version(void) ;

char *Progname ;
static char *out_like_fname = NULL ;
static int invert_flag = 0 ;

int
main(int argc, char *argv[])
{
  char        **av, *in_vol, *out_vol, *xform_fname ;
  int         ac, nargs, i, ras_flag = 0 ;
  MRI         *mri_in, *mri_out, *mri_tmp ;
  LTA         *lta ;
  MATRIX      *m, *m_total ;
  TRANSFORM   *transform ;

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

  if (argc < 4)
    usage_exit() ;

  in_vol = argv[1] ;
  out_vol = argv[argc-1] ;

  printf("reading volume from %s...\n", in_vol) ;
  mri_in = MRIread(in_vol) ;
  if (!mri_in)
    ErrorExit(ERROR_NOFILE, "%s: could not read MRI volume %s", Progname, 
              in_vol) ;
  if (out_like_fname) //E/ maybe need an out_kinda_like_fname
    {
      mri_tmp = MRIread(out_like_fname) ;
      if (!mri_tmp)
	ErrorExit(ERROR_NOFILE, "%s: could not read template volume from %s",out_like_fname) ;
      mri_out = MRIalloc(mri_tmp->width, mri_tmp->height, mri_tmp->depth, mri_tmp->type) ;
      //E/ maybe better mri_in->type?
      MRIcopyHeader(mri_tmp, mri_out) ; //E/ reinstate this
      //E/ MRIfree(&mri_tmp) ; // keep this around for recopy later.

      //E/ Hey, MRIlinearTransformInterp() just sets
      //dst->ras_good_flag to zero!  and the x/y/zsize and stuff seems
      //to go away during e.g. mghWrite.  recopy later?
    }
  else
    {
      mri_out = MRIalloc(256, 256, 256, mri_in->type) ;
      //E/ set xyzc_ras to coronal ones.. - these'll get zorched
      //by MRIlinearTransformInterp() - copy again later - is there
      //any use in having them here now?  yes, so we can pass mri_out
      //to the ras2vox fns.

      //E/ is c_ras = 0,0,0 correct?
      mri_out->x_r =-1; mri_out->y_r = 0; mri_out->z_r = 0; mri_out->c_r =0;
      mri_out->x_a = 0; mri_out->y_a = 0; mri_out->z_a = 1; mri_out->c_a =0;
      mri_out->x_s = 0; mri_out->y_s =-1; mri_out->z_s = 0; mri_out->c_s =0;
      mri_out->ras_good_flag=1;
    }
  MRIcopyPulseParameters(mri_in, mri_out) ;
  m_total = MatrixIdentity(4, NULL) ;
  for (i = 2 ; i < argc-1 ; i++)
  {
    xform_fname = argv[i] ;
    if (strcmp(xform_fname, "-I") == 0)
    {
      invert_flag = 1 ;
      continue ;
    }
    printf("reading transform %s...\n", xform_fname) ;
    transform = TransformRead(xform_fname) ;
    if (!transform)
      ErrorExit(ERROR_NOFILE, "%s: could not read transform from %s", 
                Progname, xform_fname) ;

    if (transform->type != MORPH_3D_TYPE)
    {
      lta = (LTA *)(transform->xform) ;
      m = MatrixCopy(lta->xforms[0].m_L, NULL) ;
      //E/ mri_rigid_register writes out m as src2trg
      
      if (lta->type == LINEAR_RAS_TO_RAS || LINEAR_CORONAL_RAS_TO_CORONAL_RAS)
	/* convert it to a voxel transform */
      {
        ras_flag = 1 ;
      }
      else if (ras_flag)
        ErrorExit(ERROR_UNSUPPORTED, "%s: transforms must be all RAS or all voxel",Progname) ;
      
      if (invert_flag)
      {
        MATRIX *m_tmp ;
        printf("inverting transform...\n") ;
        m_tmp = MatrixInverse(m, NULL) ;
        if (!m_tmp)
          ErrorExit(ERROR_BADPARM, "%s: transform is singular!") ;
        MatrixFree(&m) ; m = m_tmp ;
        invert_flag = 0 ;
      }
      MatrixMultiply(m, m_total, m_total) ;
      if (ras_flag)  /* convert it to a voxel transform */
      {
        MATRIX *m_tmp ;
        printf("converting RAS xform to voxel xform...\n") ;
	if (lta->type == LINEAR_RAS_TO_RAS)
	  m_tmp = MRIrasXformToVoxelXform(mri_in, mri_out, m_total, NULL) ;
	else if (lta->type == LINEAR_CORONAL_RAS_TO_CORONAL_RAS)
	  m_tmp = 
	    MT_CoronalRasXformToVoxelXform(mri_in, mri_out, m_total, NULL) ;
	else
	  //E/ how else could ras_flag be set? prev tx a R2R/CR2CR tx?
	  exit(1);
        MatrixFree(&m_total) ; m_total = m_tmp ;
      }
      LTAfree(&lta) ;
      MRIlinearTransform(mri_in, mri_out, m_total) ;
    }
    else
    {
      if (invert_flag)
        mri_out = TransformApplyInverse(transform, mri_in, NULL) ;
      else
        mri_out = TransformApply(transform, mri_in, NULL) ;
    }
    invert_flag = 0 ;
  }
  
  printf("writing output to %s.\n", out_vol) ;

  //E/ reinstate what MRIlinearTransform zorched
  if (out_like_fname)
    {
      //E/ MRIcopyHeader doesn't stick because later
      //MRIlinearTransformInterp sets dst->ras_good_flag to zero!  and
      //the x/y/zsize and stuff seems to go away during e.g. mghWrite.
      //So we recopy it now.  'cause ras xform IS good.  Well, if
      //mri_tmp's is.

      MRIcopyHeader(mri_tmp, mri_out) ;
      MRIfree(&mri_tmp) ;
    }
  else
    {
      //E/ set xyzc_ras to coronal 256/1mm^3 isotropic ones.. - they
      //got zorched by MRIlinearTransformInterp() - so copy again here

      mri_out->x_r =-1; mri_out->y_r = 0; mri_out->z_r = 0; mri_out->c_r =0;
      mri_out->x_a = 0; mri_out->y_a = 0; mri_out->z_a = 1; mri_out->c_a =0;
      mri_out->x_s = 0; mri_out->y_s =-1; mri_out->z_s = 0; mri_out->c_s =0;
      mri_out->ras_good_flag=1;
    }
  //E/  

  MRIwrite(mri_out, out_vol) ;

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
  else if (!stricmp(option, "out_like") || !stricmp(option, "ol"))
  {
    out_like_fname = argv[2] ;
    nargs = 1 ;
    printf("shaping output to be like %s...\n", out_like_fname) ;
  }
  else switch (toupper(*option))
  {
  case 'V':
    Gdiag_no = atoi(argv[2]) ;
    nargs = 1 ;
    break ;
  case 'I':
    invert_flag = 1 ;
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
  //  print_usage() ; // print_help _calls print_usage
  print_help() ;
  exit(1) ;
}

static void
print_usage(void)
{
  fprintf(stderr, 
	  //          "usage: %s [options] <input volume> <input surface> <registration file> <output .float file>\n",
	  //E/ where did that come from??

"usage: %s [options] <input volume> <lta file> <output file>\n",

          Progname) ;
}

static void
print_help(void)
{
  print_usage() ;
#if 0
  fprintf(stderr, 
     "\nThis program will paint a average Talairach stats onto a surface\n");
  fprintf(stderr, "-imageoffset <image offset> - set offset to use\n") ;
  fprintf(stderr, "-S                          - paint using surface "
          "coordinates\n") ;
#else
  fprintf(stderr, 
     "\nThis program will apply a linear transform to mri volume and write out the result.  The output volume is by default 256^3 1mm^3 isotropic, or you can specify an -out_like volume.  I think there's a bug in -i behavior if you're specifying multiple transforms.\n");
  fprintf(stderr, "-out_like <reference volume> - set out_volume parameters\n") ;
  fprintf(stderr, "-I                           - invert transform "
          "coordinates\n") ;
  

#endif



  exit(1) ;
}


static void
print_version(void)
{
  fprintf(stderr, "%s\n", vcid) ;
  exit(1) ;
}


MATRIX *
MT_CoronalRasXformToVoxelXform
(MRI *mri_in, MRI *mri_out, MATRIX *m_corras_s2corras_t, MATRIX *m_vox_s2vox_t)
{
  MATRIX *D_src, *D_trg, *D_trg_inv, *m_tmp;

  //E/ D's are the vox2coronalras matrices generated by
  //MRIxfmCRS2XYZtkreg().  Thanks, Doug.

  D_src = MRIxfmCRS2XYZtkreg(mri_in);
  D_trg = MRIxfmCRS2XYZtkreg(mri_out);
  D_trg_inv = MatrixInverse(D_trg, NULL);

  //E/ m_vox_s2vox_t = D_trg_inv * m_corras_s2corras_t * D_src
  
  m_tmp = MatrixMultiply(m_corras_s2corras_t, D_src, NULL);
  if (m_vox_s2vox_t == NULL)
    m_vox_s2vox_t = MatrixMultiply(D_trg_inv, m_tmp, NULL);
  else
    MatrixMultiply(D_trg_inv, m_tmp, m_vox_s2vox_t);

//#define _MTCRX2RX_DEBUG
#ifdef _MTCRX2RX_DEBUG
  fprintf(stderr, "m_corras_s2corras_t = \n");
  MatrixPrint(stderr, m_corras_s2corras_t);

  fprintf(stderr, "D_trg_inv = \n");
  MatrixPrint(stderr, D_trg_inv);
  fprintf(stderr, "D_src = \n");
  MatrixPrint(stderr, D_src);

  fprintf(stderr, "m_vox_s2vox_t = D_trg_inv * m_corras_s2corras_t * D_src = \n");
  MatrixPrint(stderr, m_vox_s2vox_t);
#endif

  MatrixFree(&m_tmp);
  MatrixFree(&D_src);
  MatrixFree(&D_trg);
  MatrixFree(&D_trg_inv);

  return(m_vox_s2vox_t);
}
