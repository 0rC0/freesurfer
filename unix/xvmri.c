/*----------------------------------------------------------------------
           File Name:  xvmri.h

           Author:

           Description:

           Conventions:

----------------------------------------------------------------------*/
/*----------------------------------------------------------------------
                           INCLUDE FILES
----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "xvutil.h"
#include "mri.h"
#include "proto.h"
#include "xvmri.h"
#include "error.h"
#include "diag.h"
#include "histo.h"
#include "utils.h"

/*----------------------------------------------------------------------
                           CONSTANTS
----------------------------------------------------------------------*/

#define MAX_IMAGES  30

/*----------------------------------------------------------------------
                           GLOBAL DATA
----------------------------------------------------------------------*/

IMAGE          *Idisplay[MAX_IMAGES] = { NULL } ;
MRI            *mris[MAX_IMAGES] ;
int            mri_views[MAX_IMAGES] ;
int            mri_depths[MAX_IMAGES] ;
int            mri_frames[MAX_IMAGES] ;
int            which_click = -1 ;

/*----------------------------------------------------------------------
                           STATIC DATA
----------------------------------------------------------------------*/

static XV_FRAME       *xvf ;
static Menu           view_menu ;
static char           view_str[100] ;
static Panel_item     view_panel ;

static int            x_click ;
static int            y_click ;
static int            z_click ;

static char           image_path[100] = "." ;

static int            talairach = 0 ; /* show image or Talairach coords */

/*----------------------------------------------------------------------
                        STATIC PROTOTYPES
----------------------------------------------------------------------*/

static void viewMenuItem(Menu menu, Menu_item menu_item) ;
static IMAGE *get_next_slice(IMAGE *Iold, int which, int dir) ;
static void repaint_handler(XV_FRAME *xvf, DIMAGE *dimage) ;
static int mri_write_func(Event *event, DIMAGE *dimage, char *fname) ;

/*----------------------------------------------------------------------
                              FUNCTIONS
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void 
mri_event_handler(XV_FRAME *xvf, Event *event,DIMAGE *dimage, 
                  int *px, int *py, int *pz)
{
  int       x, y, z, which, depth, view, frame, which2 ;
  DIMAGE    *dimage2 ;
  Real      xr, yr, zr, xt, yt, zt, xv, yv, zv, xtv, ytv, ztv ;
  float     xf, yf, zf, xft, yft, zft ;
  MRI       *mri ;
  char      fname[100] ;
  FILE      *fp ;

  which = dimage->which ;
  mri = mris[which] ;
  depth = mri_depths[which] ;
  frame = mri_frames[which] ;
  view = mri_views[which] ;

  /* click can occur in the middle of other stuff (sort of asynchonous) */
  if (!mri || !mri->slices)  
    return ;

/*
  The convention  is  that  positive xspace coordinates run
  from the patient's  left  side  to  right  side,  positive
  yspace  coordinates run from patient posterior to anterior
  and positive zspace coordinates run from inferior to superior.
 */   
  switch (mri_views[which])
  {
  default:
  case MRI_CORONAL:
    /*
      Z=0 is the back of the head,
      X=0 is the right side of the head
      Y=0 is the neck/brain stem
      */
    x = event_x(event) ;
    if (xvf->ydir < 0)
      y = mri->height - (event_y(event)+1) ;
    else
      y = event_y(event) ;
    z = depth - mri->imnr0 ;
    break ;
  case MRI_HORIZONTAL:
    x = event_x(event) ;
    y = depth ;
    if (xvf->ydir < 0)
      z = mri->height - (event_y(event)+1) ;
    else
      z = event_y(event) ;
    break ;
  case MRI_SAGITAL:
    x = depth ;
    if (xvf->ydir < 0)
      y = mri->height - (event_y(event)+1) ;
    else
      y = event_y(event) ;
    z = event_x(event) ;
    break ;
  }

  if (x <= 0)
    x = 0 ;
  else if (x >= mri->width)
    x = mri->width-1 ;
  if (y <= 0)
    y = 0 ;
  else if (y >= mri->height)
    y = mri->height-1 ;
  if (z <= 0)
    z = 0 ;
  else if (z >= mri->depth)
    z = mri->depth-1 ;

  if (talairach)
  {
    MRIvoxelToTalairach(mri, (Real)x, (Real)y, (Real)z, &xt, &yt, &zt) ;
    MRIvoxelToTalairachVoxel(mri, (Real)x, (Real)y, (Real)z, &xtv,&ytv,&ztv);
  }
  MRIvoxelToWorld(mri, (Real)x, (Real)y, (Real)z, &xr, &yr, &zr) ;
  switch (event_id(event))
  {
  case LOC_DRAG:
    if (!event_left_is_down(event))
      return ;
  case MS_LEFT:
    switch (mri->type)
    {
    case MRI_UCHAR:
      if (talairach)
        XVprintf(xvf, 0, "T: (%d,%d,%d) --> %d",
                 nint(xt),nint(yt),nint(zt),
                 MRIseq_vox(mri, nint(xtv), nint(ytv), nint(ztv),frame));
      else
        XVprintf(xvf, 0, "(%d,%d,%d) --> %d",x,y,z,
                 MRIseq_vox(mri,x,y,z,frame));
      break ;
    case MRI_FLOAT:
      if (talairach)
        XVprintf(xvf, 0, "T: (%d,%d,%d) --> %2.3f",
                 nint(xt),nint(yt),nint(zt),
                 MRIFseq_vox(mri, nint(xtv), nint(ytv), nint(ztv),frame));
      else
        XVprintf(xvf, 0, "(%d,%d,%d) --> %2.3f",x,y,z,
                 MRIFseq_vox(mri, x, y, z,frame));
      break ;
    }
    break ;
  default:
    if (event_is_up(event)) switch ((char)event->ie_code)
    {
    case 'S':   /* change all views and slices to be the same */
      for (which2 = 0 ; which2 < xvf->rows*xvf->cols ; which2++)
      {
        if (which2 == which)
          continue ;
        dimage2 = XVgetDimage(xvf, which2, DIMAGE_IMAGE) ;
        XVMRIsetView(xvf, which2, mri_views[which]) ;
      }
      break ;
    case 'G':
    case 'g':
      /* look in 4 places for edit.dat - same dir as image, tmp/edit.dat
         ../tmp and ../../tmp
         */
      sprintf(fname, "%s/edit.dat", image_path) ;
      if (!FileExists(fname))
      {
        sprintf(fname, "%s/../tmp/edit.dat", image_path) ;
        if (!FileExists(fname))
        {
          sprintf(fname, "%s/../../tmp/edit.dat", image_path) ;
          if (!FileExists(fname))
          {
            sprintf(fname, "%s/tmp/edit.dat", image_path) ;
            if (!FileExists(fname))
            {
              XVprintf(xvf, 0, "could not find edit.dat from %s", image_path) ;
              return ;
            }
          }
        }
      }
      fp = fopen(fname, "r") ;
      if (fscanf(fp, "%f %f %f", &xf, &yf, &zf) != 3)
      {
        XVprintf(xvf, 0, "could not scan coordinates out of %s", fname) ;
        fclose(fp) ;
        return ;
      }
      if (fscanf(fp, "%f %f %f", &xft, &yft, &zft) != 3)
      {
        XVprintf(xvf,0,"could not scan Talairach coordinates out of %s",fname);
        fclose(fp) ;
        return ;
      }
      fclose(fp) ;
      if (talairach)
        MRItalairachToVoxel(mri, (Real)xft, (Real)yft, (Real)zft,&xv, &yv,&zv);
      else
        MRIworldToVoxel(mri, (Real)xf, (Real)yf, (Real)zf, &xv, &yv, &zv) ;
      XVMRIsetPoint(xvf, which, nint(xv), nint(yv), nint(zv)) ;
      XVprintf(xvf, 0, "current point: (%d, %d, %d) --> (%d, %d, %d)",
               nint(xf), nint(yf), nint(zf), nint(xv), nint(yv), nint(zv)) ;
#if 0
fprintf(stderr, "read (%2.3f, %2.3f, %2.3f) and (%2.3f, %2.3f, %2.3f)\n",
        xf, yf, zf, xft, yft, zft) ;
fprintf(stderr, "voxel (%d, %d, %d)\n", nint(xv), nint(yv), nint(zv)) ;
#endif
      break ;
    case 'W':
    case 'w':
      /* look in 4 places for edit.dat - same dir as image, tmp/edit.dat
         ../tmp and ../../tmp
         */
      sprintf(fname, "%s/edit.dat", image_path) ;
      if (!FileExists(fname))
      {
        sprintf(fname, "%s/../tmp/edit.dat", image_path) ;
        if (!FileExists(fname))
        {
          sprintf(fname, "%s/../../tmp/edit.dat", image_path) ;
          if (!FileExists(fname))
          {
            sprintf(fname, "%s/tmp/edit.dat", image_path) ;
            if (!FileExists(fname))
            {
              XVprintf(xvf, 0, "could not find edit.dat from %s", image_path) ;
              return ;
            }
          }
        }
      }
      fp = fopen(fname, "w") ;
      MRIvoxelToWorld(mri, (Real)x_click, (Real)y_click, (Real)z_click, 
                      &xr, &yr, &zr) ;
      MRIvoxelToTalairach(mri, (Real)x_click, (Real)y_click, (Real)z_click, 
                          &xt, &yt, &zt) ;
      fprintf(fp, "%f %f %f\n", (float)xr, (float)yr, (float)zr) ;
      fprintf(fp, "%f %f %f\n", (float)xt, (float)yt, (float)zt) ;
      fclose(fp) ;
#if 0
fprintf(stderr, "wrote (%2.3f, %2.3f, %2.3f) and (%2.3f, %2.3f, %2.3f)\n",
        xr, yr, zr, xt, yt, zt) ;
fprintf(stderr, "voxel (%d, %d, %d)\n", x_click, y_click, z_click) ;
#endif
      break ;
    case 'x':
    case 'X':
      XVMRIsetView(xvf, which, MRI_SAGITAL) ;
      break ;
    case 'y':
    case 'Y':
      XVMRIsetView(xvf, which, MRI_HORIZONTAL) ;
      break ;
    case 'z':
    case 'Z':
      XVMRIsetView(xvf, which, MRI_CORONAL) ;
      break ;
    case 'T':
      talairach = 1 ;
      break ;
    case 't':
      talairach = 0 ;
      break ;
    case 'h':
#if 0
      {
        HISTOGRAM *histo ;
        float     fmin, fmax ;
        XV_FRAME  *xvnew ;

        MRIvalRange(mri, &fmin, &fmax) ;
        histo = MRIhistogram(mri, (int)(fmax - fmin + 1)) ;
        xvnew = XValloc(1, 1, 2, 200, 200, "histogram tool", NULL) ;
        XVshowHistogram(xvnew, 0, histo) ;
        xv_main_loop(xvnew->frame);
        HISTOfree(&histo) ;
      }
#endif
      break ;
    }
    break ;
  }

#if 0
  if (event_is_up(event) && (event_id(event) == MS_LEFT))
#else
  if (event_id(event) == MS_LEFT)
#endif
  {
    int view, old_which ;

    if (which_click != which)   /* erase old point */
    {
      old_which = which_click ;
      which_click = dimage->which ;
      XVrepaintImage(xvf, old_which) ;
      view = mri_views[which] ;
      sprintf(view_str, "view: %s", 
              view == MRI_CORONAL ? "CORONAL" :
              view == MRI_SAGITAL ? "SAGITAL" : "HORIZONTAL") ;
      xv_set(view_panel, PANEL_LABEL_STRING, view_str, NULL) ;
    }

    XVMRIsetPoint(xvf, which_click, x, y, z) ;
  }
  if (px)
    *px = x ;
  if (py)
    *py = y ;
  if (pz)
    *pz = z ;
}

/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVMRIdrawPoint(XV_FRAME *xvf, int which, int view, int depth, MRI *mri,
               int x,int y,int z,int color)
{
  int xi, yi ;

  switch (mri_views[which])
  {
  default:
  case MRI_CORONAL:
    /*
      Z=0 is the back of the head,
      X=0 is the right side of the head
      Y=0 is the neck/brain stem
      */
    xi = x ;
    yi = y ;
    break ;
  case MRI_HORIZONTAL:
    xi = x ;
    yi = z ;
    break ;
  case MRI_SAGITAL:
    xi = z ;
    yi = y ;
    break ;
  }
  XVdrawPoint(xvf, which, xi, yi, color) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVMRIdrawRegion(XV_FRAME *xvf, int which, int view, int depth, MRI *mri,
                MRI_REGION *reg, int color)
{
  int xi, yi, dx, dy ;

  switch (view)
  {
  default:
  case MRI_CORONAL:
    /*
      Z=0 is the back of the head,
      X=0 is the right side of the head
      Y=0 is the neck/brain stem
      */
    xi = reg->x ;
    yi = reg->y ;
    dx = reg->dx ;
    dy = reg->dy ;
    break ;
  case MRI_HORIZONTAL:
    xi = reg->x ;
    yi = reg->z ;
    dx = reg->dx ;
    dy = reg->dz ;
    break ;
  case MRI_SAGITAL:
    xi = reg->z ;
    dx = reg->dz ;
    yi = reg->y ;
    dy = reg->dy ;
    break ;
  }
  XVdrawBox(xvf, which, xi, yi, dx, dy, color) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
XVMRIshowFrame(XV_FRAME *xvf, MRI *mri, int which, int slice,int frame)
{
  IMAGE  *I ;
  float  mag ;

  if (frame < 0)
    frame = 0 ;

  if (which_click == which && mri != mris[which])
    which_click = -1 ;  /* a new MR image shown */

  if (!mri)
    return(NULL) ;

  mri_frames[which] = frame ;

  if (slice < 0)  /* set slice to middle of slice direction */
  {
    switch (mri_views[which])
    {
    case MRI_CORONAL:
      slice = (mri->imnr0 + mri->imnr1) / 2 ;
      break ;
    case MRI_SAGITAL:
      slice = mri->width / 2 ;
      break ;
    case MRI_HORIZONTAL:
      slice = mri->height / 2 ;
      break ;
    }
  }

  mri_depths[which] = slice ;
  switch (mri_views[which])
  {
  case MRI_CORONAL:
    slice -= mri->imnr0 ;
    break ;
  case MRI_SAGITAL:
  case MRI_HORIZONTAL:
    break ;
  }

  
  I = MRItoImageView(mri, Idisplay[which], slice, mri_views[which], frame) ;
  if (!I)
    return(NULL) ;


  mag = MIN((float)xvf->orig_disp_rows / (float)I->rows,
            (float)xvf->orig_disp_cols / (float)I->cols) ;

  XVsetImageSize(xvf, which, nint((float)I->rows*mag), 
                 nint((float)I->cols*mag));
  XVresize(xvf) ;

  /* must be done before XVshowImage to draw point properly */
  if (which_click < 0)  /* reset current click point */
  {
    which_click = which ;
    z_click = slice ;
    y_click = mri->height / 2 ;
    x_click = mri->width / 2 ;
#if 0
    XVMRIdrawPoint(xvf, which, mri_views[which], 0, mri, x_click,
                   y_click, z_click, XXOR);
#endif
  }
  XVshowImage(xvf, which, I, 0) ;


  if (Idisplay[which] && (I != Idisplay[which]))
    ImageFree(&Idisplay[which]) ;

  Idisplay[which] = I ;

  mris[which] = mri ;
  return(I) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
IMAGE *
XVMRIshow(XV_FRAME *xvf, MRI *mri, int which, int slice)
{
  return(XVMRIshowFrame(xvf, mri, which, slice, 0)) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int  
XVMRIinit(XV_FRAME *xvf_init, int view_row, int view_col)
{
  int i ;

  for (i = 0 ; i < MAX_IMAGES ; i++)
    mri_views[i] = MRI_CORONAL ;

  xvf = xvf_init ;
  XVsetDepthFunc(xvf, get_next_slice) ;
  XVsetPrintStatus(xvf, 0) ;
  XVsetYDir(xvf, 1) ;
  XVsetWriteFunc(xvf, "WRITE MRI", "MR image file name", mri_write_func) ;
  sprintf(view_str, "view: CORONAL") ;
  view_menu = (Menu)
    xv_create((Xv_opaque)NULL, MENU,
              MENU_NOTIFY_PROC,    viewMenuItem,
              MENU_STRINGS,        "CORONAL", "SAGITAL", "HORIZONTAL", NULL,
              NULL) ;

  view_panel = (Panel_item)
    xv_create(xvf->panel, PANEL_BUTTON,
              PANEL_LABEL_STRING,    view_str,
              XV_X,                  view_col,
              XV_Y,                  view_row,
              PANEL_ITEM_MENU,       view_menu,
              NULL) ;

  XVsetRepaintHandler(repaint_handler) ;
  return(NO_ERROR) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void 
viewMenuItem(Menu menu, Menu_item menu_item)
{
  char   *menu_str ;
  MRI    *mri ;
  int    which, view ;

  which = which_click ;
  if (which_click < 0)   /* no current window */
    which = 0 ;

  mri = mris[which] ;
  if (!mri)              /* no mri in current window */
    return ;

  menu_str = (char *)xv_get(menu_item, MENU_STRING) ;

  if (!stricmp(menu_str, "CORONAL"))
    view = MRI_CORONAL ;
  else if (!stricmp(menu_str, "SAGITAL"))
    view = MRI_SAGITAL ;
  else
    view = MRI_HORIZONTAL ;

  XVMRIsetView(xvf, which, view) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static IMAGE *
get_next_slice(IMAGE *Iold, int which, int dir)
{
  IMAGE *I ;
  MRI   *mri ;
  int   depth, offset ;

  if (!Iold)
    return(NULL) ;

  mri = mris[which] ;
  if (mri)
  {
    depth = mri_depths[which] + dir ;
    if (mri_views[which] == MRI_CORONAL)
      offset = mri->imnr0 ;
    else
      offset = 0 ;

    I = MRItoImageView(mri, Idisplay[which], depth-offset, mri_views[which],
                       mri_frames[which]);
    
    if (!I)
      I = Idisplay[which] ;         /* failed to get next slice */
    else
    {
      mri_depths[which] = depth ;   /* new depth is valid */
      if (Idisplay[which] && (Idisplay[which] != I))
        ImageFree(&Idisplay[which]) ;
      Idisplay[which] = I ;
    }

    /* if current image is changing depth, then change the location of
       the current click to agree with the current depth.
       */
    if (which_click == which) 
    {
      switch (mri_views[which])
      {
      case MRI_CORONAL:
        z_click = depth - mri->imnr0 ;
        break ;
      case MRI_SAGITAL:
        x_click = depth ;
        break ;
      case MRI_HORIZONTAL:
        y_click = depth ;
        break ;
      }
      XVMRIsetPoint(xvf, which_click, x_click, y_click, z_click) ;
    }
  }
  else
    I = Iold ;

  return(I) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
void
XVMRIshowAll(XV_FRAME *xvf)
{
  int i ;

  for (i = 0 ; i < MAX_IMAGES ; i++)
    XVMRIshowFrame(xvf, mris[i], i, mri_depths[i], mri_frames[i]) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static void
repaint_handler(XV_FRAME *xvf, DIMAGE *dimage)
{
  if (dimage->which == which_click && (mris[which_click] != NULL))
    XVMRIdrawPoint(xvf, which_click, mri_views[which_click], 0, 
                   mris[which_click], x_click, y_click, z_click, XRED) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
XVMRIfree(MRI **pmri, int which)
{
  MRI *mri ;

  mri = *pmri ;
  mris[which] = NULL ;
  MRIfree(pmri) ;
  return(NO_ERROR) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
XVMRIsetView(XV_FRAME *xvf, int which, int view)
{
  int     slice, which2, offset, slice2, sync ;
  DIMAGE  *dimage, *dimage2 ;
  MRI     *mri, *mri2 ;
  char    *menu_str ;

  if (!mris[which])
    return(NO_ERROR) ;

  mri_views[which] = view ;
  mri = mris[which] ;

  switch (view)
  {
  default:
  case MRI_CORONAL:
    slice = z_click + mri->imnr0 ;
    menu_str = "CORONAL" ;
    break ;
  case MRI_SAGITAL:
    slice = x_click ;
    menu_str = "SAGITAL" ;
    break ;
  case MRI_HORIZONTAL:
    slice = y_click ;
    menu_str = "HORIZONTAL" ;
    break ;
  }

  if (which == which_click)
  {
    sprintf(view_str, "view: %s", menu_str) ;
    xv_set(view_panel, PANEL_LABEL_STRING, view_str, NULL) ;
  }

  dimage = XVgetDimage(xvf, which, DIMAGE_IMAGE) ;
  sync = dimage->sync ;
  if (sync)
  {
    for (which2 = 0 ; which2 < xvf->rows*xvf->cols ; which2++)
    {
      if (which2 == which)
        continue ;
      dimage2 = XVgetDimage(xvf, which2, DIMAGE_IMAGE) ;
      mri2 = mris[which2] ;
      if (dimage2 && mri2)
      {
        switch (view)
        {
        case MRI_CORONAL:
          offset = mri->zstart - mri2->zstart ;
          slice2 = slice - mri->imnr0 ;  /* turn it into an index */
          slice2 = (slice2+offset) / mri2->zsize + mri2->imnr0 ;
          break ;
        case MRI_SAGITAL:
          offset = mri->xstart - mri2->xstart ;
          slice2 = (slice+offset) / mri2->xsize ;
          break ;
        case MRI_HORIZONTAL:
          offset = mri->ystart - mri2->ystart ;
          slice2 = (slice+offset) / mri2->ysize ;
          break ;
        default:
          slice2 = slice ;
        }
/*        XVMRIsetView(xvf, which2, view) ;*/
        mri_views[which2] = view ;
        XVMRIshowFrame(xvf, mri2, which2, slice2, mri_frames[which2]) ;
      }
    }
  }

  /* will reset syncs */
  XVMRIshowFrame(xvf, mri, which, slice, mri_frames[which]) ;  
  if (sync)  /* if they were synced, reinstate it */
    XVsyncAll(xvf, which) ;
  return(NO_ERROR) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
static int
mri_write_func(Event *event, DIMAGE *dimage, char *fname)
{
  int  which ;
  MRI  *mri ;

  which = dimage->which ;
  mri = mris[which] ;
  if (!mri)
    return(ERROR_BADPARM) ;
  MRIwrite(mri, fname) ;
  return(NO_ERROR) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
----------------------------------------------------------------------*/
int
XVMRIsetPoint(XV_FRAME *xvf, int which, int x, int y, int z)
{
  DIMAGE   *dimage, *dimage2 ;
  int      which2, x2, y2, z2, slice ;
  MRI      *mri, *mri2 ;
  char     fmt[150], title[50], buf[100] ;
  Real     xr, yr, zr ;

  dimage = XVgetDimage(xvf, which, DIMAGE_IMAGE) ;
  mri = mris[which] ;
  x_click = x ;
  y_click = y ;
  z_click = z ;

  switch (mri_views[which])   /* make sure correct slice is shown */
  {
  default:
  case MRI_CORONAL:
    slice = z + mri->imnr0 ;
    break ;
  case MRI_SAGITAL:
    slice = x ;
    break ;
  case MRI_HORIZONTAL:
    slice = y ;
    break ;
  }

  if (slice != mri_depths[which])
    XVMRIshowFrame(xvf, mri, which, slice, mri_frames[which]) ;
  else
    XVrepaintImage(xvf, which) ;

  if (dimage->sync)
  {
    for (which2 = 0 ; which2 < xvf->rows*xvf->cols ; which2++)
    {
      dimage2 = XVgetDimage(xvf, which2, DIMAGE_IMAGE) ;
      mri2 = mris[which2] ;
      if (dimage2 /* && (which2 != which) */ && mri2)
      {
        MRIvoxelToVoxel(mri, mri2, (Real)x, (Real)y, (Real)z, &xr, &yr, &zr);
        x2 = nint(xr) ; y2 = nint(yr) ; z2 = nint(zr) ;
        x2 = MAX(0, x2) ; y2 = MAX(0, y2) ; z2 = MAX(0, z2) ;
        x2 = MIN(mri2->width-1, x2) ; y2 = MIN(mri2->height-1, y2) ; 
        z2 = MIN(mri2->depth-1, z2) ;
        XVgetTitle(xvf, which2, title, 0) ;
        sprintf(fmt, "%%10.10s: (%%3d, %%3d) --> %%2.%dlf\n",xvf->precision);
        switch (mri2->type)
        {
        case MRI_UCHAR:
          sprintf(buf, "%d", MRIseq_vox(mri2, x2, y2,z2,mri_frames[which2]));
          break ;
        case MRI_FLOAT:
          sprintf(buf,"%2.3f",MRIFseq_vox(mri2,x2,y2,z2,mri_frames[which2]));
          break ;
        }
        XVshowImageTitle(xvf, which2, "%s (%s)", title, buf) ;
      }
      
    }
  }
  return(NO_ERROR) ;
}
/*----------------------------------------------------------------------
            Parameters:

           Description:
              set the current path. This will be used to try and
              pass data back and forth between applications like
              surfer.
----------------------------------------------------------------------*/
int  
XVMRIsetImageName(XV_FRAME *xvf, char *image_name)
{
  FileNamePath(image_name, image_path) ;
  return(NO_ERROR) ;
}

