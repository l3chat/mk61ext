#include "Keypad.h"

Keypad::Keypad(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols)
    : rowPins(row), columnPins(col), sizeKpd{numRows, numCols} {
  begin(userKeymap);
  setDebounceTime(5);
  setHoldTime(500);
}

void Keypad::begin(char *userKeymap) {
  keymap = userKeymap;
}

char Keypad::getKey() {
  char pressedKey = NO_KEY;
  single_key = true;

  if (getKeys() && key[0].stateChanged && (key[0].kstate == PRESSED)) {
    pressedKey = key[0].kchar;
  }

  single_key = false;
  return pressedKey;
}

bool Keypad::getKeys() {
  const unsigned long now = millis();

  if ((now - startTime) <= debounceTime) {
    return false;
  }

  scanKeys();
  startTime = now;
  return updateList();
}

void Keypad::scanKeys() {
  for (byte row = 0; row < sizeKpd.rows; row++) {
    pin_mode(rowPins[row], INPUT_PULLUP);
    bitMap[row] = 0;
  }

  for (byte col = 0; col < sizeKpd.columns; col++) {
    pin_mode(columnPins[col], OUTPUT);
    pin_write(columnPins[col], LOW);

    for (byte row = 0; row < sizeKpd.rows; row++) {
      bitWrite(bitMap[row], col, !pin_read(rowPins[row]));
    }

    pin_write(columnPins[col], HIGH);
    pin_mode(columnPins[col], INPUT);
  }
}

bool Keypad::updateList() {
  bool anyActivity = false;

  for (byte index = 0; index < LIST_MAX; index++) {
    if (key[index].kstate == IDLE) {
      key[index].kchar = NO_KEY;
      key[index].kcode = -1;
      key[index].stateChanged = false;
      key[index].holdStartTime = 0;
    }
  }

  for (byte row = 0; row < sizeKpd.rows; row++) {
    for (byte col = 0; col < sizeKpd.columns; col++) {
      const bool button = bitRead(bitMap[row], col);
      const char keyChar = keymap[row * sizeKpd.columns + col];
      const int keyCode = row * sizeKpd.columns + col;
      const int listIndex = findInList(keyCode);

      if (listIndex > -1) {
        nextKeyState(listIndex, button);
      }

      if ((listIndex == -1) && button) {
        for (byte index = 0; index < LIST_MAX; index++) {
          if (key[index].kchar == NO_KEY) {
            key[index].kchar = keyChar;
            key[index].kcode = keyCode;
            key[index].kstate = IDLE;
            nextKeyState(index, button);
            break;
          }
        }
      }
    }
  }

  for (byte index = 0; index < LIST_MAX; index++) {
    if (key[index].stateChanged) {
      anyActivity = true;
    }
  }

  return anyActivity;
}

void Keypad::nextKeyState(byte idx, bool button) {
  const unsigned long now = millis();
  key[idx].stateChanged = false;

  switch (key[idx].kstate) {
    case IDLE:
      if (button == CLOSED) {
        transitionTo(idx, PRESSED);
        key[idx].holdStartTime = now;
      }
      break;

    case PRESSED:
      if ((now - key[idx].holdStartTime) > holdTime) {
        transitionTo(idx, HOLD);
      } else if (button == OPEN) {
        transitionTo(idx, RELEASED);
      }
      break;

    case HOLD:
      if (button == OPEN) {
        transitionTo(idx, RELEASED);
      }
      break;

    case RELEASED:
      transitionTo(idx, IDLE);
      break;
  }
}

bool Keypad::isPressed(char keyChar) {
  for (byte index = 0; index < LIST_MAX; index++) {
    if ((key[index].kchar == keyChar) && (key[index].kstate == PRESSED) &&
        key[index].stateChanged) {
      return true;
    }
  }

  return false;
}

int Keypad::findInList(char keyChar) {
  for (byte index = 0; index < LIST_MAX; index++) {
    if (key[index].kchar == keyChar) {
      return index;
    }
  }

  return -1;
}

int Keypad::findInList(int keyCode) {
  for (byte index = 0; index < LIST_MAX; index++) {
    if (key[index].kcode == keyCode) {
      return index;
    }
  }

  return -1;
}

char Keypad::waitForKey() {
  char waitKey = NO_KEY;

  while ((waitKey = getKey()) == NO_KEY) {
  }

  return waitKey;
}

KeyState Keypad::getState() {
  return key[0].kstate;
}

bool Keypad::keyStateChanged() {
  return key[0].stateChanged;
}

byte Keypad::numKeys() {
  return LIST_MAX;
}

void Keypad::setDebounceTime(uint debounce) {
  debounceTime = debounce < 1 ? 1 : debounce;
}

void Keypad::setHoldTime(uint hold) {
  holdTime = hold;
}

void Keypad::addEventListener(void (*listener)(char)) {
  keypadEventListener = listener;
}

void Keypad::transitionTo(byte idx, KeyState nextState) {
  key[idx].kstate = nextState;
  key[idx].stateChanged = true;

  if (keypadEventListener == nullptr) {
    return;
  }

  if (single_key && idx != 0) {
    return;
  }

  keypadEventListener(key[idx].kchar);
}
