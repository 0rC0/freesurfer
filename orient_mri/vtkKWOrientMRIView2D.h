/**
 * @file  vtkKWOrientMRIView2D.h
 * @brief Viewing pipeline for volume slice
 *
 * Pipeline objects for displaying a volume slice.
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: kteich $
 *    $Date: 2007/10/18 15:41:37 $
 *    $Revision: 1.3 $
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

#include <vector>

#ifndef vtkKWOrientMRIView2D_h
#define vtkKWOrientMRIView2D_h

#include "vtkKWRenderWidget.h"
#include "vtkSmartPointer.h"

class vtkActor;
class vtkArrowPipeline;
class vtkCellArray;
class vtkFSVolumeSource;
class vtkImageData;
class vtkImageMapToColors;
class vtkImagePlaneWidget;
class vtkImageReslice;
class vtkLookupTable;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkTexture;
class vtkTransform;
class vtkWindowLevelLookupTable;

class vtkKWOrientMRIView2D : public vtkKWRenderWidget {

public:

  static vtkKWOrientMRIView2D* New ();
  vtkTypeRevisionMacro( vtkKWOrientMRIView2D, vtkKWRenderWidget );

  // Description:
  // Create our axes actor.
  void Create ();

  // Description:
  // Set the fs volume, image and color function. Setting the volume
  // will cause it to be drawn in the view.
  void SetFSVolumeAndImage ( vtkFSVolumeSource& iVolume,vtkImageData& iImage );
  void SetImageColors ( vtkLookupTable& iColors );

  // Description:
  // Set the orientation for this 2D view.
  void SetOrientation ( int iOrientation );

  // Description:
  // Set the current through-plane coordinate to view.
  void SetThroughPlane ( float iZ );

  // Description: 
  // Called when the user modifies the ortho lines. The view redraws
  // the lines.
  void UpdateOrthoLine ( int iWhich );

  //BTX
  // Description:
  // Reset the ortho lines to 0,0,0.
  static void ResetOrthoPoints ();

  // Description:
  // Callback function for OrthoLineChanged events. Calls the view's
  // UpdateOrthoLines function.
  static void OrthoLineChanged ( vtkObject* iCaller, unsigned long iEventId,
				 void* iClientData, void* iCallData );

  // Description:
  // Return pointers to our reortho points.
  static void GetReorthoPoints ( double const*& oXStart, double const*& oXEnd,
				 double const*& oYStart, double const*& oYEnd,
				 double const*& oZStart, double const*& oZEnd);
  //ETX

protected:

  vtkKWOrientMRIView2D ();
  virtual ~vtkKWOrientMRIView2D ();

  //BTX
  // Return a color for an orientation.
  double const* GetColorForOrientation ( int inOrientation ) const;

  // Sets up the view with the current orientation and Z coord.
  void SetUpView ();
  
  // Our orientation.
  enum { UninitedPlaneOrientation = -1 };
  int mPlaneOrientation;

  // The orientation for which the camera is currently configured.
  int mCurrentCameraOrientation;
  
  // Our through plane.
  float mThroughPlane;

  // Set by the window; we don't own these.
  vtkSmartPointer<vtkFSVolumeSource> mFSVolume;
  vtkSmartPointer<vtkImageData> mImage;
  vtkSmartPointer<vtkLookupTable> mImageColors;

  // Our pipeline objects.
  vtkSmartPointer<vtkImageReslice> mReslice;
  vtkSmartPointer<vtkWindowLevelLookupTable> mColorTable;
  vtkSmartPointer<vtkImageMapToColors> mColorMap;
  vtkSmartPointer<vtkTexture> mTexture;
  vtkSmartPointer<vtkTransform> mPlaneTransform;
  vtkSmartPointer<vtkPolyDataMapper> mPlaneMapper;
  vtkSmartPointer<vtkActor> mPlaneActor;
  vtkSmartPointer<vtkActor> mOutlineActor;
  vtkSmartPointer<vtkArrowPipeline> mReorthoArrow[3];

  // Our points for the re-orthogonalize tool.
  static double sReorthoPoints[3][2][3]; // [orientation][start/end][x/y/z]

  // We keep a static list of our instances so we can notify them all
  // when one of them gets a OrthLineChanged event.
  static std::vector<vtkKWOrientMRIView2D*> slViews;

  //ETX
};

#endif
