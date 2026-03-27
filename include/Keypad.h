/*
||
|| @file Keypad.h
|| @version 3.1
|| @author Mark Stanley, Alexander Brevig
|| @contact mstanley@technologist.com, alexanderbrevig@gmail.com
||
|| @description
|| | This library provides a simple interface for using matrix
|| | keypads. It supports multiple keypresses while maintaining
|| | backwards compatibility with the old single key library.
|| | It also supports user selectable pins and definable keymaps.
|| #
||
|| @license
|| | This library is free software; you can redistribute it and/or
|| | modify it under the terms of the GNU Lesser General Public
|| | License as published by the Free Software Foundation; version
|| | 2.1 of the License.
|| |
|| | This library is distributed in the hope that it will be useful,
|| | but WITHOUT ANY WARRANTY; without even the implied warranty of
|| | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|| | Lesser General Public License for more details.
|| |
|| | You should have received a copy of the GNU Lesser General Public
|| | License along with this library; if not, write to the Free Software
|| | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
|| #
||
*/

#ifndef KEYPAD_H
#define KEYPAD_H

#include "Key.h"

#define LIST_MAX 10
#define MAPSIZE 10
#define makeKeymap(x) ((char *)x)

using KeypadEvent = char;
using ulong = unsigned long;

struct KeypadSize {
  byte rows;
  byte columns;
};

using KeypadEventListener = void (*)(char);

class Keypad : public Key {
public:
  Keypad(char *userKeymap, byte *row, byte *col, byte numRows, byte numCols);

  virtual void pin_mode(byte pinNum, byte mode) { pinMode(pinNum, mode); }
  virtual void pin_write(byte pinNum, bool level) { digitalWrite(pinNum, level); }
  virtual int pin_read(byte pinNum) { return digitalRead(pinNum); }

  uint bitMap[MAPSIZE] = {};
  Key key[LIST_MAX];

  char getKey();
  bool getKeys();
  KeyState getState();
  void begin(char *userKeymap);
  bool isPressed(char keyChar);
  void setDebounceTime(uint debounce);
  void setHoldTime(uint hold);
  void addEventListener(void (*listener)(char));
  int findInList(char keyChar);
  int findInList(int keyCode);
  char waitForKey();
  bool keyStateChanged();
  byte numKeys();

private:
  unsigned long startTime = 0;
  char *keymap = nullptr;
  byte *rowPins = nullptr;
  byte *columnPins = nullptr;
  KeypadSize sizeKpd{0, 0};
  uint debounceTime = 1;
  uint holdTime = 500;
  bool single_key = false;

  void scanKeys();
  bool updateList();
  void nextKeyState(byte idx, bool button);
  void transitionTo(byte idx, KeyState nextState);
  KeypadEventListener keypadEventListener = nullptr;
};

#endif
