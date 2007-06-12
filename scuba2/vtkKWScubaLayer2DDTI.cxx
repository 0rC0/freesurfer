/**
 * @file  vtkKWScubaLayer2DDTI.cxx
 * @brief Displays 2D slices of surfaces
 *
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: dsjen $
 *    $Date: 2007/06/12 19:53:26 $
 *    $Revision: 1.7 $
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


#include <string>
#include <stdexcept>

#include "vtkKWScubaLayer2DDTI.h"

#include "ScubaCollectionProperties.h"
#include "ScubaCollectionPropertiesDTI.h"
#include "ScubaInfoItem.h"
#include "vtkActor.h"
#include "vtkFDTensorGlyph.h"
#include "vtkFSVolumeSource.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageClip.h"
#include "vtkImageReslice.h"
#include "vtkImageShrink3D.h"
#include "vtkLODActor.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineFilter.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"

using namespace std;

vtkStandardNewMacro( vtkKWScubaLayer2DDTI );
vtkCxxRevisionMacro( vtkKWScubaLayer2DDTI, "$Revision: 1.7 $" );

vtkKWScubaLayer2DDTI::vtkKWScubaLayer2DDTI () :
  mDTIProperties( NULL ),
  mVolumeToRAS( NULL ),
  mReducedVolume( NULL ),
  mClip( NULL ),
  mGlyph( NULL ),
  mMapper( NULL ),
  mActor( NULL ),
  mEdgeActor( NULL ) 
{
  for( int n = 0; n < 3; n++ ) {
    mWorldCenter[n] = 0;
    mWorldSize[n] = 0;
  }
}

vtkKWScubaLayer2DDTI::~vtkKWScubaLayer2DDTI () {

  if( mVolumeToRAS )
    mVolumeToRAS->Delete();
  if( mReducedVolume )
    mReducedVolume->Delete();
  if( mClip )
    mClip->Delete();
  if( mGlyph )
    mGlyph->Delete();
  if( mMapper )
    mMapper->Delete();
  if ( mActor )
    mActor->Delete();
  if ( mEdgeActor )
    mEdgeActor->Delete();
}

void
vtkKWScubaLayer2DDTI::SetDTIProperties ( ScubaCollectionPropertiesDTI* const iProperties ) {
  mDTIProperties = iProperties;
}


void
vtkKWScubaLayer2DDTI::Create () {

  // Bail if we don't have our source yet.
  if( NULL == mDTIProperties )
    throw runtime_error( "vtkKWScubaLayer2DDTI::Create: No source" );
  
  // Get some values from the MRI.
  float RASBounds[6];
  this->GetUnscaledRASBounds( RASBounds );

  mWorldSize[0] = RASBounds[1] - RASBounds[0];
  mWorldSize[1] = RASBounds[3] - RASBounds[2];
  mWorldSize[2] = RASBounds[5] - RASBounds[4];

  mWorldCenter[0] = mDTIProperties->GetFAVolumeSource()->GetRASCenterX();
  mWorldCenter[1] = mDTIProperties->GetFAVolumeSource()->GetRASCenterY();
  mWorldCenter[2] = mDTIProperties->GetFAVolumeSource()->GetRASCenterZ();

  // 
  // Merged source
  //
  vtkImageAppendComponents* source = mDTIProperties->GetMergedSource();
  
  //
  // Transform
  //
  mVolumeToRAS = vtkImageReslice::New();
  mVolumeToRAS->InterpolateOn();

  // Nearest neighbor interpolation copies the vectors, so only linear
  // and cubic interpolations are allowed.
  mVolumeToRAS->
    SetInterpolationMode( mDTIProperties->GetTensorInterpolationType() );

  mVolumeToRAS->SetInputConnection( source->GetOutputPort() );
  mVolumeToRAS->SetOutputDimensionality( 3 );

  // This rotates the volume to the proper orientation. From
  // ImageReslice: "applying a transform to the resampling grid (which
  // lies in the output coordinate system) is equivalent to applying
  // the inverse of that transform to the input volume."
  double* rtv = mDTIProperties->GetFAVolumeSource()->GetRASToVoxelMatrix();
  
  // take out the scale component, so our reslice only rotates
  rtv[0] *= mDTIProperties->GetFAVolumeSource()->GetPixelSizeX();
  rtv[5] *= mDTIProperties->GetFAVolumeSource()->GetPixelSizeY();
  rtv[10] *= mDTIProperties->GetFAVolumeSource()->GetPixelSizeZ(); 
  
  vtkMatrix4x4* matrix = vtkMatrix4x4::New();
  matrix->SetElement( 0, 0, rtv[0] );
  matrix->SetElement( 0, 1, rtv[1] );
  matrix->SetElement( 0, 2, rtv[2] );
  matrix->SetElement( 0, 3, 0 );

  matrix->SetElement( 1, 0, rtv[4] );
  matrix->SetElement( 1, 1, rtv[5] );
  matrix->SetElement( 1, 2, rtv[6] );
  matrix->SetElement( 1, 3, 0 );

  matrix->SetElement( 2, 0, rtv[8] );
  matrix->SetElement( 2, 1, rtv[9] );
  matrix->SetElement( 2, 2, rtv[10] );
  matrix->SetElement( 2, 3, 0 );

  matrix->SetElement( 3, 0, 0 );
  matrix->SetElement( 3, 1, 0 );
  matrix->SetElement( 3, 2, 0 );
  matrix->SetElement( 3, 3, 1 );

  vtkTransform* transform = vtkTransform::New();
  transform->SetMatrix( matrix );
  matrix->Delete();

  mVolumeToRAS->SetResliceTransform( transform );
  mVolumeToRAS->BorderOff();
  transform->Delete();

  // This sets our output extent.
  mVolumeToRAS->SetOutputExtent( (int)RASBounds[0], (int)RASBounds[1],
                                (int)RASBounds[2], (int)RASBounds[3],
                                (int)RASBounds[4], (int)RASBounds[5] );

  //
  // Used for fast rendering.
  //
  mReducedVolume = vtkImageShrink3D::New();
  mReducedVolume->SetInputConnection( mVolumeToRAS->GetOutputPort() );

  // 
  // Clipper
  // 
  mClip = vtkImageClip::New();

  // Start out not displaying anything.
  mClip->SetOutputWholeExtent( 0, 0, 0, 0, 0, 0 );
  mClip->SetInputConnection( mReducedVolume->GetOutputPort() );
      
  // set up the transform for reorienting the tensors
  double *vtr = mDTIProperties->GetFAVolumeSource()->GetVoxelToRASMatrix();

  // remove the scale component
  vtr[0] *= mDTIProperties->GetFAVolumeSource()->GetPixelSizeX();
  vtr[5] *= mDTIProperties->GetFAVolumeSource()->GetPixelSizeY();
  vtr[10] *= mDTIProperties->GetFAVolumeSource()->GetPixelSizeZ(); 
  
  vtkMatrix4x4* voxelToRAS = vtkMatrix4x4::New();
  voxelToRAS->SetElement( 0, 0, vtr[0] );
  voxelToRAS->SetElement( 0, 1, vtr[1] );
  voxelToRAS->SetElement( 0, 2, vtr[2] );
  voxelToRAS->SetElement( 0, 3, 0 );
  voxelToRAS->SetElement( 1, 0, vtr[4] );
  voxelToRAS->SetElement( 1, 1, vtr[5] );
  voxelToRAS->SetElement( 1, 2, vtr[6] );
  voxelToRAS->SetElement( 1, 3, 0 );
  voxelToRAS->SetElement( 2, 0, vtr[8] );
  voxelToRAS->SetElement( 2, 1, vtr[9] );
  voxelToRAS->SetElement( 2, 2, vtr[10] );
  voxelToRAS->SetElement( 2, 3, 0 );
  voxelToRAS->SetElement( 3, 0, 0 );
  voxelToRAS->SetElement( 3, 1, 0 );
  voxelToRAS->SetElement( 3, 2, 0 );
  voxelToRAS->SetElement( 3, 3, 1 );
  
  vtkTransform *voxelTransform = vtkTransform::New();
  voxelTransform->PostMultiply();
  voxelTransform->SetMatrix( voxelToRAS );
  
  voxelToRAS->Delete();
  
  //
  // Glyph
  //
  mGlyph = vtkFDTensorGlyph::New();
  mGlyph->SetInputConnection( mClip->GetOutputPort() );

  // this reorients each tensor because when we resample the volume, the vector
  // data isn't reoriented
  mGlyph->SetVoxelToMeasurementFrameTransform( voxelTransform );
  
  voxelTransform->Delete();

  // scale the plane so that it's the voxel size is correct
  vtkTransform* sliceTransform = vtkTransform::New();
  sliceTransform->Scale( 
    mDTIProperties->GetFAVolumeSource()->GetPixelSizeX(),
    mDTIProperties->GetFAVolumeSource()->GetPixelSizeY(),
    mDTIProperties->GetFAVolumeSource()->GetPixelSizeZ() );
    
  vtkTransformPolyDataFilter* planePDF = vtkTransformPolyDataFilter::New();
  planePDF->SetInput( mGlyph->GetOutput() );
  planePDF->SetTransform( sliceTransform );
      
  sliceTransform->Delete();
  
  //
  // Mapper
  //
  mMapper = vtkPolyDataMapper::New();
  mMapper->UseLookupTableScalarRangeOn();
  mMapper->SetInput( planePDF->GetOutput() );
  mMapper->SetLookupTable( (vtkScalarsToColors*)mGlyph->GetColorTable() );
  
  planePDF->Delete();
  
  //
  // Main Actor
  //
  mActor = vtkLODActor::New();
  mActor->SetMapper( mMapper );
  mActor->GetProperty()->SetAmbient( .3 );
  mActor->GetProperty()->SetDiffuse( .2 );
  mActor->GetProperty()->SetSpecular( .2 );

  // Set the size and number of points for the LOD when it goes into
  // point mode.
  mActor->GetProperty()->SetPointSize( 5.0 );
  mActor->SetNumberOfCloudPoints( 800 );
  
  // 
  // Edge Mapper
  //
  vtkPolyDataMapper* edgeMapper = vtkPolyDataMapper::New();
  edgeMapper->UseLookupTableScalarRangeOn();
  edgeMapper->SetLookupTable( (vtkScalarsToColors*)mGlyph->GetColorTable() );
  edgeMapper->SetInput( planePDF->GetOutput() );

  //
  // Edge Actor
  //
  mEdgeActor = vtkLODActor::New();
  mEdgeActor->SetMapper( edgeMapper );
  mEdgeActor->GetProperty()->SetRepresentationToWireframe();
  mEdgeActor->GetProperty()->SetColor( 0, 0, 0 );
  mEdgeActor->GetProperty()->SetLineWidth( 1.4 );

  // Initial visibility.
  if( !mDTIProperties->GetRenderEdges() )
    mEdgeActor->SetVisibility( false );

  this->AddProp( mActor );
  this->AddProp( mEdgeActor );
  
  // Set up for the initial viewing state.
  this->UpdateOpacity();
  this->UpdateDetail();
  this->UpdateGlyphScaling();
  this->Update2DInfo();
}

void
vtkKWScubaLayer2DDTI::AddControls ( vtkKWWidget* iPanel ) {
  
}

void
vtkKWScubaLayer2DDTI::RemoveControls () {
  
}

void
vtkKWScubaLayer2DDTI::DoListenToMessage ( string isMessage, void* iData ) {

  if( isMessage == "OpacityChanged" ) {
    this->UpdateOpacity();
  } else if( isMessage == "TensorDetailChanged" ) {
    this->UpdateDetail();
  } else if( isMessage == "TensorScalingChanged" ) {
    this->UpdateGlyphScaling();
  } else if( isMessage == "RenderEdgesChanged" ) {
    this->UpdateEdges();
  } else if( isMessage == "Layer2DInfoChanged" ) {
    this->Update2DInfo();
  } else if( isMessage == "TensorInterpolationChanged" ) {

    if( NULL != mDTIProperties && NULL != mVolumeToRAS ) {
      
      mVolumeToRAS->SetInterpolationMode( 
        mDTIProperties->GetTensorInterpolationType() );

      this->PipelineChanged();
    }

  }
}

void
vtkKWScubaLayer2DDTI::GetRASBounds ( float ioBounds[6] ) const {

  if( mDTIProperties && mDTIProperties->GetFAVolumeSource() )
    mDTIProperties->GetFAVolumeSource()->GetRASBounds( ioBounds );
  else
    vtkKWScubaLayer::GetRASBounds( ioBounds );
}

void
vtkKWScubaLayer2DDTI::GetUnscaledRASBounds ( float ioBounds[6] ) const {
    
  this->GetRASBounds( ioBounds );
  
  const float scales[] = {
    mDTIProperties->GetFAVolumeSource()->GetPixelSizeX(),
    mDTIProperties->GetFAVolumeSource()->GetPixelSizeY(),
    mDTIProperties->GetFAVolumeSource()->GetPixelSizeZ()
  };
    
  // unscale the bounds
  for( int nDim=0; nDim<3; nDim++ ) {
    
    // unscale the bottom
    int nBound = nDim * 2;    
    ioBounds[ nBound ] = ioBounds[ nBound ] / scales[ nDim ];    
    
    // unscale the top
    nBound++;
    ioBounds[ nBound ] = ioBounds[ nBound ] / scales[ nDim ];
  }
    
}

void
vtkKWScubaLayer2DDTI::Get2DRASZIncrementHint ( float ioHint[3]) const {

  ioHint[0] = 1.0;
  ioHint[1] = 1.0;
  ioHint[2] = 1.0;
}

void
vtkKWScubaLayer2DDTI::GetInfoItems ( float iRAS[3],
                                      list<ScubaInfoItem>& ilInfo ) const {

  
  if( mDTIProperties ) {

    // get the index of the RAS our FA volume
    vtkFSVolumeSource* faSource = mDTIProperties->GetFAVolumeSource();
    int index[ 3 ];
    faSource->ConvertRASToIndex( iRAS[0], iRAS[1], iRAS[2],
      index[0], index[1], index[2] );
      
    // get the fa at this index
    const float fa = faSource->GetValueAtIndex( index[0], index[1], index[2] );
    
    char infoValue[1024];
    snprintf( infoValue, sizeof(infoValue), "%f", fa );

    ScubaInfoItem faInfo;
    faInfo.Clear();
    faInfo.SetLabel( "FA" );
    faInfo.SetValue( infoValue );
    faInfo.SetShortenHint( false );

    // Return it.
    ilInfo.push_back( faInfo );
 
    // return the eigenvalues
    vtkFSVolumeSource* eigenValuesSource = mDTIProperties->GetEigenValueVolumeSource();
      
    // each location will have three eigenvalues
    float eigenValues[ 3 ];
    for( int cValue=0; cValue<3; cValue++ ) {
      eigenValues[ cValue ] = eigenValuesSource->GetValueAtIndex( index[0], index[1], index[2], cValue );
    }
    
    snprintf( infoValue, sizeof(infoValue), "%f %f %f", eigenValues[ 0 ], eigenValues[ 1 ], eigenValues[ 2 ] );
    ScubaInfoItem eigenValueInfo;
    eigenValueInfo.Clear();
    eigenValueInfo.SetLabel( "Eigen Values" );
    eigenValueInfo.SetValue( infoValue );
    eigenValueInfo.SetShortenHint( false );

    // Return the eigenvalues
    ilInfo.push_back( eigenValueInfo );  
    
    for( int n=0; n<3; n++ ) {
      
      const int currentEigenVectorId = n + 1;
      
      // get the eigenvectors
      vtkFSVolumeSource* eigenVectorSource = mDTIProperties->GetEigenVectorVolumeSource( currentEigenVectorId );
      float eigenVector[ 3 ];
  
      for( int cValue=0; cValue<3; cValue++ ) {
        eigenVector[ cValue ] = eigenVectorSource->GetValueAtIndex( index[0], index[1], index[2], cValue );
      }
      
      snprintf( infoValue, sizeof(infoValue), "%f %f %f", eigenVector[ 0 ], eigenVector[ 1 ], eigenVector[ 2 ] );
      ScubaInfoItem eigenVectorInfo;
      eigenVectorInfo.Clear();
      
      char label[1024];
      snprintf( label, sizeof(label), "Eigen Vector %i", currentEigenVectorId );
      eigenVectorInfo.SetLabel( label );
      eigenVectorInfo.SetValue( infoValue );
      eigenVectorInfo.SetShortenHint( false );
  
      // Return the eigenvector
      ilInfo.push_back( eigenVectorInfo );  
    }

  
    
  } else {

    ScubaInfoItem info;
    info.Clear();
    info.SetLabel( "" );
    info.SetValue( "" );
    info.SetShortenHint( false );
  
    // Return it.
    ilInfo.push_back( info );
    
  }
}

void
vtkKWScubaLayer2DDTI::UpdateOpacity () {
  if ( mActor )
    if ( mActor->GetProperty() ) {
      mActor->GetProperty()->SetOpacity( mProperties->GetOpacity() );
      this->PipelineChanged();
    }
}

void
vtkKWScubaLayer2DDTI::UpdateDetail () {

  if( mReducedVolume ) {
    
    // figure out the level of detail
    int shrinkage = this->GetCurrentShrinkageValue();
        
    for( int n = 0; n<3; n++ ) {
      mReducedVolume->SetShrinkFactors( shrinkage, shrinkage, shrinkage );
    }
    mReducedVolume->AveragingOn();

    this->Update2DInfo();
  }
}

void
vtkKWScubaLayer2DDTI::UpdateGlyphScaling () {

  bool bUniform = false;
  bool bFA = false;
  
  switch( mDTIProperties->GetTensorScaling() ) {
  case ScubaCollectionPropertiesDTI::Uniform:
    bUniform = true;
    break;
  case ScubaCollectionPropertiesDTI::FA:
    bFA = true;
    break;
  }
    
  mGlyph->SetUniformScaling( bUniform );
  mGlyph->SetFAScaling( bFA );
  
  this->PipelineChanged();
}

void
vtkKWScubaLayer2DDTI::Update2DInfo () {
  
  if( NULL == mProperties )
    return;

  float RASBounds[6];
  
  this->GetUnscaledRASBounds( RASBounds );

  int RASBoundsI[6];
  for( int n = 0; n < 6; n++ )
    RASBoundsI[n] = static_cast< int >( RASBounds[n] );

  // We adjust the position here to take into effect our reduced
  // volume, basically scaling the ras Z position by the shrinkage
  // factor and remove any pixel scaling.
  float rasZ = 
    mViewProperties->Get2DRASZ() / mDTIProperties->GetFAVolumeSource()->GetPixelSize( mViewProperties->Get2DInPlane() );
  
  const int position = 
    static_cast< int >( floor( rasZ / static_cast< float >( this->GetCurrentShrinkageValue() ) ) );

  // Set the bounds of the clipper according to our in plane.
  switch ( mViewProperties->Get2DInPlane() ) {
  case 0:
    mClip->SetOutputWholeExtent( position, position,
				 RASBoundsI[2], RASBoundsI[3],
				 RASBoundsI[4], RASBoundsI[5] );
    break;
  case 1:
    mClip->SetOutputWholeExtent( RASBoundsI[0], RASBoundsI[1],
				 position, position,
				 RASBoundsI[4], RASBoundsI[5] );
    break;
  case 2:
    mClip->SetOutputWholeExtent( RASBoundsI[0], RASBoundsI[1],
				 RASBoundsI[2], RASBoundsI[3],
				 position, position );
    break;
  }
  
  // Now we need to move the actor into the correct and relative RAS
  // positive in the view plane, and also move the clipped volume
  // plane onto the view plane by adjusting it by the ras position,
  // using this reposition value to take into effect the reduced
  // volume size.
  const float reposition = 
    (float)abs( (int)rasZ % this->GetCurrentShrinkageValue() ) + 1;

  const int inPlane = mViewProperties->Get2DInPlane();
  mActor->SetPosition( (inPlane==0) ? -(rasZ - mWorldCenter[0]) + reposition :
		       mWorldCenter[0],
		       (inPlane==1) ? -(rasZ - mWorldCenter[1]) + reposition :
		       mWorldCenter[1],
		       (inPlane==2) ? -(rasZ - mWorldCenter[2]) - reposition :
		       mWorldCenter[2] );
  mEdgeActor->SetPosition( (inPlane==0) ? -(rasZ - mWorldCenter[0]) + reposition :
           mWorldCenter[0],
           (inPlane==1) ? -(rasZ - mWorldCenter[1]) + reposition :
           mWorldCenter[1],
           (inPlane==2) ? -(rasZ - mWorldCenter[2]) - reposition :
           mWorldCenter[2] );

  this->PipelineChanged();
}

int
vtkKWScubaLayer2DDTI::GetCurrentShrinkageValue () {
  
  int shrinkage = 0;

  switch( mDTIProperties->GetTensorDetail() ) {
  case ScubaCollectionPropertiesDTI::Least:
    shrinkage = 6;
    break;
  case ScubaCollectionPropertiesDTI::Less:
    shrinkage = 3;
    break;
  case ScubaCollectionPropertiesDTI::Normal:
    shrinkage = 1;
    break;
  }
  
  return shrinkage;
}

void
vtkKWScubaLayer2DDTI::UpdateEdges() {
  
  const bool isEdgeVisible = mDTIProperties->GetRenderEdges();
  mEdgeActor->SetVisibility( isEdgeVisible );    

  this->PipelineChanged();
}

