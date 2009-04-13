/**
 * @file  MainWindow.cpp
 * @brief Main window.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: rpwang $
 *    $Date: 2009/04/13 21:31:32 $
 *    $Revision: 1.46 $
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

#include <wx/xrc/xmlres.h>
#include <wx/config.h>
#include <wx/image.h>
#include <wx/sashwin.h>
#include <wx/laywin.h>
#include <wx/filedlg.h>
#include <wx/progdlg.h>
#include <wx/msgdlg.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/app.h>
#include <wx/docview.h>
#include <wx/aui/auibook.h>
#include <wx/msgdlg.h>
#include <wx/textdlg.h>
#include <wx/filename.h>
#include <wx/spinctrl.h>

#include "MainWindow.h"
#include "ControlPanel.h"
#include "PixelInfoPanel.h"
#include "RenderView2D.h"
#include "RenderView3D.h"
#include "LayerCollection.h"
#include "LayerMRI.h"
#include "LayerPropertiesMRI.h"
#include "LayerPropertiesWayPoints.h"
#include "LayerROI.h"
#include "LayerDTI.h"
#include "LayerSurface.h"
#include "LayerWayPoints.h"
#include "LayerOptimal.h"
#include "FSSurface.h"
#include "Interactor2DROIEdit.h"
#include "Interactor2DVoxelEdit.h"
#include "DialogPreferences.h"
#include "DialogLoadDTI.h"
#include "WindowQuickReference.h"
#include "StatusBar.h"
#include "DialogNewVolume.h"
#include "DialogNewROI.h"
#include "DialogNewWayPoints.h"
#include "DialogLoadVolume.h"
#include "WorkerThread.h"
#include "MyUtils.h"
#include "LUTDataHolder.h"
#include "BrushProperty.h"
#include "Cursor2D.h"
#include "Cursor3D.h"
#include "ToolWindowEdit.h"
#include "DialogRotateVolume.h"
#include "DialogOptimalVolume.h"
#include "WindowHistogram.h"
#include "WindowOverlayConfiguration.h"
#include "SurfaceOverlay.h"
#include "SurfaceOverlayProperties.h"

#define CTRL_PANEL_WIDTH 240

#define ID_FILE_RECENT1  10001

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------
IMPLEMENT_CLASS(MainWindow, wxFrame)
BEGIN_EVENT_TABLE(MainWindow, wxFrame)
  EVT_MENU        ( XRCID( "ID_FILE_OPEN" ),              MainWindow::OnFileOpen )
  EVT_MENU        ( XRCID( "ID_FILE_NEW" ),               MainWindow::OnFileNew )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_NEW" ),               MainWindow::OnFileNewUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SAVE" ),              MainWindow::OnFileSave )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SAVE" ),              MainWindow::OnFileSaveUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SAVE_AS" ),           MainWindow::OnFileSaveAs )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SAVE_AS" ),           MainWindow::OnFileSaveAsUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_EXIT") ,              MainWindow::OnFileExit )
  EVT_MENU_RANGE  ( wxID_FILE1, wxID_FILE1+16,            MainWindow::OnFileRecent )
  EVT_MENU        ( XRCID( "ID_FILE_NEW_ROI" ),           MainWindow::OnFileNewROI )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_NEW_ROI" ),           MainWindow::OnFileNewROIUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_LOAD_ROI" ),          MainWindow::OnFileLoadROI )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_LOAD_ROI" ),          MainWindow::OnFileLoadROIUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SAVE_ROI" ),          MainWindow::OnFileSaveROI )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SAVE_ROI" ),          MainWindow::OnFileSaveROIUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SAVE_ROI_AS" ),       MainWindow::OnFileSaveROIAs )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SAVE_ROI_AS" ),       MainWindow::OnFileSaveROIAsUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_LOAD_DTI" ),          MainWindow::OnFileLoadDTI )
  EVT_MENU        ( XRCID( "ID_FILE_NEW_WAYPOINTS" ),     MainWindow::OnFileNewWayPoints )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_NEW_WAYPOINTS" ),     MainWindow::OnFileNewWayPointsUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_LOAD_WAYPOINTS" ),    MainWindow::OnFileLoadWayPoints )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_LOAD_WAYPOINTS" ),    MainWindow::OnFileLoadWayPointsUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SAVE_WAYPOINTS" ),    MainWindow::OnFileSaveWayPoints )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SAVE_WAYPOINTS" ),    MainWindow::OnFileSaveWayPointsUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SAVE_WAYPOINTS_AS" ), MainWindow::OnFileSaveWayPointsAs )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SAVE_WAYPOINTS_AS" ), MainWindow::OnFileSaveWayPointsAsUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_SCREENSHOT" ),        MainWindow::OnFileSaveScreenshot )
  EVT_UPDATE_UI   ( XRCID( "ID_FILE_SCREENSHOT" ),        MainWindow::OnFileSaveScreenshotUpdateUI )
  EVT_MENU        ( XRCID( "ID_FILE_LOAD_SURFACE" ),      MainWindow::OnFileLoadSurface )
  EVT_MENU        ( XRCID( "ID_MODE_NAVIGATE" ),          MainWindow::OnModeNavigate )
  EVT_UPDATE_UI   ( XRCID( "ID_MODE_NAVIGATE" ),          MainWindow::OnModeNavigateUpdateUI )
  EVT_MENU        ( XRCID( "ID_MODE_VOXEL_EDIT" ),        MainWindow::OnModeVoxelEdit )
  EVT_UPDATE_UI   ( XRCID( "ID_MODE_VOXEL_EDIT" ),        MainWindow::OnModeVoxelEditUpdateUI )
  EVT_MENU        ( XRCID( "ID_MODE_ROI_EDIT" ),          MainWindow::OnModeROIEdit )
  EVT_UPDATE_UI   ( XRCID( "ID_MODE_ROI_EDIT" ),          MainWindow::OnModeROIEditUpdateUI )
  EVT_MENU        ( XRCID( "ID_MODE_WAYPOINTS_EDIT" ),    MainWindow::OnModeWayPointsEdit )
  EVT_UPDATE_UI   ( XRCID( "ID_MODE_WAYPOINTS_EDIT" ),    MainWindow::OnModeWayPointsEditUpdateUI )
  EVT_MENU        ( XRCID( "ID_EDIT_COPY" ),              MainWindow::OnEditCopy )
  EVT_UPDATE_UI   ( XRCID( "ID_EDIT_COPY" ),              MainWindow::OnEditCopyUpdateUI )
  EVT_MENU        ( XRCID( "ID_EDIT_PASTE" ),             MainWindow::OnEditPaste )
  EVT_UPDATE_UI   ( XRCID( "ID_EDIT_PASTE" ),             MainWindow::OnEditPasteUpdateUI )
  EVT_MENU        ( XRCID( "ID_EDIT_UNDO" ),              MainWindow::OnEditUndo )
  EVT_UPDATE_UI   ( XRCID( "ID_EDIT_UNDO" ),              MainWindow::OnEditUndoUpdateUI )
  EVT_MENU        ( XRCID( "ID_EDIT_REDO" ),              MainWindow::OnEditRedo )
  EVT_UPDATE_UI   ( XRCID( "ID_EDIT_REDO" ),              MainWindow::OnEditRedoUpdateUI )
  EVT_MENU        ( XRCID( "ID_EDIT_PREFERENCES" ),       MainWindow::OnEditPreferences )
  EVT_MENU        ( XRCID( "ID_VIEW_LAYOUT_1X1" ),        MainWindow::OnViewLayout1X1 )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_LAYOUT_1X1" ),        MainWindow::OnViewLayout1X1UpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_LAYOUT_2X2" ),        MainWindow::OnViewLayout2X2 )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_LAYOUT_2X2" ),        MainWindow::OnViewLayout2X2UpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_LAYOUT_1N3" ),        MainWindow::OnViewLayout1N3 )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_LAYOUT_1N3" ),        MainWindow::OnViewLayout1N3UpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_LAYOUT_1N3_H" ),      MainWindow::OnViewLayout1N3_H )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_LAYOUT_1N3_H" ),      MainWindow::OnViewLayout1N3_HUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_SAGITTAL" ),          MainWindow::OnViewSagittal )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SAGITTAL" ),          MainWindow::OnViewSagittalUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_CORONAL" ),           MainWindow::OnViewCoronal )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_CORONAL" ),           MainWindow::OnViewCoronalUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_AXIAL" ),             MainWindow::OnViewAxial )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_AXIAL" ),             MainWindow::OnViewAxialUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_3D" ),                MainWindow::OnView3D )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_3D" ),                MainWindow::OnView3DUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_RESET" ),             MainWindow::OnViewReset )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_RESET" ),             MainWindow::OnViewResetUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_SCALAR_BAR" ),        MainWindow::OnViewScalarBar )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SCALAR_BAR" ),        MainWindow::OnViewScalarBarUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_COORDINATE" ),        MainWindow::OnViewCoordinate )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_COORDINATE" ),        MainWindow::OnViewCoordinateUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_CYCLE_LAYER" ),       MainWindow::OnViewCycleLayer )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_CYCLE_LAYER" ),       MainWindow::OnViewCycleLayerUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_TOGGLE_VOLUME" ),     MainWindow::OnViewToggleVolumeVisibility )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_TOGGLE_VOLUME" ),     MainWindow::OnViewToggleVolumeVisibilityUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_TOGGLE_ROI" ),        MainWindow::OnViewToggleROIVisibility )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_TOGGLE_ROI" ),        MainWindow::OnViewToggleROIVisibilityUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_TOGGLE_SURFACE" ),    MainWindow::OnViewToggleSurfaceVisibility )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_TOGGLE_SURFACE" ),    MainWindow::OnViewToggleSurfaceVisibilityUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_TOGGLE_WAYPOINTS" ),  MainWindow::OnViewToggleWayPointsVisibility )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_TOGGLE_WAYPOINTS" ),  MainWindow::OnViewToggleWayPointsVisibilityUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_TOGGLE_CURSOR" ),     MainWindow::OnViewToggleCursorVisibility )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_TOGGLE_CURSOR" ),     MainWindow::OnViewToggleCursorVisibilityUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_TOGGLE_VOXEL_COORDS" ), MainWindow::OnViewToggleVoxelCoordinates )
  
  EVT_MENU        ( XRCID( "ID_VIEW_SURFACE_MAIN" ),      MainWindow::OnViewSurfaceMain )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SURFACE_MAIN" ),      MainWindow::OnViewSurfaceMainUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_SURFACE_INFLATED" ),  MainWindow::OnViewSurfaceInflated )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SURFACE_INFLATED" ),  MainWindow::OnViewSurfaceInflatedUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_SURFACE_WHITE" ),     MainWindow::OnViewSurfaceWhite )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SURFACE_WHITE" ),     MainWindow::OnViewSurfaceWhiteUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_SURFACE_PIAL" ),      MainWindow::OnViewSurfacePial )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SURFACE_PIAL" ),      MainWindow::OnViewSurfacePialUpdateUI )
  EVT_MENU        ( XRCID( "ID_VIEW_SURFACE_ORIGINAL" ),  MainWindow::OnViewSurfaceOriginal )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_SURFACE_ORIGINAL" ),  MainWindow::OnViewSurfaceOriginalUpdateUI )
  
  EVT_MENU        ( XRCID( "ID_VIEW_CONTROL_PANEL" ),     MainWindow::OnViewControlPanel )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_CONTROL_PANEL" ),     MainWindow::OnViewControlPanelUpdateUI )
  
  EVT_MENU        ( XRCID( "ID_VIEW_HISTOGRAM" ),         MainWindow::OnViewHistogram )
  EVT_UPDATE_UI   ( XRCID( "ID_VIEW_HISTOGRAM" ),         MainWindow::OnViewHistogramUpdateUI )
  
  EVT_MENU        ( XRCID( "ID_TOOL_ROTATE_VOLUME" ),     MainWindow::OnToolRotateVolume )
  EVT_UPDATE_UI   ( XRCID( "ID_TOOL_ROTATE_VOLUME" ),     MainWindow::OnToolRotateVolumeUpdateUI )
  EVT_MENU        ( XRCID( "ID_TOOL_OPTIMAL_VOLUME" ),    MainWindow::OnToolCreateOptimalVolume )
  EVT_UPDATE_UI   ( XRCID( "ID_TOOL_OPTIMAL_VOLUME" ),    MainWindow::OnToolCreateOptimalVolumeUpdateUI )
  
  EVT_MENU  ( XRCID( "ID_HELP_QUICK_REF" ),               MainWindow::OnHelpQuickReference )
  EVT_MENU  ( XRCID( "ID_HELP_ABOUT" ),                   MainWindow::OnHelpAbout )
  /*
  EVT_SASH_DRAGGED_RANGE(ID_LOG_WINDOW, ID_LOG_WINDOW, MainWindow::OnSashDrag)
  EVT_IDLE  (MainWindow::OnIdle)
  EVT_MENU  (XRCID("ID_EVENT_LOAD_DATA"), MainWindow::OnWorkerEventLoadData)
  EVT_MENU  (XRCID("ID_EVENT_CALCULATE_MATRIX"), MainWindow::OnWorkerEventCalculateMatrix)
  */
  
  EVT_MENU      ( ID_WORKER_THREAD,                   MainWindow::OnWorkerThreadResponse )
  EVT_SPINCTRL  ( XRCID( "ID_SPIN_BRUSH_SIZE" ),      MainWindow::OnSpinBrushSize )
  EVT_SPINCTRL  ( XRCID( "ID_SPIN_BRUSH_TOLERANCE" ), MainWindow::OnSpinBrushTolerance )
  EVT_CHECKBOX  ( XRCID( "ID_CHECK_TEMPLATE" ),       MainWindow::OnCheckBrushTemplate )
  EVT_CHOICE    ( XRCID( "ID_CHOICE_TEMPLATE" ),      MainWindow::OnChoiceBrushTemplate )
  
  EVT_ENTER_WINDOW  ( MainWindow::OnMouseEnterWindow )
  EVT_ACTIVATE      ( MainWindow::OnActivate )
  EVT_CLOSE         ( MainWindow::OnClose )
  EVT_KEY_DOWN      ( MainWindow::OnKeyDown )
  EVT_ICONIZE       ( MainWindow::OnIconize )
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MainWindow::MainWindow() : Listener( "MainWindow" ), Broadcaster( "MainWindow" )
{
// m_bLoading = false;
// m_bSaving = false;
  m_bProcessing = false;
  m_bResampleToRAS = true;
  m_bToUpdateToolbars = false;
  m_layerVolumeRef = NULL;
  m_nPrevActiveViewId = -1;
  m_luts = new LUTDataHolder();
  m_propertyBrush = new BrushProperty();

  wxXmlResource::Get()->LoadFrame( this, NULL, wxT("ID_MAIN_WINDOW") );

  // must be created before the following controls
  m_layerCollectionManager = new LayerCollectionManager();

  m_panelToolbarHolder = XRCCTRL( *this, "ID_PANEL_HOLDER", wxPanel );
// wxBoxSizer* sizer = (wxBoxSizer*)panelHolder->GetSizer(); //new wxBoxSizer( wxVERTICAL );

  // create the main splitter window
// m_splitterMain = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );
  m_splitterMain = XRCCTRL( *this, "ID_SPLITTER_MAIN", wxSplitterWindow );
  m_splitterMain->SetMinimumPaneSize( 80 );
// sizer->Add( m_splitterMain, 1, wxEXPAND );

  m_toolbarVoxelEdit = XRCCTRL( *this, "ID_TOOLBAR_VOXEL_EDIT", wxToolBar );
  m_toolbarROIEdit = XRCCTRL( *this, "ID_TOOLBAR_ROI_EDIT", wxToolBar );
  m_toolbarBrush = XRCCTRL( *this, "ID_TOOLBAR_BRUSH", wxToolBar );
  m_toolbarBrush->Show( false );
  m_toolbarVoxelEdit->Show( false );
  m_toolbarROIEdit->Show( false );

// this->SetSizer( sizer );
// sizer->Add( ( wxToolBar* )XRCCTRL( *this, "m_toolBar2", wxToolBar ), 0, wxEXPAND );

  // create the main control panel
  m_controlPanel = new ControlPanel( m_splitterMain );
  m_splitterSub = new wxSplitterWindow( m_splitterMain, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );

  m_splitterMain->SplitVertically( m_controlPanel, m_splitterSub );

  // create the panel holder for the 4 render views
  m_renderViewHolder = new wxPanel( m_splitterSub, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
  m_pixelInfoPanel = new PixelInfoPanel( m_splitterSub );

  m_splitterSub->SplitHorizontally( m_renderViewHolder, m_pixelInfoPanel );
  m_splitterSub->SetMinimumPaneSize( 80 );
  m_splitterSub->SetSashGravity( 1 );

  // create 4 render views
  m_viewCoronal = new RenderView2D( m_renderViewHolder, wxID_ANY );
  m_viewSagittal = new RenderView2D( m_renderViewHolder, wxID_ANY );
  m_viewAxial = new RenderView2D( m_renderViewHolder, wxID_ANY );
  m_view3D = new RenderView3D( m_renderViewHolder, wxID_ANY );
  m_viewSagittal->SetViewPlane( 0 );
  m_viewCoronal->SetViewPlane( 1 );
  m_viewAxial->SetViewPlane( 2 );
  m_viewSagittal->SetFocusFrameColor( 1, 0, 0 );
  m_viewCoronal->SetFocusFrameColor( 0, 1, 0 );
  m_viewAxial->SetFocusFrameColor( 0, 0, 1 );
  m_viewRender[0] = m_viewSagittal;
  m_viewRender[1] = m_viewCoronal;
  m_viewRender[2] = m_viewAxial;
  m_viewRender[3] = m_view3D;
  for ( int i = 0; i < 3; i++ )
  {
    // m_viewRender[i]->Render();
    for ( int j = 0; j < 3; j++ )
    {
      if ( i != j )
        m_viewRender[i]->AddListener( m_viewRender[j] );
    }
  }

  m_layerCollectionManager->AddListener( m_viewAxial );
  m_layerCollectionManager->AddListener( m_viewSagittal );
  m_layerCollectionManager->AddListener( m_viewCoronal );
  m_layerCollectionManager->AddListener( m_view3D );
  m_layerCollectionManager->GetLayerCollection( "MRI" )->AddListener( m_pixelInfoPanel );
  m_layerCollectionManager->GetLayerCollection( "Surface" )->AddListener( m_pixelInfoPanel );
  m_layerCollectionManager->AddListener( this );

  m_wndQuickReference = new WindowQuickReference( this );
  m_wndQuickReference->Hide();

  m_statusBar = new StatusBar( this );
  SetStatusBar( m_statusBar );
  PositionStatusBar();

  m_toolWindowEdit = NULL;
  m_dlgRotateVolume = NULL;

  m_wndHistogram = new WindowHistogram( this );
  m_wndHistogram->Hide();
  
  m_wndOverlayConfiguration = new WindowOverlayConfiguration( this );
  m_wndOverlayConfiguration->Hide();

  UpdateToolbars();
  
  m_nViewLayout = VL_2X2;
  m_nMainView = MV_Sagittal;
  m_nMaxRecentFiles = 10;
  wxConfigBase* config = wxConfigBase::Get();
  m_fileHistory = new wxFileHistory( m_nMaxRecentFiles, wxID_FILE1 );
  wxMenu* fileMenu = GetMenuBar()->GetMenu( 0 )->FindItem( XRCID("ID_FILE_SUBMENU_RECENT") )->GetSubMenu();
  if ( config )
  {
    int x = config->Read( _T("/MainWindow/PosX"), 50L );
    int y = config->Read( _T("/MainWindow/PosY"), 50L );
    int w = config->Read( _T("/MainWindow/Width"), 950L );
    int h = config->Read( _T("/MainWindow/Height"), 750L );
    SetSize( x, y, w, h );
    m_splitterMain->SetSashPosition( config->Read( _T("/MainWindow/SplitterPosition"), 260L ) );
    // m_splitterSub->SetSashPosition( config->Read( _T("/MainWindow/SplitterPositionSub"), 50L ) );
    // m_splitterSub->UpdateSize();
    m_strLastDir = config->Read( _T("/MainWindow/LastDir"), "");
    m_fileHistory->Load( *config );
    m_fileHistory->UseMenu( fileMenu );
    m_fileHistory->AddFilesToMenu( fileMenu );
    m_nViewLayout = config->Read( _T("/MainWindow/ViewLayout"), m_nViewLayout );
    m_nMainView = config->Read( _T("/MainWindow/MainView"), m_nMainView );

    wxColour color = wxColour( config->Read( _T("RenderWindow/BackgroundColor"), "rgb(0,0,0)" ) );
    m_viewAxial->SetBackgroundColor( color );
    m_viewSagittal->SetBackgroundColor( color );
    m_viewCoronal->SetBackgroundColor( color );
    m_view3D->SetBackgroundColor( color );

    color = wxColour( config->Read( _T("RenderWindow/CursorColor"), "rgb(255,0,0)" ) );
    m_viewAxial->GetCursor2D()->SetColor( color );
    m_viewSagittal->GetCursor2D()->SetColor( color );
    m_viewCoronal->GetCursor2D()->SetColor( color );
    m_view3D->GetCursor3D()->SetColor( color );

    m_nScreenshotFilterIndex = config->Read( _T("/MainWindow/ScreenshotFilterIndex"), 0L );

    bool bScalarBar;
    config->Read( _T("/RenderWindow/ShowScalarBar"), &bScalarBar, false );
    if ( bScalarBar )
      ShowScalarBar( bScalarBar );
    
    config->Read( _T("/RenderWindow/SyncZoomFactor"), &m_settings2D.SyncZoomFactor, true );
    config->Read( _T("/Screenshot/Magnification" ), &m_settingsScreenshot.Magnification, 1 );
    config->Read( _T("/Screenshot/HideCursor" ), &m_settingsScreenshot.HideCursor, false );
    config->Read( _T("/Screenshot/HideCoords" ), &m_settingsScreenshot.HideCoords, false );
    config->Read( _T("/Screenshot/AntiAliasing" ), &m_settingsScreenshot.AntiAliasing, false );

    bool bShow = true;
    config->Read( _T("/MainWindow/ShowControlPanel" ), &bShow, true );

    ShowControlPanel( bShow );
  }
  SetViewLayout( m_nViewLayout );
}

// frame destructor
MainWindow::~MainWindow()
{
  m_viewAxial->Delete();
  m_viewSagittal->Delete();
  m_viewCoronal->Delete();
  m_view3D->Delete();

  delete m_layerCollectionManager;
  delete m_luts;

  delete m_propertyBrush;
}

MainWindow* MainWindow::GetMainWindowPointer()
{
  return wxDynamicCast( wxTheApp->GetTopWindow(), MainWindow );
}

void MainWindow::OnClose( wxCloseEvent &event )
{
  if ( IsProcessing() )
  {
    wxMessageDialog dlg( this, "There is on-going data processing. Please wait till it finishes before closing.", "Quit", wxOK );
    dlg.ShowModal();
    return;
  }

  LayerCollection* lc_mri = GetLayerCollection( "MRI" );
  LayerCollection* lc_roi = GetLayerCollection( "ROI" );
  LayerCollection* lc_wp = GetLayerCollection( "WayPoints" );
  wxString text = "";
  for ( int i = 0; i < lc_mri->GetNumberOfLayers(); i++ )
  {
    LayerEditable* layer = ( LayerEditable* )lc_mri->GetLayer( i );
    if ( layer->IsModified() )
      text += wxString( layer->GetName() ) + "\t(" + wxFileName( layer->GetFileName() ).GetShortPath() + ")\n";
  }
  for ( int i = 0; i < lc_roi->GetNumberOfLayers(); i++ )
  {
    LayerEditable* layer = ( LayerEditable* )lc_roi->GetLayer( i );
    if ( layer->IsModified() )
      text += wxString( layer->GetName() ) + + "\t(" + wxFileName( layer->GetFileName() ).GetShortPath() + ")\n";
  }
  for ( int i = 0; i < lc_wp->GetNumberOfLayers(); i++ )
  {
    LayerEditable* layer = ( LayerEditable* )lc_wp->GetLayer( i );
    if ( layer->IsModified() )
      text += wxString( layer->GetName() ) + + "\t(" + wxFileName( layer->GetFileName() ).GetShortPath() + ")\n";
  }

  if ( !text.IsEmpty() )
  {
    wxString msg = "The following volume(s) and/or label(s) have been modified but not saved. \n\n";
    msg += text + "\nDo you still want to quit the program?";
    wxMessageDialog dlg( this, msg, "Quit", wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT );
    if ( dlg.ShowModal() != wxID_YES )
      return;
  }

  wxConfigBase* config = wxConfigBase::Get();
  if ( config )
  {
    // save the frame position
    int x, y, w, h;
    if ( !IsFullScreen() && !IsIconized() && !IsMaximized() )
    {
      GetPosition( &x, &y );
      GetSize( &w, &h );
      config->Write( _T("/MainWindow/PosX"), (long) x );
      config->Write( _T("/MainWindow/PosY"), (long) y );
      config->Write( _T("/MainWindow/Width"), (long) w );
      config->Write( _T("/MainWindow/Height"), (long) h );
    }
    config->Write( _T("/MainWindow/FullScreen"), IsFullScreen() );
    if ( m_controlPanel->IsShown() )
      config->Write( _T("/MainWindow/SplitterPosition"), m_splitterMain->GetSashPosition() );
    config->Write( _T("/MainWindow/SplitterPositionSub"), m_splitterSub->GetSashPosition() );
    config->Write( _T("/MainWindow/LastDir"), m_strLastDir );
    config->Write( _T("/MainWindow/ViewLayout"), m_nViewLayout );
    config->Write( _T("/MainWindow/MainView"), m_nMainView );
    config->Write( _T("MainWindow/ScreenshotFilterIndex"), m_nScreenshotFilterIndex );

    config->Write( _T("RenderWindow/BackgroundColor"), m_viewAxial->GetBackgroundColor().GetAsString( wxC2S_CSS_SYNTAX ) );
    config->Write( _T("RenderWindow/CursorColor"), m_viewAxial->GetCursor2D()->GetColor().GetAsString( wxC2S_CSS_SYNTAX ) );
    config->Write( _T("/RenderWindow/SyncZoomFactor"), m_settings2D.SyncZoomFactor );
    
    config->Write( _T("/RenderWindow/ShowScalarBar"), 
                   ( m_nMainView >= 0 ? m_viewRender[m_nMainView]->GetShowScalarBar() : false ) );

    config->Write( _T("/Screenshot/Magnification" ),  m_settingsScreenshot.Magnification );
    config->Write( _T("/Screenshot/HideCursor" ),  m_settingsScreenshot.HideCursor );
    config->Write( _T("/Screenshot/HideCoords" ),  m_settingsScreenshot.HideCoords );
    config->Write( _T("/Screenshot/AntiAliasing" ),  m_settingsScreenshot.AntiAliasing );

    config->Write( _T("/MainWindow/ShowControlPanel" ), m_controlPanel->IsShown() );

    m_fileHistory->Save( *config );
  }

  event.Skip();
}


void MainWindow::OnFileOpen( wxCommandEvent& event )
{
  LoadVolume();
}

void MainWindow::NewVolume()
{
  // first check if there is any volume/MRI layer and if the current one is visible
  LayerCollection* col_mri = m_layerCollectionManager->GetLayerCollection( "MRI" );
  if ( !col_mri->GetActiveLayer() )
  {
    wxMessageDialog dlg( this, "Can not create new volume without visible template volume.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }

  // enter the name of the new ROI
  DialogNewVolume dlg( this, col_mri );
  dlg.SetVolumeName( "New volume" );
  if ( dlg.ShowModal() != wxID_OK )
    return;

  // finally we are about to create new volume.
  LayerMRI* layer_new = new LayerMRI( dlg.GetTemplate() );
  layer_new->Create( dlg.GetTemplate(), dlg.GetCopyVoxel() );
  layer_new->SetName( dlg.GetVolumeName().c_str() );
  col_mri->AddLayer( layer_new );

  m_controlPanel->RaisePage( "Volumes" );

// m_viewAxial->SetInteractionMode( RenderView2D::IM_ROIEdit );
// m_viewCoronal->SetInteractionMode( RenderView2D::IM_ROIEdit );
// m_viewSagittal->SetInteractionMode( RenderView2D::IM_ROIEdit );
}

void MainWindow::SaveVolume()
{
  // first check if there is any volume/MRI layer and if the current one is visible
  LayerCollection* col_mri = m_layerCollectionManager->GetLayerCollection( "MRI" );
  LayerMRI* layer_mri = ( LayerMRI* )col_mri->GetActiveLayer();
  if ( !layer_mri)
  {
    return;
  }
  else if ( !layer_mri->IsVisible() )
  {
    wxMessageDialog dlg( this, "Current volume layer is not visible. Please turn it on before saving.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }
  wxString fn = layer_mri->GetFileName();
  if ( fn.IsEmpty() )
  {
    wxFileDialog dlg( this, _("Save volume file"), m_strLastDir, _(""),
                      _T("Volume files (*.nii;*.nii.gz;*.img;*.mgz)|*.nii;*.nii.gz;*.img;*.mgz|All files (*.*)|*.*"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    if ( dlg.ShowModal() == wxID_OK )
    {
      fn = dlg.GetPath();
    }
  }

  if ( !fn.IsEmpty() )
  {
    if ( !MyUtils::HasExtension( fn, "nii" ) &&
         !MyUtils::HasExtension( fn, "nii.gz" ) &&
         !MyUtils::HasExtension( fn, "img" ) &&
         !MyUtils::HasExtension( fn, "mgz" )
       )
    {
      fn += ".mgz";
    }
    layer_mri->SetFileName( fn.char_str() );
    layer_mri->ResetModified();
    WorkerThread* thread = new WorkerThread( this );
    thread->SaveVolume( layer_mri );
  }
}

void MainWindow::SaveVolumeAs()
{
  LayerCollection* col_mri = m_layerCollectionManager->GetLayerCollection( "MRI" );
  LayerMRI* layer_mri = ( LayerMRI* )col_mri->GetActiveLayer();
  if ( !layer_mri)
  {
    return;
  }
  else if ( !layer_mri->IsVisible() )
  {
    wxMessageDialog dlg( this, "Current volume layer is not visible. Please turn it on before saving.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }

  wxString fn = layer_mri->GetFileName();
  wxFileDialog dlg( this, _("Save volume file as"), m_strLastDir, "",
                    _T("Volume files (*.nii;*.nii.gz;*.img;*.mgz)|*.nii;*.nii.gz;*.img;*.mgz|All files (*.*)|*.*"),
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
  if ( dlg.ShowModal() == wxID_OK )
  {
    fn = dlg.GetPath();
    layer_mri->SetFileName( dlg.GetPath().c_str() );
    SaveVolume();
    m_controlPanel->UpdateUI();
  }
}

void MainWindow::LoadVolume()
{
// if ( GetLayerCollection( "MRI" )->IsEmpty() )
  {
    DialogLoadVolume dlg( this, GetLayerCollection( "MRI" )->IsEmpty() );
    dlg.SetLastDir( m_strLastDir );
    wxArrayString list;
    for ( int i = 0; i < m_fileHistory->GetMaxFiles(); i++ )
      list.Add( m_fileHistory->GetHistoryFile( i ) );
    dlg.SetRecentFiles( list );
    if ( dlg.ShowModal() == wxID_OK )
    {
      this->LoadVolumeFile( dlg.GetVolumeFileName(), dlg.GetRegFileName(),
                            ( GetLayerCollection( "MRI" )->IsEmpty() ? dlg.IsToResample() : m_bResampleToRAS ) );
    }
  }
  /* else
   {
    wxFileDialog dlg( this, _("Open volume file"), m_strLastDir, _(""),
        _T("Volume files (*.nii;*.nii.gz;*.img;*.mgz)|*.nii;*.nii.gz;*.img;*.mgz|All files (*.*)|*.*"),
        wxFD_OPEN );
    if ( dlg.ShowModal() == wxID_OK )
    {
     this->LoadVolumeFile( dlg.GetPath(), "", m_bResampleToRAS );
    }
  } */
}

void MainWindow::LoadVolumeFile( const wxString& filename, 
                                 const wxString& reg_filename, 
                                 bool bResample )
{
// cout << bResample << endl;
  m_strLastDir = MyUtils::GetNormalizedPath( filename );

  m_bResampleToRAS = bResample;
  LayerMRI* layer = new LayerMRI( m_layerVolumeRef );
  layer->SetResampleToRAS( bResample );
  layer->GetProperties()->SetLUTCTAB( m_luts->GetColorTable( 0 ) );
  wxFileName fn( filename );
  fn.Normalize();
  wxString layerName = fn.GetName();
  if ( fn.GetExt().Lower() == "gz" )
    layerName = wxFileName( layerName ).GetName();
  layer->SetName( layerName.c_str() );
  layer->SetFileName( fn.GetFullPath().c_str() );
  if ( !reg_filename.IsEmpty() )
  {
    wxFileName reg_fn( reg_filename );
    reg_fn.Normalize( wxPATH_NORM_ALL, fn.GetPath() );
    layer->SetRegFileName( reg_fn.GetFullPath().c_str() );
  }

// if ( !bResample )
  /* {
    LayerMRI* mri = (LayerMRI* )GetLayerCollection( "MRI" )->GetLayer( 0 );
    if ( mri )
    {
     layer->SetRefVolume( mri->GetSourceVolume() );
    }
   }
  */
  WorkerThread* thread = new WorkerThread( this );
  thread->LoadVolume( layer );
}


void MainWindow::RotateVolume( std::vector<RotationElement>& rotations )
{
  WorkerThread* thread = new WorkerThread( this );
  thread->RotateVolume( rotations );
}


void MainWindow::OnFileExit( wxCommandEvent& event )
{
  Close();
}

void MainWindow::OnActivate( wxActivateEvent& event )
{
#ifdef __WXGTK__
  NeedRedraw();
#endif
  event.Skip();
}

void MainWindow::OnIconize( wxIconizeEvent& event )
{
#ifdef __WXGTK__
  if ( !event.Iconized() )
    NeedRedraw();
#endif
  event.Skip();
}


void MainWindow::OnFileRecent( wxCommandEvent& event )
{
  wxString fn( m_fileHistory->GetHistoryFile( event.GetId() - wxID_FILE1 ) );
  if ( !fn.IsEmpty() )
    this->LoadVolumeFile( fn, "", m_bResampleToRAS );
}

LayerCollection* MainWindow::GetLayerCollection( std::string strType )
{
  return m_layerCollectionManager->GetLayerCollection( strType );
}


LayerCollectionManager* MainWindow::GetLayerCollectionManager()
{
  return m_layerCollectionManager;
}

void MainWindow::LoadROI()
{
  if ( GetLayerCollection( "MRI" )->IsEmpty() )
  {
    return;
  }
  wxFileDialog dlg( this, _("Open ROI file"), m_strLastDir, _(""),
                    _T("Label files (*.label)|*.label|All files (*.*)|*.*"),
                    wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    this->LoadROIFile( dlg.GetPath() );
  }
}

void MainWindow::LoadROIFile( const wxString& fn )
{
  m_strLastDir = MyUtils::GetNormalizedPath( fn );

  LayerCollection* col_mri = GetLayerCollection( "MRI" );
  LayerMRI* mri = ( LayerMRI* )col_mri->GetActiveLayer();
  LayerROI* roi = new LayerROI( mri );
  roi->SetName( wxFileName( fn ).GetName().c_str() );
  if ( roi->LoadROIFromFile( fn.c_str() ) )
  {
    LayerCollection* col_roi = GetLayerCollection( "ROI" );
    if ( col_roi->IsEmpty() )
    {
      col_roi->SetWorldOrigin( col_mri->GetWorldOrigin() );
      col_roi->SetWorldSize( col_mri->GetWorldSize() );
      col_roi->SetWorldVoxelSize( col_mri->GetWorldVoxelSize() );
      col_roi->SetSlicePosition( col_mri->GetSlicePosition() );
    }
    col_roi->AddLayer( roi );

    m_controlPanel->RaisePage( "ROIs" );
  }
  else
  {
    delete roi;
    wxMessageDialog dlg( this, wxString( "Can not load ROI from " ) + fn, "Error", wxOK );
    dlg.ShowModal();
  }
}

void MainWindow::NewROI()
{
  // first check if there is any volume/MRI layer and if the current one is visible
  LayerCollection* col_mri = m_layerCollectionManager->GetLayerCollection( "MRI" );
  LayerMRI* layer_mri = ( LayerMRI* )col_mri->GetActiveLayer();
  if ( !layer_mri)
  {
    wxMessageDialog dlg( this, "Can not create new ROI without volume image.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }

  // enter the name of the new ROI
  DialogNewROI dlg( this, col_mri );
  dlg.SetROIName( "New ROI" );
  if ( dlg.ShowModal() != wxID_OK )
    return;

  // finally we are about to create new ROI.
  LayerCollection* col_roi = m_layerCollectionManager->GetLayerCollection( "ROI" );
  if ( col_roi->IsEmpty() )
  {
    col_roi->SetWorldOrigin( col_mri->GetWorldOrigin() );
    col_roi->SetWorldSize( col_mri->GetWorldSize() );
    col_roi->SetWorldVoxelSize( col_mri->GetWorldVoxelSize() );
    col_roi->SetSlicePosition( col_mri->GetSlicePosition() );
  }
  LayerROI* layer_roi = new LayerROI( dlg.GetTemplate() );
  layer_roi->SetName( dlg.GetROIName().c_str() );
  col_roi->AddLayer( layer_roi );

  m_controlPanel->RaisePage( "ROIs" );

  m_viewAxial->SetInteractionMode( RenderView2D::IM_ROIEdit );
  m_viewCoronal->SetInteractionMode( RenderView2D::IM_ROIEdit );
  m_viewSagittal->SetInteractionMode( RenderView2D::IM_ROIEdit );
}

void MainWindow::SaveROI()
{
  LayerCollection* col_roi = m_layerCollectionManager->GetLayerCollection( "ROI" );
  LayerROI* layer_roi = ( LayerROI* )col_roi->GetActiveLayer();
  if ( !layer_roi )
  {
    return;
  }
  else if ( !layer_roi->IsVisible() )
  {
    wxMessageDialog dlg( this, "Current ROI layer is not visible. Please turn it on before saving.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }
  wxString fn = layer_roi->GetFileName();
  if ( fn.IsEmpty() )
  {
    wxFileDialog dlg( this, _("Save ROI file"), m_strLastDir, _(""),
                      _T("Label files (*.label)|*.label|All files (*.*)|*.*"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    if ( dlg.ShowModal() == wxID_OK )
    {
      fn = dlg.GetPath();
    }
  }

  if ( !fn.IsEmpty() )
  {
    if ( !MyUtils::HasExtension( fn, "label" ) )
    {
      fn += ".label";
    }
    layer_roi->SetFileName( fn.char_str() );
    layer_roi->ResetModified();
    WorkerThread* thread = new WorkerThread( this );
    thread->SaveROI( layer_roi );
  }
}


void MainWindow::SaveROIAs()
{
  LayerCollection* col_roi = m_layerCollectionManager->GetLayerCollection( "ROI" );
  LayerROI* layer_roi = ( LayerROI* )col_roi->GetActiveLayer();
  if ( !layer_roi )
  {
    return;
  }
  else if ( !layer_roi->IsVisible() )
  {
    wxMessageDialog dlg( this, "Current ROI layer is not visible. Please turn it on before saving.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }

  wxString fn = layer_roi->GetFileName();
  wxFileDialog dlg( this, _("Save ROI file as"), m_strLastDir, fn,
                    _T("Label files (*.label)|*.label|All files (*.*)|*.*"),
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
  if ( dlg.ShowModal() == wxID_OK )
  {
    layer_roi->SetFileName( dlg.GetPath().c_str() );
    SaveROI();
    m_controlPanel->UpdateUI();
  }
}

void MainWindow::OnFileNewROI( wxCommandEvent& event )
{
  NewROI();
}

void MainWindow::OnFileNewROIUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() );
}


void MainWindow::OnFileNewWayPoints( wxCommandEvent& event )
{
  NewWayPoints();
}

void MainWindow::OnFileNewWayPointsUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() );
}


void MainWindow::LoadWayPoints()
{
  if ( GetLayerCollection( "MRI" )->IsEmpty() )
  {
    return;
  }
  wxFileDialog dlg( this, _("Open Way Points file"), m_strLastDir, _(""),
                    _T("Way Points files (*.label)|*.label|All files (*.*)|*.*"),
                    wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    this->LoadWayPointsFile( dlg.GetPath() );
  }
}

void MainWindow::LoadWayPointsFile( const wxString& fn )
{
  m_strLastDir = MyUtils::GetNormalizedPath( fn );

  LayerCollection* col_mri = GetLayerCollection( "MRI" );
  LayerMRI* mri = ( LayerMRI* )col_mri->GetActiveLayer();
  LayerWayPoints* wp = new LayerWayPoints( mri );
  wp->SetName( wxFileName( fn ).GetName().c_str() );
  if ( wp->LoadFromFile( fn.c_str() ) )
  {
    LayerCollection* col_wp = GetLayerCollection( "WayPoints" );
    if ( col_wp->IsEmpty() )
    {
      col_wp->SetWorldOrigin( col_mri->GetWorldOrigin() );
      col_wp->SetWorldSize( col_mri->GetWorldSize() );
      col_wp->SetWorldVoxelSize( col_mri->GetWorldVoxelSize() );
      col_wp->SetSlicePosition( col_mri->GetSlicePosition() );
    }
    col_wp->AddLayer( wp );

    m_controlPanel->RaisePage( "Way Points" );
  }
  else
  {
    delete wp;
    wxMessageDialog dlg( this, wxString( "Can not load Way Points from " ) + fn, "Error", wxOK );
    dlg.ShowModal();
  }

}


void MainWindow::NewWayPoints()
{
  // first check if there is any volume/MRI layer and if the current one is visible
  LayerCollection* col_mri = m_layerCollectionManager->GetLayerCollection( "MRI" );
  LayerMRI* layer_mri = ( LayerMRI* )col_mri->GetActiveLayer();
  if ( !layer_mri)
  {
    wxMessageDialog dlg( this, "Can not create new way points without volume image.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }

  // enter the name of the new ROI
  DialogNewWayPoints dlg( this, col_mri );
  dlg.SetWayPointsName( "New Way Points" );
  if ( dlg.ShowModal() != wxID_OK )
    return;

  // finally we are about to create new ROI.
  LayerCollection* col_wp = m_layerCollectionManager->GetLayerCollection( "WayPoints" );
  if ( col_wp->IsEmpty() )
  {
    col_wp->SetWorldOrigin( col_mri->GetWorldOrigin() );
    col_wp->SetWorldSize( col_mri->GetWorldSize() );
    col_wp->SetWorldVoxelSize( col_mri->GetWorldVoxelSize() );
    col_wp->SetSlicePosition( col_mri->GetSlicePosition() );
  }
  LayerWayPoints* layer_wp = new LayerWayPoints( dlg.GetTemplate() );
  layer_wp->SetName( dlg.GetWayPointsName().c_str() );
  col_wp->AddLayer( layer_wp );

  m_controlPanel->RaisePage( "Way Points" );

  m_viewAxial->SetInteractionMode( RenderView2D::IM_WayPointsEdit );
  m_viewCoronal->SetInteractionMode( RenderView2D::IM_WayPointsEdit );
  m_viewSagittal->SetInteractionMode( RenderView2D::IM_WayPointsEdit );
}

void MainWindow::SaveWayPoints()
{
  LayerCollection* col_wp = m_layerCollectionManager->GetLayerCollection( "WayPoints" );
  LayerWayPoints* layer_wp = ( LayerWayPoints* )col_wp->GetActiveLayer();
  if ( !layer_wp )
  {
    return;
  }
  else if ( !layer_wp->IsVisible() )
  {
    wxMessageDialog dlg( this, "Current Way Points layer is not visible. Please turn it on before saving.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }
  wxString fn = layer_wp->GetFileName();
  if ( fn.IsEmpty() )
  {
    wxFileDialog dlg( this, _("Save Way Points file"), m_strLastDir, _(""),
                      _T("Way Points files (*.label)|*.label|All files (*.*)|*.*"),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
    if ( dlg.ShowModal() == wxID_OK )
    {
      fn = dlg.GetPath();
    }
  }

  if ( !fn.IsEmpty() )
  {
    if ( !MyUtils::HasExtension( fn, "label" ) )
    {
      fn += ".label";
    }
    layer_wp->SetFileName( fn.char_str() );
    layer_wp->ResetModified();
    WorkerThread* thread = new WorkerThread( this );
    thread->SaveWayPoints( layer_wp );
  }
}


void MainWindow::SaveWayPointsAs()
{
  LayerCollection* col_wp = m_layerCollectionManager->GetLayerCollection( "WayPoints" );
  LayerWayPoints* layer_wp = ( LayerWayPoints* )col_wp->GetActiveLayer();
  if ( !layer_wp )
  {
    return;
  }
  else if ( !layer_wp->IsVisible() )
  {
    wxMessageDialog dlg( this, "Current Way Points layer is not visible. Please turn it on before saving.", "Error", wxOK );
    dlg.ShowModal();
    return;
  }

  wxString fn = layer_wp->GetFileName();
  wxFileDialog dlg( this, _("Save Way Points file as"), m_strLastDir, fn,
                    _T("Way Points files (*.label)|*.label|All files (*.*)|*.*"),
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
  if ( dlg.ShowModal() == wxID_OK )
  {
    layer_wp->SetFileName( dlg.GetPath().c_str() );
    SaveWayPoints();
    m_controlPanel->UpdateUI();
  }
}

void MainWindow::OnKeyDown( wxKeyEvent& event )
{
// cout << "test" << endl;
// m_viewAxial->GetEventHandler()->ProcessEvent( event );

  event.Skip();
}

void MainWindow::UpdateToolbars()
{
  m_bToUpdateToolbars = true;
}

void MainWindow::DoUpdateToolbars()
{
// bool bVoxelEditVisible = m_toolbarVoxelEdit->IsShown();
// bool bROIEditVisible = m_toolbarROIEdit->IsShown();
// if ( bVoxelEditVisible != (m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit) ||
//   bROIEditVisible != (m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit) )

// bool bToolWindowVisible = m_toolWindowEdit->IsShown();
// if ( bToolWindowVisible != (m_viewAxial->GetInteractionMode() != RenderView2D::IM_Navigate ) )
  {
    // m_toolbarVoxelEdit->Show( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit );
    // m_toolbarROIEdit->Show( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit );
    // m_toolbarBrush->Show( m_viewAxial->GetInteractionMode() != RenderView2D::IM_Navigate );
    // m_panelToolbarHolder->Layout();
    // bool bNeedReposition = ( m_toolWindowEdit->IsShown() != (m_viewAxial->GetInteractionMode() != RenderView2D::IM_Navigate) );
    if ( !m_toolWindowEdit )
      m_toolWindowEdit = new ToolWindowEdit( this );

    if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit ||
         m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
    {
      m_toolWindowEdit->Show();
    }
    else
      m_toolWindowEdit->Hide();

    m_toolWindowEdit->UpdateTools();
  }
  /*
  XRCCTRL( *m_toolbarBrush, "ID_STATIC_BRUSH_SIZE", wxStaticText )->Enable( m_viewAxial->GetAction() != Interactor2DROIEdit::EM_Fill );
  XRCCTRL( *m_toolbarBrush, "ID_SPIN_BRUSH_SIZE", wxSpinCtrl )->Enable( m_viewAxial->GetAction() != Interactor2DROIEdit::EM_Fill );
  wxCheckBox* checkTemplate = XRCCTRL( *m_toolbarBrush, "ID_CHECK_TEMPLATE", wxCheckBox );
  wxChoice* choiceTemplate = XRCCTRL( *m_toolbarBrush, "ID_CHOICE_TEMPLATE", wxChoice );
  // checkTemplate->Enable( m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
  // choiceTemplate->Enable( checkTemplate->IsChecked() && m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
  XRCCTRL( *m_toolbarBrush, "ID_STATIC_BRUSH_TOLERANCE", wxStaticText )->Enable( checkTemplate->IsChecked() ); //&& m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
  XRCCTRL( *m_toolbarBrush, "ID_SPIN_BRUSH_TOLERANCE", wxSpinCtrl )->Enable( checkTemplate->IsChecked() );//&& m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );

  LayerEditable* layer = NULL;
  if ( choiceTemplate->GetSelection() != wxNOT_FOUND )
   layer = ( LayerEditable* )(void*)choiceTemplate->GetClientData( choiceTemplate->GetSelection() );

  choiceTemplate->Clear();
  LayerCollection* lc = GetLayerCollection( "MRI" );
  int nSel = 0;
  for ( int i = 0; i < lc->GetNumberOfLayers(); i++ )
  {
   LayerMRI* mri = (LayerMRI*)lc->GetLayer( i );
   if ( layer == mri )
   {
    nSel = i;
    m_propertyBrush->SetReferenceLayer( layer );
   }
   
   choiceTemplate->Append( mri->GetName(), (void*)mri );
  }
  if ( !lc->IsEmpty() )
   choiceTemplate->SetSelection( nSel );
  if ( !checkTemplate->IsChecked() )
   m_propertyBrush->SetReferenceLayer( NULL );

  if ( checkTemplate->IsChecked() )
   m_propertyBrush->SetBrushTolerance( XRCCTRL( *m_toolbarBrush, "ID_SPIN_BRUSH_TOLERANCE", wxSpinCtrl )->GetValue() );
  else
   m_propertyBrush->SetBrushTolerance( 0 );
  */

  /*
   switch ( m_viewAxial->GetInteractionMode() )
   {
    case RenderView2D::IM_VoxelEdit:
     m_controlPanel->RaisePage( "Volumes" );
     break;
    case RenderView2D::IM_ROIEdit:
     m_controlPanel->RaisePage( "ROIs" );
     break;
    case RenderView2D::IM_WayPointsEdit:
     m_controlPanel->RaisePage( "Way Points" );
     break;
    default:
    break;
   }
   */

  m_bToUpdateToolbars = false;
}

void MainWindow::SetMode( int nMode )
{
  m_viewAxial->SetInteractionMode( nMode );
  m_viewCoronal->SetInteractionMode( nMode );
  m_viewSagittal->SetInteractionMode( nMode );

  UpdateToolbars();
}

void MainWindow::OnModeNavigate( wxCommandEvent& event )
{
  SetMode( RenderView2D::IM_Navigate );
}

void MainWindow::OnModeNavigateUpdateUI( wxUpdateUIEvent& event)
{
  event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_Navigate );
}

void MainWindow::OnModeVoxelEdit( wxCommandEvent& event )
{
  SetMode( RenderView2D::IM_VoxelEdit );
}

void MainWindow::OnModeVoxelEditUpdateUI( wxUpdateUIEvent& event)
{
  event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit );
  event.Enable( m_layerCollectionManager->HasLayer( "MRI" ) );
}

void MainWindow::OnModeROIEdit( wxCommandEvent& event )
{
  SetMode( RenderView2D::IM_ROIEdit );
}

void MainWindow::OnModeROIEditUpdateUI( wxUpdateUIEvent& event)
{
  event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit );
  event.Enable( m_layerCollectionManager->HasLayer( "ROI" ) );
}

void MainWindow::OnModeWayPointsEdit( wxCommandEvent& event )
{
  SetMode( RenderView2D::IM_WayPointsEdit );
}

void MainWindow::OnModeWayPointsEditUpdateUI( wxUpdateUIEvent& event)
{
  event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_WayPointsEdit );
  event.Enable( m_layerCollectionManager->HasLayer( "WayPoints" ) );
}

void MainWindow::SetAction( int nAction )
{
  m_viewAxial->SetAction( nAction );
  m_viewCoronal->SetAction( nAction );
  m_viewSagittal->SetAction( nAction );
  UpdateToolbars();
}

void MainWindow::OnEditUndo( wxCommandEvent& event )
{
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    if ( roi )
      roi->Undo();
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    if ( mri )
      mri->Undo();
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_WayPointsEdit )
  {
    LayerWayPoints* wp = ( LayerWayPoints* )GetLayerCollection( "WayPoints" )->GetActiveLayer();
    if ( wp )
      wp->Undo();
  }
}

void MainWindow::OnEditUndoUpdateUI( wxUpdateUIEvent& event )
{
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    event.Enable( roi && roi->IsVisible() && roi->HasUndo() );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    event.Enable( mri && mri->IsVisible() && mri->HasUndo() );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_WayPointsEdit )
  {
    LayerWayPoints* wp = ( LayerWayPoints* )GetLayerCollection( "WayPoints" )->GetActiveLayer();
    event.Enable( wp && wp->IsVisible() && wp->HasUndo() );
  }
  else
    event.Enable( false );
}

void MainWindow::OnEditRedo( wxCommandEvent& event )
{
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    if ( roi )
      roi->Redo();
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    if ( mri )
      mri->Redo();
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_WayPointsEdit )
  {
    LayerWayPoints* wp = ( LayerWayPoints* )GetLayerCollection( "WayPoints" )->GetActiveLayer();
    if ( wp )
      wp->Redo();
  }
}

void MainWindow::OnEditRedoUpdateUI( wxUpdateUIEvent& event )
{
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    event.Enable( roi && roi->IsVisible() && roi->HasRedo() );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    event.Enable( mri && mri->IsVisible() && mri->HasRedo() );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_WayPointsEdit )
  {
    LayerWayPoints* wp = ( LayerWayPoints* )GetLayerCollection( "WayPoints" )->GetActiveLayer();
    event.Enable( wp && wp->IsVisible() && wp->HasRedo() );
  }
  else
    event.Enable( false );
}


void MainWindow::OnEditCopy( wxCommandEvent& event )
{
  int nWnd = GetActiveViewId();
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    if ( roi && nWnd >= 0 && nWnd < 3 )
      roi->Copy( nWnd );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    if ( mri && nWnd >= 0 && nWnd < 3 )
      mri->Copy( nWnd );
  }
}

void MainWindow::OnEditCopyUpdateUI( wxUpdateUIEvent& event )
{
  int nWnd = GetActiveViewId();
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    event.Enable( roi && roi->IsVisible() && nWnd >= 0 && nWnd < 3 );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    event.Enable( mri && mri->IsVisible() && nWnd >= 0 && nWnd < 3 );
  }
  else
    event.Enable( false );
}

void MainWindow::OnEditPaste( wxCommandEvent& event )
{
  int nWnd = GetActiveViewId();
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    if ( roi && nWnd >= 0 && nWnd < 3 )
      roi->Paste( nWnd );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    if ( mri && nWnd >= 0 && nWnd < 3 )
      mri->Paste( nWnd );
  }
}

void MainWindow::OnEditPasteUpdateUI( wxUpdateUIEvent& event )
{
  int nWnd = GetActiveViewId();
  if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit )
  {
    LayerROI* roi = ( LayerROI* )GetLayerCollection( "ROI" )->GetActiveLayer();
    event.Enable( roi && roi->IsVisible() && roi->IsEditable() && nWnd >= 0 && nWnd < 3 && roi->IsValidToPaste( nWnd ) );
  }
  else if ( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit )
  {
    LayerMRI* mri = ( LayerMRI* )GetLayerCollection( "MRI" )->GetActiveLayer();
    event.Enable( mri && mri->IsVisible() && mri->IsEditable() && nWnd >= 0 && nWnd < 3 && mri->IsValidToPaste( nWnd ) );
  }
  else
    event.Enable( false );
}

void MainWindow::OnViewToggleVoxelCoordinates( wxCommandEvent& event )
{
  m_pixelInfoPanel->ToggleShowVoxelCoordinates();
}


void MainWindow::SetViewLayout( int nLayout )
{
  m_nViewLayout = nLayout;

  RenderView* view[4] = { 0 };
  switch ( m_nMainView )
  {
  case MV_Coronal:
    view[0] = m_viewCoronal;
    view[1] = m_viewSagittal;
    view[2] = m_viewAxial;
    view[3] = m_view3D;
    break;
  case MV_Axial:
    view[0] = m_viewAxial;
    view[1] = m_viewSagittal;
    view[2] = m_viewCoronal;
    view[3] = m_view3D;
    break;
  case MV_3D:
    view[0] = m_view3D;
    view[1] = m_viewSagittal;
    view[2] = m_viewCoronal;
    view[3] = m_viewAxial;
    break;
  default:
    view[0] = m_viewSagittal;
    view[1] = m_viewCoronal;
    view[2] = m_viewAxial;
    view[3] = m_view3D;
    break;
  }

  if ( m_nViewLayout == VL_1X1 )
  {
    view[0]->Show();
    view[1]->Hide();
    view[2]->Hide();
    view[3]->Hide();
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_renderViewHolder->SetSizer( sizer );
    sizer->Add( view[0], 1, wxEXPAND );
  }
  else if ( m_nViewLayout == VL_1N3 )
  {
    for ( int i = 0; i < 4; i++ )
      view[i]->Show();

    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_renderViewHolder->SetSizer( sizer );

    sizer->Add( view[0], 2, wxEXPAND );
    sizer->AddSpacer( 1 );
    wxBoxSizer* sizer2 = new wxBoxSizer( wxHORIZONTAL );
    sizer->Add( sizer2, 1, wxEXPAND );
    sizer2->Add( view[1], 1, wxEXPAND );
    sizer2->AddSpacer( 1 );
    sizer2->Add( view[2], 1, wxEXPAND );
    sizer2->AddSpacer( 1 );
    sizer2->Add( view[3], 1, wxEXPAND );
  }
  else if ( m_nViewLayout == VL_1N3_H )
  {
    for ( int i = 0; i < 4; i++ )
      view[i]->Show();

    wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
    m_renderViewHolder->SetSizer( sizer );

    sizer->Add( view[0], 2, wxEXPAND );
    sizer->AddSpacer( 1 );
    wxBoxSizer* sizer2 = new wxBoxSizer( wxVERTICAL );
    sizer->Add( sizer2, 1, wxEXPAND );
    sizer2->Add( view[1], 1, wxEXPAND );
    sizer2->AddSpacer( 1 );
    sizer2->Add( view[2], 1, wxEXPAND );
    sizer2->AddSpacer( 1 );
    sizer2->Add( view[3], 1, wxEXPAND );
  }
  else
  {
    wxGridSizer* grid = new wxGridSizer( 2, 2, 1, 1 );
    m_renderViewHolder->SetSizer( grid );
    for ( int i = 0; i < 4; i++ )
    {
      view[i]->Show();
      grid->Add( view[i], 1, wxEXPAND );
    }
  }

  m_renderViewHolder->Layout();
  view[0]->SetFocus();

  NeedRedraw();
}

void MainWindow::SetMainView( int nView )
{
  if ( m_nMainView >= 0 )
  {
    // udpate scalar bar
    bool bScalarBar = m_viewRender[m_nMainView]->GetShowScalarBar();
    if ( bScalarBar )
    {
      m_viewRender[m_nMainView]->ShowScalarBar( false );
      m_viewRender[nView]->ShowScalarBar( true );
    }
  }
  m_nMainView = nView;

  SetViewLayout( m_nViewLayout );
}

void MainWindow::OnViewLayout1X1( wxCommandEvent& event )
{
  SetViewLayout( VL_1X1 );
}

void MainWindow::OnViewLayout1X1UpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nViewLayout == VL_1X1 );
}

void MainWindow::OnViewLayout2X2( wxCommandEvent& event )
{
  SetViewLayout( VL_2X2 );
}

void MainWindow::OnViewLayout2X2UpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nViewLayout == VL_2X2 );
}

void MainWindow::OnViewLayout1N3( wxCommandEvent& event )
{
  SetViewLayout( VL_1N3 );
}

void MainWindow::OnViewLayout1N3UpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nViewLayout == VL_1N3 );
}


void MainWindow::OnViewLayout1N3_H( wxCommandEvent& event )
{
  SetViewLayout( VL_1N3_H );
}

void MainWindow::OnViewLayout1N3_HUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nViewLayout == VL_1N3_H );
}

void MainWindow::OnViewSagittal( wxCommandEvent& event )
{
  SetMainView( MV_Sagittal );
}

void MainWindow::OnViewSagittalUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nMainView == MV_Sagittal );
}

void MainWindow::OnViewCoronal( wxCommandEvent& event )
{
  SetMainView( MV_Coronal );
}

void MainWindow::OnViewCoronalUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nMainView == MV_Coronal );
}

void MainWindow::OnViewAxial( wxCommandEvent& event )
{
  SetMainView( MV_Axial );
}

void MainWindow::OnViewAxialUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nMainView == MV_Axial );
}

void MainWindow::OnView3D( wxCommandEvent& event )
{
  SetMainView( MV_3D );
}

void MainWindow::OnView3DUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_nMainView == MV_3D );
}

void MainWindow::OnViewReset( wxCommandEvent& event )
{
  wxWindow* wnd = FindFocus();
  if ( wnd == m_viewAxial )
    m_viewAxial->ResetView();
  else if ( wnd == m_viewSagittal )
    m_viewSagittal->ResetView();
  else if ( wnd == m_viewCoronal )
    m_viewCoronal->ResetView();
  else if ( wnd == m_view3D )
    m_view3D->ResetView();
}

void MainWindow::OnViewResetUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() || !GetLayerCollection( "Surface" )->IsEmpty() );
}

void MainWindow::ShowControlPanel( bool bShow )
{
  wxConfigBase* config = wxConfigBase::Get();
  if ( bShow )
  {
    m_splitterMain->SplitVertically( m_controlPanel, m_splitterSub,
                                     config ? config->Read( _T("/MainWindow/SplitterPosition"), 260L ) : 260 );
  }
  else
  {
    if ( config )
      config->Write( _T("/MainWindow/SplitterPosition"), m_splitterMain->GetSashPosition() );
    m_splitterMain->Unsplit( m_controlPanel );
  }
}

void MainWindow::OnViewControlPanel( wxCommandEvent& event )
{
  ShowControlPanel( event.IsChecked() );
}

void MainWindow::OnViewControlPanelUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_controlPanel->IsShown() );
}

void MainWindow::OnViewScalarBar( wxCommandEvent& event )
{
  ShowScalarBar( event.IsChecked() );
}

void MainWindow::ShowScalarBar( bool bShow )
{
  for ( int i = 0; i < 4; i++ )
  {
    if ( i != m_nMainView )
      m_viewRender[i]->ShowScalarBar( false );
  }
  if ( m_nMainView >= 0 && m_nMainView < 4 )
  {
    m_viewRender[m_nMainView]->ShowScalarBar( bShow );
    m_viewRender[m_nMainView]->UpdateScalarBar();
  }

  NeedRedraw( 1 );
}

void MainWindow::OnViewScalarBarUpdateUI( wxUpdateUIEvent& event )
{
//  LayerMRI* mri = (LayerMRI*)MainWindow::GetMainWindowPointer()->GetLayerCollection( "MRI" )->GetActiveLayer();
//  LayerSurface* surf = (LayerSurface*)MainWindow::GetMainWindowPointer()->GetLayerCollection( "Surface" )->GetActiveLayer();
//  event.Enable( mri || ( m_nMainView == 3 && surf && surf->GetActiveOverlay() ) );
  if ( m_nMainView >= 0 && m_nMainView < 4 )
    event.Check( m_viewRender[m_nMainView]->GetShowScalarBar() );
}


void MainWindow::OnViewCoordinate( wxCommandEvent& event )
{
  for ( int i = 0; i < 3; i++ )
  {
    ( (RenderView2D*)m_viewRender[i] )->ShowCoordinateAnnotation( event.IsChecked() );
  }

  NeedRedraw( 1 );
}

void MainWindow::OnViewCoordinateUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( MainWindow::GetMainWindowPointer()->GetLayerCollection( "MRI" )->GetActiveLayer() );
  event.Check( m_viewAxial->GetShowCoordinateAnnotation() );
}

void MainWindow::OnViewCycleLayer( wxCommandEvent& event )
{
  LayerCollection* lc = NULL;
  switch ( m_controlPanel->GetCurrentLayerCollectionIndex() )
  {
  case 0:   // Volume
    lc = GetLayerCollection( "MRI" );
    break;
  case 1:   // ROI
    lc = GetLayerCollection( "ROI" );
    break;
  case 2:
    lc = GetLayerCollection( "Surface" );
  }

  if ( lc )
  {
    lc->CycleLayer();
  }
}

void MainWindow::OnViewCycleLayerUpdateUI( wxUpdateUIEvent& event )
{
  LayerCollection* lc = NULL;
  switch ( m_controlPanel->GetCurrentLayerCollectionIndex() )
  {
  case 0:   // Volume
    lc = GetLayerCollection( "MRI" );
    break;
  case 1:   // ROI
    lc = GetLayerCollection( "ROI" );
    break;
  }

  event.Enable( lc && lc->GetNumberOfLayers() > 1 );
}

void MainWindow::OnViewToggleVolumeVisibility( wxCommandEvent& event )
{
  LayerCollection* lc = GetLayerCollection( "MRI" );
  if ( !lc->IsEmpty() )
  {
    Layer* layer = lc->GetActiveLayer();
    if ( layer )
      layer->SetVisible( !layer->IsVisible() );
  }
}

void MainWindow::OnViewToggleVolumeVisibilityUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() );
}


void MainWindow::OnViewToggleROIVisibility( wxCommandEvent& event )
{
  LayerCollection* lc = GetLayerCollection( "ROI" );
  if ( !lc->IsEmpty() )
  {
    Layer* layer = lc->GetActiveLayer();
    if ( layer )
      layer->SetVisible( !layer->IsVisible() );
  }
}

void MainWindow::OnViewToggleROIVisibilityUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "ROI" )->IsEmpty() );
}


void MainWindow::OnViewToggleSurfaceVisibility( wxCommandEvent& event )
{
  LayerCollection* lc = GetLayerCollection( "Surface" );
  if ( !lc->IsEmpty() )
  {
    Layer* layer = lc->GetActiveLayer();
    if ( layer )
      layer->SetVisible( !layer->IsVisible() );
  }
}

void MainWindow::OnViewToggleSurfaceVisibilityUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "Surface" )->IsEmpty() );
}


void MainWindow::OnViewToggleWayPointsVisibility( wxCommandEvent& event )
{
  LayerCollection* lc = GetLayerCollection( "WayPoints" );
  if ( !lc->IsEmpty() )
  {
    Layer* layer = lc->GetActiveLayer();
    if ( layer )
      layer->SetVisible( !layer->IsVisible() );
  }
}

void MainWindow::OnViewToggleWayPointsVisibilityUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "WayPoints" )->IsEmpty() );
}

void MainWindow::OnViewToggleCursorVisibility( wxCommandEvent& event )
{
  bool bCur = m_viewAxial->GetCursor2D()->IsShown();
  m_viewAxial->GetCursor2D()->Show( !bCur );
  m_viewSagittal->GetCursor2D()->Show( !bCur );
  m_viewCoronal->GetCursor2D()->Show( !bCur );
  m_view3D->GetCursor3D()->Show( !bCur );
  NeedRedraw( 1 );
}

void MainWindow::OnViewToggleCursorVisibilityUpdateUI( wxUpdateUIEvent& event )
{}

void MainWindow::OnViewSurfaceMain( wxCommandEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
    layer->SetActiveSurface( FSSurface::SurfaceMain );
}

void MainWindow::OnViewSurfaceMainUpdateUI( wxUpdateUIEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  event.Enable( layer );
  event.Check( layer && layer->GetActiveSurface() == FSSurface::SurfaceMain );
}

void MainWindow::OnViewSurfaceInflated( wxCommandEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
    layer->SetActiveSurface( FSSurface::SurfaceInflated );
}

void MainWindow::OnViewSurfaceInflatedUpdateUI( wxUpdateUIEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  FSSurface* surf = ( layer ? layer->GetSourceSurface() : NULL );
  event.Enable( layer && surf && surf->IsSurfaceLoaded( FSSurface::SurfaceInflated ) );
  event.Check( layer && layer->GetActiveSurface() == FSSurface::SurfaceInflated );
}

void MainWindow::OnViewSurfaceWhite( wxCommandEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
    layer->SetActiveSurface( FSSurface::SurfaceWhite );
}

void MainWindow::OnViewSurfaceWhiteUpdateUI( wxUpdateUIEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  FSSurface* surf = ( layer ? layer->GetSourceSurface() : NULL );
  event.Enable( layer && surf && surf->IsSurfaceLoaded( FSSurface::SurfaceWhite ) );
  event.Check( layer && layer->GetActiveSurface() == FSSurface::SurfaceWhite );
}

void MainWindow::OnViewSurfacePial( wxCommandEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
    layer->SetActiveSurface( FSSurface::SurfacePial );
}

void MainWindow::OnViewSurfacePialUpdateUI( wxUpdateUIEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  FSSurface* surf = ( layer ? layer->GetSourceSurface() : NULL );
  event.Enable( layer && surf && surf->IsSurfaceLoaded( FSSurface::SurfacePial ) );
  event.Check( layer && layer->GetActiveSurface() == FSSurface::SurfacePial );
}

void MainWindow::OnViewSurfaceOriginal( wxCommandEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
    layer->SetActiveSurface( FSSurface::SurfaceOriginal );
}

void MainWindow::OnViewSurfaceOriginalUpdateUI( wxUpdateUIEvent& event )
{
  LayerSurface* layer = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  FSSurface* surf = ( layer ? layer->GetSourceSurface() : NULL );
  event.Enable( layer && surf && surf->IsSurfaceLoaded( FSSurface::SurfaceOriginal ) );
  event.Check( layer && layer->GetActiveSurface() == FSSurface::SurfaceOriginal );
}

void MainWindow::NeedRedraw( int nCount )
{
  m_nRedrawCount = nCount;
}

void MainWindow::OnInternalIdle()
{
  wxFrame::OnInternalIdle();

#ifdef __WXGTK__
  if ( IsShown() && m_nRedrawCount > 0 )
  {
    for ( int i = 0; i < 4; i++ )
      if ( m_viewRender[i]->IsShown() )
        m_viewRender[i]->Render();
    m_nRedrawCount--;
  }
#endif

  if ( m_bToUpdateToolbars )
    DoUpdateToolbars();

  // hack to restore previously saved sub-splitter postion.
  static bool run_once = false;
  if ( !run_once && IsShown() )
  {
    wxConfigBase* config = wxConfigBase::Get();
    if ( config )
    {
      if ( config->Exists( _T("/MainWindow/SplitterPositionSub") ) )
      {
        m_splitterSub->SetSashPosition( config->Read( _T("/MainWindow/SplitterPositionSub"), 80L ) );
        m_splitterSub->UpdateSize();
      }
    }
    run_once = true;
  }
}

void MainWindow::OnEditPreferences( wxCommandEvent& event )
{
  DialogPreferences dlg( this );
  dlg.SetBackgroundColor( m_viewAxial->GetBackgroundColor() );
  dlg.SetCursorColor( m_viewAxial->GetCursor2D()->GetColor() );
  dlg.Set2DSettings( m_settings2D );
  dlg.SetScreenshotSettings( m_settingsScreenshot );

  if ( dlg.ShowModal() == wxID_OK )
  {
    wxColour color = dlg.GetCursorColor();
    m_viewAxial->GetCursor2D()->SetColor( color );
    m_viewSagittal->GetCursor2D()->SetColor( color );
    m_viewCoronal->GetCursor2D()->SetColor( color );

    color = dlg.GetBackgroundColor();
    m_viewAxial->SetBackgroundColor( color );
    m_viewSagittal->SetBackgroundColor( color );
    m_viewCoronal->SetBackgroundColor( color );
    m_view3D->SetBackgroundColor( color );
    m_settings2D = dlg.Get2DSettings();
    m_settingsScreenshot = dlg.GetScreenshotSettings();
  }
}

void MainWindow::OnHelpQuickReference( wxCommandEvent& event )
{
  m_wndQuickReference->Show();
  wxConfigBase* config = wxConfigBase::Get();
  if ( config )
  {
    int x = config->Read( _T("/QuickRefWindow/PosX"), 280L );
    int y = config->Read( _T("/QuickRefWindow/PosY"), 30L );
    int x1, y1;
    GetPosition( &x1, &y1 );
    m_wndQuickReference->Move( x1 + x, y1 + y );
  }
}

void MainWindow::OnHelpAbout( wxCommandEvent& event )
{
  wxString msg = wxString( "freeview 1.0 (internal) \r\nbuild ") + MyUtils::GetDateAndTime();

  wxMessageDialog dlg( this, msg, "About freeview", wxOK | wxICON_INFORMATION );
  dlg.ShowModal();
}

void MainWindow::OnFileNew( wxCommandEvent& event )
{
  NewVolume();
}

void MainWindow::OnFileNewUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() );
}

void MainWindow::OnFileSave( wxCommandEvent& event )
{
  SaveVolume();
}

void MainWindow::OnFileSaveUpdateUI( wxUpdateUIEvent& event )
{
  LayerMRI* layer = ( LayerMRI* )( GetLayerCollection( "MRI" )->GetActiveLayer() );
  event.Enable( layer && layer->IsModified() && !IsProcessing() );
}


void MainWindow::OnFileSaveAs( wxCommandEvent& event )
{
  SaveVolumeAs();
}

void MainWindow::OnFileSaveAsUpdateUI( wxUpdateUIEvent& event )
{
  LayerMRI* layer = ( LayerMRI* )( GetLayerCollection( "MRI" )->GetActiveLayer() );
  event.Enable( layer && layer->IsEditable() && !IsProcessing() );
}

void MainWindow::OnFileLoadROI( wxCommandEvent& event )
{
  LoadROI();
}

void MainWindow::OnFileLoadROIUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() && !IsProcessing() );
}


void MainWindow::OnFileSaveROI( wxCommandEvent& event )
{
  SaveROI();
}

void MainWindow::OnFileSaveROIUpdateUI( wxUpdateUIEvent& event )
{
  LayerROI* layer = ( LayerROI* )( GetLayerCollection( "ROI" )->GetActiveLayer() );
  event.Enable( layer && layer->IsModified() && !IsProcessing() );
}


void MainWindow::OnFileSaveROIAs( wxCommandEvent& event )
{
  SaveROIAs();
}

void MainWindow::OnFileSaveROIAsUpdateUI( wxUpdateUIEvent& event )
{
  LayerROI* layer = ( LayerROI* )( GetLayerCollection( "ROI" )->GetActiveLayer() );
  event.Enable( layer && !IsProcessing() );
}


void MainWindow::OnFileLoadWayPoints( wxCommandEvent& event )
{
  LoadWayPoints();
}

void MainWindow::OnFileLoadWayPointsUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() && !IsProcessing() );
}


void MainWindow::OnFileSaveWayPoints( wxCommandEvent& event )
{
  SaveWayPoints();
}

void MainWindow::OnFileSaveWayPointsUpdateUI( wxUpdateUIEvent& event )
{
  LayerWayPoints* layer = ( LayerWayPoints* )( GetLayerCollection( "WayPoints" )->GetActiveLayer() );
  event.Enable( layer && layer->IsModified() && !IsProcessing() );
}


void MainWindow::OnFileSaveWayPointsAs( wxCommandEvent& event )
{
  SaveWayPointsAs();
}

void MainWindow::OnFileSaveWayPointsAsUpdateUI( wxUpdateUIEvent& event )
{
  LayerWayPoints* layer = ( LayerWayPoints* )( GetLayerCollection( "WayPoints" )->GetActiveLayer() );
  event.Enable( layer && !IsProcessing() );
}


void MainWindow::OnWorkerThreadResponse( wxCommandEvent& event )
{
  wxString strg = event.GetString();

  if ( strg.Left( 6 ) == "Failed" )
  {
    m_statusBar->m_gaugeBar->Hide();
    m_bProcessing = false;
    EnableControls( true );
    m_controlPanel->UpdateUI();
    strg = strg.Mid( 6 );
    if ( strg.IsEmpty() )
      strg = "Operation failed. See console for more information.";
    wxMessageDialog dlg( this, strg, "Error", wxOK | wxICON_ERROR );
    dlg.ShowModal();
    return;
  }

  if ( event.GetInt() == -1 )  // successfully finished
  {
    m_bProcessing = false;
    EnableControls( true );
    m_statusBar->m_gaugeBar->Hide();

    Layer* layer = ( Layer* )(void*)event.GetClientData();
    wxASSERT( layer != NULL || strg != "Load" || strg != "Save" );
    LayerCollection* lc_mri = GetLayerCollection( "MRI" );
    LayerCollection* lc_surface = GetLayerCollection( "Surface" );

    // loading operation finished
    if ( strg == "Load" )
    {
      // volume loaded
      if ( layer->IsTypeOf( "MRI" ) )
      {
        LayerMRI* mri = (LayerMRI*)layer;
        if ( lc_mri->IsEmpty() )
        {
          double worigin[3], wsize[3];
          mri->GetWorldOrigin( worigin );
          mri->GetWorldSize( wsize );
          if ( lc_surface->IsEmpty() )
          {
            mri->SetSlicePositionToWorldCenter();
            m_viewAxial->SetWorldCoordinateInfo( worigin, wsize );
            m_viewSagittal->SetWorldCoordinateInfo( worigin, wsize );
            m_viewCoronal->SetWorldCoordinateInfo( worigin, wsize );
            m_view3D->SetWorldCoordinateInfo( worigin, wsize );
          }
          else
          {
            mri->SetSlicePosition( lc_surface->GetSlicePosition() );
            lc_surface->SetWorldVoxelSize( mri->GetWorldVoxelSize() );
            lc_surface->SetWorldOrigin( mri->GetWorldOrigin() );
            lc_surface->SetWorldSize( mri->GetWorldSize() );
          }

          lc_mri->AddLayer( mri, true );
          lc_mri->SetCursorRASPosition( lc_mri->GetSlicePosition() );
          m_layerVolumeRef = mri;

          mri->AddListener( this );
        }
        else
          lc_mri->AddLayer( layer );

        m_fileHistory->AddFileToHistory( MyUtils::GetNormalizedFullPath( mri->GetFileName() ) );

        m_controlPanel->RaisePage( "Volumes" );
      }
      // surface loaded
      else if ( layer->IsTypeOf( "Surface" ) )
      {
        LayerSurface* sf = (LayerSurface*)layer;
        if ( lc_surface->IsEmpty() )
        {
          double worigin[3], wsize[3];
          sf->GetWorldOrigin( worigin );
          sf->GetWorldSize( wsize );
          sf->SetSlicePositionToWorldCenter();
          if ( lc_mri->IsEmpty() )
          {
            m_viewAxial->SetWorldCoordinateInfo( worigin, wsize );
            m_viewSagittal->SetWorldCoordinateInfo( worigin, wsize );
            m_viewCoronal->SetWorldCoordinateInfo( worigin, wsize );
            m_view3D->SetWorldCoordinateInfo( worigin, wsize );
            lc_surface->AddLayer( sf, true );
          }
          else
          {
            lc_surface->SetWorldOrigin( lc_mri->GetWorldOrigin() );
            lc_surface->SetWorldSize( lc_mri->GetWorldSize() );
            lc_surface->SetWorldVoxelSize( lc_mri->GetWorldVoxelSize() );
            lc_surface->SetSlicePosition( lc_mri->GetSlicePosition() );
            lc_surface->AddLayer( sf );
          }
          // lc_surface->SetCursorRASPosition( lc_surface->GetSlicePosition() );
        }
        else
          lc_surface->AddLayer( layer );

        // m_fileHistory->AddFileToHistory( MyUtils::GetNormalizedFullPath( layer->GetFileName() ) );

        m_controlPanel->RaisePage( "Surfaces" );
      }

      m_viewAxial->SetInteractionMode( RenderView2D::IM_Navigate );
      m_viewCoronal->SetInteractionMode( RenderView2D::IM_Navigate );
      m_viewSagittal->SetInteractionMode( RenderView2D::IM_Navigate );
    }

    // Saving operation finished
    else if ( strg == "Save" )
    {
      cout << ( (LayerEditable*)layer )->GetFileName() << " saved successfully." << endl;
    }

    else if ( strg == "Rotate" )
    {
      m_bResampleToRAS = false;
      m_layerCollectionManager->RefreshSlices();
    }

    m_controlPanel->UpdateUI();

    RunScript();
  }
  else if ( event.GetInt() == 0 )  // just started
  {
    m_bProcessing = true;
    if ( strg == "Rotate" )
    {
      EnableControls( false );
    }

    m_statusBar->ActivateProgressBar();
    NeedRedraw();
    m_controlPanel->UpdateUI( true );
  }
  else
  {
    // if ( event.GetInt() > m_statusBar->m_gaugeBar->GetValue() )
    m_statusBar->m_gaugeBar->SetValue( event.GetInt() );
  }
}

void MainWindow::DoListenToMessage ( std::string const iMsg, void* iData, void* sender )
{
  if ( iMsg == "LayerAdded" || iMsg == "LayerRemoved" || iMsg == "LayerMoved" )
  {
    UpdateToolbars();
    if ( !GetLayerCollection( "MRI" )->IsEmpty() )
    {
      SetTitle( wxString( GetLayerCollection( "MRI" )->GetLayer( 0 )->GetName() ) + " - freeview" );
    }
    else if ( !GetLayerCollection( "Surface" )->IsEmpty() )
    {
      SetTitle( wxString( GetLayerCollection( "Surface" )->GetLayer( 0 )->GetName() ) + " - freeview" );
    }
    else
    {
      SetTitle( "freeview" );
    }
  }
  else if ( iMsg == "LayerObjectDeleted" )
  {
    if ( m_layerVolumeRef == iData )
      m_layerVolumeRef = NULL;
  }
  else if ( iMsg == "MRINotVisible" )
  {
    wxMessageDialog dlg( this, "Active volume is not visible. Please turn it on before editing.", "Error", wxOK | wxICON_ERROR );
    dlg.ShowModal();
  }
  else if ( iMsg == "MRINotEditable" )
  {
    wxMessageDialog dlg( this, "Active volume is not editable.", "Error", wxOK | wxICON_ERROR );
    dlg.ShowModal();
  }
  else if ( iMsg == "ROINotVisible" )
  {
    wxMessageDialog dlg( this, "Active ROI is not visible. Please turn it on before editing.", "Error", wxOK | wxICON_ERROR );
    dlg.ShowModal();
  }
}

void MainWindow::OnFileLoadDTI( wxCommandEvent& event )
{
  DialogLoadDTI dlg( this );
  dlg.Initialize( m_bResampleToRAS, GetLayerCollection( "MRI" )->IsEmpty() );
  dlg.SetLastDir( m_strLastDir );

  wxArrayString list;
  for ( int i = 0; i < m_fileHistory->GetMaxFiles(); i++ )
    list.Add( m_fileHistory->GetHistoryFile( i ) );
  dlg.SetRecentFiles( list );

  if ( dlg.ShowModal() != wxID_OK )
    return;

  this->LoadDTIFile( dlg.GetVectorFileName(), dlg.GetFAFileName(), dlg.GetRegFileName(), dlg.IsToResample() );
}

void MainWindow::LoadDTIFile( const wxString& fn_vector,
                              const wxString& fn_fa,
                              const wxString& reg_filename,
                              bool bResample )
{
  m_strLastDir = MyUtils::GetNormalizedPath( fn_fa );
  m_bResampleToRAS = bResample;

  LayerDTI* layer = new LayerDTI( m_layerVolumeRef );
  layer->SetResampleToRAS( bResample );
  wxString layerName = wxFileName( fn_vector ).GetName();
  if ( wxFileName( fn_fa ).GetExt().Lower() == "gz" )
    layerName = wxFileName( layerName ).GetName();
  layer->SetName( layerName.c_str() );
  layer->SetFileName( fn_fa.c_str() );
  layer->SetVectorFileName( fn_vector.c_str() );
  if ( !reg_filename.IsEmpty() )
  {
    wxFileName reg_fn( reg_filename );
    reg_fn.Normalize( wxPATH_NORM_ALL, m_strLastDir );
    layer->SetRegFileName( reg_fn.GetFullPath().c_str() );
  }

  /* LayerMRI* mri = (LayerMRI* )GetLayerCollection( "MRI" )->GetLayer( 0 );
   if ( mri )
   {
    layer->SetRefVolume( mri->GetSourceVolume() );
   }
  */
  WorkerThread* thread = new WorkerThread( this );
  thread->LoadVolume( layer );
}

void MainWindow::AddScript( const wxArrayString& script )
{
  m_scripts.push_back( script );
}

void MainWindow::RunScript()
{
  if ( m_scripts.size() == 0 )
    return;

  wxArrayString sa = m_scripts[0];
  m_scripts.erase( m_scripts.begin() );
  if ( sa[0] == "loadvolume" )
  {
    CommandLoadVolume( sa );
  }
  else if ( sa[0] == "loaddti" )
  {
    CommandLoadDTI( sa );
  }
  else if ( sa[0] == "loadsurface" )
  {
    CommandLoadSurface( sa );
  }
  else if ( sa[0] == "loadsurfacevector" )
  {
    CommandLoadSurfaceVector( sa );
  }
  else if ( sa[0] == "loadsurfacecurvature" )
  {
    CommandLoadSurfaceCurvature( sa );
  }
  else if ( sa[0] == "loadsurfaceoverlay" )
  {
    CommandLoadSurfaceOverlay( sa );
  }
  else if ( sa[0] == "loadroi" || sa[0] == "loadlabel" )
  {
    CommandLoadROI( sa );
  }
  else if ( sa[0] == "loadwaypoints" )
  {
    CommandLoadWayPoints( sa );
  }
  else if ( sa[0] == "screencapture" )
  {
    CommandScreenCapture( sa );
  }
  else if ( sa[0] == "quit" || sa[0] == "exit" )
  {
    Close();
  }
  else if ( sa[0] == "setviewport" )
  {
    CommandSetViewport( sa );
  }
  else if ( sa[0] == "zoom" )
  {
    CommandZoom( sa );
  }
  else if ( sa[0] == "ras" )
  {
    CommandSetRAS( sa );
  }
  else if ( sa[0] == "slice" )
  {
    CommandSetSlice( sa );
  }
  else if ( sa[0] == "setcolormap" )
  {
    CommandSetColorMap( sa );
  }
  else if ( sa[0] == "setlut" )
  {
    CommandSetLUT( sa );
  }
  else if ( sa[0] == "setsurfaceoverlaymethod" )
  {
    CommandSetSurfaceOverlayMethod( sa );
  }
  else if ( sa[0] == "setwaypointscolor" )
  {
    CommandSetWayPointsColor( sa );
  }
  else if ( sa[0] == "setwaypointsradius" )
  {
    CommandSetWayPointsRadius( sa );
  }
  else if ( sa[0] == "setdisplayvector" )
  {
    CommandSetDisplayVector( sa );
  }
}

void MainWindow::CommandLoadVolume( const wxArrayString& sa )
{
  wxArrayString sa_vol = MyUtils::SplitString( sa[1], ":" );
  wxString fn = sa_vol[0];
  wxString reg_fn;
  wxArrayString scales;
  wxString colormap = "grayscale";
  wxString colormap_scale = "grayscale";
  wxString lut_name;
  wxString vector_display = "no", vector_inversion = "none";
  for ( size_t i = 1; i < sa_vol.GetCount(); i++ )
  {
    wxString strg = sa_vol[i];
    int n = strg.Find( "=" );
    if ( n != wxNOT_FOUND )
    {
      if ( strg.Left( n ).Lower() == "colormap" )
      {
        colormap = strg.Mid( n + 1 ).Lower();
      }
      else if ( strg.Left( n ).Lower() == "grayscale" || 
                strg.Left( n ).Lower() == "heatscale" || 
                strg.Left( n ).Lower() == "jetscale" ) 
      {
        colormap_scale = strg.Left( n ).Lower();    // colormap scale might be different from colormap!
        scales = MyUtils::SplitString( strg.Mid(n+1), "," );
      }
      else if ( strg.Left( n ).Lower() == "lut" )
      {
        lut_name = strg.Mid( n + 1 );
        if ( lut_name.IsEmpty() )
        {
          cerr << "Missing lut name." << endl;
        }
      }
      else if ( strg.Left( n ).Lower() == "vector" )
      {
        vector_display = strg.Mid( n + 1 ).Lower();
        if ( vector_display.IsEmpty() )
        {
          cerr << "Missing vector display argument." << endl;
        }
      }
      else if ( strg.Left( n ).Lower() == "inversion" || strg.Left( n ).Lower() == "invert" )
      {
        vector_inversion = strg.Mid( n + 1 ).Lower();
        if ( vector_inversion.IsEmpty() )
        {
          cerr << "Missing vector inversion argument." << endl;
          vector_inversion = "none";
        }
      }
      else if ( strg.Left( n ).Lower() == "reg" )
      {
        reg_fn = strg.Mid( n + 1 );
      }
      else
      {
        cerr << "Unrecognized sub-option flag '" << strg << "'." << endl;
        return;
      }
    }
    else
    {
      cerr << "Unrecognized sub-option flag '" << strg << "'." << endl;
      return;
    }
  }
  bool bResample = true;
  if ( sa[ sa.GetCount()-1 ] == "nr" )
    bResample = false;

  if ( scales.size() > 0 || colormap != "grayscale" )
  {
    wxArrayString script;
    script.Add( "setcolormap" );
    script.Add( colormap );
    script.Add( colormap_scale ); 
    for ( size_t i = 0; i < scales.size(); i++ )
      script.Add( scales[i] );
    
    m_scripts.insert( m_scripts.begin(), script );
  }
  
  if ( !lut_name.IsEmpty() )
  {
    wxArrayString script;
    script.Add( "setlut" );
    script.Add( lut_name );
        
    m_scripts.insert( m_scripts.begin(), script );
  }
  
  if ( !vector_display.IsEmpty() && vector_display != "no" )
  {
    wxArrayString script;
    script.Add( "setdisplayvector" );
    script.Add( vector_display );
    script.Add( vector_inversion );
  
    m_scripts.insert( m_scripts.begin(), script );  
  }
  
  LoadVolumeFile( fn, reg_fn, bResample );
}

void MainWindow::CommandSetColorMap( const wxArrayString& sa )
{
  int nColorMap = LayerPropertiesMRI::Grayscale;;
  wxString strg = sa[1];
  if ( strg == "heat" || strg == "heatscale" )
    nColorMap = LayerPropertiesMRI::Heat;
  else if ( strg == "jet" || strg == "jetscale" )
    nColorMap = LayerPropertiesMRI::Jet;
  else if ( strg == "lut" )
    nColorMap = LayerPropertiesMRI::LUT;
  else if ( strg != "grayscale" )
    cerr << "Unrecognized colormap name '" << strg << "'." << endl;
  
  int nColorMapScale = LayerPropertiesMRI::Grayscale;
  strg = sa[2];
  if ( strg == "heatscale" )
    nColorMapScale = LayerPropertiesMRI::Heat;
  else if ( strg == "jetscale" )
    nColorMapScale = LayerPropertiesMRI::Jet;
  else if ( strg == "lut" )
    nColorMapScale = LayerPropertiesMRI::LUT;
  
  std::vector<double> pars;
  for ( size_t i = 3; i < sa.size(); i++ )
  {
    double dValue;
    if ( !sa[i].ToDouble( &dValue ) )
    {
      cerr << "Invalid color scale value(s). " << endl;
      break;
    }
    else
    {
      pars.push_back( dValue );
    }
  }
  
  SetVolumeColorMap( nColorMap, nColorMapScale, pars );
  
  ContinueScripts();
}


void MainWindow::CommandSetDisplayVector( const wxArrayString& cmd )
{
  if ( cmd[1].Lower() == "yes" || cmd[1].Lower() == "true" || cmd[1].Lower() == "1" )
  {
    LayerMRI* mri = (LayerMRI*)GetLayerCollection( "MRI" )->GetActiveLayer();
    if ( mri )
    {
      if ( mri->GetNumberOfFrames() < 3 )
      {
        cerr << "Volume has less than 3 frames. Can not display as vectors." << endl;
      }
      else
      {
        mri->GetProperties()->SetDisplayVector( true );  
      
        if ( cmd[2].Lower() != "none" )
        {
          if ( cmd[2].Lower() == "x" )
            mri->GetProperties()->SetVectorInversion( LayerPropertiesMRI::VI_X );
          else if ( cmd[2].Lower() == "y" )
            mri->GetProperties()->SetVectorInversion( LayerPropertiesMRI::VI_Y );
          else if ( cmd[2].Lower() == "z" )
            mri->GetProperties()->SetVectorInversion( LayerPropertiesMRI::VI_Z );
          else
            cerr << "Unknown inversion flag '" << cmd[2].c_str() << "'." << endl;
        }
      }
    }
  }
  
  ContinueScripts();
}

void MainWindow::CommandSetLUT( const wxArrayString& sa )
{
  LayerMRI* mri = (LayerMRI*)GetLayerCollection( "MRI" )->GetActiveLayer();
  if ( mri )
  {  
    COLOR_TABLE* ct = m_luts->LoadColorTable( sa[1].c_str() );
    /*
    if ( !ct )
    {
      ct = m_luts->LoadColorTable( sa[1].c_str() );
    }
    */
    if ( ct )
    {
      mri->GetProperties()->SetLUTCTAB( ct );
    }
  }
  
  ContinueScripts();
}

void MainWindow::ContinueScripts()
{
  // create a fake worker event to notify end of operation 
  // so scripts in queue will continue on
  wxCommandEvent event( wxEVT_COMMAND_MENU_SELECTED, ID_WORKER_THREAD );
  event.SetInt( -1 );
  event.SetString( "ByPass" );
  wxPostEvent( this, event );
}

void MainWindow::CommandLoadDTI( const wxArrayString& sa )
{
  bool bResample = true;
  if ( sa.GetCount() > 3 && sa[3] == "nr" )
    bResample = false;
  if ( sa.GetCount() > 2 )
  {
    wxArrayString sa_vol = MyUtils::SplitString( sa[1], ":" );
    wxString fn = sa_vol[0];
    wxString strg, reg_fn;
    if ( sa_vol.Count() > 1 )
      strg = sa_vol[1];
    int n;
    if ( ( n = strg.Find( "=" ) ) != wxNOT_FOUND && strg.Left( n ).Lower() == "reg" )
    {
      reg_fn = strg.Mid( n + 1 );
    }
    // cout << reg_fn.c_str() << endl;
    this->LoadDTIFile( fn, sa[2], reg_fn, bResample );
  }
}

void MainWindow::CommandLoadROI( const wxArrayString& cmd )
{
  LoadROIFile( cmd[1] );

  ContinueScripts();
}

void MainWindow::CommandLoadSurface( const wxArrayString& cmd )
{
  wxString fullfn = cmd[1];
  int nIgnoreStart = fullfn.find( "#" );
  int nIgnoreEnd = fullfn.find( "#", nIgnoreStart+1 );
  wxArrayString sa_fn = MyUtils::SplitString( fullfn, ":", nIgnoreStart, nIgnoreEnd - nIgnoreStart + 1 );
  wxString fn = sa_fn[0];
//  wxString fn_vector = "";
  for ( size_t k = 1; k < sa_fn.size(); k++ )
  {
    int n = sa_fn[k].Find( "=" );
    if ( n != wxNOT_FOUND  )
    {
      wxString subOption = sa_fn[k].Left( n ).Lower(); 
      wxString subArgu = sa_fn[k].Mid( n+1 );
      if ( subOption == "curv" || subOption == "curvature" )
      {
        // add script to load surface curvature file
        wxArrayString script;
        script.Add( "loadsurfacecurvature" );
        script.Add( subArgu );
        m_scripts.insert( m_scripts.begin(), script );
      }
      else if ( subOption == "overlay" )
      {
        // add script to load surface overlay files
        nIgnoreStart = subArgu.find( "#" );
        nIgnoreEnd = subArgu.find( "#", nIgnoreStart+1 );
        wxArrayString overlay_fns = MyUtils::SplitString( subArgu, ",", nIgnoreStart, nIgnoreEnd - nIgnoreStart + 1 );
        for ( int i = overlay_fns.size() - 1 ; i >= 0 ; i-- )
        {
          wxArrayString script;
          script.Add( "loadsurfaceoverlay" );          
          int nSubStart = overlay_fns[i].find( "#" );      
          int nSubEnd = overlay_fns[i].find( "#", nSubStart+1 );
          if ( nSubEnd == wxNOT_FOUND )
            nSubEnd = overlay_fns[i].Length() - 1;
          if ( nSubStart != wxNOT_FOUND )
            script.Add( overlay_fns[i].Left( nSubStart ) );
          else
            script.Add( overlay_fns[i] );
          m_scripts.insert( m_scripts.begin(), script );
          
          // if there are sub-options attached with overlay file, parse them
          wxString opt_strg;
          if ( nSubStart != wxNOT_FOUND )
            opt_strg = overlay_fns[i].Mid( nSubStart+1, nSubEnd - nSubStart - 1 ); 

          wxArrayString overlay_opts = MyUtils::SplitString( opt_strg, ":" );
          
          wxString method = "linearopaque";
          wxArrayString thresholds;
          for ( size_t j = 0; j < overlay_opts.GetCount(); j++ )
          {
            wxString strg = overlay_opts[j];
            if ( ( n = strg.Find( "=" ) ) != wxNOT_FOUND && strg.Left( n ).Lower() == "method" )
            {
              method = strg.Mid( n+1 ).Lower();
            }
            else if ( ( n = strg.Find( "=" ) ) != wxNOT_FOUND && strg.Left( n ).Lower() == "threshold" )
            {
              thresholds = MyUtils::SplitString( strg.Mid( n+1 ), "," );
            }
          }
          
          if ( method != "linearopaque" || !thresholds.IsEmpty() )
          {
            script.Clear();
            script.Add( "setsurfaceoverlaymethod" );
            script.Add( method );
            for ( size_t j = 0; j < thresholds.GetCount(); j++ )
              script.Add( thresholds[j] );
            
            // insert right AFTER loadsurfaceoverlay command
            m_scripts.insert( m_scripts.begin()+1, script );
          }
        }
      }
      else if ( subOption == "vector" )
      {
        // add script to load surface vector files
        wxArrayString vector_fns = MyUtils::SplitString( subArgu, "," );
        for ( int i = vector_fns.size() - 1 ; i >= 0 ; i-- )
        {
          wxArrayString script;
          script.Add( "loadsurfacevector" );
          script.Add( vector_fns[i] );
          m_scripts.insert( m_scripts.begin(), script );
        }
      }
      else
      {
        cerr << "Unrecognized sub-option flag '" << subOption << "'." << endl;
        return;
      }
    }
  }
  LoadSurfaceFile( fn );
}

void MainWindow::CommandSetSurfaceOverlayMethod( const wxArrayString& cmd )
{
  LayerSurface* surf = (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( surf )
  {
    SurfaceOverlay* overlay = surf->GetActiveOverlay();
    if ( overlay )
    {
      int nMethod = SurfaceOverlayProperties::CM_LinearOpaque;
      if ( cmd[1] == "linear" )
        nMethod = SurfaceOverlayProperties::CM_Linear;
      else if ( cmd[1] == "piecewise" )
        nMethod = SurfaceOverlayProperties::CM_Piecewise;
      else if ( cmd[1] != "linearopaque" )
      {
        cerr << "Unrecognized overlay method name '" << cmd[1] << "'." << endl;
        ContinueScripts();
        return;
      }
      
      overlay->GetProperties()->SetColorMethod( nMethod );
      
      double values[3];
      if ( cmd.GetCount() - 2 >= 3 )   // 3 values
      {
        if ( cmd[2].ToDouble( &(values[0]) ) &&
             cmd[3].ToDouble( &(values[1]) ) &&
             cmd[4].ToDouble( &(values[2]) ) )
        {
          overlay->GetProperties()->SetMinPoint( values[0] );
          overlay->GetProperties()->SetMidPoint( values[1] );
          overlay->GetProperties()->SetMaxPoint( values[2] );
        }
        else
        {
          cerr << "Invalid input for overlay threshold." << endl;
        }
      }   
      else if ( cmd.GetCount() - 2 == 2 )   // 2 values
      {
        if ( cmd[2].ToDouble( &(values[0]) ) &&
             cmd[3].ToDouble( &(values[1]) ) )
        {
          overlay->GetProperties()->SetMinPoint( values[0] );
          overlay->GetProperties()->SetMaxPoint( values[1] );
          overlay->GetProperties()->SetMidPoint( ( values[0] + values[1] ) / 2 );
        }
        else
        {
          cerr << "Invalid input for overlay threshold." << endl;
        }
      }  
      surf->UpdateOverlay();    
    }
  }
  
  ContinueScripts();
}

void MainWindow::CommandLoadSurfaceVector( const wxArrayString& cmd )
{
  LoadSurfaceVectorFile( cmd[1] );
}

void MainWindow::CommandLoadSurfaceCurvature( const wxArrayString& cmd )
{
  LoadSurfaceCurvatureFile( cmd[1] );
 
  ContinueScripts();
}

void MainWindow::CommandLoadSurfaceOverlay( const wxArrayString& cmd )
{
  LoadSurfaceOverlayFile( cmd[1] );

  ContinueScripts();
}

void MainWindow::CommandLoadWayPoints( const wxArrayString& cmd )
{
  wxArrayString options = MyUtils::SplitString( cmd[1], ":" );
  wxString fn = options[0];
  wxString color = "null";
  wxString spline_color = "null";
  wxString radius = "0";
  wxString spline_radius = "0";
  for ( size_t i = 1; i < options.GetCount(); i++ )
  {
    wxString strg = options[i];
    int n = strg.Find( "=" );
    if ( n != wxNOT_FOUND )
    {
      wxString option = strg.Left( n ).Lower();
      if ( option == "color" )
      {
        color = strg.Mid( n+1 );
      }
      else if ( option == "splinecolor" )
      {
        spline_color = strg.Mid( n+1 );
      }
      else if ( option == "radius" )
      {
        radius = strg.Mid( n+1 );
      }
      else if ( option == "splineradius" )
      {
        spline_radius = strg.Mid( n+1 );
      }
      else
      {
        cerr << "Unrecognized sub-option flag '" << strg << "'." << endl;
      }
    }
  }
  
  if ( color != "null" || spline_color != "null" )
  {
    wxArrayString script;
    script.Add( "setwaypointscolor" );
    script.Add( color );
    script.Add( spline_color ); 
    
    m_scripts.insert( m_scripts.begin(), script );
  }
  
  if ( radius != "0" || spline_radius != "0" )
  {
    wxArrayString script;
    script.Add( "setwaypointsradius" );
    script.Add( radius );
    script.Add( spline_radius ); 
    
    m_scripts.insert( m_scripts.begin(), script );
  }
  
  LoadWayPointsFile( fn );

  ContinueScripts();
}

void MainWindow::CommandSetWayPointsColor( const wxArrayString& cmd )
{
  LayerWayPoints* wp = (LayerWayPoints*)GetLayerCollection( "WayPoints" )->GetActiveLayer();
  if ( wp )
  {
    if ( cmd[1] != "null" )
    {
      wxColour color( cmd[1] );
      if ( !color.IsOk() )      
      {
        double rgb[3];
        wxArrayString rgb_strs = MyUtils::SplitString( cmd[1], "," );
        if ( rgb_strs.GetCount() < 3 || 
             !rgb_strs[0].ToDouble( &(rgb[0]) ) ||
             !rgb_strs[1].ToDouble( &(rgb[1]) ) ||
             !rgb_strs[2].ToDouble( &(rgb[2]) ) )
        {
          cerr << cerr << "Invalid color name or value " << cmd[1].c_str() << endl;
        }
        else
        {
          color.Set( (int)( rgb[0]*255 ), (int)( rgb[1]*255 ), (int)( rgb[2]*255 ) );
        }
      }
      
      if ( color.IsOk() )
        wp->GetProperties()->SetColor( color.Red()/255.0, color.Green()/255.0, color.Blue()/255.0 );
      else
        cerr << "Invalid color name or value " << cmd[1].c_str() << endl;
    }
    
    if ( cmd[2] != "null" )
    {
      wxColour color( cmd[2] );
      if ( !color.IsOk() )      
      {
        double rgb[3];
        wxArrayString rgb_strs = MyUtils::SplitString( cmd[2], "," );
        if ( rgb_strs.GetCount() < 3 || 
             !rgb_strs[0].ToDouble( &(rgb[0]) ) ||
             !rgb_strs[1].ToDouble( &(rgb[1]) ) ||
             !rgb_strs[2].ToDouble( &(rgb[2]) ) )
        {
          cerr << cerr << "Invalid color name or value " << cmd[2].c_str() << endl;
        }
        else
        {
          color.Set( (int)( rgb[0]*255 ), (int)( rgb[1]*255 ), (int)( rgb[2]*255 ) );
        }
      }
      
      if ( color.IsOk() )
        wp->GetProperties()->SetSplineColor( color.Red()/255.0, color.Green()/255.0, color.Blue()/255.0 );
      else
        cerr << "Invalid color name or value " << cmd[1].c_str() << endl;
    }
  }
  
  ContinueScripts();
}

void MainWindow::CommandSetWayPointsRadius( const wxArrayString& cmd )
{
  LayerWayPoints* wp = (LayerWayPoints*)GetLayerCollection( "WayPoints" )->GetActiveLayer();
  if ( wp )
  {
    if ( cmd[1] != "0" )
    {
      double dvalue;
      if ( cmd[1].ToDouble( &dvalue ) ) 
        wp->GetProperties()->SetRadius( dvalue );
      else
        cerr << "Invalid way points radius." << endl;
    }
    
    if ( cmd[2] != "0" )
    {
      double dvalue;
      if ( cmd[2].ToDouble( &dvalue ) )
        wp->GetProperties()->SetSplineRadius( dvalue );
      else
        cerr << "Invalid spline radius." << endl;
    }
  }
  
  ContinueScripts();
}

void MainWindow::CommandScreenCapture( const wxArrayString& cmd )
{
  m_viewRender[m_nMainView]->SaveScreenshot( cmd[1].c_str(), 
                                             m_settingsScreenshot.Magnification, 
                                             m_settingsScreenshot.AntiAliasing );

  ContinueScripts();
}

void MainWindow::CommandSetViewport( const wxArrayString& cmd )
{
  if ( cmd[1] == "x" )
    SetMainView( MV_Sagittal );
  else if ( cmd[1] == "y" )
    SetMainView( MV_Coronal );
  else if ( cmd[1] == "z" )
    SetMainView( MV_Axial );
  else if ( cmd[1] == "3d" )
    SetMainView( MV_3D );

  ContinueScripts();
}

void MainWindow::CommandZoom( const wxArrayString& cmd )
{
  double dValue;
  cmd[1].ToDouble( &dValue );
  if ( m_nMainView >= 0 )
  {
    m_viewRender[m_nMainView]->Zoom( dValue );
  }
  
  ContinueScripts();
}

void MainWindow::CommandSetRAS( const wxArrayString& cmd )
{
  double ras[3];
  if ( cmd[1].ToDouble( &(ras[0]) ) &&
       cmd[2].ToDouble( &(ras[1]) ) &&
       cmd[3].ToDouble( &(ras[2]) ) )
  {
    GetLayerCollection( "MRI" )->SetCursorRASPosition( ras );
    m_layerCollectionManager->SetSlicePosition( ras );
  }
  else
  {
    cerr << "Invalid input values for RAS coordinates. " << endl;
  }

  ContinueScripts();
}


void MainWindow::CommandSetSlice( const wxArrayString& cmd )
{
  LayerCollection* lc_mri = GetLayerCollection( "MRI" );
  if ( !lc_mri->IsEmpty() )
  {
    LayerMRI* mri = (LayerMRI*)lc_mri->GetLayer( lc_mri->GetNumberOfLayers()-1 );;
    long x, y, z;
    if ( cmd[1].ToLong( &x ) && cmd[2].ToLong( &y ) && cmd[3].ToLong( &z ) )
    {
      int slice[3] = { x, y, z };
      double ras[3];
      mri->OriginalIndexToRAS( slice, ras );
      mri->RASToTarget( ras, ras );
      
      lc_mri->SetCursorRASPosition( ras );
      m_layerCollectionManager->SetSlicePosition( ras );
    }
    else
    {
      cerr << "Invalide slice number(s). " << endl;
    }
  }
  else
  {
    cerr << "No volume was loaded. Set slice failed." << endl;
  }

  ContinueScripts();
}

void MainWindow::OnSpinBrushSize( wxSpinEvent& event )
{
  m_propertyBrush->SetBrushSize( event.GetInt() );
}

void MainWindow::OnSpinBrushTolerance( wxSpinEvent& event )
{
  m_propertyBrush->SetBrushTolerance( event.GetInt() );
}

void MainWindow::OnChoiceBrushTemplate( wxCommandEvent& event )
{
// wxChoice* choiceTemplate = XRCCTRL( *m_toolbarBrush, "ID_CHOICE_TEMPLATE", wxChoice );
// LayerEditable* layer = (LayerEditable*)(void*)choiceTemplate->GetClientData( event.GetSelection() );
// if ( layer )
//  m_propertyBrush->SetReferenceLayer( layer );

  UpdateToolbars();
}

void MainWindow::OnCheckBrushTemplate( wxCommandEvent& event )
{
  UpdateToolbars();
}

int MainWindow::GetActiveViewId()
{
  wxWindow* wnd = FindFocus();
  int nId = -1;
  if ( wnd == m_viewSagittal )
    nId = 0;
  else if ( wnd == m_viewCoronal )
    nId = 1;
  else if ( wnd == m_viewAxial )
    nId = 2;
  else if ( wnd == m_view3D )
    nId = 3;

  if ( nId >= 0 )
    m_nPrevActiveViewId = nId;

  return nId;
}

RenderView* MainWindow::GetActiveView()
{
  int nId = GetActiveViewId();
  if ( nId >= 0 )
    return m_viewRender[nId];
  else
    return NULL;
}

RenderView* MainWindow::GetPreviousActiveView()
{
  if ( m_nPrevActiveViewId < 0 )
    return NULL;
  else
    return m_viewRender[ m_nPrevActiveViewId ];
}

void MainWindow::OnFileSaveScreenshot( wxCommandEvent& event )
{
  int nId = GetActiveViewId();
  if ( nId < 0 )
    nId = m_nPrevActiveViewId;

  if ( nId < 0 )
    return;

  wxString fn;
  wxFileDialog dlg( this, _("Save screenshot as"), m_strLastDir, _(""),
                    _T("PNG files (*.png)|*.png|JPEG files (*.jpg;*.jpeg)|*.jpg;*.jpeg|TIFF files (*.tif;*.tiff)|*.tif;*.tiff|Bitmap files (*.bmp)|*.bmp|PostScript files (*.ps)|*.ps|VRML files (*.wrl)|*.wrl|All files (*.*)|*.*"),
                    wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
  dlg.SetFilterIndex( m_nScreenshotFilterIndex );
  if ( dlg.ShowModal() == wxID_OK )
  {
    fn = dlg.GetPath();
  }

  if ( !fn.IsEmpty() )
  {
    m_strLastDir = MyUtils::GetNormalizedPath( fn );
    m_nScreenshotFilterIndex = dlg.GetFilterIndex();
    m_viewRender[nId]->SaveScreenshot( fn.c_str(), 
                                       m_settingsScreenshot.Magnification,
                                       m_settingsScreenshot.AntiAliasing );
  }
}

void MainWindow::OnFileSaveScreenshotUpdateUI( wxUpdateUIEvent& event )
{
  event.Enable( ( GetActiveViewId() >= 0 || m_nPrevActiveViewId >= 0 ) && m_layerCollectionManager->HasAnyLayer() );
}


void MainWindow::OnFileLoadSurface( wxCommandEvent& event )
{
  LoadSurface();
}

void MainWindow::LoadSurface()
{
  {
    wxFileDialog dlg( this, _("Open surface file"), m_strLastDir, _(""),
                      _T("Surface files (*.*)|*.*"),
                      wxFD_OPEN );
    if ( dlg.ShowModal() == wxID_OK )
    {
      this->LoadSurfaceFile( dlg.GetPath() );
    }
  }
}

void MainWindow::LoadSurfaceFile( const wxString& filename, const wxString& fn_vector )
{
  m_strLastDir = MyUtils::GetNormalizedPath( filename );

  LayerSurface* layer = new LayerSurface( m_layerVolumeRef );
  wxFileName fn( filename );
  wxString layerName = fn.GetFullName();
// if ( fn.GetExt().Lower() == "gz" )
//  layerName = wxFileName( layerName ).GetName();
  layer->SetName( layerName.c_str() );
  layer->SetFileName( fn.GetFullPath().c_str() );
  layer->SetVectorFileName( fn_vector.c_str() );

  WorkerThread* thread = new WorkerThread( this );
  thread->LoadSurface( layer );
}


void MainWindow::LoadSurfaceVector()
{
  wxFileDialog dlg( this, _("Open surface file as vector"), m_strLastDir, _(""),
                    _T("Surface files (*.*)|*.*"),
                    wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    this->LoadSurfaceVectorFile( dlg.GetPath() );
  }
}


void MainWindow::LoadSurfaceVectorFile( const wxString& filename )
{
  wxString fn = filename;
  if ( fn.Contains( "/" ) )
    fn = MyUtils::GetNormalizedFullPath( filename );

  LayerSurface* layer = ( LayerSurface* )GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
  {
    layer->SetVectorFileName( fn.c_str() );

    WorkerThread* thread = new WorkerThread( this );
    thread->LoadSurfaceVector( layer );
    
    m_strLastDir = MyUtils::GetNormalizedPath( filename );
  }
}

void MainWindow::LoadSurfaceCurvature()
{
  wxFileDialog dlg( this, _("Open curvature file"), m_strLastDir, _(""),
                    _T("Curvature files (*.*)|*.*"),
                    wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    this->LoadSurfaceCurvatureFile( dlg.GetPath() );
  }
}

void MainWindow::LoadSurfaceCurvatureFile( const wxString& filename )
{
  wxString fn = filename;
  if ( fn.Contains( "/" ) )
    fn = MyUtils::GetNormalizedFullPath( filename );
  LayerSurface* layer = ( LayerSurface* )GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
  {
    if ( layer->LoadCurvatureFromFile( fn.c_str() ) )
      m_strLastDir = MyUtils::GetNormalizedPath( filename );
  }
}


void MainWindow::LoadSurfaceOverlay()
{
  wxFileDialog dlg( this, _("Open overlay file"), m_strLastDir, _(""),
                    _T("Overlay files (*.*)|*.*"),
                    wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    this->LoadSurfaceOverlayFile( dlg.GetPath() );
  }
}

void MainWindow::LoadSurfaceOverlayFile( const wxString& filename )
{
  wxString fn = filename;
  if ( fn.Contains( "/" ) )
    fn = MyUtils::GetNormalizedFullPath( filename );
  LayerSurface* layer = ( LayerSurface* )GetLayerCollection( "Surface" )->GetActiveLayer();
  if ( layer )
  {
    if ( layer->LoadOverlayFromFile( fn.c_str() ) )
       m_strLastDir = MyUtils::GetNormalizedPath( filename );
  }
}

void MainWindow::OnToolRotateVolume( wxCommandEvent& event )
{
  if ( !m_dlgRotateVolume )
    m_dlgRotateVolume = new DialogRotateVolume( this );

  if ( !m_dlgRotateVolume->IsVisible() )
  {
    wxMessageDialog dlg( this, "Rotation can only apply to volume for now. If you data includes ROI/Surface/Way Points, please do not use this feature yet.", "Warning", wxOK );
    dlg.ShowModal();
    m_dlgRotateVolume->Show();
  }
}

void MainWindow::OnToolRotateVolumeUpdateUI( wxUpdateUIEvent& event )
{
// event.Check( m_dlgRotateVolume && m_dlgRotateVolume->IsShown() );
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() && !IsProcessing() );
}


void MainWindow::OnToolCreateOptimalVolume( wxCommandEvent& event )
{
  DialogOptimalVolume dlg( this, GetLayerCollection( "MRI" ) );
  if ( dlg.ShowModal() == wxID_OK )
  {
    std::vector<LayerMRI*> layers = dlg.GetSelectedLayers();
    LayerOptimal* layer_new = new LayerOptimal( layers[0] );
    layer_new->Create( dlg.GetLabelVolume(), layers );

    layer_new->SetName( dlg.GetVolumeName().c_str() );
    GetLayerCollection( "MRI" )->AddLayer( layer_new );

    m_controlPanel->RaisePage( "Volumes" );
  }
}

void MainWindow::OnToolCreateOptimalVolumeUpdateUI( wxUpdateUIEvent& event )
{
// event.Check( m_dlgRotateVolume && m_dlgRotateVolume->IsShown() );
  event.Enable( GetLayerCollection( "MRI" )->GetNumberOfLayers() > 1 && !IsProcessing() );
}


void MainWindow::EnableControls( bool bEnable )
{
  m_controlPanel->Enable( bEnable );
  if ( m_dlgRotateVolume )
    m_dlgRotateVolume->Enable( bEnable );
}

void MainWindow::OnMouseEnterWindow( wxMouseEvent& event )
{
  if ( m_viewAxial->GetInteractionMode() != RenderView2D::IM_Navigate && FindFocus() == m_toolWindowEdit )
  {
    this->Raise();
    SetFocus();
  }
}

void MainWindow::OnViewHistogram( wxCommandEvent& event )
{
  if ( m_wndHistogram->IsVisible() )
    m_wndHistogram->Hide();
  else
	 m_wndHistogram->Show( !m_wndHistogram->IsVisible() );
}

void MainWindow::OnViewHistogramUpdateUI( wxUpdateUIEvent& event )
{
  event.Check( m_wndHistogram->IsVisible() );
  event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() || !GetLayerCollection( "Surface" )->IsEmpty() );
}

void MainWindow::ConfigureOverlay()
{
  m_wndOverlayConfiguration->ShowWindow( (LayerSurface*)GetLayerCollection( "Surface" )->GetActiveLayer() );
}

void MainWindow::SetVolumeColorMap( int nColorMap, int nColorMapScale, std::vector<double>& scales )
{
  if ( GetLayerCollection( "MRI" )->GetActiveLayer() )
  {
    LayerPropertiesMRI* p = ( (LayerMRI*)GetLayerCollection( "MRI" )->GetActiveLayer() )->GetProperties();
    p->SetColorMap( (LayerPropertiesMRI::ColorMapType) nColorMap );
    switch ( nColorMapScale )
    {
      case LayerPropertiesMRI::Grayscale:
        if ( scales.size() >= 2 )
        {
          p->SetMinMaxGrayscaleWindow( scales[0], scales[1] );
        }
        else if ( !scales.empty() )
          cerr << "Need 2 values for grayscale." << endl;
        break;
      case LayerPropertiesMRI::Heat:
        if ( scales.size() >= 3 )
        {
          p->SetHeatScaleMinThreshold( scales[0] );
          p->SetHeatScaleMidThreshold( scales[1] );
          p->SetHeatScaleMaxThreshold( scales[2] );
        }
        else if ( !scales.empty() )
          cerr << "Need 3 values for heatscale." << endl;
        break;
      case LayerPropertiesMRI::Jet:
        if ( scales.size() >= 2 )
        {
          p->SetMinMaxJetScaleWindow( scales[0], scales[1] );
        }
        else if ( !scales.empty() )
          cerr << "Need 2 values for jetscale." << endl;
        break;
      case LayerPropertiesMRI::LUT:
        if ( scales.size() >= 1 )
        {
        }
        else if ( !scales.empty() )
          cerr << "Need a value for lut." << endl;
        break;
    }
  }
}

void MainWindow::LoadLUT()
{
  wxFileDialog dlg( this, _("Load lookup table file"), m_strLastDir, _(""),
                  _T("LUT files (*.*)|*.*"),
                  wxFD_OPEN );
  if ( dlg.ShowModal() == wxID_OK )
  {
    m_scripts.clear();
    wxArrayString sa;
    sa.Add( "setlut" );
    sa.Add( dlg.GetPath().c_str() );
    CommandSetLUT( sa );
  }
}
