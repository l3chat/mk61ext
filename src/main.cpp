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
#include <cstring>

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
constexpr int kDisplayWidth = 128;
constexpr int kStatusBarHeight = 8;
constexpr int kStackFirstY = 10;
constexpr int kStackRowHeight = 14;
constexpr int kStackValueMaxPrecision = 15;
constexpr int kStackValueMinGap = 4;
constexpr int kHelpTitleY = 10;
constexpr int kHelpBodyY = 18;
constexpr int kHelpLineHeight = 7;
constexpr uint8_t kHelpBodyLineCount = 6;
constexpr int kHelpTextWidth = 126;
constexpr uint32_t kSettingsEepromAddress = 0;
constexpr uint32_t kSettingsMagic = 0x4D4B3631u;  // "MK61"
constexpr uint8_t kSettingsVersion = 1;

struct KeypadDiagnostics {
  char lastKey = NO_KEY;
  KeyState lastState = IDLE;
  bool hasEvent = false;
};

struct HelpState {
  bool enabled = false;
  bool hasSelection = false;
  char key = NO_KEY;
  CalculatorPrefix prefix = CalculatorPrefix::None;
};

struct StoredSettings {
  uint32_t magic = kSettingsMagic;
  uint8_t version = kSettingsVersion;
  uint8_t angleMode = static_cast<uint8_t>(CalculatorAngleMode::Radians);
  uint8_t showStackLabels = 1;
  uint8_t reserved0 = 0;
  uint16_t brightness = 0;
  uint8_t reserved[8] = {};
};

enum class PendingRegisterOperation : uint8_t {
  None,
  DirectRecall,
  DirectStore,
  IndirectRecall,
  IndirectStore,
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

int brightness = 0;
bool showStackLevelNames = true;
bool eepromReady = false;
KeypadDiagnostics keypadDiagnostics;
HelpState helpState;
PendingRegisterOperation pendingRegisterOperation = PendingRegisterOperation::None;

StoredSettings defaultStoredSettings() {
  return StoredSettings{};
}

const char *angleModeShortName(CalculatorAngleMode mode) {
  switch (mode) {
    case CalculatorAngleMode::Radians:
      return "RAD";
    case CalculatorAngleMode::Gradians:
      return "GRD";
    case CalculatorAngleMode::Degrees:
      return "DEG";
  }

  return "RAD";
}

CalculatorAngleMode nextAngleMode(CalculatorAngleMode mode) {
  switch (mode) {
    case CalculatorAngleMode::Radians:
      return CalculatorAngleMode::Gradians;
    case CalculatorAngleMode::Gradians:
      return CalculatorAngleMode::Degrees;
    case CalculatorAngleMode::Degrees:
      return CalculatorAngleMode::Radians;
  }

  return CalculatorAngleMode::Radians;
}

bool isValidStoredSettings(const StoredSettings &settings) {
  if ((settings.magic != kSettingsMagic) || (settings.version != kSettingsVersion)) {
    return false;
  }

  if (settings.angleMode > static_cast<uint8_t>(CalculatorAngleMode::Degrees)) {
    return false;
  }

  if (settings.brightness > 256) {
    return false;
  }

  return true;
}

StoredSettings currentStoredSettings() {
  StoredSettings settings = defaultStoredSettings();
  settings.angleMode = static_cast<uint8_t>(calculator.angleMode());
  settings.showStackLabels = showStackLevelNames ? 1 : 0;
  settings.brightness = static_cast<uint16_t>(brightness);
  return settings;
}

void applyBrightness() {
  if (brightness >= 256) {
    digitalWrite(kBacklightPin, LOW);  // on
  } else if (brightness <= 0) {
    digitalWrite(kBacklightPin, HIGH);  // off
  } else {
    analogWrite(kBacklightPin, 255 - brightness);
  }
}

void saveSettings() {
  if (!eepromReady) {
    return;
  }

  const StoredSettings settings = currentStoredSettings();
  (void)eeprom.putChanged(kSettingsEepromAddress, settings);
}

void applyStoredSettings(const StoredSettings &settings) {
  brightness = settings.brightness;
  showStackLevelNames = settings.showStackLabels != 0;
  calculator.setAngleMode(static_cast<CalculatorAngleMode>(settings.angleMode));
  applyBrightness();
}

void loadSettings() {
  StoredSettings settings;
  eeprom.get(kSettingsEepromAddress, settings);

  if (!isValidStoredSettings(settings)) {
    settings = defaultStoredSettings();
    applyStoredSettings(settings);
    saveSettings();
    return;
  }

  applyStoredSettings(settings);
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
  saveSettings();
}

void decreaseBrightness() {
  brightness /= 2;
  applyBrightness();
  saveSettings();
}

void cycleAngleMode() {
  calculator.setAngleMode(nextAngleMode(calculator.angleMode()));
  saveSettings();
}

void toggleStackLevelNames() {
  showStackLevelNames = !showStackLevelNames;
  saveSettings();
}

void clearHelpSelection() {
  helpState.hasSelection = false;
  helpState.key = NO_KEY;
  helpState.prefix = CalculatorPrefix::None;
}

void clearPendingRegisterOperation() {
  pendingRegisterOperation = PendingRegisterOperation::None;
}

void armPendingRegisterOperation(PendingRegisterOperation operation) {
  pendingRegisterOperation = operation;
  resetCalculatorKeymapState();
}

const char *pendingRegisterOperationName() {
  switch (pendingRegisterOperation) {
    case PendingRegisterOperation::DirectRecall:
      return "RCL";
    case PendingRegisterOperation::DirectStore:
      return "STO";
    case PendingRegisterOperation::IndirectRecall:
      return "RCLI";
    case PendingRegisterOperation::IndirectStore:
      return "STOI";
    case PendingRegisterOperation::None:
      return "";
  }

  return "";
}

bool decodeRegisterKey(char keyPressed, uint8_t &index) {
  if ((keyPressed >= '0') && (keyPressed <= '9')) {
    index = static_cast<uint8_t>(keyPressed - '0');
    return true;
  }

  switch (keyPressed) {
    case '.':
      index = 10;
      return true;
    case 'x':
      index = 11;
      return true;
    case 'y':
      index = 12;
      return true;
    case 'z':
      index = 13;
      return true;
    case 'v':
      index = 14;
      return true;
    default:
      return false;
  }
}

void setHelpMode(bool enabled) {
  helpState.enabled = enabled;
  clearHelpSelection();
  clearPendingRegisterOperation();
  resetCalculatorKeymapState();
}

void toggleHelpMode() {
  setHelpMode(!helpState.enabled);
}

void rememberHelpSelection(char keyPressed, CalculatorPrefix prefix) {
  helpState.hasSelection = true;
  helpState.key = keyPressed;
  helpState.prefix = prefix;
}

void handleHelpPressedKey(char keyPressed) {
  if ((keyPressed == 'a') || (keyPressed == 'b')) {
    resetCalculatorKeymapState();
    rememberHelpSelection(keyPressed, CalculatorPrefix::None);
    return;
  }

  const CalculatorPrefix prefixBefore =
      ((keyPressed == 'k') || (keyPressed == 'p')) ? CalculatorPrefix::None : activeCalculatorPrefix();
  (void)translateKeyToCalculatorAction(keyPressed);
  rememberHelpSelection(keyPressed, prefixBefore);
}

bool handleRegisterOperationKey(char keyPressed) {
  const CalculatorPrefix prefix = activeCalculatorPrefix();

  if ((prefix == CalculatorPrefix::None) && (keyPressed == 'q')) {
    armPendingRegisterOperation(PendingRegisterOperation::DirectRecall);
    return true;
  }

  if ((prefix == CalculatorPrefix::None) && (keyPressed == 'r')) {
    armPendingRegisterOperation(PendingRegisterOperation::DirectStore);
    return true;
  }

  if ((prefix == CalculatorPrefix::K) && (keyPressed == 'q')) {
    armPendingRegisterOperation(PendingRegisterOperation::IndirectRecall);
    return true;
  }

  if ((prefix == CalculatorPrefix::K) && (keyPressed == 'r')) {
    armPendingRegisterOperation(PendingRegisterOperation::IndirectStore);
    return true;
  }

  if (pendingRegisterOperation == PendingRegisterOperation::None) {
    return false;
  }

  const PendingRegisterOperation operation = pendingRegisterOperation;
  clearPendingRegisterOperation();

  uint8_t registerIndex = 0;
  if (!decodeRegisterKey(keyPressed, registerIndex)) {
    return true;
  }

  switch (operation) {
    case PendingRegisterOperation::DirectRecall:
      (void)calculator.recallRegister(registerIndex);
      return true;
    case PendingRegisterOperation::DirectStore:
      (void)calculator.storeRegister(registerIndex);
      return true;
    case PendingRegisterOperation::IndirectRecall:
      (void)calculator.recallIndirectRegister(registerIndex);
      return true;
    case PendingRegisterOperation::IndirectStore:
      (void)calculator.storeIndirectRegister(registerIndex);
      return true;
    case PendingRegisterOperation::None:
      return false;
  }

  return false;
}

void handlePressedKey(char keyPressed) {
  if (keyPressed == 'e') {
    toggleHelpMode();
    return;
  }

  if (helpState.enabled) {
    handleHelpPressedKey(keyPressed);
    return;
  }

  if (handleRegisterOperationKey(keyPressed)) {
    return;
  }

  if (keyPressed == 'a') {
    resetCalculatorKeymapState();
    increaseBrightness();
    return;
  }

  if (keyPressed == 'b') {
    resetCalculatorKeymapState();
    decreaseBrightness();
    return;
  }

  if (keyPressed == 'c') {
    resetCalculatorKeymapState();
    cycleAngleMode();
    return;
  }

  if (keyPressed == 'd') {
    resetCalculatorKeymapState();
    toggleStackLevelNames();
    return;
  }

  calculator.apply(translateKeyToCalculatorAction(keyPressed));
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

void updateInput() {
  const bool keyActivity = keypad.getKeys();

  for (byte index = 0; index < keypad.numKeys(); index++) {
    const Key &key = keypad.key[index];

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
}

void formatStackValue(CalculatorValue value, char *buffer, size_t bufferSize, int precision) {
  const CalculatorValue normalizedValue = (std::fabs(value) < 1e-12) ? 0.0 : value;
  snprintf(buffer, bufferSize, "%.*g", precision, normalizedValue);
}

void drawRightAlignedText(int rightEdge, int y, const char *text) {
  display.drawStr(rightEdge - display.getStrWidth(text), y, text);
}

void formatStackValueToFit(CalculatorValue value,
                           char *buffer,
                           size_t bufferSize,
                           int maxPixelWidth) {
  for (int precision = kStackValueMaxPrecision; precision >= 1; --precision) {
    formatStackValue(value, buffer, bufferSize, precision);
    if (display.getStrWidth(buffer) <= maxPixelWidth) {
      return;
    }
  }

  formatStackValue(value, buffer, bufferSize, 1);
}

void drawStackTextLine(int y, const char *label, const char *text) {
  display.setFont(u8g2_font_6x10_mr);
  if ((label != nullptr) && (label[0] != '\0')) {
    display.drawStr(0, y, label);
  }
  drawRightAlignedText(kDisplayWidth - 1, y, text);
}

void drawStackLine(int y, const char *label, CalculatorValue value) {
  char valueBuffer[32];

  display.setFont(u8g2_font_6x10_mr);
  const int labelWidth = ((label != nullptr) && (label[0] != '\0')) ? display.getStrWidth(label) : 0;
  const int gap = (labelWidth > 0) ? kStackValueMinGap : 0;
  const int maxValueWidth = kDisplayWidth - labelWidth - gap;
  formatStackValueToFit(value, valueBuffer, sizeof(valueBuffer), maxValueWidth);
  drawStackTextLine(y, label, valueBuffer);
}

void formatXDisplayToFit(char *buffer, size_t bufferSize, int maxPixelWidth) {
  for (int precision = kStackValueMaxPrecision; precision >= 1; --precision) {
    calculator.formatXForDisplay(buffer, bufferSize, precision);
    if (display.getStrWidth(buffer) <= maxPixelWidth) {
      return;
    }
  }

  calculator.formatXForDisplay(buffer, bufferSize, 1);
}

void setupDisplay() {
  brightness = 0;
  pinMode(kBacklightPin, OUTPUT);
  applyBrightness();

  display.begin();
  display.setPowerSave(0);
  display.setFlipMode(1);
  display.setContrast(kDisplayContrast);
  display.setFont(u8g2_font_t0_15b_mr);
  display.setFontRefHeightExtendedText();
  display.setDrawColor(1);
  display.setFontPosTop();
  display.setFontDirection(0);
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
  eepromReady = true;
}

const char *calculatorModeName() {
  if (calculator.hasError()) {
    return "ERR";
  }

  if (calculator.isEnteringExponent()) {
    return "EEX";
  }

  if (calculator.isEntering()) {
    return "ENT";
  }

  return "RUN";
}

void formatEventLabel(char *buffer, size_t bufferSize) {
  if (!keypadDiagnostics.hasEvent) {
    snprintf(buffer, bufferSize, "--");
    return;
  }

  snprintf(buffer, bufferSize, "%c %s", keypadDiagnostics.lastKey, keyStateName(keypadDiagnostics.lastState));
}

void drawStatusBar() {
  char leftBuffer[24];
  char rightBuffer[18];

  if (helpState.enabled) {
    const char *prefixName = activeCalculatorPrefixName();
    if (prefixName[0] != '\0') {
      snprintf(leftBuffer, sizeof(leftBuffer), "MK61 %s HELP %s", angleModeShortName(calculator.angleMode()), prefixName);
    } else {
      snprintf(leftBuffer, sizeof(leftBuffer), "MK61 %s HELP", angleModeShortName(calculator.angleMode()));
    }
    formatEventLabel(rightBuffer, sizeof(rightBuffer));
  } else if (calculator.hasError()) {
    snprintf(leftBuffer, sizeof(leftBuffer), "ERR %s", calculator.errorMessage());
    rightBuffer[0] = '\0';
  } else if (pendingRegisterOperation != PendingRegisterOperation::None) {
    snprintf(leftBuffer,
             sizeof(leftBuffer),
             "MK61 %s %s",
             angleModeShortName(calculator.angleMode()),
             pendingRegisterOperationName());
    formatEventLabel(rightBuffer, sizeof(rightBuffer));
  } else {
    const char *prefixName = activeCalculatorPrefixName();
    if (prefixName[0] != '\0') {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "MK61 %s %s %s",
               prefixName,
               angleModeShortName(calculator.angleMode()),
               calculatorModeName());
    } else {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "MK61 %s %s",
               angleModeShortName(calculator.angleMode()),
               calculatorModeName());
    }
    formatEventLabel(rightBuffer, sizeof(rightBuffer));
  }

  display.setFont(u8g2_font_5x7_mr);
  display.drawBox(0, 0, kDisplayWidth, kStatusBarHeight);
  display.setDrawColor(0);
  display.drawStr(2, 1, leftBuffer);
  if (rightBuffer[0] != '\0') {
    drawRightAlignedText(kDisplayWidth - 2, 1, rightBuffer);
  }
  display.setDrawColor(1);
}

void drawWrappedText(int x, int y, const char *text, int maxPixelWidth, uint8_t maxLines) {
  char lineBuffer[64];
  size_t position = 0;

  for (uint8_t line = 0; line < maxLines; ++line) {
    while (text[position] == ' ') {
      ++position;
    }

    if (text[position] == '\0') {
      return;
    }

    const size_t lineStart = position;
    size_t lineEnd = lineStart;
    size_t lastSpace = static_cast<size_t>(-1);
    size_t lastFittingEnd = lineStart;

    while ((text[position] != '\0') && (text[position] != '\n')) {
      if (text[position] == ' ') {
        lastSpace = position;
      }

      const size_t nextLength = (position - lineStart) + 1;
      if (nextLength >= sizeof(lineBuffer)) {
        break;
      }

      std::memcpy(lineBuffer, text + lineStart, nextLength);
      lineBuffer[nextLength] = '\0';
      if (display.getStrWidth(lineBuffer) > maxPixelWidth) {
        break;
      }

      lastFittingEnd = position + 1;
      ++position;
    }

    if (text[position] == '\n') {
      lineEnd = position;
      ++position;
    } else if (text[position] == '\0') {
      lineEnd = position;
    } else if ((lastSpace != static_cast<size_t>(-1)) && (lastSpace >= lineStart) && (lastSpace < lastFittingEnd)) {
      lineEnd = lastSpace;
      position = lastSpace + 1;
    } else if (lastFittingEnd > lineStart) {
      lineEnd = lastFittingEnd;
      position = lastFittingEnd;
    } else {
      lineEnd = position + 1;
      ++position;
    }

    size_t lineLength = lineEnd - lineStart;
    if (lineLength >= sizeof(lineBuffer)) {
      lineLength = sizeof(lineBuffer) - 1;
    }

    std::memcpy(lineBuffer, text + lineStart, lineLength);
    lineBuffer[lineLength] = '\0';
    display.drawStr(x, y + (line * kHelpLineHeight), lineBuffer);
  }
}

void formatHelpTitle(char *buffer, size_t bufferSize) {
  if (!helpState.hasSelection) {
    snprintf(buffer, bufferSize, "Help mode");
    return;
  }

  const char *label = calculatorKeyHelpLabel(helpState.key, helpState.prefix);
  const char *prefixName = (helpState.prefix == CalculatorPrefix::F)
                               ? "F"
                               : (helpState.prefix == CalculatorPrefix::K) ? "K" : "";

  if (prefixName[0] != '\0') {
    snprintf(buffer, bufferSize, "%s %c: %s", prefixName, helpState.key, label);
  } else {
    snprintf(buffer, bufferSize, "%c: %s", helpState.key, label);
  }
}

const char *currentHelpDescription() {
  if (!helpState.hasSelection) {
    return "Press any key to see its meaning. Use k or p first to inspect F or K shifts. Press e again to leave help.";
  }

  return calculatorKeyHelpDescription(helpState.key, helpState.prefix);
}

void drawHelpScreen() {
  char titleBuffer[32];

  drawStatusBar();
  formatHelpTitle(titleBuffer, sizeof(titleBuffer));

  display.setFont(u8g2_font_5x7_mr);
  display.drawStr(0, kHelpTitleY, titleBuffer);

  display.setFont(u8g2_font_5x7_mr);
  drawWrappedText(0, kHelpBodyY, currentHelpDescription(), kHelpTextWidth, kHelpBodyLineCount);
}

void drawCalculatorScreen() {
  const CalculatorStack stack = calculator.stack();
  char xBuffer[RpnCalculator::kEntryBufferSize];
  const char *tLabel = showStackLevelNames ? "T:" : "";
  const char *zLabel = showStackLevelNames ? "Z:" : "";
  const char *yLabel = showStackLevelNames ? "Y:" : "";

  drawStatusBar();

  drawStackLine(kStackFirstY + (0 * kStackRowHeight), tLabel, stack.t);
  drawStackLine(kStackFirstY + (1 * kStackRowHeight), zLabel, stack.z);
  drawStackLine(kStackFirstY + (2 * kStackRowHeight), yLabel, stack.y);
  display.setFont(u8g2_font_6x10_mr);
  const char *xLabel = showStackLevelNames ? (calculator.isEntering() ? "X>" : "X:") : "";
  const int labelWidth = (xLabel[0] != '\0') ? display.getStrWidth(xLabel) : 0;
  const int gap = (labelWidth > 0) ? kStackValueMinGap : 0;
  const int maxValueWidth = kDisplayWidth - labelWidth - gap;
  formatXDisplayToFit(xBuffer, sizeof(xBuffer), maxValueWidth);
  drawStackTextLine(kStackFirstY + (3 * kStackRowHeight), xLabel, xBuffer);
}

}  // namespace

void setup() {
  setupDisplay();

  display.clearBuffer();
  setupEeprom();
  loadSettings();
  display.sendBuffer();
  delay(3000);
  calculator.seedRandom(static_cast<uint32_t>(micros()) ^ 0xA5A55A5Au);
}

void loop() {
  display.clearBuffer();
  updateInput();
  if (helpState.enabled) {
    drawHelpScreen();
  } else {
    drawCalculatorScreen();
  }

  display.sendBuffer();
  delay(kLoopDelayMs);
}
