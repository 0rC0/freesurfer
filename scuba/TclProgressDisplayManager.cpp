#include "string_fixed.h"
#include <sstream>
#include "TclProgressDisplayManager.h"
#include "TclCommandManager.h"

using namespace std;

TclProgressDisplayManager& 
TclProgressDisplayManager::GetManager() {

  static TclProgressDisplayManager* sManager = NULL;
  if( NULL == sManager ) {
    sManager = new TclProgressDisplayManager();
  }

  return *sManager;
}

void 
TclProgressDisplayManager::NewTask( string isTitle,
				    string isText,
				    bool ibUseMeter,
				    list<string> ilsButtons ) {
  
  stringstream ssCommand;
  ssCommand << "NewTask ";
  if( isTitle != "" ) {
    ssCommand << "-title \"" << isTitle << "\" ";
  }
  if( isText != "" ) {
    ssCommand << "-text \"" << isTitle << "\" ";
  }
  if( ibUseMeter ) {
    ssCommand << "-meter true ";
  } else {
    ssCommand << "-meter false ";
  }
  if( ilsButtons.size() > 0 ) {
    ssCommand << "-buttons {";
    list<string>::iterator tButtons;
    for( tButtons = ilsButtons.begin(); tButtons != ilsButtons.end();
	 ++tButtons ) {
      ssCommand << "\"" << *tButtons << "\" ";
    }
    ssCommand << "}";
  }

  TclCommandManager& manager = TclCommandManager::GetManager();
  manager.SendCommand( ssCommand.str() );

  mlButtons = ilsButtons;

  // Since we're stuck in a c loop at this point, we need to let the
  // Tcl environment handle an event. Otherwise all our windowing
  // stuff will go unnoticed.
  manager.DoTclEvent();
  
}
  
void 
TclProgressDisplayManager::UpdateTask( string isText,
			    float iPercent ) {

  stringstream ssCommand;
  ssCommand << "UpdateTask ";
  if( isText != "" ) {
    ssCommand << "-text \"" << isText << "\" ";
  }
  if( iPercent != -1 ) {
    ssCommand << "-percent " << iPercent << " ";
  }

  TclCommandManager& manager = TclCommandManager::GetManager();
  manager.SendCommand( ssCommand.str() );
}
  
int 
TclProgressDisplayManager::CheckTaskForButton() {

  TclCommandManager& manager = TclCommandManager::GetManager();
  string sResult =  manager.SendCommand( "CheckTaskForButtons" );
  
  // Since we're stuck in a c loop at this point, we need to let the
  // Tcl environment handle an event. Otherwise all our windowing
  // stuff will go unnoticed.
  manager.DoTclEvent();
  
  // Search the string for the title of each button. If we find it,
  // return its index.
  int nButton = 0;
  list<string>::iterator tButtons;
  for( tButtons = mlButtons.begin(); tButtons != mlButtons.end();
       ++tButtons ) {

    string::size_type position = sResult.find( *tButtons, 0 );
    if( position != string::npos ) {
      return nButton;
    }

    nButton++;
  }

  return -1;
}

void 
TclProgressDisplayManager::EndTask() {

  TclCommandManager& manager = TclCommandManager::GetManager();
  manager.SendCommand( "EndTask" );
}

