#include "UndoManager.h"

using namespace std;

UndoManager&
UndoManager::GetManager () {

  static UndoManager* sManager = NULL;
  if( NULL == sManager ) {
    sManager = new UndoManager();

    TclCommandManager& commandMgr = TclCommandManager::GetManager();
    commandMgr.AddCommand( *sManager, "GetUndoTitle", 0, "",
			   "Returns the title for the undo action." );
    commandMgr.AddCommand( *sManager, "GetRedoTitle", 0, "",
			   "Returns the title for the redo action." );
    commandMgr.AddCommand( *sManager, "Undo", 0, "",
			   "Undoes the last action." );
    commandMgr.AddCommand( *sManager, "Redo", 0, "",
			   "Redoes last undone action." );
  };

  return *sManager;
}

UndoManager::UndoManager () {
  mcMaxActions = 30;
}

void
UndoManager::BeginAction ( std::string isTitle ) {

  mCurrentAction = new UndoableAction();
  mCurrentAction->msTitle = isTitle;
}

void
UndoManager::AddAction ( UndoAction* iAction ) {

  if( NULL == mCurrentAction ) {
    throw new runtime_error( "Tried to add undo action without beginning an action first." );
  }

  mCurrentAction->mActions.push_back( iAction );
}

void
UndoManager::EndAction () {

  mUndoActions.push_back( mCurrentAction );
  mCurrentAction = NULL;

  if( mUndoActions.size() + mRedoActions.size() > 
      (unsigned int)mcMaxActions ) {

    // First pop the oldest redo item if we have one, because that's
    // probably the lest neeeded item. If the redo list is empty, do
    // the undo list. The oldest one in each list will be the
    // front. (That's notnecessarily true, but good enough for us.)
    if( mRedoActions.size() > 0 ) {
      UndoableAction* toDelete = mRedoActions.front();
      mRedoActions.pop_front();
      delete toDelete;
    } else {
      UndoableAction* toDelete = mUndoActions.front();
      mUndoActions.pop_front();
      delete toDelete;
    }
  }
}

string
UndoManager::GetUndoTitle () { 

  if( mUndoActions.size() > 0 ) {
    UndoableAction* undoableAction = mUndoActions.back();
    if( NULL != undoableAction ) {
      stringstream ss;
      ss << "Undo " << undoableAction->msTitle;
      return ss.str();
    }
  }
  return "No Action to Undo";
}

string
UndoManager::GetRedoTitle () { 

  if( mRedoActions.size() > 0 ) {
    UndoableAction* redoableAction = mRedoActions.back();
    if( NULL != redoableAction ) {
      stringstream ss;
      ss << "Redo " << redoableAction->msTitle;
      return ss.str();
    }
  }
  return "No Action to Redo";
}

void
UndoManager::Undo () {

  if( mUndoActions.size() > 0 ) {
    UndoableAction* undoableAction =  mUndoActions.back();
    if( NULL != undoableAction ) {
      mUndoActions.pop_back();
      
      list<UndoAction*>::iterator tActions;
      for( tActions = undoableAction->mActions.begin();
	   tActions != undoableAction->mActions.end(); 
	   ++tActions ) {
	UndoAction* action = *tActions;
	action->Undo();
      }
      
      mRedoActions.push_back( undoableAction );
    }
  }
}

void
UndoManager::Redo () {

  if( mRedoActions.size() > 0 ) {
    UndoableAction* redoableAction = mRedoActions.back();
    if( NULL != redoableAction ) {
      mRedoActions.pop_back();
      
      list<UndoAction*>::iterator tActions;
      for( tActions = redoableAction->mActions.begin();
	   tActions != redoableAction->mActions.end(); 
	   ++tActions ) {
	UndoAction* action = *tActions;
	action->Redo();
      }
      
      mUndoActions.push_back( redoableAction );
    }
  }
}

void
UndoManager::Clear () {

  list<UndoableAction*>::iterator tAction;
  for( tAction = mUndoActions.begin();
       tAction != mUndoActions.end(); 
       ++tAction ) {
    UndoableAction* action = *tAction;
    delete action;
  }
      
  for( tAction = mRedoActions.begin();
       tAction != mRedoActions.end(); 
       ++tAction ) {
    UndoableAction* action = *tAction;
    delete action;
  }
     
  mUndoActions.clear();
  mRedoActions.clear();
}

TclCommandManager::TclCommandResult
UndoManager::DoListenToTclCommand ( char* isCommand, 
				    int iArgc, char** iasArgv ) {


  // GetUndoTitle
  if( 0 == strcmp( isCommand, "GetUndoTitle" ) ) {

    sReturnFormat = "s";
    stringstream ssReturnValues;
    ssReturnValues << "\"" << GetUndoTitle() << "\"";
    sReturnValues = ssReturnValues.str();
  }

  // GetRedoTitle
  if( 0 == strcmp( isCommand, "GetRedoTitle" ) ) {

    sReturnFormat = "s";
    stringstream ssReturnValues;
    ssReturnValues << "\"" << GetRedoTitle() << "\"";
    sReturnValues = ssReturnValues.str();
  }

  // Undo
  if( 0 == strcmp( isCommand, "Undo" ) ) {
    Undo();
  }

  // Redo
  if( 0 == strcmp( isCommand, "Redo" ) ) {
    Redo();
  }

  return ok;
}

UndoAction::UndoAction () {

}

UndoAction::~UndoAction () {

}

void 
UndoAction::Undo () {

}

void 
UndoAction::Redo () {

}

UndoableAction::~UndoableAction () {

  list<UndoAction*>::iterator tActions;
  for( tActions = mActions.begin();
       tActions != mActions.end(); 
       ++tActions ) {
    UndoAction* action = *tActions;
    delete action;
  }
}
