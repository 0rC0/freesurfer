/**
 * @file  Interactor2D.h
 * @brief Base Interactor class to manage mouse and key input on 2D render view.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: krish $
 *    $Date: 2011/03/12 00:28:48 $
 *    $Revision: 1.9 $
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

#ifndef Interactor2D_h
#define Interactor2D_h

#include "Interactor.h"

class QString;
class Layer;
class Interactor2D : public Interactor
{
  Q_OBJECT
public:
  Interactor2D( QObject* parent = NULL );
  virtual ~Interactor2D();

  // return true if to have parent Interactor2D continue processing the event
  // return false to stop event from further processing
  virtual bool ProcessMouseDownEvent( QMouseEvent* event, RenderView* view );
  virtual bool ProcessMouseUpEvent  ( QMouseEvent* event, RenderView* view );
  virtual bool ProcessMouseMoveEvent( QMouseEvent* event, RenderView* view );
  virtual bool ProcessKeyDownEvent  ( QKeyEvent* event, RenderView* view );

  virtual void ProcessPostMouseWheelEvent ( QWheelEvent* event, RenderView* view );
  virtual void ProcessPostMouseMoveEvent  ( QMouseEvent* event, RenderView* view );

signals:
  void Error( const QString& message, Layer* layer = NULL );

protected:
  int  m_nMousePosX;
  int  m_nMousePosY;

  bool m_bWindowLevel;
  bool m_bChangeSlice;
  bool m_bMovingCursor;
  bool m_bSelecting;
};

#endif


