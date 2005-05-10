#include <list>
#include "ScubaLayer2DMRIS.h"

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
      mSurface->GetNthVertexInFace_Unsafe( nFace, nVertex, vRAS.xyz() );
      mSurface->GetNthVertexInFace_Unsafe( nFace, nNextVertex, vnRAS.xyz() );

      // Get the coordinate we need to compare for this plane. We look
      // at the inplane coordinates in each vertex.
      float vertexCoord, nextVertexCoord, planeCoord;
      switch( iViewState.mInPlane ) {
      case ViewState::X:
	vertexCoord     = vRAS.x();
	nextVertexCoord = vnRAS.x();
	planeCoord      = iViewState.mCenterRAS[0];
	break;
      case ViewState::Y:
	vertexCoord     = vRAS.y();
	nextVertexCoord = vnRAS.y();;
	planeCoord      = iViewState.mCenterRAS[1];
	break;
      case ViewState::Z:
      default:
	vertexCoord     = vRAS.z();
	nextVertexCoord = vnRAS.z();
	planeCoord      = iViewState.mCenterRAS[2];
	break;
      }

      // If they cross the view's in plane coordinate...
      if( (vertexCoord - planeCoord) * (nextVertexCoord - planeCoord) <= 0.0 ) {

	// Calculate the intersection point of the edge with this
	// plane.
	float f = (planeCoord - vertexCoord) / (nextVertexCoord - vertexCoord);

	float world[3];
	switch( iViewState.mInPlane ) {
	case ViewState::X:
	  world[0] = iViewState.mCenterRAS[0];
	  world[1] = vRAS.y() + f * (vnRAS.y() - vRAS.y());
	  world[2] = vRAS.z() + f * (vnRAS.z() - vRAS.z());
	  break;
	case ViewState::Y:
	  world[0] = vRAS.x() + f * (vnRAS.x() - vRAS.x());
	  world[1] = iViewState.mCenterRAS[1];
	  world[2] = vRAS.z() + f * (vnRAS.z() - vRAS.z());
	  break;
	case ViewState::Z:
	  world[0] = vRAS.x() + f * (vnRAS.x() - vRAS.x());
	  world[1] = vRAS.y() + f * (vnRAS.y() - vRAS.y());
	  world[2] = iViewState.mCenterRAS[2];
	  break;
	}

	// Translate it to a window coord. If it's in the view...
	int window[2];
	iTranslator.TranslateRASToWindow( world, window );

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
ScubaLayer2DMRIS::GetInfoAtRAS ( float[3],
				 std::map<std::string,std::string>& ) {

}

void
ScubaLayer2DMRIS::HandleTool ( float iRAS[3], ViewState& iViewState,
			       ScubaWindowToRASTranslator& iTranslator,
			       ScubaToolState& iTool, InputState& iInput ) {

}

void
ScubaLayer2DMRIS::FindRASLocationOfVertex ( int inVertex, float oRAS[3] ) {

  if( inVertex < mSurface->GetNumVertices() ) {
    mSurface->GetNthVertex_Unsafe( inVertex, oRAS );
  } else {
    throw runtime_error( "Vertex index is out of bounds." );
  }
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
    catch( runtime_error e ) {
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

  return Layer::DoListenToTclCommand( isCommand, iArgc, iasArgv );
}

