/*----------------------------------------------------------------------
           File Name:  xvmri.h

           Author:

           Description:

           Conventions:

----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
                           INCLUDE FILES
----------------------------------------------------------------------*/

#ifndef XVMRI_H
#define XVMRI_H

#include "xvutil.h"
#include "mri.h"
#include "region.h"
#include "image.h"

void mri_event_handler(XV_FRAME *xvf, Event *event,
                       DIMAGE *dimage, int *px, int *py, int *pz) ;
void XVMRIdrawPoint(XV_FRAME *xvf, int which, int view, int depth, MRI *mri,
                    int x, int y, int z, int color) ;

void XVMRIdrawRegion(XV_FRAME *xvf, int which, int view, int depth, MRI *mri,
                MRI_REGION *reg, int color) ;
IMAGE *XVMRIshow(XV_FRAME *xvf, MRI *mri, int which, int slice) ;
int  XVMRIinit(XV_FRAME *xvf, int view_row, int view_col) ;
void XVMRIshowAll(XV_FRAME *xvf) ;
int  XVMRIfree(MRI **pmri, int which) ;
int  XVMRIsetView(XV_FRAME *xvf, int which, int view) ;



extern IMAGE *Idisplay[] ;
extern int   mri_views[] ;
extern int   mri_depths[] ;
extern MRI   *mris[] ;

#endif
