/**
 * @file  LayerMRI.h
 * @brief Layer data object for MRI volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2008/03/27 20:38:59 $
 *    $Revision: 1.2 $
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
 
#ifndef LayerMRI_h
#define LayerMRI_h

#include "LayerEditable.h"
#include "vtkSmartPointer.h"
#include <string>

class vtkFSVolumeSource;
class vtkImageReslice;
class vtkImageMapToColors;
class vtkTransform;
class vtkTexture;
class vtkPolyDataMapper;
class vtkActor;
class vtkImageActor;
class vtkImageData;
class LayerPropertiesMRI;
class FSVolume;
class wxWindow;
class wxCommandEvent;

class LayerMRI : public LayerEditable
{
	public:
		LayerMRI();
		virtual ~LayerMRI();
					
//		bool LoadVolumeFromFile( std::string filename );
		bool LoadVolumeFromFile( wxWindow* wnd, wxCommandEvent& event );
		bool Create( LayerMRI* mri, bool bCopyVoxel );
		
		void Append2DProps( vtkRenderer* renderer, int nPlane );
		void Append3DProps( vtkRenderer* renderer );
		
//		void SetSliceNumber( int* sliceNumber );
		void SetSlicePositionToWorldCenter();
		
		virtual double GetVoxelValue( double* pos );
		virtual std::string GetLabelName( double value );
				
		LayerPropertiesMRI* GetProperties();
		
		virtual void DoListenToMessage ( std::string const iMessage, void* const iData );
		
		virtual void SetVisible( bool bVisible = true );
		virtual bool IsVisible();
		
		FSVolume* GetSourceVolume()
			{ return m_volumeSource; }
		
		bool SaveVolume( wxWindow* wnd, wxCommandEvent& event );
		
		void SetResampleToRAS( bool bResample );
		
		bool GetResampleToRAS()
			{ return m_bResampleToRAS; }
		
		void RemapPositionToRealRAS( const double* pos_in, double* pos_out );

		void RemapPositionToRealRAS( double x_in, double y_in, double z_in, 
									 double& x_out, double& y_out, double& z_out );
	protected:
		virtual void SetModified();
		
		void InitializeVolume();
		void InitializeActors();		
		void UpdateOpacity();
		void UpdateResliceInterpolation();
		void UpdateTextureSmoothing();
		virtual void UpdateColorMap();
		
		virtual void OnSlicePositionChanged( int nPlane );	
		
		LayerPropertiesMRI* 					mProperties;
		// Pipeline ------------------------------------------------------------
		vtkSmartPointer<vtkImageReslice> 		mReslice[3];
		vtkSmartPointer<vtkImageMapToColors> 	mColorMap[3];

		FSVolume*			m_volumeSource;
		bool				m_bResampleToRAS;
		
		vtkImageActor*		m_sliceActor2D[3];
		vtkImageActor*		m_sliceActor3D[3];
};

#endif 


