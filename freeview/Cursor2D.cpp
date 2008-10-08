/**
 * @file  Cursor2D.cpp
 * @brief View for rendering.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2008/10/08 19:14:34 $
 *    $Revision: 1.7 $
 *
 * Copyright (C) 2002-2008,
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
 
#include <wx/xrc/xmlres.h> 
#include "Cursor2D.h"
#include "vtkRenderer.h"
#include "vtkActor2D.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyData.h"
#include "MainWindow.h"
#include "RenderView2D.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include <vtkProperty2D.h>
#include "MyUtils.h"
#include "LayerMRI.h"
#include "LayerPropertiesMRI.h"


Cursor2D::Cursor2D( RenderView2D* view ) :
	m_view( view ),
	m_nRadius( 5 )
{
	m_actorCursor = vtkSmartPointer<vtkActor2D>::New();
	m_actorCursor->GetProperty()->SetColor( 1, 0, 0 );

	Update();
}

Cursor2D::~Cursor2D()
{
}

void Cursor2D::SetPosition2( double* pos)
{
	for ( int i = 0; i < 3; i++ )
	{
		m_dPosition2[i] = pos[i];
	}
}

void Cursor2D::SetPosition( double* pos, bool bConnectPrevious  )
{
	for ( int i = 0; i < 3; i++ )
	{
		m_dPosition[i] = pos[i];
	}

	Update( bConnectPrevious );
}

void Cursor2D::Update( bool bConnectPrevious )
{
	vtkRenderer* renderer = m_view->GetRenderer();

	double dLen = m_nRadius;
	double pos[3] = { m_dPosition[0], m_dPosition[1], m_dPosition[2] };
	double prev_pos[3] = { m_dPosition2[0], m_dPosition2[1], m_dPosition2[2] };	
	renderer->WorldToView( prev_pos[0], prev_pos[1], prev_pos[2] );
	renderer->WorldToView( pos[0], pos[1], pos[2] );
	renderer->ViewToNormalizedViewport( prev_pos[0], prev_pos[1], prev_pos[2] );
	renderer->ViewToNormalizedViewport( pos[0], pos[1], pos[2] );
	renderer->NormalizedViewportToViewport( prev_pos[0], prev_pos[1] );
	renderer->NormalizedViewportToViewport( pos[0], pos[1] );

	int n = 0;
	vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
	if ( bConnectPrevious )
	{
		points->InsertNextPoint( prev_pos );
		points->InsertNextPoint( pos );
		lines->InsertNextCell( 2 );
		lines->InsertCellPoint( n++ );
		lines->InsertCellPoint( n++ );
	}
	points->InsertNextPoint( pos[0] + dLen, pos[1], pos[2] );
	points->InsertNextPoint( pos[0] - dLen, pos[1], pos[2] );
	lines->InsertNextCell( 2 );
	lines->InsertCellPoint( n++ );	
	lines->InsertCellPoint( n++ );
	points->InsertNextPoint( pos[0], pos[1] + dLen, pos[2] );
	points->InsertNextPoint( pos[0], pos[1] - dLen, pos[2] );
	lines->InsertNextCell( 2 );
	lines->InsertCellPoint( n++ );	
	lines->InsertCellPoint( n++ );
	
	vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->SetPoints( points );
	polydata->SetLines( lines );
	vtkSmartPointer<vtkPolyDataMapper2D> mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
	mapper->SetInput( polydata );
	vtkSmartPointer<vtkCoordinate> coords = vtkSmartPointer<vtkCoordinate>::New();
	coords->SetCoordinateSystemToViewport();
	mapper->SetTransformCoordinate( coords );

	m_actorCursor->SetMapper( mapper );	
}

void Cursor2D::AppendActor( vtkRenderer* renderer )
{
	renderer->AddViewProp( m_actorCursor );
}

void Cursor2D::SetColor( double r, double g, double b )
{
	m_actorCursor->GetProperty()->SetColor( r, g, b );
}

void Cursor2D::SetColor( const wxColour& color )
{
	m_actorCursor->GetProperty()->SetColor( color.Red()/255.0, color.Green()/255.0, color.Blue()/255.0 );
}

void Cursor2D::GetColor( double* rgb )
{
	m_actorCursor->GetProperty()->GetColor( rgb );
}

wxColour Cursor2D::GetColor()
{
	double* rgb = m_actorCursor->GetProperty()->GetColor();
	return wxColour( (int)(rgb[0]*255), (int)(rgb[1]*255), (int)(rgb[2]*255) );
}

int Cursor2D::GetRadius()
{
	return m_nRadius;
}

void Cursor2D::SetRadius( int nRadius )
{
	if ( nRadius > 1 )
	{
		m_nRadius = nRadius;
		Update();
	}
}

double* Cursor2D::GetPosition()
{
	return m_dPosition;
}

void Cursor2D::GetPosition( double* pos )
{
	for ( int i = 0; i < 3; i++ )
		pos[i] = m_dPosition[i];
}

void Cursor2D::Show( bool bShow )
{
	m_actorCursor->SetVisibility( bShow?1:0 );
}
	
bool Cursor2D::IsShown()
{
	return m_actorCursor->GetVisibility() > 0;
}
