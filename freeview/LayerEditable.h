/**
 * @file  LayerEditable.h
 * @brief Layer data object for MRI volume.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2008/07/18 20:23:24 $
 *    $Revision: 1.5 $
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
 
#ifndef LayerEditable_h
#define LayerEditable_h

#include "Layer.h"
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include <string>
#include <vector>

class vtkImageData;
class BrushProperty;

class LayerEditable : public Layer
{
	public:
		LayerEditable();
		virtual ~LayerEditable();

		void SetVoxelByRAS( double* ras, int nPlane, bool bAdd = true );
		void SetVoxelByRAS( double* ras1, double*ras2, int nPlane, bool bAdd = true );	
		void FloodFillByRAS( double* ras, int nPlane, bool bAdd = true );	
			
		bool HasUndo();
		bool HasRedo();
		
		void Undo();
		void Redo();
		
		void Copy( int nPlane );
		void Paste( int nPlane );
		
		void SaveForUndo( int nPlane );
		
		float GetFillValue();
		void SetFillValue( float fFill );
		
		float GetBlankValue();
		void SetBlankValue( float fBlank );
		
		int GetBrushRadius();
		void SetBrushRadius( int nRadius );
		
		virtual void Append2DProps( vtkRenderer* renderer, int nPlane ) = 0;
		virtual void Append3DProps( vtkRenderer* renderer ) = 0;
		
		virtual void SetVisible( bool bVisible = true ) = 0;
		virtual bool IsVisible() = 0;	
		
		virtual int GetActiveFrame()
			{ return m_nActiveFrame; }
		
		virtual void SetActiveFrame( int nFrame )
			{ m_nActiveFrame = nFrame; }
		
		vtkImageData* GetRASVolume()
			{ return m_volumeRAS; }

		vtkImageData* GetRefVolume()
			{ return m_volumeRef; }
		
		void ResetModified()
			{ m_bModified = false; }
		
		bool IsModified()
			{ return m_bModified; }
		
		const char* GetFileName()
			{ return m_sFilename.c_str(); }
		
		void SetFileName( const char* fn )
			{ m_sFilename = fn; } 
		
		void SetEditable( bool bEditable = true )
			{ m_bEditable = bEditable; }
		
		bool IsEditable()
			{ return m_bEditable; }
		
		void SetBrushProperty( BrushProperty* brush )
			{ m_propertyBrush = brush; }
		
		bool IsValidToPaste( int nPlane );
							
	protected:
		virtual void SetModified();
		
		bool SetVoxelByIndex( int* n, int nPlane, bool bAdd = true );	// true is to add, false is to remove
		bool SetVoxelByIndex( int* n1, int* n2, int nPlane, bool bAdd = true );
		bool FloodFillByIndex( int* n, int nPlane, bool bAdd = true );
		
		bool GetConnectedToOld( vtkImageData* img, int nFrame, int* n, int nPlane );
		
		struct UndoRedoBufferItem	
		{
			int 	plane;
			int 	slice;
			char*	data;
		};			
		
		void SaveBufferItem( UndoRedoBufferItem& item, int nPlane, int nSlice );
		void LoadBufferItem( UndoRedoBufferItem& item );
					
		vtkSmartPointer<vtkImageData>	m_volumeRAS;
		vtkSmartPointer<vtkImageData>	m_volumeRef;
		
		float			m_fFillValue;
		float			m_fBlankValue;
		
		std::vector<UndoRedoBufferItem>		m_bufferUndo;
		std::vector<UndoRedoBufferItem>		m_bufferRedo;
		UndoRedoBufferItem				m_bufferClipboard;
		
		int			m_nBrushRadius;
		
		int			m_nMaxUndoSteps;		
		bool		m_bModified;
		
		bool 		m_bEditable;
		BrushProperty*		m_propertyBrush;
		
		int					m_nActiveFrame;
		
		std::string			m_sFilename;
};

#endif 


