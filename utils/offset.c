/*
 *       FILE NAME:   offset.c
 *
 *       DESCRIPTION: routines for generating displacement vector fields. Split from
 *                    image.c
 *
 *       AUTHOR:      Bruce Fischl
 *       DATE:        7/1/96
 *
*/

/*-----------------------------------------------------
                    INCLUDE FILES
-------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <memory.h>
#include <unistd.h>        /* for unlink */
#include <hipl_format.h>

#include "hips.h"

#include "image.h"
#include "error.h"
#include "utils.h"
#include "macros.h"
#include "machine.h"
#include "proto.h"
#include "diag.h"

/*-----------------------------------------------------
                    MACROS AND CONSTANTS
-------------------------------------------------------*/

/*-----------------------------------------------------
                    STATIC PROTOTYPES
-------------------------------------------------------*/
static  void break_here(void) ;
static  void break_here(void){}

/*----------------------------------------------------------------------
            Parameters:

           Description:
             calculate an x and a y offset for each point in the image
             using a modified Nitzberg-Shiota algorithm.
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
  FILE         *xfp = NULL, *yfp = NULL, *ofp = NULL ;

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
        *g = (float)exp((double)-sqrt((double)xc*xc+yc*yc) / (2.0*SIGMA*SIGMA));
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
    
    unlink("ix.dat") ;
    unlink("iy.dat") ;
    unlink("offset.log") ;

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

      if ((Gdiag & DIAG_WRITE) && (x0 == 14 && y0 == 63))
      {
        xfp = fopen("ix.dat", "a") ;
        yfp = fopen("iy.dat", "a") ;
        ofp = fopen("offset.log", "a") ;
        fprintf(xfp, "(%d, %d)\n", x0, y0) ;
        fprintf(yfp, "(%d, %d)\n", x0, y0) ;
        fprintf(ofp, "(%d, %d)\n", x0, y0) ;
      }
      else
        xfp = yfp = ofp = NULL ;
        
      vx = vy = sx = sy = sxneg = syneg = sxpos = sypos = 0.0f ;
      for (g = gaussian, y = -whalf ; y <= whalf ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 ;
        if ((yc < 0) || (yc >= rows))
        {
          g += wsize ;
          continue ;
        }
        
        for (x = -whalf ; x <= whalf ; x++, g++)
        {
          xc = x0 + x ;
          if ((xc < 0) || (xc >= cols))
            continue ;
          
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
      vx = vx*c1 / (float)sqrt((double)(mu*mu + vsq)) ;
      vy = vy*c1 / (float)sqrt((double)(mu*mu + vsq)) ;

      /* offset should never be larger than half of the window size */
      if (vx > whalf+1)  vx = whalf+1 ;
      if (vx < -whalf-1) vx = -whalf-1 ;
      if (vy > whalf+1)  vy = whalf+1 ;
      if (vy < -whalf-1) vy = -whalf-1 ;

      *xpix = -sgn * vx ;
      *ypix = -sgn * vy ;
      if (xfp)
      {
        fprintf(xfp, "\n") ;
        fprintf(yfp, "\n") ;
        fprintf(ofp, "\n") ;
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
  int    x0, y0, rows, cols, x, y, whalf ;
  float  vx, vy, vsq, c1, *g, *xpix, *ypix ;
  static float *gaussian = NULL ;
  static int   w = 0 ;
  register float gauss, fxpix, fypix, dot_product ;
  register int   xc, yc ;

  mu *= mu ;     
  vsq = 0.0f ;  /* prevent compiler warning */

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
        *g = (float)exp(-36.0 * sqrt((double)(xc*xc+yc*yc)) / (double)den) ;
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
        if ((yc < 0) || (yc >= rows))
        {
          g += wsize ;
          continue ;
        }
        
        for (x = -whalf ; x <= whalf ; x++, g++)
        {
          xc = x0 + x ;
          if ((xc < 0) || (xc >= cols))
            continue ;
          
          fxpix = *IMAGEFpix(Ix, xc, yc) ;
          fypix = *IMAGEFpix(Iy, xc, yc) ;
          dot_product = x * fxpix + y * fypix ;
          gauss = *g ;
          dot_product *= gauss ;
          vx += (dot_product * fxpix) ;
          vy += (dot_product * fypix) ;
        }
      }

#if 0
      /* calculated phi(V) */
      vsq = vx*vx + vy*vy ;

      vx = vx*c1 / (float)sqrt((double)(mu*mu + vsq)) ;
      vy = vy*c1 / (float)sqrt((double)(mu*mu + vsq)) ;
#endif
      *xpix = -vx ;
      *ypix = -vy ;
    }
  }

  return(0) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
             normalize offset distances by searching for isopotential 
             surfaces in offset direction, then modifying offset distance
             to be the distance to the isopotential surface.
----------------------------------------------------------------------*/
IMAGE *
ImageNormalizeOffsetDistances(IMAGE *Isrc, IMAGE *Idst, int maxsteps)
{
  float  *src_xpix, *src_ypix, *dst_xpix, *dst_ypix, slope, xf, yf, dot ;
  int    x0, y0, rows, cols, delta, i ;
  register int x, y ;
  register float  dx, dy, odx, ody ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols,Isrc->pixel_format,
                      Isrc->num_frame);

  rows = Isrc->rows ;
  cols = Isrc->cols ;

  src_xpix = IMAGEFpix(Isrc, 0, 0) ;
  src_ypix = IMAGEFseq_pix(Isrc, 0, 0, 1) ;
  dst_xpix = IMAGEFpix(Idst, 0, 0) ;
  dst_ypix = IMAGEFseq_pix(Idst, 0, 0, 1) ;

  /* for each point in the image */
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++,src_xpix++,src_ypix++,dst_xpix++,dst_ypix++)
    {
/* 
      search for the first point in the offset direction who's dot product
      with the current offset vector is below some threshold.
*/
      dx = *src_xpix ;
      dy = *src_ypix ;
      if (FZERO(dx) && (FZERO(dy)))
        continue ;
      
      if (fabs(dx) > fabs(dy))  /* use unit steps in x direction */
      {
        delta = nint(dx / (float)fabs(dx)) ;
        slope = delta  * dy / dx ;
        for (i = 0, yf = (float)y0+slope, x = x0+delta; i <= maxsteps;
             x += delta, yf += slope, i++)
        {
          y = nint(yf) ;
          if (y <= 0 || y >= (rows-1) || x <= 0 || x >= (cols-1))
            break ;
          
          odx = *IMAGEFpix(Isrc, x, y) ;
          ody = *IMAGEFseq_pix(Isrc, x, y, 1) ;
          dot = odx * dx + ody * dy ;
          if (dot <= 0)
            break ;
        }
        x -= delta ;
        y = nint(yf - slope) ;
      }
      else                     /* use unit steps in y direction */
      {
        delta = nint(dy / (float)fabs(dy)) ;
        slope = delta * dx /dy ;
        for (i = 0, xf = (float)x0+slope, y = y0+delta; i < maxsteps;
             y += delta, xf += slope, i++)
        {
          x = nint(xf) ;
          if (y <= 0 || y >= (rows-1) || x <= 0 || x >= (cols-1))
            break ;

          odx = *IMAGEFpix(Isrc, x, y) ;
          ody = *IMAGEFseq_pix(Isrc, x, y, 1) ;
          dot = odx * dx + ody * dy ;
          if (dot <= 0)
            break ;
        }
        y -= delta ;
        x = nint(xf - slope) ;
      }

      *dst_xpix = x - x0 ;
      *dst_ypix = y - y0 ;
    }
  }

  return(Idst) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              smooth offsets by using winner-take-all algorithm in
              a neighborhood in the direction orthogonal to the
              offset.
----------------------------------------------------------------------*/
#define ISSMALL(m)   (fabs(m) < 0.00001f)

#if 0
static float avg[] = { 1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
                       1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f,
                       1.0f/9.0f, 1.0f/9.0f, 1.0f/9.0f } ;
#endif

#if 1
IMAGE *
ImageSmoothOffsets(IMAGE *Isrc, IMAGE *Idst, int wsize)
{
#if 0
  ImageConvolve3x3(Isrc, avg, Idst) ;
#else
  float  *src_xpix, *src_ypix, *dst_xpix, *dst_ypix, slope, dx,dy, f,
         xf, yf, *wdx, *wdy, *wphase, *wmag, dist, mag  ;
  int    x0, y0, rows, cols, x = 0, y, delta, i, whalf, *wx, *wy ;

  wsize = 5 ;
  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols,Isrc->pixel_format,
                      Isrc->num_frame);

  ImageCopy(Isrc, Idst) ;
  whalf = (wsize-1) / 2 ;
/*
  allocate five windows of the size specified by the user. Two to hold
  the offset vectors, one for the magnitude, one for the phase, and 1 for 
  the voting weight.
*/
  wdx = (float *)calloc(wsize, sizeof(float)) ;
  wdy = (float *)calloc(wsize, sizeof(float)) ;
  wphase = (float *)calloc(wsize, sizeof(float)) ;
  wmag = (float *)calloc(wsize, sizeof(float)) ;
  wx = (int *)calloc(wsize, sizeof(float)) ;
  wy = (int *)calloc(wsize, sizeof(float)) ;

  rows = Isrc->rows ;
  cols = Isrc->cols ;

  src_xpix = IMAGEFpix(Isrc, 0, 0) ;
  src_ypix = IMAGEFseq_pix(Isrc, 0, 0, 1) ;
  dst_xpix = IMAGEFpix(Idst, 0, 0) ;
  dst_ypix = IMAGEFseq_pix(Idst, 0, 0, 1) ;

  /* for each point in the image */
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++,src_xpix++,src_ypix++,dst_ypix++,dst_xpix++)
    {
      /* fill the offset vector array */
      dx = *src_xpix ;
      dy = *src_ypix ;




      /* calculate orthogonal slope = -dx/dy */
      f = dx ;
      dx = dy ;
      dy = -f ;
      mag = (float)hypot(dx, dy) ;

      if (ISSMALL(mag))/* don't know what direction to search in */
        continue ;
      else
        if (fabs(dx) > fabs(dy))  /* use unit steps in x direction */
      {
        delta = nint(dx / (float)fabs(dx)) ;
        slope = delta  * dy / dx ;  /* orthogonal slope */
        
        yf = (float)y0-(float)whalf*slope    ;
        x = x0 - whalf * delta ;
        for (i = 0 ; i < wsize ; x += delta, yf += slope, i++)
        {
          y = nint(yf) ;
          wx[i] = x ;
          wy[i] = y ;
          if (y <= 0 || y >= (rows-1) || x <= 0 || x >= (cols-1))
            wdx[i] = wdy[i] = wmag[i]=wphase[i] = 0.0f;
          else
          {
            dx = *IMAGEFpix(Isrc, x, y) ;
            dy = *IMAGEFseq_pix(Isrc, x, y, 1) ;
            wdx[i] = dx ;
            wdy[i] = dy ;
            wmag[i] = (float)hypot((double)dx, (double)dy) ;
            wphase[i] = (float)latan2((double)dy, (double)dx) ;
          }
        }
      }
      else                     /* use unit steps in y direction */
      {
        delta = nint(dy / (float)fabs(dy)) ;
        slope = delta * dx /dy ;          /* orthogonal slope */
        xf = (float)x0-(float)whalf*slope ;
        y = y0 - whalf * delta ;
        for (i = 0 ; i < wsize ; y += delta, xf += slope, i++)
        {
          wx[i] = x ;
          wy[i] = y ;
          x = nint(xf) ;
          if (y <= 0 || y >= (rows-1) || x <= 0 || x >= (cols-1))
            wdx[i] = wdy[i] = wmag[i]=wphase[i] = 0.0f;
          else
          {
            dx = *IMAGEFpix(Isrc, x, y) ;
            dy = *IMAGEFseq_pix(Isrc, x, y, 1) ;
            wdx[i] = dx ;
            wdy[i] = dy ;
            wmag[i] = (float)hypot((double)dx, (double)dy) ;
            wphase[i] = (float)latan2((double)dy, (double)dx) ;
          }
        }
      }

/* 
   check both left and right neighbors in direction orthogonal to our
   offset direction. If our neighbors  left and right neighbors are coherent
   in terms of direction (within some threshold of each other), and the
   neighbor's diirection is not bracketed by them, then modify it.
*/
#define MIN_ANGLE_DIST RADIANS(30.0f)

      if (!ISSMALL(wmag[0]))  /* check 'left' neighbor */
      {
        dist = angleDistance(wphase[0], wphase[2]) ;
        if (dist < MIN_ANGLE_DIST)
        {
#if 0
          if (wx[1] == 13 && wy[1] == 57)
          {
            fprintf(stderr, "left: (%d, %d) | (%d, %d) | (%d, %d)\n", 
                    wx[0], wy[0], wx[1], wy[1], x0, y0) ;
            fprintf(stderr, "phase:  %2.0f -- %2.0f ===> %2.0f\n", 
                    DEGREES(wphase[2]), DEGREES(wphase[4]), 
                    DEGREES((wphase[2]+wphase[4])/2.0f)) ;
            fprintf(stderr, 
              "offset: (%2.2f, %2.2f) -- (%2.2f, %2.2f) ==> (%2.2f, %2.2f)\n", 
                    wdx[2], wdy[2], wdx[4],wdy[4], (wdx[2]+wdx[4])/2.0f,
                    (wdy[2]+wdy[4])/2.0f) ;
          }
#endif

          dx = (wdx[2] + wdx[0]) / 2.0f ;
          dy = (wdy[2] + wdy[0]) / 2.0f ;
/* 
          check to see that the computed angle is not too close to being
          exactly 180 degrees out of alignment with the current one which
          would indicate we are close to the border of a coherent edge.
*/
          dist = angleDistance(wphase[2], wphase[1]) ;
          if ((dist - PI) > MIN_ANGLE_DIST)
          {
            *IMAGEFpix(Idst, wx[1], wy[1]) = dx ;
            *IMAGEFseq_pix(Idst, wx[1], wy[1], 1) = dy ;
          }
        }
      }
      
      if (!ISSMALL(wmag[4]))  /* check 'right' neighbor */
      {
        dist = angleDistance(wphase[4], wphase[2]) ;
        if (dist < MIN_ANGLE_DIST)
        {
#if 0
          if (wx[3] == 13 && wy[3] == 57)
          {
            fprintf(stderr, "right: (%d, %d) | (%d, %d) | (%d, %d)\n", 
                    x0, y0, wx[3], wy[3], wx[4], wy[4]) ;
            fprintf(stderr, "phase:  %2.0f -- %2.0f ===> %2.0f\n", 
                    DEGREES(wphase[2]), DEGREES(wphase[4]), 
                    DEGREES((wphase[2]+wphase[4])/2.0f)) ;
            fprintf(stderr, 
              "offset: (%2.2f, %2.2f) -- (%2.2f, %2.2f) ==> (%2.2f, %2.2f)\n", 
                    wdx[2], wdy[2], wdx[4],wdy[4], (wdx[2]+wdx[4])/2.0f,
                    (wdy[2]+wdy[4])/2.0f) ;
          }
#endif


          dx = (wdx[2] + wdx[4]) / 2.0f ;
          dy = (wdy[2] + wdy[4]) / 2.0f ;
/* 
          check to see that the computed angle is not too close to being
          exactly 180 degrees out of alignment with the current one which
          would indicate we are close to the border of a coherent edge.
*/
          dist = angleDistance(wphase[2], wphase[3]) ;
          if ((dist - PI) > MIN_ANGLE_DIST)
          {
            *IMAGEFpix(Idst, wx[3], wy[3]) = dx ;
            *IMAGEFseq_pix(Idst, wx[3], wy[3], 1) = dy ;
          }
        }
      }
    }
  }

  free(wx) ;
  free(wy) ;
  free(wdx) ;
  free(wdy) ;
  free(wphase) ;
  free(wmag) ;
#endif
  return(Idst) ;
}
#else
IMAGE *
ImageSmoothOffsets(IMAGE *Isrc, IMAGE *Idst, int wsize)
{
  float  *src_xpix, *src_ypix, *dst_xpix, *dst_ypix, slope, dx,dy, f,
         xf, yf, *wdx, *wdy, *weights, *wphase, *wmag, dist, phase, maxw  ;
  int    x0, y0, rows, cols, x, y, delta, i, j, whalf ;

  if (!Idst)
    Idst = ImageAlloc(Isrc->rows, Isrc->cols,Isrc->pixel_format,
                      Isrc->num_frame);

  whalf = (wsize-1) / 2 ;
/*
  allocate five windows of the size specified by the user. Two to hold
  the offset vectors, one for the magnitude, one for the phase, and 1 for 
  the voting weight.
*/
  weights = (float *)calloc(wsize, sizeof(float)) ;
  wdx = (float *)calloc(wsize, sizeof(float)) ;
  wdy = (float *)calloc(wsize, sizeof(float)) ;
  wphase = (float *)calloc(wsize, sizeof(float)) ;
  wmag = (float *)calloc(wsize, sizeof(float)) ;

  rows = Isrc->rows ;
  cols = Isrc->cols ;

  src_xpix = IMAGEFpix(Isrc, 0, 0) ;
  src_ypix = IMAGEFseq_pix(Isrc, 0, 0, 1) ;
  dst_xpix = IMAGEFpix(Idst, 0, 0) ;
  dst_ypix = IMAGEFseq_pix(Idst, 0, 0, 1) ;

  /* for each point in the image */
  for (y0 = 0 ; y0 < rows ; y0++)
  {
    for (x0 = 0 ; x0 < cols ; x0++,src_xpix++,src_ypix++,dst_ypix++,dst_xpix++)
    {
      /* fill the offset vector array */
      dx = *src_xpix ;
      dy = *src_ypix ;
      if (x0 == 30 && y0 == 40 && 0)
        fprintf(stderr, "input (%d, %d) = (%2.1f, %2.1f)\n",
                x0, y0, dx, dy) ;

      /* calculate orthogonal slope = -dx/dy */
      f = dx ;
      dx = dy ;
      dy = -f ;

      if (FZERO(dx) && (FZERO(dy)))/* don't know what direction to search in */
      {
        *dst_xpix = dx ;
        *dst_ypix = dy ;
        continue ;
      }
      else
        if (fabs(dx) > fabs(dy))  /* use unit steps in x direction */
      {
        delta = dx / fabs(dx) ;
        slope = delta  * dy / dx ;  /* orthogonal slope */
        
        yf = (float)y0-(float)whalf*slope    ;
        x = x0 - whalf * delta ;
        for (i = -whalf ; i <= whalf ; x += delta, yf += slope, i++)
        {
          y = nint(yf) ;
          if (y <= 0 || y >= (rows-1) || x <= 0 || x >= (cols-1))
            wdx[i+whalf] = wdy[i+whalf] = wmag[i+whalf]=wphase[i+whalf] = 0.0f;
          else
          {
            dx = *IMAGEFpix(Isrc, x, y) ;
            dy = *IMAGEFseq_pix(Isrc, x, y, 1) ;
            wdx[i+whalf] = dx ;
            wdy[i+whalf] = dy ;
            wmag[i+whalf] = (float)hypot((double)dx, (double)dy) ;
            wphase[i+whalf] = latan2((double)dy, (double)dx) ;
          }
        }
      }
      else                     /* use unit steps in y direction */
      {
        delta = dy / fabs(dy) ;
        slope = delta * dx /dy ;          /* orthogonal slope */
        xf = (float)x0-(float)whalf*slope ;
        y = y0 - whalf * delta ;
        for (i = -whalf ; i <= whalf ; y += delta, xf += slope, i++)
        {
          x = nint(xf) ;
          if (y <= 0 || y >= (rows-1) || x <= 0 || x >= (cols-1))
            wdx[i+whalf] = wdy[i+whalf] = wmag[i+whalf]=wphase[i+whalf] = 0.0f;
          else
          {
            dx = *IMAGEFpix(Isrc, x, y) ;
            dy = *IMAGEFseq_pix(Isrc, x, y, 1) ;
            wdx[i+whalf] = dx ;
            wdy[i+whalf] = dy ;
            wmag[i+whalf] = (float)hypot((double)dx, (double)dy) ;
            wphase[i+whalf] = latan2((double)dy, (double)dx) ;
          }
        }
      }

      /* now fill in weight array */
      for (i = 0 ; i < wsize ; i++)
      {
        phase = wphase[i] ;
        weights[i] = 0.0f ;
        for (j = 0 ; j < wsize ; j++)
        {
          dist = angleDistance(phase, wphase[j]) ;
          weights[i] += (PI - dist) * wmag[j] ;
        }
      }

      /* find maximum weight, and use that as offset vector */
      for (maxw = 0.0f, i = j = 0 ; i < wsize ; i++)
      {
        if (weights[i] > maxw)
        {
          maxw = weights[i] ;
          j = i ;
        }
      }
      *dst_xpix = wdx[j] ;
      *dst_ypix = wdy[j] ;
      if (x0 == 30 && y0 == 40 && 0)
        fprintf(stderr, "output (%d, %d) @ %d = (%2.1f, %2.1f)\n",
                x0, y0, j, wdx[j], wdy[j]) ;

    }
  }

  free(weights) ;
  free(wdx) ;
  free(wdy) ;
  free(wphase) ;
  free(wmag) ;

  return(Idst) ;
}
#endif
/*----------------------------------------------------------------------
            Parameters:

           Description:
              apply an offset vector to a filtered image.
----------------------------------------------------------------------*/
IMAGE *
ImageApplyOffset(IMAGE *Isrc, IMAGE *Ioffset, IMAGE *Idst)
{
  int x, y, rows, cols, dx, dy ;
  float  *dst, *src, *dx_pix, *dy_pix ;
  IMAGE  *Iout, *Iin ;

  rows = Isrc->rows ;
  cols = Isrc->cols ;

  if (!Idst)
    Idst = ImageAlloc(rows, cols, PFFLOAT, 1) ;

  if (Idst->pixel_format != PFFLOAT)
    Iout = ImageAlloc(rows, cols, PFFLOAT, 1) ;
  else
    Iout = Idst ;
  if (Isrc->pixel_format != PFFLOAT)
  {
    Iin = ImageAlloc(rows, cols, PFFLOAT, 1) ;
    ImageCopy(Isrc, Iin) ;
  }
  else
    Iin = Isrc ;

  if (!ImageCheckSize(Isrc, Idst, 0, 0, 0))
    ErrorReturn(NULL, (ERROR_SIZE, "ImageApplyOffset: dst not big enough")) ;

  dst = IMAGEFpix(Iout, 0, 0) ;
  dx_pix = IMAGEFpix(Ioffset, 0, 0) ;
  dy_pix = IMAGEFseq_pix(Ioffset, 0, 0, 1) ;

  for (y = 0 ; y < rows ; y++)
  {
    for (x = 0 ; x < cols ; x++)
    {
      dx = (int)*dx_pix++ ;
      dy = (int)*dy_pix++ ;
      src = IMAGEFpix(Iin, x+dx, y+dy) ;
      *dst++ = *src ;
    }
  }

  if (Iin != Isrc)
    ImageFree(&Iin) ;
  if (Iout != Idst)
  {
    ImageCopy(Iout, Idst) ;
    ImageFree(&Iout) ;
  }
  return(Idst) ;
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
              use an offset vector field to specify edge locations.
----------------------------------------------------------------------*/
IMAGE *
ImageOffsetMedialAxis(IMAGE *Ioffset, IMAGE *Iedge)
{
  int    x, y, rows, cols, dx, dy ;
  float  *dx_pix, *dy_pix ;
  UCHAR  *edge ;
  IMAGE  *Iout ;

  rows = Ioffset->rows ;
  cols = Ioffset->cols ;

  if (!Iedge)
    Iedge = ImageAlloc(rows, cols, PFBYTE, 1) ;

  if (Iedge->pixel_format != PFBYTE)
    Iout = ImageAlloc(rows, cols, PFBYTE, 1) ;
  else
    Iout = Iedge ;

#if 0
  if (!ImageCheckSize(Ioffset, Iout, 0, 0, 1))
    ErrorReturn(NULL,(ERROR_SIZE,"ImageOffsetEdgeDetect: dst not big enough"));
#endif

  /* assume everything is an edge */
  ImageClearArea(Iout, -1, -1, -1, -1, 0.0f) ;  
  dx_pix = IMAGEFpix(Ioffset, 0, 0) ;
  dy_pix = IMAGEFseq_pix(Ioffset, 0, 0, 1) ;

  for (y = 0 ; y < rows ; y++)
  {
    for (x = 0 ; x < cols ; x++)
    {
      dx = (int)*dx_pix++ ;
      dy = (int)*dy_pix++ ;
      edge = IMAGEpix(Iout, x+dx, y+dy) ;
      (*edge)++ ;               /* count # of times used */
    }
  }

  if (Iout != Iedge)
  {
    ImageCopy(Iout, Iedge) ;
    ImageFree(&Iout) ;
  }
  return(Iedge) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
ImageCalculateOffsetDirection(IMAGE *Ix, IMAGE *Iy, int wsize, IMAGE *Ioffset)
{
  int    x0, y0, rows, cols, x, y, whalf ;
  float  vx, vy, *g, *xpix, *ypix ;
  static float *gaussian = NULL ;
  static int   w = 0 ;
  register float gauss, fxpix, fypix, dot_product ;
  register int   xc, yc ;

  rows = Ix->rows ;
  cols = Ix->cols ;

  if (!Ioffset)
    Ioffset = ImageAlloc(rows, cols, PFFLOAT, 2) ;

  whalf = (wsize-1)/2 ;

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
        *g = (float)exp(-36.0 * sqrt((double)(xc*xc+yc*yc)) / (double)den) ;
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

  xpix = IMAGEFpix(Ioffset, 0, 0) ;
  ypix = IMAGEFseq_pix(Ioffset, 0, 0, 1) ;
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
        if ((yc < 0) || (yc >= rows))
        {
          g += wsize ;
          continue ;
        }
        
        for (x = -whalf ; x <= whalf ; x++, g++)
        {
          xc = x0 + x ;
          if ((xc < 0) || (xc >= cols))
            continue ;
          
          fxpix = *IMAGEFpix(Ix, xc, yc) ;
          fypix = *IMAGEFpix(Iy, xc, yc) ;
          dot_product = x * fxpix + y * fypix ;
          gauss = *g ;
          dot_product *= gauss ;
          vx += (dot_product * fxpix) ;
          vy += (dot_product * fypix) ;
        }
      }

      *xpix = -vx ;
      *ypix = -vy ;
    }
  }

  return(Ioffset) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              use an offset field to scale an image

              first generate an offset field using the full resolution, but
              only performing the offset calculation on pixel centers of the
              scaled image. Then do length computation using gradient reversal
              in the full image as the criterion.
----------------------------------------------------------------------*/
#define SMOOTH_SIGMA 4.0f
#define WSIZE        3
#define WHALF        (WSIZE-1)/2

/* global for debugging, make local if this routine is ever really used. */
IMAGE *Ioffset = NULL, *Ioffset2 = NULL, *Ix = NULL, *Iy = NULL, *Ismooth = NULL ;

#define MEDIAN_INDEX  ((3*3-1)/2)
static int compare_sort_array(const void *pf1, const void *pf2) ;

IMAGE *
ImageOffsetScale(IMAGE *Isrc, IMAGE *Idst)
{
  static IMAGE   *Igauss = NULL ;
  int     srows, scols, drows, dcols, xs, ys ;
  int     x0, y0, x, y, ystep, xstep, maxsteps, i, idx, idy ;
  float   vx, vy, *g, *xpix, *ypix, dx, dy, ox, oy, delta, slope,
          xf, yf, odx, ody, *dpix, *spix, sort_array[3*3], *sptr ;
  static float *gaussian = NULL ;
  register float gauss, fxpix, fypix, dot_product ;
  register int   xc, yc ;

  srows = Isrc->rows ;
  scols = Isrc->cols ;
  drows = Idst->rows ;
  dcols = Idst->cols ;

  xstep = nint((double)scols / (double)dcols) ;
  ystep = nint((double)srows / (double)drows) ;

  if (Ioffset && ((Ioffset->rows != drows) || (Ioffset->cols != dcols)))
  {
    ImageFree(&Ioffset) ;
    ImageFree(&Ioffset2) ;
  }
  if (Ix && ((Ix->rows != srows) || (Ix->cols != scols)))
  {
    ImageFree(&Ix) ;
    ImageFree(&Iy) ;
    ImageFree(&Ismooth) ;
  }

  if (!Ix)
  {
    Ix = ImageAlloc(srows, scols, PFFLOAT, 1) ;
    Iy = ImageAlloc(srows, scols, PFFLOAT, 1) ;
    Ismooth = ImageAlloc(srows, scols, PFFLOAT, 1) ;
  }
  if (!Ioffset)
  {
    Ioffset = ImageAlloc(drows, dcols, PFFLOAT, 2) ;
    Ioffset2 = ImageAlloc(drows, dcols, PFFLOAT, 2) ;
  }
  if (!Igauss)
    Igauss = ImageGaussian1d(SMOOTH_SIGMA, 5) ;

  ImageConvolveGaussian(Isrc, Igauss, Ismooth, 0) ;
/* ImageWrite(Ismooth, "smooth.hipl") ;*/
  ImageSobel(Ismooth, NULL, Ix, Iy) ;

  /* now calculate offset image */
  if (!gaussian)  /* allocate a gaussian bump */
  {
    float den, norm ;

    gaussian = (float *)calloc(WSIZE*WSIZE, sizeof(float)) ;
    den = WSIZE*WSIZE + WSIZE + 1 ;
    norm = 0.0f ;
    for (g = gaussian, y = 0 ; y < WSIZE ; y++)
    {
      yc = y - WHALF ;
      for (x = 0 ; x < WSIZE ; x++, g++) 
      {
        xc = x - WHALF ;
        *g = (float)exp(-36.0 * sqrt((double)(xc*xc+yc*yc)) / (double)den) ;
        norm += *g ;
      }
    }

    /* normalize gaussian */
    for (g = gaussian, y = 0 ; y < WSIZE ; y++)
    {
      for (x = 0 ; x < WSIZE ; x++, g++) 
        *g /= norm ;
    }
  }

  xpix = IMAGEFpix(Ioffset, 0, 0) ;
  ypix = IMAGEFseq_pix(Ioffset, 0, 0, 1) ;
/*
  x0 and y0 are the coordinates of the center of the window in the unscaled image.
  xc and yc are the coordinates of the window used for the offset computation in
     the unscaled coordinates.
  x and y are local window coordinates (i.e. -1 .. 1)
*/
  for (y0 = nint(0.5*(double)ystep) ; y0 < srows ; y0 += ystep)
  {
    for (x0 = nint(0.5*(double)xstep) ; x0 < scols ; x0 += xstep, xpix++, ypix++)
    {
      
/*
  x and y are in window coordinates, while xc and yc are in image
  coordinates.
*/
      /* build offset direction */
      vx = vy = 0.0f ;
      for (g = gaussian, y = -WHALF ; y <= WHALF ; y++)
      {
        /* reflect across the boundary */
        yc = y + y0 ;
        if ((yc < 0) || (yc >= srows))
        {
          g += WSIZE ;
          continue ;
        }
        
        for (x = -WHALF ; x <= WHALF ; x++, g++)
        {
          xc = x0 + x ;
          if ((xc < 0) || (xc >= scols))
            continue ;
          
          fxpix = *IMAGEFpix(Ix, xc, yc) ;
          fypix = *IMAGEFpix(Iy, xc, yc) ;
          dot_product = x * fxpix + y * fypix ;
          gauss = *g ;
          dot_product *= gauss ;
          vx += (dot_product * fxpix) ;
          vy += (dot_product * fypix) ;
        }
      }

      *xpix = -vx ;
      *ypix = -vy ;
    }
  }


#if 0
ImageWrite(Ioffset, "offset1.hipl") ;
ImageWrite(Ix, "ix.hipl") ;
ImageWrite(Iy, "iy.hipl") ;
#endif

  ImageCopy(Ioffset, Ioffset2) ;
/* 
  now normalize offset lengths by searching for reversal in gradient field 
  in offset direction
 */
  maxsteps = MAX(xstep, ystep)*2 ;
  xpix = IMAGEFpix(Ioffset2, 0, 0) ;
  ypix = IMAGEFseq_pix(Ioffset2, 0, 0, 1) ;
  for (ys = 0, y0 = nint(0.5*(double)ystep) ; y0 < srows ; y0 += ystep, ys++)
  {
    for (xs=0,x0 = nint(0.5*(double)xstep) ; x0 < scols ; x0 += xstep, xpix++, 
         ypix++,xs++)
    {
      ox = *xpix ;   /* initial offset direction */
      oy = *ypix ;

#if 0
      if (xs == 29 && ys == 27)
      {
        fprintf(stderr, "initial offset: (%2.5f, %2.5f)\n", ox, oy) ;
        break_here() ;
      }
#endif
      dx = *IMAGEFpix(Ix, x0, y0) ;  /* starting gradient values */
      dy = *IMAGEFpix(Iy, x0, y0) ;
#if 0
      if (xs == 29 && ys == 27)
        fprintf(stderr, "initial gradient is (%2.5f, %2.5f)\n", dx, dy) ;
#endif

#if 0
      if (FZERO(ox) && (FZERO(oy)))
#else
#define SMALL 0.00001f

      if ((fabs(ox) < SMALL) && (fabs(oy) < SMALL))
#endif
        continue ;

      if (fabs(ox) > fabs(oy))  /* use unit steps in x direction */
      {
        delta = nint(ox / (float)fabs(ox)) ;
        slope = delta  * oy / ox ;
        for (i = 0, yf = (float)y0+slope, x = x0+delta; i <= maxsteps;
             x += delta, yf += slope, i++)
        {
          y = nint(yf) ;
          if (y <= 0 || y >= (srows-1) || x <= 0 || x >= (scols-1))
            break ;
          
          odx = *IMAGEFpix(Ix, x, y) ;
          ody = *IMAGEFpix(Iy, x, y) ;
          dot_product = odx * dx + ody * dy ;
          if (dot_product <= 0)
            break ;
        }
        x -= delta ;
        y = nint(yf - slope) ;
      }
      else                     /* use unit steps in y direction */
      {
        delta = nint(oy / (float)fabs(oy)) ;
        slope = delta * ox / oy ;
        for (i = 0, xf = (float)x0+slope, y = y0+delta; i < maxsteps;
             y += delta, xf += slope, i++)
        {
          x = nint(xf) ;
          if (y <= 0 || y >= (srows-1) || x <= 0 || x >= (scols-1))
            break ;

          odx = *IMAGEFpix(Ix, x, y) ;
          ody = *IMAGEFpix(Iy, x, y) ;
          dot_product = odx * dx + ody * dy ;
          if (dot_product <= 0)
            break ;
        }
        y -= delta ;
        x = nint(xf - slope) ;
      }

      if (((x - x0) > maxsteps+1) || ((y-y0) > maxsteps+1))
        break_here() ;
      *xpix = x - x0 ;
      *ypix = y - y0 ;
    }
  }

#if 0 
ImageWrite(Ioffset2, "offset2.hipl") ;
#endif

  /* now use the offset field to scale the image with a median filter */
  xpix = IMAGEFpix(Ioffset2, 0, 0) ;
  ypix = IMAGEFseq_pix(Ioffset2, 0, 0, 1) ;
  dpix = IMAGEFpix(Idst, 0, 0) ;

  /* apply median filter */
  for (y0 = nint(0.5*(double)ystep) ; y0 < srows ; y0 += ystep)
  {
    for (x0 = nint(0.5*(double)xstep) ; x0 < scols ; x0 += xstep,xpix++,ypix++)
    {
      idx = nint(*xpix) ;   /*  offset direction */
      idy = nint(*ypix) ;

      for (sptr = sort_array, y = -WHALF ; y <= WHALF ; y++)
      {
        yc = y + y0 + idy ;
        if (yc < 0)
          yc = 0 ;
        else if (yc >= srows)
          yc = srows - 1 ;

        spix = IMAGEFpix(Isrc, 0, yc) ;
        for (x = -WHALF ; x <= WHALF ; x++)
        {
          xc = x0 + x + idx ;
          if (xc < 0)
            xc = 0 ;
          else if (xc >= scols)
            xc = scols - 1 ;
#if 0            
          *sptr++ = *IMAGEFseq_pix(inImage, xc, yc, frame) ;
#else
          *sptr++ = *(spix + xc) ;
#endif
        }
      }
      qsort(sort_array, 3*3, sizeof(float), compare_sort_array) ;
      *dpix++ = sort_array[MEDIAN_INDEX] ;
    }
  }

#if 0
ImageWrite(Idst, "scale.hipl") ;
#endif

#if 0
  ImageFree(&Igauss) ;
  ImageFree(&Ismooth) ;
  ImageFree(&Ix) ;
  ImageFree(&Iy) ;
  ImageFree(&Ioffset) ;
  ImageFree(&Ioffset2) ;
#endif

  return(Idst) ;
}

static int
compare_sort_array(const void *pf1, const void *pf2)
{
  register float f1, f2 ;

  f1 = *(float *)pf1 ;
  f2 = *(float *)pf2 ;

/*  return(f1 > f2 ? 1 : f1 == f2 ? 0 : -1) ;*/
  if (f1 > f2)
    return(1) ;
  else if (f1 < f2)
    return(-1) ;

  return(0) ;
}
