/*
 *       FILE NAME:   image.c
 *
 *       DESCRIPTION: image processing utilities
 *
 *       AUTHOR:      Bruce Fischl
 *       DATE:        2/5/96
 *
 * replaced alloc_image with alloc_image_buffer (dng, 3/12/96).
*/

/*-----------------------------------------------------
                    INCLUDE FILES
-------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <fcntl.h>
#include <unistd.h>

#include <hipl_format.h>

#include "hips.h"

#include "image.h"
#include "error.h"
#include "matrix.h"
#include "matfile.h"
#include "utils.h"
#include "macros.h"
#include "hmem.h"
#include "machine.h"
#include "proto.h"
#include "diag.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/

/*-----------------------------------------------------
                    STATIC PROTOTYPES
-------------------------------------------------------*/
static int alloc_image_buffer(struct header *hd) ;

/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE   *
ImageAlloc(int rows, int cols, int format, int nframes)
{
  IMAGE *I ;

  I = (IMAGE *)calloc(1, sizeof(IMAGE)) ;
  if (!I)
    ErrorExit(ERROR_NO_MEMORY, "ImageAlloc: could not allocate header\n") ;

  init_header(I, "orig", "seq", nframes, "today", rows,cols,format,1, "temp");
  alloc_image_buffer(I) ;
  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           stolen from hips2 code and modified to allocate multiple frames.
------------------------------------------------------*/
static int
alloc_image_buffer(struct header *hd)
{
  int fcb,cb;

  if (hd->sizeimage == (hsize_t) 0)  /*dng*/
  {
    hd->imdealloc = (h_boolean) FALSE; /*dng*/
    return(HIPS_OK);
  }
  if((hd->image = hcalloc(hd->sizeimage*hd->num_frame,sizeof(byte))) == 
     (byte *)NULL)    return(HIPS_ERROR);
  if (hd->pixel_format == PFMSBF || hd->pixel_format == PFLSBF) 
  {
    fcb = hd->fcol/8;
    cb = (hd->ocols + 7)/8;
    hd->firstpix = hd->image + ((cb * hd->frow) + fcb);
  }
  else
    hd->firstpix = hd->image +
      (((long)hd->ocols * (long)hd->frow) + (long)hd->fcol) * hd->sizepix;
  hd->imdealloc = TRUE;
  hmemset(hd->image, 0, hd->sizeimage*hd->num_frame) ;
  return(HIPS_OK);
}

/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageFree(IMAGE **pI)
{
  IMAGE *I = *pI ;

  if (!I)
    ErrorExit(ERROR_BADPARM, "ImageFree: null pointer") ;

  free_header(I) ;
  *pI = NULL ;
  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageWrite(IMAGE *I, char *fname)
{
  FILE  *fp ;
  int   ecode ;

  fp = fopen(fname, "wb") ;
  if (!fp)
    ErrorReturn(-1, (ERROR_NO_FILE, "ImageWrite(%s) failed\n", fname)) ;

  ecode = ImageFWrite(I, fp, fname) ;
  fclose(fp) ;
  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageFWrite(IMAGE *I, FILE *fp, char *fname)
{
  int ecode, frame ;
  byte *image ;

  if (!fname)
    fname = "ImageFWrite" ;

  ecode = fwrite_header(fp,I,"fwrite") ;
  if (ecode != HIPS_OK)
    ErrorExit(ERROR_NO_FILE, "ImageFWrite: fwrite_header failed (%d)\n",ecode);

  image = I->image ;
  for (frame = 0 ; frame < I->num_frame ; frame++)
  {
    ecode = fwrite_image(fp, I, frame, "fwrite") ;
    if (ecode != HIPS_OK)
      ErrorExit(ERROR_NO_FILE, 
              "ImageFWrite: fwrite_image frame %d failed (%d)\n",ecode,frame);
    I->image += I->sizeimage ;  /* next frame */
  }
  I->image = image ;
  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageUpdateHeader(IMAGE *I, char *fname)
{
  FILE  *fp ;
  int   ecode ;

  fp = fopen(fname, "r+b") ;
  if (!fp)
    ErrorReturn(-1, (ERROR_NO_FILE, "ImageUpdateHeader(%s) failed\n", fname)) ;

  ecode = fwrite_header(fp, I, fname) ;
  if (ecode != HIPS_OK)
    ErrorReturn(-1, (ERROR_NO_FILE, 
                  "ImageUpdateHeader: fwrite_header failed (%d)\n",ecode)) ;

  fclose(fp) ;
  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageFRead(FILE *fp, char *fname, int start, int nframes)
{
  int    ecode, end_frame, frame ;
  IMAGE  *I ;
  byte   *startpix ;

  if (!fname)
    fname = "ImageFRead" ;

  I = (IMAGE *)calloc(1, sizeof(IMAGE)) ;
  if (!I)
    ErrorExit(ERROR_NO_MEMORY,"ImageFRead: could not allocate header\n") ;

  ecode = fread_header(fp, I, fname) ;
  if (ecode != HIPS_OK)
    ErrorExit(ERROR_NO_FILE, "ImageFRead: fread_header failed (%d)\n",ecode);

  if (start < 0)    /* read all frames */
  {
    start = 0 ;
    nframes = I->num_frame ;
  }
  else              /* read only specified frames */
  {
    if (fseek(fp, I->sizeimage*start, SEEK_CUR) < 0)
    {
      ImageFree(&I) ;
      ErrorReturn(NULL, 
                  (ERROR_BADFILE, 
                   "ImageFRead(%s, %d) - could not seek to specified frame",
                   fname, start)) ;
    }
  }

  if (nframes < 0)
    nframes = I->num_frame - start + 1 ;

  end_frame = start + nframes - 1 ;
  if (end_frame >= I->num_frame)
    ErrorReturn(NULL, 
                (ERROR_BADFILE,
                 "ImageFRead(%s, %d) - frame out of bounds", fname, end_frame));
  I->num_frame = nframes ;
  alloc_image_buffer(I) ;

  startpix = I->image ;
  for (frame = start ; frame <= end_frame ; frame++)
  {
    ecode = fread_image(fp, I, frame, fname) ;
    if (ecode != HIPS_OK)
      ErrorExit(ERROR_NO_FILE, "ImageFRead: fread_image failed (%d)\n", ecode);
    I->image += I->sizeimage ;
  }
  I->image = startpix ;
  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageReadFrames(char *fname, int start, int nframes)
{
  IMAGE *I ;
  FILE  *fp ;

  fp = fopen(fname, "rb") ;
  if (!fp)
    ErrorReturn(NULL, (ERROR_NO_FILE, "ImageReadFrames(%s) fopen failed\n", 
                       fname)) ;

  I = ImageFRead(fp, fname, start, nframes) ;
  fclose(fp) ;
  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageReadHeader(char *fname)
{
  IMAGE   *I = NULL ;
  FILE    *fp ;
  int     type, frame, ecode ;
  char    buf[100] ;

  strcpy(buf, fname) ;   /* don't destroy callers string */
  fname = buf ;
  ImageUnpackFileName(fname, &frame, &type, fname) ;

  fp = fopen(fname, "rb") ;
  if (!fp)
    ErrorReturn(NULL, (ERROR_NO_FILE, "ImageReadHeader(%s, %d) failed\n", 
                       fname, frame)) ;

  I = (IMAGE *)calloc(1, sizeof(IMAGE)) ;
  if (!I)
    ErrorExit(ERROR_NO_MEMORY, "ImageReadHeader: could not allocate header\n") ;
  ecode = fread_header(fp, I, fname) ;
  if (ecode != HIPS_OK)
  {
    fclose(fp) ;
    ErrorReturn(NULL, (ERROR_NO_FILE, 
                     "ImageReadHeader(%s): fread_header failed (%d)\n", 
                     fname, ecode)) ;
  }
  fclose(fp) ;

  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
          read an image from file and convert it to the specified
          format.
------------------------------------------------------*/
IMAGE *
ImageReadType(char *fname, int pixel_format)
{
  IMAGE *Itmp, *I ;

  Itmp = ImageRead(fname) ;
  if (Itmp->pixel_format != pixel_format)
  {
    I = ImageAlloc(Itmp->rows, Itmp->cols, pixel_format, Itmp->num_frame) ;
    ImageCopy(Itmp, I) ;
    ImageFree(&Itmp) ;
  }
  else
    I = Itmp ;

  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageRead(char *fname)
{
  IMAGE   *I = NULL ;
  MATRIX  *mat ;
  FILE    *fp ;
  int     type, frame ;
  char    buf[100] ;

  strcpy(buf, fname) ;   /* don't destroy callers string */
  fname = buf ;
  ImageUnpackFileName(fname, &frame, &type, fname) ;
  DiagPrintf(DIAG_WRITE, "ImageRead: unpacking filename\n");

  switch (type)
  {
  case MATLAB_IMAGE:
    DiagPrintf(DIAG_WRITE, 
               "ImageRead: fname=%s, frame=%d, type=%d (M=%d,H=%d)\n",
               fname, frame, type , MATLAB_IMAGE, HIPS_IMAGE);
    mat = MatlabRead(fname) ;
    if (!mat)
      ErrorReturn(NULL, (ERROR_NO_FILE, "ImageRead(%s) failed\n", fname)) ;
    I = ImageFromMatrix(mat, NULL) ;
    h_invert(I,I) ;
    MatrixFree(&mat) ;
    break ;
  case HIPS_IMAGE:
    fp = fopen(fname, "rb") ;
    if (!fp)
      ErrorReturn(NULL, (ERROR_NO_FILE, "ImageRead(%s, %d) failed\n", 
                         fname, frame)) ;
    I = ImageFRead(fp, fname, frame, 1) ;
    fclose(fp) ;
    break ;
  default:
    break ;
  }
  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageThreshold(IMAGE *Isrc, IMAGE *Idst, float threshold)
{
  int      ecode ;
  Pixelval p ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols,Isrc->pixel_format,
                      Isrc->num_frame);

  p.v_float = threshold ;
  ecode = h_softthresh(Isrc, Idst, &p) ;
  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageDFT(IMAGE *Isrc, IMAGE *Idst)
{
  /*float    loglen ;*/
  int      ecode ;
  IMAGE    *Itmp ;
  /*Pixelval p ;*/

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, PFCOMPLEX, 1) ;

  if (Isrc->pixel_format == PFBYTE)  /* must convert to float */
  {
    Itmp = ImageAlloc(Isrc->rows, Isrc->cols, PFFLOAT, 1) ;
    ImageCopy(Isrc, Itmp) ;
    Isrc = Itmp ;
  }
  else
    Itmp = NULL ;

  hips_rtocplx = CPLX_RVI0 ;
  ecode = h_toc(Isrc, Idst) ;
  if  (ecode != HIPS_OK)
  {
    ImageFree(&Idst) ;
    ErrorReturn(NULL, (ecode, "ImageDFT: h_fourtr failed (%d)\n", ecode)) ;
  }
#if 0
  loglen = log2(Isrc->rows) ;
  if ((Isrc->rows == Isrc->cols) && (floor(loglen) == loglen)) /* FFT */
  {
  }
  else   /* not a power of 2 - use DFT */
  {
  }
#endif
  ecode = h_fourtr(Idst) ;
  if  (ecode != HIPS_OK)
    ErrorExit(ecode, "ImageDFT: h_fourtr failed (%d)\n", ecode) ;

#if 0
  /* h_divscale can't handle complex quantities */
  p.v_complex[REAL_PIX] = 1.0f/(float)(Idst->numpix) ;
  p.v_complex[IMAG_PIX] = 0.0f ;
  ImageMulScale(Idst, Idst, &p) ;
#endif

  if (Itmp)
    ImageFree(&Itmp) ;

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           perform the inverse fourier transform of the input image,
           scaling the output by 1/n.
------------------------------------------------------*/
IMAGE *
ImageInverseDFT(IMAGE *Isrc, IMAGE *Idst)
{
  /*float    loglen ;*/
  IMAGE    *Itmp ;
  int      ecode ;
  Pixelval p ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, PFFLOAT, 1) ;
  Itmp = ImageAlloc(Isrc->rows, Isrc->cols, PFCOMPLEX, 1) ;

  ImageCopy(Isrc, Itmp) ;

#if 0
  loglen = log2(Isrc->rows) ;
  if ((Isrc->rows == Isrc->cols) && (floor(loglen) == loglen)) /* FFT */
  {
  }
  else   /* not a power of 2 - use DFT */
  {
  }
#endif
  ecode = h_invfourtr(Itmp) ;
  if  (ecode != HIPS_OK)
    ErrorExit(ecode, "ImageInverseDFT: h_invfourtr failed (%d)\n", ecode) ;

  hips_cplxtor = CPLX_REAL ;
  h_tof(Itmp, Idst) ;
#if 1
  p.v_float = (float)Idst->numpix ;
  ecode = h_divscale(Idst, Idst, &p) ;
  if  (ecode != HIPS_OK)
    ErrorExit(ecode, "ImageInverseDFT: h_divscale failed (%d)\n", ecode) ;
#endif
  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageMul(IMAGE *Isrc1, IMAGE *Isrc2, IMAGE *Idst)
{
  int   ecode ;

  if (!Idst)
    Idst = ImageAlloc(Isrc1->rows, Isrc1->cols, 
                      Isrc1->pixel_format, Isrc1->num_frame) ;

  ecode = h_mul(Isrc1, Isrc2, Idst) ;
  if (ecode != HIPS_OK)
    ErrorExit(ecode, "ImageMul: h_mul failed (%d)\n", ecode) ;

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageResize(IMAGE *Isrc, IMAGE *Idst, int drows, int dcols)
{
  float x_scale, y_scale ;
  int   ecode ;

  if (!Idst)
    Idst = ImageAlloc(drows, dcols, Isrc->pixel_format, Isrc->num_frame) ;

  x_scale = (float)dcols / (float)Isrc->cols ;
  y_scale = (float)drows / (float)Isrc->rows ;

  if (FEQUAL(x_scale, 1.0f))
    ImageCopy(Isrc, Idst) ;
  else
    switch (Isrc->pixel_format)
    {
    case PFBYTE:
#if 0
      ecode = h_affine(Isrc, Idst, x_scale, 0.0f, 0.0f, 0.0f, y_scale, 0.0f) ;
      if (ecode != HIPS_OK)
        ErrorExit(ecode,
                  "ImageResize: h_affine(%2.3f, %2.3f) returned %d\n",ecode);
#else
      if (x_scale > 1.0f)
        ecode = h_enlarge(Isrc, Idst, nint(x_scale), nint(y_scale) ) ;
      else 
        ecode = h_reduce(Isrc, Idst, nint(1.0f/x_scale), nint(1.0f/y_scale)) ;
      if (ecode != HIPS_OK)
        ErrorExit(ecode,
                  "ImageResize: h_%s(%2.3f, %2.3f) returned %d\n",
                  x_scale > 1.0f ? "enlarge" : "reduce", ecode);
#endif
      break ;
    default:
      if (x_scale > 1.0f)
        ecode = h_enlarge(Isrc, Idst, nint(x_scale), nint(y_scale)) ;
      else 
        ecode = h_reduce(Isrc, Idst, nint(1.0f/x_scale), nint(1.0f/y_scale)) ;
      if (ecode != HIPS_OK)
        ErrorExit(ecode,
                  "ImageResize: h_%s(%2.3f, %2.3f) returned %d\n",
                  x_scale > 1.0f ? "enlarge" : "reduce", ecode);
      break ;
    }
  
  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageCopy(IMAGE *Isrc, IMAGE *Idst)
{
  int  old, ecode, frame, nframes ;
  byte *src_image, *dst_image ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, 
                      Isrc->num_frame) ;

  src_image = Isrc->image ;
  dst_image = Idst->image ;
  nframes = Isrc->num_frame ;
  Isrc->num_frame = Idst->num_frame = 1 ;
  
  for (frame = 0 ; frame < nframes ; frame++)
  {
    if (Idst->pixel_format == Isrc->pixel_format)
    {
      ecode = h_copy(Isrc, Idst) ;
      if (ecode != HIPS_OK)
        ErrorExit(ecode, "ImageCopy: h_copy failed (%d)\n", ecode) ;
    }
    else
    {
      switch (Idst->pixel_format)
      {
      case PFFLOAT:
        old = hips_cplxtor ;
        hips_cplxtor = CPLX_REAL ;
        ecode = h_tof(Isrc, Idst) ;
        if (ecode != HIPS_OK)
          ErrorExit(ecode, "ImageCopy: h_tof failed (%d)\n", ecode) ;
        hips_cplxtor = old ;
        break ;
      case PFCOMPLEX:
        old = hips_rtocplx ;
        hips_rtocplx = CPLX_RVI0 ;
        ecode = h_toc(Isrc, Idst) ;
        if (ecode != HIPS_OK)
          ErrorExit(ecode, "ImageCopy: h_toc failed (%d)\n", ecode) ;
        hips_rtocplx = old ;
        break ;
      case PFDBLCOM:
        old = hips_rtocplx ;
        hips_rtocplx = CPLX_RVI0 ;
        ecode = h_todc(Isrc, Idst) ;
        if (ecode != HIPS_OK)
          ErrorExit(ecode, "ImageCopy: h_todc failed (%d)\n", ecode) ;
        hips_rtocplx = old ;
        break ;
      case PFINT:
        ecode = h_toi(Isrc, Idst) ;
        if (ecode != HIPS_OK)
          ErrorExit(ecode, "ImageCopy: h_toi failed (%d)\n", ecode) ;
        break ;
      case PFBYTE:
        ecode = h_tob(Isrc, Idst) ;
        if (ecode != HIPS_OK)
          ErrorExit(ecode, "ImageCopy: h_tob failed (%d)\n", ecode) ;
        break ;
      default:
        ErrorExit(ERROR_UNSUPPORTED,
                  "ImageCopy %d-->%d, unsupported conversion\n",
                  Isrc->pixel_format, Idst->pixel_format) ;
        break ;
      }
    }
    Isrc->firstpix += Isrc->sizeimage ;
    Isrc->image += Isrc->sizeimage ;
    if (Idst != Isrc)
    {
      Idst->firstpix += Idst->sizeimage ;
      Idst->image += Idst->sizeimage ;
    }
  }

  Isrc->firstpix = Isrc->image = src_image ;
  Idst->firstpix = Idst->image = dst_image ;
  Isrc->num_frame =  Idst->num_frame = nframes ;

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#define DISCEDGE_VARCRIT  0.0f
#define DISCEDGE_SIZE     7

IMAGE   *
ImageEdgeDetect(IMAGE *Isrc, IMAGE *Idst, float sigma, int wsize, 
                float lthresh, float uthresh, int dothin)
{
  int    ecode ;
  IMAGE  *Iout ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, PFBYTE, Isrc->num_frame) ;

  ImageScale(Isrc, Isrc, .0f, 255.0f) ;
  if (Idst->pixel_format != PFBYTE)
    Iout = ImageAlloc(Isrc->rows, Isrc->cols, PFBYTE, Isrc->num_frame) ;
  else
    Iout = Idst ;

  ecode = h_canny(Isrc, Iout, sigma, wsize, lthresh, uthresh, dothin) ;
  if (Iout != Idst)
  {
    ImageCopy(Iout, Idst) ;
    ImageFree(&Iout) ;
  }

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE   *
ImageCorrelate(IMAGE *Itemplate, IMAGE *Isrc, int zeropad, IMAGE *Icorr)
{
  IMAGE *Iconj, *Ifcorr, *Ifsrc ;
  int   ecode ;

  if (zeropad)
    ErrorReturn(NULL, 
  (ERROR_UNSUPPORTED, "ImageCorrelate: zero padding unsupported")) ;

  /* assumes the template as already been FTed */
  Iconj = ImageConjugate(Itemplate, NULL) ;
  Ifsrc = ImageDFT(Isrc, NULL) ;
  Ifcorr = ImageMul(Iconj, Ifsrc, NULL) ;
  Icorr = ImageInverseDFT(Ifcorr, Icorr) ;

  ecode = h_flipquad(Icorr, Icorr) ;
  if (ecode != HIPS_OK)
    ErrorExit(ecode, "ImageCorrelate: h_flipquad failed (%d)\n", ecode) ;

#if 0
ImageWrite(Itemplate, "Itemplate.hipl") ;
ImageWrite(Isrc, "Isrc.hipl") ;
ImageWrite(Ifsrc, "Ifsrc.hipl") ;
ImageWrite(Iconj, "Iconj.hipl") ;
ImageWrite(Ifcorr, "Ifcorr.hipl") ;
ImageWrite(Icorr, "Iflip.hipl") ;
#endif

  ImageFree(&Iconj) ;
  ImageFree(&Ifcorr) ;
  ImageFree(&Ifsrc) ;

  return(Icorr) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#if 0
IMAGE   *
ImageCopyArea(IMAGE *Isrc, IMAGE *Idst, int srow, int scol,
            int drow, int dcol, int rows, int cols)
{
  return(Idst) ;
}
#endif
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageClearArea(IMAGE *I, int r0, int c0, int rows, int cols, float val)
{
  float   *fptr ;
  int     row, col ;
  byte    *cptr, cval ;

  rows = MIN(I->rows, r0+rows) ;
  cols = MIN(I->cols, c0+cols) ;
  
  for (row = r0 ; row < rows ; row++)
  {
    switch (I->pixel_format)
    {
    case PFFLOAT:
      fptr = IMAGEFpix(I, c0, row) ;
      for (col = c0 ; col < cols ; col++)
        *fptr++ = val ;
      break ;
    case PFBYTE:
      cptr = IMAGEpix(I, c0, row) ;
      cval = (char)val ;
      for (col = c0 ; col < cols ; col++)
        *cptr++ = cval ;
      break ;
    default:
      ErrorReturn(ERROR_UNSUPPORTED, (ERROR_UNSUPPORTED, "ImageClearArea: only handles PFFLOAT")) ;
      break ;
    }
  }
  return(0) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
float
ImageFindPeak(IMAGE *I, int *prow, int *pcol, float *pval)
{
  float  max_val, *fpix, val ;
  int    max_row = -1, max_col = -1, row, col, rows, cols ;

  if (I->pixel_format != PFFLOAT)
    ErrorReturn(0.0f, (ERROR_UNSUPPORTED, "ImageFindPeak: only supports PFFLOAT")) ;

  rows = I->rows ;
  cols = I->cols ;

  fpix = IMAGEFpix(I, 0, 0) ;
  max_val = -1000000.0f ;
  for (row = 0 ; row < rows ; row++)
  {
    for (col = 0 ; col < cols ; col++)
    {
      val = *fpix++ ;
      if (val >= max_val)
      {
        max_val = val ;
        max_row = row ;
        max_col = col ;
      }
    }
  }

  *prow = max_row ;
  *pcol = max_col ;
  *pval = max_val ;
  return(max_val) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImagePowerSpectrum(IMAGE *Isrc, IMAGE *Idst)
{
  IMAGE  *Idft, *Iconj ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, PFFLOAT, Isrc->num_frame) ;

  Idft = ImageAlloc(Isrc->rows, Isrc->cols, PFCOMPLEX, Isrc->num_frame) ;

  if (Isrc->pixel_format != PFCOMPLEX)  /* not FFT'd yet */
    ImageDFT(Isrc, Idft) ;
  else
    ImageCopy(Isrc, Idft) ;

  Iconj = ImageConjugate(Idft, NULL) ;

  ImageMul(Idft, Iconj, Iconj) ;
  ImageCopy(Iconj, Idst) ;       /* change it to floating point */

  ImageFree(&Iconj) ;

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
#if 0
static int
imageLargeEnough(IMAGE *Isrc, IMAGE *Idst)
{
  if (Isrc->num_frame > Idst->num_frame)
    return(0) ;
  if (Isrc->numpix > Idst->numpix)
    return(0) ;

  return(1) ;
}
#endif
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageNormalizePix(IMAGE *Isrc, IMAGE *Idst)
{
  float  scale, fmin = 0.0f, fmax = 0.0f ;
  int    ecode ;
  Pixelval  pmin, pmax ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, 
                      Isrc->num_frame) ;

  ecode = h_minmax(Isrc, &pmin, &pmax, 0) ;
  switch (Isrc->pixel_format)
  {
  case PFBYTE:
    fmin = (float)pmin.v_byte ;
    fmax = (float)pmax.v_byte ;
    break ;
  case PFFLOAT:
    fmin = pmin.v_float ;
    fmax = pmax.v_float ;
    break ;
  default:
    ErrorExit(ERROR_UNSUPPORTED, 
              "ImageNormalize: unsupported pixel format %d\n",
              Isrc->pixel_format) ;
    break ;
  }

  if (ecode != HIPS_OK)
    ErrorReturn(NULL, (ecode,"ImageNormalize: h_minmax failed (%d)\n", ecode));

  if (FEQUAL(fmax, fmin))
    ErrorReturn(NULL, (ERROR_BADPARM, "ImageNormalize: constant image")) ;

  scale = 1.0f / (fmax - fmin) ;
  fmin = -fmin * scale ;

  ecode = h_linscale(Isrc, Idst, scale, fmin) ;
  if (ecode != HIPS_OK)
    ErrorReturn(NULL, 
                (ecode, "ImageNormalize: h_linscale failed (%d)\n", ecode)) ;

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageConjugate(IMAGE *Isrc, IMAGE *Idst)
{
  CPIX       *spix, *dpix ;
  int        npix, i ;

  npix = Isrc->orows * Isrc->ocols * Isrc->num_frame ;
  switch (Isrc->pixel_format)
  {
  case PFCOMPLEX:
    if (!Idst)
      Idst = ImageAlloc(Isrc->rows, Isrc->cols, PFCOMPLEX, Isrc->num_frame) ;
    spix = (CPIX *)IMAGECpix(Isrc, 0, 0) ;
    dpix = (CPIX *)IMAGECpix(Idst, 0, 0) ;
    for (i = 0 ; i < npix ; i++, spix++, dpix++)
    {
      dpix->real = spix->real ;
      dpix->imag = -spix->imag ;
    }
    break ;
  default:
    ErrorExit(ERROR_UNSUPPORTED, 
              "ImageConjugate: unsupported pixel format %d\n", 
              Isrc->pixel_format) ;
    break ;
  }

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
Pixelval
ImageAccum(IMAGE *Isrc)
{
  Pixelval retval ;
  int      row, col, endrow, endcol ;
  float    real, imag ;
  CPIX     *cpix ;

  endrow = Isrc->frow + Isrc->rows ;
  endcol = Isrc->fcol + Isrc->cols ;
  switch (Isrc->pixel_format)
  {
  case PFCOMPLEX:
    real = imag = 0.0f ;
    for (row = Isrc->frow ; row < endrow ; row++)
    {
      cpix = IMAGECpix(Isrc, row, Isrc->fcol) ;
      for (col = Isrc->fcol ; col < endcol ; col++, cpix++)
      {
        real += cpix->real ;
        imag += cpix->imag ;
      }
    }
    retval.v_complex[REAL_PIX] = real ;
    retval.v_complex[IMAG_PIX] = imag ;
    break ;
  default:
    ErrorExit(ERROR_UNSUPPORTED, "ImageAccum: unsupported pixel format %d\n",
              Isrc->pixel_format) ;
  }

  return(retval) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
MATRIX *
ImageToMatrix(IMAGE *I)
{
  MATRIX *mat ;
  int    format = 0, bytes ;

  switch (I->pixel_format)
  {
  case PFCOMPLEX:
    format = MATRIX_COMPLEX ;
    break ;
  case PFFLOAT:
    format = MATRIX_REAL ;
    break ;
  default:
    ErrorExit(ERROR_UNSUPPORTED, "ImageToMatrix: unsupported image type %d",
              I->pixel_format) ;
    break ;
  }

  mat = MatrixAlloc(I->rows, I->cols, format) ;
  bytes = mat->rows * mat->cols * sizeof(float) ;
  if (mat->type == MATRIX_COMPLEX)
    bytes *= 2 ;

  hmemcpy(mat->data, I->image, bytes) ; 
  return(mat) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageFromMatrix(MATRIX *matrix, IMAGE *I)
{
  int    format, bytes ;

  format = (matrix->type == MATRIX_COMPLEX) ? PFCOMPLEX : PFFLOAT ;

  if (!I)
    I = ImageAlloc(matrix->rows, matrix->cols, format, 1) ;
  else
    if (I->rows != matrix->rows || I->cols != matrix->cols)
      ErrorExit(ERROR_BADPARM, "ImageFromMatrix: size mismatch") ;

  bytes = matrix->rows * matrix->cols * sizeof(float) ;
  if (matrix->type == MATRIX_COMPLEX)
    bytes *= 2 ;

  hmemcpy(I->image, matrix->data, bytes) ; 
  return(I) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageInverse(IMAGE *Isrc, IMAGE *Idst)
{
  MATRIX *mat, *mat_inverse ;

  mat = ImageToMatrix(Isrc) ;
  mat_inverse = MatrixInverse(mat, NULL) ;
  Idst = ImageFromMatrix(mat_inverse, Idst) ;
  MatrixFree(&mat) ;
  MatrixFree(&mat_inverse) ;
  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
IMAGE *
ImageMatrixMul(IMAGE *Isrc1, IMAGE *Isrc2, IMAGE *Idst)
{
  MATRIX *mat1, *mat2, *mat_dst ;

  if (Isrc2->rows != Isrc1->cols)
    ErrorExit(ERROR_BADPARM, 
              "ImageMatrixMul: inner dimensions must agree (%d, %d)",
              Isrc1->cols, Isrc2->rows) ;

  mat1 = ImageToMatrix(Isrc1) ;
  mat2 = ImageToMatrix(Isrc2) ;
  mat_dst = MatrixMultiply(mat1, mat2, NULL) ;
  Idst = ImageFromMatrix(mat_dst, Idst) ;
  MatrixFree(&mat1) ;
  MatrixFree(&mat2) ;
  MatrixFree(&mat_dst) ;

  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageType(char *fname)
{
  char *dot, buf[200] ;

  strcpy(buf, fname) ;
  dot = strrchr(buf, '.') ;

  if (dot)
  {
    dot = StrUpper(dot+1) ;
    if (!strcmp(dot, "MAT"))
      return(MATLAB_IMAGE) ;
  }

  return(HIPS_IMAGE) ;
} 
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageFrame(char *fname)
{
  char *colon, buf[200] ;
  int   frame ;

  strcpy(buf, fname) ;
  colon = strrchr(buf, ':') ;

  if (colon)
  {
    sscanf(colon+1, "%d", &frame) ;
    *colon = 0 ;
  }
  else
    frame = 0 ;

  return(frame) ;
} 
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
           since h_linscale only outputs FLOAT images (and doesn't
           complain otherwise!), we must allocate a temp. image for
           output purposes unless the supplied one is already in
           float format.
------------------------------------------------------*/
IMAGE *
ImageScale(IMAGE *Isrc, IMAGE *Idst, float new_min, float new_max)
{
  float  scale, old_min, old_max ;
  int    ecode, nframes, frame ;
  Pixelval pmin, pmax ;
  IMAGE  *Iout ;
  byte   *src_image, *out_image ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, 
                      Isrc->num_frame) ;

  old_min = old_max = 0.0f ;  /* remove warning */

  if (Idst->pixel_format != PFFLOAT)  /* h_linscale outputs floats */
    Iout = ImageAlloc(Isrc->rows, Isrc->cols, PFFLOAT, Isrc->num_frame) ;
  else
    Iout = Idst ;


  src_image = Isrc->image ;
  out_image = Iout->image ;
  nframes = Isrc->num_frame ;
  Isrc->num_frame =  Iout->num_frame = 1 ;
  for (frame = 0 ; frame < nframes ; frame++)
  {
    ecode = h_minmax(Isrc, &pmin, &pmax, 0) ;
    switch (Isrc->pixel_format)
    {
    case PFBYTE:
      old_min = (float)pmin.v_byte ;
      old_max = (float)pmax.v_byte ;
      break ;
    case PFINT:
      old_min = (float)pmin.v_int ;
      old_max = (float)pmax.v_int ;
      break ;
    case PFFLOAT:
      old_min = pmin.v_float ;
      old_max = pmax.v_float ;
      break ;
    default:
      ErrorExit(ERROR_UNSUPPORTED, 
                "ImageScale: unsupported pixel format %d\n",
                Isrc->pixel_format) ;
      break ;
    }
    
    if (ecode != HIPS_OK)
      ErrorExit(ecode, "ImageScale: h_minmax failed (%d)\n", ecode) ;
    
    scale = (new_max - new_min) / (old_max - old_min) ;
    if (FEQUAL(old_max, old_min))
      ErrorReturn(NULL, (ERROR_BADPARM, "ImageScale: constant image")) ;


    ecode = h_linscale(Isrc, Iout, scale, new_min - old_min * scale) ;
    if (ecode != HIPS_OK)
      ErrorReturn(NULL, (ecode,"ImageScale: h_linscale failed (%d)\n",ecode));

    Isrc->firstpix += Isrc->sizeimage ;
    Isrc->image += Isrc->sizeimage ;
    if (Isrc != Iout)
    {
      Iout->firstpix += Iout->sizeimage ;
      Iout->image += Iout->sizeimage ;
    }
  }

  Isrc->firstpix = Isrc->image = src_image ;
  Iout->firstpix = Iout->image = out_image ;
  Isrc->num_frame =  Iout->num_frame = nframes ;

  if (Iout != Idst)  /* copy it to desired format */
  {
    ImageCopy(Iout, Idst) ;
    ImageFree(&Iout) ;
  }
  return(Idst) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description
------------------------------------------------------*/
int
ImageCheckSize(IMAGE *inImage,IMAGE *outImage, int rows, int cols, int nframes)
{
  long  inPix, outPix ;

  if (!outImage)
    return(0) ;

  if (!rows)
    rows = inImage->rows ;
  if (!cols)
    cols = inImage->cols ;
  if (!nframes)
    nframes = inImage->num_frame ;

  inPix = (long)rows * (long)cols * (long)nframes * (long)inImage->sizepix ;
  outPix = (long)outImage->numpix * (long)outImage->sizepix * 
           (long)outImage->num_frame ;

  return(outPix >= inPix) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageCopyFrames(IMAGE *inImage, IMAGE *outImage,int start, int nframes, 
               int dst_frame)
{
  byte  *cIn, *cOut ;
  unsigned int  *iIn, *iOut ;
  float *fsrc, *fdst ;
  int   size, frameno, pix_per_frame, end ;

  if (!ImageCheckSize(inImage, outImage, 0, 0, dst_frame + nframes))
  {
    fprintf(stderr, "ImageCopyFrames: outImage not large enough\n") ;
    return(-1) ;
  }

  end = start + nframes - 1 ;
  pix_per_frame = inImage->rows * inImage->cols ;
  for (frameno = start ; frameno <= end ; frameno++)
  {
    size = inImage->rows * inImage->cols ;
    switch (inImage->pixel_format)
    {
    case PFFLOAT:
      if (outImage->pixel_format != PFFLOAT)
        ErrorExit(ERROR_UNSUPPORTED, 
                  "ImageCopyFrames: unsupported image pixel format %d -> %d\n",
                  inImage->pixel_format, outImage->pixel_format) ;

      fsrc = IMAGEFseq_pix(inImage, 0, 0, frameno) ;
      fdst = IMAGEFseq_pix(outImage, 0, 0, dst_frame+frameno-start) ;
      hmemcpy((char *)fdst, (char *)fsrc, pix_per_frame*sizeof(float)) ;
      break ;
    case PFBYTE:
      if (outImage->pixel_format == PFBYTE)
        hmemcpy(IMAGEseq_pix(inImage,0,0,frameno),
               IMAGEseq_pix(outImage, 0, 0,frameno),
               pix_per_frame * sizeof(char)) ;
      else
      {
        size = inImage->rows * inImage->cols ;
        cIn = IMAGEseq_pix(inImage, 0, 0, frameno) ;
        switch (outImage->pixel_format)
        {
        case PFFLOAT:
          fdst = IMAGEFseq_pix(outImage, 0, 0, frameno) ;
          while (size--)
            *fdst++ = (float)*cIn++ ;
          break ;
        case PFBYTE:
          cOut = (byte *)IMAGEseq_pix(outImage, 0, 0, frameno) ;
          while (size--)
            *cOut++ = *cIn++ ;
          break ;
        case PFINT:
          iOut = (unsigned int *)IMAGEIseq_pix(outImage, 0, 0, frameno) ;
          while (size--)
            *iOut++ = (UINT)*cIn++ ;
          break ;
      default:
          ErrorExit(ERROR_UNSUPPORTED, 
                    "ImageCopyFrames: unsupported image pixel format %d -> %d\n"
                    , inImage->pixel_format, outImage->pixel_format) ;
          break ;
        }
        break ;
      }
    
    case PFINT:
      iIn = IMAGEIpix(inImage, 0, 0) + pix_per_frame * frameno ;
      switch (outImage->pixel_format)
      {
      case PFINT:
        iOut = IMAGEIpix(outImage, 0, 0) + pix_per_frame * frameno ;
        hmemcpy((char *)iOut, (char *)iIn, pix_per_frame*sizeof(int)) ;
        break ;
      case PFBYTE:
        cOut = IMAGEpix(outImage, 0, 0) + pix_per_frame * frameno ;
        while (size--)
          *cOut++ = (UCHAR)*iIn++ ;
        break ;
      default:
        ErrorExit(ERROR_UNSUPPORTED, 
                  "ImageCopyFrames: unsupported image pixel format %d -> %d\n",
                  inImage->pixel_format, outImage->pixel_format) ;
        break ;
      }
      break ;
    default:
      ErrorExit(ERROR_UNSUPPORTED, 
                "ImageCopyFrames: unsupported image pixel format %d -> %d\n",
                inImage->pixel_format, outImage->pixel_format) ;
      break ;
    }
  }

  outImage->rows = inImage->rows ;
  outImage->cols = inImage->cols ;

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int  
ImageScaleRange(IMAGE *image, float fmin, float fmax, int low, int high)
{
  int    size;
  byte   *csrc, cmin_val, cmax_val, cval ;
  int    *isrc, imin_val, imax_val, ival ;
  float  *fsrc,  fval, norm ;

  size = image->cols * image->rows ;
  switch (image->pixel_format)
  {
  case PFBYTE:
    cmax_val = (UCHAR)fmax ;
    cmin_val = (UCHAR)fmin ;
    size = image->cols * image->rows ;
    csrc = IMAGEpix(image, 0, 0) ;
    norm = ((float)high - (float)low) / 
      ((float)cmax_val - (float)cmin_val) ;
    while (size--)
    {
      cval = *csrc ;
      fval = (float)(cval - cmin_val) * norm ;
      cval = (byte)((byte)fval +(byte)low) ;
      *csrc++ = cval ;
    }
    break ;

  case PFINT:
    imax_val = (int)fmax ;
    imin_val = (int)fmin ;
    size = image->cols * image->rows ;
    isrc = (int *)IMAGEIpix(image, 0, 0) ;
    norm = ((float)high - (float)low) / ((float)imax_val-(float)imin_val);
    while (size--)
    {
      ival = *isrc ;
      ival = (int)((float)((float)ival - (float)imin_val) * norm) + low ;
      *isrc++ = ival ;
    }
    break ;

  case PFFLOAT:
    size = image->cols * image->rows ;
    fsrc = (float *)IMAGEFpix(image, 0, 0) ;
    norm = ((float)high - (float)low) / (fmax-fmin);
    while (size--)
    {
      fval = *fsrc ;
      fval = ((fval - fmin)*norm) + (float)low ;
      *fsrc++ = fval ;
    }
    break ;

  default:
    fprintf(stderr, "ImageScale: unsupported format %d\n", image->pixel_format);
    exit(1) ;
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:
              fname - the name of the file to read from

           Description:
              read a hips image from a file, and allocate an image
              header and data space for it.  Returns the newly
              allocated image.
----------------------------------------------------------------------*/
int
ImageReadInto(char *fname, IMAGE *I, int image_no)
{
  FILE   *fp ;
  int    ecode ;

  fp = fopen(fname, "rb") ;
  if (!fp)
    ErrorPrintf(ERROR_NO_FILE, "ImageReadInto(%s) failed\n", fname) ;
  
  ecode = fread_header(fp, I, fname) ;
  if (ecode != HIPS_OK)
    ErrorExit(ERROR_NO_FILE, "ImageReadInto(%s): fread_header failed (%d)\n", 
              fname, ecode) ;
  ecode = fread_image(fp, I, image_no, fname) ;
  if (ecode != HIPS_OK)
    ErrorExit(ERROR_NO_FILE, "ImageReadInto(%s): fread_image failed (%d)\n", 
              fname, ecode) ;
  
  fclose(fp) ;
  
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:
              image - the image to write
              fname - the name of the file to write to.

           Description:
              write a hips image to file 'fname'
----------------------------------------------------------------------*/
int
ImageWriteFrames(IMAGE *image, char *fname, int start, int nframes)
{
  IMAGE  *tmp_image ;
  
  tmp_image = ImageAlloc(image->cols, image->rows,image->pixel_format,nframes);
  ImageCopyFrames(image, tmp_image, start, nframes, 0) ;
  ImageWrite(tmp_image, fname) ;
  ImageFree(&tmp_image) ;
  return(NO_ERROR);
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageRescale(IMAGE *inImage, IMAGE *outImage, float scale)
{
  int rows, cols ;

  cols = nint((float)inImage->cols * scale) ;
  rows = nint((float)inImage->rows * scale) ;
  if (!outImage)
    outImage = ImageAlloc(rows, cols, inImage->pixel_format,inImage->num_frame);

  if (scale == 1)
    ImageCopy(inImage, outImage) ;
  else if (scale > 1)
    ImageScaleUp(inImage, outImage, scale) ;
  else
    ImageScaleDown(inImage, outImage, scale) ;

  return(outImage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageScaleDown(IMAGE *inImage, IMAGE *outImage, float scale)
{
  int    inRow, inCol, outRow, outCol, inCols, inRows, 
         outRows, outCols, frame ;
  UCHAR  *outPix ;
  byte   *in_image, *out_image ;
  float  *foutPix ;

  if (!ImageCheckSize(inImage, outImage, nint((float)inImage->rows*scale),
                          nint((float)inImage->cols*scale), inImage->num_frame) )
  {
    fprintf(stderr, "ImageScaleDown: output image not big enough\n") ;
    return(-1) ;
  }

  outImage->cols = nint((float)inImage->cols * scale) ;
  outImage->rows = nint((float)inImage->rows * scale) ;
  inRows = inImage->rows ;
  inCols = inImage->cols ;
  outRows = outImage->rows ;
  outCols = outImage->cols ;

  in_image = inImage->image ;
  out_image = outImage->image ;
  for (frame = 0 ; frame < inImage->num_frame ; frame++)
  {
    switch (inImage->pixel_format)
    {
    case PFBYTE:
      switch (outImage->pixel_format)
      {
      case PFFLOAT:  /* byte --> float */
        foutPix = IMAGEFpix(outImage, 0, 0) ;
        for (outRow = 0 ; outRow < outRows ; outRow++)
          for (outCol = 0 ; outCol < outCols ; outCol++, foutPix++)
          {
            /* map center point to this output point */
            inRow = nint((float)outRow / scale) ;
            inCol = nint((float)outCol / scale) ;
            *foutPix = (float)*IMAGEpix(inImage, inCol, inRow) ;
          }
        break ;
      case PFBYTE:  /* byte --> byte */
        outPix = IMAGEpix(outImage, 0, 0) ;
        for (outRow = 0 ; outRow < outRows ; outRow++)
          for (outCol = 0 ; outCol < outCols ; outCol++, outPix++)
          {
            /* map center point to this output point */
            inRow = nint((float)outRow / scale) ;
            inCol = nint((float)outCol / scale) ;
            if (inRow >= inRows || outRow >= outRows ||
                inCol >= inCols || outCol >= outCols)
            {
              fprintf(stderr, "in: %d, %d --> out: %d, %d!\n",
                      inRow, inCol, outRow, outCol) ;
              exit(2) ;
            }
            *outPix = *IMAGEpix(inImage, inCol, inRow) ;
          }
        break ;
      default:
        fprintf(stderr, "ImageScaleDown: unsupported output pixel format %d\n",
                outImage->pixel_format) ;
        return(-1) ;
        break ;
      }
      break ;
    case PFFLOAT:   /* float --> byte */
      switch (outImage->pixel_format)
      {
      case PFBYTE:
        outPix = IMAGEpix(outImage, 0, 0) ;
        for (outRow = 0 ; outRow < outRows ; outRow++)
          for (outCol = 0 ; outCol < outCols ; outCol++, outPix++)
          {
            /* map center point to this output point */
            inRow = nint((float)outRow / scale) ;
            inCol = nint((float)outCol / scale) ;
            *outPix = (UCHAR)*IMAGEFpix(inImage, inCol, inRow) ;
          }
        break ;
      case PFFLOAT:
        foutPix = IMAGEFpix(outImage, 0, 0) ;
        for (outRow = 0 ; outRow < outRows ; outRow++)
          for (outCol = 0 ; outCol < outCols ; outCol++, foutPix++)
          {
            /* map center point to this output point */
            inRow = nint((float)outRow / scale) ;
            inCol = nint((float)outCol / scale) ;
            *foutPix = (float)*IMAGEFpix(inImage, inCol, inRow) ;
          }
        break ;
      default:
        fprintf(stderr, "ImageScaleDown: unsupported output pixel format %d\n",
                outImage->pixel_format) ;
        return(-1) ;
        break ;
      }
      break ;
    case PFINT:
      
    default:
      fprintf(stderr, "ImageScaleDown: unsupported pixel format %d\n", 
              inImage->pixel_format) ;
      return(-2) ;
    }
    inImage->image += inImage->sizeimage ;
    inImage->firstpix += inImage->sizeimage ;
    if (inImage != outImage)
    {
      outImage->image += outImage->sizeimage ;
      outImage->firstpix += outImage->sizeimage ;
    }
  }
  inImage->image = inImage->firstpix = in_image ;
  outImage->image = outImage->firstpix = out_image ;
  

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageScaleUp(IMAGE *inImage, IMAGE *outImage, float scale)
{
  int    inRow, inCol, outRow, outCol, inCols, inRows, endCol, endRow,
         outRows, outCols, frame ;
  UCHAR  *inPix, *outPix ;
  UINT   *inIPix, *outIPix ;
  float  *finPix, *foutPix ;
  byte   *in_image, *out_image ;

  if (!ImageCheckSize(inImage, outImage, nint(inImage->rows*scale),
                          nint(inImage->cols*scale), inImage->num_frame))
  {
    fprintf(stderr, 
          "ImageScaleUp: output image not large enough %d x %d -> %d x %d\n",
            inImage->rows, inImage->cols, outImage->rows, outImage->cols);
    return(-1) ;
  }

  outCols = outImage->cols = nint((float)inImage->cols * scale) ;
  outRows = outImage->rows = nint((float)inImage->rows * scale) ;

  inRows = inImage->rows ;
  inCols = inImage->cols ;

  in_image = inImage->image ;
  out_image = outImage->image ;
  for (frame = 0 ; frame < inImage->num_frame ; frame++)
  {
    switch (inImage->pixel_format)
    {
    case PFBYTE:
      switch (outImage->pixel_format)
      {
      case PFBYTE:
        outPix = IMAGEpix(outImage, 0, 0) ;
        for (outRow = 0 ; outRow < outRows ; outRow++)
        {
          for (outCol = 0 ; outCol < outCols ; outCol++)
          {
            inCol = (int)((float)outCol / scale) ;
            inRow = (int)((float)outRow / scale) ;
            inPix = IMAGEpix(inImage, inCol, inRow) ;
            *outPix++ = *inPix ;
          }
        }
        break ;
      case PFFLOAT:
        inPix = IMAGEpix(inImage, 0, 0) ;
        for (inRow = 0 ; inRow < inRows ; inRow++)
          for (inCol = 0 ; inCol < inCols ; inCol++, inPix++)
          {
            /* fill in a scale x scale area in the output image */
            endRow = nint( (float)inRow * scale + scale) ;
            endCol = nint( (float)inCol * scale + scale) ;
            for (outRow = nint( (float)inRow *scale);outRow < endRow ;outRow++)
            {
              foutPix = IMAGEFpix(outImage, nint((float)inCol * scale),outRow);
              
              for (outCol = nint( (float)inCol * scale);outCol < endCol ; 
                   outCol++,foutPix++)
                *foutPix = (float)(*inPix) ;
            }
          }
        break ;
      default:
        fprintf(stderr, "ImageScaleUp: unsupported output pixel format %d\n", 
                outImage->pixel_format) ;
        return(-1) ;
        break ;
      }
      break ;
    case PFFLOAT:
      switch (outImage->pixel_format)
      {
      case PFBYTE:
        finPix = IMAGEFpix(inImage, 0, 0) ;
        for (inRow = 0 ; inRow < inRows ; inRow++)
          for (inCol = 0 ; inCol < inCols ; inCol++, finPix++)
          {
            /* fill in a scale x scale area in the output image */
            endRow = nint( (float)inRow * scale + scale) ;
            endCol = nint( (float)inCol * scale + scale) ;
            for (outRow = nint( (float)inRow*scale);outRow < endRow ;outRow++)
            {
              outPix = IMAGEpix(outImage, nint((float)inCol * scale), outRow) ;
              
              for (outCol = nint( (float)inCol * scale) ; outCol < endCol ; 
                   outCol++, outPix++)
                *outPix = (UCHAR)(*finPix) ;
            }
          }
        break ;
      case PFFLOAT:
        finPix = IMAGEFpix(inImage, 0, 0) ;
        for (inRow = 0 ; inRow < inRows ; inRow++)
          for (inCol = 0 ; inCol < inCols ; inCol++, finPix++)
          {
            /* fill in a scale x scale area in the output image */
            endRow = nint( (float)inRow * scale + scale) ;
            endCol = nint( (float)inCol * scale + scale) ;
            for (outRow = nint( (float)inRow*scale);outRow < endRow ;outRow++)
            {
              foutPix = IMAGEFpix(outImage, nint((float)inCol * scale),outRow);
              
              for (outCol = nint( (float)inCol * scale) ; outCol < endCol ; 
                   outCol++,foutPix++)
                *foutPix = *finPix ;
            }
          }
        break ;
      default:
        fprintf(stderr, "ImageScaleUp: unsupported output pixel format %d\n", 
                outImage->pixel_format) ;
        return(-1) ;
        break ;
      }
      break ;
    case PFINT:
      inIPix = IMAGEIpix(inImage, 0, 0) ;
      inRows = inImage->rows ;
      inCols = inImage->cols ;
      for (inRow = 0 ; inRow < inRows ; inRow++)
        for (inCol = 0 ; inCol < inCols ; inCol++, inIPix++)
        {
          /* fill in a scale x scale area in the output image */
          endRow = nint( (float)inRow * scale + scale) ;
          endCol = nint( (float)inCol * scale + scale) ;
          for (outRow = nint( (float)inRow * scale);outRow < endRow ; outRow++)
          {
            outIPix = IMAGEIpix(outImage, nint((float)inCol * scale), outRow) ;
            
            for (outCol = nint( (float)inCol * scale); outCol < endCol ; 
                 outCol++, outIPix++)
              *outIPix = *inIPix ;
          }
        }
      break ;
    default:
      fprintf(stderr, "ImageScaleUp: unsupported pixel format %d\n", 
              inImage->pixel_format) ;
      return(-2) ;
    }
    inImage->image += inImage->sizeimage ;
    inImage->firstpix += inImage->sizeimage ;
    if (inImage != outImage)
    {
      outImage->image += outImage->sizeimage ;
      outImage->firstpix += outImage->sizeimage ;
    }
  }

  inImage->image = inImage->firstpix = in_image ;
  outImage->image = outImage->firstpix = out_image ;
  
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageReflect(IMAGE *inImage, IMAGE *outImage, int how)
{
  int           x, y, ymax ;
  unsigned char *src, *dst, *tmp ;
  unsigned int  *isrc, *idst, *itmp ;

  if (!ImageCheckSize(inImage, outImage, 0, 0, 0))
  {
    fprintf(stderr, "ImageReflect: output image not large enough\n") ;
    return(-1) ;
  }
  outImage->rows = inImage->rows ;
  outImage->cols = inImage->cols ;
  switch(inImage->pixel_format)
  {
  case PFBYTE:
    switch (how)
    {
    case IMAGE_REFLECT_AROUND_X_AXIS:
      ymax = inImage->rows - 1 ;
      src = inImage->image ;
      tmp = outImage->image ;
      for (y = 0 ; y < inImage->rows ; y++)
      {
        for (x = 0 ; x < inImage->cols ; x++)
        {
          dst = IMAGEpix(outImage, x, ymax - y) ;
          *dst = *src++ ;
        }
      }
      break ;
    case IMAGE_REFLECT_AROUND_Y_AXIS:
      break ;
    default:
      fprintf(stderr, "ImageReflect: unknown how parm (%d)\n", how) ;
      exit(1) ;
    }
    break ;
  case PFINT:
    switch (how)
    {
    case IMAGE_REFLECT_AROUND_X_AXIS:
      ymax = inImage->rows - 1 ;
      isrc = (unsigned int *)inImage->image ;
      itmp = (unsigned int *)outImage->image ;
      for (y = 0 ; y < inImage->rows ; y++)
      {
        for (x = 0 ; x < inImage->cols ; x++)
        {
          idst = IMAGEIpix(outImage, x, ymax - y) ;
          *idst = *isrc++ ;
        }
      }
      break ;
    case IMAGE_REFLECT_AROUND_Y_AXIS:
      break ;
    default:
      fprintf(stderr, "ImageReflect: unknown how parm (%d)\n", how) ;
      exit(1) ;
    }
    break ;

  default:
    fprintf(stderr, "ImageReflect: unsupported image format %d\n", 
            inImage->pixel_format) ;
    break ;
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              add multiplicative "speckle" noise to an image.
----------------------------------------------------------------------*/
int
ImageAddSpeckleNoise(IMAGE *inImage,IMAGE *outImage, float amp)
{
  int    npix ;
  float  *inPix, *outPix, gnoise ;

  if (inImage->pixel_format != PFFLOAT)
  {
    fprintf(stderr, "ImageAddNoise: unsupported input format %d\n",
            inImage->pixel_format) ;
    return(-1) ;
  }
  if (outImage->pixel_format != PFFLOAT)
  {
    fprintf(stderr, "ImageAddNoise: unsupported output format %d\n",
            outImage->pixel_format) ;
    return(-1) ;
  }

  npix = inImage->rows * inImage->cols * inImage->num_frame ;
  inPix = IMAGEFpix(inImage, 0, 0) ;
  outPix = IMAGEFpix(outImage, 0, 0) ;
  while (npix--)
  {
    gnoise = (float)randomNumber(1.0-(double)amp, 1.0+(double)amp) ;
    *outPix++ += *inPix++ * gnoise ;
  }
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              corrupt an image with salt & pepper noise: randomly 
              generated 0s and 1s.
----------------------------------------------------------------------*/
int
ImageAddSaltNoise(IMAGE *inImage,IMAGE *outImage, float density)
{
  int    npix ;
  float  *inPix, *outPix, gnoise, in ;

  if (inImage->pixel_format != PFFLOAT)
  {
    fprintf(stderr, "ImageAddNoise: unsupported input format %d\n",
            inImage->pixel_format) ;
    return(-1) ;
  }
  if (outImage->pixel_format != PFFLOAT)
  {
    fprintf(stderr, "ImageAddNoise: unsupported output format %d\n",
            outImage->pixel_format) ;
    return(-1) ;
  }

  npix = inImage->rows * inImage->cols * inImage->num_frame ;
  inPix = IMAGEFpix(inImage, 0, 0) ;
  outPix = IMAGEFpix(outImage, 0, 0) ;
  while (npix--)
  {
    gnoise = (float)randomNumber(0.0, 1.0) ;
    in = *inPix++ ;
    if (gnoise < density)
    {
      if (gnoise < density/2.0f)
        in = 0.0f ;
      else
        in = 1.0f ;
    }
    *outPix++ = in ;
  }
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
             corrupt an image with additive zero mean gaussian noise.
----------------------------------------------------------------------*/
int
ImageAddNoise(IMAGE *inImage, IMAGE *outImage, float amp)
{
  int    npix ;
  float  *inPix, *outPix, gnoise ;

  if (inImage->pixel_format != PFFLOAT)
  {
    fprintf(stderr, "ImageAddNoise: unsupported input format %d\n",
            inImage->pixel_format) ;
    return(-1) ;
  }
  if (outImage->pixel_format != PFFLOAT)
  {
    fprintf(stderr, "ImageAddNoise: unsupported output format %d\n",
            outImage->pixel_format) ;
    return(-1) ;
  }

  npix = inImage->rows * inImage->cols * inImage->num_frame ;
  inPix = IMAGEFpix(inImage, 0, 0) ;
  outPix = IMAGEFpix(outImage, 0, 0) ;
  while (npix--)
  {
    gnoise = (float)randomNumber(-(double)amp, (double)amp) ;
    *outPix++ = *inPix++ + gnoise ;
  }
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int  
ImageValRange(IMAGE *image, void *pvmin, void *pvmax)
{
  float  *pfmax, *pfmin, fmax, fmin, *fpix ;
  int    size ;

  size = image->rows * image->cols * image->num_frame ;
  switch (image->pixel_format)
  {
  case PFFLOAT:
    pfmax = (float *)pvmax ;
    pfmin = (float *)pvmin ;
    fpix = IMAGEFpix(image, 0, 0) ;
    fmax = fmin = *fpix ;
    while (size--)
    {
      if (*fpix > fmax)
        fmax = *fpix ;
      if (*fpix < fmin)
        fmin = *fpix ;
      fpix++ ;
    }
    *pfmax = fmax ;
    *pfmin = fmin ;
    break ;
  default:
    fprintf(stderr, "ImageValRange: unsupported pixel format %d\n",
            image->pixel_format) ;
    return(-1);
  }

    
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageCatSeq(IMAGE *Isrc1, IMAGE *Isrc2, IMAGE *Idst)
{
  IMAGE *Itmp ;
  int   num_frame, frameno ;

  if ((Isrc1->rows != Isrc2->rows) || (Isrc1->cols != Isrc2->cols))
    return(NULL) ;

  if (!Idst)
    Idst = ImageAlloc(Isrc1->rows, Isrc1->cols, Isrc1->pixel_format,
                      Isrc1->num_frame + Isrc2->num_frame) ;

  num_frame = Isrc2->num_frame ;
  if (Isrc1)
    num_frame += Isrc1->num_frame ;
  Itmp = ImageAlloc(Isrc2->rows, Isrc2->cols, Isrc2->pixel_format, num_frame) ;
  if (Isrc1)
  {
    ImageCopyFrames(Isrc1, Itmp, 0, Isrc1->num_frame, 0) ;
    frameno = Isrc1->num_frame ;
  }
  else
    frameno = 0 ;
  ImageCopyFrames(Isrc2, Itmp, 0, Isrc2->num_frame, frameno) ;

#if 0
  if (frameno > 0)
    ImageWrite(Itmp, "Itmp.hipl") ;
  if (frameno > 0)
    ImageWrite(Isrc2, "Isrc2.hipl") ;
{
  IMAGE *Itmp3 ;

  static int n = 0 ;
  char fname[100] ;
  sprintf(fname, "new%d.hipl", n) ;
  ImageWrite(Isrc2, fname) ;
  sprintf(fname, "add%d.hipl", n++) ;
  ImageWrite(Itmp, fname) ;

  Itmp3 = ImageAlloc(Itmp->rows, Itmp->cols, Itmp->pixel_format, 1) ;
  ImageCopyFrames(Itmp, Itmp3, Itmp->num_frame-1, 1, 0) ;
  ImageWrite(Itmp3, "Ilast.hipl") ;
  ImageFree(&Itmp3) ;
}
#endif

  if (Isrc1)
    ImageFree(&Isrc1) ;
  return(Itmp) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageMulScale(IMAGE *Isrc, IMAGE *Idst, Pixelval *p)
{
  int    ecode;
  hsize_t size ;
  float  real, imag, sreal, simag ;
  CPIX   *csrc, *cdst ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, 1);

  switch (Isrc->pixel_format)
  {
  case PFCOMPLEX:
    csrc = IMAGECpix(Isrc, 0, 0) ;
    cdst = IMAGECpix(Idst, 0, 0) ;
    real = p->v_complex[REAL_PIX] ;
    imag = p->v_complex[IMAG_PIX] ;
    size = Isrc->numpix ;
    while (size--)
    {
      simag = csrc->imag ;
      sreal = csrc->real ;
      cdst->real = real * sreal - imag * simag ;
      cdst->imag = real * simag + sreal * imag ;
      csrc++ ;
      cdst++ ;
    }
    break ;
  default:
    ecode = h_mulscale(Isrc, Idst, p) ;
    if (ecode != HIPS_OK)
      ErrorExit(ecode, "ImageMulScale: h_mulscale failed (%d)", ecode) ;
    break ;
  }
  return(Idst) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageAddScalar(IMAGE *Isrc, IMAGE *Idst, float scalar)
{
  hsize_t    size ;
  float  *fpix ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, 
                      Isrc->num_frame);

  switch (Isrc->pixel_format)
  {
  case PFFLOAT:
    size = Isrc->numpix * (hsize_t)Isrc->num_frame ;
    fpix = IMAGEFpix(Isrc, 0, 0) ;
    while (size--)
      *fpix++ += scalar ;
    break ;
  default:
    ErrorExit(ERROR_UNSUPPORTED, "ImageAddScalar: unsupported pixel type %d",
              Isrc->pixel_format) ;
    break ;
  }

  return(Idst) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE    *
ImageReplace(IMAGE *Isrc, IMAGE *Idst, float inpix, float outpix)
{
  float  *fin, *fout ;
  byte   *cin, *cout, cinpix, coutpix ;
  hsize_t    npix ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, Isrc->num_frame) ;

  if (Idst->pixel_format != Isrc->pixel_format)
    ErrorReturn(NULL, (ERROR_BADPARM, "ImageReplace: src and dst formats must match")) ;

  npix = Isrc->numpix * (hsize_t)Isrc->num_frame ;
  switch (Isrc->pixel_format)
  {
  case PFFLOAT:
    fin = IMAGEFpix(Isrc, 0, 0) ;
    fout = IMAGEFpix(Idst, 0, 0) ;
    while (npix--)
    {
      if (*fin == inpix)
      {
        *fout++ = outpix ;
        fin++;
      }
      else
        *fout++ = *fin++ ;
    }
    break ;
  case PFBYTE:
    cinpix = (byte)inpix ;
    coutpix = (byte)outpix ;
    cin = IMAGEpix(Isrc, 0, 0) ;
    cout = IMAGEpix(Idst, 0, 0) ;
    while (npix--)
    {
      if (*cin == cinpix)
      {
        *cout++ = coutpix ;
        cin++;
      }
      else
        *cout++ = *cin++ ;
    }
    break ;
  default:
    ErrorReturn(NULL, 
                (ERROR_UNSUPPORTED, "ImageReplace: unsupported pixel format %d", Isrc->pixel_format));
    break ;
  }

  return(Idst) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              decompose a file name, extracting the type and the frame #.
----------------------------------------------------------------------*/
int
ImageUnpackFileName(char *inFname, int *pframe, int *ptype, char *outFname)
{
  char *colon, *dot, buf[100] ;

  strcpy(outFname, inFname) ;
  colon = strrchr(outFname, ':') ;
  dot = strrchr(outFname, '.') ;

  if (colon)   /* : in filename indicates frame # */
  {
    if (sscanf(colon+1, "%d", pframe) < 1)
      *pframe = -1 ;
    *colon = 0 ;
  }
  else
    *pframe = -1 ;

  if (dot)
  {
    dot = StrUpper(strcpy(buf, dot+1)) ;
    if (!strcmp(dot, "MAT"))
      *ptype = MATLAB_IMAGE ;
    else
      *ptype = HIPS_IMAGE ;
  }
  else
    *ptype = HIPS_IMAGE ;

  return(NO_ERROR) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              compare 2 images. Return values are:
              0  - images are the same
              1  - images are linearly independent
              -1 - images are linearly dependent
----------------------------------------------------------------------*/
int
ImageCmp(IMAGE *Isrc, IMAGE *Idst)
{
  int      ret, ecode ;
  IMAGE    *Idiv ;
  Pixelval pmin, pmax ;
  float    fmin, fmax ;

  Idiv = ImageAlloc(Isrc->rows, Isrc->cols, Isrc->pixel_format, 1) ;

  ecode = h_div(Isrc, Idst, Idiv) ;
  if (ecode != HIPS_OK)
    ErrorReturn(-1, (ecode, "ImageCmp: h_div returned %d", ecode)) ;

  ecode = h_minmax(Idiv, &pmin, &pmax, 0) ;
  if (ecode != HIPS_OK)
    ErrorExit(ecode, "ImageCmp: h_minmax failed (%d)\n", ecode) ;

  switch (Isrc->pixel_format)
  {
  case PFBYTE:
    fmin = (float)pmin.v_byte ;
    fmax = (float)pmax.v_byte ;
    break ;
  case PFFLOAT:
    fmin = pmin.v_float ;
    fmax = pmax.v_float ;
    break ;
  default:
    ErrorExit(ERROR_UNSUPPORTED, 
              "ImageCmp: unsupported pixel format %d\n", Isrc->pixel_format);
    break ;
  }

  if (fmin != fmax)  /* if Idiv is constant - they are linearly dependent */
    ret = 1 ;
  else
  {
    if (FEQUAL(fmin, 1.0f))
      ret = 0 ;
    else
      ret = -1 ;
  }

  return(ret);
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              return the number of frames stored in the file 'fname'
----------------------------------------------------------------------*/
int
ImageNumFrames(char *fname)
{
  IMAGE  I ;
  FILE   *fp ;
  int    frame, type, ecode, nframes ;
  char   buf[100] ;

  ImageUnpackFileName(fname, &frame, &type, buf) ;
  fname = buf ;
  if ((frame >= 0) || (type != HIPS_IMAGE))
    return(1) ;

  fp = fopen(fname, "rb") ;
  if (!fp)
    ErrorReturn(ERROR_NO_FILE, 
                (ERROR_NO_FILE, "ImageNumFrame(%s) could not open file\n", 
                 fname)) ;

  ecode = fread_header(fp, &I, fname) ;
  if (ecode != HIPS_OK)
    ErrorReturn(ERROR_NO_FILE,
                (ERROR_NO_FILE, 
                 "ImageNumFrame: fread_header failed (%d)\n",ecode));

  nframes = I.num_frame ;
  fclose(fp) ;
  free_hdrcon(&I) ;
  return(nframes) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              append an image to the end of a hips sequence file, incrementing
              the number of frames recorded in the header.
----------------------------------------------------------------------*/
int
ImageAppend(IMAGE *I, char *fname)
{
  FILE   *fp ;
  int    ecode, frame, nframes ;
  IMAGE  Iheader, *Iframe ;
  char   tmpname[200] ;

  fp = fopen(fname, "r+b") ;
  if (!fp)
    ErrorReturn(-1, (ERROR_NO_FILE, "ImageAppend(%s) failed\n", fname)) ;
  
  ecode = fread_header(fp, &Iheader, fname) ;
  if (ecode != HIPS_OK)
    ErrorExit(ERROR_NO_FILE, "ImageAppend: fread_header failed (%d)\n",ecode);

  /* increment # of frames, and update file header */
  Iheader.num_frame++ ;
  nframes = Iheader.num_frame ;
  if (nframes == 10 || nframes == 100 || nframes == 1000)
  {
    /* header size will grow by 1 byte, must copy whole file (ughhh) */
    fclose(fp) ;
    strcpy(tmpname,FileTmpName(NULL)) ;
    FileRename(fname, tmpname) ;

    /* write out new header */
    fp = fopen(fname, "wb") ;
    if (!fp)
      ErrorReturn(-1, (ERROR_NO_FILE, "ImageAppend(%s) failed\n", fname)) ;
    
    ecode = fwrite_header(fp, &Iheader, fname) ;
    if (ecode != HIPS_OK)
      ErrorExit(ERROR_NO_FILE,"ImageAppend: fwrite_header failed (%d)\n",ecode);

    nframes = Iheader.num_frame - 1 ;
    for (frame = 0 ; frame < nframes ; frame++)
    {
      Iframe = ImageReadFrames(tmpname, frame, 1) ;
      if (!Iframe)
        ErrorReturn(-3, (ERROR_BADFILE, 
                         "ImageAppend: could not read %dth frame", frame)) ;
      ecode = fwrite_image(fp, Iframe, frame, fname) ;
      if (ecode != HIPS_OK)
        ErrorReturn(-4, (ERROR_BADFILE, 
                          "ImageAppend: fwrite_image frame %d failed (%d)\n",
                          ecode,frame));
    }
    unlink(tmpname) ;
  }
  else    /* seek back to start and increment # of frames */
  {
    if (fseek(fp, 0L, SEEK_SET) < 0)
      ErrorReturn(-2, (ERROR_BADFILE,"ImageAppend(%s): could not seek to end"));
    ecode = fwrite_header(fp, &Iheader, fname) ;
    if (ecode != HIPS_OK)
      ErrorExit(ERROR_NO_FILE,"ImageAppend: fwrite_header failed (%d)\n",ecode);
  }

  if (fseek(fp, 0L, SEEK_END) < 0)
    ErrorReturn(-2, (ERROR_BADFILE, "ImageAppend(%s): could not seek to end"));

  ecode = fwrite_image(fp, I, frame, "fwrite") ;
  if (ecode != HIPS_OK)
    ErrorReturn(-1, (ERROR_BADFILE, 
              "ImageAppend: fwrite_image frame %d failed (%d)\n",ecode,frame));

  free_hdrcon(&Iheader) ;
  return(NO_ERROR) ;
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int compare_sort_array(const void *pf1, const void *pf2) ;
int        
ImageMedianFilter(IMAGE *inImage, int wsize, 
                            IMAGE *offsetImage, IMAGE *outImage)
{
  static float *sort_array = NULL ;
  static int   sort_size = 0 ;
  int    ecode, x0, y0, rows, cols, x, y, whalf, xc, yc, dx, dy ;
  float  *sptr, *outPix ;

  if (!offsetImage && inImage->pixel_format == PFBYTE && 
      outImage->pixel_format == PFBYTE)
  {
    ecode = h_median(inImage, outImage, wsize) ;
    if (ecode != HIPS_OK)
      ErrorReturn(-1, (ecode, "ImageMedian: h_median failed (%d)", ecode)) ;
    return(NO_ERROR) ;
  }

  rows = inImage->rows ;
  cols = inImage->cols ;
  whalf = (wsize-1)/2 ;

  /* create a static array for sorting pixels in */
  if (wsize > sort_size)
  {
    sort_size = wsize ;
    if (sort_array)
      sort_array = NULL ;
  }

  if (!sort_array)
    sort_array = (float *)calloc(wsize*wsize, sizeof(float)) ;

  outPix = IMAGEFpix(outImage, 0, 0) ;
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++)
    {
      
/*
  x and y are in window coordinates, while xc and yc are in image
  coordinates.
*/
      if (offsetImage)
      {
        dx = nint(*IMAGEFpix(offsetImage, x0, y0)) ;
        dy = nint(*IMAGEFseq_pix(offsetImage, x0, y0, 1)) ;
      }
      else
        dx = dy = 0 ;

      for (sptr = sort_array, y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 + dy ;
        if (yc < 0)
          yc = -yc ;
        else if (yc >= rows)
          yc = rows - (yc - rows + 1) ;
        
        for (x = -whalf ; x <= whalf ; x++)
        {
          xc = x0 + x + dx ;
          if (xc < 0)
            xc = -xc ;
          else if (xc >= cols)
            xc = cols - (xc - cols + 1) ;

          *sptr++ = *IMAGEFpix(inImage, xc, yc) ;
        }
      }
      qsort(sort_array, wsize*wsize, sizeof(float), compare_sort_array) ;
      *outPix++ = sort_array[wsize*wsize/2] ;
    }
  }
          
  return(0) ;
}
static int
compare_sort_array(const void *pf1, const void *pf2)
{
  float f1, f2 ;

  f1 = *(float *)pf1 ;
  f2 = *(float *)pf2 ;

/*  return(f1 > f2 ? 1 : f1 == f2 ? 0 : -1) ;*/
  if (f1 > f2)
    return(1) ;
  else if (f1 == f2)
    return(0) ;

  return(-1) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int        
ImageBuildExponentialFilter(IMAGE *gradImage, int wsize, float k,
                           IMAGE *offsetImage, 
                           IMAGE *filterSequence)
{
  int    rows, cols, x, y, whalf, xc, yc, x0, y0,
         dx, dy, frame ;
  float  fpix, *g, norm, val, *filterPix ;
  static float *gaussian = NULL ;
  static int   w = 0 ;

  rows = gradImage->rows ;
  cols = gradImage->cols ;

  whalf = (wsize-1)/2 ;

  if (wsize != w)
  {
    w = wsize ;
    free(gaussian) ;
    gaussian = NULL ;
  }

  if (!gaussian)  /* allocate a gaussian bump */
  {
    float den ;

    gaussian = (float *)calloc(wsize*wsize, sizeof(float)) ;
    den = wsize*wsize + wsize + 1 ;
    norm = 0.0f ;
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      yc = y - whalf ;
      for (x = 0 ; x < wsize ; x++, g++) 
      {
        xc = x - whalf ;
        *g = exp(-36.0f * sqrt(xc*xc+yc*yc) / den) ;
        norm += *g ;
      }
    }

    /* normalize gaussian */
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      for (x = 0 ; x < wsize ; x++, g++) 
        *g /= norm ;
    }
  }

/*
  x and y are in window coordinates, while xc and yc are in image
  coordinates.
*/
  for (frame = y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++, frame++)
    {
      if (offsetImage)
      {
        dx = nint(*IMAGEFpix(offsetImage, x0, y0)) ;
        dy = nint(*IMAGEIseq_pix(offsetImage, x0, y0, 1)) ;
      }
      else
        dx = dy = 0 ;

      norm = 0.0f ;
      filterPix = IMAGEFseq_pix(filterSequence, 0, 0, frame) ;

      if (x0 == 5 && y0 == 10)
        x0 = 70 ;

      for (g = gaussian, y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 + dy ;
        if (yc < 0)
          yc = -yc ;
        else if (yc >= rows)
          yc = rows - (yc - rows + 1) ;
        
        for (x = -whalf ; x <= whalf ; x++)
        {
          xc = x0 + x + dx ;
          if (xc < 0)
            xc = -xc ;
          else if (xc >= cols)
            xc = cols - (xc - cols + 1) ;
          
          fpix = *IMAGEFpix(gradImage, xc, yc) ;
          val = exp(-fpix*fpix / k)/*  * *g++ */ ;
          norm += val ;
          *filterPix++ = val ;
        }
      }

      if (FZERO(norm))
        continue ;

      /* normalize kernel weights to sum to 1 */
      filterPix = IMAGEFseq_pix(filterSequence, 0, 0, frame) ;
      for (y = 0 ; y < wsize ; y++)
      {
        for (x = 0 ; x < wsize ; x++)
          *filterPix++ /= norm ;
      }
    }
  }

  ImageWrite(filterSequence, "filter.hipl") ;
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageCalculateMomentOffset(IMAGE *gradImage, int wsize, float c,
                          IMAGE *offsetImage)
{
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageSpaceVariantFilter(IMAGE *inImage, IMAGE *filterSequence, 
                                  IMAGE *outImage)
{
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int        
ImageExponentialFilter(IMAGE *inImage, IMAGE *gradImage, 
                      int wsize, float k,
                           IMAGE *offsetImage, 
                           IMAGE *outImage)
{
  int    rows, cols, x, y, whalf, xc, yc, x0, y0, dx, dy ;
  float  fpix, *g, norm, val, *filterPix, *filter, *outPix ;
  static float w, *gaussian ;

  filter = (float *)calloc(wsize*wsize, sizeof(float)) ;
  if (!filter)
  {
    fprintf(stderr,"ImageExponentialFilter: could not allocate filter space\n");
    return(-1) ;
  }

  rows = gradImage->rows ;
  cols = gradImage->cols ;

  whalf = (wsize-1)/2 ;

  if (wsize != w)
  {
    w = wsize ;
    free(gaussian) ;
    gaussian = NULL ;
  }

  if (!gaussian)  /* allocate a gaussian bump */
  {
    float den ;

    gaussian = (float *)calloc(wsize*wsize, sizeof(float)) ;
    den = wsize*wsize + wsize + 1 ;
    norm = 0.0f ;
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      yc = y - whalf ;
      for (x = 0 ; x < wsize ; x++, g++) 
      {
        xc = x - whalf ;
        *g = exp(-36.0f * sqrt(xc*xc+yc*yc) / den) ;
        norm += *g ;
      }
    }

    /* normalize gaussian */
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      for (x = 0 ; x < wsize ; x++, g++) 
        *g /= norm ;
    }
  }

/*
  x and y are in window coordinates, while xc and yc are in image
  coordinates.
*/
  outPix = IMAGEFpix(outImage, 0, 0) ;
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++)
    {
      if (offsetImage)
      {
        dx = nint(*IMAGEFpix(offsetImage, x0, y0)) ;
        dy = nint(*IMAGEFseq_pix(offsetImage, x0, y0, 1)) ;
      }
      else
        dx = dy = 0 ;

      norm = 0.0f ;
      filterPix = filter ;

      for (g = gaussian, y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 + dy ;
        if (yc < 0)
          yc = -yc ;
        else if (yc >= rows)
          yc = rows - (yc - rows + 1) ;
        
        for (x = -whalf ; x <= whalf ; x++)
        {
          xc = x0 + x + dx ;
          if (xc < 0)
            xc = -xc ;
          else if (xc >= cols)
            xc = cols - (xc - cols + 1) ;
          
          fpix = *IMAGEFpix(gradImage, xc, yc) ;
          val = exp(-fpix*fpix / k)/*  * *g++ */ ;
          norm += val ;
          *filterPix++ = val ;
        }
      }

      if (FZERO(norm)) /* neigborhood is all zeros */
      {
        *outPix++ = 0.0f ;
        continue ;
      }

      /* normalize kernel weights to sum to 1 */
      filterPix = filter ;
      for (y = 0 ; y < wsize ; y++)
      {
        for (x = 0 ; x < wsize ; x++)
          *filterPix++ /= norm ;
      }

/* 
      now apply filter to this point in the image, taking possible 
      offset into accound 
*/
      filterPix = filter ;
      val = 0.0f ;
      for (y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 + dy ;
        if (yc < 0)
          yc = -yc ;
        else if (yc >= rows)
          yc = rows - (yc - rows + 1) ;
        
        for (x = -whalf ; x <= whalf ; x++)
        {
          xc = x0 + x + dx ;
          if (xc < 0)
            xc = -xc ;
          else if (xc >= cols)
            xc = cols - (xc - cols + 1) ;
          
          fpix = *IMAGEFpix(inImage, xc, yc) ;
          val += fpix * *filterPix++ ;
        }
      }

      if (isnan(val))
      {
        fprintf(stderr, "(%d, %d) = NaN!\n", xc, yc) ;
      }
      *outPix++ = val ;
    }
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
             calculate an x and a y offset for each point in the image
             using the Nitzberg-Shiota algorithm.
----------------------------------------------------------------------*/
int        
ImageCalculateOffset(IMAGE *Ix, IMAGE *Iy, int wsize, float mu, 
                    float c, IMAGE *offsetImage)
{
  int    x0, y0, rows, cols, x, y, whalf, xc, yc ;
  float  sx, sy, sgn, fxpix, fypix, vsq, c1, *g, vx, vy, dot_product,
         sxneg, sxpos, syneg, sypos, *xpix, *ypix ;
  static float *gaussian = NULL ;
  static int   w = 0 ;
  FILE         *xfp = NULL, *yfp = NULL, *ofp ;

  rows = Ix->rows ;
  cols = Ix->cols ;

  whalf = (wsize-1)/2 ;
  c1 = c * (float)whalf ;

  /* create a local gaussian window */
  if (wsize != w)
  {
    free(gaussian) ;
    gaussian = NULL ;
    w = wsize ;
  }

  if (!gaussian)  /* allocate a gaussian bump */
  {
    float den, norm ;

    gaussian = (float *)calloc(wsize*wsize, sizeof(float)) ;
    den = wsize*wsize + wsize + 1 ;
    norm = 0.0f ;
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      yc = y - whalf ;
      for (x = 0 ; x < wsize ; x++, g++) 
      {
        xc = x - whalf ;
#if 0
        /* Nitzberg-Shiota window */
        *g = exp(-36.0f * sqrt(xc*xc+yc*yc) / den) ;
#else
#define SIGMA 0.5f

        /* standard gaussian window with sigma=2 */
        *g = exp(-sqrt(xc*xc+yc*yc) / (2.0f*SIGMA*SIGMA)) ;
#endif
        norm += *g ;
      }
    }

    /* normalize gaussian */
#if 0
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      for (x = 0 ; x < wsize ; x++, g++) 
        *g /= norm ;
    }
#endif
  }

  if (Gdiag & DIAG_WRITE)
  {
    FILE *fp ;
    fp = fopen("g.dat", "w") ;
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      for (x = 0 ; x < wsize ; x++, g++) 
        fprintf(fp, "%2.3f  ", *g) ;
      fprintf(fp, "\n") ;
    }
    fclose(fp) ;
  }

  xpix = IMAGEFpix(offsetImage, 0, 0) ;
  ypix = IMAGEFseq_pix(offsetImage, 0, 0, 1) ;

  /* for each point in the image */
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++, xpix++, ypix++)
    {
      
/*
  x and y are in window coordinates, while x0, y0, xc and yc are in image
  coordinates.
*/

      if ((Gdiag & DIAG_WRITE) && (x0 == cols/2 && y0 == rows/2))
      {
        xfp = fopen("ix.dat", "w") ;
        yfp = fopen("iy.dat", "w") ;
        ofp = fopen("offset.log", "w") ;
      }
        
      vx = vy = sx = sy = sxneg = syneg = sxpos = sypos = 0.0f ;
      for (g = gaussian, y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 ;
        if (yc < 0)
          yc = -yc ;
        else if (yc >= rows)
          yc = rows - (yc - rows + 1) ;
        
        for (x = -whalf ; x <= whalf ; x++, g++)
        {
          xc = x0 + x ;
          if (xc < 0)
            xc = -xc ;
          else if (xc >= cols)
            xc = cols - (xc - cols + 1) ;
          
          fxpix = *IMAGEFpix(Ix, xc, yc) ;
          fypix = *IMAGEFpix(Iy, xc, yc) ;

          fxpix *= *g ;   /* apply gaussian window */
          fypix *= *g ;

          /* calculate sign of offset components */
          /* Nitzberg-Shiota offset */
          dot_product = x * fxpix + y * fypix ;
          sx += (dot_product * fxpix) ;
          sy += (dot_product * fypix) ;

          /* calculate the average gradient direction in the nbd */
          vx += fxpix ;
          vy += fypix ;
          if (xfp)
          {
            fprintf(ofp, 
                    "(%d, %d): Ix %2.2f, Iy, %2.2f, sx = %2.3f, sy = %2.3f\n",
                    xc, yc, fxpix, fypix, sx, sy) ;
            fprintf(xfp, "%+2.3f  ", fxpix) ;
            fprintf(yfp, "%+2.3f  ", fypix) ;
          }
        }
        if (xfp)
        {
          fprintf(xfp, "\n") ;
          fprintf(yfp, "\n") ;
        }
      }

      vsq = vx*vx + vy*vy ;

/*  decide whether offset should be in gradient or negative gradient direction
*/
#if 1
/* 
      ignore magnitude of sx and sy as they are the difference of the two
      hemifields, and thus not significant.
*/
      sx = (sx < 0.0f) ? -1.0f : 1.0f ;
      sy = (sy < 0.0f) ? -1.0f : 1.0f ;
#endif
      sgn = sx*vx + sy*vy ;
      sgn = (sgn < 0.0f) ? -1.0f : 1.0f ;

      /* calculated phi(V) */
      vx = vx*c1 / sqrt(mu*mu + vsq) ;
      vy = vy*c1 / sqrt(mu*mu + vsq) ;

      /* offset should never be larger than half of the window size */
      if (vx > whalf+1)  vx = whalf+1 ;
      if (vx < -whalf-1) vx = -whalf-1 ;
      if (vy > whalf+1)  vy = whalf+1 ;
      if (vy < -whalf-1) vy = -whalf-1 ;

      *xpix = -sgn * vx ;
      *ypix = -sgn * vy ;
      if (xfp)
      {
        fclose(xfp) ;
        fclose(yfp) ;
        fprintf(ofp, 
                "vx %2.3f, vy %2.3f, sgn %2.2f, sx %2.2f, sy %2.2f, vx %2.3f, vy %2.3f\n", 
                vx, vy, sgn, sx, sy, vx, vy) ;
        fclose(ofp) ;
      }
    }
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageCalculateNitShiOffset(IMAGE *Ix, IMAGE *Iy, int wsize, 
                          float mu, float c, IMAGE *offsetImage)
{
  int    x0, y0, rows, cols, x, y, whalf, xc, yc ;
  float  vx, vy, fxpix, fypix, vsq, c1, *g, dot_product, *xpix, *ypix ;
  static float *gaussian = NULL ;
  static int   w = 0 ;

  rows = Ix->rows ;
  cols = Ix->cols ;

  whalf = (wsize-1)/2 ;
  c1 = c * (float)whalf ;

  /* create a local gaussian window */
  if (wsize != w)
  {
    free(gaussian) ;
    gaussian = NULL ;
    w = wsize ;
  }

  if (!gaussian)  /* allocate a gaussian bump */
  {
    float den, norm ;

    gaussian = (float *)calloc(wsize*wsize, sizeof(float)) ;
    den = wsize*wsize + wsize + 1 ;
    norm = 0.0f ;
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      yc = y - whalf ;
      for (x = 0 ; x < wsize ; x++, g++) 
      {
        xc = x - whalf ;
        *g = exp(-36.0f * sqrt(xc*xc+yc*yc) / den) ;
        norm += *g ;
      }
    }

    /* normalize gaussian */
    for (g = gaussian, y = 0 ; y < wsize ; y++)
    {
      for (x = 0 ; x < wsize ; x++, g++) 
        *g /= norm ;
    }
  }

  xpix = IMAGEFpix(offsetImage, 0, 0) ;
  ypix = IMAGEFseq_pix(offsetImage, 0, 0, 1) ;
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++, xpix++, ypix++)
    {
      
/*
  x and y are in window coordinates, while xc and yc are in image
  coordinates.
*/
      /* first build offset direction */
      vx = vy = 0.0f ;
      for (g = gaussian, y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 ;
        if (yc < 0)
          yc = -yc ;
        else if (yc >= rows)
          yc = rows - (yc - rows + 1) ;
        
        for (x = -whalf ; x <= whalf ; x++, g++)
        {
          xc = x0 + x ;
          if (xc < 0)
            xc = -xc ;
          else if (xc >= cols)
            xc = cols - (xc - cols + 1) ;
          
          fxpix = *IMAGEFpix(Ix, xc, yc) ;
          fypix = *IMAGEFpix(Iy, xc, yc) ;
          dot_product = x * fxpix + y * fypix ;
          vx += *g * (dot_product * fxpix) ;
          vy += *g * (dot_product * fypix) ;
        }
      }

      vsq = vx*vx + vy*vy ;

      /* calculated phi(V) */
      vx = vx*c1 / sqrt(mu*mu + vsq) ;
      vy = vy*c1 / sqrt(mu*mu + vsq) ;
      *xpix = -vx ;
      *ypix = -vy ;
    }
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageAbs(IMAGE *inImage, IMAGE *outImage)
{
  UCHAR *cIn, *cOut ;
  UINT  *iIn, *iOut ;
  float *fIn, *fOut ;
  int   size, nframes, frameno, pix_per_frame, rows, cols ;

  rows = inImage->rows ;
  cols = inImage->cols ;
  if (!outImage)
  {
    outImage = ImageAlloc(cols, rows, 1, inImage->pixel_format) ;
  }

  nframes = inImage->num_frame ;
  pix_per_frame = inImage->rows * inImage->cols ;
  for (frameno = 0 ; frameno < nframes ; frameno++)
  {
    size = inImage->rows * inImage->cols ;
    switch (inImage->pixel_format)
    {
    case PFFLOAT:
      fIn = IMAGEFpix(inImage, 0, 0) + pix_per_frame * frameno ;
      fOut = IMAGEFpix(outImage, 0, 0) + pix_per_frame * frameno  ;
      while (size--)
        *fOut++ = fabs(*fIn++) ;
      break ;
    case PFBYTE:
      cIn = IMAGEpix(inImage, 0, 0) + pix_per_frame * frameno ;
      switch (outImage->pixel_format)
      {
      case PFBYTE:
        cOut = IMAGEpix(outImage, 0, 0) + pix_per_frame * frameno ;
        while (size--)
          *cOut++ = abs(*cIn++) ;
        break ;
      case PFINT:
        iOut = IMAGEIpix(outImage, 0, 0) + pix_per_frame * frameno ;
        while (size--)
          *iOut++ = (UINT)abs(*cIn++) ;
        break ;
      default:
        ErrorExit(ERROR_BADPARM,
                  "ImageAbs: unsupported output image pixel format (%d)\n",
                  outImage->pixel_format) ;
        return(NULL) ;
        break ;
      }
      break ;
    case PFINT:
      iIn = IMAGEIpix(inImage, 0, 0) + pix_per_frame * frameno ;
      switch (outImage->pixel_format)
      {
      case PFINT:
        iOut = IMAGEIpix(outImage, 0, 0) + pix_per_frame * frameno ;
        while (size--)
          *iOut++ = abs(*iIn++) ;
        break ;
        break ;
      default:
        ErrorExit(ERROR_BADPARM,
                  "ImageAbs: unsupported output image pixel format (%d)\n",
                  outImage->pixel_format) ;
        return(NULL) ;
        break ;
      }
      break ;
    default:
      ErrorExit(ERROR_BADPARM,
                "ImageAbs: unsupported input image pixel format (%d)\n",
                inImage->pixel_format) ;
      return(NULL) ;
      break ;
    }
  }

  outImage->rows = inImage->rows ;
  outImage->cols = inImage->cols ;
  
  return(outImage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int        
ImageSobel(IMAGE *inImage, IMAGE *gradImage, 
                     IMAGE *dxImage, IMAGE *dyImage)
{
  static IMAGE *xImage = NULL, *yImage = NULL ;
  int          x, y, rows, cols ;
  float        *xpix, *ypix, *gradpix, xval, yval, gval ;

  rows = inImage->rows ;
  cols = inImage->cols ;

  if (!dxImage)
  {
    dxImage = xImage ;
    if (!ImageCheckSize(inImage, xImage, 0, 0, 0))
    {
      if (xImage)
        ImageFree(&xImage) ;
      xImage = ImageAlloc(cols, rows, PFFLOAT, 1) ;
    }
    else
    {
      xImage->rows = rows ;
      xImage->cols = cols ;
    }
  }

  if (!dyImage)
  {
    dyImage = yImage ;
    if (!ImageCheckSize(inImage, yImage, 0, 0, 0))
    {
      if (yImage)
        ImageFree(&yImage) ;
      yImage = ImageAlloc(cols, rows, PFFLOAT, 1) ;
    }
    else
    {
      yImage->rows = rows ;
      yImage->cols = cols ;
    }
  }

  
  dxImage->cols = dyImage->cols = cols ;
  dxImage->rows = dyImage->rows = rows ;
  ImageConvolve3x3(inImage, sx, dxImage) ;
  ImageConvolve3x3(inImage, sy, dyImage) ;
  if (gradImage)
  {
    gradImage->cols = cols ;
    gradImage->rows = rows ;
    gradpix = IMAGEFpix(gradImage, 0, 0) ;
  }

  xpix = IMAGEFpix(dxImage, 0, 0) ;
  ypix = IMAGEFpix(dyImage, 0, 0) ;
  for (y = 0 ; y < rows ; y++)
  {
    for (x = 0 ; x < cols ; x++)
    {
      xval = *xpix++ ;
      yval = *ypix++ ;
      if (gradImage)
      {
        gval = sqrt(xval * xval + yval * yval) ;
        *gradpix++ = gval ;
      }
    }
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageSobelX(IMAGE *inImage, IMAGE *xImage)
{
  ImageConvolve3x3(inImage, sx, xImage) ;
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageSobelY(IMAGE *inImage, IMAGE *yImage)
{
  ImageConvolve3x3(inImage, sy, yImage) ;
  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageXDerivative(IMAGE *inImage, IMAGE *xImage)
{
  if (!xImage)
    xImage = ImageAlloc(inImage->cols, inImage->rows, PFFLOAT, 1) ;

  ImageConvolve3x3(inImage, sx, xImage) ;

  return(xImage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageYDerivative(IMAGE *inImage, IMAGE *yImage)
{
  if (!yImage)
    yImage = ImageAlloc(inImage->cols, inImage->rows, PFFLOAT, 1) ;

  ImageConvolve3x3(inImage, sy, yImage) ;

  return(yImage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageConvolve3x3(IMAGE *inImage, float kernel[], IMAGE *outImage)
{
  int     rows, cols, x, y, xk, yk, k, xi, yi ;
  float   *fkpix, sum, *fopix, *fipix ;

  rows = inImage->rows ;
  cols = inImage->cols ;

  switch (inImage->pixel_format)
  {
  case PFFLOAT:
    fopix = (float *)IMAGEFpix(outImage, 0, 0) ;
    for (y = 0 ; y < rows ; y++)
    {
      for (x = 0 ; x < cols ; x++, fopix++)
      {
        fkpix = kernel ;
        for (sum = 0.0, k = 0, yk = -1 ; yk <= 1 ; yk++)
        {
          yi = y + yk ;    /* image coordinate */
          if (yi < 0)
            yi = 0 ;
          else if (yi >= rows)
            yi = rows-1 ;

          for (xk = -1 ; xk <= 1 ; xk++, k++, fkpix++)
          {
            xi = x + xk ;   /* image coordinate */
            if (xi < 0)
              xi = 0 ;
            else if (xi >= cols)
              xi = cols-1 ;
            fipix = IMAGEFpix(inImage, xi, yi) ;
            sum = sum + *fipix * *fkpix ;
          }
        }
        *fopix = sum ;
      }
    }
    break ;
  default:
    fprintf(stderr, "ImageConvolve3x3: unsupported pixel format %d\n",
            inImage->pixel_format) ;
    exit(-1);
    break ;
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageAdd(IMAGE *Is1, IMAGE *Is2, IMAGE *Idst)
{
  int ecode ;

  if (!Idst)
    Idst = ImageAlloc(Is1->rows, Is1->cols,Is1->pixel_format, Is1->num_frame);

  ecode = h_add(Is1, Is2, Idst) ;
  if (ecode != HIPS_OK)
    ErrorPrintf(ecode, "ImageAdd: h_add failed (%d)", ecode) ;

  return(Idst) ;
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int xoffsets[] = { 0, -1, 0, 1, 0 } ;
static int yoffsets[] = { -1, 0, 0, 0, 1 } ;
#define ONE_EIGHTH (1.0f/8.0f)
static float weights[] = 
{          ONE_EIGHTH, 
    ONE_EIGHTH, -0.5f, ONE_EIGHTH, 
            ONE_EIGHTH 
} ;
#define LAPLACIAN_POINTS   (sizeof(xoffsets) / sizeof(xoffsets[0]))

IMAGE  *
ImageLaplacian(IMAGE *inImage, IMAGE *outImage)
{
  int     rows, cols, x, y, xi, yi, i ;
  float   *fkpix, sum, *fopix, fival ;

  rows = inImage->rows ;
  cols = outImage->cols ;

  if (!outImage)
  {
    outImage = ImageAlloc(cols, rows, 1, PFFLOAT) ;
  }

  switch (inImage->pixel_format)
  {
  case PFFLOAT:
    fopix = (float *)IMAGEFpix(outImage, 0, 0) ;
    for (y = 0 ; y < rows ; y++)
    {
      for (x = 0 ; x < cols ; x++, fopix++)
      {
        fkpix = weights ;
        for (sum = 0.0f, i = 0 ; i < LAPLACIAN_POINTS ; i++)
        {
          yi = y + yoffsets[i] ;    /* image coordinate */
          if (yi < 0)
            yi = 0 ;
          else if (yi >= rows)
            yi = rows-1 ;

          xi = x + xoffsets[i] ;   /* image coordinate */
          if (xi < 0)
            xi = 0 ;
          else if (xi >= cols)
            xi = cols-1 ;
          fival = *IMAGEFpix(inImage, xi, yi) ;
          sum += fival * *fkpix++ ;
        }
        *fopix = sum ;
      }
    }
    break ;
  default:
    fprintf(stderr, "ImageLaplacian: unsupported pixel format %d\n",
            inImage->pixel_format) ;
    exit(-1);
    break ;
  }

  return(outImage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageConvolveGaussian(IMAGE *inImage,IMAGE *gImage, IMAGE *outImage,
                     int dst_frameno)
{
  static IMAGE     *tmpImage = NULL ;
  int              ksize ;
  float            *kernel, *buf ;

  if (!ImageCheckSize(inImage, tmpImage, 0, 0, 0))
  {
    if (tmpImage)
      ImageFree(&tmpImage) ;
    tmpImage = ImageAlloc(inImage->cols, inImage->rows, PFFLOAT, 1) ;
  }
  if (!outImage)
    outImage = ImageAlloc(inImage->cols, inImage->rows, PFFLOAT, 1) ;

  tmpImage->rows = inImage->rows ;
  tmpImage->cols = inImage->cols ;

  kernel = IMAGEFpix(gImage, 0, 0) ;
  ksize = gImage->cols ;
  ImageConvolve1d(inImage, tmpImage, kernel, ksize, IMAGE_VERTICAL) ;
  buf = IMAGEFpix(outImage, 0, 0) ;
  outImage->image = (UCHAR *)IMAGEFseq_pix(outImage, 0, 0, dst_frameno) ;
  ImageConvolve1d(tmpImage, outImage, kernel, ksize, IMAGE_HORIZONTAL) ;
  outImage->image = (UCHAR *)buf ;
  return(outImage) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
ImageConvolve1d(IMAGE *I, IMAGE *J, float k[], int len, int axis)
{
  int    x, y, i, width, height, xi,yi, halflen ;
  float  total ;

  width = I->cols ;
  height = I->rows ;

  halflen = len/2 ;
  if (axis == IMAGE_HORIZONTAL)
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        total = 0.0f ;

        for (i = 0 ; i < len ; i++)
        {
          xi = x + i - halflen ;
#if 0
          /* Neumann boundary conditions */
          if (xi < 0)
            xi = -xi ;
          else if (xi >= width)
            xi = width - (xi - width+1) ;
#else
          if (xi < 0)
            xi = 0 ;
          else if (xi >= width)
            xi = width - 1 ;
#endif
          
          total = total + k[i] * *IMAGEFpix(I, xi, y) ;
        }
        *IMAGEFpix(J, x, y) = total ;
      }
    }
  }
  else
  {
    for (y = 0 ; y < height ; y++)
    {
      for (x = 0 ; x < width ; x++)
      {
        total = 0.0f ;

        for (i = 0 ; i < len ; i++)
        {
          yi = y + i - halflen ;
#if 0
          /* Neumann boundary conditions */
          if (yi < 0)
            yi = -yi ;
          else if (yi >= height)
            yi = height - (yi - height+1) ;
#else
          if (yi < 0)
            yi = 0 ;
          else if (yi >= height)
            yi = height - 1 ;
#endif          
          total = total + k[i] * *IMAGEFpix(I, x, yi) ;
        }
        *IMAGEFpix(J, x, y) = total ;
      }
    }
  }
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
             use k[] to scale the image down by 2.
----------------------------------------------------------------------*/
void
ImageReduce1d(IMAGE *I, IMAGE *J, float k[], int len, int axis)
{
  int    x, y, i, Jwidth, Jheight, xi,yi, halflen, Iwidth, Iheight ;
  float  total ;

  Jwidth = J->cols ;
  Jheight = J->rows ;
  Iwidth = I->cols ;
  Iheight = I->rows ;

  halflen = (len-1)/2 ;
  if (axis == IMAGE_HORIZONTAL)
  {
    for (y = 0 ; y < Jheight ; y++)
    {
      yi = 2*y ;
      for (x = 0 ; x < Jwidth ; x++)
      {
        total = 0.0f ;

        if (x >= 3 && x <= 6)
          i = 0 ;

        for (i = 0 ; i < len ; i++)
        {
          /* Neumann boundary conditions */
          xi = 2*x + i - halflen ;
#if 0
          if (xi < 0)
            xi = -xi ;
          else if (xi >= Iwidth)
            xi = Iwidth - (xi - Iwidth+1) ;
#else
          if (xi < 0)
            xi = 0 ;
          else if (xi >= Iwidth)
            xi = Iwidth - 1 ;
#endif
          
          total = total + k[i] * *IMAGEFpix(I, xi, yi) ;
        }
        *IMAGEFpix(J, x, y) = total ;
      }
    }
  }
  else
  {
    for (y = 0 ; y < Jheight ; y++)
    {
      for (x = 0 ; x < Jwidth ; x++)
      {
        total = 0.0f ;
        xi = 2*x ;

#if 0
        if (((x == 9) && (y == 127)) ||
            ((y == 9) && (x == 127)))
          i = 0 ;
#endif

        for (i = 0 ; i < len ; i++)
        {
          /* Neumann boundary conditions */
          yi = 2*y + i - halflen ;
#if 0
          if (yi < 0)
            yi = -yi ;
          else if (yi >= Iheight)
            yi = Iheight - (yi - Iheight+1) ;
#else
          if (yi < 0)
            yi = 0 ;
          else if (yi >= Iheight)
            yi = Iheight - 1 ;
#endif
          
          total = total + k[i] * *IMAGEFpix(I, xi, yi) ;
        }
        *IMAGEFpix(J, x, y) = total ;
      }
    }
  }
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
             construct a gaussian bump which tails to 0. Returns
             an image which is ((4*xsigma)+1) x ((4*ysigma)+1)
----------------------------------------------------------------------*/
IMAGE *
ImageGaussian(float xsigma, float ysigma)
{
  IMAGE *image ;
  float     norm, ytwo_sigma, xtwo_sigma, fx, fy, k ;
  int       x, y, xlen, ylen, xhalf, yhalf ;

  /* build the kernel in k */
  xlen = (int)nint(8.0f * xsigma)+1 ;
  ylen = (int)nint(8.0f * ysigma)+1 ;
  xhalf = xlen/2 ;
  yhalf = ylen/2 ;
  image = ImageAlloc(xlen, ylen, PFFLOAT, 1) ;

  norm = 0.0f ;
  xtwo_sigma = 2.0f * xsigma ;
  ytwo_sigma = 2.0f * ysigma ;

  for (x = 0 ; x < xlen ; x++)
  {
    fx = (float)(x-xhalf) ;
    if (fabs(fx) <= xtwo_sigma)
      k = exp(-fx*fx/(xtwo_sigma*xsigma)) ;  /* parens added!! */
    else if (xtwo_sigma < fabs(fx) && fabs(fx) <= 4*xsigma)
      k = 1.0f / (16.0f * M_E * M_E) * pow(4.0f - fabs(fx)/xsigma, 4.0) ;
    else
      k = 0 ;

    for (y = 0 ; y < ylen ; y++)
      *IMAGEFpix(image, x, y) = k ;
  }

  for (y = 0 ; y < ylen ; y++)
  {
    fy = (float)(y-yhalf) ;
    if (fabs(fy) <= ytwo_sigma)
      k = exp(-fy*fy/(ytwo_sigma*ysigma)) ;  /* parens added!! */
    else if (ytwo_sigma < fabs(fy) && fabs(fy) <= 4*ysigma)
      k = 1.0f / (16.0f * M_E * M_E) * pow(4.0f - fabs(fy)/ysigma, 4.0) ;
    else
      k = 0 ;

    for (x = 0 ; x < xlen ; x++)
    {
      *IMAGEFpix(image, x, y) *= k ;
      norm += *IMAGEFpix(image, x, y) ;
    }
  }

  /* normalize kernel to sum to 1 */
  for (x = 0 ; x < xlen ; x++)
  {
    for (y = 0 ; y < ylen ; y++)
      *IMAGEFpix(image, x, y) /= norm ;
  }

  return(image) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
             construct a gaussian bump which tails to 0. Returns
             an image which is (4*sigma)+1
----------------------------------------------------------------------*/
IMAGE *
ImageGaussian1d(float sigma)
{
  IMAGE *image ;
  float     norm, two_sigma, fx, k ;
  int       x, half, len ;

  /* build the kernel in k */
  len = (int)nint(8.0f * sigma)+1 ;
  half = len/2 ;
  image = ImageAlloc(len, 1, PFFLOAT, 1) ;

  norm = 0.0f ;
  two_sigma = 2.0f * sigma ;

  for (norm = 0.0f, x = 0 ; x < len ; x++)
  {
    fx = (float)(x-half) ;
    if (fabs(fx) <= two_sigma)
      k = exp(-fx*fx/(two_sigma*sigma)) ;  /* parens added!! */
    else if (two_sigma < fabs(fx) && fabs(fx) <= 4*sigma)
      k = 1.0f / (16.0f * M_E * M_E) * pow(4.0f - fabs(fx)/sigma, 4.0) ;
    else
      k = 0 ;

    *IMAGEFpix(image, x, 0) = k ;
    norm += k ;
  }

  /* normalize kernel to sum to 1 */
  for (x = 0 ; x < len ; x++)
    *IMAGEFpix(image, x, 0) /= norm ;

  return(image) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageExtractInto(IMAGE *inImage, IMAGE *outImage, int x0, int y0, 
                     int dx, int dy, int xdst, int ydst)
{
  UCHAR    *csrc, *cdst ;
  float    *fsrc, *fdst ;
  int      xin, yin, yout, x1, y1, yend, xend ;

  if (inImage->pixel_format != outImage->pixel_format)
  {
    fprintf(stderr, "ImageExtractInto: out format must match input format\n");
    return(-1) ;
  }

  x1 = x0 + dx ;
  y1 = y0 + dy ;
  xend = inImage->cols-1 ;
  yend = inImage->rows-1 ;

  switch (inImage->pixel_format)
  {
  case PFBYTE:
    yout = ydst ;
    cdst = IMAGEpix(outImage, xdst, ydst) ;
    for (yin = y0 ; yin < y1 ; yin++, yout++)
    {
      csrc = IMAGEpix(inImage, x0, yin) ;
      cdst = IMAGEpix(outImage, xdst, yout) ;
      for (xin = x0 ; xin < x1 ; xin++, cdst++, csrc++)
      {
        if (xin < 0 || xin > xend || yin < 0 || yin > yend)
          *cdst = 0 ;
        else
          *cdst = *csrc ;
      }
    }
    break ;
  case PFFLOAT:
    yout = ydst ;
    fdst = IMAGEFpix(outImage, xdst, ydst) ;
    for (yin = y0 ; yin < y1 ; yin++, yout++)
    {
      fsrc = IMAGEFpix(inImage, x0, yin) ;
      fdst = IMAGEFpix(outImage, xdst, yout) ;
      for (xin = x0 ; xin < x1 ; xin++, fdst++, fsrc++)
      {
        if (xin < 0 || xin > xend || yin < 0 || yin > yend)
          *fdst = 0.0f ;
        else
          *fdst = *fsrc ;
      }
    }
    break ;
  default:
    fprintf(stderr,"ImageExtractInto: unsupported image format %d\n", 
            inImage->pixel_format) ;
    return(-1) ;
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
ImageExtract(IMAGE *inImage, IMAGE *outImage, int x0, int y0,int dx,
            int dy)
{
  UCHAR    *csrc, *cdst ;
  int      xin, yin, xout, yout, x1, y1, yend, xend ;

  x1 = x0 + dx ;
  y1 = y0 + dy ;
  xend = inImage->cols-1 ;
  yend = inImage->rows-1 ;

  switch (inImage->pixel_format)
  {
  case PFBYTE:
    yout = xout = 0 ;
    cdst = IMAGEpix(outImage, 0, 0) ;
    for (yin = y0 ; yin < y1 ; yin++, yout++)
    {
      csrc = IMAGEpix(inImage, x0, yin) ;
      cdst = IMAGEpix(outImage, 0, yout) ;
      for (xout = 0, xin = x0 ; xin < x1 ; xin++, xout++, cdst++, csrc++)
      {
        if (xin < 0 || xin > xend || yin < 0 || yin > yend)
          *cdst = 0 ;
        else
          *cdst = *csrc ;
      }
    }
    break ;

  default:
    fprintf(stderr,"ImageExtract: unsupported image format %d\n", 
            inImage->pixel_format) ;
    return(-1) ;
  }

  return(0) ;
}
