#include "string_fixed.h"
#include <errno.h>
#include <stdexcept>
#include "SurfaceCollection.h"
#include "DataManager.h"
#include "VolumeCollection.h"

using namespace std;


SurfaceCollection::SurfaceCollection () :
  DataCollection() {

  mMRIS = NULL;
  mbIsUsingVolumeForTransform = false;
  mTransformVolume = NULL;

  TclCommandManager& commandMgr = TclCommandManager::GetManager();
  commandMgr.AddCommand( *this, "SetSurfaceCollectionFileName", 2, 
			 "collectionID fileName", 
			 "Sets the file name for a given surface collection.");
  commandMgr.AddCommand( *this, "GetSurfaceCollectionFileName", 1, 
			 "collectionID", 
			 "Gets the file name for a given surface collection.");
  commandMgr.AddCommand( *this, "SetSurfaceDataToSurfaceTransformFromVolume",
			 2, "collectionID volumeID", 
			 "Gets the data to surface transform from a volume." );
  commandMgr.AddCommand( *this, "SetSurfaceDataToSurfaceTransformToDefault",
			 1, "collectionID", 
			 "Sets the data to surface transform for a surface "
			 "to the default, which will be ../mri/orig or "
			 "identity." );
  commandMgr.AddCommand(*this,"IsSurfaceUsingDataToSurfaceTransformFromVolume",
			 1, "collectionID", 
			 "Returns whether or not a surface collection is "
			 "using a volume to get its data to surface "
			 "transform." );
  commandMgr.AddCommand( *this, "GetSurfaceDataToSurfaceTransformVolume",
			 1, "collectionID", 
			 "If a surface collection is using a volume "
			 "to get its data to surface transform, returns "
			 "the volume's collection ID." );
}

SurfaceCollection::~SurfaceCollection() {

  DataManager dataMgr = DataManager::GetManager();
  MRISLoader mrisLoader = dataMgr.GetMRISLoader();
  try { 
    mrisLoader.ReleaseData( &mMRIS );
  } 
  catch(...) {
    cerr << "Couldn't release data"  << endl;
  }
}


void
SurfaceCollection::SetSurfaceFileName ( string& ifnMRIS ) {


  if( NULL != mMRIS ) {
    
    DataManager dataMgr = DataManager::GetManager();
    MRISLoader mrisLoader = dataMgr.GetMRISLoader();
    try { 
      mrisLoader.ReleaseData( &mMRIS );
    } 
    catch(...) {
      cerr << "Couldn't release data"  << endl;
    }

    mMRIS = NULL;
  }

  mfnMRIS = ifnMRIS;

  GetMRIS();
}

MRIS*
SurfaceCollection::GetMRIS () { 

  if( NULL == mMRIS ) {
    
    DataManager dataMgr = DataManager::GetManager();
    MRISLoader mrisLoader = dataMgr.GetMRISLoader();
    
    mMRIS = NULL;
    try { 
      mMRIS = mrisLoader.GetData( mfnMRIS );
    }
    catch( exception e ) {
      throw logic_error( "Couldn't load MRIS" );
    }

    // Check the volume geometry info in the surface. If it's there,
    // get our transform from there.
    if( mMRIS->vg.valid ) {

      mDataToSurfaceTransform.SetMainTransform
	( mMRIS->vg.x_r, mMRIS->vg.y_r, mMRIS->vg.z_r, mMRIS->vg.c_r,
	  mMRIS->vg.x_a, mMRIS->vg.y_a, mMRIS->vg.z_a, mMRIS->vg.c_a,
	  mMRIS->vg.x_s, mMRIS->vg.y_s, mMRIS->vg.z_s, mMRIS->vg.c_s,
	               0,            0,             0,             1 );


      // Otherwise look for it in the lta field.
    } else if( NULL != mMRIS->lta ) {

      // This is our RAS -> surfaceRAS transform.
      mDataToSurfaceTransform.SetMainTransform
	( 1, 0, 0, -mMRIS->lta->xforms[0].src.c_r,
	  0, 1, 0, -mMRIS->lta->xforms[0].src.c_a,
	  0, 0, 1, -mMRIS->lta->xforms[0].src.c_s,
	  0, 0, 0, 1 );
    }

    CalcWorldToSurfaceTransform();

    

  }

  if( msLabel == "" ) {
    SetLabel( mfnMRIS );
  }

  return mMRIS;
}

TclCommandListener::TclCommandResult 
SurfaceCollection::DoListenToTclCommand ( char* isCommand,
					 int iArgc, char** iasArgv ) {

  // SetSurfaceCollectionFileName <collectionID> <fileName>
  if( 0 == strcmp( isCommand, "SetSurfaceCollectionFileName" ) ) {
    int collectionID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad collection ID";
      return error;
    }
    
    if( mID == collectionID ) {
      
      string fnSurface = iasArgv[2];
      SetSurfaceFileName( fnSurface );
    }
  }
  
  // GetSurfaceCollectionFileName <collectionID>
  if( 0 == strcmp( isCommand, "GetSurfaceCollectionFileName" ) ) {
    int collectionID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad collection ID";
      return error;
    }
    
    if( mID == collectionID ) {
      
      sReturnFormat = "s";
      sReturnValues = mfnMRIS;
    }
  }
  
  // SetSurfaceDataToSurfaceTransformFromVolume <collectionID> <volumeID>
  if( 0 == strcmp( isCommand, "SetSurfaceDataToSurfaceTransformFromVolume") ) {

    int collectionID;
    try { 
      collectionID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
      }
    catch( runtime_error e ) {
      sResult = string("bad collection ID: ") + e.what();
      return error;
    }
    
    if( mID == collectionID ) {
      
      try {
	int volumeID = 
	  TclCommandManager::ConvertArgumentToInt( iasArgv[2] );

	DataCollection& col = DataCollection::FindByID( volumeID );
	VolumeCollection& vol = (VolumeCollection&)col;
	//	  dynamic_cast<VolumeCollection&>( col );
	
	SetDataToSurfaceTransformFromVolume( vol );
      }
      catch( runtime_error e ) {
	sResult = e.what();
	return error;
      }
    }
  }
  
  // SetSurfaceDataToSurfaceTransformToDefault <collectionID>
  if( 0 == strcmp( isCommand, "SetSurfaceDataToSurfaceTransformToDefault") ) {

    int collectionID;
    try { 
      collectionID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
      }
    catch( runtime_error e ) {
      sResult = string("bad collection ID: ") + e.what();
      return error;
    }
    
    if( mID == collectionID ) {
      
      SetDataToSurfaceTransformToDefault();
    }
  }
  
  // IsSurfaceUsingDataToSurfaceTransformFromVolume <collectionID>
  if( 0 == strcmp( isCommand, "IsSurfaceUsingDataToSurfaceTransformFromVolume" ) ) {

    int collectionID;
    try { 
      collectionID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
      }
    catch( runtime_error e ) {
      sResult = string("bad collection ID: ") + e.what();
      return error;
    }
    
    if( mID == collectionID ) {
      
      sReturnValues = 
	TclCommandManager::ConvertBooleanToReturnValue( mbIsUsingVolumeForTransform );
      sReturnFormat = "i";
    }
  }

  // GetSurfaceDataToSurfaceTransformVolume <transformID>
  if( 0 == strcmp( isCommand, "GetSurfaceDataToSurfaceTransformVolume" ) ) {

    int collectionID;
    try { 
      collectionID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
      }
    catch( runtime_error e ) {
      sResult = string("bad collection ID: ") + e.what();
      return error;
    }
    
    if( mID == collectionID ) {
      
      stringstream ssReturnValues;
      if( NULL != mTransformVolume ) {
	ssReturnValues << mTransformVolume->GetID();
      } else {
	ssReturnValues << 0;
      }
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "i";
    }
  }

  return DataCollection::DoListenToTclCommand( isCommand, iArgc, iasArgv );
}

void
SurfaceCollection::DoListenToMessage ( string isMessage, void* iData ) {
  
  if( isMessage == "transformChanged" ) {
    CalcWorldToSurfaceTransform();
  }

  DataCollection::DoListenToMessage( isMessage, iData );
}

ScubaROI*
SurfaceCollection::DoNewROI () {

  return NULL;
}

void
SurfaceCollection::RASToSurface  ( float iRAS[3], float oSurface[3] ) {

  mWorldToSurfaceTransform.MultiplyVector3( iRAS, oSurface );
}

void
SurfaceCollection::SurfaceToRAS  ( float iSurface[3], float oRAS[3] ) {

  mWorldToSurfaceTransform.InvMultiplyVector3( iSurface, oRAS );
}

int
SurfaceCollection::GetNumFaces () {

  if( NULL != mMRIS ) {
    return mMRIS->nfaces;
  }

  return 0;
}

int
SurfaceCollection::GetNumVerticesPerFace_Unsafe ( int ) {

  return VERTICES_PER_FACE;
}

void
SurfaceCollection::GetNthVertexInFace_Unsafe ( int inFace, int inVertex, 
					       float oRAS[3] ) {

  VERTEX* vertex = &(mMRIS->vertices[mMRIS->faces[inFace].v[inVertex]]);
  float dataRAS[3];
  dataRAS[0] = vertex->x;
  dataRAS[1] = vertex->y;
  dataRAS[2] = vertex->z;

  SurfaceToRAS( dataRAS, oRAS );
}

void
SurfaceCollection::CalcWorldToSurfaceTransform () {
  
  Transform44 worldToData = mDataToWorldTransform->Inverse();
  Transform44 tmp = mDataToSurfaceTransform * worldToData;
  mWorldToSurfaceTransform = tmp;

  DataChanged();
}

void
SurfaceCollection::SetDataToSurfaceTransformFromVolume( VolumeCollection&
							iVolume ) {

  if( mbIsUsingVolumeForTransform ) {
    if( NULL != mTransformVolume ) {
      if( mTransformVolume->GetID() == iVolume.GetID() ) {
	return;
      }
    }
  }

  // We want to get the MRIsurfaceRASToRAS matrix from the volume.
  MRI* mri = iVolume.GetMRI();
  if( NULL == mri ) {
    throw runtime_error( "Couldn't get MRI from volume" );
  }
  
  MATRIX* RASToSurfaceRASMatrix = surfaceRASFromRAS_( mri );
  if( NULL == RASToSurfaceRASMatrix ) {
    throw runtime_error( "Couldn't get SurfaceRASFromRAS_ from MRI" );
  }
  
  mDataToSurfaceTransform.SetMainTransform( RASToSurfaceRASMatrix );

  CalcWorldToSurfaceTransform();

  MatrixFree( &RASToSurfaceRASMatrix );
  
  mbIsUsingVolumeForTransform = true;
  mTransformVolume = &iVolume;
}

void
SurfaceCollection::SetDataToSurfaceTransformToDefault () {

  if( !mbIsUsingVolumeForTransform )
    return;

  // We want to get the lta from the mris or else use identity.
  if( NULL != mMRIS->lta ) {
    
    // This is our RAS -> surfaceRAS transform.
    mDataToSurfaceTransform.SetMainTransform
      ( 1, 0, 0, -mMRIS->lta->xforms[0].src.c_r,
	0, 1, 0, -mMRIS->lta->xforms[0].src.c_a,
	0, 0, 1, -mMRIS->lta->xforms[0].src.c_s,
	0, 0, 0, 1 );
  } else {

    mDataToSurfaceTransform.MakeIdentity();
  }
  
  CalcWorldToSurfaceTransform();

  mbIsUsingVolumeForTransform = false;
  mTransformVolume = NULL;
}
