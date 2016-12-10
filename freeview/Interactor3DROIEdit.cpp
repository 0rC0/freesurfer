/**
 * @file  Interactor3DROIEdit.cpp
 * @brief Interactor for navigating in 3D render view.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: zkaufman $
 *    $Date: 2016/12/10 05:42:29 $
 *    $Revision: 1.1.2.2 $
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
 *
 */


#include "Interactor3DROIEdit.h"
#include "RenderView3D.h"
#include <QMouseEvent>
#include "LayerROI.h"
#include "LayerSurface.h"
#include "MainWindow.h"
#include "CursorFactory.h"

Interactor3DROIEdit::Interactor3DROIEdit(QObject* parent) :
  Interactor3D(parent)
{

}

bool Interactor3DROIEdit::ProcessMouseDownEvent( QMouseEvent* event, RenderView* renderview )
{
  RenderView3D* view = ( RenderView3D* )renderview;
  if (event->button() == Qt::LeftButton && !(event->modifiers() & CONTROL_MODIFIER))
  {
      LayerROI* roi = (LayerROI*)MainWindow::GetMainWindow()->GetActiveLayer("ROI");
      if (roi && roi->GetMappedSurface())
      {
         int nvo = view->PickCurrentSurfaceVertex(event->x(), event->y(), roi->GetMappedSurface());
         if (nvo >= 0)
         {
             roi->EditVertex(nvo, !(event->modifiers() & Qt::ShiftModifier));
         }
      }
  }

  return Interactor3D::ProcessMouseDownEvent( event, renderview );
}

bool Interactor3DROIEdit::ProcessMouseMoveEvent( QMouseEvent* event, RenderView* view )
{
  bool ret = Interactor3D::ProcessMouseMoveEvent(event, view);
  view->setCursor( CursorFactory::CursorPencil );
  return ret;
}

bool Interactor3DROIEdit::ProcessMouseUpEvent( QMouseEvent* event, RenderView* view )
{
    bool ret = Interactor3D::ProcessMouseUpEvent(event, view);
    view->setCursor( CursorFactory::CursorPencil );
    return ret;
}
