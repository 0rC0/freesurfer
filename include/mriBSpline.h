/**
 * @file  mrispline.h
 * @brief Compute B-Spline Coefficients and Interpolate MRI
 *
 * This Code is based on:
 *		P. Thevenaz, T. Blu, M. Unser, "Interpolation Revisited,"
 *		IEEE Transactions on Medical Imaging,
 *		vol. 19, no. 7, pp. 739-758, July 2000.
 *
 * and code (for 2D images) obtained from Philippe Thevenaz at
 *   http://bigwww.epfl.ch/thevenaz/interpolation/
 *
 */
/*
 * Original Author: Martin Reuter
 * CVS Revision Info:
 *    $Author: mreuter $
 *    $Date: 2011/09/29 00:17:12 $
 *    $Revision: 1.2 $
 *
 * Copyright © 2011 The General Hospital Corporation (Boston, MA) "MGH"
 *
 * Terms and conditions for use, reproduction, distribution and contribution
 * are found in the 'FreeSurfer Software License Agreement' contained
 * in the file 'LICENSE' found in the FreeSurfer distribution, and here:
 *
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferSoftwareLicense
 *
 * Reporting: freesurfer@nmr.mgh.harvard.edu
 *
 */

#ifndef MRIBSPLINE_H
#define MRIBSPLINE_H

#if defined(__cplusplus)
extern "C" {
#endif

#include "mri.h"

typedef struct
{
   int degree;
   MRI * coeff;
   int srctype;
}
MRI_BSPLINE;

// allocate the bspline (mainly coefficients)
// usually not called from outside 
// has to be same size as the MRI for which this will be computed
MRI_BSPLINE* MRIallocBSpline(int width, int height, int depth, int nframes);

// guess:
int MRIfreeBSpline(MRI_BSPLINE **pbspline) ;

// Step 1: Compute B-spline coefficients from image 
//  bspline needs to be same dim as mri_src or NULL
//  mri_src   needs to be isotrophic
MRI_BSPLINE* MRItoBSpline (const MRI	*mri_src,	MRI_BSPLINE *bspline, int degree);

// Step 2: Based on pre-computed B-spline coefficients, interpolate image
int MRIsampleBSpline(const MRI_BSPLINE * bspline, double x, double y, double z, int frame, double *pval);

// direct methods for up and downsample (based on simplified algo):
MRI *MRIdownsample2BSpline(const MRI* mri_src, MRI *mri_dst) ;
MRI *MRIupsample2BSpline(const MRI* mri_src, MRI *mri_dst) ;


#if defined(__cplusplus)
};
#endif

#endif
