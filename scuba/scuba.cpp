#include "ScubaGlobalPreferences.h"
#include "PreferencesManager.h"
#include "ToglManager.h"
#include "ScubaFrame.h"
#include "ScubaView.h"
#include "ScubaLayerFactory.h"
#include "ScubaLayer2DMRI.h"
#include "ScubaDataCollectionFactory.h"
#include "Scuba-impl.h"

char* Progname = "scuba";


// Togl tester ---------------------------------------------------------

extern "C" {
int Scuba_Init ( Tcl_Interp* iInterp ) {
    
    try {
    ToglManager& toglMgr = ToglManager::GetManager();
    toglMgr.InitializeTogl( iInterp );
    toglMgr.SetFrameFactory( new ScubaFrameFactory );

    ScubaFrame::SetViewFactory( new ScubaViewFactory );

    TclCommandManager& commandMgr = TclCommandManager::GetManager();
    commandMgr.SetOutputStreamToCerr();
    commandMgr.Start( iInterp );

    ScubaLayerFactory& layerFactory = 
      ScubaLayerFactory::GetFactory();
    layerFactory.SetOutputStreamToCerr();

    ScubaDataCollectionFactory& dataFactory = 
      ScubaDataCollectionFactory::GetFactory();
    dataFactory.SetOutputStreamToCerr();

    PreferencesManager& prefsMgr = PreferencesManager::GetManager();
    //    prefsMgr.SetOutputStreamToCerr();
    prefsMgr.UseFile( ".scuba" );

    ScubaGlobalPreferences preferences =
      ScubaGlobalPreferences::GetPreferences();
  }
  catch( ... ) {
    return TCL_ERROR;
  }

  return TCL_OK;
}
}
