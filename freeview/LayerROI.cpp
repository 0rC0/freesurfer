/**
 * @file  LayerROI.cpp
 * @brief Layer data object for MRI volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: zkaufman $
 *    $Date: 2016/12/11 15:13:53 $
 *    $Revision: 1.47 $
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

#include "LayerROI.h"
#include "vtkRenderer.h"
#include "vtkImageReslice.h"
#include "vtkImageActor.h"
#include "vtkPolyDataMapper.h"
#include "vtkSmartPointer.h"
#include "vtkMatrix4x4.h"
#include "vtkImageMapToColors.h"
#include "vtkImageFlip.h"
#include "vtkTransform.h"
#include "vtkPlaneSource.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTexture.h"
#include "vtkImageActor.h"
#include "vtkActor.h"
#include "vtkRGBAColorTransferFunction.h"
#include "vtkLookupTable.h"
#include "vtkProperty.h"
#include "vtkFreesurferLookupTable.h"
#include "vtkImageChangeInformation.h"
#include "LayerPropertyROI.h"
#include "MyUtils.h"
#include "LayerMRI.h"
#include "FSLabel.h"
#include "FSSurface.h"
#include <stdlib.h>
#include <QDebug>
#include "LayerSurface.h"

LayerROI::LayerROI( LayerMRI* layerMRI, QObject* parent ) : LayerVolumeBase( parent )
{
    m_strTypeNames.push_back( "ROI" );
    m_sPrimaryType = "ROI";
    m_nVertexCache = NULL;

    m_label = new FSLabel( this );
    for ( int i = 0; i < 3; i++ )
    {
        m_dSlicePosition[i] = 0;
        m_sliceActor2D[i] = vtkImageActor::New();
        m_sliceActor3D[i] = vtkImageActor::New();
        m_sliceActor2D[i]->InterpolateOff();
        m_sliceActor3D[i]->InterpolateOff();
    }

    mProperty = new LayerPropertyROI( this );

    m_layerMappedSurface = NULL;
    m_layerSource = layerMRI;
    m_imageDataRef = layerMRI->GetImageData();
    if ( m_layerSource )
    {
        SetWorldOrigin( m_layerSource->GetWorldOrigin() );
        SetWorldVoxelSize( m_layerSource->GetWorldVoxelSize() );
        SetWorldSize( m_layerSource->GetWorldSize() );

        m_imageData = vtkSmartPointer<vtkImageData>::New();
        // m_imageData->DeepCopy( m_layerSource->GetRASVolume() );

        m_imageData->SetNumberOfScalarComponents( 1 );
        m_imageData->SetScalarTypeToFloat();
        m_imageData->SetOrigin( GetWorldOrigin() );
        m_imageData->SetSpacing( GetWorldVoxelSize() );
        m_imageData->SetDimensions( ( int )( m_dWorldSize[0] / m_dWorldVoxelSize[0] + 0.5 ),
                ( int )( m_dWorldSize[1] / m_dWorldVoxelSize[1] + 0.5 ),
                ( int )( m_dWorldSize[2] / m_dWorldVoxelSize[2] + 0.5 ) );
        m_imageData->AllocateScalars();
        float* ptr = (float*)m_imageData->GetScalarPointer();
        int* dim = m_imageData->GetDimensions();
        size_t nsize = ((size_t)dim[0])*dim[1]*dim[2];
        for (size_t i = 0; i < nsize; i++)
        {
            ptr[i] = -1;
        }
        m_fBlankValue = -1;
        InitializeActors();
    }

    connect( mProperty, SIGNAL(ColorMapChanged()), this, SLOT(UpdateColorMap()) );
    connect( mProperty, SIGNAL(OpacityChanged(double)), this, SLOT(UpdateOpacity()) );
    connect( mProperty, SIGNAL(ThresholdChanged(double)), this, SLOT(UpdateThreshold()));

    connect(this, SIGNAL(BaseVoxelEdited(QList<int>,bool)), SLOT(OnBaseVoxelEdited(QList<int>,bool)));
}

LayerROI::~LayerROI()
{
    for ( int i = 0; i < 3; i++ )
    {
        m_sliceActor2D[i]->Delete();
        m_sliceActor3D[i]->Delete();
    }
    if (m_nVertexCache)
        delete[] m_nVertexCache;
}

bool LayerROI::LoadROIFromFile( const QString& filename )
{
    if ( !m_label->LabelRead( filename.toAscii().data() ) )
    {
        return false;
    }
    double range[2];
    m_label->GetStatsRange(range);
    if (range[0] == range[1])
        range[1] = range[0] + 1;
    GetProperty()->SetHeatscaleValues(range[0], range[1]);
    GetProperty()->SetValueRange(range);
    m_fBlankValue = range[0]-1;

    m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume(), GetProperty()->GetThreshold());

    m_sFilename = filename;

    return true;
}

void LayerROI::InitializeActors()
{
    if ( m_layerSource == NULL )
    {
        return;
    }

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
        mColorMap[i]->SetLookupTable( GetProperty()->GetLookupTable() );
        mColorMap[i]->SetInputConnection( mReslice[i]->GetOutputPort() );
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

    this->UpdateOpacity();
    this->UpdateColorMap();
}


void LayerROI::UpdateOpacity()
{
    for ( int i = 0; i < 3; i++ )
    {
        m_sliceActor2D[i]->SetOpacity( GetProperty()->GetOpacity() );
        m_sliceActor3D[i]->SetOpacity( GetProperty()->GetOpacity() );
    }

    emit ActorUpdated();
}

void LayerROI::UpdateColorMap ()
{
    for ( int i = 0; i < 3; i++ )
    {
        mColorMap[i]->SetLookupTable( GetProperty()->GetLookupTable() );
        //    m_sliceActor2D[i]->GetProperty()->SetColor(1, 0, 0);
    }

    emit ActorUpdated();
}

void LayerROI::UpdateThreshold()
{
    if (m_label)
    {
        m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume(), GetProperty()->GetThreshold() );
        emit ActorUpdated();
    }
}

void LayerROI::Append2DProps( vtkRenderer* renderer, int nPlane )
{
    renderer->AddViewProp( m_sliceActor2D[nPlane] );
}

void LayerROI::Append3DProps( vtkRenderer* renderer, bool* bSliceVisibility )
{
    for ( int i = 0; i < 3; i++ )
    {
        if ( bSliceVisibility == NULL || bSliceVisibility[i] )
        {
            renderer->AddViewProp( m_sliceActor3D[i] );
        }
    }
}

void LayerROI::OnSlicePositionChanged( int nPlane )
{
    if ( !m_layerSource )
    {
        return;
    }

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
}

void LayerROI::SetVisible( bool bVisible )
{
    for ( int i = 0; i < 3; i++ )
    {
        m_sliceActor2D[i]->SetVisibility( bVisible ? 1 : 0 );
        m_sliceActor3D[i]->SetVisibility( bVisible ? 1 : 0 );
    }
    LayerVolumeBase::SetVisible(bVisible);
}

bool LayerROI::IsVisible()
{
    return m_sliceActor2D[0]->GetVisibility() > 0;
}

void LayerROI::SetModified()
{
    mReslice[0]->Modified();
    mReslice[1]->Modified();
    mReslice[2]->Modified();

    LayerVolumeBase::SetModified();
}

bool LayerROI::SaveROI( )
{
    if ( m_sFilename.size() == 0 || m_imageData.GetPointer() == NULL )
    {
        return false;
    }

    if (!m_layerMappedSurface)
        m_label->UpdateLabelFromImage( m_imageData, m_layerSource->GetSourceVolume() );

    bool bSaved = m_label->LabelWrite( m_sFilename.toAscii().data() );
    if ( !bSaved )
    {
        m_bModified = true;
    }

    return bSaved;
}

bool LayerROI::HasProp( vtkProp* prop )
{
    for ( int i = 0; i < 3; i++ )
    {
        if ( m_sliceActor2D[i] == prop || m_sliceActor3D[i] == prop )
        {
            return true;
        }
    }
    return false;
}

bool LayerROI::DoRotate( std::vector<RotationElement>& rotations )
{
    m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume() );
    return true;
}

void LayerROI::DoRestore()
{
    m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume() );
}

void LayerROI::UpdateLabelData( )
{
    if ( IsModified() )
    {
        m_label->UpdateLabelFromImage( m_imageData, m_layerSource->GetSourceVolume() );
    }
}

bool LayerROI::GetCentroidPosition(double *pos)
{
    UpdateLabelData();
    if (m_label->GetCentroidRASPosition(pos, m_layerSource->GetSourceVolume()))
    {
        m_layerSource->RASToTarget(pos, pos);
        return true;
    }
    else
        return false;
}

void LayerROI::GetStats(int nPlane, int *count_out, float *area_out,
                        LayerMRI *underlying_mri, double *mean_out, double *sd_out)
{
    if ( !m_imageData || nPlane < 0 || nPlane > 2 )
    {
        return;
    }

    int* dim = m_imageData->GetDimensions();
    double* origin = m_imageData->GetOrigin();
    double vs[3];
    m_imageData->GetSpacing( vs );
    unsigned char* ptr = (unsigned char*)m_imageData->GetScalarPointer();

    int cnt = 0;
    //  QList<int> indices;
    QList<float> coords;
    for ( int i = 0; i < dim[0]; i++ )
    {
        for ( int j = 0; j < dim[1]; j++ )
        {
            for ( int k = 0; k < dim[2]; k++ )
            {
                if ( ptr[k*dim[0]*dim[1]+j*dim[0]+i] != 0 )
                {
                    cnt++;
                    //          indices << i << j << k;
                    coords << i*vs[0]+origin[0] << j*vs[1]+origin[1] << k*vs[2]+origin[2];
                }
            }
        }
    }
    vs[nPlane] = 1.0;

    *count_out = cnt;
    *area_out = cnt*vs[0]*vs[1]*vs[2];

    if (underlying_mri)
        underlying_mri->GetVoxelStatsByTargetRAS(coords, mean_out, sd_out);
}

void LayerROI::SetMappedSurface(LayerSurface *s)
{
    if (m_layerMappedSurface)
        m_layerMappedSurface->RemoveMappedLabel(this);
    m_layerMappedSurface = s;
    if (s)
    {
        s->AddMappedLabel(this);
        //        connect(this, SIGNAL(Modified()), this, SLOT(OnUpdateLabelRequested()), Qt::UniqueConnection);
        m_label->Initialize(m_layerSource->GetSourceVolume(), s->GetSourceSurface(), s->IsInflated()?WHITE_VERTICES:CURRENT_VERTICES);
        OnUpdateLabelRequested();
    }
    else
        disconnect(this, SIGNAL(Modified()), this, SLOT(OnUpdateLabelRequested()));
}

void LayerROI::OnUpdateLabelRequested()
{
    //    UpdateLabelData();
    if (m_layerMappedSurface)
    {
        //        int coords = CURRENT_VERTICES;
        //        if (m_layerMappedSurface->IsInflated())
        //            coords = WHITE_VERTICES;
        //        m_label->FillUnassignedVertices(m_layerMappedSurface->GetSourceSurface(), m_layerSource->GetSourceVolume(), coords);
        m_layerMappedSurface->UpdateOverlay();
    }
}

void LayerROI::MapLabelColorData( unsigned char* colordata, int nVertexCount )
{
    double* rgbColor = GetProperty()->GetColor();
    double dThreshold = GetProperty()->GetThreshold();
    LABEL* label = m_label->GetRawLabel();
    for ( int i = 0; i < label->n_points; i++ )
    {
        int vno = label->lv[i].vno;
        if (vno < nVertexCount && vno >= 0 && !label->lv[i].deleted)
        {
            double opacity = 1;
            if (label->lv[i].stat >= dThreshold)
                opacity = 1;
            else
                opacity = 0;
            double rgb[4] = { rgbColor[0], rgbColor[1], rgbColor[2], 1 };
            //      if (m_nColorCode == Heatscale)
            //      {
            //        m_lut->GetColor(m_label->lv[i].stat, rgb);
            //      }
            colordata[vno*4]    = ( int )( colordata[vno*4]   * ( 1 - opacity ) + rgb[0] * 255 * opacity );
            colordata[vno*4+1]  = ( int )( colordata[vno*4+1] * ( 1 - opacity ) + rgb[1] * 255 * opacity );
            colordata[vno*4+2]  = ( int )( colordata[vno*4+2] * ( 1 - opacity ) + rgb[2] * 255 * opacity );
        }
    }
}

void LayerROI::OnBaseVoxelEdited(const QList<int> voxel_list, bool bAdd)
{
    if (m_layerMappedSurface)
    {
        int total_cnt = 0;
        for (int i = 0; i < voxel_list.size(); i+=3)
        {
            int n[3] = {voxel_list[i], voxel_list[i+1], voxel_list[i+2]};
            m_layerSource->TargetIndexToOriginalIndex(n, n);
            if (!m_nVertexCache && m_layerMappedSurface)
                m_nVertexCache = new int[m_layerMappedSurface->GetNumberOfVertices()];

            int cnt = 0;
            m_label->EditVoxel(n[0], n[1], n[2], bAdd, m_nVertexCache, &cnt);
            total_cnt += cnt;
        }
        if (total_cnt > 0)
        {
            qDebug() << "# of edited vertices:" << total_cnt;
            m_layerMappedSurface->UpdateOverlay(true, true);
        }
    }
}

void LayerROI::EditVertex(int nvo, bool bAdd)
{
    if (m_layerMappedSurface)
    {
        int ret = -1;
        int coords = m_layerMappedSurface->IsInflated()?WHITE_VERTICES:CURRENT_VERTICES;
        if (bAdd)
            ret = ::LabelAddVertex(m_label->GetRawLabel(), nvo, coords);
        else
            ret = ::LabelDeleteVertex(m_label->GetRawLabel(), nvo, coords);

        if (ret >= 0)
        {
            qDebug() << "Vertex" << (bAdd?"added":"deleted") << nvo;
            m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume() );
            m_layerMappedSurface->UpdateOverlay(true, true);
        }
    }
}

void LayerROI::Dilate(int nTimes)
{
    if (m_layerMappedSurface)
    {
        ::LabelDilate(m_label->GetRawLabel(), m_layerMappedSurface->GetSourceSurface()->GetMRIS(), nTimes, CURRENT_VERTICES);
        m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume() );
        m_layerMappedSurface->UpdateOverlay(true, true);
    }
}

void LayerROI::Erode(int nTimes)
{
    if (m_layerMappedSurface)
    {
        ::LabelErode(m_label->GetRawLabel(), m_layerMappedSurface->GetSourceSurface()->GetMRIS(), nTimes);
        m_label->UpdateRASImage( m_imageData, m_layerSource->GetSourceVolume() );
        m_layerMappedSurface->UpdateOverlay(true, true);
    }
}

