#ifndef InputState_h
#define InputState_h

#include <iostream>
#include <string>

class InputState {
  friend class ToglManager;
  friend class InputStateTester;
 public:
  InputState();
  bool IsShiftKeyDown();
  bool IsAltKeyDown();
  bool IsControlKeyDown();
  int Button();
  bool IsButtonDown();
  bool IsButtonDragging();
  std::string Key();
 protected:
  bool mbShiftKey;
  bool mbAltKey;
  bool mbControlKey;
  bool mbButtonDown;
  bool mbButtonDragging;
  int mButton;
  std::string msKey;
};

std::ostream& operator << ( std::ostream& os, InputState& iInput );



#endif
