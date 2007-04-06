/**
 * @file  vtkKWScubaLayer2DMRI.h
 * @brief A vtkKWScubaLayer that displayes 2DMRI slices
 *
 * Displayes MRI volumes (from a vtkFSVolumeSource) in a slice.
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

#ifndef vtkKWScubaLayer2DMRI_h
#define vtkKWScubaLayer2DMRI_h

#include <list>
#include "vtkKWScubaLayer.h"
#include "ScubaInfoItem.h"

//BTX
class ScubaCollectionPropertiesMRI;
//ETX
class vtkFSVolumeSource;
class vtkImageReslice;
class vtkImageMapToColors;
class vtkTransform;
class vtkTexture;
class vtkPolyDataMapper;
class vtkActor;

class vtkKWScubaLayer2DMRI : public vtkKWScubaLayer {

public:

  static vtkKWScubaLayer2DMRI* New ();
  vtkTypeRevisionMacro( vtkKWScubaLayer2DMRI, vtkKWScubaLayer );

  // Description:
  // Set the properties item. Done by the collection.
  //BTX
  void SetMRIProperties ( ScubaCollectionPropertiesMRI* const iProperties );
  //ETX

  // Description:
  // Listens for:
  // OpacityChanged - Changes the plane's property's opacity.
  // ColorMapChanged - Sets the lookup table in the color map object.
  // ResliceInterpolationChanged - Sets the texture interpolation mode.
  // TextureSmoothingChanged - Sets the texture interpolation feature.
  // Layer2DInfoChanged - Sets the plane transform's position and origin.
  //BTX
  virtual void DoListenToMessage ( std::string const isMessage,
				   void* const iData );
  //ETX

  // Description:
  // Make and set up our pipeline.
  void Create ();

  // Returns the bounds of the volume.
  virtual void GetRASBounds ( float ioBounds[6] ) const;

  // Description:
  // Returns the pixel size all dimensions.
  virtual void Get2DRASZIncrementHint ( float ioHint[3]) const;

  // Description:
  // Return our index and value info.
  //BTX
  void GetInfoItems ( float iRAS[3], std::list<ScubaInfoItem>& ilInfo ) const;
  //ETX

  // Description:
  // Get a pointer to the source for manipulation.
  vtkFSVolumeSource* GetSource () const;

protected:

  vtkKWScubaLayer2DMRI ();
  virtual ~vtkKWScubaLayer2DMRI ();

  // Description:
  // Populate a UI page with controls for this layer.
  virtual void AddControls ( vtkKWWidget* iPanel );
  virtual void RemoveControls ();

  // Description:
  // Update itself with various settings from the properties.
  void UpdateOpacity ();
  void UpdateColorMap ();
  void UpdateResliceInterpolation ();
  void UpdateTextureSmoothing ();
  void Update2DInfo ();

  //BTX
  ScubaCollectionPropertiesMRI const* mMRIProperties;

  // Pipeline ------------------------------------------------------------
  vtkImageReslice* mReslice;
  vtkImageMapToColors* mColorMap;
  vtkTransform* mPlaneTransform;
  vtkTexture* mTexture;
  vtkPolyDataMapper* mPlaneMapper;
  vtkActor* mPlaneActor;
  // ---------------------------------------------------------------------

  // Volume info.
  double mWorldCenter[3];
  double mWorldSize[3];

  //ETX

};

#endif
