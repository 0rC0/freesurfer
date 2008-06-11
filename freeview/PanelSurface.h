/**
 * @file  PanelSurface.h
 * @brief Layer control panel.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2008/06/11 21:30:19 $
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
#ifndef PanelSurface_h
#define PanelSurface_h

#include <wx/wx.h>
#include "Listener.h"
#include "Broadcaster.h"


class wxAuiNotebook;
class wxListBox;
class wxCheckListBox;
class wxColourPickerCtrl;
class wxColourPickerEvent;
class Layer;

class PanelSurface : public wxPanel, public Listener, public Broadcaster
{
public:
	PanelSurface(wxWindow* parent);
	virtual ~PanelSurface();
	
	void UpdateUI( bool bForce = false );
	
protected:
	void OnInternalIdle();
		
private:
	void OnSliderOpacity( wxScrollEvent& event );
	void OnLayerSelectionChanged( wxCommandEvent& event );
	void OnLayerVisibilityChanged( wxCommandEvent& event );
	
	void OnButtonLoad( wxCommandEvent& event );
	void OnButtonMoveUp( wxCommandEvent& event );
	void OnButtonMoveDown( wxCommandEvent& event );
	void OnButtonDelete( wxCommandEvent& event );
	void OnColorChanged( wxColourPickerEvent& event );
	void OnEdgeColorChanged( wxColourPickerEvent& event );
	
	void DoUpdateUI();
	
	void UpdateLayerList( Layer* layer );
				
	virtual void DoListenToMessage( std::string const iMsg, void* iData );
		
	wxCheckListBox*	m_listBoxLayers;
	wxButton*		m_btnMoveUp;
	wxButton*		m_btnMoveDown;
	wxButton*		m_btnNew;
	wxButton*		m_btnLoad;
	wxButton*		m_btnSave;
	wxButton*		m_btnDelete;
	wxSlider*		m_sliderOpacity;	
	wxColourPickerCtrl*		m_colorPicker;
	wxColourPickerCtrl*		m_colorPickerEdge;
	wxTextCtrl*		m_textFileName;

	bool			m_bUINeedUpdate;
    
	DECLARE_EVENT_TABLE()
};

#endif 

