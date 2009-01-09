/**
 * @file  LayerSurface.h
 * @brief Layer data object for MRI volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2009/01/09 20:11:07 $
 *    $Revision: 1.10 $
 *
 * Copyright (C) 2002-2009,
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
 
#ifndef LayerSurface_h
#define LayerSurface_h

#include "Layer.h"
#include "vtkSmartPointer.h"
#include <string>	

class vtkImageReslice;
class vtkImageMapToColors;
class vtkTransform;
class vtkTexture;
class vtkPolyDataMapper;
class vtkActor;
class vtkLODActor;
class vtkImageActor;
class vtkImageData;
class vtkPlane;
class vtkDecimatePro;
class vtkProp;
class LayerPropertiesSurface;
class FSSurface;
class wxWindow;
class wxCommandEvent;
class LayerMRI;

class LayerSurface : public Layer
{
	public:
		LayerSurface( LayerMRI* mri = NULL );
		virtual ~LayerSurface();
					
		bool LoadSurfaceFromFile( wxWindow* wnd, wxCommandEvent& event );
		
		void Append2DProps( vtkRenderer* renderer, int nPlane );
		void Append3DProps( vtkRenderer* renderer, bool* bSliceVisibility = NULL );
		bool HasProp( vtkProp* prop );
		
		void SetSlicePositionToWorldCenter();
		
		LayerPropertiesSurface* GetProperties();
		
		virtual void DoListenToMessage ( std::string const iMessage, void* const iData );
		
		virtual void SetVisible( bool bVisible = true );
		virtual bool IsVisible();
		
		int GetVertexIndexAtRAS( double* ras, double* distance );
		
		int GetVertexIndexAtTarget( double* ras, double* distance );		

		bool GetRASAtVertex( int nVertex, double* ras );

		bool GetTargetAtVertex( int nVertex, double* ras );
		
		FSSurface* GetSourceSurface()
			{ return m_surfaceSource; }
		
		const char* GetFileName()
			{ return m_sFilename.c_str(); }
		
		void SetFileName( const char* fn )
			{ m_sFilename = fn; } 
		
		void SetVectorFileName( const char* fn )
			{ m_sVectorFilename = fn; }
		
		int GetActiveSurface();
		
		void SetActiveSurface( int nSurfaceType );
		
	protected:
		void InitializeSurface();
		void InitializeActors();		
		void UpdateOpacity();
		void UpdateColorMap();
		void UpdateEdgeThickness();
		
		virtual void OnSlicePositionChanged( int nPlane );	
		
		LayerPropertiesSurface* 				mProperties;
		// Pipeline ------------------------------------------------------------
		vtkSmartPointer<vtkPlane> 				mReslicePlane[3];
		vtkSmartPointer<vtkImageMapToColors> 	mColorMap[3];
		
		vtkSmartPointer<vtkDecimatePro>			mLowResFilter;
		vtkSmartPointer<vtkDecimatePro>			mMediumResFilter;
		
		FSSurface*			m_surfaceSource;
		bool				m_bResampleToRAS;
		LayerMRI*			m_volumeRef;
		
		std::string			m_sFilename;
		std::string			m_sVectorFilename;
				
		vtkActor*			m_sliceActor2D[3];
		vtkActor*			m_sliceActor3D[3];
	//	vtkLODActor*		m_mainActor;
		vtkActor*			m_mainActor;
		vtkActor*			m_vectorActor;
};

#endif 


