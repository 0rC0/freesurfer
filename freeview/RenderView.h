/**
 * @file  RenderView.h
 * @brief View class for rendering.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2008/08/08 20:13:40 $
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
 
#ifndef RenderView_h
#define RenderView_h

#include "wxVTKRenderWindowInteractor.h"
#include "Listener.h"
#include "Broadcaster.h"

class vtkRenderer;
class vtkRenderWindow;
class vtkActor2D;
class LayerCollection;
class Interactor;

class VTK_RENDERING_EXPORT RenderView : public wxVTKRenderWindowInteractor, public Listener, public Broadcaster
{
  DECLARE_DYNAMIC_CLASS( RenderView )

public:
	RenderView();
    RenderView( wxWindow *parent, int id );
    virtual ~RenderView();	
    
    static RenderView * New();
    void PrintSelf( ostream& os, vtkIndent indent );
    
	void OnSetFocus		( wxFocusEvent& event);
	void OnKillFocus	( wxFocusEvent& event );
	void OnButtonDown	( wxMouseEvent& event );
	void OnButtonUp		( wxMouseEvent& event );
	void OnMouseMove	( wxMouseEvent& event );
	void OnMouseWheel	( wxMouseEvent& event );
	void OnMouseEnter	( wxMouseEvent& event );
	void OnMouseLeave	( wxMouseEvent& event );
	void OnKeyDown		( wxKeyEvent& event );
	void OnKeyUp		( wxKeyEvent& event );
	void OnSize			( wxSizeEvent& event );
		
	void SetWorldCoordinateInfo( const double* origin, const double* size );
	virtual void UpdateViewByWorldCoordinate() {}	
	
	virtual void RefreshAllActors() {}
	
	virtual void TriggerContextMenu( const wxPoint& pos ) {}
	
	virtual void DoListenToMessage( std::string const iMessage, void* const iData );
	
	
	int	GetInteractionMode();
	virtual void SetInteractionMode( int nMode );
	
	int GetAction();
	void SetAction( int nAction );
	
	wxColour GetBackgroundColor() const;
	void SetBackgroundColor( const wxColour& color );
	
	virtual void MoveUp();
	virtual void MoveDown();
	virtual void MoveLeft();
	virtual void MoveRight();
	
	void NeedRedraw();
	
	void ResetView();
	
	vtkRenderer* GetRenderer()
		{ return m_renderer; }
	
	bool SaveScreenshot( const wxString& fn, int nMagnification = 1 );
	
	virtual void PreScreenshot() {}
	virtual void PostScreenshot() {}
	
protected:
	void InitializeRenderView();
	virtual void OnInternalIdle(); 

protected:
	vtkRenderer*		m_renderer;
	vtkRenderWindow*	m_renderWindow;
	vtkActor2D*			m_actorFocusFrame;

	double				m_dWorldOrigin[3];
	double				m_dWorldSize[3];	
	
	Interactor*			m_interactor;
	int					m_nInteractionMode;	
	
	int					m_nRedrawCount;
	
    // any class wishing to process wxWindows events must use this macro
    DECLARE_EVENT_TABLE()

};

#endif 


