#include "Key.h"

Key::Key(char userKeyChar) : kchar(userKeyChar) {}

void Key::key_update(char userKeyChar, KeyState userState, bool userStatus) {
  kchar = userKeyChar;
  kstate = userState;
  stateChanged = userStatus;
}
