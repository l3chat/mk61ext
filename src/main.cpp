/*
 * ADD TO PCB
 * ----------
 * + pull up resistors I2C SDA, SCL
 * + on/off switch: GND to 3V3_EN
 * + buttons RESET, BOOTSELECT
 * - SWD connector
 * + battery charger with MOSFET ON/OFF switch
 *
 *
 * DISPLAY
 * -------
 * GMG12864-06D Ver.2.2
 * ST7565R
 * https://www.lcd-module.de/eng/pdf/zubehoer/st7565r.pdf
 * https://www.youtube.com/watch?v=YwYkXSt6HfA
 * https://rcl-radio.ru/?p=83094
 * http://arduino.ru/forum/apparatnye-voprosy/podklyuchenie-displeya-gmg12864-06d-na-st7565r
 * https://www.sparkfun.com/datasheets/LCD/GDM12864H.pdf
 *
 * 1 CS  -                            - 17 //17
 * 2 RSE - reset RST                  - 18 //20
 * 3 RS  - Data/Command select DC A0  - 19 //21
 * 4 SCL -                            - 20 //18
 * 5 SI  - MOSI                       - 21 //19
 * 6 VDD - 3v3
 * 7 VSS - GND
 * 8 A   - (3v3)                      - D-pin of the P-channel MOSFET
 * 9 K   - GND
 * 10 IC_SCL
 * 11 IC_CS
 * 12 IC_SO
 * 13 IC_SI
 *
 *
 *
 * P-channel MOSFET for backlight
 * ----------------
 * G - 16
 * D - DISPLAY 8 A
 * S - 3v3
 *
 *
 *
 * external EEPROM
 * ---------------
 * AT24C256C-SSHL-T - 256 kbits
 * jlcpcb - C6482
 * https://datasheet.lcsc.com/szlcsc/Microchip-Tech-AT24C256C-SSHL-T_C6482.pdf
 *
 * Firmware expects I2C address 0x50, so tie A0/A1/A2 low.
 *
 * 1 A0  - GND
 * 2 A1  - GND
 * 3 A2  - GND
 * 4 GND - GND
 * 5 SDA - GPIO 14, with pull-up to 3v3
 * 6 SCL - GPIO 15, with pull-up to 3v3
 * 7 WP  - GND for normal writes, or 3v3 to write-protect
 * 8 VCC - 3v3
 *
 *
 *
 * LIPO charger
 * ------------
 * https://shop.pimoroni.com/products/pico-lipo-shim
 * https://cdn.shopify.com/s/files/1/0174/1800/files/lipo_shim_for_pico_schematic.pdf?v=1618573454
 * https://www.best-microcontroller-projects.com/tp4056.html
 * https://datasheet.lcsc.com/lcsc/1809261820_TOPPOWER-Nanjing-Extension-Microelectronics-TP4056-42-ESOP8_C16581.pdf
 *
 * TP4056-42-ESOP8
 * C16581
 * https://datasheet.lcsc.com/lcsc/1809261820_TOPPOWER-Nanjing-Extension-Microelectronics-TP4056-42-ESOP8_C16581.pdf
 *
 * DW01-G
 * C14213
 * https://datasheet.lcsc.com/lcsc/1810101017_Fortune-Semicon-DW01-G_C14213.pdf
 *
 * https://datasheet.lcsc.com/lcsc/1809050110_Fortune-Semicon-FS8205A_C16052.pdf
 */

#include <Arduino.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <cmath>
#include <cstdio>

#include <SparkFun_External_EEPROM.h>  // http://librarymanager/All#SparkFun_External_EEPROM

#include "CalculatorKeymap.h"
#include "Keypad.h"
#include "RpnCalculator.h"

namespace {

constexpr byte kRowCount = 8;
constexpr byte kColumnCount = 5;

char kKeyMap[kRowCount][kColumnCount] = {
    {'a', 'b', 'c', 'd', 'e'},
    {'f', 'g', 'h', 'i', 'j'},
    {'k', 'l', 'm', 'n', 'o'},
    {'p', 'q', 'r', 's', 't'},
    {'7', '8', '9', '-', '/'},
    {'4', '5', '6', '+', '*'},
    {'1', '2', '3', 'u', 'v'},
    {'0', '.', 'x', 'y', 'z'},
};

byte kRowPins[kRowCount] = {7, 6, 5, 4, 3, 2, 1, 0};  // connect to the row pinouts of the keypad
byte kColumnPins[kColumnCount] = {8, 9, 10, 11, 12};  // connect to the column pinouts of the keypad

constexpr uint8_t kDisplayClockPin = 20;
constexpr uint8_t kDisplayDataPin = 21;
constexpr uint8_t kDisplayChipSelectPin = 17;
constexpr uint8_t kDisplayDataCommandPin = 19;
constexpr uint8_t kDisplayResetPin = 18;
constexpr uint8_t kBacklightPin = 16;

constexpr uint8_t kEepromAddress = 0x50;
constexpr uint8_t kEepromSdaPin = 14;
constexpr uint8_t kEepromSclPin = 15;
constexpr uint32_t kEepromClockHz = 400000;
constexpr uint16_t kDisplayContrast = 20;
constexpr uint16_t kLoopDelayMs = 100;

struct KeypadDiagnostics {
  char lastKey = NO_KEY;
  KeyState lastState = IDLE;
  byte activeKeys = 0;
  bool hasEvent = false;
};

Keypad keypad(makeKeymap(kKeyMap), kRowPins, kColumnPins, kRowCount, kColumnCount);
TwoWire eepromWire(kEepromSdaPin, kEepromSclPin);
ExternalEEPROM eeprom;
RpnCalculator calculator;
U8G2_ST7565_ERC12864_F_4W_SW_SPI display(
    U8G2_R0,
    kDisplayClockPin,
    kDisplayDataPin,
    kDisplayChipSelectPin,
    kDisplayDataCommandPin,
    kDisplayResetPin);

int brightness = 256;
KeypadDiagnostics keypadDiagnostics;

void applyBrightness() {
  if (brightness >= 256) {
    digitalWrite(kBacklightPin, LOW);  // on
  } else if (brightness <= 0) {
    digitalWrite(kBacklightPin, HIGH);  // off
  } else {
    analogWrite(kBacklightPin, 255 - brightness);
  }
}

void increaseBrightness() {
  brightness *= 2;
  if (brightness > 256) {
    brightness = 256;
  }
  if (brightness == 0) {
    brightness = 1;
  }
  applyBrightness();
}

void decreaseBrightness() {
  brightness /= 2;
  applyBrightness();
}

void handlePressedKey(char keyPressed) {
  if (keyPressed == 'a') {
    increaseBrightness();
  } else if (keyPressed == 'b') {
    decreaseBrightness();
  } else {
    calculator.apply(translateKeyToCalculatorAction(keyPressed));
  }
}

const char *keyStateName(KeyState state) {
  switch (state) {
    case IDLE:
      return "IDLE";
    case PRESSED:
      return "PRESS";
    case HOLD:
      return "HOLD";
    case RELEASED:
      return "REL";
  }

  return "?";
}

const char *activeLegend() {
  const uint8_t page = (millis() / 2000U) % calculatorLegendPageCount();
  return calculatorLegend(page);
}

void updateInput() {
  const bool keyActivity = keypad.getKeys();

  byte activeKeys = 0;
  for (byte index = 0; index < keypad.numKeys(); index++) {
    const Key &key = keypad.key[index];

    if ((key.kchar != NO_KEY) && (key.kstate != IDLE)) {
      activeKeys++;
    }

    if (!keyActivity) {
      continue;
    }

    if (key.stateChanged) {
      keypadDiagnostics.lastKey = key.kchar;
      keypadDiagnostics.lastState = key.kstate;
      keypadDiagnostics.hasEvent = true;

      if (key.kstate == PRESSED) {
        handlePressedKey(key.kchar);
      }
    }
  }

  keypadDiagnostics.activeKeys = activeKeys;
}

void formatStackValue(double value, char *buffer, size_t bufferSize) {
  const double normalizedValue = (std::fabs(value) < 1e-12) ? 0.0 : value;
  snprintf(buffer, bufferSize, "%.10g", normalizedValue);
}

void drawStackLine(int y, const char *label, double value) {
  char valueBuffer[18];
  char lineBuffer[22];

  formatStackValue(value, valueBuffer, sizeof(valueBuffer));
  snprintf(lineBuffer, sizeof(lineBuffer), "%s %s", label, valueBuffer);
  display.drawStr(0, y, lineBuffer);
}

void setupDisplay() {
  display.begin();
  display.setPowerSave(0);
  display.setFlipMode(1);
  display.setContrast(kDisplayContrast);
  display.setFont(u8g2_font_t0_15b_mr);
  display.setFontRefHeightExtendedText();
  display.setDrawColor(1);
  display.setFontPosTop();
  display.setFontDirection(0);

  pinMode(kBacklightPin, OUTPUT);
  applyBrightness();
}

void setupEeprom() {
  eepromWire.setClock(kEepromClockHz);
  eepromWire.begin();

  if (!eeprom.begin(kEepromAddress, eepromWire)) {
    display.drawStr(0, 0, "no memory found");
    while (true) {
      delay(1);
    }
  }

  display.drawStr(0, 0, "  memory found");
  eeprom.setMemorySize(262144 / 8);  // EEPROM is the AT24C256 (256 Kbit = 32768 bytes)
  eeprom.setPageSize(64);
  eeprom.setPageWriteTime(3);
  eeprom.disablePollForWriteComplete();
}

void drawCalculatorScreen() {
  char statusBuffer[22];
  const CalculatorStack stack = calculator.stack();
  const char keyLabel = keypadDiagnostics.hasEvent ? keypadDiagnostics.lastKey : '-';
  const char *xLabel = calculator.isEntering() ? "X>" : "X:";

  display.setFont(u8g2_font_6x10_mr);
  drawStackLine(0, "T:", stack.t);
  drawStackLine(10, "Z:", stack.z);
  drawStackLine(20, "Y:", stack.y);
  drawStackLine(30, xLabel, stack.x);

  if (calculator.hasError()) {
    snprintf(statusBuffer, sizeof(statusBuffer), "err:%s", calculator.errorMessage());
  } else {
    snprintf(statusBuffer,
             sizeof(statusBuffer),
             "evt:%c %s act:%u",
             keyLabel,
             keypadDiagnostics.hasEvent ? keyStateName(keypadDiagnostics.lastState) : "-",
             keypadDiagnostics.activeKeys);
  }

  display.drawStr(0, 42, statusBuffer);
  display.drawStr(0, 54, activeLegend());
}

}  // namespace

void setup() {
  setupDisplay();

  display.clearBuffer();
  setupEeprom();
  display.sendBuffer();
  delay(3000);
}

void loop() {
  display.clearBuffer();
  updateInput();
  drawCalculatorScreen();

  display.sendBuffer();
  delay(kLoopDelayMs);
}
