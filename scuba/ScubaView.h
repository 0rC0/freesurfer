#ifndef ScubaView_h
#define ScubaView_h

#include "string_fixed.h"
#include <GL/gl.h>
#include "View.h"
#include "DataCollection.h"
#include "InputState.h"
#include "ViewState.h"
#include "Layer.h"
#include "ScubaWindowToRASTranslator.h"
#include "ScubaToolState.h"
#include "ScubaTransform.h"
#include "Point2.h"
#include "VolumeCollection.h"

class ScubaViewStaticTclListener : public DebugReporter, 
				   public TclCommandListener {
  
public:
  ScubaViewStaticTclListener ();
  ~ScubaViewStaticTclListener ();

    virtual TclCommandResult
      DoListenToTclCommand ( char* isCommand, int iArgc, char** iArgv );
};


class ScubaView : public View, 
		  public ScubaWindowToRASTranslator {
  
  friend class ScubaViewTester;
  friend class ScubaViewStaticTclListener;
  
public:
  
  ScubaView();
  virtual ~ScubaView();

  static int const kBytesPerPixel;         // for buffer size
  static int const kcInPlaneMarkerColors;  // number of preset colors

  // Sets the view. Used by something that wants to explicitly set up
  // the view area, such as a linked view broadcasting its position or
  // a script.
  void Set2DRASCenter ( float iRASCenter[3] );
  void Set2DZoomLevel ( float iZoom );
  void Set2DInPlane ( ViewState::Plane iPlane );
  void Set2DPlaneNormal ( float iNormal[3] );
  void Set2DPlaneNormalOrthogonalToInPlane ();

  void Get2DRASCenter ( float oRASCenter[3] );
  float Get2DZoomLevel ();
  ViewState::Plane Get2DInPlane ();
  std::string Get2DInPlaneAsString ();
  void Get2DPlaneNormal ( float oNormal[3] );

  // Add and remove layers that this view at a specific level. Note
  // that only one layer can be at a specific level, and adding a
  // layer at a level that is already occupied will replace the
  // existing layer.
  void SetLayerAtLevel ( int iLayerID, int iLevel );
  int GetLayerAtLevel ( int iLevel );
  void RemoveAllLayers ();
  void RemoveLayerAtLevel ( int iLevel );

  // Get the first draw level with no layer assigned to it.
  int GetFirstUnusedDrawLevel ();

  // If a level is not visible, it won't be drawn.
  void SetDrawLevelVisibility ( int inLevel, bool ibVisible );
  bool GetDrawLevelVisibility ( int inLevel );

  // Sets the same layers in another view.
  void CopyLayerSettingsToView ( ScubaView& iView );

  // Set the display transform for this view.
  void SetWorldToViewTransform ( int iTransformID );
  int GetWorldToViewTransform ();

  void SetInPlaneIncrement ( ViewState::Plane iInPlane, float iIncrement );
  float GetInPlaneIncrement ( ViewState::Plane iInPlane );

  void SetLinkedStatus ( bool ibLinked ) {mViewIDLinkedList[GetID()]=ibLinked;}
  bool GetLinkedStatus () { return mViewIDLinkedList[GetID()]; }

  void SetLockOnCursor ( bool ibLock ) { mbLockOnCursor = ibLock; }
  bool GetLockOnCursor () { return mbLockOnCursor; }

  // Get view state.
  ViewState& GetViewState () { return mViewState; }

  // Get the map of label values.
  std::map<std::string,std::string>& GetLabelValueMap ( std::string isSet );

  // Handle Tcl commands.
  virtual TclCommandResult
    DoListenToTclCommand ( char* isCommand, int iArgc, char** iasArgv );

  // Handle broadcast messages.
  virtual void
    DoListenToMessage ( std::string isMessage, void* iData );

  // Implement ScubaWindowToRASTranslator.
  void TranslateWindowToRAS ( int const iWindow[2], float oRAS[3] );
  void TranslateRASToWindow ( float const iRAS[3], int oWindow[2] );

  // Set the flag to rebuild the draw overlay.
  void RebuildOverlayDrawList () { mbRebuildOverlayDrawList = true; }

  // Access the left/right flip flag.
  void SetFlipLeftRightYZ ( bool iFlip );
  bool GetFlipLeftRightYZ () { return mbFlipLeftRightInYZ; }

  // Get the inplane marker color.
  void GetInPlaneMarkerColor ( float oColor[3] );

  // Sets a marker in the view, wrapping around the number of markers.
  static void SetNextMarker ( float iRAS[3] );
  static void HideNearestMarker ( float iRAS[3] );
  
  // Sets the number of markers to use, as well as initializes new
  // markers.
  static void SetNumberOfMarkers ( int icMarkers );
  static int GetNumberOfMarkers () { return mcMarkers; }

  static bool IsNthMarkerVisible ( int inMarker );
  static void GetNthMarker ( int inMarker, float oMarkerRAS[3] );

  // Export markers to control points for a volume.
  static void ExportMarkersToControlPointsForVolume 
    ( std::string ifnControlPoints,
      VolumeCollection& iVolume );
  static void ImportMarkersFromControlPointsForVolume
    ( std::string ifnControlPoints,
      VolumeCollection& iVolume );


  // Gets a histogram of values in the current view from the given
  // volume. Generates a list of RAS points in the current view and
  // passes that to the volume to generate a histogram, and returns
  // the data.
  void GetVolumeHistogramInView ( VolumeCollection& iSourceVol,
				  ScubaROIVolume* iROI,
				  int icBins,
				  float& oMinBinValue, float& oBinIncrement,
				  std::map<int,int>& oBinCounts );

  void BeginValueRangeFill ( VolumeCollection& iSourceVol,
			     ScubaROIVolume* iROI,
			     VolumeCollection& iDestVol );
  void DoOneValueRangeFill ( float iBeginValueRange,
			     float iEndValueRange,
			     float iFillValue );
  void EndValueRangeFill   ();

protected:

  // Tells all the layers to draw in the correct order to the frame
  // buffer and then writes the frame buffer to the GL context.
  virtual void DoDraw ();

  // Resizes the frame buffer.
  virtual void DoReshape ( int iWidth, int iHeight );

  // Passes to layers.
  virtual void DoTimer ();
  
  // On mouse moves, this calls GetInfoAtRAS on all Layers and writes
  // the info as strings on the window.
  virtual void DoMouseMoved ( int iWindow[2], 
			      InputState& iInput, ScubaToolState& iTool );

  // Mouse up sets a marker at the current location. Mouse down and
  // mouse up may trigger tool effects.
  virtual void DoMouseUp ( int iWindow[2], 
			   InputState& iInput, ScubaToolState& iTool );
  virtual void DoMouseDown ( int iWindow[2], 
			     InputState& iInput, ScubaToolState& iTool );

  // Key up and down may trigger commands.
  virtual void DoKeyDown ( int iWindow[2], 
			   InputState& iInput, ScubaToolState& iTool );
  virtual void DoKeyUp ( int iWindow[2], 
			 InputState& Translates, ScubaToolState& iTool );

  // iInput window coords to RAS coordinates based on the current
  // view port.
  float ConvertWindowToRAS ( float iWindow, float iRASCenter, 
			     float iWindowDimension );
  float ConvertRASToWindow ( float iRAS, float iRASCenter, 
			     float iWindowDimension );

  // Translates RAS (view) coordinates by a vector in window space.
  void TranslateRASInWindowSpace ( float iRAS[3], float iMove[3],
				   float oRAS[3] );
				    
  // Markers are shared between views so these are static functions.

  // Sets the cursor, a single special marker.
  static void SetCursor ( float iRAS[3] );

  // The different steps in building our display. BuildFrameBuffer()
  // tells all the layers to copy their data to the frame
  // buffer. DrawFrameBuffer() copies it to the screen. BuildOverlay()
  // makes the draw lists for the gl command overlay. DrawOverlay()
  // executes the draw list.
  void BuildFrameBuffer ();
  void DrawFrameBuffer ();
  void BuildOverlay();
  void DrawOverlay ();

  // Rebuilds the label-value stuff.
  void RebuildLabelValueInfo ( float iRAS[3], std::string isLabel );

  // The draw list for the view overlay and a boolean saying whether
  // it should be rebuilt, usually when the view changes. This view
  // will actually use list kOverlayDrawListID + mID.
  #define kOverlayDrawListID 1
  bool mbRebuildOverlayDrawList;

  std::map<std::string,std::map<std::string,std::string> > mLabelValueMaps;

  // List of layers and their levels (level, layerID).
  std::map<int,int> mLevelLayerIDMap;
  std::map<int,bool> mLevelVisibilityMap;
  
  // Current view information for this view.
  ViewState mViewState;

  // For keeping track of mouse movement navigation;
  float mMouseMoveDelta[2];
  int mLastMouseMoved[2];
  int mLastMouseDown[2];
  float mOriginalCenterRAS[3];
  float mOriginalZoom;
  Point3<float> mOriginalPlaneNormal;

  // A map of increments that override the layer preferred ones, for
  // each layer.
  typedef std::map<ViewState::Plane,float> InPlaneIncrements;
  std::map<int,InPlaneIncrements > mLayerIDInPlaneIncrements;

  // The buffer for this view.
  GLubyte* mBuffer;

  // Key assignments.
  std::string msMoveViewLeft;
  std::string msMoveViewRight;
  std::string msMoveViewUp;
  std::string msMoveViewDown;
  std::string msMoveViewIn;
  std::string msMoveViewOut;
  std::string msZoomViewIn;
  std::string msZoomViewOut;

  // Link stuff.
  static std::map<int,bool> mViewIDLinkedList;

  // The world to view transform, or the 'view transform.' Actually
  // goes from world -> view. Used in conjunction with mWorldToWindow
  // get get window coords. This is user settable.
  ScubaTransform* mWorldToView;

  // This is the view -> window or display plane transform. It is
  // based on the mViewState current inplane and plane normal. It is
  // updated any time the view plane changes.
  Transform44 mViewToWindow;

  void CalcViewToWindowTransform ();

  // This is our actual world -> window transform composed of
  // mWorldToView and mViewToWindow. It is updated any time these two
  // are changed.
  Transform44 mWorldToWindow;

  void CalcWorldToWindowTransform ();

  // Calculates the RAS points of the intersection with a view's
  // inplane, our inplane, and our viewing box.
  void CalcAllViewIntersectionPoints ();
  void CalcViewIntersectionPoints ( int iViewID );
  //  std::map<int,Point2<Point3<float> > > mViewIDViewIntersectionPointMap;
  Point3<float> mViewIDViewIntersectionPointMap[10][2];

  // For moving view intersection markers.
  int mCurrentMovingViewIntersection;

  // Whether to flip the right/left coordinates.
  bool mbFlipLeftRightInYZ;

  // Whether to stay locked on the cursor.
  bool mbLockOnCursor;

  // The color to use when drawing this view's inplane on another view.
  float mInPlaneMarkerColor[3];

  // Markers.
  static Point3<float> mCursor;
  static int mcMarkers;
  static int mNextMarker;
  static std::map<int,Point3<float> > mMarkerRAS;  // index from 0 - mcMarkers
  static std::map<int,bool> mMarkerVisible;

  static ScubaViewStaticTclListener mStaticListener;

  class ValueRangeFillElement {
  public:
    ValueRangeFillElement ( float iBegin, float iEnd, float iValue ) :
      mBegin(iBegin), mEnd(iEnd), mValue(iValue) {}
    float mBegin, mEnd, mValue;
  };

  class ValueRangeFillParams {
  public:
    ValueRangeFillParams ( VolumeCollection& iSrc, ScubaROIVolume* iROI, VolumeCollection& iDest ) : mSourceVol(iSrc), mROI(iROI), mDestVol(iDest) {} 
    VolumeCollection& mSourceVol;
    ScubaROIVolume* mROI;
    VolumeCollection& mDestVol;
    std::list<ValueRangeFillElement> mFillElements;
  };

  ValueRangeFillParams* mValueRangeFillParams;
};  

class ScubaViewFactory : public ViewFactory {
 public:
  virtual View* NewView();
};

class ScubaViewBroadcaster : public Broadcaster {
 public:
  ScubaViewBroadcaster();
  static ScubaViewBroadcaster& GetBroadcaster ();
  virtual void SendBroadcast ( std::string iMessage, void* iData );
 protected:
  int mCurrentBroadcaster;
};



#endif
