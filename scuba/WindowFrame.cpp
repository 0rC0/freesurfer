#include "string_fixed.h"
#include <stdexcept>
#include "WindowFrame.h"
#include "Point2.h"

using namespace std;

DeclareIDTracker(WindowFrame);

WindowFrame::WindowFrame( ID iID ) {
  mLastMoved[0] = 0;
  mLastMoved[1] = 1;
  mWidth = 0;
  mHeight = 0;
}

WindowFrame::~WindowFrame() {

}

void 
WindowFrame::Draw() {

  this->DoDraw();
}

void 
WindowFrame::Reshape( int iWidth, int iHeight ) {

  mWidth = iWidth;
  mHeight = iHeight;

  this->DoReshape();
}

void 
WindowFrame::Timer() {

  this->DoTimer();
}

void
WindowFrame::MouseMoved( int iWindow[2], InputState& iInput ) {

  if( iWindow[0] > 0 && iWindow[0] < mWidth-1 &&
      iWindow[1] > 0 && iWindow[1] < mHeight-1 ) {

    // Calculate the delta. Make sure there is one; if not, no mouse
    // moved event.
    float delta[2];
    delta[0] = iWindow[0] - mLastMoved[0];
    delta[1] = iWindow[1] - mLastMoved[1];
    if( delta[0] != 0 || delta[1] != 0 ) {

      // Find the greater one (absolute value). Divide each delta by
      // this value to get the step; one will be 1.0, and the other
      // will be < 1.0.
      float greater = fabs(delta[0]) > fabs(delta[1]) ? 
	fabs(delta[0]) : fabs(delta[1]);
      delta[0] /= greater;
      delta[1] /= greater;

      // We step window coords in floats, but transform to ints. Start
      // at the last moved place.
      float windowF[2];
      int   windowI[2];
      windowF[0] = mLastMoved[0];
      windowF[1] = mLastMoved[1];

      // While we're not at the current location...
      while( !(fabs((float)iWindow[0] - windowF[0]) < 0.01 && 
	       fabs((float)iWindow[1] - windowF[1]) < 0.01) ) {

	// Get an integer value and send it to the frame.
	windowI[0] = (int) windowF[0];
	windowI[1] = (int) windowF[1];
	this->DoMouseMoved( windowI, iInput );
	
	// Increment the float window coords.
	windowF[0] += delta[0];
	windowF[1] += delta[1];
      } 
    } 

    // Save this position.
    mLastMoved[0] = iWindow[0];
    mLastMoved[1] = iWindow[1];
  }
}

void
WindowFrame::MouseUp( int iWindow[2], InputState& iInput ) {

  if( iWindow[0] > 0 && iWindow[0] < mWidth-1 &&
      iWindow[1] > 0 && iWindow[1] < mHeight-1 ) {
    this->DoMouseUp( iWindow, iInput );
  }
}

void
WindowFrame::MouseDown( int iWindow[2], InputState& iInput ) {

  if( iWindow[0] > 0 && iWindow[0] < mWidth-1 &&
      iWindow[1] > 0 && iWindow[1] < mHeight-1 ) {
    this->DoMouseDown( iWindow, iInput );
    mLastMoved[0] = iWindow[0];
    mLastMoved[1] = iWindow[1];
  }
}

void
WindowFrame::KeyDown( int iWindow[2], InputState& iInput ) {

  if( iWindow[0] > 0 && iWindow[0] < mWidth-1 &&
      iWindow[1] > 0 && iWindow[1] < mHeight-1 ) {
    this->DoKeyDown( iWindow, iInput );
  }
}

void
WindowFrame::KeyUp( int iWindow[2], InputState& iInput ) {

  if( iWindow[0] > 0 && iWindow[0] < mWidth-1 &&
      iWindow[1] > 0 && iWindow[1] < mHeight-1 ) {
    this->DoKeyUp( iWindow, iInput );
  }
}

void
WindowFrame::DoDraw() {

  DebugOutput( << "WindowFrame " << mID << ": DoDraw()" );
}

void
WindowFrame::DoReshape() {

  DebugOutput( << "WindowFrame " << mID << ": DoReshape()" );
}

void
WindowFrame::DoTimer() {

  DebugOutput( << "WindowFrame " << mID << ": DoTimer()" );
}

void
WindowFrame::DoMouseMoved( int iWindow[2], InputState& iInput ) {

  cerr << "WindowFrame::DoMouseMoved " << mID << endl;
  DebugOutput( << "WindowFrame " << mID << ": DoMouseMoved()" );
}

void
WindowFrame::DoMouseUp( int iWindow[2], InputState& iInput ) {

  DebugOutput( << "WindowFrame " << mID << ": DoMouseUp()" );
}

void
WindowFrame::DoMouseDown( int iWindow[2], InputState& iInput ) {

  DebugOutput( << "WindowFrame " << mID << ": DoMouseDown()" );
}

void
WindowFrame::DoKeyDown( int iWindow[2], InputState& iInput ) {

  DebugOutput( << "WindowFrame " << mID << ": DoKeyDown()" );
}

void
WindowFrame::DoKeyUp( int iWindow[2], InputState& iInput ) {

  DebugOutput( << "WindowFrame " << mID << ": DoKeyUp()" );
}
