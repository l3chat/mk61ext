/*
||
|| @file Key.h
|| @version 1.0
|| @author Mark Stanley
|| @contact mstanley@technologist.com
||
|| @description
|| | Key class provides an abstract definition of a key or button
|| | and was initially designed to be used in conjunction with a
|| | state-machine.
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

#ifndef KEYPADLIB_KEY_H_
#define KEYPADLIB_KEY_H_

#include <Arduino.h>

#define OPEN LOW
#define CLOSED HIGH

using uint = unsigned int;
enum KeyState : uint8_t { IDLE, PRESSED, HOLD, RELEASED };

constexpr char NO_KEY = '\0';

class Key {
public:
  Key() = default;
  explicit Key(char userKeyChar);
  void key_update(char userKeyChar, KeyState userState, bool userStatus);

  char kchar = NO_KEY;
  int kcode = -1;
  KeyState kstate = IDLE;
  bool stateChanged = false;
  unsigned long holdStartTime = 0;
};

#endif
