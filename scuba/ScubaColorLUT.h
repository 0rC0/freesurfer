#ifndef ScubaColorLUT_h
#define ScubaColorLUT_h

#include <string>
extern "C" {
#include "colortab.h"
}
#include "DebugReporter.h"
#include "TclCommandManager.h"
#include "IDTracker.h"

class ScubaColorLUTStaticTclListener : public DebugReporter, public TclCommandListener {

 public:
  ScubaColorLUTStaticTclListener ();
  ~ScubaColorLUTStaticTclListener ();
  
  virtual TclCommandResult
    DoListenToTclCommand ( char* isCommand, int iArgc, char** iasArgv );
};

class ScubaColorLUT : public DebugReporter, public IDTracker<ScubaColorLUT>, public TclCommandListener {

  friend class ScubaColorLUTTester;

 public:

  ScubaColorLUT ();
  ~ScubaColorLUT ();

  virtual TclCommandResult
    DoListenToTclCommand ( char* isCommand, int iArgc, char** iasArgv );

  void UseFile ( std::string ifnLUT );

  void GetColorAtIndex ( int iIndex, int oColor[3] );
  void GetIndexOfColor ( int iColor[3], int& oIndex );

  std::string GetLabelAtIndex ( int iIndex );

  void SetLabel( std::string isLabel ) { msLabel = isLabel; }
  std::string GetLabel() { return msLabel; }

 protected:

  const static int cDefaultEntries;

  void ReadFile ();

  std::string mfnLUT;
  int mcEntries;
  struct LUTEntry { std::string msLabel; int color[3]; };
  std::map<int,LUTEntry> mEntries; 

  std::string msLabel;

  static ScubaColorLUTStaticTclListener mStaticListener;

};


#endif
