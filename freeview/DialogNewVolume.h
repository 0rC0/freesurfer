/**
 * @file  DialogNewVolume.h
 * @brief Preferences Dialog.
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
#ifndef DialogNewVolume_h
#define DialogNewVolume_h

#include <wx/wx.h>


class wxCheckBox;
class wxChoice;
class wxTextCtrl;
class LayerMRI;
class LayerCollection;

class DialogNewVolume : public wxDialog
{
public:
	DialogNewVolume( wxWindow* parent, LayerCollection* col );
	virtual ~DialogNewVolume();
	
	wxString GetVolumeName();
	void SetVolumeName( const wxString& name );
	
	bool GetCopyVoxel();
	void SetCopyVoxel( bool bVoxel );
	
	LayerMRI* GetTemplate();
	
	void OnOK( wxCommandEvent& event );
	
private:
	wxCheckBox*		m_checkCopyVoxel;
	wxChoice*		m_choiceTemplate;
	wxTextCtrl*		m_textName;
	
	DECLARE_EVENT_TABLE()
};

#endif 

