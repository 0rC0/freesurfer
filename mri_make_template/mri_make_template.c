#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "mri.h"
#include "macros.h"
#include "error.h"
#include "diag.h"
#include "proto.h"
#include "mrimorph.h"
#include "transform.h"

int main(int argc, char *argv[]) ;
static int get_option(int argc, char *argv[]) ;

char *Progname ;

static void usage_exit(int code) ;
static MRI *MRIcomputePriors(MRI *mri_priors, int ndof, MRI *mri_char_priors);
int MRIaccumulateMeansAndVariances(MRI *mri, MRI *mri_mean, MRI *mri_std) ;
int MRIcomputeMeansAndStds(MRI *mri_mean, MRI *mri_std, int ndof) ;
int MRIcomputeMaskedMeansAndStds(MRI *mri_mean, MRI *mri_std, MRI *mri_dof) ;
MRI *MRIfloatToChar(MRI *mri_src, MRI *mri_dst) ;
int MRIaccumulateMaskedMeansAndVariances(MRI *mri, MRI *mri_mask,MRI *mri_dof,
                                         float low_val, float hi_val, 
                                         MRI *mri_mean, MRI *mri_std) ;
static MRI *MRIupdatePriors(MRI *mri_binary, MRI *mri_priors) ;
static MRI *MRIcomputePriors(MRI *mri_priors, int ndof, MRI *mri_char_priors);

static char *transform_fname = NULL ;
static char *T1_name = "T1" ;
static char *var_fname = NULL ;
static char *binary_name = NULL ;

/* just for T1 volume */
#define T1_MEAN_VOLUME        0
#define T1_STD_VOLUME         1

static int first_transform = 0 ;

#define BUILD_PRIORS             0
#define ON_STATS                 1
#define OFF_STATS                2


#define NPARMS  12
static MATRIX *m_xforms[NPARMS] ;
static MATRIX *m_xform_mean = NULL ;
static MATRIX *m_xform_covariance = NULL ;
static char *xform_mean_fname = NULL ;
static char *xform_covariance_fname = NULL ;
static int stats_only = 0 ;

int
main(int argc, char *argv[])
{
  char   **av, subjects_dir[STRLEN], *cp ;
  int    ac, nargs, i, dof, no_transform, which, sno = 0, nsubjects = 0 ;
  MRI    *mri, *mri_mean = NULL, *mri_std, *mri_T1,*mri_binary,*mri_dof=NULL,
         *mri_priors = NULL ;
  char   *subject_name, *out_fname, fname[STRLEN] ;

  Progname = argv[0] ;
  ErrorInit(NULL, NULL, NULL) ;
  DiagInit(NULL, NULL, NULL) ;

  cp = getenv("SUBJECTS_DIR") ;
  if (!cp)
    ErrorExit(ERROR_BADPARM, "%s: SUBJECTS_DIR not defined in environment.\n",
              Progname) ;
  strcpy(subjects_dir, cp) ;
  ac = argc ;
  av = argv ;
  for ( ; argc > 1 && ISOPTION(*argv[1]) ; argc--, argv++)
  {
    nargs = get_option(argc, argv) ;
    argc -= nargs ;
    argv += nargs ;
  }

  if (argc < 3)
    usage_exit(1) ;

  out_fname = argv[argc-1] ;

  no_transform = first_transform ;
  if (binary_name)   /* generate binarized volume with priors and */
  {                  /* separate means and variances */
    for (which = BUILD_PRIORS ; which <= OFF_STATS ; which++)
    {
      /* for each subject specified on cmd line */
      for (dof = 0, i = 1 ; i < argc-1 ; i++) 
      {
        if (*argv[i] == '-')   /* don't do transform for next subject */
        { no_transform = 1 ; continue ; }
        dof++ ;
        subject_name = argv[i] ;
        if (which != BUILD_PRIORS)
        {
          sprintf(fname, "%s/%s/mri/%s", subjects_dir, subject_name, T1_name);
          fprintf(stderr, "%d of %d: reading %s...\n", i, argc-2, fname) ;
          mri_T1 = MRIread(fname) ;
          if (!mri_T1)
            ErrorExit(ERROR_NOFILE,"%s: could not open volume %s",
                      Progname,fname);
        }
        
        sprintf(fname, "%s/%s/mri/%s",subjects_dir,subject_name,binary_name);
        fprintf(stderr, "%d of %d: reading %s...\n", i, argc-2, fname) ;
        mri_binary = MRIread(fname) ;
        if (!mri_binary)
          ErrorExit(ERROR_NOFILE,"%s: could not open volume %s",
                    Progname,fname);

        /* only count voxels which are mostly labeled */
        MRIbinarize(mri_binary, mri_binary, WM_MIN_VAL, 0, 100) ;
        if (transform_fname && no_transform-- <= 0)
        {
          int       type ;
          MORPH_3D  *m3d ;
          LTA       *lta ;
          MRI       *mri_tmp ;
          
          sprintf(fname, "%s/%s/mri/transforms/%s", 
                  subjects_dir, subject_name, transform_fname) ;
          
          fprintf(stderr, "reading transform %s...\n", fname) ;
          type = TransformFileNameType(fname) ;
          switch (type)
          {
          default:
          case TRANSFORM_ARRAY_TYPE:
            lta = LTAread(fname) ;
            if (!lta)
              ErrorExit(ERROR_NOFILE, 
                        "%s: could not open transform file %s\n",
                        Progname, fname) ;
            if (which != BUILD_PRIORS)
            {
              mri_tmp = LTAtransform(mri_T1, NULL, lta) ;
              MRIfree(&mri_T1) ; mri_T1 = mri_tmp ;
            }
            mri_tmp = LTAtransform(mri_binary, NULL, lta) ;
            MRIfree(&mri_binary) ; mri_binary = mri_tmp ;
            LTAfree(&lta) ;
            break ;
          case MORPH_3D_TYPE:
            m3d = MRI3DreadSmall(fname) ;
            if (!m3d)
              ErrorExit(ERROR_NOFILE, 
                        "%s: could not open transform file %s\n",
                        Progname, transform_fname) ;
            fprintf(stderr, "applying transform...\n") ;
            if (which != BUILD_PRIORS)
            {
              mri_tmp = MRIapply3DMorph(mri_T1, m3d, NULL) ;
              MRIfree(&mri_T1) ; mri_T1 = mri_tmp ;
            }
            mri_tmp = MRIapply3DMorph(mri_binary, m3d, NULL) ;
            MRIfree(&mri_binary) ; mri_binary = mri_tmp ;
            MRI3DmorphFree(&m3d) ;
            break ;
          }
          fprintf(stderr, "transform application complete.\n") ;
        }
        if (which == BUILD_PRIORS)
        {
          mri_priors = 
            MRIupdatePriors(mri_binary, mri_priors) ;
        }
        else
        {
          if (!mri_mean)
          {
            mri_dof = MRIalloc(mri_T1->width, mri_T1->height, mri_T1->depth, 
                               MRI_UCHAR) ;
            mri_mean = 
              MRIalloc(mri_T1->width, mri_T1->height,mri_T1->depth,MRI_FLOAT);
            mri_std = 
              MRIalloc(mri_T1->width,mri_T1->height,mri_T1->depth,MRI_FLOAT);
            if (!mri_mean || !mri_std)
              ErrorExit(ERROR_NOMEMORY, "%s: could not allocate templates.\n",
                        Progname) ;
          }
        
          fprintf(stderr, "updating mean and variance estimates...\n") ;
          if (which == ON_STATS)
          {
            MRIaccumulateMaskedMeansAndVariances(mri_T1, mri_binary, mri_dof,
                                                 90, 100, mri_mean, mri_std) ;
            fprintf(stderr, "T1 = %d, binary = %d, mean = %2.1f\n",
                    MRIvox(mri_T1, 141,100,127), 
                    MRIvox(mri_binary, 141,100,127),
                    MRIFvox(mri_mean, 141,100,127)) ;
          }
          else  /* computing means and vars for off */
            MRIaccumulateMaskedMeansAndVariances(mri_T1, mri_binary, mri_dof,
                                                 0, WM_MIN_VAL-1, 
                                                 mri_mean, mri_std) ;
          MRIfree(&mri_T1) ; 
        }
        MRIfree(&mri_binary) ;
      }

      if (which == BUILD_PRIORS)
      {
        mri = MRIcomputePriors(mri_priors, dof, NULL) ;
        MRIfree(&mri_priors) ;
        fprintf(stderr, "writing priors to %s...\n", out_fname) ;
      }
      else
      {
        MRIcomputeMaskedMeansAndStds(mri_mean, mri_std, mri_dof) ;
        mri_mean->dof = dof ;
        mri = MRIfloatToChar(mri_mean, NULL) ;
        
        fprintf(stderr, "writing T1 means with %d dof to %s...\n", mri->dof, 
                out_fname) ;
        if (!which)
          MRIwrite(mri, out_fname) ;
        else
          MRIappend(mri, out_fname) ;
        MRIfree(&mri_mean) ; MRIfree(&mri) ;
        fprintf(stderr, "writing T1 variances to %s...\n", out_fname);
        mri = MRIfloatToChar(mri_std, NULL) ;
        MRIfree(&mri_std) ; 
        if (dof <= 1)
          MRIreplaceValues(mri, mri, 0, 1) ;
      }

      if (!which)
        MRIwrite(mri, out_fname) ;
      else
        MRIappend(mri, out_fname) ;
      MRIfree(&mri) ;
    }
  }
  else
  {
    /* for each subject specified on cmd line */
    if (xform_mean_fname)
    {
      m_xform_mean = MatrixAlloc(4,4,MATRIX_REAL) ;
      /*      m_xform_covariance = MatrixAlloc(12,12,MATRIX_REAL) ;*/
    }
    for (dof = 0, i = 1 ; i < argc-1 ; i++) 
    {
      if (*argv[i] == '-')   /* don't do transform for next subject */
      { no_transform = 1 ; continue ; }
      dof++ ;
      subject_name = argv[i] ;
      sprintf(fname, "%s/%s/mri/%s", subjects_dir, subject_name, T1_name);
      fprintf(stderr, "%d of %d: reading %s...\n", i, argc-2, fname) ;
      mri_T1 = MRIread(fname) ;
      if (!mri_T1)
        ErrorExit(ERROR_NOFILE,"%s: could not open volume %s",Progname,fname);
      if (transform_fname && no_transform-- <= 0)
      {
        int       type ;
        MORPH_3D  *m3d ;
        LTA       *lta ;
        MRI       *mri_tmp ;
        
        sprintf(fname, "%s/%s/mri/transforms/%s", 
                subjects_dir, subject_name, transform_fname) ;
        
        fprintf(stderr, "reading transform %s...\n", fname) ;
        type = TransformFileNameType(fname) ;
        switch (type)
        {
        default:
        case MNI_TRANSFORM_TYPE:
        case TRANSFORM_ARRAY_TYPE:
          lta = LTAread(fname) ;
          if (!lta)
            ErrorExit(ERROR_NOFILE, "%s: could not open transform file %s\n",
                      Progname, fname) ;
          if (xform_mean_fname)
          {
            MATRIX *m_ras ;

            m_ras = MRIvoxelXformToRasXform(mri_T1,mri_T1,lta->xforms[0].m_L,
                                            NULL);
            m_xforms[sno] = m_ras ;
            MatrixAdd(m_xform_mean, m_xforms[sno], m_xform_mean) ;
            sno++ ;
          }
          if (stats_only)
            continue ;
          mri_tmp = LTAtransform(mri_T1, NULL, lta) ;
          LTAfree(&lta) ;
          break ;
        case MORPH_3D_TYPE:
          m3d = MRI3DreadSmall(fname) ;
          if (!m3d)
            ErrorExit(ERROR_NOFILE, "%s: could not open transform file %s\n",
                      Progname, transform_fname) ;
          fprintf(stderr, "applying transform...\n") ;
          mri_tmp = MRIapply3DMorph(mri_T1, m3d, NULL) ;
          MRI3DmorphFree(&m3d) ;
          break ;
        }
        MRIfree(&mri_T1) ; mri_T1 = mri_tmp ;
        fprintf(stderr, "transform application complete.\n") ;
      }
      if (!mri_mean)
      {
        mri_mean = 
          MRIalloc(mri_T1->width, mri_T1->height, mri_T1->depth, MRI_FLOAT) ;
        mri_std = 
          MRIalloc(mri_T1->width, mri_T1->height, mri_T1->depth, MRI_FLOAT) ;
        if (!mri_mean || !mri_std)
          ErrorExit(ERROR_NOMEMORY, "%s: could not allocate templates.\n",
                    Progname) ;
      }

      MRIfree(&mri_T1) ;
    }

    if (xform_mean_fname)
    {
      FILE   *fp ;
      VECTOR *v = NULL, *vT = NULL ;
      MATRIX *m_vvT = NULL ;
      int    rows, cols ;

      nsubjects = sno ;

      fp = fopen(xform_covariance_fname, "w") ;
      if (!fp)
        ErrorExit(ERROR_NOFILE, "%s: could not open covariance file %s",
                  Progname, xform_covariance_fname) ;
      fprintf(fp, "nsubjects=%d\n", nsubjects) ;

      MatrixScalarMul(m_xform_mean, 1.0/(double)nsubjects, m_xform_mean) ;
      printf("means:\n") ;
      MatrixPrint(stdout, m_xform_mean) ;
      MatrixAsciiWrite(xform_mean_fname, m_xform_mean) ;

      /* subtract the mean from each transform */
      rows = m_xform_mean->rows ; cols = m_xform_mean->cols ; 
      for (sno = 0 ; sno < nsubjects ; sno++)
      {
        MatrixSubtract(m_xforms[sno], m_xform_mean, m_xforms[sno]) ;
        v = MatrixReshape(m_xforms[sno], v, rows*cols, 1) ;
        vT = MatrixTranspose(v, vT) ;
        m_vvT = MatrixMultiply(v, vT, m_vvT) ;
        if (!m_xform_covariance)
          m_xform_covariance = 
            MatrixAlloc(m_vvT->rows, m_vvT->cols,MATRIX_REAL) ;
        MatrixAdd(m_vvT, m_xform_covariance, m_xform_covariance) ;
        MatrixAsciiWriteInto(fp, m_xforms[sno]) ;
      }

      MatrixScalarMul(m_xform_covariance, 1.0/(double)nsubjects, 
                      m_xform_covariance) ;
      printf("covariance:\n") ;
      MatrixPrint(stdout, m_xform_covariance) ;
      MatrixAsciiWriteInto(fp, m_xform_covariance) ;
      fclose(fp) ;
      if (stats_only)
        exit(0) ;
    }

    MRIcomputeMeansAndStds(mri_mean, mri_std, dof) ;

    mri_mean->dof = dof ;
    mri = MRIfloatToChar(mri_mean, NULL) ;
      
    fprintf(stderr, "\nwriting T1 means with %d dof to %s...\n", mri->dof, 
            out_fname) ;
    MRIwrite(mri, out_fname) ;
    MRIfree(&mri_mean) ; MRIfree(&mri) ;
    fprintf(stderr, "\nwriting T1 variances to %s...\n", out_fname);
    mri = MRIfloatToChar(mri_std, NULL) ;
    if (dof <= 1) /* can't calulate variances - set them to reasonable val */
      MRIreplaceValues(mri, mri, 0, 1) ;
    if (!var_fname)
      MRIappend(mri, out_fname) ;
    else
      MRIwrite(mri, var_fname) ;
    MRIfree(&mri_std) ; MRIfree(&mri) ;

  }

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
  if (!stricmp(option, "T1"))
  {
    T1_name = argv[2] ;
    fprintf(stderr,"reading T1 volume from directory '%s'\n",T1_name) ;
    nargs = 1 ;
  }
  else if (!stricmp(option, "statsonly"))
  {
    stats_only = 1 ;
  }
  else switch (toupper(*option))
  {
  case 'X':
    xform_mean_fname = argv[2] ;
    xform_covariance_fname = argv[3] ;
    printf("writing means (%s) and covariances (%s) of xforms\n",
           xform_mean_fname, xform_covariance_fname) ;
    nargs = 2 ;
    break ;
  case 'V':
    var_fname = argv[2] ;
    nargs = 1 ;
    fprintf(stderr, "writing variances to %s...\n", var_fname) ;
    break ;
  case 'B':
    binary_name = argv[2] ;
    fprintf(stderr, "generating binary template from %s volume\n", 
            binary_name) ;
    nargs = 1 ;
    break ;
  case 'N':
    first_transform = 1 ;  /* don't use transform on first volume */
    break ;
  case 'T':
    transform_fname = argv[2] ;
    fprintf(stderr, "applying transformation %s to each volume\n", 
            transform_fname) ;
    nargs = 1 ;
    break ;
  case '?':
  case 'U':
    usage_exit(0) ;
    break ;
  default:
    fprintf(stderr, "unknown option %s\n", argv[1]) ;
    exit(1) ;
    break ;
  }

  return(nargs) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
usage_exit(int code)
{
  printf("usage: %s <subject> <subject> ... <output volume>\n", Progname) ;
  exit(code) ;
}

int
MRIaccumulateMeansAndVariances(MRI *mri, MRI *mri_mean, MRI *mri_std)
{
  int    x, y, z, width, height, depth ;
  float  val, *pmean, *pstd ;
  
  width = mri->width ; height = mri->height ; depth = mri->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pmean = &MRIFvox(mri_mean, 0, y, z) ;
      pstd = &MRIFvox(mri_std, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == 128 && y == 128 && z == 128)
          DiagBreak() ;
        val = MRIvox(mri,x,y,z) ;
#if 1
        *pmean++ += val ;
        *pstd++ += val*val ;
#else
        MRIFvox(mri_mean,x,y,z) += val ;
        MRIFvox(mri_std,x,y,z) += val*val ;
#endif
      }
    }
  }
  return(NO_ERROR) ;
}

int
MRIcomputeMeansAndStds(MRI *mri_mean, MRI *mri_std, int ndof)
{
  int    x, y, z, width, height, depth ;
  float  sum, sum_sq, mean, var ;
  
  width = mri_std->width ; height = mri_std->height ; depth = mri_std->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        if (x == 128 && y == 128 && z == 128)
          DiagBreak() ;
        sum = MRIFvox(mri_mean,x,y,z) ;
        sum_sq = MRIFvox(mri_std,x,y,z) / ndof ;
        mean = MRIFvox(mri_mean,x,y,z) = sum / ndof ; 
        var = sum_sq - mean*mean; 
        MRIFvox(mri_std,x,y,z) = sqrt(var) ;
      }
    }
  }
  return(NO_ERROR) ;
}
int
MRIcomputeMaskedMeansAndStds(MRI *mri_mean, MRI *mri_std, MRI *mri_dof)
{
  int    x, y, z, width, height, depth, ndof ;
  float  sum, sum_sq, mean, var ;
  
  width = mri_std->width ; height = mri_std->height ; depth = mri_std->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        if (x == 128 && y == 128 && z == 128)
          DiagBreak() ;
        sum = MRIFvox(mri_mean,x,y,z) ;
        ndof = MRIvox(mri_dof, x, y, z) ;
        if (!ndof)   /* variance will be 0 in any case */
          ndof = 1 ;
        sum_sq = MRIFvox(mri_std,x,y,z) / ndof ;
        mean = MRIFvox(mri_mean,x,y,z) = sum / ndof ; 
        var = sum_sq - mean*mean; 
        MRIFvox(mri_std,x,y,z) = sqrt(var) ;
      }
    }
  }
  return(NO_ERROR) ;
}

MRI *
MRIfloatToChar(MRI *mri_src, MRI *mri_dst)
{
  int   width, height, depth/*, x, y, z, out_val*/ ;
  /*  float fmax, fmin ;*/
  
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  if (!mri_dst)
    mri_dst = MRIalloc(width, height, depth, MRI_UCHAR) ;
#if 1
  MRIcopy(mri_src, mri_dst) ;
#else
  MRIvalRange(mri_src, &fmin, &fmax) ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
      }
    }
  }
#endif
  return(mri_dst) ;
}

#if 0
MRI *
MRIbinarizeEditting(MRI *mri_src, MRI *mri_dst)
{
  int     width, height, depth, x, y, z, val ;
  BUFTYPE *
  
  width = mri_src->width ; height = mri_src->height ; depth = mri_src->depth ; 
  mri_dst = MRIalloc(width, height, depth, MRI_UCHAR) ;
  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  MRIvalRange(mri_src, &fmin, &fmax) ;
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
      }
    }
  }

  return(mri_dst) ;
}
#endif
int
MRIaccumulateMaskedMeansAndVariances(MRI *mri, MRI *mri_mask, MRI *mri_dof,
                                     float low_val, 
                                     float hi_val,MRI *mri_mean,MRI *mri_std)
{
  int     x, y, z, width, height, depth ;
  float   val ;
  BUFTYPE *pmask, mask ;
  
  width = mri->width ; height = mri->height ; depth = mri->depth ;

  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pmask = &MRIvox(mri_mask, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == 128 && y == 128 && z == 128)
          DiagBreak() ;
        mask = *pmask++ ;
        if (mask >= low_val && mask <= hi_val)
        {
          val = MRIvox(mri,x,y,z) ;
          MRIFvox(mri_mean,x,y,z) += val ;
          MRIFvox(mri_std,x,y,z) += val*val ;
          MRIvox(mri_dof,x,y,z)++ ;
        }
      }
    }
  }
  return(NO_ERROR) ;
}
static MRI *
MRIupdatePriors(MRI *mri_binary, MRI *mri_priors)
{
  int     width, height, depth, x, y, z ;
  BUFTYPE *pbin ;
  float   prob ;

  width = mri_binary->width ; 
  height = mri_binary->height ; 
  depth = mri_binary->depth ;
  if (!mri_priors)
  {
    mri_priors = MRIalloc(width, height, depth, MRI_FLOAT) ;
  }
  
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pbin = &MRIvox(mri_binary, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == 128 && y == 128 && z == 128)
          DiagBreak() ;
        prob = *pbin++ / 100.0f ;
        MRIFvox(mri_priors, x, y, z) += prob ;
      }
    }
  }
  return(mri_priors) ;
}

static MRI *
MRIcomputePriors(MRI *mri_priors, int ndof, MRI *mri_char_priors)
{
  int     width, height, depth, x, y, z ;
  BUFTYPE *pchar_prior, char_prior ;
  float   *pprior, prior ;

  width = mri_priors->width ; 
  height = mri_priors->height ; 
  depth = mri_priors->depth ;
  if (!mri_char_priors)
  {
    mri_char_priors = MRIalloc(width, height, depth, MRI_UCHAR) ;
  }
  
  for (z = 0 ; z < depth ; z++)
  {
    for (y = 0 ; y < height ; y++)
    {
      pprior = &MRIFvox(mri_priors, 0, y, z) ;
      pchar_prior = &MRIvox(mri_char_priors, 0, y, z) ;
      for (x = 0 ; x < width ; x++)
      {
        if (x == 128 && y == 128 && z == 128)
          DiagBreak() ;
        prior = *pprior++ ;
        if (prior > 0)
          DiagBreak() ;
        if (prior > 10)
          DiagBreak() ;
        char_prior = (BUFTYPE)nint(100.0f*prior/(float)ndof) ;
        if (char_prior > 101)
          DiagBreak() ;
        *pchar_prior++ = char_prior ;
      }
    }
  }
  return(mri_char_priors) ;
}
