/**
 * @file  ScubaCollectionPropertiesMRI.h
 * @brief The common properties available to MRI layers
 *
 * An abstract interface implemented by a collection. Layers will get
 * a pointer to an object of this type so they can get access to
 * shared layer settings.
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: kteich $
 *    $Date: 2007/04/06 22:23:04 $
 *    $Revision: 1.1 $
 *
 * Copyright (C) 2002-2007,
 * The General Hospital Corporation (Boston, MA). 
 * All rights reserved.
 *
 * Distribution, usage and copying of this software is covered under the
 * terms found in the License Agreement file named 'COPYING' found in the
 * FreeSurfer source code root directory, and duplicated here:
 * https://surfer.nmr.mgh.harvard.edu/fswiki/FreeSurferOpenSourceLicense
 *
 * General inquiries: freesurfer@nmr.mgh.harvard.edu
 * Bug reports: analysis-bugs@nmr.mgh.harvard.edu
 *
 */

#ifndef ScubaCollectionPropertiesMRI_h
#define ScubaCollectionPropertiesMRI_h

extern "C" {
#include "colortab.h"
}

// Defined in X11/X.h
#ifdef GrayScale
#undef GrayScale
#endif

class vtkFSVolumeSource;
class vtkFreesurferLookupTable;
class vtkRGBATransferFunction;

class ScubaCollectionPropertiesMRI {

 public:

  // Description:
  // The color map types in which a volume can be drawn.
  enum ColorMapType { 
    NoColorMap=-1, 
    GrayScale, HeatScale, LUT, cColorMapTypes 
  }; 

  // Description:
  // Get a pointer to the source volume.
  virtual vtkFSVolumeSource* GetSource () const = 0;

  // Description:
  // Get a pointer to the various color table lookup tables. These are
  // managed by the collection and can be plugged into a layer's
  // pipeline.
  virtual vtkFreesurferLookupTable* GetLUTTable () const = 0;
  virtual vtkRGBATransferFunction* GetGrayScaleTable () const = 0;
  virtual vtkRGBATransferFunction* GetHeatScaleTable () const = 0;

  // Description:
  // Get the current color map type.
  virtual ColorMapType GetColorMap () const = 0;

  // Description:
  // Get the current reslice interpolation type: VTK_RESLICE_NEAREST,
  // VTK_RESLICE_LINEAR, or VTK_RESLICE_CUBIC.
  virtual int GetResliceInterpolation () const = 0;

  // Description:
  // Get the flag for texture smoothing.
  virtual int GetTextureSmoothing () const = 0;

  // Description:
  // Get a pointer to the FreeSurface color table being used.
  virtual COLOR_TABLE* GetLUTCTAB () const = 0;
};

#endif
