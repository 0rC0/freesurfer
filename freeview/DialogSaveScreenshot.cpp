/**
 * @file  DialogSaveScreenshot.h
 * @brief Dialog to load DTI data.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2009/12/08 22:21:21 $
 *    $Revision: 1.2 $
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



#include "DialogSaveScreenshot.h"
#include <wx/xrc/xmlres.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/spinctrl.h>
#include "MainWindow.h"

BEGIN_EVENT_TABLE( DialogSaveScreenshot, wxDialog )
  EVT_BUTTON    ( wxID_SAVE,  DialogSaveScreenshot::OnOK )
  EVT_BUTTON    ( XRCID( "ID_BUTTON_FILENAME" ),    DialogSaveScreenshot::OnButtonOpen )
END_EVENT_TABLE()


DialogSaveScreenshot::DialogSaveScreenshot( wxWindow* parent )
{
  wxXmlResource::Get()->LoadDialog( this, parent, wxT("ID_DIALOG_SAVE_SCREENSHOT") );
  m_textFilename        = XRCCTRL( *this, "ID_TEXT_FILENAME",           wxTextCtrl );
  m_btnOpen             = XRCCTRL( *this, "ID_BUTTON_FILENAME",         wxButton );
  m_checkAntiAliasing   = XRCCTRL( *this, "ID_CHECK_ANTIALIASING",      wxCheckBox );
  m_checkHideCursor     = XRCCTRL( *this, "ID_CHECK_HIDE_CURSOR",       wxCheckBox );
  m_checkHideCoords     = XRCCTRL( *this, "ID_CHECK_HIDE_COORDINATES",  wxCheckBox );
  m_checkKeepWindow     = XRCCTRL( *this, "ID_CHECK_KEEP_WINDOW",       wxCheckBox );
  m_spinMagnification   = XRCCTRL( *this, "ID_SPINBOX_MAGNIFICATION",   wxSpinCtrl );
  
  m_textFilename->SetFocus();
}

DialogSaveScreenshot::~DialogSaveScreenshot()
{}

wxString DialogSaveScreenshot::GetFileName()
{
  return m_textFilename->GetValue().Trim( true ).Trim( false );
}

void DialogSaveScreenshot::SetFileName( const wxString& filename )
{
  m_textFilename->ChangeValue( filename );
}

void DialogSaveScreenshot::OnOK( wxCommandEvent& event )
{
  if ( GetFileName().IsEmpty() )
  {
    wxMessageDialog dlg( this, 
	                       _("File name can not be empty."), 
	                       _("Error"), wxOK | wxICON_ERROR );
    dlg.ShowModal();
    return;
  }
  else 
  {
    if ( wxFile::Exists( GetFileName() ) )
    {
      wxMessageDialog dlg( this, 
                          _("File exists. Do you want to overwrite it?"), 
                          _("Warning"), 
                          wxYES_NO | wxNO_DEFAULT );
      if ( dlg.ShowModal() == wxID_NO )
        return;
    }
  }
  
  if ( !MainWindow::GetMainWindowPointer()->SaveScreenshot() || m_checkKeepWindow->IsChecked() )
    return;
  
  Hide();
}

void DialogSaveScreenshot::OnButtonOpen( wxCommandEvent& event )
{
  wxFileDialog dlg( this, _("Save screenshot as"), m_strLastDir, _(""),
                    _("PNG files (*.png)|*.png|JPEG files (*.jpg;*.jpeg)|*.jpg;*.jpeg|TIFF files (*.tif;*.tiff)|*.tif;*.tiff|Bitmap files (*.bmp)|*.bmp|PostScript files (*.ps)|*.ps|VRML files (*.wrl)|*.wrl|All files (*.*)|*.*"),
                    wxFD_SAVE );
  dlg.SetFilterIndex( m_nScreenshotFilterIndex );
  if ( dlg.ShowModal() == wxID_OK )
  {
    m_textFilename->ChangeValue( dlg.GetPath() );
    m_textFilename->ShowPosition( m_textFilename->GetLastPosition() );
    m_nScreenshotFilterIndex = dlg.GetFilterIndex();
  }
}

void DialogSaveScreenshot::SetSettings( SettingsScreenshot s )
{
  m_checkAntiAliasing->SetValue( s.AntiAliasing );
  m_checkHideCursor->SetValue( s.HideCursor );
  m_checkHideCoords->SetValue( s.HideCoords );
  m_spinMagnification->SetValue( s.Magnification );
}
  
SettingsScreenshot DialogSaveScreenshot::GetSettings()
{
  SettingsScreenshot s;
  s.AntiAliasing  = m_checkAntiAliasing->IsChecked();
  s.HideCursor    = m_checkHideCursor->IsChecked();
  s.HideCoords    = m_checkHideCoords->IsChecked();
  s.Magnification = m_spinMagnification->GetValue();
  
  return s;
}


