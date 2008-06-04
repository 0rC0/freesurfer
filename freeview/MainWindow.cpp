/**
 * @file  MainWindow.cpp
 * @brief Main window.
 *
 */
/*
 * Original Author: Ruopeng Wang
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2008/06/04 20:43:25 $
 *    $Revision: 1.6.2.1 $
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
#include "LayerROI.h"
#include "LayerDTI.h"
#include "LayerSurface.h"
#include "Interactor2DROIEdit.h"
#include "Interactor2DVoxelEdit.h"
#include "DialogPreferences.h"
#include "DialogLoadDTI.h"
#include "WindowQuickReference.h"
#include "StatusBar.h"
#include "DialogNewVolume.h"
#include "DialogNewROI.h"
#include "DialogLoadVolume.h"
#include "WorkerThread.h"
#include "MyUtils.h"
#include "LUTDataHolder.h"
#include "BrushProperty.h"

#define	CTRL_PANEL_WIDTH	240

#define ID_FILE_RECENT1		10001

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------
IMPLEMENT_CLASS(MainWindow, wxFrame)
BEGIN_EVENT_TABLE(MainWindow, wxFrame)
    EVT_MENU		( XRCID( "ID_FILE_OPEN" ),				MainWindow::OnFileOpen )
	EVT_MENU		( XRCID( "ID_FILE_NEW" ),				MainWindow::OnFileNew )    
    EVT_UPDATE_UI	( XRCID( "ID_FILE_NEW" ),				MainWindow::OnFileNewUpdateUI ) 
	EVT_MENU		( XRCID( "ID_FILE_SAVE" ),				MainWindow::OnFileSave )    
    EVT_UPDATE_UI	( XRCID( "ID_FILE_SAVE" ),				MainWindow::OnFileSaveUpdateUI ) 
	EVT_MENU		( XRCID( "ID_FILE_SAVE_AS" ),			MainWindow::OnFileSaveAs )    
    EVT_UPDATE_UI	( XRCID( "ID_FILE_SAVE_AS" ),			MainWindow::OnFileSaveAsUpdateUI ) 
    EVT_MENU		( XRCID( "ID_FILE_EXIT") ,				MainWindow::OnFileExit )
    EVT_MENU_RANGE	( wxID_FILE1, wxID_FILE1+16,			MainWindow::OnFileRecent )    
    EVT_MENU		( XRCID( "ID_FILE_NEW_ROI" ),			MainWindow::OnFileNewROI )      
    EVT_UPDATE_UI	( XRCID( "ID_FILE_NEW_ROI" ),			MainWindow::OnFileNewROIUpdateUI )    
    EVT_MENU		( XRCID( "ID_FILE_LOAD_ROI" ),			MainWindow::OnFileLoadROI )      
    EVT_UPDATE_UI	( XRCID( "ID_FILE_LOAD_ROI" ),			MainWindow::OnFileLoadROIUpdateUI )    
    EVT_MENU		( XRCID( "ID_FILE_SAVE_ROI" ),			MainWindow::OnFileSaveROI )      
    EVT_UPDATE_UI	( XRCID( "ID_FILE_SAVE_ROI" ),			MainWindow::OnFileSaveROIUpdateUI )  
    EVT_MENU		( XRCID( "ID_FILE_SAVE_ROI_AS" ),		MainWindow::OnFileSaveROIAs )      
    EVT_UPDATE_UI	( XRCID( "ID_FILE_SAVE_ROI_AS" ),		MainWindow::OnFileSaveROIAsUpdateUI )   
    EVT_MENU		( XRCID( "ID_FILE_LOAD_DTI" ),			MainWindow::OnFileLoadDTI )  
    EVT_MENU		( XRCID( "ID_FILE_SCREENSHOT" ),		MainWindow::OnFileSaveScreenshot )           
    EVT_UPDATE_UI	( XRCID( "ID_FILE_SCREENSHOT" ),		MainWindow::OnFileSaveScreenshotUpdateUI ) 
    EVT_MENU		( XRCID( "ID_FILE_LOAD_SURFACE" ),		MainWindow::OnFileLoadSurface ) 
    EVT_MENU		( XRCID( "ID_MODE_NAVIGATE" ),			MainWindow::OnModeNavigate )  
    EVT_UPDATE_UI	( XRCID( "ID_MODE_NAVIGATE" ),			MainWindow::OnModeNavigateUpdateUI )    
    EVT_MENU		( XRCID( "ID_MODE_VOXEL_EDIT" ),		MainWindow::OnModeVoxelEdit )  
    EVT_UPDATE_UI	( XRCID( "ID_MODE_VOXEL_EDIT" ),		MainWindow::OnModeVoxelEditUpdateUI )    
    EVT_MENU		( XRCID( "ID_MODE_ROI_EDIT" ),			MainWindow::OnModeROIEdit )  
    EVT_UPDATE_UI	( XRCID( "ID_MODE_ROI_EDIT" ),			MainWindow::OnModeROIEditUpdateUI ) 
    EVT_MENU		( XRCID( "ID_ACTION_ROI_FREEHAND" ),	MainWindow::OnActionROIFreehand )
    EVT_UPDATE_UI	( XRCID( "ID_ACTION_ROI_FREEHAND" ),	MainWindow::OnActionROIFreehandUpdateUI )
    EVT_MENU		( XRCID( "ID_ACTION_ROI_FILL" ),		MainWindow::OnActionROIFill )
    EVT_UPDATE_UI	( XRCID( "ID_ACTION_ROI_FILL" ),		MainWindow::OnActionROIFillUpdateUI )
    EVT_MENU		( XRCID( "ID_ACTION_ROI_POLYLINE" ),	MainWindow::OnActionROIPolyline )
    EVT_UPDATE_UI	( XRCID( "ID_ACTION_ROI_POLYLINE" ),	MainWindow::OnActionROIPolylineUpdateUI )
    EVT_MENU		( XRCID( "ID_ACTION_VOXEL_FREEHAND" ),	MainWindow::OnActionVoxelFreehand )
    EVT_UPDATE_UI	( XRCID( "ID_ACTION_VOXEL_FREEHAND" ),	MainWindow::OnActionVoxelFreehandUpdateUI )
    EVT_MENU		( XRCID( "ID_ACTION_VOXEL_FILL" ),		MainWindow::OnActionVoxelFill )
    EVT_UPDATE_UI	( XRCID( "ID_ACTION_VOXEL_FILL" ),		MainWindow::OnActionVoxelFillUpdateUI )
    EVT_MENU		( XRCID( "ID_ACTION_VOXEL_POLYLINE" ),	MainWindow::OnActionVoxelPolyline )
    EVT_UPDATE_UI	( XRCID( "ID_ACTION_VOXEL_POLYLINE" ),	MainWindow::OnActionVoxelPolylineUpdateUI )
    EVT_MENU		( XRCID( "ID_EDIT_COPY" ),				MainWindow::OnEditCopy )
    EVT_UPDATE_UI	( XRCID( "ID_EDIT_COPY" ),				MainWindow::OnEditCopyUpdateUI )
    EVT_MENU		( XRCID( "ID_EDIT_PASTE" ),				MainWindow::OnEditPaste )
    EVT_UPDATE_UI	( XRCID( "ID_EDIT_PASTE" ),				MainWindow::OnEditPasteUpdateUI )
    EVT_MENU		( XRCID( "ID_EDIT_UNDO" ),				MainWindow::OnEditUndo )
    EVT_UPDATE_UI	( XRCID( "ID_EDIT_UNDO" ),				MainWindow::OnEditUndoUpdateUI )
    EVT_MENU		( XRCID( "ID_EDIT_REDO" ),				MainWindow::OnEditRedo )
    EVT_UPDATE_UI	( XRCID( "ID_EDIT_REDO" ),				MainWindow::OnEditRedoUpdateUI )
    EVT_MENU		( XRCID( "ID_EDIT_PREFERENCES" ),		MainWindow::OnEditPreferences )
    EVT_MENU		( XRCID( "ID_VIEW_LAYOUT_1X1" ),		MainWindow::OnViewLayout1X1 )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_LAYOUT_1X1" ),		MainWindow::OnViewLayout1X1UpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_LAYOUT_2X2" ),		MainWindow::OnViewLayout2X2 )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_LAYOUT_2X2" ),		MainWindow::OnViewLayout2X2UpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_LAYOUT_1N3" ),		MainWindow::OnViewLayout1N3 )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_LAYOUT_1N3" ),		MainWindow::OnViewLayout1N3UpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_SAGITTAL" ),			MainWindow::OnViewSagittal )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_SAGITTAL" ),			MainWindow::OnViewSagittalUpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_CORONAL" ),			MainWindow::OnViewCoronal )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_CORONAL" ),			MainWindow::OnViewCoronalUpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_AXIAL" ),				MainWindow::OnViewAxial )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_AXIAL" ),				MainWindow::OnViewAxialUpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_3D" ),				MainWindow::OnView3D )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_3D" ),				MainWindow::OnView3DUpdateUI )
    EVT_MENU		( XRCID( "ID_VIEW_RESET" ),				MainWindow::OnViewReset )
    EVT_UPDATE_UI	( XRCID( "ID_VIEW_RESET" ),				MainWindow::OnViewResetUpdateUI )
    EVT_MENU		( XRCID( "ID_HELP_QUICK_REF" ),			MainWindow::OnHelpQuickReference )
    EVT_MENU		( XRCID( "ID_HELP_ABOUT" ),				MainWindow::OnHelpAbout )
/*    EVT_SASH_DRAGGED_RANGE(ID_LOG_WINDOW, ID_LOG_WINDOW, MainWindow::OnSashDrag)
	EVT_IDLE		(MainWindow::OnIdle)
	EVT_MENU		(XRCID("ID_EVENT_LOAD_DATA"),	MainWindow::OnWorkerEventLoadData)
	EVT_MENU		(XRCID("ID_EVENT_CALCULATE_MATRIX"),	MainWindow::OnWorkerEventCalculateMatrix)*/	

	EVT_MENU		( ID_WORKER_THREAD,						MainWindow::OnWorkerThreadResponse )
	EVT_SPINCTRL	( XRCID( "ID_SPIN_BRUSH_SIZE" ),		MainWindow::OnSpinBrushSize )
	EVT_SPINCTRL	( XRCID( "ID_SPIN_BRUSH_TOLERANCE" ),	MainWindow::OnSpinBrushTolerance )
	EVT_CHECKBOX	( XRCID( "ID_CHECK_TEMPLATE" ), 		MainWindow::OnCheckBrushTemplate )
	EVT_CHOICE		( XRCID( "ID_CHOICE_TEMPLATE" ), 		MainWindow::OnChoiceBrushTemplate )
			
    EVT_ACTIVATE	( MainWindow::OnActivate )
    EVT_CLOSE		( MainWindow::OnClose )   
	EVT_KEY_DOWN    ( MainWindow::OnKeyDown )
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

// frame constructor
MainWindow::MainWindow() : Listener( "MainWindow" ), Broadcaster( "MainWindow" )
{
	m_bLoading = false;
	m_bSaving = false;
	m_bResampleToRAS = true;
	m_bToUpdateToolbars = false;
	m_nPrevActiveViewId = -1;
	m_luts = new LUTDataHolder();		
	m_propertyBrush = new BrushProperty();
	
    wxXmlResource::Get()->LoadFrame( this, NULL, wxT("ID_MAIN_WINDOW") );
	
	// must be created before the following controls	
	m_layerCollectionManager = new LayerCollectionManager();
    
	m_panelToolbarHolder = XRCCTRL( *this, "ID_PANEL_HOLDER", wxPanel );
//	wxBoxSizer* sizer = (wxBoxSizer*)panelHolder->GetSizer(); //new wxBoxSizer( wxVERTICAL );	
    
    // create the main splitter window
//	m_splitterMain = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );
	m_splitterMain = XRCCTRL( *this, "ID_SPLITTER_MAIN", wxSplitterWindow );
	m_splitterMain->SetMinimumPaneSize( 80 );
//	sizer->Add( m_splitterMain, 1, wxEXPAND );
	
	m_toolbarVoxelEdit = XRCCTRL( *this, "ID_TOOLBAR_VOXEL_EDIT", wxToolBar );
	m_toolbarROIEdit = XRCCTRL( *this, "ID_TOOLBAR_ROI_EDIT", wxToolBar );	
	m_toolbarBrush = XRCCTRL( *this, "ID_TOOLBAR_BRUSH", wxToolBar );	
			
//	this->SetSizer( sizer );
//	sizer->Add( ( wxToolBar* )XRCCTRL( *this, "m_toolBar2", wxToolBar ), 0, wxEXPAND );
	
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
	m_viewRender[0] = m_viewSagittal;
	m_viewRender[1] = m_viewCoronal;
	m_viewRender[2] = m_viewAxial;
	m_viewRender[3] = m_view3D;

	m_layerCollectionManager->AddListener( m_viewAxial );
	m_layerCollectionManager->AddListener( m_viewSagittal );
	m_layerCollectionManager->AddListener( m_viewCoronal );
	m_layerCollectionManager->AddListener( m_view3D );	
	m_layerCollectionManager->GetLayerCollection( "MRI" )->AddListener( m_pixelInfoPanel );
	m_layerCollectionManager->AddListener( this );
	
	m_wndQuickReference = new WindowQuickReference( this );
	m_wndQuickReference->Hide();
	
	m_statusBar = new StatusBar( this );
	SetStatusBar( m_statusBar );
	PositionStatusBar();

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
	//	m_splitterSub->SetSashPosition( config->Read( _T("/MainWindow/SplitterPositionSub"), 50L ) );	
	//	m_splitterSub->UpdateSize();
		m_strLastDir = config->Read( _T("/MainWindow/LastDir"), "");
		m_fileHistory->Load( *config );
		m_fileHistory->UseMenu( fileMenu );
		m_fileHistory->AddFilesToMenu( fileMenu );
		m_nViewLayout = config->Read( _T("/MainWindow/ViewLayout"), m_nViewLayout );
		m_nMainView = config->Read( _T("/MainWindow/MainView"), m_nMainView );
		
		wxColour color = wxColour( config->Read( _T("RenderWindow/BackgroundColor"), "rgb(0,0,0" ) );
		m_viewAxial->SetBackgroundColor( color );
		m_viewSagittal->SetBackgroundColor( color );
		m_viewCoronal->SetBackgroundColor( color );
		m_view3D->SetBackgroundColor( color );
		
		m_nScreenshotFilterIndex = config->Read( _T("/MainWindow/ScreenshotFilterIndex"), 0L );
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
	if ( IsSaving() || IsLoading() )
	{
		wxMessageDialog dlg( this, "There is on-going data loading or saving process. Please wait till it finishes before closing.", "Quit", wxOK );
		dlg.ShowModal();
		return;
	}
	
	LayerCollection* lc_mri = GetLayerCollection( "MRI" );
	LayerCollection* lc_roi = GetLayerCollection( "ROI" );
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
		config->Write( _T("/MainWindow/SplitterPosition"), m_splitterMain->GetSashPosition() );
		config->Write( _T("/MainWindow/SplitterPositionSub"), m_splitterSub->GetSashPosition() );		
		config->Write( _T("/MainWindow/LastDir"), m_strLastDir );
		config->Write( _T("/MainWindow/ViewLayout"), m_nViewLayout );
		config->Write( _T("/MainWindow/MainView"), m_nMainView );
		config->Write( _T("MainWindow/ScreenshotFilterIndex"), m_nScreenshotFilterIndex );
		
		config->Write( _T("RenderWindow/BackgroundColor"), m_viewAxial->GetBackgroundColor().GetAsString( wxC2S_CSS_SYNTAX ) );
		
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
	LayerMRI* layer_new = new LayerMRI();
	layer_new->Create( dlg.GetTemplate(), dlg.GetCopyVoxel() );
	layer_new->SetName( dlg.GetVolumeName().c_str() );
	col_mri->AddLayer( layer_new );

	m_controlPanel->RaisePage( "Volumes" );
	
//	m_viewAxial->SetInteractionMode( RenderView2D::IM_ROIEdit );
//	m_viewCoronal->SetInteractionMode( RenderView2D::IM_ROIEdit );
//	m_viewSagittal->SetInteractionMode( RenderView2D::IM_ROIEdit );
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
	if ( GetLayerCollection( "MRI" )->IsEmpty() )
	{
		DialogLoadVolume dlg( this );
		dlg.SetLastDir( m_strLastDir );
		wxArrayString list;
		for ( int i = 0; i < m_fileHistory->GetMaxFiles(); i++ )
			list.Add( m_fileHistory->GetHistoryFile( i ) );
		dlg.SetRecentFiles( list );
		if ( dlg.ShowModal() == wxID_OK )
		{
			this->LoadVolumeFile( dlg.GetVolumeFileName(), dlg.IsToResample() );
		}
	}
	else
	{
		wxFileDialog dlg( this, _("Open volume file"), m_strLastDir, _(""), 
						_T("Volume files (*.nii;*.nii.gz;*.img;*.mgz)|*.nii;*.nii.gz;*.img;*.mgz|All files (*.*)|*.*"), 
						wxFD_OPEN );
		if ( dlg.ShowModal() == wxID_OK )
		{
			this->LoadVolumeFile( dlg.GetPath(), m_bResampleToRAS );
		}
	}
}

void MainWindow::LoadVolumeFile( const wxString& filename, bool bResample, int nColorMap )
{
	m_strLastDir = MyUtils::GetNormalizedPath( filename );

	m_bResampleToRAS = bResample;	
	LayerMRI* layer = new LayerMRI();
	layer->SetResampleToRAS( bResample );
	layer->GetProperties()->SetLUTCTAB( m_luts->GetColorTable( 0 ) );	
	layer->GetProperties()->SetColorMap( ( LayerPropertiesMRI::ColorMapType )nColorMap );
	wxFileName fn( filename );	
	wxString layerName = fn.GetName();
	if ( fn.GetExt().Lower() == "gz" )
		layerName = wxFileName( layerName ).GetName();
	layer->SetName( layerName.c_str() );
	layer->SetFileName( fn.GetFullPath().c_str() );
	
	WorkerThread* thread = new WorkerThread( this );
	thread->LoadVolume( layer );
}

void MainWindow::OnFileExit( wxCommandEvent& event )
{
	Close();
}

void MainWindow::OnActivate(wxActivateEvent& event)
{
#ifdef __WXGTK__
	NeedRedraw();
#endif	
	event.Skip();
}

void MainWindow::OnFileRecent( wxCommandEvent& event )
{
	wxString fn( m_fileHistory->GetHistoryFile( event.GetId() - wxID_FILE1 ) );
	if ( !fn.IsEmpty() )
		this->LoadVolumeFile( fn, m_bResampleToRAS );
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
		delete roi;
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

void MainWindow::OnKeyDown( wxKeyEvent& event )
{
//	cout << "test" << endl;
//	m_viewAxial->GetEventHandler()->ProcessEvent( event );
	
	event.Skip();
}

void MainWindow::UpdateToolbars()
{
	m_bToUpdateToolbars = true;
}

void MainWindow::DoUpdateToolbars()
{
	bool bVoxelEditVisible = m_toolbarVoxelEdit->IsShown();
	bool bROIEditVisible = m_toolbarROIEdit->IsShown();
	if ( bVoxelEditVisible != (m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit) ||
			bROIEditVisible != (m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit) )
	{
		m_toolbarVoxelEdit->Show( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit );
		m_toolbarROIEdit->Show( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit );
		m_toolbarBrush->Show( m_viewAxial->GetInteractionMode() != RenderView2D::IM_Navigate );
		m_panelToolbarHolder->Layout();
	}
		
	XRCCTRL( *m_toolbarBrush, "ID_STATIC_BRUSH_SIZE", wxStaticText )->Enable( m_viewAxial->GetAction() != Interactor2DROIEdit::EM_Fill );
	XRCCTRL( *m_toolbarBrush, "ID_SPIN_BRUSH_SIZE", wxSpinCtrl )->Enable( m_viewAxial->GetAction() != Interactor2DROIEdit::EM_Fill );
	wxCheckBox* checkTemplate = XRCCTRL( *m_toolbarBrush, "ID_CHECK_TEMPLATE", wxCheckBox );
	wxChoice* choiceTemplate = XRCCTRL( *m_toolbarBrush, "ID_CHOICE_TEMPLATE", wxChoice );
	checkTemplate->Enable( m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
	choiceTemplate->Enable( checkTemplate->IsChecked() && m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
	XRCCTRL( *m_toolbarBrush, "ID_STATIC_BRUSH_TOLERANCE", wxStaticText )->Enable( checkTemplate->IsChecked() && m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
	XRCCTRL( *m_toolbarBrush, "ID_SPIN_BRUSH_TOLERANCE", wxSpinCtrl )->Enable( checkTemplate->IsChecked() && m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
	
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
	
	m_bToUpdateToolbars = false;
}

void MainWindow::OnModeNavigate( wxCommandEvent& event )
{
	m_viewAxial->SetInteractionMode( RenderView2D::IM_Navigate );
	m_viewCoronal->SetInteractionMode( RenderView2D::IM_Navigate );
	m_viewSagittal->SetInteractionMode( RenderView2D::IM_Navigate );
	UpdateToolbars();
}

void MainWindow::OnModeNavigateUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_Navigate );
}

void MainWindow::OnModeVoxelEdit( wxCommandEvent& event )
{
	m_viewAxial->SetInteractionMode( RenderView2D::IM_VoxelEdit );
	m_viewCoronal->SetInteractionMode( RenderView2D::IM_VoxelEdit );
	m_viewSagittal->SetInteractionMode( RenderView2D::IM_VoxelEdit );
	UpdateToolbars();
}

void MainWindow::OnModeVoxelEditUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit );
	event.Enable( m_layerCollectionManager->HasLayer( "MRI" ) );
}

void MainWindow::OnModeROIEdit( wxCommandEvent& event )
{
	m_viewAxial->SetInteractionMode( RenderView2D::IM_ROIEdit );
	m_viewCoronal->SetInteractionMode( RenderView2D::IM_ROIEdit );
	m_viewSagittal->SetInteractionMode( RenderView2D::IM_ROIEdit );
	UpdateToolbars();
}

void MainWindow::OnModeROIEditUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit );
	event.Enable( m_layerCollectionManager->HasLayer( "ROI" ) );
}

void MainWindow::OnActionROIFreehand( wxCommandEvent& event )
{
	m_viewAxial->SetAction( Interactor2DROIEdit::EM_Freehand );
	m_viewCoronal->SetAction( Interactor2DROIEdit::EM_Freehand );
	m_viewSagittal->SetAction( Interactor2DROIEdit::EM_Freehand );
	UpdateToolbars();
}

void MainWindow::OnActionROIFreehandUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit 
			&& m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Freehand );
	event.Enable( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit 
					&& m_layerCollectionManager->HasLayer( "ROI" ) );
}

void MainWindow::OnActionROIFill( wxCommandEvent& event )
{
	m_viewAxial->SetAction( Interactor2DROIEdit::EM_Fill );
	m_viewCoronal->SetAction( Interactor2DROIEdit::EM_Fill );
	m_viewSagittal->SetAction( Interactor2DROIEdit::EM_Fill );
	UpdateToolbars();
}

void MainWindow::OnActionROIFillUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit 
			&& m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Fill );
	
	event.Enable( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit 
			&& m_layerCollectionManager->HasLayer( "ROI" ) );
}

void MainWindow::OnActionROIPolyline( wxCommandEvent& event )
{
	m_viewAxial->SetAction( Interactor2DROIEdit::EM_Polyline );
	m_viewCoronal->SetAction( Interactor2DROIEdit::EM_Polyline );
	m_viewSagittal->SetAction( Interactor2DROIEdit::EM_Polyline );
	UpdateToolbars();
}

void MainWindow::OnActionROIPolylineUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit 
			&& m_viewAxial->GetAction() == Interactor2DROIEdit::EM_Polyline );
	
	event.Enable( m_viewAxial->GetInteractionMode() == RenderView2D::IM_ROIEdit 
			&& m_layerCollectionManager->HasLayer( "ROI" ) );
}

void MainWindow::OnActionVoxelFreehand( wxCommandEvent& event )
{
	m_viewAxial->SetAction( Interactor2DVoxelEdit::EM_Freehand );
	m_viewCoronal->SetAction( Interactor2DVoxelEdit::EM_Freehand );
	m_viewSagittal->SetAction( Interactor2DVoxelEdit::EM_Freehand );
	UpdateToolbars();
}

void MainWindow::OnActionVoxelFreehandUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit 
			&& m_viewAxial->GetAction() == Interactor2DVoxelEdit::EM_Freehand );
	event.Enable( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit 
			&& m_layerCollectionManager->HasLayer( "MRI" ) );
}

void MainWindow::OnActionVoxelFill( wxCommandEvent& event )
{
	m_viewAxial->SetAction( Interactor2DVoxelEdit::EM_Fill );
	m_viewCoronal->SetAction( Interactor2DVoxelEdit::EM_Fill );
	m_viewSagittal->SetAction( Interactor2DVoxelEdit::EM_Fill );
	UpdateToolbars();
}

void MainWindow::OnActionVoxelFillUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit 
			&& m_viewAxial->GetAction() == Interactor2DVoxelEdit::EM_Fill );
	
	event.Enable( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit 
			&& m_layerCollectionManager->HasLayer( "MRI" ) );
}

void MainWindow::OnActionVoxelPolyline( wxCommandEvent& event )
{
	m_viewAxial->SetAction( Interactor2DVoxelEdit::EM_Polyline );
	m_viewCoronal->SetAction( Interactor2DVoxelEdit::EM_Polyline );
	m_viewSagittal->SetAction( Interactor2DVoxelEdit::EM_Polyline );
	UpdateToolbars();
}

void MainWindow::OnActionVoxelPolylineUpdateUI( wxUpdateUIEvent& event)
{
	event.Check( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit 
			&& m_viewAxial->GetAction() == Interactor2DVoxelEdit::EM_Polyline );
	
	event.Enable( m_viewAxial->GetInteractionMode() == RenderView2D::IM_VoxelEdit 
			&& m_layerCollectionManager->HasLayer( "MRI" ) );
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
	event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() );
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
		m_viewAxial->Render();
		m_viewSagittal->Render();
		m_viewCoronal->Render();
		m_view3D->Render();
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
	
	if ( dlg.ShowModal() == wxID_OK )
	{
		wxColour color = dlg.GetBackgroundColor();
		m_viewAxial->SetBackgroundColor( color );
		m_viewSagittal->SetBackgroundColor( color );
		m_viewCoronal->SetBackgroundColor( color );
		m_view3D->SetBackgroundColor( color );
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
	event.Enable( layer && layer->IsModified() && !IsLoading() && !IsSaving() );
}


void MainWindow::OnFileSaveAs( wxCommandEvent& event )
{
	SaveVolumeAs();
}
	
void MainWindow::OnFileSaveAsUpdateUI( wxUpdateUIEvent& event )
{
	LayerMRI* layer = ( LayerMRI* )( GetLayerCollection( "MRI" )->GetActiveLayer() );	
	event.Enable( layer && layer->IsEditable() && !IsLoading() && !IsSaving() );
}

void MainWindow::OnFileLoadROI( wxCommandEvent& event )
{
	LoadROI();
}
	
void MainWindow::OnFileLoadROIUpdateUI( wxUpdateUIEvent& event )
{
	event.Enable( !GetLayerCollection( "MRI" )->IsEmpty() && !IsLoading() && !IsSaving() );
}


void MainWindow::OnFileSaveROI( wxCommandEvent& event )
{
	SaveROI();
}
	
void MainWindow::OnFileSaveROIUpdateUI( wxUpdateUIEvent& event )
{
	LayerROI* layer = ( LayerROI* )( GetLayerCollection( "ROI" )->GetActiveLayer() );	
	event.Enable( layer && layer->IsModified() && !IsLoading() && !IsSaving() );
}


void MainWindow::OnFileSaveROIAs( wxCommandEvent& event )
{
	SaveROIAs();
}
	
void MainWindow::OnFileSaveROIAsUpdateUI( wxUpdateUIEvent& event )
{
	LayerROI* layer = ( LayerROI* )( GetLayerCollection( "ROI" )->GetActiveLayer() );	
	event.Enable( layer && !IsLoading() && !IsSaving() );
}


void MainWindow::OnWorkerThreadResponse( wxCommandEvent& event )
{
	wxString strg = event.GetString();
	if ( strg == "Save" || strg == "Load" )
	{
		if ( event.GetInt() == -1 )		// successfully finished
		{
			m_bSaving = false;
			m_bLoading = false;
			m_statusBar->m_gaugeBar->Hide();
			
			Layer* layer = ( Layer* )(void*)event.GetClientData();
			if ( layer && layer->IsTypeOf( "MRI" ) )
			{
				if ( strg == "Load" )
				{
					LayerCollection* lc = GetLayerCollection( "MRI" );
					LayerMRI* mri = (LayerMRI*)layer;					
					if ( lc->GetNumberOfLayers() < 1 )
					{
						double worigin[3], wsize[3];
						mri->GetWorldOrigin( worigin );
						mri->GetWorldSize( wsize );
						mri->SetSlicePositionToWorldCenter();
						m_viewAxial->SetWorldCoordinateInfo( worigin, wsize );
						m_viewSagittal->SetWorldCoordinateInfo( worigin, wsize );
						m_viewCoronal->SetWorldCoordinateInfo( worigin, wsize );
						m_view3D->SetWorldCoordinateInfo( worigin, wsize );
						lc->AddLayer( mri, true );
						lc->SetCursorRASPosition( lc->GetSlicePosition() );
					}
					else
						lc->AddLayer( layer );
			
					m_fileHistory->AddFileToHistory( MyUtils::GetNormalizedFullPath( mri->GetFileName() ) );
			
					m_controlPanel->RaisePage( "Volumes" );
				}		
			}
			else if ( layer && layer->IsTypeOf( "Surface" ) )
			{
				if ( strg == "Load" )
				{
					LayerCollection* lc = GetLayerCollection( "Surface" );
					lc->AddLayer( layer );
				
				//	m_fileHistory->AddFileToHistory( MyUtils::GetNormalizedFullPath( layer->GetFileName() ) );
				
					m_controlPanel->RaisePage( "Surfaces" );
				}		
			}
			
			if ( strg == "Load" )
			{
				m_viewAxial->SetInteractionMode( RenderView2D::IM_Navigate );
				m_viewCoronal->SetInteractionMode( RenderView2D::IM_Navigate );
				m_viewSagittal->SetInteractionMode( RenderView2D::IM_Navigate );
			}
			
			m_controlPanel->UpdateUI();	
			
			RunScript();		
		}
		else if ( event.GetInt() == 0 )		// just started
		{
			if ( strg == "Save" )
				m_bSaving = true;
			else if ( strg == "Load" )
				m_bLoading = true;
			
			m_statusBar->ActivateProgressBar();
			NeedRedraw();			
			m_controlPanel->UpdateUI( true );	
		}
		else
		{	
			if ( event.GetInt() > m_statusBar->m_gaugeBar->GetValue() )
				m_statusBar->m_gaugeBar->SetValue( event.GetInt() );
		}
	}
	else if ( strg.Left( 6 ) == "Failed" )
	{
		m_statusBar->m_gaugeBar->Hide();
		m_bSaving = false;
		m_bLoading = false;
		Layer* layer = ( Layer* )(void*)event.GetClientData();
		if ( strg == "Load" && layer )
			delete layer;	
		m_controlPanel->UpdateUI();
		strg = strg.Mid( 6 );
		if ( strg.IsEmpty() )
			strg = "Operation failed.";
		wxMessageDialog dlg( this, strg, "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
	}
}

void MainWindow::DoListenToMessage ( std::string const iMsg, void* const iData )
{
	if ( iMsg == "LayerAdded" || iMsg == "LayerRemoved" || iMsg == "LayerMoved" )
			UpdateToolbars();	
	
	if ( iMsg == "MRINotVisible" )
	{
		wxMessageDialog dlg( this, "Active volume is not visible. Please turn it on before editing.", "Error", wxOK | wxICON_ERROR );
		dlg.ShowModal();
	}
	if ( iMsg == "MRINotEditable" )
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
	
	wxString fn_vector = dlg.GetVectorFileName();
	wxString fn_fa = dlg.GetFAFileName();
	LoadDTIFile( fn_fa, fn_vector, dlg.IsToResample() );
}

void MainWindow::LoadDTIFile( const wxString& fn_fa, const wxString& fn_vector, bool bResample )
{	
	m_strLastDir = MyUtils::GetNormalizedPath( fn_fa );
	m_bResampleToRAS = bResample;

	LayerDTI* layer = new LayerDTI();
	layer->SetResampleToRAS( bResample );
	wxString layerName = wxFileName( fn_vector ).GetName();
	if ( wxFileName( fn_fa ).GetExt().Lower() == "gz" )
		layerName = wxFileName( layerName ).GetName();
	layer->SetName( layerName.c_str() );
	layer->SetFileName( fn_fa.c_str() );
	layer->SetVectorFileName( fn_vector.c_str() );
	
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
		wxString strValue = sa[1];
		int nColorMap = LayerPropertiesMRI::Grayscale;
		int n = strValue.Find( ":" );
		if ( n != wxNOT_FOUND )
		{
			wxString strg = strValue.Mid( n + 1 );
			strValue = 	strValue.Left( n );	
			if ( ( n = strg.Find( "=" ) ) != wxNOT_FOUND && strg.Left( n ).Lower() == "colormap" )
			{
				strg = strg.Mid( n + 1 ).Lower();
				if ( strg == "heat" )
					nColorMap = LayerPropertiesMRI::Heat;
				else if ( strg == "jet" )
					nColorMap = LayerPropertiesMRI::Jet;
				else if ( strg == "lut" )
					nColorMap = LayerPropertiesMRI::LUT;
			}
		}
		bool bResample = true;
		if ( sa.GetCount() > 2 && sa[2] == "nr" )
			bResample = false;
		LoadVolumeFile( strValue, bResample, nColorMap );
	} 
	else if ( sa[0] == "loaddti" )
	{
		bool bResample = true;
		if ( sa.GetCount() > 3 && sa[3] == "nr" )
			bResample = false;
		if ( sa.GetCount() > 2 )
			LoadDTIFile( sa[1], sa[2], bResample );
	}
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
	wxChoice* choiceTemplate = XRCCTRL( *m_toolbarBrush, "ID_CHOICE_TEMPLATE", wxChoice );
	LayerEditable* layer = (LayerEditable*)(void*)choiceTemplate->GetClientData( event.GetSelection() );
	if ( layer )
		m_propertyBrush->SetReferenceLayer( layer );
	
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

void MainWindow::OnFileSaveScreenshot( wxCommandEvent& event )
{
	int nId = GetActiveViewId();
	if ( nId < 0 )
		nId = m_nPrevActiveViewId;
	
	if ( nId < 0 )
		return;
	
	wxString fn;
	wxFileDialog dlg( this, _("Save screen capture"), m_strLastDir, _(""), 
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
		m_viewRender[nId]->SaveScreenshot( fn.c_str() );
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

void MainWindow::LoadSurfaceFile( const wxString& filename )
{
	m_strLastDir = MyUtils::GetNormalizedPath( filename );

	LayerSurface* layer = new LayerSurface();
	wxFileName fn( filename );	
	wxString layerName = fn.GetName();
	if ( fn.GetExt().Lower() == "gz" )
		layerName = wxFileName( layerName ).GetName();
	layer->SetName( layerName.c_str() );
	layer->SetFileName( fn.GetFullPath().c_str() );
	
	WorkerThread* thread = new WorkerThread( this );
	thread->LoadSurface( layer );
}
