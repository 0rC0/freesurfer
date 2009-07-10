/**
 * @file  DialogLoadVolume.h
 * @brief Dialog to load volume data.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2009/07/10 19:40:19 $
 *    $Revision: 1.15 $
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



#include "DialogLoadVolume.h"
#include <wx/xrc/xmlres.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include "MyUtils.h"

BEGIN_EVENT_TABLE( DialogLoadVolume, wxDialog )
  EVT_BUTTON    ( wxID_OK,                        DialogLoadVolume::OnOK )
  EVT_BUTTON    ( XRCID( "ID_BUTTON_FILE" ),      DialogLoadVolume::OnButtonOpen )
  EVT_COMBOBOX  ( XRCID( "ID_COMBO_FILENAME" ),   DialogLoadVolume::OnFileSelectionChanged )
  EVT_BUTTON    ( XRCID( "ID_BUTTON_REG_FILE" ),  DialogLoadVolume::OnButtonRegFile )
  EVT_CHECKBOX  ( XRCID( "ID_CHECK_APPLY_REG" ),  DialogLoadVolume::OnCheckApplyReg )
END_EVENT_TABLE()


DialogLoadVolume::DialogLoadVolume( wxWindow* parent, bool bEnableResample )
{
  wxXmlResource::Get()->LoadDialog( this, parent, 
				    wxT("ID_DIALOG_LOAD_VOLUME") );
  m_checkResample = XRCCTRL( *this, "ID_CHECK_RESAMPLE", wxCheckBox );
  m_checkResample->Show( bEnableResample );
  m_btnOpen       = XRCCTRL( *this, "ID_BUTTON_FILE", wxButton );
  m_comboFileName = XRCCTRL( *this, "ID_COMBO_FILENAME", wxComboBox );
  m_comboFileName->SetFocus();
  m_checkApplyReg = XRCCTRL( *this, "ID_CHECK_APPLY_REG", wxCheckBox );
  m_textRegFile   = XRCCTRL( *this, "ID_TEXT_REG_FILE", wxTextCtrl );
  m_btnRegFile    = XRCCTRL( *this, "ID_BUTTON_REG_FILE", wxButton );
  m_radioNearest  = XRCCTRL( *this, "ID_RADIO_NEAREST", wxRadioButton );
  m_radioTrilinear  = XRCCTRL( *this, "ID_RADIO_TRILINEAR", wxRadioButton );
}

DialogLoadVolume::~DialogLoadVolume()
{}

wxArrayString DialogLoadVolume::GetVolumeFileNames()
{
  return MyUtils::SplitString(m_comboFileName->GetValue(), _(";") );
}

wxString DialogLoadVolume::GetRegFileName()
{
  if ( m_checkApplyReg->IsChecked() )
    return m_textRegFile->GetValue().Trim( true ).Trim( false );
  else
    return _("");
}

void DialogLoadVolume::OnOK( wxCommandEvent& event )
{
  if ( GetVolumeFileNames().IsEmpty())
  {
    wxMessageDialog dlg( this, 
			 _("Volume file names cannot be empty."), 
			 _("Error"), 
			 wxOK | wxICON_ERROR );
    dlg.ShowModal();
    return;
  }
  else if ( m_checkApplyReg->IsChecked() && 
	    m_textRegFile->GetValue().Trim( true ).Trim( false ).IsEmpty() )
  {
    wxMessageDialog dlg( this, 
			 _("Registration file name cannot be empty."), 
			 _("Error"), 
			 wxOK | wxICON_ERROR );
    dlg.ShowModal();
    return;
  }

  event.Skip();
}

bool DialogLoadVolume::IsToResample()
{
  return m_checkResample->IsChecked();
}

int DialogLoadVolume::GetSampleMethod()
{
  if ( m_radioNearest->GetValue() )
    return 0;         
  else
    return 1;
} 

void DialogLoadVolume::SetRecentFiles( const wxArrayString& list )
{
  m_comboFileName->Clear();
  for ( int i = 0; i < (int)list.GetCount(); i++ )
  {
    m_comboFileName->Append( list[i] );
  }
  if ( list.GetCount() > 0 )
  {
    m_comboFileName->SetSelection( 0 );
    m_comboFileName->SetInsertionPointEnd();
  }
}

void DialogLoadVolume::OnButtonOpen( wxCommandEvent& event )
{
  wxFileDialog dlg
    ( this, 
      _("Open volume file"), 
      m_strLastDir, _(""),
      _("Volume files (*.nii;*.nii.gz;*.img;*.mgz)|*.nii;*.nii.gz;*.img;*.mgz|All files (*.*)|*.*"),
      wxFD_OPEN | wxFD_MULTIPLE );
  if ( dlg.ShowModal() == wxID_OK )
  {
    wxArrayString fns;
    dlg.GetPaths( fns );
    wxString text;
    for ( size_t i = 0; i < fns.GetCount(); i++ )
    {
      text += fns[i];
      if ( i != fns.GetCount()-1 )
        text += _(";");
    }
    m_comboFileName->SetValue( text );
    m_comboFileName->SetInsertionPointEnd();
    m_strLastDir = wxFileName( dlg.GetPath() ).GetPath();
  }
}


void DialogLoadVolume::OnButtonRegFile( wxCommandEvent& event )
{
  wxArrayString fns = GetVolumeFileNames();
  if ( !fns.IsEmpty() )
    m_strLastDir = wxFileName( fns[0] ).GetPath();
  wxFileDialog dlg
    ( this, 
      _("Open registration file"), 
      m_strLastDir, _(""),
      _("Registration files (*.dat;*.xfm;*.lta;*.mat)|*.dat;*.xfm;*.lta;*.mat|All files (*.*)|*.*"),
      wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    m_textRegFile->SetValue( dlg.GetPath() );
    m_textRegFile->SetInsertionPointEnd();
    // m_strLastDir = wxFileName( dlg.GetPath() ).GetPath();
  }
}

void DialogLoadVolume::OnFileSelectionChanged( wxCommandEvent& event )
{
  m_comboFileName->SetInsertionPointEnd();
}

void DialogLoadVolume::OnCheckApplyReg( wxCommandEvent& event )
{
  m_textRegFile->Enable( event.IsChecked() );
  m_btnRegFile->Enable( event.IsChecked() );
}
