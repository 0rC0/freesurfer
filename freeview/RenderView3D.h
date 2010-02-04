/**
 * @file  RenderView3D.h
 * @brief View class for 2D image rendering.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2010/02/04 22:41:46 $
 *    $Revision: 1.17 $
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

#ifndef RenderView3D_h
#define RenderView3D_h

#include "RenderView.h"
#include "vtkSmartPointer.h"

class Cursor3D;
class vtkActor;

class VTK_RENDERING_EXPORT RenderView3D : public RenderView
{
  DECLARE_DYNAMIC_CLASS(RenderView3D)

public:
  RenderView3D();
  RenderView3D(wxWindow *parent, int id);
  virtual ~RenderView3D();

  static RenderView3D * New();
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void RefreshAllActors();

  void UpdateMouseRASPosition( int posX, int posY );
  void CancelUpdateMouseRASPosition();

  void UpdateCursorRASPosition( int posX, int posY );
  void UpdateConnectivityDisplay();

  void UpdateViewByWorldCoordinate();

  Cursor3D* GetCursor3D()
  {
    return m_cursor3D;
  }

  void ShowVolumeSlice( int nPlane, bool bShow = true );

  bool GetShowVolumeSlice( int nPlane );

  void UpdateScalarBar();
  
  void Azimuth( double angle );
  
  void SnapToNearestAxis();
  
  inline int GetHighlightedSlice()
  {
    return m_nSliceHighlighted;
  }
  
  void MoveSliceInScreenCoord( int x1, int y1, int x2, int y2 );
  
protected:
  void OnInternalIdle();
  void DoUpdateRASPosition( int posX, int posY, bool bCursor = false );
  void DoUpdateConnectivityDisplay();
  virtual void DoListenToMessage ( std::string const iMessage, void* iData, void* sender );

  void UpdateSliceFrames();
  void HighlightSliceFrame( int n );
  bool UpdateBounds();
  
  void PreScreenshot();
  void PostScreenshot();

private:
  void InitializeRenderView3D();

  int  m_nPickCoord[2];
  int  m_nCursorCoord[2];
  bool m_bToUpdateRASPosition;
  bool m_bToUpdateCursorPosition;
  bool m_bToUpdateConnectivity;

  Cursor3D* m_cursor3D;
  bool m_bSliceVisibility[3];
  vtkSmartPointer<vtkActor> m_actorSliceFrames[3];
  
  double  m_dBounds[6];
  int     m_nSliceHighlighted;

  // any class wishing to process wxWindows events must use this macro
  DECLARE_EVENT_TABLE()

};

#endif


