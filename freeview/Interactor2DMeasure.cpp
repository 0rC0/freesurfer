/**
 * @file  Interactor2DMeasure.cpp
 * @brief Interactor for measure tool in 2D render view.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2009/11/03 22:51:28 $
 *    $Revision: 1.2 $
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

#include "Interactor2DMeasure.h"
#include "RenderView2D.h"
#include "Cursor2D.h"
#include "MainWindow.h"
#include "LayerCollection.h"
#include "LayerCollectionManager.h"
#include "LayerVolumeBase.h"
#include "LayerMRI.h"
#include "CursorFactory.h"
#include "Region2DLine.h"
#include "Region2DRectangle.h"
#include <vtkRenderer.h>

#define FV_CURSOR_PENCIL wxCursor( wxImage("res/images/cursor_pencil.png") )
#define FV_CURSOR_FILL  wxCursor( wxImage("res/images/cursor_fill.png" ) )

Interactor2DMeasure::Interactor2DMeasure() :
    Interactor2D(),
    m_bEditing( false ),
    m_bDrawing( false ),
    m_nPointIndex( -1 ),
    m_region( NULL )
{
}

Interactor2DMeasure::~Interactor2DMeasure()
{}

bool Interactor2DMeasure::ProcessMouseDownEvent( wxMouseEvent& event, RenderView* renderview )
{
  RenderView2D* view = ( RenderView2D* )renderview;
// UpdateCursor( event, view );

  if ( m_region )
    m_region->Highlight( false );
  
  if ( event.LeftDown() )
  {
    if ( event.ControlDown() && event.ShiftDown() )
      return Interactor2D::ProcessMouseDownEvent( event, renderview );

    LayerCollection* lc = MainWindow::GetMainWindowPointer()->GetLayerCollectionManager()->GetLayerCollection( "MRI" );
    LayerVolumeBase* mri = ( LayerVolumeBase* )lc->GetActiveLayer();
    if ( mri )
    {
      m_nMousePosX = event.GetX();
      m_nMousePosY = event.GetY();
      
      Region2D* reg = view->GetRegion( m_nMousePosX, m_nMousePosY, &m_nPointIndex );
      if ( !reg )   // drawing
      {
        if ( m_nAction == MM_Line )
        {
          Region2DLine* reg_line = new Region2DLine( view );
          reg_line->SetLine( m_nMousePosX, m_nMousePosY, m_nMousePosX, m_nMousePosY ); 
          view->AddRegion( reg_line );
          m_region = reg_line;
        }
        else if ( m_nAction == MM_Rectangle )
        {
          Region2DRectangle* reg_rect = new Region2DRectangle( view );
          reg_rect->SetRect( m_nMousePosX, m_nMousePosY, 1, 1 );
          view->AddRegion( reg_rect );
          m_region = reg_rect;
        }
        m_bDrawing = true;
      }
      else      // editing
      {
        m_region = reg;
        m_bEditing = true;
        m_region->Highlight();
        view->NeedRedraw();
      }
      return false;
    }
  }
  
  return Interactor2D::ProcessMouseDownEvent( event, renderview ); // pass down the event
}

bool Interactor2DMeasure::ProcessMouseUpEvent( wxMouseEvent& event, RenderView* renderview )
{
  RenderView2D* view = ( RenderView2D* )renderview;
  UpdateCursor( event, renderview );

  if ( m_bDrawing )
  {
    if ( m_nMousePosX != event.GetX() || m_nMousePosY != event.GetY() )
    {
      m_nMousePosX = event.GetX();
      m_nMousePosY = event.GetY();
  
  //    if ( event.LeftUp() )
  
 //     LayerCollection* lc = MainWindow::GetMainWindowPointer()->GetLayerCollection( "MRI" );
 //     LayerVolumeBase* mri = ( LayerVolumeBase* )lc->GetActiveLayer();
 //     mri->SendBroadcast( "LayerEdited", mri );
      if ( m_region )
      {
        if ( m_nAction == MM_Line )
        {
          ((Region2DLine*)m_region)->SetPoint2( m_nMousePosX, m_nMousePosY );
        }
        else if ( m_nAction == MM_Rectangle )
        {
          ((Region2DRectangle*)m_region)->SetBottomRight( m_nMousePosX, m_nMousePosY );
        }
      }
    }
    else
    {
      if ( m_region )
      {
        view->DeleteRegion( m_region );
        m_region = NULL;
      }
    } 
    
    m_bEditing = false;
    m_bDrawing = false;
    return false;
  }
  else if ( m_bEditing )
  {
    m_bEditing = false;
    m_bDrawing = false;
    return false;
  }
  else
  {
    return Interactor2D::ProcessMouseUpEvent( event, renderview );
  }
}

bool Interactor2DMeasure::ProcessMouseMoveEvent( wxMouseEvent& event, RenderView* renderview )
{
  RenderView2D* view = ( RenderView2D* )renderview;

  if ( m_bDrawing )
  {
    UpdateCursor( event, view );
    int posX = event.GetX();
    int posY = event.GetY();

//    LayerCollection* lc = MainWindow::GetMainWindowPointer()->GetLayerCollection( m_strLayerTypeName.c_str() );
//    LayerVolumeBase* mri = ( LayerVolumeBase* )lc->GetActiveLayer();

    if ( m_region )
    {
      if ( m_nAction == MM_Line )
      {
        ((Region2DLine*)m_region)->SetPoint2( posX, posY );
      }
      else if ( m_nAction == MM_Rectangle )
      {
        ((Region2DRectangle*)m_region)->SetBottomRight( posX, posY );
      }
      view->NeedRedraw();
    }

    return false;
  }
  else if ( m_bEditing )
  {    
    UpdateCursor( event, view );
    int offsetX = event.GetX() - m_nMousePosX;
    int offsetY = event.GetY() - m_nMousePosY;
    if ( m_region )
    {
      m_nMousePosX = event.GetX();
      m_nMousePosY = event.GetY();
      if ( m_nPointIndex >= 0 )
        m_region->UpdatePoint( m_nPointIndex, m_nMousePosX, m_nMousePosY );
      else
        m_region->Offset( offsetX, offsetY );
      view->NeedRedraw();
    }
    
    return false;
  }
  else
  {
    return Interactor2D::ProcessMouseMoveEvent( event, renderview );
  }
}

bool Interactor2DMeasure::ProcessKeyDownEvent( wxKeyEvent& event, RenderView* renderview )
{
  RenderView2D* view = ( RenderView2D* )renderview;
  UpdateCursor( event, renderview );

  if ( m_region && ( event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_NUMPAD_DELETE ) )
  {
    view->DeleteRegion( m_region );
    m_region = NULL;
    return false;
  }
  else
  {
    return Interactor2D::ProcessKeyDownEvent( event, renderview );
  }
}

bool Interactor2DMeasure::ProcessKeyUpEvent( wxKeyEvent& event, RenderView* renderview )
{
  UpdateCursor( event, renderview );

  return Interactor2D::ProcessKeyUpEvent( event, renderview );
}

void Interactor2DMeasure::UpdateCursor( wxEvent& event, wxWindow* wnd )
{
  RenderView2D* view = ( RenderView2D* )wnd;
  if ( wnd->FindFocus() == wnd )
  {
    if ( event.IsKindOf( CLASSINFO( wxMouseEvent ) ) )
    {
      wxMouseEvent* e = ( wxMouseEvent* )&event;
      if ( ( ( e->MiddleDown() || e->RightDown() ) && !m_bEditing ) ||
           ( e->ControlDown() && e->ShiftDown() ) )
      {
        Interactor2D::UpdateCursor( event, wnd );
        return;
      }
      else if ( ( !m_bEditing && !m_bDrawing && view->GetRegion( e->GetX(), e->GetY() ) ) ||
                ( m_bEditing && m_nPointIndex < 0 ) )
      {
        wnd->SetCursor( CursorFactory::CursorGrab );
      }
      else
      {
        wnd->SetCursor( CursorFactory::CursorMeasure );
      }
    }   
  }
  else
    Interactor2D::UpdateCursor( event, wnd );
}
