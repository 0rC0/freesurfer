/**
 * @file  Layer.h
 * @brief Base Layer class. A layer is an independent data object with 2D and 3D graphical representations.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2010/06/21 18:37:50 $
 *    $Revision: 1.14 $
 *
 * Copyright (C) 2008-2009,
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

#ifndef Layer_h
#define Layer_h

#include "Listener.h"
#include "Broadcaster.h"
#include "CommonDataStruct.h"
#include <string>
#include <vector>

class vtkRenderer;
class vtkProp;
class wxWindow;
class wxCommandEvent;
class LayerProperties;

class Layer : public Listener, public Broadcaster
{
public:
  Layer();
  virtual ~Layer();

  const char* GetName()
  {
    return m_strName.c_str();
  }

  void SetName( const char* name );

  virtual void Append2DProps( vtkRenderer* renderer, int nPlane ) = 0;
  virtual void Append3DProps( vtkRenderer* renderer, bool* bPlaneVisibility = NULL ) = 0;

  virtual bool HasProp( vtkProp* prop ) = 0;

  virtual void SetVisible( bool bVisible = true ) = 0;
  virtual bool IsVisible() = 0;

  virtual bool Rotate( std::vector<RotationElement>& rotations, wxWindow* wnd, wxCommandEvent& event )
  {
    return true;
  }

  double* GetWorldOrigin();
  void GetWorldOrigin( double* origin );
  void SetWorldOrigin( double* origin );

  double* GetWorldVoxelSize();
  void GetWorldVoxelSize( double* voxelsize );
  void SetWorldVoxelSize( double* voxelsize );

  double* GetWorldSize();
  void GetWorldSize( double* size );
  void SetWorldSize( double* size );

  double* GetSlicePosition();
  void GetSlicePosition( double* slicePos );
  void SetSlicePosition( double* slicePos );
  void SetSlicePosition( int nPlane, double slicePos );

  void RASToVoxel( const double* pos, int* n );
  void VoxelToRAS( const int* n, double* pos );

  virtual void OnSlicePositionChanged( int nPlane ) = 0;

  bool IsTypeOf( std::string tname );

  std::string GetErrorString()
  {
    return m_strError;
  }

  void SetErrorString( std::string msg )
  {
    m_strError = msg;
  }

  bool IsLocked()
  {
    return m_bLocked;
  }

  void Lock( bool bLock );

  std::string GetEndType();
  
  inline LayerProperties* GetProperties()
  {
    return mProperties;
  }
  
  virtual void GetBounds( double* bounds );
  
  virtual void GetDisplayBounds( double* bounds );

protected:
  virtual void DoListenToMessage( std::string const iMessage, void* iData, void* sender );
  std::string  m_strName;
  double    m_dSlicePosition[3];
  double    m_dWorldOrigin[3];
  double    m_dWorldVoxelSize[3];
  double    m_dWorldSize[3];

  bool   m_bLocked;

  LayerProperties*  mProperties;  
      
  std::string  m_strError;
  std::vector<std::string> m_strTypeNames;
};

#endif


