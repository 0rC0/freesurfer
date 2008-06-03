/**
 * @file  vtkKWScubaApp.cxx
 * @brief Main app for Scuba
 *
 * The main app code. Parse command line options and does some initial
 * set up.
 */
/*
 * Original Author: Kevin Teich
 * CVS Revision Info:
 *    $Author: nicks $
 *    $Date: 2008/06/03 23:51:47 $
 *    $Revision: 1.8 $
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

#include <assert.h>
#include "vtkKWScubaApp.h"
#include "vtkKWScubaWindow.h"
#include "vtkObjectFactory.h"
#include "vtkKWOptionDataBase.h"
#include "IconLoader.h"
#include "vtksys/CommandLineArguments.hxx"
#include "vtksys/SystemTools.hxx"

using namespace std;

vtkStandardNewMacro( vtkKWScubaApp );
vtkCxxRevisionMacro( vtkKWScubaApp, "$Revision: 1.8 $" );

vtkKWScubaApp::vtkKWScubaApp () {

  this->GetOptionDataBase()->
    AddEntry( "vtkKWWidget", "SetBackgroundColor", "1 1 1" );

  // Init the icon loader with the app and load our icons.
  try {
    IconLoader::Initialize( this );

    try {
      IconLoader::LoadIconsFromFile( "./ScubaIcons.txt" );
    }
    catch(...) {
      char* pfnFreesurferDir = getenv( "FREESURFER_HOME" );
      if( NULL != pfnFreesurferDir ) {
	string fnIcons = 
	  string(pfnFreesurferDir) + "/lib/resource/ScubaIcons.txt";
	IconLoader::LoadIconsFromFile( fnIcons.c_str() );
      }
    }
  }
  catch( exception& e ) {
    cerr << "Error loading icons: " << e.what() << endl;
  }

  // Set our app name and load our settings. This needs to be done
  // before we make our window because the window will try to get
  // stuff from the registry too.
  this->SetName( "Scuba" );
  this->RestoreApplicationSettingsFromRegistry();

  // Create the main window.
  mWindow = vtkSmartPointer<vtkKWScubaWindow>::New();
  mWindow->SetApplication( this );
  this->AddWindow( mWindow );
  mWindow->Create();

}

vtkKWScubaApp::~vtkKWScubaApp () {

  this->SaveApplicationSettingsToRegistry();
}

void
vtkKWScubaApp::Start ( int argc, char* argv[] ) {

  assert( mWindow.GetPointer() );

  mWindow->Display();

  // Create our command line argument parser.
  vtksys::CommandLineArguments args;
  args.Initialize( argc, argv );

  // Add the arguments we'll look for.
  args.AddArgument( "--subject", args.SPACE_ARGUMENT, &msSubjectName,
		    "A subject name whose directory (prepended with the value of SUBJECTS_DIR and appened with the relative subdirectory) will be used as a prefix for data file names." );

  vector<string> lfnVolumes;
#if ((VTK_MAJOR_VERSION == 5)&&(VTK_MINOR_VERSION > 0))
  args.AddArgument( "--volume", args.MULTI_ARGUMENT, &lfnVolumes,
		    "A volume file or list of files to load" );
#else
  string fnVolume;
  args.AddArgument( "--volume", args.SPACE_ARGUMENT, &fnVolume,
		    "A volume file to load" );
  lfnVolumes.push_back(fnVolume);
#endif

  vector<string> lfnSurfaces;
#if ((VTK_MAJOR_VERSION == 5)&&(VTK_MINOR_VERSION > 0))
  args.AddArgument( "--surface", args.MULTI_ARGUMENT, &lfnSurfaces,
		    "A surface file or list of files to load" );
#else
  string fnSurface;
  args.AddArgument( "--surface", args.SPACE_ARGUMENT, &fnSurface,
		    "A surface file to load" );
  lfnSurfaces.push_back(fnSurface);
#endif

  vector<string> lfnDTIs;
#if ((VTK_MAJOR_VERSION == 5)&&(VTK_MINOR_VERSION > 0))
  args.AddArgument( "--dti", args.MULTI_ARGUMENT, &lfnDTIs,
		    "A DTI file or list of files to load" );
#else
  string fnDTI;
  args.AddArgument( "--dti", args.SPACE_ARGUMENT, &fnDTI,
		    "A DTI file to load" );
  lfnDTIs.push_back(fnDTI);
#endif

  vector<string> lfnPaths;
#if ((VTK_MAJOR_VERSION == 5)&&(VTK_MINOR_VERSION > 0))
  args.AddArgument( "--path", args.MULTI_ARGUMENT, &lfnPaths,
		    "A path file or list of files to load" );
#else
  string fnPath;
  args.AddArgument( "--path", args.SPACE_ARGUMENT, &fnPath,
		    "A path file to load" );
  lfnPaths.push_back(fnPath);
#endif

  // Try and parse the arguments. If there was an error, print our
  // help message and quit.
  if( !args.Parse() ) {
    cerr << "Error parsing arguments." << endl;
    cerr << args.GetHelp() << endl;
    exit( 1 );
  }

  // Load up the data we got.
  vector<string>::iterator tfn;
  for( tfn = lfnVolumes.begin(); tfn != lfnVolumes.end(); ++tfn )
    this->LoadVolume( tfn->c_str() );

  for( tfn = lfnSurfaces.begin(); tfn != lfnSurfaces.end(); ++tfn )
    this->LoadSurface( tfn->c_str() );

  for( tfn = lfnDTIs.begin(); tfn != lfnDTIs.end(); ++tfn )
    this->LoadDTI( tfn->c_str() );

  for( tfn = lfnPaths.begin(); tfn != lfnPaths.end(); ++tfn )
    this->LoadPath( tfn->c_str() );

  // Tell the super to start.
  this->Superclass::Start( argc, argv );
}

void
vtkKWScubaApp::LoadVolume ( const char* ifnVolume ) {
  
  assert( mWindow.GetPointer() );

  mWindow->LoadVolume( this->FormatFileNameUsingSubjectName(ifnVolume, "mri").c_str() );
}

void
vtkKWScubaApp::LoadSurface ( const char* ifnSurface ) {

  assert( mWindow.GetPointer() );

  mWindow->LoadSurface( this->FormatFileNameUsingSubjectName(ifnSurface, "surf").c_str() );
}

void
vtkKWScubaApp::LoadDTI ( const char* ifnDTI ) {

  assert( mWindow.GetPointer() );

  mWindow->LoadDTI( this->FormatFileNameUsingSubjectName(ifnDTI, "").c_str() );
}

void
vtkKWScubaApp::LoadPath ( const char* ifnPath ) {

  assert( mWindow.GetPointer() );

  mWindow->LoadPath( this->FormatFileNameUsingSubjectName(ifnPath, "").c_str() );
}

string
vtkKWScubaApp::FormatFileNameUsingSubjectName ( const char* ifnMain, 
						const char* ifnSubDir ) {

  // If this is a full path, don't touch it.
  if( vtksys::SystemTools::FileIsFullPath( ifnMain ) )
    return string(ifnMain);

  // Try to load the file relatively first, like if it's ./file.dat
  if( vtksys::SystemTools::FileExists( ifnMain ) )
    return string(ifnMain);

  // Try to make a file name from SUBJECTS_DIR.
  string sSubjectsDir;
  if( vtksys::SystemTools::GetEnv( "SUBJECTS_DIR", sSubjectsDir )) {
    
    // SUBJECTS_DIR/subjetname/ifnSubDir/ifnMain
    string fnFormatted = sSubjectsDir + "/" + msSubjectName + "/" +
      string(ifnSubDir) + "/" + string(ifnMain);

    if( vtksys::SystemTools::FileExists( fnFormatted.c_str() ) )
      return fnFormatted;
  }

  // That didn't work, we can't find the file. Just return the file
  // name we got.
  return string(ifnMain);
}
