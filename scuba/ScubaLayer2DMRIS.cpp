#include <list>
#include "ScubaLayer2DMRIS.h"
#include "VectorOps.h"

using namespace std;

ScubaLayer2DMRIS::ScubaLayer2DMRIS () {
  SetOutputStreamToCerr();
  mSurface = NULL;
  maLineColor[0] = 0;
  maLineColor[1] = 255;
  maLineColor[2] = 0;
  maVertexColor[0] = 255;
  maVertexColor[1] = 0;
  maVertexColor[2] = 255;

  TclCommandManager& commandMgr = TclCommandManager::GetManager();
  commandMgr.AddCommand( *this, "Set2DMRISLayerSurfaceCollection", 2, 
			 "layerID collectionID",
			 "Sets the surface collection for this layer." );
  commandMgr.AddCommand( *this, "Get2DMRISLayerSurfaceCollection", 1, 
			 "layerID",
			 "Returns the surface collection for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRISLayerLineColor", 4, 
			 "layerID red green blue",
			 "Sets the line color for this layer. red, green, "
			 "and blue should be 0-255 integers." );
  commandMgr.AddCommand( *this, "Get2DMRISLayerLineColor", 1, "layerID",
			 "Returns the line color for this layer as a list "
			 " of red, green, and blue integers from 0-255." );
  commandMgr.AddCommand( *this, "Set2DMRISLayerVertexColor", 4, 
			 "layerID red green blue",
			 "Sets the vertex color for this layer. red, green, "
			 "and blue should be 0-255 integers." );
  commandMgr.AddCommand( *this, "Get2DMRISLayerVertexColor", 1, "layerID",
			 "Returns the vertex color for this layer as a list "
			 " of red, green, and blue integers from 0-255." );
  commandMgr.AddCommand( *this, "Get2DMRISRASCoordsFromVertexIndex", 2,
			 "layerID vertexIndex", "Returns as a list of RAS "
			 "coords the location of the vertex." );
  commandMgr.AddCommand( *this, "Get2DMRISNearestVertexIndex", 4,
			 "layerID x y z", "Returns the index of the vertex "
			 "closest to the input RAS coords and the distance "
			 "to that vertex." );

}

ScubaLayer2DMRIS::~ScubaLayer2DMRIS () {

}

void 
ScubaLayer2DMRIS::SetSurfaceCollection ( SurfaceCollection& iSurface ) {

  mSurface = &iSurface;

  mSurface->GetMRIS();
}

void
ScubaLayer2DMRIS::DrawIntoBuffer ( GLubyte* iBuffer, int iWidth, int iHeight,
				   ViewState& iViewState,
				   ScubaWindowToRASTranslator& iTranslator ) {

  if( NULL == mSurface ) {
    DebugOutput( << "No surface to draw" );
    return;
  }

  // Get a point and a normal for our view plane.
  Point3<float> planeRAS( iViewState.mCenterRAS );
  Point3<float> planeN( iViewState.mPlaneNormal );
  
  list<int> drawList;
  int cIntersectionsInFace = 0;
  int intersectionPair[2][2];

  // We need to look for intersections of edges in a face and the
  // current plane.
  int cFaces = mSurface->GetNumFaces();
  for( int nFace = 0; nFace < cFaces; nFace++ ) {

    cIntersectionsInFace = 0;

    // Look at each edge in this face...
    int cVerticesPerFace = mSurface->GetNumVerticesPerFace_Unsafe( nFace );
    for( int nVertex = 0; nVertex < cVerticesPerFace; nVertex++ ) {

      int nNextVertex = nVertex + 1;
      if( nNextVertex >= cVerticesPerFace ) nNextVertex = 0;

      // Get the vertices.
      Point3<float> vRAS, vnRAS;
      bool bRipped, bNextRipped;
      mSurface->GetNthVertexInFace_Unsafe( nFace, nVertex, 
					   vRAS.xyz(), &bRipped );
      mSurface->GetNthVertexInFace_Unsafe( nFace, nNextVertex, 
					   vnRAS.xyz(), &bNextRipped );

      // Don't draw ripped verts.
      if( bRipped || bNextRipped ) {
	continue;
      }

      // If they cross the view's in plane coordinate...
      VectorOps::IntersectionResult rInt;
      Point3<float> intersectionRAS;
      rInt = VectorOps::SegmentIntersectsPlane
	( vRAS, vnRAS, planeRAS, planeN, intersectionRAS );
      if( VectorOps::intersect == rInt ) {

	// Translate intersection point to a window coord. If it's in
	// the view...
	int window[2];
	iTranslator.TranslateRASToWindow( intersectionRAS.xyz(), window );

	if( window[0] >= 0 && window[0] < iWidth && 
	    window[1] >= 0 && window[1] < iHeight ) { 

	  // Add this intersection window point to our pair. If we
	  // have two intersections, they make a line of the
	  // intersection of this face and the in plane, so add them
	  // to the list of points to draw.
	  intersectionPair[cIntersectionsInFace][0] = window[0];
	  intersectionPair[cIntersectionsInFace][1] = window[1];
	  
	  cIntersectionsInFace++;

	  if( cIntersectionsInFace == 2 ) {
	    cIntersectionsInFace = 0;
	    drawList.push_back( intersectionPair[0][0] );
	    drawList.push_back( intersectionPair[0][1] );
	    drawList.push_back( intersectionPair[1][0] );
	    drawList.push_back( intersectionPair[1][1] );
	  }

	}
      }
    }
  }


  // Draw all the intersection points we just calced.
  bool bDraw = false;
  int window1[2];
  int window2[2];
  list<int>::iterator tDrawList;
  for( tDrawList = drawList.begin(); 
       tDrawList != drawList.end(); ++tDrawList ) {
    
    // First time around, just save a point, next time around, draw
    // the line they make. Also draw pixels for the intersection
    // points themselves.
    if( !bDraw ) {

      window1[0] = *tDrawList;
      window1[1] = *(++tDrawList);
      bDraw = true;

    } else {

      window2[0] = *tDrawList;
      window2[1] = *(++tDrawList);
      bDraw = false;

      DrawPixelIntoBuffer( iBuffer, iWidth, iHeight, window1, 
			   maVertexColor, mOpacity );
      DrawPixelIntoBuffer( iBuffer, iWidth, iHeight, window2, 
			   maVertexColor, mOpacity );

      DrawLineIntoBuffer( iBuffer, iWidth, iHeight, window1, window2,
			  maLineColor, 1, mOpacity );
    }
  }
}
  
void
ScubaLayer2DMRIS::GetInfoAtRAS ( float iRAS[3],
				 map<string,string>& iLabelValues) {
  
  if( NULL != mSurface ) {
    
    float distance;
    int nVertex = FindNearestVertexToRAS( iRAS, &distance );
    
    stringstream ssVertex;
    ssVertex << nVertex;
    iLabelValues[mSurface->GetLabel() + ",vertex"] = ssVertex.str();
    
    stringstream ssDistance;
    ssDistance << distance;
    iLabelValues[mSurface->GetLabel() + ",distance"] = ssDistance.str();
  }
}

void
ScubaLayer2DMRIS::HandleTool ( float iRAS[3], ViewState& iViewState,
			       ScubaWindowToRASTranslator& iTranslator,
			       ScubaToolState& iTool, InputState& iInput ) {

}

void
ScubaLayer2DMRIS::FindRASLocationOfVertex ( int inVertex, float oRAS[3] ) {

  if( NULL == mSurface ) {
    throw runtime_error( "No surface loaded." );
  }

  if( inVertex < mSurface->GetNumVertices() ) {
    mSurface->GetNthVertex_Unsafe( inVertex, oRAS, NULL );
  } else {
    throw runtime_error( "Vertex index is out of bounds." );
  }
}

int
ScubaLayer2DMRIS::FindNearestVertexToRAS ( float iRAS[3], float* oDistance ) {

  if( NULL == mSurface ) {
    throw runtime_error( "No surface loaded." );
  }

  float minDistance = 1000;
  int nClosestVertex = -1;
  for( int nVertex = 0; nVertex < mSurface->GetNumVertices(); nVertex++ ) {

    float ras[3];
    bool bRipped;
    mSurface->GetNthVertex_Unsafe( nVertex, ras, &bRipped );
    
    if( !bRipped ) {

      float dx = iRAS[0] - ras[0];
      float dy = iRAS[1] - ras[1];
      float dz = iRAS[2] - ras[2];
      float distance = sqrt( dx*dx + dy*dy + dz*dz );
      if( distance < minDistance ) {
	minDistance = distance;
	nClosestVertex = nVertex;
      }
    }
  }

  if( -1 == nClosestVertex ) {
    throw runtime_error( "No vertices found.");
  }

  if( NULL != oDistance ) {
    *oDistance = minDistance;
  }
  return nClosestVertex;
}

TclCommandManager::TclCommandResult 
ScubaLayer2DMRIS::DoListenToTclCommand ( char* isCommand, 
					 int iArgc, char** iasArgv ) {

  // Set2DMRISLayerSurfaceCollection <layerID> <collectionID>
  if( 0 == strcmp( isCommand, "Set2DMRISLayerSurfaceCollection" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      int collectionID = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad collection ID";
	return error;
      }
	
      try { 
	DataCollection& data = DataCollection::FindByID( collectionID );
	if( data.GetID() != collectionID ) {
	  cerr << "IDs didn't match" << endl;
	}
	SurfaceCollection& surface = (SurfaceCollection&)data;
	// VolumeCollection& volume = dynamic_cast<VolumeCollection&>(data);
	
	SetSurfaceCollection( surface );
      }
      catch(...) {
	sResult = "bad collection ID, collection not found";
	return error;
      }
    }
  }

  // Get2DMRISLayerSurfaceCollection <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRISLayerSurfaceCollection" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {

      stringstream ssReturnValues;
      ssReturnValues << (int) (mSurface->GetID());
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "i";
    }
  }

  // Set2DMRISLayerLineColor <layerID> <red> <green> <blue>
  if( 0 == strcmp( isCommand, "Set2DMRISLayerLineColor" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      int red = strtol( iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad red";
	return error;
      }
      
      int green = strtol( iasArgv[3], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad green";
	return error;
      }
      
      int blue = strtol( iasArgv[4], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad blue";
	return error;
      }
      
      int color[3];
      color[0] = red;
      color[1] = green;
      color[2] = blue;
      SetLineColor3d( color );
    }
  }

  // Get2DMRISLayerLineColor <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRISLayerLineColor" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "Liiil";
      stringstream ssReturnValues;
      ssReturnValues << maLineColor[0] << " " << maLineColor[1] << " "
		     << maLineColor[2];
      sReturnValues = ssReturnValues.str();
    }
  }

  // Set2DMRISLayerVertexColor <layerID> <red> <green> <blue>
  if( 0 == strcmp( isCommand, "Set2DMRISLayerVertexColor" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      int red = strtol( iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad red";
	return error;
      }
      
      int green = strtol( iasArgv[3], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad green";
	return error;
      }
      
      int blue = strtol( iasArgv[4], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad blue";
	return error;
      }
      
      int color[3];
      color[0] = red;
      color[1] = green;
      color[2] = blue;
      SetVertexColor3d( color );
    }
  }

  // Get2DMRISLayerVertexColor <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRISLayerVertexColor" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "Liiil";
      stringstream ssReturnValues;
      ssReturnValues << maVertexColor[0] << " " << maVertexColor[1] << " "
		     << maVertexColor[2];
      sReturnValues = ssReturnValues.str();
    }
  }

  // Get2DMRISRASCoordsFromVertexIndex <layerID> <vertexIndex>
  if( 0 == strcmp( isCommand, "Get2DMRISRASCoordsFromVertexIndex" ) ) {
    int layerID;
    try {
      layerID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
    }
    catch( runtime_error& e ) {
      sResult = string("bad layerID: ") + e.what();
      return error;
    }
    
    if( mID == layerID ) {

      int nVertex;
      nVertex = TclCommandManager::ConvertArgumentToInt( iasArgv[2] );

      float ras[3];
      FindRASLocationOfVertex( nVertex, ras );
      
      stringstream ssReturnValues;
      ssReturnValues << ras[0] << " " << ras[1] << " " << ras[2];
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "Lfffl";

      return ok;
    }  
  }

  // Get2DMRISNearestVertexIndex <layerID> <x> <y> <z>
  if( 0 == strcmp( isCommand, "Get2DMRISNearestVertexIndex" ) ) {
    int layerID;
    try {
      layerID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
    }
    catch( runtime_error& e ) {
      sResult = string("bad layerID: ") + e.what();
      return error;
    }
    
    if( mID == layerID ) {

      float ras[3];
      try { 
	ras[0] = TclCommandManager::ConvertArgumentToFloat( iasArgv[2] );
	ras[1] = TclCommandManager::ConvertArgumentToFloat( iasArgv[3] );
	ras[2] = TclCommandManager::ConvertArgumentToFloat( iasArgv[4] );
      }
      catch( runtime_error& e ) {
	sResult = string("bad RAS coord: ") + e.what();
	return error;
      }

      try {
	int nVertex;
	float distance;
	nVertex = FindNearestVertexToRAS( ras, &distance );
	
	stringstream ssReturnValues;
	ssReturnValues << nVertex << " " << distance;
	sReturnValues = ssReturnValues.str();
	sReturnFormat = "Lifl";
      }
      catch( runtime_error& e) {
	sResult = string("Couldn't find vertex: ") + e.what();
	return error;
      }

      return ok;
    }  
  }

  return Layer::DoListenToTclCommand( isCommand, iArgc, iasArgv );
}

