#include "string_fixed.h"
#include <stdlib.h>
#include <stdexcept>
#include "glut.h"
#include "ScubaView.h"
#include "PreferencesManager.h"
#include "ScubaGlobalPreferences.h"
#include "Timer.h"

using namespace std;

int const ScubaView::kBytesPerPixel = 4;
map<int,bool> ScubaView::mViewIDLinkedList;
int ScubaView::mCurrentBroadcaster = -1;

int const ScubaView::kcInPlaneMarkerColors = 12;
float kInPlaneMarkerColors[ScubaView::kcInPlaneMarkerColors][3] = {
  { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
  { 1, 1, 0 }, { 1, 0, 1 }, { 0, 1, 1 }, 
  { 0.25, 0.75, 0 }, { 0.25, 0, 0.75 }, { 0, 0.25, 0.75 }, 
  { 0.75, 0.25, 0 }, { 0.75, 0, 0.25 }, { 0, 0.75, 0.25 } };

ScubaView::ScubaView() {
  mBuffer = NULL;
  mbPostRedisplay = false;
  mbRebuildOverlayDrawList = true;
  mViewIDLinkedList[GetID()] = false;
  mbFlipLeftRightInYZ = true;
  mInPlaneMovementIncrements[0] = 1.0;
  mInPlaneMovementIncrements[1] = 1.0;
  mInPlaneMovementIncrements[2] = 1.0;
  int nMarkerColor = GetID() % kcInPlaneMarkerColors;
  mInPlaneMarkerColor[0] = kInPlaneMarkerColors[nMarkerColor][0];
  mInPlaneMarkerColor[1] = kInPlaneMarkerColors[nMarkerColor][1];
  mInPlaneMarkerColor[2] = kInPlaneMarkerColors[nMarkerColor][2];
  mbVisibleInFrame = false;


  ScubaGlobalPreferences& globalPrefs =
    ScubaGlobalPreferences::GetPreferences();
  globalPrefs.AddListener( this );

  ScubaViewBroadcaster& broadcaster = ScubaViewBroadcaster::GetBroadcaster();
  broadcaster.AddListener( this );

  mbFlipLeftRightInYZ = 
    globalPrefs.GetPrefAsBool( ScubaGlobalPreferences::ViewFlipLeftRight );

  // Try setting our initial transform to the default transform with
  // id 0. If it's not there, create it.
  try { 
    mWorldToView = &(ScubaTransform::FindByID( 0 ));
    mWorldToView->AddListener( this );
  }
  catch(...) {

    ScubaTransform* transform = new ScubaTransform();
    transform->SetLabel( "Identity" );
    transform->AddListener( this );
    
    try {
      mWorldToView = &(ScubaTransform::FindByID( 0 ));
    }
    catch(...) {
      DebugOutput( << "Couldn't make default transform!" );
    }
  }


  TclCommandManager& commandMgr = TclCommandManager::GetManager();
  commandMgr.AddCommand( *this, "SetViewInPlane", 2, "viewID inPlane",
			 "Sets the in plane in a view. inPlane should be "
			 "one of the following: x y z" );
  commandMgr.AddCommand( *this, "GetViewInPlane", 1, "viewID",
			 "Returns the in plane in a view." );
  commandMgr.AddCommand( *this, "SetViewZoomLevel", 2, "viewID zoomLevel",
			 "Sets the zoom level in a view. zoomLevel should be "
			 "a float." );
  commandMgr.AddCommand( *this, "SetViewRASCenter", 4, "viewID x y z",
			 "Sets the view center. x, y, and z should be floats "
			 "in world RAS coordinates." );
  commandMgr.AddCommand( *this, "SetLayerInViewAtLevel", 3, 
			 "viewID layerID level",
			 "Sets the layer in a view at a given draw level. "
			 "Higher draw levels will draw later." );
  commandMgr.AddCommand( *this, "GetLayerInViewAtLevel", 2, "viewID level",
			 "Returns the layer in a view at a given draw level.");
  commandMgr.AddCommand( *this, "RemoveAllLayersFromView", 1, "viewID",
			 "Remove all layers from a view." );
  commandMgr.AddCommand( *this, "RemoveLayerFromViewAtLevel", 2, 
			 "viewID layer",
			 "Remove a layer from a view." );
  commandMgr.AddCommand( *this, "GetLabelValuesSet", 2, "viewID setName",
			 "Get a set of label value pairs." );
  commandMgr.AddCommand( *this, "GetFirstUnusedDrawLevelInView", 1, "viewID",
			 "Returns the first unused draw level." );
  commandMgr.AddCommand( *this, "SetViewLinkedStatus", 2, "viewID linked",
			 "Set the linked status for a view." );
  commandMgr.AddCommand( *this, "GetViewLinkedStatus", 1, "viewID",
			 "Returns the linked status for a view." );
  commandMgr.AddCommand( *this, "SetViewTransform", 2, "viewID transformID",
			 "Set the view to world transform for a view." );
  commandMgr.AddCommand( *this, "GetViewTransform", 1, "viewID",
			 "Returns the transformID of a view's view to "
			 "world transform." );
  commandMgr.AddCommand( *this, "SetViewFlipLeftRightYZ", 2, "viewID flip",
			 "Set the left-right flip flag for a view." );
  commandMgr.AddCommand( *this, "GetViewFlipLeftRightYZ", 1, "viewID",
			 "Returns the left-right flip flag for a view." );
  commandMgr.AddCommand( *this, "SetViewInPlaneMovementIncrement", 3,
			 "viewID inPlane increment",
			 "Set the amount that using the in plane movement "
			 "keys will increment or decrement the in plane "
			 "RAS value. inPlane should be x, y, or z." );
  commandMgr.AddCommand( *this, "GetViewInPlaneMovementIncrement", 2,
			 "viewID inPlane",
			 "Returns the amount the in plane movement "
			 "increment for inPlane. inPlane should be x, "
			 "y, or z." );

  // Get some prefs values
  ScubaGlobalPreferences& prefs = ScubaGlobalPreferences::GetPreferences();
  msMoveViewLeft = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyMoveViewLeft );
  msMoveViewRight = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyMoveViewRight );
  msMoveViewUp = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyMoveViewUp );
  msMoveViewDown = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyMoveViewDown );
  msMoveViewIn = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyMoveViewIn );
  msMoveViewOut = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyMoveViewOut );
  msZoomViewIn = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyZoomViewIn );
  msZoomViewOut = 
    prefs.GetPrefAsString( ScubaGlobalPreferences::KeyZoomViewOut );
  

  map<string,string> labelValueMap;
  mLabelValueMaps["cursor"] = labelValueMap;
}

ScubaView::~ScubaView() {
}

void
ScubaView::Set2DRASCenter ( float iRASCenter[3] ) {

  mViewState.mCenterRAS[0] = iRASCenter[0];
  mViewState.mCenterRAS[1] = iRASCenter[1];
  mViewState.mCenterRAS[2] = iRASCenter[2];

  // Broadcast this change.
  ScubaViewBroadcaster& broadcaster = ScubaViewBroadcaster::GetBroadcaster();
  broadcaster.SendBroadcast( "2DRASCenterChanged", (void*)&mID );

  // Changed our view center, so we need to rebuild the overlay.
  RebuildOverlayDrawList();
  RequestRedisplay();
}

void
ScubaView::Set2DZoomLevel ( float iZoomLevel ) {

  mViewState.mZoomLevel = iZoomLevel;

  // Broadcast this change.
  ScubaViewBroadcaster& broadcaster = ScubaViewBroadcaster::GetBroadcaster();
  broadcaster.SendBroadcast( "2DZoomLevelChanged", (void*)&mID );

  // Changed zoom, so we need to rebuild the overlay.
  RebuildOverlayDrawList();
  RequestRedisplay();
}

void
ScubaView::Set2DInPlane ( ViewState::Plane iPlane ) {

  mViewState.mInPlane = iPlane;

  // Broadcast this change.
  ScubaViewBroadcaster& broadcaster = ScubaViewBroadcaster::GetBroadcaster();
  broadcaster.SendBroadcast( "2DInPlaneChanged", (void*)&mID );

  // Changed in plane, so we need to rebuild the overlay.
  RebuildOverlayDrawList();
  RequestRedisplay();
}

void
ScubaView::Get2DRASCenter ( float oRASCenter[3] ) {

  oRASCenter[0] = mViewState.mCenterRAS[0];
  oRASCenter[1] = mViewState.mCenterRAS[1];
  oRASCenter[2] = mViewState.mCenterRAS[2];
}

float
ScubaView::Get2DZoomLevel () {
  
  return mViewState.mZoomLevel;
}

ViewState::Plane
ScubaView::Get2DInPlane () {

  return mViewState.mInPlane;
}


void 
ScubaView::SetLayerAtLevel ( int iLayerID, int iLevel ) {

  // If we got layer ID -1, remove this layer. Otherwise set it in our
  // map.
  if( iLayerID == -1 ) {

    RemoveLayerAtLevel( iLevel );
    
  } else {
    
    try {
      Layer& layer = Layer::FindByID( iLayerID );
      
      mLevelLayerIDMap[iLevel] = iLayerID;

      // Set the layer's width and height.
      layer.SetWidth( mWidth );
      layer.SetHeight( mHeight );

      // If this is level 0, get our in plane increments from it.
      if( 0 == iLevel ) {
	layer.GetPreferredInPlaneIncrements( mInPlaneMovementIncrements );
      }

    }
    catch(...) {
      DebugOutput( << "Couldn't find layer " << iLayerID );
    }
  }

  RequestRedisplay();
}

int
ScubaView::GetLayerAtLevel ( int iLevel ) {

  // Look for this level in the level layer ID map. If we found
  // it, return the layer ID, otherwise return -1.
  map<int,int>::iterator tLevelLayerID = mLevelLayerIDMap.find( iLevel );
  if( tLevelLayerID == mLevelLayerIDMap.end() ) {
    return -1;
  } else {
    return mLevelLayerIDMap[iLevel];
  }
}

void 
ScubaView::RemoveAllLayers () {

  mLevelLayerIDMap.clear();
}

void 
ScubaView::RemoveLayerAtLevel ( int iLevel ) {

  mLevelLayerIDMap.erase( iLevel );
}

void 
ScubaView::CopyLayerSettingsToView ( ScubaView& iView ) {

  map<int,int>::iterator tLevelLayerID;
  for( tLevelLayerID = mLevelLayerIDMap.begin(); 
       tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {
    
    int level = (*tLevelLayerID).first;
    int layerID = (*tLevelLayerID).second;
    iView.SetLayerAtLevel( level, layerID );
  }
}

void
ScubaView::SetWorldToViewTransform ( int iTransformID ) {

  try {
    mWorldToView->RemoveListener( this );
    mWorldToView = &(ScubaTransform::FindByID( iTransformID ));
    mWorldToView->AddListener( this );
    RequestRedisplay();
  }
  catch(...) {
    DebugOutput( << "Couldn't find transform " << iTransformID );
  }
}

int
ScubaView::GetWorldToViewTransform () {

  return mWorldToView->GetID();
}


TclCommandListener::TclCommandResult
ScubaView::DoListenToTclCommand( char* isCommand, int iArgc, char** iasArgv ) {

  // SetViewInPlane <viewID> <inPlane>
  if( 0 == strcmp( isCommand, "SetViewInPlane" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      ViewState::Plane inPlane;
      if( 0 == strcmp( iasArgv[2], "X" ) || 
	  0 == strcmp( iasArgv[2], "x" ) ||
	  0 == strcmp( iasArgv[2], "R" ) || 
	  0 == strcmp( iasArgv[2], "r" ) ) {
	inPlane = ViewState::X;
      } else if( 0 == strcmp( iasArgv[2], "Y" ) || 
		 0 == strcmp( iasArgv[2], "y" ) ||
		 0 == strcmp( iasArgv[2], "A" ) || 
		 0 == strcmp( iasArgv[2], "a" ) ) {
	inPlane = ViewState::Y;
      } else if( 0 == strcmp( iasArgv[2], "Z" ) || 
		 0 == strcmp( iasArgv[2], "z" ) ||
		 0 == strcmp( iasArgv[2], "S" ) || 
		 0 == strcmp( iasArgv[2], "s" )) {
	inPlane = ViewState::Z;
      } else {
	sResult = "bad inPlane \"" + string(iasArgv[2]) + 
	  "\", should be x, y, or z, or r, a, or s";
	return error;
      }
      Set2DInPlane( inPlane );
    }
  }

  // GetViewInPlane <viewID>
  if( 0 == strcmp( isCommand, "GetViewInPlane" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      sReturnFormat = "s";
      switch( mViewState.mInPlane ) {
      case ViewState::X:
	sReturnValues = "x";
	break;
      case ViewState::Y:
	sReturnValues = "y";
	break;
      case ViewState::Z:
	sReturnValues = "z";
	break;
      }
    }
  }

  // SetViewZoomLevel <viewID> <zoomLevel>
  if( 0 == strcmp( isCommand, "SetViewZoomLevel" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      float zoomLevel = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad zoom level";
	return error;
      }
      
      Set2DZoomLevel( zoomLevel );
    }
  }

  // SetViewRASCenter <viewID> <X> <Y> <Z>
  if( 0 == strcmp( isCommand, "SetViewRASCenter" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }

    if( mID == viewID ) {
      
      float x = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad x coordinate";
	return error;
      }
      float y = (float) strtod( iasArgv[3], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad y coordinate";
	return error;
      }
      float z = (float) strtod( iasArgv[4], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad z coordinate";
	return error;
      }
      
      float center[3];
      center[0] = x; center[1] = y; center[2] = z;
      Set2DRASCenter( center );
    }
  }

  // SetLayerInViewAtLevel <viewID> <layerID> <level>
  if( 0 == strcmp( isCommand, "SetLayerInViewAtLevel" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      int layerID = strtol( iasArgv[2], (char**)NULL, 10 );
      if( ERANGE == errno ) {
	sResult = "bad layer ID";
	return error;
      }
      int level = strtol( iasArgv[3], (char**)NULL, 10 );
      if( ERANGE == errno ) {
	sResult = "bad level";
	return error;
      }
      
      SetLayerAtLevel( layerID, level );
    }
  }

  // GetLayerInViewAtLevel <viewID> <level>
  if( 0 == strcmp( isCommand, "GetLayerInViewAtLevel" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      int level = strtol( iasArgv[2], (char**)NULL, 10 );
      if( ERANGE == errno ) {
	sResult = "bad level";
	return error;
      }
      
      stringstream ssReturn;
      sReturnFormat = "i";
      ssReturn << GetLayerAtLevel( level );
      sReturnValues = ssReturn.str();
      return ok;
    }
  }

  // RemoveLayerFromViewAtLevel <viewID> <level>
  if( 0 == strcmp( isCommand, "RemoveLayerFromViewAtLevel" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      int level = strtol( iasArgv[2], (char**)NULL, 10 );
      if( ERANGE == errno ) {
	sResult = "bad level";
	return error;
      }
      
      RemoveLayerAtLevel( level );
    }
  }

  // RemoveAllLayersFromView <viewID>
  if( 0 == strcmp( isCommand, "RemoveAllLayersFromView" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      RemoveAllLayers();
    }
  }

  // GetLabelValuesSet <viewID> <setName>
  if( 0 == strcmp( isCommand, "GetLabelValuesSet" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {

      string sSetName = iasArgv[2];

      map<string,map<string,string> >::iterator tMap = 
	mLabelValueMaps.find( sSetName );
      if( tMap == mLabelValueMaps.end() ) {
	stringstream ssResult;
	ssResult << "Set name " << sSetName << " doesn't exist.";
	sResult = ssResult.str();
	return error;
      }

      map<string,string> labelValueMap = mLabelValueMaps[sSetName];
      map<string,string>::iterator tLabelValue;

      stringstream ssFormat;
      stringstream ssResult;
      ssFormat << "L";

      for( tLabelValue = labelValueMap.begin(); 
	   tLabelValue != labelValueMap.end(); ++tLabelValue ) {
	
	ssFormat << "Lssl";
	ssResult << "\"" << (*tLabelValue).first << "\" \"" 
		 << (*tLabelValue).second << "\" ";
      }
      ssFormat << "l";
      
      sReturnFormat = ssFormat.str();
      sReturnValues = ssResult.str();
    }
  }

  // GetFirstUnusedDrawLevelInView <viewID>
  if( 0 == strcmp( isCommand, "GetFirstUnusedDrawLevelInView" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {

      sReturnFormat = "i";
      stringstream ssReturnValues;
      ssReturnValues << GetFirstUnusedDrawLevel();
      sReturnValues = ssReturnValues.str();
    }
  }


  // SetViewLinkedStatus <viewID> <linked>
  if( 0 == strcmp( isCommand, "SetViewLinkedStatus" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      if( 0 == strcmp( iasArgv[2], "true" ) || 
	  0 == strcmp( iasArgv[2], "1" )) {
	SetLinkedStatus( true );
      } else if( 0 == strcmp( iasArgv[2], "false" ) ||
		 0 == strcmp( iasArgv[2], "0" ) ) {
	SetLinkedStatus( false );
      } else {
	sResult = "bad linkedStatus \"" + string(iasArgv[2]) +
	  "\", should be true, 1, false, or 0";
	return error;	
      }
    }
  }

  // GetViewLinkedStatus <viewID>
  if( 0 == strcmp( isCommand, "GetViewLinkedStatus" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {

      sReturnFormat = "i";
      stringstream ssReturnValues;
      ssReturnValues << (int)GetLinkedStatus();
      sReturnValues = ssReturnValues.str();
    }
  }

  // SetViewTransform <viewID> <transformID>
  if( 0 == strcmp( isCommand, "SetViewTransform" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      int transformID = strtol( iasArgv[2], (char**)NULL, 10 );
      if( ERANGE == errno ) {
	sResult = "bad transformID";
	return error;
      }
      
      SetWorldToViewTransform( transformID );
    }
  }

  // GetViewTransform <viewID>
  if( 0 == strcmp( isCommand, "GetViewTransform" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {

      sReturnFormat = "i";
      stringstream ssReturnValues;
      ssReturnValues << (int)GetWorldToViewTransform();
      sReturnValues = ssReturnValues.str();
    }
  }

  // SetViewFlipLeftRightYZ <viewID> <flip>
  if( 0 == strcmp( isCommand, "SetViewFlipLeftRightYZ" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      if( 0 == strcmp( iasArgv[2], "true" ) || 
	  0 == strcmp( iasArgv[2], "1" )) {
	SetFlipLeftRightYZ( true );
      } else if( 0 == strcmp( iasArgv[2], "false" ) ||
		 0 == strcmp( iasArgv[2], "0" ) ) {
	SetFlipLeftRightYZ( false );
      } else {
	sResult = "bad flip value \"" + string(iasArgv[2]) +
	  "\", should be true, 1, false, or 0";
	return error;	
      }
    }
  }

  // GetViewFlipLeftRightYZ <viewID>
  if( 0 == strcmp( isCommand, "GetViewFlipLeftRightYZ" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {

      sReturnFormat = "i";
      stringstream ssReturnValues;
      ssReturnValues << (int)GetFlipLeftRightYZ();
      sReturnValues = ssReturnValues.str();
    }
  }

  // SetViewInPlaneMovementIncrement <viewID> <inPlane> <increment>
  if( 0 == strcmp( isCommand, "SetViewInPlaneMovementIncrement" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {
      
      float increment = (float) strtod( iasArgv[3], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad increment";
	return error;
      }
      
      if( 0 == strcmp( iasArgv[2], "x" ) ||
	  0 == strcmp( iasArgv[2], "X" ) ) {

	mInPlaneMovementIncrements[0] = increment;

      } else if( 0 == strcmp( iasArgv[2], "y" ) ||
		 0 == strcmp( iasArgv[2], "Y" ) ) {

	mInPlaneMovementIncrements[1] = increment;

      } else if( 0 == strcmp( iasArgv[2], "z" ) ||
		 0 == strcmp( iasArgv[2], "Z" ) ) {

	mInPlaneMovementIncrements[2] = increment;

      } else {
	stringstream ssResult;
	ssResult << "bad inPlane \"" << iasArgv[1] << "\", should be "
		 << "x, y, or z.";
	sResult = ssResult.str();
	return error;
      }
    }
  }

  // GetViewInPlaneMovementIncrement <viewID> <inPlane>
  if( 0 == strcmp( isCommand, "GetViewInPlaneMovementIncrement" ) ) {
    int viewID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad view ID";
      return error;
    }
    
    if( mID == viewID ) {

      float increment = 0;
      if( 0 == strcmp( iasArgv[2], "x" ) ||
	  0 == strcmp( iasArgv[2], "X" ) ) {

	increment = mInPlaneMovementIncrements[0];

      } else if( 0 == strcmp( iasArgv[2], "y" ) ||
		 0 == strcmp( iasArgv[2], "Y" ) ) {

	increment = mInPlaneMovementIncrements[1];

      } else if( 0 == strcmp( iasArgv[2], "z" ) ||
		 0 == strcmp( iasArgv[2], "Z" ) ) {

	increment = mInPlaneMovementIncrements[2];

      } else {
	stringstream ssResult;
	ssResult << "bad inPlane \"" << iasArgv[2] << "\", should be "
		 << "x, y, or z.";
	sResult = ssResult.str();
	return error;
      }

      sReturnFormat = "f";
      stringstream ssReturnValues;
      ssReturnValues << increment;
      sReturnValues = ssReturnValues.str();
    }
  }

  return ok;
}

void
ScubaView::DoListenToMessage ( string isCommand, void* iData ) {
  
  if( isCommand == "transformChanged" ) {
    RebuildOverlayDrawList(); // our overlay coords are different now
    RequestRedisplay();
  }

  if( isCommand == "DrawCoordinateOverlay" ||
      isCommand == "DrawCenterCrosshairOverlay" ) {
    RebuildOverlayDrawList(); // our overlay will be different
    RequestRedisplay();
  }

  // If we're linked, we need to change our view.
  if( isCommand == "2DRASCenterChanged" ||
      isCommand == "2DZoomLevelChanged" ||
      isCommand == "2DInPlaneChanged" ) {
    int viewID = *(int*)iData;
    if( mViewIDLinkedList[GetID()] && mViewIDLinkedList[viewID] ) {
      View& view = View::FindByID( viewID );
      // ScubaView& scubaView = dynamic_cast<ScubaView&>(view);
      ScubaView& scubaView = (ScubaView&)view;

      // Change center or zoom level if linked. Don't link the in plane.
      if( isCommand == "2DRASCenterChanged" ) {
	float RASCenter[3];
	scubaView.Get2DRASCenter( RASCenter );
	Set2DRASCenter( RASCenter );
      } else if ( isCommand == "2DZoomLevelChanged" ) {
	float zoomLevel;
	zoomLevel = scubaView.Get2DZoomLevel();
	Set2DZoomLevel( zoomLevel );
      }
    }

    // If we're visible request redisplays.
    if( IsVisibleInFrame() ) {
      RebuildOverlayDrawList();
      RequestRedisplay();
    }
  }

  // We cache these values but down have to act on them right away.
  if( isCommand == "KeyMoveViewLeft" )  msMoveViewLeft  = *(string*)iData;
  if( isCommand == "KeyMoveViewRight" ) msMoveViewRight = *(string*)iData;
  if( isCommand == "KeyMoveViewUp" )    msMoveViewUp    = *(string*)iData;
  if( isCommand == "KeyMoveViewDown" )  msMoveViewDown  = *(string*)iData;
  if( isCommand == "KeyMoveViewIn" )    msMoveViewIn    = *(string*)iData;
  if( isCommand == "KeyMoveViewOut" )   msMoveViewOut   = *(string*)iData;
  if( isCommand == "KeyZoomViewIn" )    msZoomViewIn    = *(string*)iData;
  if( isCommand == "KeyZoomViewOut" )   msZoomViewOut   = *(string*)iData;
}

void
ScubaView::DoDraw() {

#if 1
  ::Timer timer;
  timer.Start();
#endif

  BuildFrameBuffer();
  DrawFrameBuffer();
  DrawOverlay();

#if 1
  int msec = timer.TimeNow();
  float fps = 1.0 / ((float)msec/1000.0);

  stringstream ssCommand;
  ssCommand << "SetStatusBarText \"" << fps << " fps\"";

  TclCommandManager& mgr = TclCommandManager::GetManager();
  mgr.SendCommand( ssCommand.str() );
#endif
}

void
ScubaView::DoReshape( int iWidth, int iHeight ) {

  if( iWidth < 0 || iHeight < 0 ) {
    stringstream sError;
    sError << "Invalid width " << mWidth << " or height " << mHeight;
    DebugOutput( << sError.str() );
    throw new runtime_error( sError.str() );
  }

  // Set the view state buffer height and width.
  mViewState.mBufferWidth  = iWidth;
  mViewState.mBufferHeight = iHeight;

  // Allocate a new buffer.
  GLubyte* newBuffer = (GLubyte*) malloc( mWidth * mHeight * kBytesPerPixel );
  if( NULL == newBuffer ) {
    stringstream sError;
    sError << "Couldn't allocate buffer for width " << mWidth 
	   << " height " << mHeight;
    DebugOutput( << sError.str() );
    throw new runtime_error( sError.str() );
  }

  // Get rid of the old one.
  if( NULL != mBuffer ) {
    free( mBuffer );
  }
  
  // Save the new one.
  mBuffer = newBuffer;

  // Set the width and height in all the layers.
  map<int,int>::iterator tLevelLayerID;
  for( tLevelLayerID = mLevelLayerIDMap.begin(); 
       tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {
    
    int layerID = (*tLevelLayerID).second;

    try {
      Layer& layer = Layer::FindByID( layerID );
      
      layer.SetWidth( mWidth );
      layer.SetHeight( mHeight );
    }
    catch(...) {
      DebugOutput( << "Couldn't find layer " << layerID );
    }
  }

  mbRebuildOverlayDrawList = true;
}

void
ScubaView::DoTimer() {

}
  
void
ScubaView::DoMouseMoved( int iWindow[2], 
			 InputState& iInput, ScubaToolState& iTool ) {

  float ras[3];
  TranslateWindowToRAS( iWindow, ras );

  // Rebuild our label value info because the mouse has moved.
  RebuildLabelValueInfo( ras );


  // Handle the navigation tool.
  if( iTool.GetMode() == ScubaToolState::navigation ) {

    if( iInput.Button() && !iInput.IsControlKeyDown() ) {
      
      float delta[2];
      delta[0] = (float)(mLastMouseMoved[0] - iWindow[0]) / 
	mViewState.mZoomLevel;
      delta[1] = (float)(mLastMouseMoved[1] - iWindow[1]) /
	mViewState.mZoomLevel;
      
      /* add to the total delta */
      mMouseMoveDelta[0] += delta[0];
      mMouseMoveDelta[1] += delta[1];
      
      /* save this mouse position */
      mLastMouseMoved[0] = iWindow[0];
      mLastMouseMoved[1] = iWindow[1];
      
      float moveLeftRight = 0, moveUpDown = 0, moveInOut = 0, zoomInOut = 0;
      switch( iInput.Button() ) {
      case 1:
	moveLeftRight = mMouseMoveDelta[0];
	moveUpDown    = mMouseMoveDelta[1];
	break;
      case 2:
	moveInOut = mMouseMoveDelta[1];
	break;
      case 3:
	zoomInOut = mMouseMoveDelta[1] / 10.0;
	break;
      }

      if( mbFlipLeftRightInYZ && 
	  (mViewState.mInPlane == ViewState::Y ||
	   mViewState.mInPlane == ViewState::Z) ) {
	moveLeftRight = -moveLeftRight;
      }
      
      if( moveLeftRight || moveUpDown || moveInOut ) {
	
	float newRAS[3];
	switch( mViewState.mInPlane ) {
	case ViewState::X:
	  newRAS[0] = mOriginalCenterRAS[0] + moveInOut;
	  newRAS[1] = mOriginalCenterRAS[1] + moveLeftRight;
	  newRAS[2] = mOriginalCenterRAS[2] + moveUpDown;
	  break;
	case ViewState::Y:
	  newRAS[0] = mOriginalCenterRAS[0] + moveLeftRight;
	  newRAS[1] = mOriginalCenterRAS[1] + moveInOut;
	  newRAS[2] = mOriginalCenterRAS[2] + moveUpDown;
	  break;
	case ViewState::Z:
	  newRAS[0] = mOriginalCenterRAS[0] + moveLeftRight;
	  newRAS[1] = mOriginalCenterRAS[1] + moveUpDown;
	  newRAS[2] = mOriginalCenterRAS[2] + moveInOut;
	  break;
	}
	Set2DRASCenter( newRAS );
      }
      
      if( zoomInOut ) {
	
	float newZoom = mOriginalZoom + zoomInOut;
	if( newZoom <= 0.25 ) {
	  newZoom = 0.25;
	}
	Set2DZoomLevel( newZoom );
      }
    }
  }
  
  // If not a straight control-click, pass this tool to our layers.
  if( !(iInput.IsControlKeyDown() && 
	!iInput.IsShiftKeyDown() && !iInput.IsAltKeyDown()) ) {
    map<int,int>::iterator tLevelLayerID;
    for( tLevelLayerID = mLevelLayerIDMap.begin(); 
	 tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {
      int layerID = (*tLevelLayerID).second;
      try {
	Layer& layer = Layer::FindByID( layerID );
	layer.HandleTool( ras, mViewState, *this, iTool, iInput );
	if( layer.WantRedisplay() ) {
	  RequestRedisplay();
	  layer.RedisplayPosted();
	}
      }
      catch(...) {
	DebugOutput( << "Couldn't find layer " << layerID );
      }
    }
  }
}

void
ScubaView::DoMouseUp( int iWindow[2],
		      InputState& iInput, ScubaToolState& iTool ) {

  // No matter what tool we're on, look for ctrl-b{1,2,3} and do some
  // navigation stuff.
  if( iInput.IsControlKeyDown() && 
      !iInput.IsShiftKeyDown() && !iInput.IsAltKeyDown() ) {
    
    // Set the new view center to this point. If they also hit b1 or
    // b3, zoom in or out accordingly.
    float world[3];
    TranslateWindowToRAS( iWindow, world );
    Set2DRASCenter( world );

    switch( iInput.Button() ) {
    case 1:
      mViewState.mZoomLevel *= 2.0;
      break;
    case 3:
      mViewState.mZoomLevel /= 2.0;
      if( mViewState.mZoomLevel < 0.25 ) {
	mViewState.mZoomLevel = 0.25;
      }
      break;
    }

    mbRebuildOverlayDrawList = true;
    RequestRedisplay();
  }


  // If not a straight control-click, pass this tool to our layers.
  if( !(iInput.IsControlKeyDown() && 
	!iInput.IsShiftKeyDown() && !iInput.IsAltKeyDown()) ) {
    float ras[3];
    TranslateWindowToRAS( iWindow, ras );
    map<int,int>::iterator tLevelLayerID;
    for( tLevelLayerID = mLevelLayerIDMap.begin(); 
	 tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {
      int layerID = (*tLevelLayerID).second;
      try {
	Layer& layer = Layer::FindByID( layerID );
	layer.HandleTool( ras, mViewState, *this, iTool, iInput );
	if( layer.WantRedisplay() ) {
	  RequestRedisplay();
	  layer.RedisplayPosted();
	}
      }
      catch(...) {
	DebugOutput( << "Couldn't find layer " << layerID );
      }
    }
  }
}

void
ScubaView::DoMouseDown( int iWindow[2], 
			InputState& iInput, ScubaToolState& iTool ) {

  mLastMouseDown[0] = mLastMouseMoved[0] = iWindow[0];
  mLastMouseDown[1] = mLastMouseMoved[1] = iWindow[1];
  mMouseMoveDelta[0] = 0.0;
  mMouseMoveDelta[1] = 0.0;
  mOriginalCenterRAS[0] = mViewState.mCenterRAS[0];
  mOriginalCenterRAS[1] = mViewState.mCenterRAS[1];
  mOriginalCenterRAS[2] = mViewState.mCenterRAS[2];
  mOriginalZoom = mViewState.mZoomLevel;

  // If not a straight control-click, pass this tool to our layers.
  if( !(iInput.IsControlKeyDown() && 
	!iInput.IsShiftKeyDown() && !iInput.IsAltKeyDown()) ) {
    float ras[3];
    TranslateWindowToRAS( iWindow, ras );
    map<int,int>::iterator tLevelLayerID;
    for( tLevelLayerID = mLevelLayerIDMap.begin(); 
	 tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {
      int layerID = (*tLevelLayerID).second;
      try {
	Layer& layer = Layer::FindByID( layerID );
	layer.HandleTool( ras, mViewState, *this, iTool, iInput );
	if( layer.WantRedisplay() ) {
	  RequestRedisplay();
	  layer.RedisplayPosted();
	}
      }
      catch(...) {
      DebugOutput( << "Couldn't find layer " << layerID );
      }
    }
  }
}

void
ScubaView::DoKeyDown( int iWindow[2], 
		      InputState& iInput, ScubaToolState& iTool ) {
  string key = iInput.Key();

  // Start with a move distance of 1. If we're moving in plane, set
  // that to the specific in plane increment. If control is down,
  // multiplay that value by 10.
  float moveDistance = 1.0;
  if( key == msMoveViewIn || key == msMoveViewOut ) {
    switch( mViewState.mInPlane ) {
    case ViewState::X: 
      moveDistance = mInPlaneMovementIncrements[0]; break;
    case ViewState::Y: 
      moveDistance = mInPlaneMovementIncrements[1]; break;
    case ViewState::Z: 
      moveDistance = mInPlaneMovementIncrements[2]; break;
    }
  }
  if( iInput.IsControlKeyDown() ) {
    moveDistance = 10.0;
  }

  

  if( key == msMoveViewLeft || key == msMoveViewRight ||
      key == msMoveViewDown || key == msMoveViewUp ||
      key == msMoveViewIn   || key == msMoveViewOut ) {
    
    float newRAS[3];
    newRAS[0] = mViewState.mCenterRAS[0];
    newRAS[1] = mViewState.mCenterRAS[1];
    newRAS[2] = mViewState.mCenterRAS[2];

    if( key == msMoveViewLeft ) {
      switch( mViewState.mInPlane ) {
      case ViewState::X: 
	newRAS[1] = mViewState.mCenterRAS[1] - moveDistance; break;
      case ViewState::Y: 
	newRAS[0] = mViewState.mCenterRAS[0] - moveDistance; break;
      case ViewState::Z: 
	newRAS[0] = mViewState.mCenterRAS[0] - moveDistance; break;
      }
    } else if( key == msMoveViewRight ) {
      switch( mViewState.mInPlane ) {
      case ViewState::X: 
	newRAS[1] = mViewState.mCenterRAS[1] + moveDistance; break;
      case ViewState::Y: 
	newRAS[0] = mViewState.mCenterRAS[0] + moveDistance; break;
      case ViewState::Z: 
	newRAS[0] = mViewState.mCenterRAS[0] + moveDistance; break;
      }
    } else if( key == msMoveViewDown ) {
      switch( mViewState.mInPlane ) {
      case ViewState::X: 
	newRAS[2] = mViewState.mCenterRAS[2] - moveDistance; break;
      case ViewState::Y: 
	newRAS[2] = mViewState.mCenterRAS[2] - moveDistance; break;
      case ViewState::Z: 
	newRAS[1] = mViewState.mCenterRAS[1] - moveDistance; break;
      }
    } else if( key == msMoveViewUp ) {
      switch( mViewState.mInPlane ) {
      case ViewState::X: 
	newRAS[2] = mViewState.mCenterRAS[2] + moveDistance; break;
      case ViewState::Y: 
	newRAS[2] = mViewState.mCenterRAS[2] + moveDistance; break;
      case ViewState::Z: 
	newRAS[1] = mViewState.mCenterRAS[1] + moveDistance; break;
      }
    } else if( key == msMoveViewIn ) {
      switch( mViewState.mInPlane ) {
      case ViewState::X: 
	newRAS[0] = mViewState.mCenterRAS[0] + moveDistance; break;
      case ViewState::Y: 
	newRAS[1] = mViewState.mCenterRAS[1] + moveDistance; break;
      case ViewState::Z: 
	newRAS[2] = mViewState.mCenterRAS[2] + moveDistance; break;
      }
    } else if( key == msMoveViewOut ) {
      switch( mViewState.mInPlane ) {
      case ViewState::X: 
	newRAS[0] = mViewState.mCenterRAS[0] - moveDistance; break;
      case ViewState::Y: 
	newRAS[1] = mViewState.mCenterRAS[1] - moveDistance; break;
      case ViewState::Z: 
	newRAS[2] = mViewState.mCenterRAS[2] - moveDistance; break;
      }
    }
    Set2DRASCenter( newRAS );

    // Rebuild our label value info because the view has moved.
    float ras[3];
    TranslateWindowToRAS( iWindow, ras );
    RebuildLabelValueInfo( ras );

  } else if( key == msZoomViewIn || key == msZoomViewOut ) {

    float newZoom = mViewState.mZoomLevel;
    if( key == msZoomViewIn ) {
      newZoom = mViewState.mZoomLevel * 2.0;
    } else if( key == msZoomViewOut ) {
      newZoom = mViewState.mZoomLevel / 2.0;
      if( newZoom < 0.25 ) {
	newZoom = 0.25;
      }
    }
    Set2DZoomLevel( newZoom );

    // Rebuild our label value info because the view has moved.
    float ras[3];
    TranslateWindowToRAS( iWindow, ras );
    RebuildLabelValueInfo( ras );
  }

  RequestRedisplay();
}

void
ScubaView::DoKeyUp( int iWindow[2], 
		    InputState& iInput, ScubaToolState& iTool ) {

}

void
ScubaView::TranslateWindowToRAS ( int iWindow[2], float oRAS[3] ) {

  // At zoom level one every pixel is an RAS point, so we're scaled by
  // that and then offset by the RAS center. We find a 3D point by
  // using our mInPlane and the corresponding mCenterRAS in ViewState.

  int xWindow = iWindow[0];
  float viewRAS[3];
  switch( mViewState.mInPlane ) {
  case ViewState::X:
    viewRAS[0] = mViewState.mCenterRAS[0];
    viewRAS[1] = ConvertWindowToRAS( xWindow,mViewState.mCenterRAS[1],mWidth );
    viewRAS[2] = ConvertWindowToRAS(iWindow[1],mViewState.mCenterRAS[2],mHeight );
    break;
  case ViewState::Y:
    if( mbFlipLeftRightInYZ ) {
      xWindow = mWidth - xWindow;
    }
    viewRAS[0] = ConvertWindowToRAS( xWindow,mViewState.mCenterRAS[0],mWidth );
    viewRAS[1] = mViewState.mCenterRAS[1];
    viewRAS[2] = ConvertWindowToRAS(iWindow[1],mViewState.mCenterRAS[2],mHeight );
    break;
  case ViewState::Z:
    if( mbFlipLeftRightInYZ ) {
      xWindow = mWidth - xWindow;
    }
    viewRAS[0] = ConvertWindowToRAS( xWindow,mViewState.mCenterRAS[0],mWidth );
    viewRAS[1] = ConvertWindowToRAS(iWindow[1],mViewState.mCenterRAS[1],mHeight );
    viewRAS[2] = mViewState.mCenterRAS[2];
    break;
  }

  mWorldToView->InvMultiplyVector3( viewRAS, oRAS );
}

float
ScubaView::ConvertWindowToRAS ( float iWindow, float iRASCenter, 
				float iWindowDimension ) {

  return ((iWindow - iWindowDimension / 2.0) / mViewState.mZoomLevel) + 
    iRASCenter;
}

void
ScubaView::TranslateRASToWindow ( float iRAS[3], int oWindow[2] ) {
  
  float viewRAS[3];
  mWorldToView->MultiplyVector3( iRAS, viewRAS );

  float xWindow = 0, yWindow = 0;
  switch( mViewState.mInPlane ) {
  case ViewState::X:
    xWindow = ConvertRASToWindow( viewRAS[1],
				  mViewState.mCenterRAS[1], mWidth );
    yWindow = ConvertRASToWindow( viewRAS[2],
				  mViewState.mCenterRAS[2], mHeight );
    break;
  case ViewState::Y:
    xWindow = ConvertRASToWindow( viewRAS[0],
				  mViewState.mCenterRAS[0], mWidth );
    yWindow = ConvertRASToWindow( viewRAS[2],
				  mViewState.mCenterRAS[2], mHeight );
    if( mbFlipLeftRightInYZ ) {
      xWindow = mWidth - xWindow;
    }
    break;
  case ViewState::Z:
    xWindow = ConvertRASToWindow( viewRAS[0],
				  mViewState.mCenterRAS[0], mWidth );
    yWindow = ConvertRASToWindow( viewRAS[1],
				  mViewState.mCenterRAS[1], mHeight );
    if( mbFlipLeftRightInYZ ) {
      xWindow = mWidth - xWindow;
    }
    break;
  }

  oWindow[0] = (int) rint( xWindow );
  oWindow[1] = (int) rint( yWindow );
}

float
ScubaView::ConvertRASToWindow ( float iRAS, float iRASCenter, 
				float iWindowDimension ) {

  return ((iRAS - iRASCenter) * mViewState.mZoomLevel) +
    (iWindowDimension / 2.0);
}


int
ScubaView::GetFirstUnusedDrawLevel () {

  int highestLevel = 0;
  map<int,int>::iterator tLevelLayerID;
  for( tLevelLayerID = mLevelLayerIDMap.begin(); 
       tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {

    int level = (*tLevelLayerID).first;
    
    highestLevel = level + 1;
  }

  return highestLevel;
}

void
ScubaView::SetFlipLeftRightYZ ( bool iFlip ) {

  mbFlipLeftRightInYZ = iFlip;
  RequestRedisplay();
}


void 
ScubaView::BuildFrameBuffer () {

  // Don't draw if our buffer isn't initialized yet.
  if( NULL == mBuffer )
    return;

  // Erase the frame buffer.
  bzero( mBuffer, mHeight * mWidth * kBytesPerPixel );

  // Go through our draw levels. For each one, get the Layer.
  map<int,int>::iterator tLevelLayerID;
  for( tLevelLayerID = mLevelLayerIDMap.begin(); 
       tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {

    int layerID = (*tLevelLayerID).second;
    try { 
      Layer& layer = Layer::FindByID( layerID );

      // tell it to draw into our buffer with our view state information.
      layer.DrawIntoBuffer( mBuffer, mWidth, mHeight, mViewState, *this );
    }
    catch(...) {
      cerr << "Couldn't find layer " << layerID << endl;
    }
  }
}

void
ScubaView::DrawFrameBuffer () {

  glRasterPos2i( 0, 0 );
  glDrawPixels( mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, mBuffer );
}

void 
ScubaView::BuildOverlay () {

  if( !mbRebuildOverlayDrawList )
    return;

  // Open the overlay display list.
  glNewList( kOverlayDrawListID + mID, GL_COMPILE );
    

  // Draw the HUD overlay if necessary. We need to take our edge
  // window coords and translate them to RAS coords and draw them on
  // the sides of the screens. Note w'ere only really using one of the
  // coords in each left/right/bottom/top/plane calculation because we
  // don't care about the other dimensions.
  ScubaGlobalPreferences& prefs = ScubaGlobalPreferences::GetPreferences();
  if( prefs.GetPrefAsBool( ScubaGlobalPreferences::DrawCoordinateOverlay )) {

    int window[2];
    float ras[3];
    char sXLabel, sYLabel, sZLabel;
    float left, right, top, bottom, plane;
    switch( mViewState.mInPlane ) {
    case ViewState::X: 
      sXLabel = 'a';
      sYLabel = 's';
      sZLabel = 'r';
      window[0] = 0;       TranslateWindowToRAS( window, ras ); left  = ras[1];
      window[0] = mWidth;  TranslateWindowToRAS( window, ras ); right = ras[1];
      window[1] = 0;       TranslateWindowToRAS( window, ras ); bottom= ras[2];
      window[1] = mHeight; TranslateWindowToRAS( window, ras ); top   = ras[2];
      plane = ras[0];
      break;
    case ViewState::Y: 
      sXLabel = 'r';
      sYLabel = 's';
      sZLabel = 'a';
      window[0] = 0;       TranslateWindowToRAS( window, ras ); left  = ras[0];
      window[0] = mWidth;  TranslateWindowToRAS( window, ras ); right = ras[0];
      window[1] = 0;       TranslateWindowToRAS( window, ras ); bottom= ras[2];
      window[1] = mHeight; TranslateWindowToRAS( window, ras ); top   = ras[2];
      plane = ras[1];
      break;
    case ViewState::Z: 
    default:
      sXLabel = 'r';
      sYLabel = 'a';
      sZLabel = 's';
      window[0] = 0;       TranslateWindowToRAS( window, ras ); left  = ras[0];
      window[0] = mWidth;  TranslateWindowToRAS( window, ras ); right = ras[0];
      window[1] = 0;       TranslateWindowToRAS( window, ras ); bottom= ras[1];
      window[1] = mHeight; TranslateWindowToRAS( window, ras ); top   = ras[1];
      plane = ras[2];
      break;
    }
    
    glColor3f( 1, 1, 1 );
    
    // Now draw the labels we calc'd before.
    char sLabel[60];
    sprintf( sLabel, "%c%.2f", sXLabel, left );
    glRasterPos2i( 0, mHeight / 2 + 7 );
    for( int nChar = 0; nChar < (int)strlen( sLabel ); nChar++ ) {
      glutBitmapCharacter( GLUT_BITMAP_8_BY_13, sLabel[nChar] );
    }
    sprintf( sLabel, "%c%.2f", sXLabel, right );
    glRasterPos2i( mWidth - strlen(sLabel)*8, mHeight / 2 + 7 );
    for( int nChar = 0; nChar < (int)strlen( sLabel ); nChar++ ) {
      glutBitmapCharacter( GLUT_BITMAP_8_BY_13, sLabel[nChar] );
    }
    sprintf( sLabel, "%c%.2f", sYLabel, top );
    glRasterPos2i( mWidth / 2 - (strlen(sLabel)*8 / 2), mHeight-1-13 );
    for( int nChar = 0; nChar < (int)strlen( sLabel ); nChar++ ) {
      glutBitmapCharacter( GLUT_BITMAP_8_BY_13, sLabel[nChar] );
    }
    sprintf( sLabel, "%c%.2f", sYLabel, bottom );
    glRasterPos2i( mWidth / 2 - (strlen(sLabel)*8 / 2), 4 );
    for( int nChar = 0; nChar < (int)strlen( sLabel ); nChar++ ) {
      glutBitmapCharacter( GLUT_BITMAP_8_BY_13, sLabel[nChar] );
    }
    sprintf( sLabel, "%c%.2f", sZLabel, plane );
    glRasterPos2i( mWidth - (strlen(sLabel)*8), 4 );
    for( int nChar = 0; nChar < (int)strlen( sLabel ); nChar++ ) {
      glutBitmapCharacter( GLUT_BITMAP_8_BY_13, sLabel[nChar] );
    }
    if( mViewState.mZoomLevel != 1 ) {
      sprintf( sLabel, "%.2fx", mViewState.mZoomLevel );
      glRasterPos2i( 0, mHeight-1-13 );
      for( int nChar = 0; nChar < (int)strlen( sLabel ); nChar++ ) {
	glutBitmapCharacter( GLUT_BITMAP_8_BY_13, sLabel[nChar] );
      }
    }
  }

  if( prefs.GetPrefAsBool( ScubaGlobalPreferences::DrawCenterCrosshairOverlay )) {

    // Draw our marker color around us.
    glColor3f( mInPlaneMarkerColor[0], 
	       mInPlaneMarkerColor[1], mInPlaneMarkerColor[2] );
    glBegin( GL_LINE_STRIP );
    glVertex2d( 2, 2 );
    glVertex2d( mWidth-3, 2 );
    glVertex2d( mWidth-3, mHeight-3 );
    glVertex2d( 2, mHeight-3 );
    glVertex2d( 2, 2 );
    glEnd();
    

    // For each other visible view...
    list<int> viewIDs;
    GetIDList( viewIDs );
    list<int>::iterator tViewID;
    for( tViewID = viewIDs.begin(); tViewID != viewIDs.end(); ++tViewID ) {

      int viewID = *tViewID;
      
      if( viewID != GetID () ) {

	try {
	  View& view = View::FindByID( viewID );
	  // ScubaView& scubaView = dynamic_cast<ScubaView&>(view);
	  ScubaView& scubaView = (ScubaView&)view;
	  
	  if( scubaView.IsVisibleInFrame() ) {
	    
	    // If its inplane is not ours..
	    if( scubaView.Get2DInPlane() != Get2DInPlane() ) {
	      
	      // Get its marker color and middle coordinate draw an
	      // appropriate line.
	      float color[3];
	      float viewCenterRAS[3];
	      int viewCenterWindow[2];
	      scubaView.GetInPlaneMarkerColor( color );
	      scubaView.Get2DRASCenter( viewCenterRAS );
	      
	      TranslateRASToWindow( viewCenterRAS, viewCenterWindow );
	      
	      glColor3f( color[0], color[1], color[2] );	    
	      glBegin( GL_LINES );
	      
	      switch( Get2DInPlane() ) { 
	      case ViewState::X: 
		if( scubaView.Get2DInPlane() == ViewState::Y ) {
		  glVertex2d( viewCenterWindow[0], 0 ); 
		  glVertex2d( viewCenterWindow[0], mHeight-1 );
		} else if( scubaView.Get2DInPlane() == ViewState::Z ){
		  glVertex2d( 0, viewCenterWindow[1] ); 
		  glVertex2d( mWidth-1, viewCenterWindow[1] );
		}
		break;
	      case ViewState::Y:
		if( scubaView.Get2DInPlane() == ViewState::X ) {
		  glVertex2d( viewCenterWindow[0], 0 ); 
		  glVertex2d( viewCenterWindow[0], mHeight-1 );
		} else if( scubaView.Get2DInPlane() == ViewState::Z ) {
		  glVertex2d( 0, viewCenterWindow[1] ); 
		  glVertex2d( mWidth-1, viewCenterWindow[1] );
		}
		break;
	      case ViewState::Z:
		if( scubaView.Get2DInPlane() == ViewState::X ) {
		  glVertex2d( viewCenterWindow[0], 0 ); 
		  glVertex2d( viewCenterWindow[0], mHeight-1 );
		} else if( scubaView.Get2DInPlane() == ViewState::Y ) {
		  glVertex2d( 0, viewCenterWindow[1] ); 
		  glVertex2d( mWidth-1, viewCenterWindow[1] );
		}
		break;
	      }
	      
	      glEnd();
	    }
	  }
	}
	catch(...) {
	  
	}
      }
    }
  }

  glEndList();

  mbRebuildOverlayDrawList = false;
}

void
ScubaView::RebuildLabelValueInfo ( float iRAS[3] ) {

  map<string,string> labelValueMap;

  stringstream sID;
  sID << GetLabel() << " (" << GetID() << ")";
  labelValueMap["View"] = sID.str();

  // Get the RAS coords into a string and set that label/value.
  stringstream ssRASCoords;
  ssRASCoords.width(5);
  ssRASCoords << iRAS[0] << " " << iRAS[1] << " " << iRAS[2];
  labelValueMap["RAS"] = ssRASCoords.str();

  // Go through our draw levels. For each one, get the Layer.
  map<int,int>::iterator tLevelLayerID;
  for( tLevelLayerID = mLevelLayerIDMap.begin(); 
       tLevelLayerID != mLevelLayerIDMap.end(); ++tLevelLayerID ) {

    int layerID = (*tLevelLayerID).second;
    try {
      Layer& layer = Layer::FindByID( layerID );
      
      // Ask the layer for info strings at this point.
      layer.GetInfoAtRAS( iRAS, labelValueMap );
    }
    catch(...) {
      DebugOutput( << "Couldn't find layer " << layerID );
    }
  }

  // Set this labelValueMap in the array of label values under the
  // name 'cursor'.
  mLabelValueMaps["cursor"] = labelValueMap;

}

void
ScubaView::GetInPlaneMarkerColor ( float oColor[3] ) {

  oColor[0] = mInPlaneMarkerColor[0];
  oColor[1] = mInPlaneMarkerColor[1];
  oColor[2] = mInPlaneMarkerColor[2];
}

void
ScubaView::DrawOverlay () {

  if( mbRebuildOverlayDrawList )
    BuildOverlay();

  glCallList( kOverlayDrawListID + mID );
}

ScubaViewBroadcaster&
ScubaViewBroadcaster::GetBroadcaster () {
  static ScubaViewBroadcaster* sBroadcaster = NULL;
  if( NULL == sBroadcaster ) {
    sBroadcaster = new ScubaViewBroadcaster();
  }
  return *sBroadcaster;
}
