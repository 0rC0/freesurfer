/**
 * @file  VolumeCropper.cpp
 * @brief Class to crop volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2010/06/21 18:37:50 $
 *    $Revision: 1.1 $
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

#include "VolumeCropper.h"
#include "LayerMRI.h"
#include "RenderView.h"
#include <vtkBox.h>
#include <vtkCubeSource.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkClipPolyData.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
#include <vtkSphereSource.h>
#include <vtkImageActor.h>
#include <vtkMath.h>

VolumeCropper::VolumeCropper() : 
    Broadcaster( "VolumeCropper" ), 
    Listener( "VolumeCropper" ),
    m_mri( NULL ),
    m_bEnabled( false ),
    m_nSelectedSphere( -1 )
{
  m_actorBox = vtkSmartPointer<vtkActor>::New();
  m_actorBox->VisibilityOff();
  m_actorBox->GetProperty()->SetOpacity( 0.3 );
  m_actorBox->GetProperty()->SetColor( 1, 1, 0 );

  m_actorFrame = vtkSmartPointer<vtkActor>::New();
  m_actorFrame->VisibilityOff();
  m_actorFrame->GetProperty()->SetColor( 1, 1, 0 );
  m_actorFrame->GetProperty()->SetLineWidth( 2 );    
  m_actorFrame->GetProperty()->SetRepresentationToWireframe();
  m_actorFrame->GetProperty()->SetDiffuse( 0.0 );
  m_actorFrame->GetProperty()->SetAmbient( 1.0 );
  
  for ( int i = 0; i < 6; i++ )
  {
    m_actorSphere[i] = vtkSmartPointer<vtkActor>::New();
    m_actorSphere[i]->VisibilityOff();
    m_sphereSource[i] = vtkSmartPointer<vtkSphereSource>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInput( m_sphereSource[i]->GetOutput() );
    m_actorSphere[i]->SetMapper( mapper );
    m_actorSphere[i]->GetProperty()->SetColor( 1, 1, 0 );
  }
  
  m_actorBox2D = vtkSmartPointer<vtkActor>::New();
  m_actorBox2D->VisibilityOff();
  m_actorBox2D->SetProperty( m_actorBox->GetProperty() );
  
  m_actorFrame2D = vtkSmartPointer<vtkActor>::New();
  m_actorFrame2D->VisibilityOff();
  m_actorFrame2D->SetProperty( m_actorFrame->GetProperty() );
  
  m_boxSource = vtkSmartPointer<vtkCubeSource>::New();
  vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput( m_boxSource->GetOutput() );
  m_actorBox->SetMapper( mapper );
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput( m_boxSource->GetOutput() );
  m_actorFrame->SetMapper( mapper );
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput( m_boxSource->GetOutput() );
  m_actorBox2D->SetMapper( mapper );
  mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInput( m_boxSource->GetOutput() );
  m_actorFrame2D->SetMapper( mapper );
  m_box = vtkSmartPointer<vtkBox>::New();
  
  m_clipper = vtkSmartPointer<vtkClipPolyData>::New();
  m_clipper->SetClipFunction( m_box );
  m_clipper->InsideOutOn();
}

VolumeCropper::~VolumeCropper()
{}

vtkActor* VolumeCropper::GetProp()
{
  return m_actorBox;
}

void VolumeCropper::Append3DProps( vtkRenderer* renderer )
{
  renderer->AddViewProp( m_actorBox );
  renderer->AddViewProp( m_actorFrame );
  for ( int i = 0; i < 6; i++ )
    renderer->AddViewProp( m_actorSphere[i] );
}

void VolumeCropper::Append2DProps( vtkRenderer* renderer )
{
  renderer->AddViewProp( m_actorBox2D );  
  renderer->AddViewProp( m_actorFrame2D ); 
}

void VolumeCropper::SetVolume( LayerMRI* mri)
{
  if ( m_mri != mri )
  {
    m_mri = mri;
    Reset();
  }
}

void VolumeCropper::Reset()
{
  m_mri->GetDisplayBounds( m_bounds );
  m_boxSource->SetBounds( m_bounds );
  double dMax = 0;
  for ( int i = 0; i < 6; i+=2 )
  {
    if ( dMax < (m_bounds[i+1] - m_bounds[i]) )
      dMax = m_bounds[i+1] - m_bounds[i];
  }
  for ( int i = 0; i < 6; i++ )
    m_sphereSource[i]->SetRadius( dMax/75.0 );
    
  UpdateExtent();
  m_box->SetBounds( m_bounds );
  UpdateProps();
}

// apply contour clipper
void VolumeCropper::Apply()
{
  m_box->SetBounds( m_bounds );
}

void VolumeCropper::UpdateProps()
{
  double c[3];
  for ( int i = 0; i < 3; i++ )
    c[i] = ( m_bounds[i*2] + m_bounds[i*2+1] ) / 2;
  
  m_sphereSource[0]->SetCenter( m_bounds[0], c[1], c[2] );
  m_sphereSource[1]->SetCenter( m_bounds[1], c[1], c[2] );
  m_sphereSource[2]->SetCenter( c[0], m_bounds[2], c[2] );
  m_sphereSource[3]->SetCenter( c[0], m_bounds[3], c[2] );
  m_sphereSource[4]->SetCenter( c[0], c[1], m_bounds[4] );
  m_sphereSource[5]->SetCenter( c[0], c[1], m_bounds[5] );
  
  m_boxSource->SetBounds( m_bounds );
  
  int* ext = m_extent;
  int* dim = m_mri->GetImageData()->GetDimensions();
  m_mri->m_sliceActor2D[0]->SetDisplayExtent( dim[1]-ext[3]-1, dim[1]-ext[2]-1, ext[4], ext[5], 0, 0 );
  m_mri->m_sliceActor2D[1]->SetDisplayExtent( ext[0], ext[1], ext[4], ext[5], 0, 0 );
  m_mri->m_sliceActor2D[2]->SetDisplayExtent( ext[0], ext[1], ext[2], ext[3], 0, 0 );
  for ( int i = 0; i < 3; i++ )
    m_mri->m_sliceActor3D[i]->SetDisplayExtent( m_mri->m_sliceActor2D[i]->GetDisplayExtent() );
  
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast( m_mri->m_actorContour->GetMapper() );
  if ( mapper && mapper->GetInput() && m_clipper->GetOutput() != mapper->GetInput() )
  {
    m_clipper->SetInput( mapper->GetInput() );
    mapper->SetInput( m_clipper->GetOutput() );
  }
  
  this->SendBroadcast( "CropBoundChanged", m_mri );
}

void VolumeCropper::UpdateExtent()
{
  vtkImageData* image = m_mri->GetImageData();
  double* orig = image->GetOrigin();
  double* voxelsize = image->GetSpacing();
  for ( int i = 0; i < 6; i++ )
    m_extent[i] = (int)( (m_bounds[i] - orig[i/2])/voxelsize[i/2] + 0.5 );  
  
  m_mri->SetCroppingBounds( m_bounds );
}

void VolumeCropper::SetEnabled( bool bEnable )
{
  m_bEnabled = bEnable;
  if ( !bEnable )
    Show( false );
}

void VolumeCropper::Show( bool bShow )
{
  m_actorBox->SetVisibility( bShow );
  m_actorBox2D->SetVisibility( bShow );
  m_actorFrame->SetVisibility( bShow );
  m_actorFrame2D->SetVisibility( bShow );
  for ( int i = 0; i < 6; i++ )
    m_actorSphere[i]->SetVisibility( bShow );
}

bool VolumeCropper::IsShown()
{
  return m_actorBox->GetVisibility();
}

bool VolumeCropper::PickActiveBound( vtkProp* prop )
{
  m_nSelectedSphere = -1;
  for ( int i = 0; i < 6; i++ )
  {
    if ( m_actorSphere[i].GetPointer() == prop )
    {
      m_nSelectedSphere = i;
      m_actorSphere[i]->GetProperty()->SetColor( 1, 0, 0 );
      return true;
    }
  }
  return false;
}

void VolumeCropper::ReleaseActiveBound()
{
  if ( m_nSelectedSphere >= 0 )
    m_actorSphere[m_nSelectedSphere]->GetProperty()->SetColor( 1, 1, 0 );
  
  m_nSelectedSphere = -1;  
  // update clipper box
  m_box->SetBounds( m_bounds );
}

void VolumeCropper::MoveActiveBound( RenderView* view, int nx, int ny )
{
  if ( !m_mri )
    return;
  
  double dMaxBounds[6];
  m_mri->GetDisplayBounds( dMaxBounds );
  int x1, y1, x2, y2;
  double pt1[3], pt2[3];
  for ( int i = 0; i < 3; i++ )
  {
    pt1[i] = ( dMaxBounds[i*2] + dMaxBounds[i*2+1] ) / 2;
    pt2[i] = pt1[i];
  }
  int n = m_nSelectedSphere/2;
  pt1[n] = dMaxBounds[m_nSelectedSphere];
  if ( m_nSelectedSphere%2 == 0 )
    pt2[n] = dMaxBounds[m_nSelectedSphere+1];
  else
    pt2[n] = dMaxBounds[m_nSelectedSphere-1]; 
  view->WorldToScreen( pt1[0], pt1[1], pt1[2], x1, y1 );
  view->WorldToScreen( pt2[0], pt2[1], pt2[2], x2, y2 );
  double ratio = 0;
  if ( fabs( x1-x2 ) > fabs( y1-y2 ) )
    ratio = (x2==x1) ? 0 : ( nx / sqrt( (x2-x1)*(x2-x1) ) * (x2>x1?1:-1) );
  else
    ratio = (y2==y1) ? 0 : ( ny / sqrt( (y2-y1)*(y2-y1) ) * (y2>y1?1:-1) );

  m_bounds[m_nSelectedSphere] += ratio * (pt2[n] - pt1[n]);
  
  if ( m_bounds[m_nSelectedSphere] < dMaxBounds[n*2] )
    m_bounds[m_nSelectedSphere] = dMaxBounds[n*2];
  else if ( m_bounds[m_nSelectedSphere] > dMaxBounds[n*2+1] )
    m_bounds[m_nSelectedSphere] = dMaxBounds[n*2+1];
  
  double* voxelsize = m_mri->GetImageData()->GetSpacing();
  if ( m_nSelectedSphere%2 == 0 && m_bounds[m_nSelectedSphere] >= m_bounds[m_nSelectedSphere+1] )
    m_bounds[m_nSelectedSphere] = m_bounds[m_nSelectedSphere+1] - voxelsize[n]/2;
  else if ( m_nSelectedSphere%2 == 1 && m_bounds[m_nSelectedSphere] < m_bounds[m_nSelectedSphere-1] )
    m_bounds[m_nSelectedSphere] = m_bounds[m_nSelectedSphere-1] + voxelsize[n]/2;
  
  UpdateExtent();
  UpdateProps();
}

void VolumeCropper::SetExtent( int nComp, int nValue )
{
  m_extent[nComp] = nValue;
  if ( nComp%2 == 0 && m_extent[nComp+1] < nValue )
    m_extent[nComp] = m_extent[nComp+1];
  else if ( nComp%2 == 1 && m_extent[nComp-1] > nValue )
    m_extent[nComp] = m_extent[nComp-1];
  
  vtkImageData* image = m_mri->GetImageData();
  double* orig = image->GetOrigin();
  double* voxelsize = image->GetSpacing();
  m_bounds[nComp] = m_extent[nComp]*voxelsize[nComp/2] - orig[nComp/2];
  
  m_mri->SetCroppingBounds( m_bounds );
  UpdateProps();
}


