/**
 * @file  Interactor3D.h
 * @brief Base Interactor class for 3D render view.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: krish $
 *    $Date: 2011/03/12 00:28:48 $
 *    $Revision: 1.12 $
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

#ifndef Interactor3D_h
#define Interactor3D_h

#include "Interactor.h"

class Interactor3D : public Interactor
{
    Q_OBJECT
public:
  Interactor3D(QObject* parent = 0);
  virtual ~Interactor3D();

  // return true if to have parent Interactor3D continue processing the event
  // return false to stop event from further processing
  virtual bool ProcessMouseDownEvent ( QMouseEvent* event, RenderView* view );
  virtual bool ProcessMouseUpEvent( QMouseEvent* event, RenderView* view );
  virtual bool ProcessMouseMoveEvent( QMouseEvent* event, RenderView* view );
  virtual bool ProcessKeyDownEvent( QKeyEvent* event, RenderView* view );

protected:
  virtual void UpdateCursor( QEvent* event, QWidget* wnd );
  bool IsInAction()
  {
    return m_bWindowLevel || m_bMoveSlice;
  }
  
  int  m_nMousePosX;
  int  m_nMousePosY;

  bool m_bWindowLevel;
  bool m_bMoveSlice;
};

#endif


