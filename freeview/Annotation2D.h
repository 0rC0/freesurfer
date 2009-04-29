/**
 * @file  Annotation2D.h
 * @brief Annotation class for 2D view.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2009/04/29 22:53:48 $
 *    $Revision: 1.2.2.3 $
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

#ifndef Annotation2D_h
#define Annotation2D_h

#include "RenderView.h"
#include "vtkSmartPointer.h"

class vtkTextActor;
class vtkRenderer;
class vtkAxisActor2D;
class vtkPropCollection;

class Annotation2D
{
public:
  Annotation2D();
  virtual ~Annotation2D();

  void Update( vtkRenderer* renderer, int nPlane );

  void AppendAnnotations( vtkRenderer* renderer );

  void ShowScaleLine( bool bShow );

  bool GetShowScaleLine();

  void Show( bool bShow );
  bool IsVisible();

private:
  void UpdateScaleActors( double length, int nNumOfTicks, const char* title );

  vtkSmartPointer<vtkTextActor> m_actorCoordinates[6];
  vtkSmartPointer<vtkActor2D>  m_actorScaleLine;
  vtkSmartPointer<vtkTextActor> m_actorScaleTitle;
  vtkSmartPointer<vtkPropCollection> m_actorsAll;
};

#endif


