/**
 * @file  LayerMRI.cpp
 * @brief Layer data object for MRI volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2009/04/16 21:25:51 $
 *    $Revision: 1.21 $
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

#include <wx/wx.h>
#include "LayerMRI.h"
#include "vtkRenderer.h"
#include "vtkImageActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkSmartPointer.h"
#include "vtkMatrix4x4.h"
#include "vtkImageMapToColors.h"
#include "vtkImageActor.h"
#include "vtkActor.h"
#include "vtkRGBATransferFunction.h"
#include "vtkLookupTable.h"
#include "vtkProperty.h"
#include "vtkImageReslice.h"
#include "vtkFreesurferLookupTable.h"
#include "vtkCubeSource.h"
#include "vtkVolume.h"
#include "vtkImageThreshold.h"
#include "vtkPointData.h"
#include "vtkCellArray.h"
#include "vtkContourFilter.h"
#include "vtkTubeFilter.h"
#include "LayerPropertiesMRI.h"
#include "MyUtils.h"
#include "FSVolume.h"
#include "MainWindow.h"
#include "vtkMath.h"

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

LayerMRI::LayerMRI( LayerMRI* ref ) : LayerVolumeBase(),
    m_volumeSource( NULL),
    m_volumeRef( ref ? ref->GetSourceVolume() : NULL ),
    m_bResampleToRAS( true )
{
  m_strTypeNames.push_back( "MRI" );

  for ( int i = 0; i < 3; i++ )
  {
    // m_nSliceNumber[i] = 0;
    m_sliceActor2D[i] = vtkImageActor::New();
    m_sliceActor3D[i] = vtkImageActor::New();
    /* m_sliceActor2D[i]->GetProperty()->SetAmbient( 1 );
     m_sliceActor2D[i]->GetProperty()->SetDiffuse( 0 );
     m_sliceActor3D[i]->GetProperty()->SetAmbient( 1 );
     m_sliceActor3D[i]->GetProperty()->SetDiffuse( 0 );*/
    m_sliceActor2D[i]->InterpolateOff();
    m_sliceActor3D[i]->InterpolateOff();
    
    m_vectorActor2D[i] = vtkActor::New();
    m_vectorActor3D[i] = vtkActor::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    vtkSmartPointer<vtkPolyDataMapper> mapper2 = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_vectorActor2D[i]->SetMapper( mapper );
    m_vectorActor3D[i]->SetMapper( mapper2 );
    m_vectorActor2D[i]->GetProperty()->SetInterpolationToFlat();
    m_vectorActor3D[i]->GetProperty()->SetInterpolationToFlat();
  }
  mProperties = new LayerPropertiesMRI();
  mProperties->AddListener( this );

  m_actorContour = vtkActor::New();
  m_propVolume = vtkVolume::New();
}

LayerMRI::~LayerMRI()
{	
	for ( int i = 0; i < 3; i++ )
	{
		m_sliceActor2D[i]->Delete();
		m_sliceActor3D[i]->Delete();
    m_vectorActor2D[i]->Delete();
    m_vectorActor3D[i]->Delete();
	}
	m_actorContour->Delete();
	m_propVolume->Delete();
	
	delete mProperties;
	
	if ( m_volumeSource )
		delete m_volumeSource;
    
  for ( size_t i = 0; i < m_segActors.size(); i++ )
  {
    m_segActors[i].actor->Delete();
  }
}

/*
bool LayerMRI::LoadVolumeFromFile( std::string filename )
{
 m_sFilename = filename;

 return LoadVolumeFromFile();
}
*/

void LayerMRI::SetResampleToRAS( bool bResample )
{
  m_bResampleToRAS = bResample;
}

bool LayerMRI::LoadVolumeFromFile( wxWindow* wnd, wxCommandEvent& event )
{
  if ( m_volumeSource )
    delete m_volumeSource;

  m_volumeSource = new FSVolume( m_volumeRef );
  m_volumeSource->SetResampleToRAS( m_bResampleToRAS );

  if ( !m_volumeSource->MRIRead(  m_sFilename.c_str(),
                                  m_sRegFilename.size() > 0 ? m_sRegFilename.c_str() : NULL,
                                  wnd,
                                  event ) )
    return false;

  event.SetInt( 100 );
  wxPostEvent( wnd, event );

  InitializeVolume();
  InitializeActors();

  mProperties->SetVolumeSource( m_volumeSource );

  return true;
}

bool LayerMRI::Create( LayerMRI* mri, bool bCopyVoxelData )
{
  if ( m_volumeSource )
    delete m_volumeSource;

  m_volumeSource = new FSVolume( mri->m_volumeSource );
  m_volumeSource->Create( mri->m_volumeSource, bCopyVoxelData );

  m_bResampleToRAS = mri->m_bResampleToRAS;
  m_imageDataRef = mri->GetImageData();
// if ( m_imageDataRef.GetPointer() )
  if ( m_imageDataRef != NULL )
  {
    SetWorldOrigin( mri->GetWorldOrigin() );
    SetWorldVoxelSize( mri->GetWorldVoxelSize() );
    SetWorldSize( mri->GetWorldSize() );

    /* m_imageData = vtkSmartPointer<vtkImageData>::New();

     if ( bCopyVoxelData )
     {
      m_imageData->DeepCopy( m_imageDataRef );
     }
     else
     {
      m_imageData->SetNumberOfScalarComponents( 1 );
      m_imageData->SetScalarTypeToFloat();
      m_imageData->SetOrigin( m_imageDataRef->GetOrigin() );
      m_imageData->SetSpacing( m_imageDataRef->GetSpacing() );
      m_imageData->SetDimensions( m_imageDataRef->GetDimensions() );
      m_imageData->AllocateScalars();
      float* ptr = ( float* )m_imageData->GetScalarPointer();
      int* nDim = m_imageData->GetDimensions();
     // cout << nDim[0] << ", " << nDim[1] << ", " << nDim[2] << endl;
      memset( ptr, 0, sizeof( float ) * nDim[0] * nDim[1] * nDim[2] );
     }
    */
    m_imageData = m_volumeSource->GetImageOutput();

    InitializeActors();

    mProperties->SetVolumeSource( m_volumeSource );
    // mProperties->SetColorMap( LayerPropertiesMRI::LUT );

    m_sFilename = "";

    if ( bCopyVoxelData )
      SetModified();
  }

  return true;
}

bool LayerMRI::SaveVolume( wxWindow* wnd, wxCommandEvent& event )
{
// if ( m_sFilename.size() == 0 || m_imageData.GetPointer() == NULL )
  if ( m_sFilename.size() == 0 || m_imageData == NULL )
    return false;

  m_volumeSource->UpdateMRIFromImage( m_imageData, wnd, event );

// wxPostEvent( wnd, event );
  bool bSaved = m_volumeSource->MRIWrite( m_sFilename.c_str() );
  if ( !bSaved )
    m_bModified = true;

  event.SetInt( 99 );
  wxPostEvent( wnd, event );

  return bSaved;
}

bool LayerMRI::Rotate( std::vector<RotationElement>& rotations, wxWindow* wnd, wxCommandEvent& event )
{
  m_bResampleToRAS = false;
  m_volumeSource->SetResampleToRAS( m_bResampleToRAS );
  if ( IsModified() )
    m_volumeSource->UpdateMRIFromImage( m_imageData, wnd, event );

  return m_volumeSource->Rotate( rotations, wnd, event );
}

void LayerMRI::InitializeVolume()
{
  if ( m_volumeSource == NULL )
    return;

  FSVolume* source = m_volumeSource;

  float RASBounds[6];
  source->GetBounds( RASBounds );
  m_dWorldOrigin[0] = RASBounds[0];
  m_dWorldOrigin[1] = RASBounds[2];
  m_dWorldOrigin[2] = RASBounds[4];
  source->GetPixelSize( m_dWorldVoxelSize );

  m_dWorldSize[0] = ( ( int )( (RASBounds[1] - RASBounds[0]) / m_dWorldVoxelSize[0] ) ) * m_dWorldVoxelSize[0];
  m_dWorldSize[1] = ( ( int )( (RASBounds[3] - RASBounds[2]) / m_dWorldVoxelSize[1] ) ) * m_dWorldVoxelSize[1];
  m_dWorldSize[2] = ( ( int )( (RASBounds[5] - RASBounds[4]) / m_dWorldVoxelSize[2] ) ) * m_dWorldVoxelSize[2];

  m_imageData = source->GetImageOutput();
}


void LayerMRI::InitializeActors()
{
  for ( int i = 0; i < 3; i++ )
  {
    // The reslice object just takes a slice out of the volume.
    //
    mReslice[i] = vtkSmartPointer<vtkImageReslice>::New();
    mReslice[i]->SetInput( m_imageData );
//  mReslice[i]->SetOutputSpacing( sizeX, sizeY, sizeZ );
    mReslice[i]->BorderOff();

    // This sets us to extract slices.
    mReslice[i]->SetOutputDimensionality( 2 );

    // This will change depending what orienation we're in.
    mReslice[i]->SetResliceAxesDirectionCosines( 1, 0, 0,
        0, 1, 0,
        0, 0, 1 );

    // This will change to select a different slice.
    mReslice[i]->SetResliceAxesOrigin( 0, 0, 0 );

    //
    // Image to colors using color table.
    //
    mColorMap[i] = vtkSmartPointer<vtkImageMapToColors>::New();
    mColorMap[i]->SetLookupTable( mProperties->GetGrayScaleTable() );
    mColorMap[i]->SetInput( mReslice[i]->GetOutput() );
    mColorMap[i]->SetOutputFormatToRGBA();
    mColorMap[i]->PassAlphaToOutputOn();

    //
    // Prop in scene with plane mesh and texture.
    //
    m_sliceActor2D[i]->SetInput( mColorMap[i]->GetOutput() );
    m_sliceActor3D[i]->SetInput( mColorMap[i]->GetOutput() );

    // Set ourselves up.
    this->OnSlicePositionChanged( i );
  }

  this->UpdateResliceInterpolation();
  this->UpdateTextureSmoothing();
  this->UpdateOpacity();
  this->UpdateColorMap();
}

void LayerMRI::UpdateOpacity()
{
  for ( int i = 0; i < 3; i++ )
  {
    m_sliceActor2D[i]->SetOpacity( mProperties->GetOpacity() );
    m_sliceActor3D[i]->SetOpacity( mProperties->GetOpacity() );
  }
}

void LayerMRI::UpdateColorMap ()
{
  assert( mProperties );

  for ( int i = 0; i < 3; i++ )
    mColorMap[i]->SetActiveComponent( m_nActiveFrame );

  switch ( mProperties->GetColorMap() )
  {
  case LayerPropertiesMRI::NoColorMap:
    for ( int i = 0; i < 3; i++ )
      mColorMap[i]->SetLookupTable( NULL );
    break;

  case LayerPropertiesMRI::Grayscale:
    for ( int i = 0; i < 3; i++ )
      mColorMap[i]->SetLookupTable( mProperties->GetGrayScaleTable() );
    break;

  case LayerPropertiesMRI::Heat:
    for ( int i = 0; i < 3; i++ )
      mColorMap[i]->SetLookupTable( mProperties->GetHeatScaleTable() );
    break;
  case LayerPropertiesMRI::Jet:
    for ( int i = 0; i < 3; i++ )
      mColorMap[i]->SetLookupTable( mProperties->GetJetScaleTable() );
    break;
  case LayerPropertiesMRI::LUT:
    for ( int i = 0; i < 3; i++ )
      mColorMap[i]->SetLookupTable( mProperties->GetLUTTable() );
    break;
  default:
    break;
  }
}

void LayerMRI::UpdateResliceInterpolation ()
{
  assert( mProperties );

  for ( int i = 0; i < 3; i++ )
  {
    if ( mReslice[i].GetPointer() )
    {
      mReslice[i]->SetInterpolationMode( mProperties->GetResliceInterpolation() );
    }
  }
}

void LayerMRI::UpdateTextureSmoothing ()
{
  assert( mProperties );

  for ( int i = 0; i < 3; i++ )
  {
    m_sliceActor2D[i]->SetInterpolate( mProperties->GetTextureSmoothing() );
    m_sliceActor3D[i]->SetInterpolate( mProperties->GetTextureSmoothing() );
  }
}

void LayerMRI::UpdateContour( int nSegValue )
{
    /*
	if ( GetProperties()->GetShowAsContour() )
	{
		MyUtils::BuildContourActor( GetImageData(), 
									GetProperties()->GetContourMinThreshold(), 
									GetProperties()->GetContourMaxThreshold(),
									m_actorContour );
  }
    */
    
    /*
    if ( GetProperties()->GetShowAsContour() && GetProperties()->GetColorMap() == LayerPropertiesMRI::LUT )
    {
        if ( nSegValue >= 0 )   // only update the given index
        {
            UpdateContourActor( nSegValue );
        }
        else    // update all
        {
            double overall_range[2];
            m_imageData->GetPointData()->GetScalars()->GetRange( overall_range );
            cout << overall_range[0] << " " << overall_range[1] << endl;
            for ( size_t i = 0; i < m_segActors.size(); i++ )
            {
                m_segActors[i].actor->Delete();
            }
            m_segActors.clear();
            vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
            threshold->SetOutputScalarTypeToShort();
            threshold->SetInput( GetImageData() );            
            int range[2];
            for ( int i = (int)overall_range[0]; i <= overall_range[1]; i++ )
            {
                threshold->ThresholdBetween( i-0.5, i+0.5 );
                threshold->ReplaceOutOn();
                threshold->SetOutValue( 0 );
                threshold->Update();
                threshold->GetOutput()->GetPointData()->GetScalars()->GetRange( range[2] );
                if ( range[1] > 0 )
                {
                    SegmentationActor sa;
                    sa.id = i;
                    MyUtils::BuildContourActor( GetImageData(), nSegValue - 0.5, nSegValue + 0.5, sa.actor );
                    m_segActors.push_back( sa );
                } 
            }
        }
    }
    */
    
  if ( GetProperties()->GetShowAsContour() && GetProperties()->GetColorMap() == LayerPropertiesMRI::LUT )
  {
    double overall_range[2];
    m_imageData->GetPointData()->GetScalars()->GetRange( overall_range );
    //   cout << overall_range[0] << " " << overall_range[1] << endl;
    vtkSmartPointer<vtkImageThreshold> threshold = vtkSmartPointer<vtkImageThreshold>::New();
    threshold->SetOutputScalarTypeToShort();
    threshold->SetInput( m_imageData );
    threshold->ThresholdBetween( overall_range[0], overall_range[1] );
    threshold->ReplaceOutOn();
    threshold->SetOutValue( 0 );
    vtkSmartPointer<vtkContourFilter> contour = vtkSmartPointer<vtkContourFilter>::New();
    contour->SetInput( threshold->GetOutput() );
    int n = 0;
    if ( overall_range[0] == 0 )
      overall_range[0] = 1;
    for ( int i = (int)overall_range[0]; i <= overall_range[1]; i++ )
    {
      contour->SetValue( n++, i );
    }
    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInput( contour->GetOutput() );
    mapper->SetLookupTable( mProperties->GetLUTTable() );
    mapper->SetScalarRange( overall_range );
    mapper->ScalarVisibilityOn();
    m_actorContour->SetMapper( mapper );
  }
    
//	UpdateVolumeRendering();
}

/*
  if ( GetProperties()->GetShowAsContour() )
  {
    MyUtils::BuildContourActor( GetImageData(),
                                GetProperties()->GetContourMinThreshold(),
                                GetProperties()->GetContourMaxThreshold(),
                                m_actorContour );
  }
*/

void LayerMRI::UpdateContourActor( int nSegValue )
{
  SegmentationActor sa;
  sa.id = -1;
  for ( size_t i = 0; i < m_segActors.size(); i++ )
  {
    if ( m_segActors[i].id == nSegValue )
    {
      sa = m_segActors[i];
      break;
    }
  }
  if ( sa.id < 0 )
  {
    sa.id = nSegValue;
    sa.actor = vtkActor::New();
    m_segActors.push_back( sa );
  }
  MyUtils::BuildContourActor( GetImageData(), nSegValue - 0.5, nSegValue + 0.5, sa.actor );
}

void LayerMRI::UpdateVolumeRendering()
{
  if ( GetProperties()->GetShowAsContour() )
  {
    MyUtils::BuildVolume( GetImageData(),
                          GetProperties()->GetContourMinThreshold(),
                          GetProperties()->GetContourMaxThreshold(),
                          m_propVolume );
  }
}

void LayerMRI::Append2DProps( vtkRenderer* renderer, int nPlane )
{
  wxASSERT ( nPlane >= 0 && nPlane <= 2 );

  if ( mProperties->GetDisplayVector() )
    renderer->AddViewProp( m_vectorActor2D[nPlane] );
  else
    renderer->AddViewProp( m_sliceActor2D[nPlane] );
}

void LayerMRI::Append3DProps( vtkRenderer* renderer, bool* bSliceVisibility )
{
	bool bContour = GetProperties()->GetShowAsContour();
	for ( int i = 0; i < 3; i++ )
	{
		if ( !bContour && ( bSliceVisibility == NULL || bSliceVisibility[i] ) )
    {
      if ( mProperties->GetDisplayVector() )
        renderer->AddViewProp( m_vectorActor3D[i] );
			else
        renderer->AddViewProp( m_sliceActor3D[i] );
    } 
	}
	
	if ( bContour )
	{
   // for ( size_t i = 0; i < m_segActors.size(); i ++ )
	 // renderer->AddViewProp( m_segActors[i].actor );
    renderer->AddViewProp( m_actorContour );
	}
}

/*
void LayerMRI::SetSliceNumber( int* sliceNumber )
{
 if ( sliceNumber[0] != m_nSliceNumber[0] || sliceNumber[1] != m_nSliceNumber[1] ||
   sliceNumber[2] != m_nSliceNumber[2] )
 {
  m_nSliceNumber[0] = sliceNumber[0];
  m_nSliceNumber[1] = sliceNumber[1];
  m_nSliceNumber[2] = sliceNumber[2];


 }
}
*/

void LayerMRI::SetSlicePositionToWorldCenter()
{
  if ( m_volumeSource == NULL )
    return;

  // Get some values from the MRI.
  double pos[3];
  for ( int i = 0; i < 3; i++ )
    pos[i] = ((int)( m_dWorldSize[i]/2/m_dWorldVoxelSize[i] ) + 0.0 ) * m_dWorldVoxelSize[i] + m_dWorldOrigin[i];

  SetSlicePosition( pos );
}

void LayerMRI::OnSlicePositionChanged( int nPlane )
{
  if ( m_volumeSource == NULL || nPlane < 0 || nPlane > 2)
    return;

  assert( mProperties );

 
  // display slice image
  vtkSmartPointer<vtkMatrix4x4> matrix =
      vtkSmartPointer<vtkMatrix4x4>::New();
  matrix->Identity();
  switch ( nPlane )
  {
    case 0:
      m_sliceActor2D[0]->PokeMatrix( matrix );
      m_sliceActor2D[0]->SetPosition( m_dSlicePosition[0], 0, 0 );
      m_sliceActor2D[0]->RotateX( 90 );
      m_sliceActor2D[0]->RotateY( -90 );
      m_sliceActor3D[0]->PokeMatrix( matrix );
      m_sliceActor3D[0]->SetPosition( m_dSlicePosition[0], 0, 0 );
      m_sliceActor3D[0]->RotateX( 90 );
      m_sliceActor3D[0]->RotateY( -90 );
  
      // Putting negatives in the reslice axes cosines will flip the
      // image on that axis.
      mReslice[0]->SetResliceAxesDirectionCosines( 0, -1, 0,
          0, 0, 1,
          1, 0, 0 );
      mReslice[0]->SetResliceAxesOrigin( m_dSlicePosition[0], 0, 0  );
      mReslice[0]->Modified();
      break;
    case 1:
      m_sliceActor2D[1]->PokeMatrix( matrix );
      m_sliceActor2D[1]->SetPosition( 0, m_dSlicePosition[1], 0 );
      m_sliceActor2D[1]->RotateX( 90 );
      // m_sliceActor2D[1]->RotateY( 180 );
      m_sliceActor3D[1]->PokeMatrix( matrix );
      m_sliceActor3D[1]->SetPosition( 0, m_dSlicePosition[1], 0 );
      m_sliceActor3D[1]->RotateX( 90 );
      // m_sliceActor3D[1]->RotateY( 180 );
  
      // Putting negatives in the reslice axes cosines will flip the
      // image on that axis.
      mReslice[1]->SetResliceAxesDirectionCosines( 1, 0, 0,
          0, 0, 1,
          0, 1, 0 );
      mReslice[1]->SetResliceAxesOrigin( 0, m_dSlicePosition[1], 0 );
      mReslice[1]->Modified();
      break;
    case 2:
      m_sliceActor2D[2]->SetPosition( 0, 0, m_dSlicePosition[2] );
      // m_sliceActor2D[2]->RotateY( 180 );
      m_sliceActor3D[2]->SetPosition( 0, 0, m_dSlicePosition[2] );
      // m_sliceActor3D[2]->RotateY( 180 );
  
      mReslice[2]->SetResliceAxesDirectionCosines( 1, 0, 0,
          0, 1, 0,
          0, 0, 1 );
      mReslice[2]->SetResliceAxesOrigin( 0, 0, m_dSlicePosition[2]  );
      mReslice[2]->Modified();
      break;
  }
  // display 4D data as vector
  if ( mProperties->GetDisplayVector() )
  {
    UpdateVectorActor( nPlane );
  }
}

LayerPropertiesMRI* LayerMRI::GetProperties()
{
  return mProperties;
}

void LayerMRI::DoListenToMessage( std::string const iMessage, void* iData, void* sender )
{
  if ( iMessage == "ColorMapChanged" )
  {
    this->UpdateColorMap();
    this->SendBroadcast( "LayerActorUpdated", this, this );
  }
  else if ( iMessage == "ResliceInterpolationChanged" )
  {
    this->UpdateResliceInterpolation();
    this->SendBroadcast( "LayerActorUpdated", this, this );
  }
  else if ( iMessage == "OpacityChanged" )
  {
    this->UpdateOpacity();
    this->SendBroadcast( "LayerActorUpdated", this, this );
  }
  else if ( iMessage == "TextureSmoothingChanged" )
  {
    this->UpdateTextureSmoothing();
    this->SendBroadcast( "LayerActorUpdated", this, this );
  }
  else if ( iMessage == "LayerContourChanged" )
  {
    this->UpdateContour();
    this->SendBroadcast( "LayerActorUpdated", this, this );
  }
  else if ( iMessage == "LayerContourShown" )
  {
    this->UpdateContour();
    this->SendBroadcast( iMessage, this, this );
  }
  else if ( iMessage == "DisplayModeChanged" )
  {
    this->UpdateVectorActor();
    this->SendBroadcast( iMessage, this, this );
  }
}

void LayerMRI::SetVisible( bool bVisible )
{
  for ( int i = 0; i < 3; i++ )
  {
    m_sliceActor2D[i]->SetVisibility( bVisible ? 1 : 0 );
    m_sliceActor3D[i]->SetVisibility( bVisible ? 1 : 0 );
    m_vectorActor2D[i]->SetVisibility( bVisible ? 1 : 0 );
    m_vectorActor3D[i]->SetVisibility( bVisible ? 1 : 0 );
  }
//  m_actorContour->SetVisibility( bVisible ? 1 : 0 );
  this->SendBroadcast( "LayerActorUpdated", this );
}

bool LayerMRI::IsVisible()
{
  return m_sliceActor2D[0]->GetVisibility() > 0;
}

double LayerMRI::GetVoxelValue( double* pos )
{ 
  if ( m_imageData == NULL )
    return 0;

  double* orig = m_imageData->GetOrigin();
  double* vsize = m_imageData->GetSpacing();
  int* ext = m_imageData->GetExtent();

  int n[3];
  for ( int i = 0; i < 3; i++ )
  {
    n[i] = (int)( ( pos[i] - orig[i] ) / vsize[i] + 0.5 );
  }

  if ( n[0] < ext[0] || n[0] > ext[1] ||
       n[1] < ext[2] || n[1] > ext[3] ||
       n[2] < ext[4] || n[2] > ext[5] )
    return 0;
  else
    return m_imageData->GetScalarComponentAsDouble( n[0], n[1], n[2], m_nActiveFrame );
}

double LayerMRI::GetVoxelValueByOriginalIndex( int i, int j, int k )
{
  return m_volumeSource->GetVoxelValue( i, j, k, m_nActiveFrame );
}

void LayerMRI::SetModified()
{
  mReslice[0]->Modified();
  mReslice[1]->Modified();
  mReslice[2]->Modified();

  LayerVolumeBase::SetModified();
}

std::string LayerMRI::GetLabelName( double value )
{
  int nIndex = (int)value;
  if ( GetProperties()->GetColorMap() == LayerPropertiesMRI::LUT )
  {
    COLOR_TABLE* ct = GetProperties()->GetLUTCTAB();
    if ( !ct )
      return "";
    char name[128];
    int nValid = 0;
    int nTotalCount = 0;
    CTABgetNumberOfTotalEntries( ct, &nTotalCount );
    if ( nIndex < nTotalCount )
      CTABisEntryValid( ct, nIndex, &nValid );
    if ( nValid && CTABcopyName( ct, nIndex, name, 128 ) == 0 )
    {
      return name;
    }
  }

  return "";
}

void LayerMRI::RemapPositionToRealRAS( const double* pos_in, double* pos_out )
{
  m_volumeSource->TargetToRAS( pos_in, pos_out );
}

void LayerMRI::RemapPositionToRealRAS( double x_in, double y_in, double z_in,
                                       double& x_out, double& y_out, double& z_out )
{
  m_volumeSource->TargetToRAS( x_in, y_in, z_in, x_out, y_out, z_out );
}

void LayerMRI::RASToTarget( const double* pos_in, double* pos_out )
{
  m_volumeSource->RASToTarget( pos_in, pos_out );
}

int LayerMRI::GetNumberOfFrames()
{
  if ( m_imageData )
    return m_imageData->GetNumberOfScalarComponents();
  else
    return 1;
}

void LayerMRI::SetActiveFrame( int nFrame )
{
  if ( nFrame != m_nActiveFrame && nFrame >= 0 && nFrame < this->GetNumberOfFrames() )
  {
    m_nActiveFrame = nFrame;
    this->DoListenToMessage( "ColorMapChanged", this, this );
    this->SendBroadcast( "LayerActiveFrameChanged", this, this );
  }
}

bool LayerMRI::HasProp( vtkProp* prop )
{
  for ( int i = 0; i < 3; i++ )
  {
    if ( m_sliceActor3D[i] == prop )
      return true;
  }
  return false;
}

void LayerMRI::RASToOriginalIndex( const double* pos, int* n )
{
  m_volumeSource->RASToOriginalIndex( (float)(pos[0]), (float)(pos[1]), (float)(pos[2]),
                                      n[0], n[1], n[2] );
}

void LayerMRI::OriginalIndexToRAS( const int* n, double* pos )
{
  float x, y, z;
  m_volumeSource->OriginalIndexToRAS( n[0], n[1], n[2], x, y, z );
  pos[0] = x;
  pos[1] = y;
  pos[2] = z;
}

void LayerMRI::UpdateVectorActor()
{
  for ( int i = 0; i < 3; i++ )
  {
    UpdateVectorActor( i );
  }
}

void LayerMRI::UpdateVectorActor( int nPlane )
{
  UpdateVectorActor( nPlane, m_imageData );
}

void LayerMRI::UpdateVectorActor( int nPlane, vtkImageData* imagedata )
{
  double* pos = GetSlicePosition();
  double* orig = imagedata->GetOrigin();
  double* voxel_size = imagedata->GetSpacing();
  int* dim = imagedata->GetDimensions();
  int n[3];
  for ( int i = 0; i < 3; i++ )
    n[i] = (int)( ( pos[i] - orig[i] ) / voxel_size[i] + 0.5 );

//  vtkPolyData* polydata = vtkPolyDataMapper::SafeDownCast( m_vectorActor2D[nPlane]->GetMapper() )->GetInput();
  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkUnsignedCharArray> scalars = vtkSmartPointer<vtkUnsignedCharArray>::New();
  scalars->SetNumberOfComponents( 4 );  
  polydata->SetPoints( points );
  polydata->SetLines( lines );
  polydata->GetPointData()->SetScalars( scalars );
  if ( n[0] < 0 || n[0] >= dim[0] ||
       n[1] < 0 || n[1] >= dim[1] ||
       n[2] < 0 || n[2] >= dim[2] )
  {
    vtkPolyDataMapper::SafeDownCast( m_vectorActor2D[nPlane]->GetMapper() )->SetInput( polydata );
    vtkPolyDataMapper::SafeDownCast( m_vectorActor3D[nPlane]->GetMapper() )->SetInput( polydata );
    return;
  }
  
  int nCnt = 0;
  double scale = min( min( voxel_size[0], voxel_size[1] ), voxel_size[2] ) / 2;
  
  if ( GetProperties()->GetVectorRepresentation() == LayerPropertiesMRI::VR_Bar )
  {
    vtkSmartPointer<vtkTubeFilter> tube = vtkSmartPointer<vtkTubeFilter>::New();
    tube->SetInput( polydata );
    tube->SetNumberOfSides( 4 );
    tube->SetRadius( scale * 0.3 );
    tube->CappingOn();
    vtkPolyDataMapper::SafeDownCast( m_vectorActor2D[nPlane]->GetMapper() )->SetInput( tube->GetOutput() );
    vtkPolyDataMapper::SafeDownCast( m_vectorActor3D[nPlane]->GetMapper() )->SetInput( tube->GetOutput() );
  }
  else
  {
    vtkPolyDataMapper::SafeDownCast( m_vectorActor2D[nPlane]->GetMapper() )->SetInput( polydata );
    vtkPolyDataMapper::SafeDownCast( m_vectorActor3D[nPlane]->GetMapper() )->SetInput( polydata );
  }
  
  unsigned char c[4] = { 0, 0, 0, 255 };
  switch ( nPlane )
  {
    case 0:
      for ( int i = 0; i < dim[1]; i++ )
      {
        for ( int j = 0; j < dim[2]; j++ )
        {
          double v[3];
          double pt[3];
          v[0] = imagedata->GetScalarComponentAsDouble( n[0], i, j, 0 );
          v[1] = imagedata->GetScalarComponentAsDouble( n[0], i, j, 1 );
          v[2] = imagedata->GetScalarComponentAsDouble( n[0], i, j, 2 );
          if ( vtkMath::Normalize( v ) != 0 )
          {
            v[1] = -v[1];         // by default invert Y !!
            if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_X )
              v[0] = -v[0];
            else if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_Y )
              v[1] = -v[1];
            else if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_Z )
              v[2] = -v[2];
        
            pt[0] = orig[0] + voxel_size[0] * n[0];
            pt[1] = orig[1] + voxel_size[1] * i;
            pt[2] = orig[2] + voxel_size[2] * j;
            lines->InsertNextCell( 2 );
            points->InsertNextPoint( pt[0] + scale * v[0], 
                                    pt[1] + scale * v[1], 
                                    pt[2] + scale * v[2] );
            points->InsertNextPoint( pt[0] - scale * v[0], 
                                    pt[1] - scale * v[1], 
                                    pt[2] - scale * v[2] );
            lines->InsertCellPoint( nCnt++ );
            lines->InsertCellPoint( nCnt++ );
            c[0] = (int)(fabs( v[0] *255 ) );
            c[1] = (int)(fabs( v[1] *255 ) );
            c[2] = (int)(fabs( v[2] *255 ) );
            scalars->InsertNextTupleValue( c );
            scalars->InsertNextTupleValue( c );
          }
        }
      }
      break;
    case 1:
      for ( int i = 0; i < dim[0]; i++ )
      {
        for ( int j = 0; j < dim[2]; j++ )
        {
          double v[3];
          double pt[3];
          v[0] = imagedata->GetScalarComponentAsDouble( i, n[1], j, 0 );
          v[1] = imagedata->GetScalarComponentAsDouble( i, n[1], j, 1 );
          v[2] = imagedata->GetScalarComponentAsDouble( i, n[1], j, 2 );
          if ( vtkMath::Normalize( v ) != 0 )
          {
            v[1] = -v[1];         // by default invert Y !!
            if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_X )   
              v[0] = -v[0];
            else if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_Y )
              v[1] = -v[1];
            else if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_Z )
              v[2] = -v[2];
            
            pt[0] = orig[0] + voxel_size[0] * i;
            pt[1] = orig[1] + voxel_size[1] * n[1];
            pt[2] = orig[2] + voxel_size[2] * j;
            lines->InsertNextCell( 2 );
            points->InsertNextPoint( pt[0] + scale * v[0], 
                                    pt[1] + scale * v[1], 
                                    pt[2] + scale * v[2] );
            points->InsertNextPoint( pt[0] - scale * v[0], 
                                    pt[1] - scale * v[1], 
                                    pt[2] - scale * v[2] );
            lines->InsertCellPoint( nCnt++ );
            lines->InsertCellPoint( nCnt++ );
            c[0] = (int)(fabs( v[0] *255 ) );
            c[1] = (int)(fabs( v[1] *255 ) );
            c[2] = (int)(fabs( v[2] *255 ) );
            scalars->InsertNextTupleValue( c );
            scalars->InsertNextTupleValue( c );
          }
        }
      }
      break;
    case 2:
      for ( int i = 0; i < dim[0]; i++ )
      {
        for ( int j = 0; j < dim[1]; j++ )
        {
          double v[3];
          double pt[3];
          v[0] = imagedata->GetScalarComponentAsDouble( i, j, n[2], 0 );
          v[1] = imagedata->GetScalarComponentAsDouble( i, j, n[2], 1 );
          v[2] = imagedata->GetScalarComponentAsDouble( i, j, n[2], 2 );
          if ( vtkMath::Normalize( v ) != 0 )
          {
            v[1] = -v[1];         // by default invert Y !!
            if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_X )   
              v[0] = -v[0];
            else if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_Y )
              v[1] = -v[1];
            else if ( GetProperties()->GetVectorInversion() == LayerPropertiesMRI::VI_Z )
              v[2] = -v[2];
            
            pt[0] = orig[0] + voxel_size[0] * i;
            pt[1] = orig[1] + voxel_size[1] * j;
            pt[2] = orig[2] + voxel_size[2] * n[2];
            lines->InsertNextCell( 2 );
            points->InsertNextPoint( pt[0] + scale * v[0], 
                                    pt[1] + scale * v[1], 
                                    pt[2] + scale * v[2] );
            points->InsertNextPoint( pt[0] - scale * v[0], 
                                    pt[1] - scale * v[1], 
                                    pt[2] - scale * v[2] );
            lines->InsertCellPoint( nCnt++ );
            lines->InsertCellPoint( nCnt++ );
            c[0] = (int)(fabs( v[0] *255 ) );
            c[1] = (int)(fabs( v[1] *255 ) );
            c[2] = (int)(fabs( v[2] *255 ) );
            scalars->InsertNextTupleValue( c );
            scalars->InsertNextTupleValue( c );
          }
        }
      }
      break;
    default:
      break;
  }
}

