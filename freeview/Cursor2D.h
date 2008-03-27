/**
 * @file  Cursor2D.h
 * @brief Annotation class for 2D view.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2008/03/27 18:12:14 $
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
 
#ifndef Cursor2D_h
#define Cursor2D_h

#include "RenderView.h"
#include "vtkSmartPointer.h"

class vtkCursor3D;
class vtkRenderer;
class vtkActor;
class RenderView2D;

class Cursor2D
{
public:
	Cursor2D( RenderView2D* view );
    virtual ~Cursor2D();	

	void SetPosition( double* pos, bool bConnectPrevious = false );	
	void SetPosition2( double* pos);
	
	double* GetPosition();
	void GetPosition( double* pos );
	
	void GetColor( double* rgb );
	void SetColor( double r, double g, double b );
	
	int GetRadius();
	void SetRadius( int nRadius );
	
	void Update( bool bConnectPrevious = false );  
	
	void AppendCursor( vtkRenderer* renderer );
	
private:
	vtkSmartPointer<vtkActor>	m_actorCursor;
	
	RenderView2D*	m_view;
	
	double		m_dPosition[3];
	double		m_dPosition2[3];
	
	int			m_nRadius;
};

#endif 


