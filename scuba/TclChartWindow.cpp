#include "string_fixed.h"
#include <sstream>
#include "TclChartWindow.h"
#include "TclCommandManager.h"

using namespace std;

TclChartWindowStaticListener TclChartWindow::mStaticListener;
bool TclChartWindow::sbInitedTclFile = false;

TclChartWindow::TclChartWindow () :
  ChartWindow() {
  msTitle = "Chart";

  TclCommandManager& manager = TclCommandManager::GetManager();

  // If we're not already inited...
  if( !sbInitedTclFile ) {
    try {
      // Try using a utility function in scuba.tcl to load the
      // file. If this doesn't work...
      manager.SendCommand( "LoadScubaSupportFile TclChartWindow.tcl" );
    }
    catch(...) {
      // Just use the source command. If this doesn't work, we can't
      // find the TclChartWindow.tcl and we have a genuine error, so
      // let it be thrown.
      manager.SendCommand( "source TclChartWindow.tcl" );
    }
    
    // This inits the chart stuff.
    manager.SendCommand( "Chart_Init" );

    sbInitedTclFile = true;
  }

  // Our chart ID is the same as our IDTracker ID.
  int chartID = GetID();

  // Create a window.
  stringstream ssCommand;
  ssCommand << "Chart_NewWindow " << chartID;
  manager.SendCommand( ssCommand.str() );
  
}

TclChartWindow::~TclChartWindow () {
}

void
TclChartWindow::Draw () {

  TclCommandManager& manager = TclCommandManager::GetManager();

  // Our chart ID is the same as our IDTracker ID.
  int chartID = GetID();

  stringstream ssCommand;

  // Call the Chart_ commands from TclChartWindow.tcl to set up our
  // chart.
  ssCommand.str("");
  ssCommand << "Chart_SetWindowTitle " << chartID
	    << " \"" << msTitle << "\"";
  manager.SendCommand( ssCommand.str() );

  ssCommand.str("");
  ssCommand << "Chart_SetXAxisLabel " << chartID
	    << " \"" << msXLabel << "\"";
  manager.SendCommand( ssCommand.str() );

  ssCommand.str("");
  ssCommand << "Chart_SetYAxisLabel " << chartID
	    << " \"" << msYLabel << "\"";
  manager.SendCommand( ssCommand.str() );
  
  ssCommand.str("");
  ssCommand << "Chart_SetInfo " << chartID
	    << " \"" << msInfo << "\"";
  manager.SendCommand( ssCommand.str() );
  
  ssCommand.str("");
  ssCommand << "Chart_SetShowLegend " << chartID << " "
	    << (mbShowLegend ? "true" : "false");
  manager.SendCommand( ssCommand.str() );

  // Make a tcl command with all our data and send it.
  list<PointData>::iterator tPoint;
  ssCommand.str("");
  ssCommand << "Chart_SetPointData " << chartID << " [list ";
  for( tPoint = mPointData.begin(); tPoint != mPointData.end(); ++tPoint ) {
    PointData& point = *tPoint;
    ssCommand << "[list x " << point.mX << " y " << point.mY
	      << " label \"" << point.msLabel << "\"] ";
  }
  ssCommand << "]";
  manager.SendCommand( ssCommand.str() );

  // Make sure the window is showing.
  ssCommand.str("");
  ssCommand << "Chart_ShowWindow " << chartID;
  manager.SendCommand( ssCommand.str() );
  
}

TclChartWindowStaticListener::TclChartWindowStaticListener () {

  TclCommandManager& commandMgr = TclCommandManager::GetManager();
  commandMgr.AddCommand( *this, "DeleteTclChartWindow", 1, "chartID", 
			 "Deletes a TclChartWindow." );

}

TclCommandListener::TclCommandResult
TclChartWindowStaticListener::DoListenToTclCommand ( char* isCommand, 
						     int, char** iasArgv ) {

  // DeleteTclChartWindow <chartID>
  if( 0 == strcmp( isCommand, "DeleteTclChartWindow" ) ) {

    // Get our chart ID.
    int chartID;
    try {
      chartID = TclCommandManager::ConvertArgumentToInt( iasArgv[1] );
      }
    catch( runtime_error& e ) {
      sResult = string("bad chartID: ") + e.what();
      return error;
    }
    
    // Try to find the chart window.
    TclChartWindow* chart;
    try {
      chart = (TclChartWindow*) &TclChartWindow::FindByID( chartID );
    }
    catch(...) {
      throw runtime_error( "Couldn't find the chart window." );
    }
    
    // Delete the chart window object.
    if( NULL != chart )
      delete chart;
  }

  return ok;
}
