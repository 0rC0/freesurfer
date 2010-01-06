/**
 * @file  DialogPreferences.h
 * @brief Preferences Dialog.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2010/01/06 02:07:19 $
 *    $Revision: 1.10 $
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
#ifndef DialogPreferences_h
#define DialogPreferences_h

#include <wx/wx.h>


class wxColourPickerCtrl;
class wxCheckBox;
class wxSpinCtrl;
struct SettingsGeneral;
struct Settings2D;
struct SettingsScreenshot;

class DialogPreferences : public wxDialog
{
public:
  DialogPreferences(wxWindow* parent);
  virtual ~DialogPreferences();

  SettingsGeneral GetGeneralSettings();
  void SetGeneralSettings( const SettingsGeneral& s );

  Settings2D Get2DSettings();
  void Set2DSettings( const Settings2D& s );

  SettingsScreenshot GetScreenshotSettings();
  void SetScreenshotSettings( const SettingsScreenshot& s );

  void OnOK( wxCommandEvent& event );

private:
  wxColourPickerCtrl* m_colorPickerBackground;
  wxColourPickerCtrl* m_colorPickerCursor;
  wxChoice*           m_choiceCursorStyle;
  wxCheckBox*         m_checkSaveCopy;
  wxCheckBox*         m_checkSyncZoomFactor;

  wxCheckBox*         m_checkHideCursor;
  wxCheckBox*         m_checkHideCoords;
  wxCheckBox*         m_checkAntiAliasing;
  wxSpinCtrl*         m_spinMagnification;

  DECLARE_EVENT_TABLE()
};

#endif

