#include <fstream>
#include "ScubaLayer2DMRI.h"
#include "ViewState.h"
#include "talairachex.h"
#include "TclProgressDisplayManager.h"
#include "Utilities.h"
#include "Array2.h"
#include "PathManager.h"

using namespace std;

int const ScubaLayer2DMRI::cGrayscaleLUTEntries = 256;
int const ScubaLayer2DMRI::kMaxPixelComponentValue = 255;
float const ScubaLayer2DMRI::kMaxPixelComponentValueFloat = 255.0;

ScubaLayer2DMRI::ScubaLayer2DMRI () {

  SetOutputStreamToCerr();

  mVolume = NULL;
  mSampleMethod = nearest;
  mColorMapMethod = grayscale;
  mBrightness = 0.25;
  mContrast = 12.0; mNegContrast = -mContrast;
  mCurrentPath = NULL;
  mROIOpacity = 0.7;
  mbEditableROI = true;
  mbClearZero = false;

  // Try setting our initial color LUT to the default LUT with
  // id 0. If it's not there, create it.
  try { 
    mColorLUT = &(ScubaColorLUT::FindByID( 0 ));
  }
  catch(...) {

    ScubaColorLUT* lut = new ScubaColorLUT();
    lut->SetLabel( "Default" );
    
    try {
      mColorLUT = &(ScubaColorLUT::FindByID( 0 ));
    }
    catch(...) {
      DebugOutput( << "Couldn't make default lut!" );
    }
  }

  TclCommandManager& commandMgr = TclCommandManager::GetManager();
  commandMgr.AddCommand( *this, "Set2DMRILayerVolumeCollection", 2, 
			 "layerID collectionID",
			 "Sets the volume collection for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerColorMapMethod", 2, 
			 "layerID method",
			 "Sets the color map method for this layer." );
  commandMgr.AddCommand( *this, "Get2DMRILayerColorMapMethod", 1, "layerID",
			 "Returns the color map method for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerSampleMethod", 2, 
			 "layerID method",
			 "Sets the sample method for this layer." );
  commandMgr.AddCommand( *this, "Get2DMRILayerSampleMethod", 1, "layerID",
			 "Returns the sample method for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerBrightness", 2, 
			 "layerID brightness",
			 "Sets the brightness for this layer." );
  commandMgr.AddCommand( *this, "Get2DMRILayerBrightness", 1, "layerID",
			 "Returns the brightness for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerContrast", 2, 
			 "layerID contrast",
			 "Sets the contrast for this layer." );
  commandMgr.AddCommand( *this, "Get2DMRILayerContrast", 1, "layerID",
			 "Returns the contrast for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerColorLUT", 2, "layerID lutID",
			 "Sets the LUT  for this layer." );
  commandMgr.AddCommand( *this, "Get2DMRILayerColorLUT", 1, "layerID",
			 "Returns the LUT id for this layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerDrawZeroClear", 2, 
			 "layerID drawClear", "Sets property for drawing"
			 "values of zero clear." );
  commandMgr.AddCommand( *this, "Get2DMRILayerDrawZeroClear", 1, "layerID",
			 "Returns the value of the property for drawing"
			 "values of zero clear." );
  commandMgr.AddCommand( *this, "Set2DMRILayerMinVisibleValue", 2, 
			 "layerID value", "Sets the minimum value to be drawn."
			 "values of zero clear." );
  commandMgr.AddCommand( *this, "Get2DMRILayerMinVisibleValue", 1, "layerID",
			 "Returns the minimum value to be drawn." );
  commandMgr.AddCommand( *this, "Set2DMRILayerMaxVisibleValue", 2, 
			 "layerID value", "Sets the maximum value to be drawn."
			 "values of zero clear." );
  commandMgr.AddCommand( *this, "Get2DMRILayerMaxVisibleValue", 1, "layerID",
			 "Returns the maximum value to be drawn." );
  commandMgr.AddCommand( *this, "Get2DMRILayerMinValue", 1, "layerID",
			 "Returns the minimum value of the volume." );
  commandMgr.AddCommand( *this, "Get2DMRILayerMaxValue", 1, "layerID",
			 "Returns the maximum value of the volume." );
  commandMgr.AddCommand( *this, "Set2DMRILayerROIOpacity", 2,"layerID opacity",
			 "Sets the opacity of the ROI for a layer." );
  commandMgr.AddCommand( *this, "Get2DMRILayerROIOpacity", 1, "layerID",
			 "Returns the opacity of the ROI for a layer." );
  commandMgr.AddCommand( *this, "Set2DMRILayerEditableROI", 2, 
			 "layerID editable", "Specify whether or not this "
			 "layer's ROI is editable." );
  commandMgr.AddCommand( *this, "Get2DMRILayerEditableROI", 1, "layerID",
			 "Returns whether or not this layer's ROI is "
			 "editable." );
  
}

ScubaLayer2DMRI::~ScubaLayer2DMRI () {

}

void
ScubaLayer2DMRI::SetVolumeCollection ( VolumeCollection& iVolume ) {

  mVolume = &iVolume;

  mVolume->GetMRI();
  mMinVisibleValue = mVolume->GetMRIMinValue();
  mMaxVisibleValue = mVolume->GetMRIMaxValue();

  mVolume->AddListener( this );

  BuildGrayscaleLUT();
}

void 
ScubaLayer2DMRI::DrawIntoBuffer ( GLubyte* iBuffer, int iWidth, int iHeight,
				  ViewState& iViewState,
				  ScubaWindowToRASTranslator& iTranslator ) {

  if( NULL == mVolume ) {
    DebugOutput( << "No volume to draw" );
    return;
  }

  // Buffer for window -> RAS coords.
  Array2<Point3<float> > windowToRAS( iWidth, iHeight );

  // Precalc our color * opacity.
  GLubyte aColorTimesOpacity[256];
  GLubyte aColorTimesOneMinusOpacity[256];
  for( int i = 0; i < 256; i++ ) {
    aColorTimesOpacity[i] = (GLubyte)( (float)i * mOpacity );
    aColorTimesOneMinusOpacity[i] = (GLubyte)( (float)i * (1.0 - mOpacity) );
  }

  GLubyte* dest = iBuffer;
  int window[2];
  float RAS[3];
  float value = 0;
  int color[3];
  

  for( window[1] = 0; window[1] < iHeight; window[1]++ ) {
    for( window[0] = 0; window[0] < iWidth; window[0]++ ) {

      // Translate the window coord to an RAS and put it in our cache.
      iTranslator.TranslateWindowToRAS( window, RAS );
      windowToRAS.Set( window[0], window[1], Point3<float>( RAS ) );

      
      switch( mSampleMethod ) {
      case nearest:  value = mVolume->GetMRINearestValueAtRAS( RAS );  break;
      case trilinear:value = mVolume->GetMRITrilinearValueAtRAS( RAS );break;
      case sinc:     value = mVolume->GetMRISincValueAtRAS( RAS );     break;
      case magnitude:value = mVolume->GetMRIMagnitudeValueAtRAS( RAS );break;
      }
      
      //      memset (color, 0, sizeof(int) * 3);
      switch( mColorMapMethod ) { 
      case grayscale: GetGrayscaleColorForValue( value, dest, color );break;
      case heatScale: GetHeatscaleColorForValue( value, dest, color );break;
      case LUT:       GetColorLUTColorForValue( value, dest, color ); break;
      }
      
      dest[0] = aColorTimesOneMinusOpacity[dest[0]] + 
	aColorTimesOpacity[color[0]];
      dest[1] = aColorTimesOneMinusOpacity[dest[1]] + 
	aColorTimesOpacity[color[1]];
      dest[2] = aColorTimesOneMinusOpacity[dest[2]] + 
	aColorTimesOpacity[color[2]];
      //      dest[3] = (GLubyte) 255;
      
      dest += 3;
    }
  }


  dest = iBuffer;
  for( window[1] = 0; window[1] < iHeight; window[1]++ ) {
    for( window[0] = 0; window[0] < iWidth; window[0]++ ) {
      
      // Use our buffer to get an RAS point.
      Point3<float> RAS = windowToRAS.Get( window[0], window[1] );
      
      int selectColor[3];
      if( mVolume->IsRASInMRIBounds( RAS.xyz() ) ) {
	if( mVolume->IsRASSelected( RAS.xyz(), selectColor ) ) {

	  // Write the RGB value to the buffer. Write a 255 in the
	  // alpha byte.
	  dest[0] = (GLubyte) (((float)dest[0] * (1.0 - mROIOpacity)) +
			       ((float)selectColor[0] * mROIOpacity));
	  dest[1] = (GLubyte) (((float)dest[1] * (1.0 - mROIOpacity)) +
			       ((float)selectColor[1] * mROIOpacity));
	  dest[2] = (GLubyte) (((float)dest[2] * (1.0 - mROIOpacity)) +
			       ((float)selectColor[2] * mROIOpacity));
	}

	if( mVolume->IsRASPath( RAS.xyz() ) ) {
	  dest[0] = (GLubyte) ((float)dest[0] / 2.0 + 128.0);
	  dest[1] = (GLubyte) ((float)dest[1] / 2.0);
	  dest[2] = (GLubyte) ((float)dest[2] / 2.0);
	}
      }
      
      // Advance our pixel buffer pointer.
      dest += 3;
      
    }
  }


  // Line range.
  float range = 0;
  switch( iViewState.mInPlane ) {
  case 0: range = mVolume->GetVoxelXSize() / 2.0; break;
  case 1: range = mVolume->GetVoxelYSize() / 2.0; break;
  case 2: range = mVolume->GetVoxelZSize() / 2.0; break;
  }

  // Drawing path.
  int lineColor[3];
  if( mCurrentPath ) {
    lineColor[0] = 0; lineColor[1] = 255; lineColor[2] = 0;
    DrawRASPathIntoBuffer( iBuffer, iWidth, iHeight, lineColor, 
			   iViewState, iTranslator, *mCurrentPath );
  }
  list<Path<float>*>::iterator tPath;
  PathManager& pathMgr = PathManager::GetManager();
  list<Path<float>*>& pathList = pathMgr.GetPathList();
  for( tPath = pathList.begin(); tPath != pathList.end(); ++tPath ) {
    Path<float>* path = *tPath;
    if( path->GetNumVertices() > 0 ) {
      Point3<float>& beginRAS = path->GetVertexAtIndex( 0 );
      if( mCurrentPath != path &&
	  iViewState.IsRASVisibleInPlane( beginRAS.xyz(), range ) ) {
	lineColor[0] = 255; lineColor[1] = 0; lineColor[2] = 0;
	DrawRASPathIntoBuffer( iBuffer, iWidth, iHeight, lineColor,
			       iViewState, iTranslator, *path );
      }
    }
  }
}

void
ScubaLayer2DMRI::GetGrayscaleColorForValue ( float iValue,GLubyte* const iBase,
					     int* oColor ) {

  if( (!mbClearZero || (mbClearZero && iValue != 0)) &&
       (iValue >= mMinVisibleValue && iValue <= mMaxVisibleValue) ) {

    int nLUT = (int) floor( (cGrayscaleLUTEntries-1) * 
			    ((iValue - mMinVisibleValue) /
			     (mMaxVisibleValue - mMinVisibleValue)) );

    oColor[0] = mGrayscaleLUT[nLUT];
    oColor[1] = mGrayscaleLUT[nLUT];
    oColor[2] = mGrayscaleLUT[nLUT];

  } else {
    
    oColor[0] = (int)iBase[0]; 
    oColor[1] = (int)iBase[1]; 
    oColor[2] = (int)iBase[2];
  }
}

void
ScubaLayer2DMRI::GetHeatscaleColorForValue ( float iValue,GLubyte* const iBase,
					     int* oColor ) {

  if( (!mbClearZero || (mbClearZero && iValue != 0)) &&
       (iValue >= mMinVisibleValue && iValue <= mMaxVisibleValue) ) {
    
    float midValue;
    midValue = (mMaxVisibleValue - mMinVisibleValue) / 2.0 + mMinVisibleValue;
    
    if( fabs(iValue) >= mMinVisibleValue &&
	fabs(iValue) <= mMaxVisibleValue ) {
      
      float tmp;
      if ( fabs(iValue) > mMinVisibleValue &&
	   fabs(iValue) < midValue ) {
      tmp = fabs(iValue);
      tmp = (1.0/(midValue-mMinVisibleValue)) *
	(tmp-mMinVisibleValue)*(tmp-mMinVisibleValue) + 
	mMinVisibleValue;
      iValue = (iValue<0) ? -tmp : tmp;
      }
      
      /* calc the color */
      float red, green, blue;
      if( iValue >= 0 ) {
	red = ((iValue<mMinVisibleValue) ? 0.0 : 
	       (iValue<midValue) ? 
	       (iValue-mMinVisibleValue)/
	       (midValue-mMinVisibleValue) :
	       1.0);
	green = ((iValue<midValue) ? 0.0 :
		 (iValue<mMaxVisibleValue) ? 
		 (iValue-midValue)/
		 (mMaxVisibleValue-midValue) : 1.0);
	blue = 0.0; 
      } else {
	iValue = -iValue;
	red = 0.0;
	green = ((iValue<midValue) ? 0.0 :
		 (iValue<mMaxVisibleValue) ? 
		 (iValue-midValue)/
		 (mMaxVisibleValue-midValue) : 1.0);
	blue = ((iValue<mMinVisibleValue) ? 0.0 :
		(iValue<midValue) ? 
		(iValue-mMinVisibleValue)/
		(midValue-mMinVisibleValue) : 
		1.0);
      }
      
      if( red > 1.0 )   red = 1.0;
      if( green > 1.0 ) green = 1.0;
      if( blue > 1.0 )  blue = 1.0;
      
      oColor[0] = (int) (red * (float)kMaxPixelComponentValue);
      oColor[1] = (int) (green * (float)kMaxPixelComponentValue);
      oColor[2] = (int) (blue * (float)kMaxPixelComponentValue);
    } else {
      oColor[0] = (int) iBase[0];
      oColor[1] = (int) iBase[1];
      oColor[2] = (int) iBase[2];
    }

  } else {

    oColor[0] = iBase[0]; oColor[1] = iBase[1]; oColor[2] = iBase[2];

  }
}

void
ScubaLayer2DMRI::GetColorLUTColorForValue ( float iValue, GLubyte* const iBase,
					    int* oColor ) {
  
  if( (NULL != mColorLUT) && 
      (!mbClearZero || (mbClearZero && iValue != 0)) &&
      (iValue >= mMinVisibleValue && iValue <= mMaxVisibleValue) ) {

    mColorLUT->GetColorAtIndex( (int)iValue, oColor );

  } else {

    oColor[0] = iBase[0]; oColor[1] = iBase[1]; oColor[2] = iBase[2];
  }
}
  

  
void 
ScubaLayer2DMRI::GetInfoAtRAS ( float iRAS[3], 
				map<string,string>& iLabelValues ) {

  if( NULL == mVolume ) {
    return;
  }

  // Look up the value of the volume at this point.
  if ( mVolume->IsRASInMRIBounds( iRAS ) ) {
    
    float value;
    value = mVolume->GetMRINearestValueAtRAS( iRAS ); 

    // If this is a LUT volume, use the label from the lookup file,
    // otherwise just display the value.
    stringstream ssValue;
    if( mColorMapMethod == LUT && NULL != mColorLUT ) {
      ssValue << mColorLUT->GetLabelAtIndex((int)value);
    } else {
      ssValue << value;
    }

    iLabelValues[mVolume->GetLabel() + " value"] = ssValue.str();

    int index[3];
    mVolume->RASToMRIIndex( iRAS, index );

    stringstream ssIndex;
    ssIndex << index[0] << " " << index[1] << " " << index[2];
    iLabelValues[mVolume->GetLabel() + " index"] = ssIndex.str();
  }
}
  
TclCommandListener::TclCommandResult 
ScubaLayer2DMRI::DoListenToTclCommand ( char* isCommand, int iArgc, char** iasArgv ) {

  // Set2DMRILayerVolumeCollection <layerID> <collectionID>
  if( 0 == strcmp( isCommand, "Set2DMRILayerVolumeCollection" ) ) {
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
	VolumeCollection& volume = (VolumeCollection&)data;
	// VolumeCollection& volume = dynamic_cast<VolumeCollection&>(data);
	
	SetVolumeCollection( volume );
      }
      catch(...) {
	sResult = "bad collection ID, collection not found";
	return error;
      }
    }
  }

  // Set2DMRILayerColorMapMethod <layerID> <method>
  if( 0 == strcmp( isCommand, "Set2DMRILayerColorMapMethod" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      ColorMapMethod method;
      if( 0 == strcmp( iasArgv[2], "grayscale" ) ) {
	method = grayscale;
      } else if( 0 == strcmp( iasArgv[2], "heatScale" ) ) {
	method = heatScale;
      } else if( 0 == strcmp( iasArgv[2], "lut" ) ) {
	method = LUT;
      } else {
	sResult = "bad method \"" + string(iasArgv[2]) +
	  "\", should be grayscale, heatScale or LUT";
	return error;
      }

      SetColorMapMethod( method );
    }
  }

  // Get2DMRILayerColorMapMethod <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerColorMapMethod" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      switch ( mColorMapMethod ) {
      case grayscale:
	sReturnValues = "grayscale";
	break;
      case heatScale:
	sReturnValues = "heatScale";
	break;
      case LUT:
	sReturnValues = "lut";
	break;
      }
      sReturnFormat = "s";
    }
  }

  // Set2DMRILayerSampleMethod <layerID> <sampleMethod>
  if( 0 == strcmp( isCommand, "Set2DMRILayerSampleMethod" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      SampleMethod sampleMethod;
      if( 0 == strcmp( iasArgv[2], "nearest" ) ) {
	sampleMethod = nearest;
      } else if( 0 == strcmp( iasArgv[2], "trilinear" ) ) {
	sampleMethod = trilinear;
      } else if( 0 == strcmp( iasArgv[2], "sinc" ) ) {
	sampleMethod = sinc;
      } else if( 0 == strcmp( iasArgv[2], "magnitude" ) ) {
	sampleMethod = magnitude;
      } else {
	sResult = "bad sampleMethod \"" + string(iasArgv[2]) +
	  "\", should be nearest, trilinear, or sinc";
	return error;
      }
      
      SetSampleMethod( sampleMethod );
    }
  }

  // Get2DMRILayerSampleMethod <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerSampleMethod" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      switch( mSampleMethod ) {
      case nearest:
	sReturnValues = "nearest";
	break;
      case trilinear:
	sReturnValues = "trilinear";
	break;
      case sinc:
	sReturnValues = "sinc";
	break;
      case magnitude:
	sReturnValues = "magnitude";
	break;
      }
      sReturnFormat = "s";
    }
  }

  // Set2DMRILayerBrightness <layerID> <brightness>
  if( 0 == strcmp( isCommand, "Set2DMRILayerBrightness" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      float brightness = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad brightness";
	return error;
      }

      if( brightness > 0 && brightness < 1 ) {
	SetBrightness( brightness );
	BuildGrayscaleLUT();
      }
    }
  }

  // Get2DMRILayerBrightness <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerBrightness" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      stringstream ssReturnValues;
      ssReturnValues << mBrightness;
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "f";
    }
  }

  // Set2DMRILayerContrast <layerID> <contrast>
  if( 0 == strcmp( isCommand, "Set2DMRILayerContrast" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      float contrast = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad contrast";
	return error;
      }

      if( contrast > 0 && contrast < 30 ) {
	SetContrast( contrast );
	BuildGrayscaleLUT();
      }
    }
  }

  // Get2DMRILayerContrast <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerContrast" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      stringstream ssReturnValues;
      ssReturnValues << mContrast;
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "f";
    }
  }

  // Set2DMRILayerColorLUT <layerID> <lutID>
  if( 0 == strcmp( isCommand, "Set2DMRILayerColorLUT" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      int lutID = strtol(iasArgv[2], (char**)NULL, 10);
      if( ERANGE == errno ) {
	sResult = "bad lut ID";
	return error;
      }
    
      SetColorLUT( lutID );
    }
  }

  // Get2DMRILayerColorLUT <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerColorLUT" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      stringstream ssReturnValues;
      if( NULL != mColorLUT ) {
	ssReturnValues << mColorLUT->GetID();
      } else {
	ssReturnValues << -1;
      }
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "i";

    }
  }

  // Set2DMRIDrawZeroClear <layerID> <drawClear>
  if( 0 == strcmp( isCommand, "Set2DMRILayerDrawZeroClear" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      if( 0 == strcmp( iasArgv[2], "true" ) || 
	  0 == strcmp( iasArgv[2], "1" )) {
	mbClearZero = true;
      } else if( 0 == strcmp( iasArgv[2], "false" ) ||
		 0 == strcmp( iasArgv[2], "0" ) ) {
	mbClearZero = false;
      } else {
	sResult = "bad drawClear \"" + string(iasArgv[2]) +
	  "\", should be true, 1, false, or 0";
	return error;	
      }
    }
  }

  // Get2DMRIDrawZeroClear <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerDrawZeroClear" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      stringstream ssReturnValues;
      ssReturnValues << (int)mbClearZero;
      sReturnValues = ssReturnValues.str();
      sReturnFormat = "i";
    }
  }

  // Get2DMRILayerMinVisibleValue <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerMinVisibleValue" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "f";
      stringstream ssReturnValues;
      ssReturnValues << GetMinVisibleValue();
      sReturnValues = ssReturnValues.str();
    }
  }

  // Set2DMRILayerMinVisibleValue <layerID> <value>
  if( 0 == strcmp( isCommand, "Set2DMRILayerMinVisibleValue" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      float value = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad value";
	return error;
      }
      
      SetMinVisibleValue( value );
      BuildGrayscaleLUT();
    }
  }

  // Get2DMRILayerMaxVisibleValue <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerMaxVisibleValue" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "f";
      stringstream ssReturnValues;
      ssReturnValues << GetMaxVisibleValue();
      sReturnValues = ssReturnValues.str();
    }
  }

  // Set2DMRILayerMaxVisibleValue <layerID> <value>
  if( 0 == strcmp( isCommand, "Set2DMRILayerMaxVisibleValue" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      float value = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad value";
	return error;
      }
      
      SetMaxVisibleValue( value );
      BuildGrayscaleLUT();
    }
  }
  
  // Get2DMRILayerMinValue <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerMinValue" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "f";
      stringstream ssReturnValues;
      ssReturnValues << mVolume->GetMRIMinValue();
      sReturnValues = ssReturnValues.str();
    }
  }

  // Get2DMRILayerMaxValue <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerMaxValue" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "f";
      stringstream ssReturnValues;
      ssReturnValues << mVolume->GetMRIMaxValue();
      sReturnValues = ssReturnValues.str();
    }
  }

  // Get2DMRILayerROIOpacity <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerROIOpacity" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      sReturnFormat = "f";
      stringstream ssReturnValues;
      ssReturnValues << GetROIOpacity();
      sReturnValues = ssReturnValues.str();
    }
  }

  // Set2DMRILayerROIOpacity <layerID> <opacity>
  if( 0 == strcmp( isCommand, "Set2DMRILayerROIOpacity" ) ) {
    int layerID = strtol(iasArgv[1], (char**)NULL, 10);
    if( ERANGE == errno ) {
      sResult = "bad layer ID";
      return error;
    }
    
    if( mID == layerID ) {
      
      float opacity = (float) strtod( iasArgv[2], (char**)NULL );
      if( ERANGE == errno ) {
	sResult = "bad opacity";
	return error;
      }
      
      SetROIOpacity( opacity );
    }
  }
  
  // Set2DMRILayerEditableROI <layerID> <editable>
  if( 0 == strcmp( isCommand, "Set2DMRILayerEditableROI" ) ) {
    int layerID;
    try {
      layerID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
    }
    catch( runtime_error e ) {
      sResult = string("bad layerID: ") + e.what();
      return error;
    }
    
    if( mID == layerID ) {
      
      try {
	mbEditableROI =
	  TclCommandManager::ConvertArgumentToBoolean( iasArgv[2] );
      }
      catch( runtime_error e ) {
	sResult = "bad editable \"" + string(iasArgv[2]) + "\"," + e.what();
	return error;	
      }
    }
  }

  // Get2DMRILayerEditableROI <layerID>
  if( 0 == strcmp( isCommand, "Get2DMRILayerEditableROI" ) ) {
    int layerID;
    try {
      layerID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
    }
    catch( runtime_error e ) {
      sResult = string("bad layerID: ") + e.what();
      return error;
    }
    
    if( mID == layerID ) {

      sReturnValues =
	TclCommandManager::ConvertBooleanToReturnValue( mbEditableROI );
      sReturnFormat = "i";
    }
  }

  return Layer::DoListenToTclCommand( isCommand, iArgc, iasArgv );
}

void
ScubaLayer2DMRI::HandleTool ( float iRAS[3], ViewState& iViewState,
			      ScubaWindowToRASTranslator& iTranslator,
			      ScubaToolState& iTool, InputState& iInput ) {
  
  // Never handle control clicks, since that's a navigation thing
  // handled by the view.

  // Only do this if we're the target layer.
  if( iTool.GetLayerTarget() != GetID() )
    return;

  switch( iTool.GetMode() ) {
  case ScubaToolState::voxelEditing:
  case ScubaToolState::roiEditing:

    // If roiEditing, check editable ROI flag.
    if( ScubaToolState::roiEditing == iTool.GetMode() &&
	!mbEditableROI ) 
      return;

    // If shift key is down, we're filling. Make a flood params object
    // and fill it out, then make a flood select object, specifying
    // select or unselect in the ctor. Then run the flood object with
    // the params.
    if( iInput.IsShiftKeyDown() && iInput.IsButtonDownEvent() &&
	(2 == iInput.Button() || 3 == iInput.Button()) ) {

      if( mVolume->IsRASInMRIBounds(iRAS) ) {

	VolumeCollectionFlooder::Params params;
	params.mbStopAtPaths = iTool.GetFloodStopAtPaths();
	params.mbStopAtROIs  = iTool.GetFloodStopAtROIs();
	params.mb3D          = iTool.GetFlood3D();
	params.mFuzziness    = iTool.GetFloodFuzziness();
	params.mMaxDistance  = iTool.GetFloodMaxDistance();
	if( !iTool.GetFlood3D() ) {
	  params.mbWorkPlaneX = (iViewState.mInPlane == 0);
	  params.mbWorkPlaneY = (iViewState.mInPlane == 1);
	  params.mbWorkPlaneZ = (iViewState.mInPlane == 2);
	}
	
	// Create and run the flood object.
	VolumeCollectionFlooder* flooder = NULL;
	if( ScubaToolState::voxelEditing == iTool.GetMode() ) {
	  if( iInput.Button() == 2 ) {
	    flooder = new ScubaLayer2DMRIFloodVoxelEdit( iTool.GetNewValue() );
	  } else if( iInput.Button() == 3 ) {
	    flooder = new ScubaLayer2DMRIFloodVoxelEdit( 0 );
	  }
	} else if( ScubaToolState::roiEditing == iTool.GetMode() ) {
	  if( iInput.Button() == 2 ) {
	    flooder = new ScubaLayer2DMRIFloodSelect( true );
	  } else if( iInput.Button() == 3 ) {
	    flooder = new ScubaLayer2DMRIFloodSelect( false );
	  }
	}
	flooder->Flood( *mVolume, iRAS, params );
	delete flooder;

	RequestRedisplay();

      }      
    }

    if( (iInput.IsButtonDownEvent() || iInput.IsButtonUpEvent() ||
	 iInput.IsButtonDragEvent()) &&
	!iInput.IsShiftKeyDown() &&
	(2 == iInput.Button() || 3 == iInput.Button()) ) {

      // Otherwise we're just brushing. If this is a mouse down event,
      // open up an undo action, and if it's a mouse up event, close
      // it. Then if the mouse is down switch on the brush shape and
      // call a GetRASPointsIn{Shape} function to get the points we
      // need to brush.

      UndoManager& undoList = UndoManager::GetManager();
      if( iInput.IsButtonDownEvent() ) {
	if( ScubaToolState::voxelEditing == iTool.GetMode() ) {
	  if( iInput.Button() == 2 ) {
	    undoList.BeginAction( "Edit Voxel" );
	  } else if( iInput.Button() == 3 ) {
	    undoList.BeginAction( "Erase Voxel" );
	  }
	} else if( ScubaToolState::roiEditing == iTool.GetMode() ) {
	  if( iInput.Button() == 2 ) {
	    undoList.BeginAction( "Selection Brush" );
	  } else if( iInput.Button() == 3 ) {
	    undoList.BeginAction( "Unselection Brush" );
	  }
	}
      }

      if( iInput.IsButtonUpEvent() ) {
	undoList.EndAction();
      }

      if( iInput.IsButtonDown() ) {

	bool bBrushX, bBrushY, bBrushZ;
	bBrushX = bBrushY = bBrushZ = true;
	if( !iTool.GetBrush3D() ) {
	  bBrushX = !(iViewState.mInPlane == 0);
	  bBrushY = !(iViewState.mInPlane == 1);
	  bBrushZ = !(iViewState.mInPlane == 2);
	}
	list<Point3<float> > points;
	ScubaToolState::Shape shape = iTool.GetBrushShape();
	switch( shape ) {
	case ScubaToolState::square:
	  mVolume->GetRASPointsInCube( iRAS, iTool.GetBrushRadius(), 
				       bBrushX, bBrushY, bBrushZ, points );
	  break;
	case ScubaToolState::circle:
	  mVolume->GetRASPointsInSphere( iRAS, iTool.GetBrushRadius(), 
					 bBrushX, bBrushY, bBrushZ, points );
	  break;
	}
	
	list<Point3<float> >::iterator tPoints;
	for( tPoints = points.begin(); tPoints != points.end(); ++tPoints ) {
	  Point3<float> point = *tPoints;
	  if( mVolume->IsRASInMRIBounds(point.xyz()) ) {
	    
	    UndoAction* action = NULL;
	    if( ScubaToolState::voxelEditing == iTool.GetMode() ) {
	      float origValue = mVolume->GetMRINearestValueAtRAS( point.xyz());
	      if( iInput.Button() == 2 ) {
		mVolume->SetMRIValueAtRAS( point.xyz(), iTool.GetNewValue() );
		action = 
		  new UndoVoxelEditAction(mVolume, iTool.GetNewValue(),
					  origValue,point.xyz());
	      } else if( iInput.Button() == 3 ) {
		mVolume->SetMRIValueAtRAS( point.xyz(), 0 );
		action = 
		  new UndoVoxelEditAction(mVolume, 0, origValue,point.xyz());
	      }
	    } else if( ScubaToolState::roiEditing == iTool.GetMode() ) {
	      if( iInput.Button() == 2 ) {
		mVolume->SelectRAS( point.xyz() );
		action = new UndoSelectionAction( mVolume, true, point.xyz() );
	      } else if( iInput.Button() == 3 ) {
		mVolume->UnselectRAS( point.xyz() );
		action =new UndoSelectionAction( mVolume, false, point.xyz() );
	      }
	    }

	    undoList.AddAction( action );
	  }
	}
      
	RequestRedisplay();
      }
    }

    if( ScubaToolState::voxelEditing == iTool.GetMode() ) {
      
      // Adjust the min/max visible value.
      if( iTool.GetMode() == ScubaToolState::voxelEditing ) {
	if( iTool.GetNewValue() < mMinVisibleValue )
	  mMinVisibleValue = iTool.GetNewValue();
	if( iTool.GetNewValue() > mMaxVisibleValue )
	  mMaxVisibleValue = iTool.GetNewValue();
      }
    }

    break;

  case ScubaToolState::straightPath:
  case ScubaToolState::edgePath: {
    
    PathManager& pathMgr = PathManager::GetManager();

    if( iInput.IsButtonDownEvent() ) {

      Point3<float> ras( iRAS );
	  
      /* Button down, no current path */
      if( NULL == mCurrentPath ) {

	switch( iInput.Button() ) {
	case 1:
	  mFirstPathRAS.Set( iRAS );
	  mCurrentPath = pathMgr.NewPath();
	  mCurrentPath->AddVertex( ras );
	  mCurrentPath->MarkEndOfSegment();
	  break;
	case 2:
	case 3:
	  mCurrentPath = FindClosestPathInPlane( iRAS, iViewState );
	  mLastPathMoveRAS.Set( iRAS );
	  break;
	}

	/* Button down, current path */
      } else {

	switch( iInput.Button() ) {
	case 1:
	  mCurrentPath->AddVertex( ras );
	  mCurrentPath->MarkEndOfSegment();
	  break;
	case 2:
	  mCurrentPath->AddVertex( ras );
	  mCurrentPath = NULL;
	  break;
	case 3:
	  mCurrentPath->AddVertex( ras );

	  if( iTool.GetMode() == ScubaToolState::straightPath ) {
	    StretchPathStraight( *mCurrentPath, ras.xyz(),mFirstPathRAS.xyz());
	  } else if( iTool.GetMode() == ScubaToolState::edgePath ) {
	    StretchPathAsEdge( *mCurrentPath, ras.xyz(), 
			       mFirstPathRAS.xyz(), iViewState, iTranslator,
			       iTool.GetEdgePathStraightBias(),
			       iTool.GetEdgePathEdgeBias() );
	  }

	  mCurrentPath = NULL;
	  break;
	}
      }
      
      RequestRedisplay();

    } else if ( iInput.IsButtonDragEvent() ) {

      /* Drag event, current line */
      if( 2 == iInput.Button() ) {
	
	if( NULL != mCurrentPath ) {
	  Point3<float> deltaRAS( iRAS[0] - mLastPathMoveRAS.x(),
				  iRAS[1] - mLastPathMoveRAS.y(),
				  iRAS[2] - mLastPathMoveRAS.z() );
	  mCurrentPath->Move( deltaRAS );
	  mLastPathMoveRAS.Set( iRAS );

	  RequestRedisplay();
	}
      }

    } else if ( iInput.IsButtonUpEvent() ) {

      /* Button up, current path */
      if( NULL != mCurrentPath ) {
	
	if( 3 == iInput.Button() ) {
	  Path<float>* delPath = FindClosestPathInPlane( iRAS, iViewState );
	  if( delPath == mCurrentPath ) {
	    pathMgr.DeletePath( delPath );
	  }
	}
	
	if( 3 == iInput.Button() ||
	    2 == iInput.Button() ) {
	  
	  mCurrentPath = NULL;
	}

	RequestRedisplay();
      }
      
    } else {

      /* No mouse event, current path */
      if( NULL != mCurrentPath ) {
	
	pathMgr.DisableUpdates();

	Point3<float>& end = mCurrentPath->GetPointAtEndOfLastSegment();
	if( iTool.GetMode() == ScubaToolState::straightPath ) {
	  StretchPathStraight( *mCurrentPath, end.xyz(), iRAS );
	} else if( iTool.GetMode() == ScubaToolState::edgePath ) {
	  StretchPathAsEdge( *mCurrentPath, end.xyz(), 
			     iRAS, iViewState, iTranslator,
			     iTool.GetEdgePathStraightBias(),
			     iTool.GetEdgePathEdgeBias() );
	}
	
	pathMgr.EnableUpdates();

	RequestRedisplay();
      }
    }
    
    
  } break;
  default:
    break;
  }
}

void
ScubaLayer2DMRI::SetColorLUT ( int iLUTID ) {

  try {
    mColorLUT = &(ScubaColorLUT::FindByID( iLUTID ));
  }
  catch(...) {
    DebugOutput( << "Couldn't find color LUT " << iLUTID );
  }
  
}


void
ScubaLayer2DMRI::BuildGrayscaleLUT () {

  for( float nEntry = 0; nEntry < cGrayscaleLUTEntries; nEntry+=1 ) {

    // Get the value using the actual min/max to get highest
    // granularity within the 0 - cGrayscaleLUTEntries range.
    float value = ((nEntry * (mMaxVisibleValue-mMinVisibleValue)) / 
		   cGrayscaleLUTEntries) + mMinVisibleValue;

    // Use sigmoid to apply brightness/contrast. Gets 0-1 value.
    float bcdValue;
    if( value != 0 ) {
      bcdValue = (1.0 / (1.0 + exp( (((value-mMinVisibleValue)/(mMaxVisibleValue-mMinVisibleValue))-mBrightness) * -mContrast)));
    } else {
      bcdValue = 0;
    }

    // Normalize back to pixel component value.
    float normValue = bcdValue * kMaxPixelComponentValueFloat;

    // Assign in table.
    mGrayscaleLUT[(int)nEntry] = (int) floor( normValue );
  }
}

// PATHS =================================================================

void 
ScubaLayer2DMRI::StretchPathStraight ( Path<float>& iPath,
				       float iRASBegin[3],
				       float iRASEnd[3] ) {
  
  // Just clear the path and add the begin and end point.
  iPath.ClearLastSegment ();
  Point3<float> begin( iRASBegin );
  Point3<float> end( iRASEnd );
  iPath.AddVertex( begin );
  iPath.AddVertex( end );
}


void 
ScubaLayer2DMRI::StretchPathAsEdge ( Path<float>& iPath,
				     float iRASBegin[3],
				     float iRASEnd[3],
				     ViewState& iViewState,
				     ScubaWindowToRASTranslator& iTranslator,
				     float iStraightBias, float iEdgeBias ){

  // Make an edge path finder.
  EdgePathFinder finder( iViewState.mBufferWidth, iViewState.mBufferHeight,
			 (int)mVolume->GetMRIMagnitudeMaxValue(),
			 &iTranslator, mVolume );
  finder.DisableOutput();
  finder.SetStraightBias( iStraightBias );
  finder.SetEdgeBias( iEdgeBias );

  // Get the first point from the path and the last point as passed
  // in. Convert to window points. Then find the path between them.
  Point3<float> beginRAS( iRASBegin );
  Point3<float> endRAS( iRASEnd );

  Point2<int> beginWindow;
  Point2<int> endWindow;
  iTranslator.TranslateRASToWindow( beginRAS.xyz(), beginWindow.xy() );
  iTranslator.TranslateRASToWindow( endRAS.xyz(), endWindow.xy() );
  list<Point2<int> > windowPoints;
  
  finder.FindPath( beginWindow, endWindow, windowPoints );

  // Clear the path points and add the points we got from the path,
  // converting them to RAS one the way.
  iPath.ClearLastSegment();
  list<Point2<int> >::iterator tWindowPoint;
  for( tWindowPoint = windowPoints.begin();
       tWindowPoint != windowPoints.end();
       ++tWindowPoint ) {
    Point3<float> currentRAS;
    iTranslator.TranslateWindowToRAS( (*tWindowPoint).xy(), currentRAS.xyz() );
    iPath.AddVertex( currentRAS );
  }
}


Path<float>*
ScubaLayer2DMRI::FindClosestPathInPlane ( float iRAS[3],
					  ViewState& iViewState ) {

  float minDistance = mWidth * mHeight;
  Path<float>* closestPath = NULL;
  Point3<float> whereRAS( iRAS );

  float range = 0;
  switch( iViewState.mInPlane ) {
  case 0: range = mVolume->GetVoxelXSize() / 2.0; break;
  case 1: range = mVolume->GetVoxelYSize() / 2.0; break;
  case 2: range = mVolume->GetVoxelZSize() / 2.0; break;
  }

  PathManager& pathMgr = PathManager::GetManager();
  list<Path<float>*>::iterator tPath;
  list<Path<float>*>& paths = pathMgr.GetPathList();  
  for( tPath = paths.begin(); tPath != paths.end(); ++tPath ) {
    Path<float>* path = *tPath;
    Point3<float>& beginRAS = path->GetVertexAtIndex( 0 );
    if( iViewState.IsRASVisibleInPlane( beginRAS.xyz(), range ) ) {
      
      float minDistanceInPath = 999999;
      
      int cVertices = path->GetNumVertices();
      int nCurVertex = 1;
      for( nCurVertex = 1; nCurVertex < cVertices; nCurVertex++ ) {
	
	int nBackVertex = nCurVertex - 1;
	
	Point3<float>& curVertex  = path->GetVertexAtIndex( nCurVertex );
	Point3<float>& backVertex = path->GetVertexAtIndex( nBackVertex );
	
	float distance = 
	  Utilities::DistanceFromLineToPoint3f( curVertex, backVertex,
						whereRAS );
	
	if( distance < minDistanceInPath )
	  minDistanceInPath = distance;
      }
      
      if( minDistanceInPath < minDistance ) {
	minDistance = minDistanceInPath;
	closestPath = path;
      }
    }
  }

  return closestPath;
}

void
ScubaLayer2DMRI::DrawRASPathIntoBuffer ( GLubyte* iBuffer, 
					 int iWidth, int iHeight,
					 int iColor[3],
					 ViewState& iViewState,
				     ScubaWindowToRASTranslator& iTranslator,
					 Path<float>& iPath ) {


  // For every two RAS vertices on our path, translate them to window
  // points, and draw the path.
  int cVertices = iPath.GetNumVertices();
  for( int nCurVertex = 1; nCurVertex < cVertices; nCurVertex++ ) {
    
    int nBackVertex = nCurVertex - 1;
    
    Point3<float>& curVertex  = iPath.GetVertexAtIndex( nCurVertex );
    Point3<float>& backVertex = iPath.GetVertexAtIndex( nBackVertex );
    
    int curWindow[2], backWindow[2];
    iTranslator.TranslateRASToWindow( curVertex.xyz(), curWindow );
    iTranslator.TranslateRASToWindow( backVertex.xyz(), backWindow );
    
    DrawLineIntoBuffer( iBuffer, iWidth, iHeight, backWindow, curWindow,
			iColor, 1, 1 );
  }
}


void
ScubaLayer2DMRI::GetPreferredInPlaneIncrements ( float oIncrements[3] ) {
  
  oIncrements[0] = mVolume->GetVoxelXSize();
  oIncrements[1] = mVolume->GetVoxelYSize();
  oIncrements[2] = mVolume->GetVoxelZSize();
}


// ======================================================================

ScubaLayer2DMRIFloodVoxelEdit::ScubaLayer2DMRIFloodVoxelEdit ( float iValue ) {
  mValue = iValue;
}


void
ScubaLayer2DMRIFloodVoxelEdit::DoBegin () {
      
  // Create a task in the progress display manager.
  TclProgressDisplayManager& manager =
    TclProgressDisplayManager::GetManager();
  
  list<string> lButtons;
  lButtons.push_back( "Stop" );
  
  manager.NewTask( "Filling", "Filling voxels", false, lButtons );

  // Start our undo action.
  UndoManager& undoList = UndoManager::GetManager();

  undoList.BeginAction( "Voxel Fill" );
}

void 
ScubaLayer2DMRIFloodVoxelEdit::DoEnd () {

  // End the task.
  TclProgressDisplayManager& manager =
    TclProgressDisplayManager::GetManager();
  manager.EndTask();

  // End our undo action.
  UndoManager& undoList = UndoManager::GetManager();
  undoList.EndAction();
}

bool
ScubaLayer2DMRIFloodVoxelEdit::DoStopRequested () {

  // Check for the stop button.
  TclProgressDisplayManager& manager = 
    TclProgressDisplayManager::GetManager();
  int nButton = manager.CheckTaskForButton();
  if( nButton == 0 ) {
    return true;
  } 

  return false;
}

bool
ScubaLayer2DMRIFloodVoxelEdit::CompareVoxel ( float iRAS[3] ) {

  // Always return true.
  return true;
}

void
ScubaLayer2DMRIFloodVoxelEdit::DoVoxel ( float iRAS[3] ) {
  UndoManager& undoList = UndoManager::GetManager();

  float origValue = mVolume->GetMRINearestValueAtRAS( iRAS );

  mVolume->SetMRIValueAtRAS( iRAS, mValue );

  UndoVoxelEditAction* action = 
    new UndoVoxelEditAction( mVolume, mValue, origValue, iRAS );

  undoList.AddAction( action );
}

UndoVoxelEditAction::UndoVoxelEditAction ( VolumeCollection* iVolume,
					   float iNewValue, float iOrigValue, 
					   float iRAS[3] ) {
  mVolume = iVolume;
  mNewValue = iNewValue;
  mOrigValue = iOrigValue;
  mRAS[0] = iRAS[0];
  mRAS[1] = iRAS[1];
  mRAS[2] = iRAS[2];
}

void
UndoVoxelEditAction::Undo () {

  mVolume->SetMRIValueAtRAS( mRAS, mOrigValue );
}

void
UndoVoxelEditAction::Redo () {

  mVolume->SetMRIValueAtRAS( mRAS, mNewValue );
}

// ============================================================

ScubaLayer2DMRIFloodSelect::ScubaLayer2DMRIFloodSelect ( bool ibSelect ) {
  mbSelect = ibSelect;
}


void
ScubaLayer2DMRIFloodSelect::DoBegin () {
      
  // Create a task in the progress display manager.
  TclProgressDisplayManager& manager =
    TclProgressDisplayManager::GetManager();
  
  list<string> lButtons;
  lButtons.push_back( "Stop" );
  
  if( mbSelect ) {
    manager.NewTask( "Selecting", "Selecting voxels", false, lButtons );
  } else {
    manager.NewTask( "Unselecting", "Unselecting voxels", false, lButtons );
  }

  // Start our undo action.
  UndoManager& undoList = UndoManager::GetManager();

  if( mbSelect ) {
    undoList.BeginAction( "Selection Fill" );
  } else {
    undoList.BeginAction( "Unselection Fill" );
  }

}

void 
ScubaLayer2DMRIFloodSelect::DoEnd () {

  // End the task.
  TclProgressDisplayManager& manager =
    TclProgressDisplayManager::GetManager();
  manager.EndTask();

  // End our undo action.
  UndoManager& undoList = UndoManager::GetManager();
  undoList.EndAction();
}

bool
ScubaLayer2DMRIFloodSelect::DoStopRequested () {

  // Check for the stop button.
  TclProgressDisplayManager& manager = 
    TclProgressDisplayManager::GetManager();
  int nButton = manager.CheckTaskForButton();
  if( nButton == 0 ) {
    return true;
  } 

  return false;
}

bool
ScubaLayer2DMRIFloodSelect::CompareVoxel ( float iRAS[3] ) {

  // Always return true.
  return true;
}

void
ScubaLayer2DMRIFloodSelect::DoVoxel ( float iRAS[3] ) {
  UndoManager& undoList = UndoManager::GetManager();

  if( mbSelect ) {
    mVolume->SelectRAS( iRAS );
    UndoSelectionAction* action = 
      new UndoSelectionAction( mVolume, true, iRAS );
    undoList.AddAction( action );
  } else {
    mVolume->UnselectRAS( iRAS );
    UndoSelectionAction* action =
      new UndoSelectionAction( mVolume, false, iRAS );
    undoList.AddAction( action );
  }
}

UndoSelectionAction::UndoSelectionAction ( VolumeCollection* iVolume,
					   bool ibSelect, float iRAS[3] ) {
  mVolume = iVolume;
  mbSelect = ibSelect;
  mRAS[0] = iRAS[0];
  mRAS[1] = iRAS[1];
  mRAS[2] = iRAS[2];
}

void
UndoSelectionAction::Undo () {
  if( mbSelect ) {
    mVolume->UnselectRAS( mRAS );
  } else {
    mVolume->SelectRAS( mRAS );
  }
}

void
UndoSelectionAction::Redo () {
  if( mbSelect ) {
    mVolume->SelectRAS( mRAS );
  } else {
    mVolume->UnselectRAS( mRAS );
  }
}

// ============================================================

EdgePathFinder::EdgePathFinder ( int iViewWidth, int iViewHeight, 
				 int iLongestEdge,
				 ScubaWindowToRASTranslator* iTranslator,
				 VolumeCollection* iVolume ) {

  SetDimensions( iViewWidth, iViewHeight, iLongestEdge );
  mVolume = iVolume;
  mTranslator = iTranslator;
}
		  
float 
EdgePathFinder::GetEdgeCost ( Point2<int>& iPoint ) {

  // Get the magnitude value at this point.
  float RAS[3];
  mTranslator->TranslateWindowToRAS( iPoint.xy(), RAS );
  if( mVolume->IsRASInMRIBounds( RAS ) ) {
    return 1.0 / (mVolume->GetMRIMagnitudeValueAtRAS( RAS ) + 0.0001);
  } else {
    return mLongestEdge;
  }
}
