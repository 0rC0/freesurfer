/*
 *       FILE NAME:   mriclass.c
 *
 *       DESCRIPTION: utilities for MRI classification
 *
 *       AUTHOR:      Bruce Fischl
 *       DATE:        1/8/97
 *
*/

/*-----------------------------------------------------
                    INCLUDE FILES
-------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <memory.h>

#include "error.h"
#include "proto.h"
#include "mri.h"
#include "macros.h"
#include "diag.h"
#include "mriclass.h"
#include "matrix.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/


#define NCLASSES      4
#define BACKGROUND    0
#define GREY_MATTER   1
#define WHITE_MATTER  2
#define BRIGHT_MATTER 3

/*-----------------------------------------------------
                    STATIC DATA
-------------------------------------------------------*/

static char *class_names[NCLASSES] =
{
  "BACKGROUND",
  "GREY MATTER",
  "WHITE MATTER",
  "BRIGHT MATTER"
} ;

/*-----------------------------------------------------
                    STATIC PROTOTYPES
-------------------------------------------------------*/


/*-----------------------------------------------------
                    GLOBAL FUNCTIONS
-------------------------------------------------------*/
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
MRIC *
MRIclassAlloc(int width, int height, int depth, int scale, int nvars)
{
  MRIC *mric ;
  int  x, y, z ;

  mric = (MRIC *)calloc(1, sizeof(MRIC)) ;
  if (!mric)
    ErrorReturn(NULL, 
                (ERROR_NO_MEMORY, "MRIalloc: could not alloc struct")) ;

  width = (width + scale - 1) / scale ;
  height = (height + scale - 1) / scale ;
  depth = (depth + scale - 1) / scale ;
  mric->scale = scale ;
  mric->width = width ;
  mric->height = height ;
  mric->depth = depth ;
  mric->nvars = nvars ;

  mric->gcs = (GCLASSIFY ****)calloc(height, sizeof(GCLASSIFY ***)) ;
  if (!mric->gcs)
    ErrorExit(ERROR_NO_MEMORY, "MRIclassAlloc: could not allocate gcs") ;

  for (z = 0 ; z < depth ; z++)
  {
    mric->gcs[z] = (GCLASSIFY ***)calloc(height, sizeof(GCLASSIFY **)) ;
    if (!mric->gcs[z])
      ErrorExit(ERROR_NO_MEMORY, 
                "MRIclassAlloc: could not allocate gcs[%d]", z) ;

    for (y = 0 ; y < height ; y++)
    {
      mric->gcs[z][y] = (GCLASSIFY **)calloc(width, sizeof(GCLASSIFY *)) ;
      if (!mric->gcs[z][y])
        ErrorExit(ERROR_NO_MEMORY,
                  "GCalloc(%d,%d,%d,%d): could not allocate gc[%d][%d]",
                  width, height, depth, scale, y, z) ;

      for (x = 0 ; x < width ; x++)
      {
        mric->gcs[z][y][x] = GCalloc(NCLASSES, nvars, class_names) ;
        if (!mric->gcs[z][y][x])
          ErrorExit(ERROR_NO_MEMORY,
                    "GCalloc(%d,%d,%d,%d): could not allocate gc[%d][%d][%d]",
                    width, height, depth, scale, x, y, z) ;

      }
    }
  }

  return(mric) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
int
MRIclassFree(MRIC **pmric)
{
  MRIC  *mric ;
  int  x, y, z ;

  mric = *pmric ;
  *pmric = NULL ;


  for (z = 0 ; z < mric->depth ; z++)
  {
    if (mric->gcs[z])
    {
      for (y = 0 ; y < mric->height ; y++)
      {
        if (mric->gcs[z][y])
        {
          for (x = 0 ; x < mric->width ; x++)
          {
            if (mric->gcs[z][y][x])
              GCfree(&mric->gcs[z][y][x]) ;
          }
          free(mric->gcs[z][y]) ;
        }
      }
      free(mric->gcs[z]) ;
    }
  }

  free(mric->gcs) ;
  free(mric) ;
  return(NO_ERROR) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
#define LO_LIM  50
#define HI_LIM  150

int
MRIclassTrain(MRIC *mric, MRI *mri_src, MRI *mri_norm, MRI *mri_target)
{
  MATRIX     *m_inputs[NCLASSES] ;
  GCLASSIFY  *gc, **pgc ;
  int        x, y, z, x0, y0, z0, x1, y1, z1, xm, ym, zm, 
             width, depth, height, scale, classno, nclasses, nobs[NCLASSES],
             swidth, sheight, sdepth ;
  BUFTYPE    *psrc, *ptarget, src, target ;
  float      *pnorm ;

  scale = mric->scale ;
  for (classno = 0 ; classno < NCLASSES ; classno++)
    m_inputs[classno] = MatrixAlloc(scale*scale*scale,mric->nvars,MATRIX_REAL);

  width = mric->width ;
  height = mric->height ;
  depth = mric->depth ;
  scale = mric->scale ;
  nclasses = NCLASSES ;

  swidth = mri_src->width ;
  sheight = mri_src->height ;
  sdepth = mri_src->depth ;

  /* train each classifier, x,y,z are in classifier space */
  for (z = 0 ; z < depth ; z++)
  {
    z0 = z*scale ;
    z1 = MIN(sdepth,z0+scale)-1 ;
    for (y = 0 ; y < height ; y++)
    {
      y0 = y*scale ;
      y1 = MIN(sheight,y0+scale)-1 ;
      pgc = mric->gcs[z][y] ;
      for (x = 0 ; x < width ; x++)
      {
        gc = *pgc++ ;
        x0 = x*scale ;
        x1 = MIN(swidth,x0+scale)-1 ;
        
        memset(nobs, 0, NCLASSES*sizeof(nobs[0])) ;
        for (zm = z0 ; zm <= z1 ; zm++)
        {
          for (ym = y0 ; ym <= y1 ; ym++)
          {
            psrc = &MRIvox(mri_src, x0, ym, zm) ;
            ptarget = &MRIvox(mri_target, x0, ym, zm) ;
            pnorm = &MRIFvox(mri_norm, x0, ym, zm) ;
            for (xm = x0 ; xm <= x1 ; xm++)
            {
              src = *psrc++ ;
              target = *ptarget++ ;
              
              /* decide what class it is */
              if (target)
                classno = WHITE_MATTER ;
              else
              {
                if (src < LO_LIM)
                  classno = BACKGROUND ;
                else if (src > HI_LIM)
                  classno = BRIGHT_MATTER ;
                else
                  classno = GREY_MATTER ;
              }
              m_inputs[classno]->rptr[nobs[classno]+1][1] = src ;
              m_inputs[classno]->rptr[nobs[classno]+1][2] = *pnorm++ ;
#if 0
              m_inputs[classno]->rptr[nobs[classno]+1][3] = xm ;
              m_inputs[classno]->rptr[nobs[classno]+1][4] = ym ;
              m_inputs[classno]->rptr[nobs[classno]+1][5] = zm ;
#endif
              nobs[classno]++ ;
            }
          }
        }

        /* now apply training vectors */
        for (classno = 0 ; classno < nclasses ; classno++)
        {
          m_inputs[classno]->rows = nobs[classno] ;
          GCtrain(gc, classno, m_inputs[classno]) ;
          m_inputs[classno]->rows = scale*scale*scale ;
        }
      }
    }
  }

  for (classno = 0 ; classno < NCLASSES ; classno++)
    MatrixFree(&m_inputs[classno]) ;

  return(ERROR_NONE) ;
}
/*-----------------------------------------------------
        Parameters:

        Returns value:

        Description

------------------------------------------------------*/
#define PRETTY_SURE   .90f

MRI *
MRIclassify(MRIC *mric, MRI *mri_src, MRI *mri_norm, MRI *mri_dst, float conf)
{
  MATRIX     *m_inputs ;
  GCLASSIFY  *gc, **pgc ;
  int        x, y, z, x0, y0, z0, x1, y1, z1, xm, ym, zm, 
             width, depth, height, scale, classno, nclasses,
             swidth, sheight, sdepth ;
  BUFTYPE    *psrc, src, *pdst ;
  float      *pnorm, prob ;

  if (conf < 0.0f || conf >= 1.0f)
    conf = PRETTY_SURE ;

  scale = mric->scale ;

  if (!mri_dst)
    mri_dst = MRIclone(mri_src, NULL) ;

  m_inputs = MatrixAlloc(mric->nvars, 1, MATRIX_REAL) ;

  width = mric->width ;
  height = mric->height ;
  depth = mric->depth ;
  swidth = mri_src->width ;
  sheight = mri_src->height ;
  sdepth = mri_src->depth ;
  scale = mric->scale ;
  nclasses = NCLASSES ;

  /* train each classifier, x,y,z are in classifier space */
  for (z = 0 ; z < depth ; z++)
  {
    z0 = z*scale ;
    z1 = MIN(sdepth,z0+scale)-1 ;
    for (y = 0 ; y < height ; y++)
    {
      y0 = y*scale ;
      y1 = MIN(sheight,y0+scale)-1 ;
      pgc = mric->gcs[z][y] ;
      for (x = 0 ; x < width ; x++)
      {
        x0 = x*scale ;
        x1 = MIN(swidth,x0+scale)-1 ;
        gc = *pgc++ ;

        for (zm = z0 ; zm <= z1 ; zm++)
        {
          for (ym = y0 ; ym <= y1 ; ym++)
          {
            psrc = &MRIvox(mri_src, x0, ym, zm) ;
            pdst = &MRIvox(mri_dst, x0, ym, zm) ;
            pnorm = &MRIFvox(mri_norm, x0, ym, zm) ;
            for (xm = x0 ; xm <= x1 ; xm++)
            {
if (xm == 35 && ym == 49 && zm == (95-64))
  DiagBreak() ;
              src = *psrc++ ;
              m_inputs->rptr[1][1] = src ;
              m_inputs->rptr[2][1] = *pnorm++ ;

              /* now classify this observation */
              classno = GCclassify(gc, m_inputs, &prob) ;
              if (classno == WHITE_MATTER && prob > conf)
                *pdst++ = 255 ;
              else
                *pdst++ = src ;
#if 0
if (xm == 35 && ym == 49 && zm == (95-64))
  fprintf(stderr, "(%d, %d, %d) : class %d, prob %2.1f%%\n",
          xm, ym, zm, classno, 100.0f*prob) ;
#endif
            }
          }
        }

      }
    }
  }

  MatrixFree(&m_inputs) ;

  return(mri_dst) ;
}

