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
#include <Wire.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <SparkFun_External_EEPROM.h>  // http://librarymanager/All#SparkFun_External_EEPROM

#include "CalculatorKeymap.h"
#include "Keypad.h"
#include "Mk61extDisplay.h"
#include "ProgramRecorder.h"
#include "ProgramRunner.h"
#include "ProgramVm.h"
#include "RpnCalculator.h"

#if __has_include(<hardware/clocks.h>) && __has_include(<hardware/pll.h>)
#include <hardware/clocks.h>
#include <hardware/pll.h>
#define MK61EXT_HAS_CPU_FREQUENCY_CONTROL 1
#else
#define MK61EXT_HAS_CPU_FREQUENCY_CONTROL 0
#endif

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
constexpr uint8_t kSupplySensePin = A3;

constexpr uint8_t kEepromAddress = 0x50;
constexpr uint8_t kEepromSdaPin = 14;
constexpr uint8_t kEepromSclPin = 15;
constexpr uint32_t kEepromClockHz = 400000;
constexpr uint16_t kDisplayContrast = 20;
constexpr uint16_t kLoopDelayMs = 2;
constexpr uint16_t kSleepLoopDelayMs = 100;
constexpr int kDisplayWidth = 128;
constexpr int kStatusBarHeight = 8;
constexpr int kStackFirstY = 10;
constexpr int kStackRowHeight = 14;
constexpr int kStackValueMaxPrecision = 15;
constexpr int kStackValueMinGap = 4;
constexpr int kHelpBodyY = 8;
constexpr int kHelpLineHeight = 7;
constexpr uint8_t kHelpBodyLineCount = 8;
constexpr int kHelpTextWidth = 128;
constexpr int kProgramListFirstY = 10;
constexpr int kProgramListRowHeight = 9;
constexpr uint8_t kProgramListVisibleRows = 6;
constexpr uint16_t kKeyPollIntervalMsIdle = 2;
constexpr uint16_t kKeyPollIntervalMsRun = 10;
constexpr int kSettingsFirstRowY = 10;
constexpr int kSettingsRowHeight = 7;
constexpr int kSettingsCursorBoxWidth = 6;
constexpr int kSettingsDescriptionY = 52;
constexpr int kSettingsDescriptionLineHeight = 6;
constexpr uint8_t kSettingsDescriptionLineCount = 2;
constexpr uint32_t kSettingsEepromAddress = 0;
constexpr uint32_t kSettingsEepromReservedBytes = 256;
constexpr uint32_t kProgramEepromAddress = kSettingsEepromAddress + kSettingsEepromReservedBytes;
constexpr uint16_t kProgramEepromReservedBytes = ProgramVm::kProgramCapacity;
constexpr uint32_t kSettingsMagic = 0x4D4B3631u;  // "MK61"
constexpr uint8_t kOlderSettingsVersion = 3;
constexpr uint8_t kLegacySettingsVersion = 4;
constexpr uint8_t kSettingsVersion = 5;
constexpr uint8_t kSupplyAdcResolutionBits = 12;
constexpr uint16_t kSupplyAdcMaxReading = (1u << kSupplyAdcResolutionBits) - 1u;
constexpr uint8_t kSupplySampleCount = 8;
constexpr uint32_t kSupplyRefreshMs = 10000;
constexpr uint32_t kRecentEventDisplayMs = 1500;
constexpr float kSupplyAdcReferenceVolts = 3.3f;
constexpr float kSupplySenseDividerScale = 3.0f;
constexpr float kExternalPowerDetectOnVolts = 4.35f;
constexpr float kExternalPowerDetectOffVolts = 4.20f;

struct TimeoutOption {
  uint32_t milliseconds;
  const char *label;
};

constexpr TimeoutOption kTimeoutOptions[] = {
    {0u, "OFF"},
    {5000u, "5 s"},
    {15000u, "15 s"},
    {30000u, "30 s"},
    {60000u, "1 min"},
    {120000u, "2 min"},
    {300000u, "5 min"},
    {900000u, "15 min"},
};

constexpr uint8_t kTimeoutOptionCount = sizeof(kTimeoutOptions) / sizeof(kTimeoutOptions[0]);
constexpr uint8_t kDefaultBacklightTimeoutIndex = 0;
constexpr uint8_t kDefaultSleepTimeoutIndex = 0;

constexpr TimeoutOption kProgramRunDisplayRefreshOptions[] = {
    {100u, "100 ms"},
    {250u, "250 ms"},
    {500u, "500 ms"},
    {1000u, "1 s"},
    {2000u, "2 s"},
};

constexpr uint8_t kProgramRunDisplayRefreshOptionCount =
    sizeof(kProgramRunDisplayRefreshOptions) / sizeof(kProgramRunDisplayRefreshOptions[0]);
constexpr uint8_t kDefaultProgramRunDisplayRefreshIndex = 3;

#if MK61EXT_HAS_CPU_FREQUENCY_CONTROL
struct CpuFrequencyOption {
  uint32_t frequencyHz;
  uint32_t vcoHz;
  uint8_t postDiv1;
  uint8_t postDiv2;
  const char *label;
};

constexpr CpuFrequencyOption kCpuFrequencyOptions[] = {
    {12000000u, 432000000u, 6, 6, "12 MHz"},
    {24000000u, 432000000u, 6, 3, "24 MHz"},
    {48000000u, 1152000000u, 6, 4, "48 MHz"},
    {96000000u, 1152000000u, 6, 2, "96 MHz"},
    {125000000u, 1500000000u, 6, 2, "125 MHz"},
};

constexpr uint8_t kCpuFrequencyOptionCount =
    sizeof(kCpuFrequencyOptions) / sizeof(kCpuFrequencyOptions[0]);
constexpr uint8_t kDefaultCpuFrequencyIndex = kCpuFrequencyOptionCount - 1;
#else
constexpr uint8_t kDefaultCpuFrequencyIndex = 0;
#endif

struct KeypadDiagnostics {
  char lastKey = NO_KEY;
  KeyState lastState = IDLE;
  bool hasEvent = false;
  uint32_t lastEventMillis = 0;
};

struct HelpState {
  bool enabled = false;
  bool hasSelection = false;
  char key = NO_KEY;
  CalculatorPrefix prefix = CalculatorPrefix::None;
};

struct CommandStatus {
  bool hasLast = false;
  char last[16] = {};
};

struct StoredSettings {
  uint32_t magic = kSettingsMagic;
  uint8_t version = kSettingsVersion;
  uint8_t angleMode = static_cast<uint8_t>(CalculatorAngleMode::Radians);
  uint8_t showStackLabels = 1;
  uint8_t backlightTimeoutIndex = kDefaultBacklightTimeoutIndex;
  uint8_t sleepTimeoutIndex = kDefaultSleepTimeoutIndex;
  uint8_t cpuFrequencyIndex = kDefaultCpuFrequencyIndex;
  uint16_t brightness = 0;
  uint16_t programLength = 0;
  uint8_t programRunDisplayRefreshIndex = kDefaultProgramRunDisplayRefreshIndex;
  uint8_t reserved[4] = {};
};

static_assert(kSettingsEepromReservedBytes == 256, "settings EEPROM block must be 256 bytes");
static_assert(kProgramEepromReservedBytes == 256, "program EEPROM block must be 256 bytes");
static_assert(sizeof(StoredSettings) <= kSettingsEepromReservedBytes,
              "stored settings must fit in the reserved EEPROM settings block");

struct SettingsState {
  bool active = false;
  StoredSettings originalSettings{};
  StoredSettings stagedSettings{};
  uint8_t selectedIndex = 0;
};

enum class PendingRegisterOperation : uint8_t {
  None,
  DirectRecall,
  DirectStore,
  IndirectRecall,
  IndirectStore,
};

enum class SettingsItem : uint8_t {
  Brightness = 0,
  BacklightTimeout = 1,
  SleepTimeout = 2,
  RunDisplayRefresh = 3,
  AngleMode = 4,
  StackLabels = 5,
  CpuFrequency = 6,
};

constexpr SettingsItem kSettingsItems[] = {
    SettingsItem::Brightness,
    SettingsItem::BacklightTimeout,
    SettingsItem::SleepTimeout,
    SettingsItem::RunDisplayRefresh,
    SettingsItem::AngleMode,
    SettingsItem::StackLabels,
    SettingsItem::CpuFrequency,
};

constexpr uint8_t kSettingsItemCount = sizeof(kSettingsItems) / sizeof(kSettingsItems[0]);

Keypad keypad(makeKeymap(kKeyMap), kRowPins, kColumnPins, kRowCount, kColumnCount);
TwoWire eepromWire(kEepromSdaPin, kEepromSclPin);
ExternalEEPROM eeprom;
RpnCalculator calculator;
ProgramVm programVm;
ProgramRecorder programRecorder;
ProgramRunner programRunner;
Mk61extDisplay display(
    U8G2_R0,
    kDisplayClockPin,
    kDisplayDataPin,
    kDisplayChipSelectPin,
    kDisplayDataCommandPin,
    kDisplayResetPin);

int brightness = 0;
bool showStackLevelNames = true;
bool eepromReady = false;
uint8_t backlightTimeoutIndex = kDefaultBacklightTimeoutIndex;
uint8_t sleepTimeoutIndex = kDefaultSleepTimeoutIndex;
uint8_t cpuFrequencyIndex = kDefaultCpuFrequencyIndex;
uint8_t programRunDisplayRefreshIndex = kDefaultProgramRunDisplayRefreshIndex;
bool backlightTimedOut = false;
bool displaySleeping = false;
uint32_t lastUserActivityMs = 0;
KeypadDiagnostics keypadDiagnostics;
HelpState helpState;
CommandStatus commandStatus;
SettingsState settingsState;
PendingRegisterOperation pendingRegisterOperation = PendingRegisterOperation::None;
bool programMode = false;
uint8_t programEditAddress = 0;
float supplyVoltage = NAN;
uint32_t lastSupplySampleMs = 0;
bool runningOnBattery = true;
std::array<uint8_t, ProgramVm::kProgramCapacity> savedProgramImage{};
uint16_t savedProgramLength = 0;
bool savedProgramImageValid = false;
uint32_t lastProgramRunDisplayRefreshMs = 0;
uint32_t lastKeyPollMs = 0;

StoredSettings defaultStoredSettings() {
  StoredSettings settings{};
  settings.cpuFrequencyIndex = kDefaultCpuFrequencyIndex;
#if MK61EXT_HAS_CPU_FREQUENCY_CONTROL
  const uint32_t currentFrequencyHz = clock_get_hz(clk_sys);
  uint32_t bestDelta = UINT32_MAX;

  for (uint8_t index = 0; index < kCpuFrequencyOptionCount; ++index) {
    const uint32_t optionFrequencyHz = kCpuFrequencyOptions[index].frequencyHz;
    const uint32_t delta =
        (currentFrequencyHz > optionFrequencyHz) ? (currentFrequencyHz - optionFrequencyHz)
                                                 : (optionFrequencyHz - currentFrequencyHz);
    if (delta < bestDelta) {
      bestDelta = delta;
      settings.cpuFrequencyIndex = index;
    }
  }
#endif

  return settings;
}

uint8_t wrapOptionIndex(uint8_t currentIndex, uint8_t count, bool increase) {
  if (count == 0) {
    return 0;
  }

  if (increase) {
    return static_cast<uint8_t>((currentIndex + 1) % count);
  }

  return static_cast<uint8_t>((currentIndex + count - 1) % count);
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

CalculatorAngleMode previousAngleMode(CalculatorAngleMode mode) {
  switch (mode) {
    case CalculatorAngleMode::Radians:
      return CalculatorAngleMode::Degrees;
    case CalculatorAngleMode::Gradians:
      return CalculatorAngleMode::Radians;
    case CalculatorAngleMode::Degrees:
      return CalculatorAngleMode::Gradians;
  }

  return CalculatorAngleMode::Radians;
}

uint32_t timeoutMilliseconds(uint8_t index) {
  if (index >= kTimeoutOptionCount) {
    return kTimeoutOptions[kDefaultBacklightTimeoutIndex].milliseconds;
  }

  return kTimeoutOptions[index].milliseconds;
}

uint32_t programRunDisplayRefreshMilliseconds(uint8_t index) {
  if (index >= kProgramRunDisplayRefreshOptionCount) {
    return kProgramRunDisplayRefreshOptions[kDefaultProgramRunDisplayRefreshIndex].milliseconds;
  }

  return kProgramRunDisplayRefreshOptions[index].milliseconds;
}

const char *programRunDisplayRefreshLabel(uint8_t index) {
  if (index >= kProgramRunDisplayRefreshOptionCount) {
    return kProgramRunDisplayRefreshOptions[kDefaultProgramRunDisplayRefreshIndex].label;
  }

  return kProgramRunDisplayRefreshOptions[index].label;
}

const char *timeoutLabel(uint8_t index) {
  if (index >= kTimeoutOptionCount) {
    return kTimeoutOptions[kDefaultBacklightTimeoutIndex].label;
  }

  return kTimeoutOptions[index].label;
}

#if MK61EXT_HAS_CPU_FREQUENCY_CONTROL
const char *cpuFrequencyLabel(uint8_t index) {
  if (index >= kCpuFrequencyOptionCount) {
    return kCpuFrequencyOptions[kDefaultCpuFrequencyIndex].label;
  }

  return kCpuFrequencyOptions[index].label;
}

bool applyCpuFrequencySetting(uint8_t index) {
  if (index >= kCpuFrequencyOptionCount) {
    return false;
  }

  const CpuFrequencyOption &option = kCpuFrequencyOptions[index];
  if (clock_get_hz(clk_sys) == option.frequencyHz) {
    cpuFrequencyIndex = index;
    SystemCoreClock = option.frequencyHz;
    return true;
  }

  clock_configure(clk_sys,
                  CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                  CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_USB,
                  48 * MHZ,
                  48 * MHZ);

  pll_deinit(pll_sys);
  pll_init(pll_sys, 1, option.vcoHz, option.postDiv1, option.postDiv2);

  clock_configure(clk_ref,
                  CLOCKS_CLK_REF_CTRL_SRC_VALUE_XOSC_CLKSRC,
                  0,
                  XOSC_MHZ * MHZ,
                  XOSC_MHZ * MHZ);

  clock_configure(clk_sys,
                  CLOCKS_CLK_SYS_CTRL_SRC_VALUE_CLKSRC_CLK_SYS_AUX,
                  CLOCKS_CLK_SYS_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS,
                  option.frequencyHz,
                  option.frequencyHz);

  clock_configure(clk_peri,
                  0,
                  CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLK_SYS,
                  option.frequencyHz,
                  option.frequencyHz);

  cpuFrequencyIndex = index;
  SystemCoreClock = option.frequencyHz;
  return clock_get_hz(clk_sys) == option.frequencyHz;
}
#else
const char *cpuFrequencyLabel(uint8_t) {
  return "FIXED";
}

bool applyCpuFrequencySetting(uint8_t index) {
  cpuFrequencyIndex = kDefaultCpuFrequencyIndex;
  return index == kDefaultCpuFrequencyIndex;
}
#endif

void applyBrightness();
void updatePowerSourceState();
void normalizeProgramEditAddress();
void rememberLastCommand(const char *label);
void rememberLastCommandForKey(char keyPressed, CalculatorPrefix prefix);

void noteUserActivity() {
  lastUserActivityMs = millis();
  backlightTimedOut = false;

  if (displaySleeping) {
    displaySleeping = false;
    display.setPowerSave(0);
  }

  applyBrightness();
}

bool isValidStoredSettings(const StoredSettings &settings) {
  if (settings.magic != kSettingsMagic) {
    return false;
  }

  if ((settings.version != kSettingsVersion) && (settings.version != kLegacySettingsVersion) &&
      (settings.version != kOlderSettingsVersion)) {
    return false;
  }

  if (settings.angleMode > static_cast<uint8_t>(CalculatorAngleMode::Degrees)) {
    return false;
  }

  if (settings.brightness > 256) {
    return false;
  }

  if (settings.backlightTimeoutIndex >= kTimeoutOptionCount) {
    return false;
  }

  if (settings.sleepTimeoutIndex >= kTimeoutOptionCount) {
    return false;
  }

  if ((settings.version == kSettingsVersion) &&
      (settings.programRunDisplayRefreshIndex >= kProgramRunDisplayRefreshOptionCount)) {
    return false;
  }

#if MK61EXT_HAS_CPU_FREQUENCY_CONTROL
  if (settings.cpuFrequencyIndex >= kCpuFrequencyOptionCount) {
    return false;
  }
#endif

  if ((settings.version != kOlderSettingsVersion) &&
      (settings.programLength > ProgramVm::kProgramCapacity)) {
    return false;
  }

  return true;
}

bool normalizeStoredSettings(StoredSettings &settings) {
  if (settings.version == kSettingsVersion) {
    return false;
  }

  const uint8_t previousVersion = settings.version;
  settings.version = kSettingsVersion;
  settings.programRunDisplayRefreshIndex = kDefaultProgramRunDisplayRefreshIndex;
  if (previousVersion == kOlderSettingsVersion) {
    settings.programLength = 0;
  }
  std::memset(settings.reserved, 0, sizeof(settings.reserved));
  return true;
}

StoredSettings currentStoredSettings() {
  StoredSettings settings = defaultStoredSettings();
  settings.angleMode = static_cast<uint8_t>(calculator.angleMode());
  settings.showStackLabels = showStackLevelNames ? 1 : 0;
  settings.backlightTimeoutIndex = backlightTimeoutIndex;
  settings.sleepTimeoutIndex = sleepTimeoutIndex;
  settings.cpuFrequencyIndex = cpuFrequencyIndex;
  settings.programRunDisplayRefreshIndex = programRunDisplayRefreshIndex;
  settings.brightness = static_cast<uint16_t>(brightness);
  settings.programLength = savedProgramLength;
  return settings;
}

void applyBrightness() {
  if (displaySleeping || backlightTimedOut || (brightness <= 0)) {
    analogWrite(kBacklightPin, -1);
    digitalWrite(kBacklightPin, HIGH);  // off
  } else if (brightness >= 256) {
    analogWrite(kBacklightPin, -1);
    digitalWrite(kBacklightPin, LOW);  // on
  } else {
    analogWrite(kBacklightPin, 255 - brightness);
  }
}

void setupSupplySense() {
  analogReadResolution(kSupplyAdcResolutionBits);
}

float readSupplyVoltage() {
  uint32_t rawTotal = 0;

  // On the Pico board, A3/GPIO29 is tied to VSYS through a 1:3 divider.
  for (uint8_t sample = 0; sample < kSupplySampleCount; ++sample) {
    rawTotal += static_cast<uint32_t>(analogRead(kSupplySensePin));
  }

  const float rawAverage = static_cast<float>(rawTotal) / static_cast<float>(kSupplySampleCount);
  const float sensedVoltage = (rawAverage / static_cast<float>(kSupplyAdcMaxReading)) * kSupplyAdcReferenceVolts;
  return sensedVoltage * kSupplySenseDividerScale;
}

void updateSupplyVoltage(bool force = false) {
  const uint32_t now = millis();
  if (!force && std::isfinite(supplyVoltage) && ((now - lastSupplySampleMs) < kSupplyRefreshMs)) {
    return;
  }

  supplyVoltage = readSupplyVoltage();
  lastSupplySampleMs = now;
  updatePowerSourceState();
}

void formatSupplyVoltage(char *buffer, size_t bufferSize) {
  if (!std::isfinite(supplyVoltage)) {
    snprintf(buffer, bufferSize, "--.--V");
    return;
  }

  snprintf(buffer, bufferSize, "%.2fV", static_cast<double>(supplyVoltage));
}

void updatePowerSourceState() {
  if (!std::isfinite(supplyVoltage)) {
    return;
  }

  bool nextRunningOnBattery = runningOnBattery;
  if (runningOnBattery) {
    if (supplyVoltage >= kExternalPowerDetectOnVolts) {
      nextRunningOnBattery = false;
    }
  } else if (supplyVoltage <= kExternalPowerDetectOffVolts) {
    nextRunningOnBattery = true;
  }

  if (nextRunningOnBattery == runningOnBattery) {
    return;
  }

  runningOnBattery = nextRunningOnBattery;
  noteUserActivity();
}

void saveSettings(const StoredSettings &settings) {
  if (!eepromReady) {
    return;
  }

  (void)eeprom.putChanged(kSettingsEepromAddress, settings);
}

void applyStoredSettings(const StoredSettings &settings) {
  backlightTimeoutIndex = settings.backlightTimeoutIndex;
  sleepTimeoutIndex = settings.sleepTimeoutIndex;
  programRunDisplayRefreshIndex = settings.programRunDisplayRefreshIndex;
  brightness = settings.brightness;
  showStackLevelNames = settings.showStackLabels != 0;
  calculator.setAngleMode(static_cast<CalculatorAngleMode>(settings.angleMode));
  if (!applyCpuFrequencySetting(settings.cpuFrequencyIndex)) {
    (void)applyCpuFrequencySetting(kDefaultCpuFrequencyIndex);
  }

  noteUserActivity();
}

void loadSettings() {
  StoredSettings settings;
  eeprom.get(kSettingsEepromAddress, settings);

  if (!isValidStoredSettings(settings)) {
    settings = defaultStoredSettings();
    savedProgramLength = settings.programLength;
    savedProgramImageValid = false;
    applyStoredSettings(settings);
    saveSettings(settings);
    return;
  }

  const bool migrated = normalizeStoredSettings(settings);
  savedProgramLength = settings.programLength;
  savedProgramImageValid = false;
  applyStoredSettings(settings);
  if (migrated) {
    saveSettings(settings);
  }
}

bool restoreProgramFromEeprom() {
  if (!eepromReady) {
    return false;
  }

  if (savedProgramLength > ProgramVm::kProgramCapacity) {
    programVm.reset();
    savedProgramLength = 0;
    savedProgramImageValid = false;
    return false;
  }

  savedProgramImage.fill(0);
  std::array<uint8_t, ProgramVm::kProgramCapacity> programBytes{};
  for (uint16_t index = 0; index < savedProgramLength; ++index) {
    uint8_t value = 0;
    eeprom.get(kProgramEepromAddress + index, value);
    programBytes[index] = value;
    savedProgramImage[index] = value;
  }

  if (!programVm.loadProgram(programBytes.data(), savedProgramLength)) {
    programVm.reset();
    savedProgramImage.fill(0);
    savedProgramLength = 0;
    savedProgramImageValid = false;
    saveSettings(currentStoredSettings());
    return false;
  }

  savedProgramImageValid = true;
  programEditAddress = 0;
  programRecorder.reset();
  programRunner.stop();
  programRunner.clearPausedExecution();
  programRunner.resetRunAddress();
  normalizeProgramEditAddress();
  return true;
}

bool saveProgramToEeprom() {
  if (!eepromReady) {
    return false;
  }

  if (savedProgramLength > ProgramVm::kProgramCapacity) {
    return false;
  }

  if (!savedProgramImageValid && (savedProgramLength <= ProgramVm::kProgramCapacity)) {
    savedProgramImage.fill(0);
    for (uint16_t index = 0; index < savedProgramLength; ++index) {
      uint8_t value = 0;
      eeprom.get(kProgramEepromAddress + index, value);
      savedProgramImage[index] = value;
    }
    savedProgramImageValid = true;
  }

  const uint16_t length = programVm.programLength();
  const uint16_t previousSavedLength = savedProgramLength;
  const uint16_t writeLimit = (length > previousSavedLength) ? length : previousSavedLength;
  for (uint16_t index = 0; index < writeLimit; ++index) {
    const uint8_t value = (index < length) ? programVm.programByte(index) : 0;
    if (!savedProgramImageValid || (index >= previousSavedLength) || (savedProgramImage[index] != value)) {
      (void)eeprom.putChanged(kProgramEepromAddress + index, value);
    }

    savedProgramImage[index] = value;
  }

  savedProgramLength = length;
  savedProgramImageValid = true;
  saveSettings(currentStoredSettings());
  return true;
}

uint16_t increaseBrightnessValue(uint16_t value) {
  uint16_t updated = static_cast<uint16_t>(value * 2);
  if (updated > 256) {
    updated = 256;
  }
  if (updated == 0) {
    updated = 1;
  }
  return updated;
}

uint16_t decreaseBrightnessValue(uint16_t value) {
  return static_cast<uint16_t>(value / 2);
}

const char *settingsItemName(SettingsItem item) {
  switch (item) {
    case SettingsItem::Brightness:
      return "Brightness";
    case SettingsItem::BacklightTimeout:
      return "Backlight";
    case SettingsItem::SleepTimeout:
      return "Sleep";
    case SettingsItem::RunDisplayRefresh:
      return "Run screen";
    case SettingsItem::AngleMode:
      return "Angle";
    case SettingsItem::StackLabels:
      return "Labels";
    case SettingsItem::CpuFrequency:
      return "CPU freq";
  }

  return "";
}

void formatBrightnessSetting(char *buffer, size_t bufferSize, uint16_t value) {
  if (value == 0) {
    snprintf(buffer, bufferSize, "OFF");
    return;
  }

  snprintf(buffer, bufferSize, "%u", static_cast<unsigned>(value));
}

const char *stackLabelsSettingName(uint8_t showStackLabels) {
  return showStackLabels != 0 ? "ON" : "OFF";
}

void formatSettingValue(SettingsItem item,
                        const StoredSettings &settings,
                        char *buffer,
                        size_t bufferSize) {
  switch (item) {
    case SettingsItem::Brightness:
      formatBrightnessSetting(buffer, bufferSize, settings.brightness);
      return;
    case SettingsItem::BacklightTimeout:
      snprintf(buffer, bufferSize, "%s", timeoutLabel(settings.backlightTimeoutIndex));
      return;
    case SettingsItem::SleepTimeout:
      snprintf(buffer, bufferSize, "%s", timeoutLabel(settings.sleepTimeoutIndex));
      return;
    case SettingsItem::RunDisplayRefresh:
      snprintf(buffer, bufferSize, "%s", programRunDisplayRefreshLabel(settings.programRunDisplayRefreshIndex));
      return;
    case SettingsItem::AngleMode:
      snprintf(buffer,
               bufferSize,
               "%s",
               angleModeShortName(static_cast<CalculatorAngleMode>(settings.angleMode)));
      return;
    case SettingsItem::StackLabels:
      snprintf(buffer, bufferSize, "%s", stackLabelsSettingName(settings.showStackLabels));
      return;
    case SettingsItem::CpuFrequency:
      snprintf(buffer, bufferSize, "%s", cpuFrequencyLabel(settings.cpuFrequencyIndex));
      return;
  }

  buffer[0] = '\0';
}

const char *settingsItemDescription(SettingsItem item) {
  switch (item) {
    case SettingsItem::Brightness:
      return "Set the display backlight level.";
    case SettingsItem::BacklightTimeout:
      return "Switch off the backlight after inactivity on battery.";
    case SettingsItem::SleepTimeout:
      return "Put the system into energy-saving mode after inactivity on battery.";
    case SettingsItem::RunDisplayRefresh:
      return "Set screen refresh rate while programs are running.";
    case SettingsItem::AngleMode:
      return "Choose RAD, GRD, or DEG.";
    case SettingsItem::StackLabels:
      return "Show or hide the T Z Y X labels.";
    case SettingsItem::CpuFrequency:
      return "Select the RP2040 system clock frequency.";
  }

  return "";
}

void clearHelpSelection() {
  helpState.hasSelection = false;
  helpState.key = NO_KEY;
  helpState.prefix = CalculatorPrefix::None;
}

void clearPendingRegisterOperation() {
  pendingRegisterOperation = PendingRegisterOperation::None;
}

void normalizeProgramEditAddress() {
  const uint16_t length = programVm.programLength();
  if (programEditAddress > length) {
    programEditAddress = static_cast<uint8_t>(length);
  }

  if (!programVm.isStepBoundary(programEditAddress)) {
    programEditAddress = static_cast<uint8_t>(length);
  }
}

void enterProgramMode() {
  programRunner.stop();
  programRunner.clearPausedExecution();
  programMode = true;
  programRecorder.reset();
  normalizeProgramEditAddress();
  clearPendingRegisterOperation();
  resetCalculatorKeymapState();
  clearHelpSelection();
  noteUserActivity();
}

void exitProgramMode() {
  programMode = false;
  programRecorder.reset();
  clearPendingRegisterOperation();
  resetCalculatorKeymapState();
  noteUserActivity();
}

void enterSettingsMode() {
  settingsState.active = true;
  settingsState.originalSettings = currentStoredSettings();
  settingsState.stagedSettings = settingsState.originalSettings;
  settingsState.selectedIndex = 0;
  programRecorder.reset();
  helpState.enabled = false;
  clearHelpSelection();
  clearPendingRegisterOperation();
  resetCalculatorKeymapState();
  noteUserActivity();
}

void exitSettingsMode() {
  settingsState.active = false;
  saveSettings(settingsState.stagedSettings);
  clearPendingRegisterOperation();
  resetCalculatorKeymapState();
  noteUserActivity();
}

void cancelSettingsMode() {
  applyStoredSettings(settingsState.originalSettings);
  settingsState.active = false;
  clearPendingRegisterOperation();
  resetCalculatorKeymapState();
  noteUserActivity();
}

void applyStagedSettings() {
  applyStoredSettings(settingsState.stagedSettings);
}

void selectPreviousSettingsItem() {
  settingsState.selectedIndex =
      (settingsState.selectedIndex + kSettingsItemCount - 1) % kSettingsItemCount;
}

void selectNextSettingsItem() {
  settingsState.selectedIndex = (settingsState.selectedIndex + 1) % kSettingsItemCount;
}

void adjustSelectedSetting(bool increase) {
  const SettingsItem item = kSettingsItems[settingsState.selectedIndex];

  switch (item) {
    case SettingsItem::Brightness:
      settingsState.stagedSettings.brightness = increase
                                                    ? increaseBrightnessValue(settingsState.stagedSettings.brightness)
                                                    : decreaseBrightnessValue(settingsState.stagedSettings.brightness);
      break;
    case SettingsItem::BacklightTimeout:
      settingsState.stagedSettings.backlightTimeoutIndex =
          wrapOptionIndex(settingsState.stagedSettings.backlightTimeoutIndex, kTimeoutOptionCount, increase);
      break;
    case SettingsItem::SleepTimeout:
      settingsState.stagedSettings.sleepTimeoutIndex =
          wrapOptionIndex(settingsState.stagedSettings.sleepTimeoutIndex, kTimeoutOptionCount, increase);
      break;
    case SettingsItem::RunDisplayRefresh:
      settingsState.stagedSettings.programRunDisplayRefreshIndex =
          wrapOptionIndex(settingsState.stagedSettings.programRunDisplayRefreshIndex,
                          kProgramRunDisplayRefreshOptionCount,
                          increase);
      break;
    case SettingsItem::AngleMode: {
      const CalculatorAngleMode currentMode =
          static_cast<CalculatorAngleMode>(settingsState.stagedSettings.angleMode);
      settingsState.stagedSettings.angleMode =
          static_cast<uint8_t>(increase ? nextAngleMode(currentMode) : previousAngleMode(currentMode));
      break;
    }
    case SettingsItem::StackLabels:
      settingsState.stagedSettings.showStackLabels =
          settingsState.stagedSettings.showStackLabels != 0 ? 0 : 1;
      break;
    case SettingsItem::CpuFrequency:
      settingsState.stagedSettings.cpuFrequencyIndex =
          wrapOptionIndex(settingsState.stagedSettings.cpuFrequencyIndex,
#if MK61EXT_HAS_CPU_FREQUENCY_CONTROL
                          kCpuFrequencyOptionCount,
#else
                          1,
#endif
                          increase);
      break;
  }

  applyStagedSettings();
}

void armPendingRegisterOperation(PendingRegisterOperation operation) {
  pendingRegisterOperation = operation;
  resetCalculatorKeymapState();
}

const char *pendingRegisterOperationName() {
  switch (pendingRegisterOperation) {
    case PendingRegisterOperation::DirectRecall:
      return "MX";
    case PendingRegisterOperation::DirectStore:
      return "XM";
    case PendingRegisterOperation::IndirectRecall:
      return "MXI";
    case PendingRegisterOperation::IndirectStore:
      return "XMI";
    case PendingRegisterOperation::None:
      return "";
  }

  return "";
}

const char *pendingRegisterOperationName(PendingRegisterOperation operation) {
  switch (operation) {
    case PendingRegisterOperation::DirectRecall:
      return "MX";
    case PendingRegisterOperation::DirectStore:
      return "XM";
    case PendingRegisterOperation::IndirectRecall:
      return "MXI";
    case PendingRegisterOperation::IndirectStore:
      return "XMI";
    case PendingRegisterOperation::None:
      return "";
  }

  return "";
}

char registerName(uint8_t index) {
  if (index <= 9) {
    return static_cast<char>('0' + index);
  }

  if (index <= 15) {
    return static_cast<char>('a' + (index - 10));
  }

  return '?';
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
    case 'u':
      index = 15;
      return true;
    default:
      return false;
  }
}

void setHelpMode(bool enabled) {
  helpState.enabled = enabled;
  clearHelpSelection();
  clearPendingRegisterOperation();
  programRecorder.reset();
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

  char commandBuffer[16];
  std::snprintf(commandBuffer,
                sizeof(commandBuffer),
                "%s %c",
                pendingRegisterOperationName(operation),
                registerName(registerIndex));

  switch (operation) {
    case PendingRegisterOperation::DirectRecall:
      if (calculator.recallRegister(registerIndex)) {
        rememberLastCommand(commandBuffer);
      }
      return true;
    case PendingRegisterOperation::DirectStore:
      if (calculator.storeRegister(registerIndex)) {
        rememberLastCommand(commandBuffer);
      }
      return true;
    case PendingRegisterOperation::IndirectRecall:
      if (calculator.recallIndirectRegister(registerIndex)) {
        rememberLastCommand(commandBuffer);
      }
      return true;
    case PendingRegisterOperation::IndirectStore:
      if (calculator.storeIndirectRegister(registerIndex)) {
        rememberLastCommand(commandBuffer);
      }
      return true;
    case PendingRegisterOperation::None:
      return false;
  }

  return false;
}

void handleSettingsPressedKey(char keyPressed) {
  switch (keyPressed) {
    case 'a':
      selectPreviousSettingsItem();
      rememberLastCommand("SET PREV");
      return;
    case 'b':
      selectNextSettingsItem();
      rememberLastCommand("SET NEXT");
      return;
    case 'c':
      adjustSelectedSetting(false);
      rememberLastCommand("SET -");
      return;
    case 'd':
      adjustSelectedSetting(true);
      rememberLastCommand("SET +");
      return;
    case 'e':
      exitSettingsMode();
      rememberLastCommand("SET SAVE");
      return;
    case 'f':
      cancelSettingsMode();
      rememberLastCommand("SET ESC");
      return;
    default:
      return;
  }
}

void handleProgramPressedKey(char keyPressed) {
  if ((keyPressed == 'x') && !programRecorder.hasPendingOperand() &&
      (std::strcmp(programRecorder.activePrefixName(), "F") == 0)) {
    exitProgramMode();
    rememberLastCommand("RUN");
    return;
  }

  if (!programRecorder.hasPendingOperand() && (programRecorder.activePrefixName()[0] == '\0')) {
    if (keyPressed == 'g') {
      (void)saveProgramToEeprom();
      rememberLastCommand("SAVE PRG");
      return;
    }

    if (keyPressed == 'h') {
      (void)restoreProgramFromEeprom();
      rememberLastCommand("LOAD PRG");
      return;
    }
  }

  (void)programRecorder.handleKey(programVm, programEditAddress, keyPressed);
}

void handlePressedKey(char keyPressed) {
  const bool runControlContext =
      (pendingRegisterOperation == PendingRegisterOperation::None) &&
      (activeCalculatorPrefix() == CalculatorPrefix::None);

  if (settingsState.active) {
    handleSettingsPressedKey(keyPressed);
    return;
  }

  if (programRunner.isRunning()) {
    if (runControlContext && (keyPressed == 'o')) {
      programRunner.stop();
      rememberLastCommand("STOP");
    }
    return;
  }

  if (keyPressed == 'f') {
    toggleHelpMode();
    rememberLastCommand(helpState.enabled ? "HELP ON" : "HELP OFF");
    return;
  }

  if (helpState.enabled) {
    handleHelpPressedKey(keyPressed);
    return;
  }

  if (programMode) {
    handleProgramPressedKey(keyPressed);
    return;
  }

  if ((activeCalculatorPrefix() == CalculatorPrefix::F) && (keyPressed == 'y')) {
    enterProgramMode();
    rememberLastCommand("PRG");
    return;
  }

  if (keyPressed == 'e') {
    enterSettingsMode();
    rememberLastCommand("SETTINGS");
    return;
  }

  if (runControlContext && (keyPressed == 'o')) {
    (void)programRunner.start(programVm, calculator);
    if (programRunner.isRunning()) {
      rememberLastCommand("RUN");
    }
    return;
  }

  if (runControlContext && (keyPressed == 'n')) {
    programRunner.resetRunAddress();
    rememberLastCommand("PC 00");
    return;
  }

  if (runControlContext && (keyPressed == 't')) {
    if (programRunner.singleStep(programVm, calculator)) {
      rememberLastCommand("SST");
    }
    return;
  }

  if (runControlContext && (keyPressed == 'g')) {
    (void)saveProgramToEeprom();
    rememberLastCommand("SAVE PRG");
    return;
  }

  if (runControlContext && (keyPressed == 'h')) {
    (void)restoreProgramFromEeprom();
    rememberLastCommand("LOAD PRG");
    return;
  }

  programRunner.clearError();

  if (handleRegisterOperationKey(keyPressed)) {
    programRunner.clearPausedExecution();
    return;
  }

  const CalculatorPrefix prefixBefore = activeCalculatorPrefix();
  const CalculatorAction action = translateKeyToCalculatorAction(keyPressed);
  if (action != CalculatorAction::None) {
    programRunner.clearPausedExecution();
    rememberLastCommandForKey(keyPressed, prefixBefore);
  }
  calculator.apply(action);
}

void updateInput() {
  const uint32_t now = millis();
  const uint32_t keyPollIntervalMs = programRunner.isRunning() ? kKeyPollIntervalMsRun : kKeyPollIntervalMsIdle;
  if ((lastKeyPollMs != 0) && ((now - lastKeyPollMs) < keyPollIntervalMs)) {
    return;
  }
  lastKeyPollMs = now;

  const bool keyActivity = keypad.getKeys();
  std::array<char, kRowCount * kColumnCount> pressedKeys{};
  size_t pressedKeyCount = 0;

  for (byte index = 0; index < keypad.numKeys(); index++) {
    const Key &key = keypad.key[index];

    if (!keyActivity) {
      continue;
    }

    if (key.stateChanged) {
      keypadDiagnostics.lastKey = key.kchar;
      keypadDiagnostics.lastState = key.kstate;
      keypadDiagnostics.hasEvent = true;
      keypadDiagnostics.lastEventMillis = millis();

      if ((key.kstate == PRESSED) && (pressedKeyCount < pressedKeys.size())) {
        pressedKeys[pressedKeyCount++] = key.kchar;
      }
    }
  }

  if (pressedKeyCount == 0) {
    return;
  }

  const bool wasSleeping = displaySleeping;
  noteUserActivity();
  if (wasSleeping) {
    return;
  }

  for (size_t index = 0; index < pressedKeyCount; ++index) {
    const char key = pressedKeys[index];
    if ((key == 'k') || (key == 'p')) {
      handlePressedKey(key);
    }
  }

  for (size_t index = 0; index < pressedKeyCount; ++index) {
    const char key = pressedKeys[index];
    if ((key != 'k') && (key != 'p')) {
      handlePressedKey(key);
    }
  }
}

void updateProgramExecution() {
  if (!programRunner.isRunning()) {
    return;
  }

  const uint32_t refreshMs = programRunDisplayRefreshMilliseconds(programRunDisplayRefreshIndex);
  while (programRunner.isRunning()) {
    (void)programRunner.step(programVm, calculator);

    if (!programRunner.isRunning()) {
      break;
    }

    uint32_t now = millis();
    if ((lastKeyPollMs == 0) || (static_cast<uint32_t>(now - lastKeyPollMs) >= kKeyPollIntervalMsRun)) {
      updateInput();
      if (!programRunner.isRunning()) {
        break;
      }
      now = millis();
    }

    if ((refreshMs > 0) &&
        ((lastProgramRunDisplayRefreshMs == 0) ||
         (static_cast<uint32_t>(now - lastProgramRunDisplayRefreshMs) >= refreshMs))) {
      break;
    }
  }

  lastUserActivityMs = millis();
}

void updateIdlePowerState() {
  if (settingsState.active) {
    return;
  }

  if (programRunner.isRunning()) {
    return;
  }

  if (!runningOnBattery) {
    return;
  }

  const uint32_t idleMs = millis() - lastUserActivityMs;
  const uint32_t sleepTimeoutMs = timeoutMilliseconds(sleepTimeoutIndex);
  if (!displaySleeping && (sleepTimeoutMs > 0) && (idleMs >= sleepTimeoutMs)) {
    displaySleeping = true;
    backlightTimedOut = true;
    display.setPowerSave(1);
    applyBrightness();
    return;
  }

  const uint32_t backlightTimeoutMs = timeoutMilliseconds(backlightTimeoutIndex);
  if (!displaySleeping && !backlightTimedOut && (backlightTimeoutMs > 0) &&
      (idleMs >= backlightTimeoutMs)) {
    backlightTimedOut = true;
    applyBrightness();
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

  return "CALC";
}

const char *programRecorderErrorShortName(ProgramRecorderError error) {
  switch (error) {
    case ProgramRecorderError::None:
      return "";
    case ProgramRecorderError::InvalidKey:
      return "KEY";
    case ProgramRecorderError::InvalidOperand:
      return "OPR";
    case ProgramRecorderError::CommitFailed:
      return "MEM";
  }

  return "ERR";
}

void rememberLastCommand(const char *label) {
  if ((label == nullptr) || (label[0] == '\0')) {
    return;
  }

  commandStatus.hasLast = true;
  std::snprintf(commandStatus.last, sizeof(commandStatus.last), "%s", label);
}

const char *commandLabelForKey(char keyPressed, CalculatorPrefix prefix) {
  const CalculatorKeyAssignment *assignment = plannedCalculatorKeyAssignment(keyPressed);
  if (assignment == nullptr) {
    return nullptr;
  }

  switch (prefix) {
    case CalculatorPrefix::F:
      return assignment->fShifted;
    case CalculatorPrefix::K:
      return assignment->kShifted;
    case CalculatorPrefix::None:
      return assignment->primary;
  }

  return nullptr;
}

void rememberLastCommandForKey(char keyPressed, CalculatorPrefix prefix) {
  const char *label = commandLabelForKey(keyPressed, prefix);
  if ((label == nullptr) || (label[0] == '\0')) {
    return;
  }

  rememberLastCommand(label);
}

bool formatCommandProgress(char *buffer, size_t bufferSize) {
  if (pendingRegisterOperation != PendingRegisterOperation::None) {
    std::snprintf(buffer, bufferSize, "IN %s ?", pendingRegisterOperationName());
    return true;
  }

  const char *prefixName = activeCalculatorPrefixName();
  if (prefixName[0] != '\0') {
    std::snprintf(buffer, bufferSize, "IN %s ?", prefixName);
    return true;
  }

  return false;
}

void formatStatusRightText(char *buffer, size_t bufferSize) {
  if (formatCommandProgress(buffer, bufferSize)) {
    return;
  }

  if (commandStatus.hasLast) {
    std::snprintf(buffer, bufferSize, "CMD %.12s", commandStatus.last);
    return;
  }

  formatSupplyVoltage(buffer, bufferSize);
}

void drawStatusBar() {
  char leftBuffer[32];
  char rightBuffer[18];
  rightBuffer[0] = '\0';
  const unsigned runAddress = static_cast<unsigned>(programRunner.runAddress());

  if (settingsState.active) {
    snprintf(leftBuffer, sizeof(leftBuffer), "SET");
    snprintf(rightBuffer, sizeof(rightBuffer), "e OK f ESC");
  } else if (helpState.enabled) {
    if (helpState.hasSelection) {
      const char *label = calculatorKeyHelpLabel(helpState.key, helpState.prefix);
      snprintf(leftBuffer, sizeof(leftBuffer), "HELP: %s", label);
    } else {
      snprintf(leftBuffer, sizeof(leftBuffer), "HELP");
    }
  } else if (programMode) {
    const char *recorderErrorShort = programRecorderErrorShortName(programRecorder.error());
    const char *recorderPrefix = programRecorder.activePrefixName();
    const char *editModeShort = programRecorder.insertModeEnabled() ? "INS" : "OVR";
    const unsigned editAddress = static_cast<unsigned>(programEditAddress);
    const unsigned programLength = static_cast<unsigned>(programVm.programLength());
    if (recorderErrorShort[0] != '\0') {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "P%02X L%02X %s E:%s",
               editAddress,
               programLength,
               editModeShort,
               recorderErrorShort);
    } else if (programRecorder.hasPendingOperand()) {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "P%02X L%02X %s PND",
               editAddress,
               programLength,
               editModeShort);
    } else if (recorderPrefix[0] != '\0') {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "P%02X L%02X %s %s",
               editAddress,
               programLength,
               editModeShort,
               recorderPrefix);
    } else {
      snprintf(leftBuffer, sizeof(leftBuffer), "P%02X L%02X %s", editAddress, programLength, editModeShort);
    }
    formatStatusRightText(rightBuffer, sizeof(rightBuffer));
  } else if (programRunner.hasError()) {
    snprintf(leftBuffer,
             sizeof(leftBuffer),
             "PC%02X VM %s",
             runAddress,
             programRunnerErrorShortName(programRunner.error()));
    formatStatusRightText(rightBuffer, sizeof(rightBuffer));
  } else if (programRunner.isRunning()) {
    snprintf(leftBuffer, sizeof(leftBuffer), "PC%02X RUN", runAddress);
    formatStatusRightText(rightBuffer, sizeof(rightBuffer));
  } else if (calculator.hasError()) {
    snprintf(leftBuffer, sizeof(leftBuffer), "PC%02X ERR %s", runAddress, calculator.errorMessage());
    formatStatusRightText(rightBuffer, sizeof(rightBuffer));
  } else if (pendingRegisterOperation != PendingRegisterOperation::None) {
    snprintf(leftBuffer,
             sizeof(leftBuffer),
             "PC%02X %s %s",
             runAddress,
             angleModeShortName(calculator.angleMode()),
             pendingRegisterOperationName());
    formatStatusRightText(rightBuffer, sizeof(rightBuffer));
  } else {
    const char *prefixName = activeCalculatorPrefixName();
    if (prefixName[0] != '\0') {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "PC%02X %s %s %s",
               runAddress,
               prefixName,
               angleModeShortName(calculator.angleMode()),
               calculatorModeName());
    } else {
      snprintf(leftBuffer,
               sizeof(leftBuffer),
               "PC%02X %s %s",
               runAddress,
               angleModeShortName(calculator.angleMode()),
               calculatorModeName());
    }
    formatStatusRightText(rightBuffer, sizeof(rightBuffer));
  }

  display.setFont(u8g2_font_5x7_mr);
  display.drawBox(0, 0, kDisplayWidth, kStatusBarHeight);
  display.setDrawColor(0);
  display.drawStr(2, 1, leftBuffer);
  if ((rightBuffer[0] != '\0') &&
      ((display.getStrWidth(leftBuffer) + display.getStrWidth(rightBuffer) + 6) < kDisplayWidth)) {
    drawRightAlignedText(kDisplayWidth - 2, 1, rightBuffer);
  }
  display.setDrawColor(1);
}

void drawWrappedText(int x,
                     int y,
                     const char *text,
                     int maxPixelWidth,
                     uint8_t maxLines,
                     int lineHeight) {
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
    display.drawStr(x, y + (line * lineHeight), lineBuffer);
  }
}

void drawCharacterWrappedText(int x,
                              int y,
                              const char *text,
                              int maxPixelWidth,
                              uint8_t maxLines,
                              int lineHeight) {
  if (text == nullptr) {
    return;
  }

  char lineBuffer[64];
  size_t position = 0;

  for (uint8_t line = 0; line < maxLines; ++line) {
    if (text[position] == '\0') {
      return;
    }

    if (text[position] == '\n') {
      ++position;
      continue;
    }

    const size_t lineStart = position;
    size_t lineEnd = lineStart;

    while ((text[position] != '\0') && (text[position] != '\n')) {
      const size_t nextLength = (position - lineStart) + 1;
      if (nextLength >= sizeof(lineBuffer)) {
        break;
      }

      std::memcpy(lineBuffer, text + lineStart, nextLength);
      lineBuffer[nextLength] = '\0';
      if (display.getStrWidth(lineBuffer) > maxPixelWidth) {
        break;
      }

      lineEnd = position + 1;
      ++position;
    }

    if (text[position] == '\n') {
      lineEnd = position;
      ++position;
    } else if (lineEnd == lineStart) {
      lineEnd = lineStart + 1;
      position = lineEnd;
    }

    size_t lineLength = lineEnd - lineStart;
    if (lineLength >= sizeof(lineBuffer)) {
      lineLength = sizeof(lineBuffer) - 1;
    }

    std::memcpy(lineBuffer, text + lineStart, lineLength);
    lineBuffer[lineLength] = '\0';
    display.drawStr(x, y + (line * lineHeight), lineBuffer);
  }
}

const char *currentHelpDescription() {
  if (!helpState.hasSelection) {
    return "Press any key to see its meaning. Use k or p first to inspect F or K shifts. Press f again to leave help.";
  }

  return calculatorKeyHelpDescription(helpState.key, helpState.prefix);
}

void drawHelpScreen() {
  drawStatusBar();
  display.setFont(u8g2_font_5x7_mr);
  drawCharacterWrappedText(
      0, kHelpBodyY, currentHelpDescription(), kHelpTextWidth, kHelpBodyLineCount, kHelpLineHeight);
}

void drawSettingsRow(int y, bool selected, uint8_t index, const char *label, const char *value) {
  char leftBuffer[32];

  if (selected) {
    display.drawBox(0, y, kSettingsCursorBoxWidth, 7);
    display.setDrawColor(0);
    display.drawStr(1, y, ">");
    display.setDrawColor(1);
  }

  snprintf(leftBuffer, sizeof(leftBuffer), "%u. %s", static_cast<unsigned>(index + 1), label);
  display.drawStr(kSettingsCursorBoxWidth + 2, y, leftBuffer);
  drawRightAlignedText(kDisplayWidth - 1, y, value);
}

void drawSettingsScreen() {
  char valueBuffer[16];

  drawStatusBar();
  display.setFont(u8g2_font_5x7_mr);
  for (uint8_t index = 0; index < kSettingsItemCount; ++index) {
    const SettingsItem item = kSettingsItems[index];
    formatSettingValue(item, settingsState.stagedSettings, valueBuffer, sizeof(valueBuffer));
    drawSettingsRow(kSettingsFirstRowY + (index * kSettingsRowHeight),
                    settingsState.selectedIndex == index,
                    index,
                    settingsItemName(item),
                    valueBuffer);
  }

  display.setFont(u8g2_font_4x6_mr);
  const int settingsRowsBottom = kSettingsFirstRowY + ((kSettingsItemCount - 1) * kSettingsRowHeight) + 7;
  if ((settingsRowsBottom + kSettingsDescriptionLineHeight) <= 64) {
    drawWrappedText(0,
                    kSettingsDescriptionY,
                    settingsItemDescription(kSettingsItems[settingsState.selectedIndex]),
                    kHelpTextWidth,
                    kSettingsDescriptionLineCount,
                    kSettingsDescriptionLineHeight);
  }
}

void drawProgramScreen() {
  std::array<uint16_t, ProgramVm::kProgramCapacity + 1> listingAddresses{};
  uint16_t listingCount = 0;
  uint16_t cursor = 0;
  const uint16_t programLength = programVm.programLength();

  normalizeProgramEditAddress();
  cursor = 0;

  while ((cursor < programLength) && (listingCount < ProgramVm::kProgramCapacity)) {
    listingAddresses[listingCount++] = cursor;

    const ProgramVm::DecodedStep step = programVm.decodeAt(static_cast<uint8_t>(cursor));
    const uint16_t stepWidth = (step.width > 0) ? step.width : 1;
    const uint16_t next = cursor + stepWidth;
    if ((next <= cursor) || (next > programLength)) {
      cursor = programLength;
    } else {
      cursor = next;
    }
  }

  listingAddresses[listingCount++] = programLength;

  uint16_t cursorIndex = listingCount - 1;
  for (uint16_t index = 0; index < listingCount; ++index) {
    if (listingAddresses[index] == static_cast<uint16_t>(programEditAddress)) {
      cursorIndex = index;
      break;
    }
  }

  uint16_t firstVisible = 0;
  if (listingCount > static_cast<uint16_t>(kProgramListVisibleRows)) {
    const uint16_t halfWindow = static_cast<uint16_t>(kProgramListVisibleRows / 2);
    if (cursorIndex > halfWindow) {
      firstVisible = cursorIndex - halfWindow;
    }

    const uint16_t maxFirstVisible = listingCount - kProgramListVisibleRows;
    if (firstVisible > maxFirstVisible) {
      firstVisible = maxFirstVisible;
    }
  }

  drawStatusBar();

  display.setFont(u8g2_font_5x7_mr);
  for (uint8_t row = 0; row < kProgramListVisibleRows; ++row) {
    const uint16_t lineIndex = firstVisible + row;
    if (lineIndex >= listingCount) {
      break;
    }

    const uint16_t lineAddress = listingAddresses[lineIndex];
    const bool isCursorLine = lineAddress == static_cast<uint16_t>(programEditAddress);

    char listingBuffer[64];
    if (lineAddress == programLength) {
      snprintf(listingBuffer, sizeof(listingBuffer), "%02X: --      END", static_cast<unsigned>(lineAddress));
    } else if (!programVm.formatListingLine(static_cast<uint8_t>(lineAddress),
                                            listingBuffer,
                                            sizeof(listingBuffer))) {
      snprintf(listingBuffer, sizeof(listingBuffer), "%02X: --      END", static_cast<unsigned>(lineAddress));
    }

    char rowBuffer[72];
    snprintf(rowBuffer, sizeof(rowBuffer), "%c%s", isCursorLine ? '>' : ' ', listingBuffer);
    display.drawStr(0, kProgramListFirstY + (row * kProgramListRowHeight), rowBuffer);
  }
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
  setupSupplySense();

  display.clearBuffer();
  setupEeprom();
  loadSettings();
  updateSupplyVoltage(true);
  display.sendBuffer();
  delay(3000);
  calculator.seedRandom(static_cast<uint32_t>(micros()) ^ 0xA5A55A5Au);
}

void loop() {
  updateInput();
  updateProgramExecution();
  updateSupplyVoltage();
  updateIdlePowerState();
  bool shouldRefreshDisplay = true;
  if (programRunner.isRunning()) {
    const uint32_t now = millis();
    const uint32_t refreshMs = programRunDisplayRefreshMilliseconds(programRunDisplayRefreshIndex);
    if ((refreshMs > 0) && ((now - lastProgramRunDisplayRefreshMs) < refreshMs)) {
      shouldRefreshDisplay = false;
    } else {
      lastProgramRunDisplayRefreshMs = now;
    }
  } else {
    lastProgramRunDisplayRefreshMs = 0;
  }

  if (shouldRefreshDisplay) {
    display.clearBuffer();
    if (settingsState.active) {
      drawSettingsScreen();
    } else if (helpState.enabled) {
      drawHelpScreen();
    } else if (programMode) {
      drawProgramScreen();
    } else {
      drawCalculatorScreen();
    }

    display.sendBuffer();
  }
  if (!programRunner.isRunning()) {
    delay(displaySleeping ? kSleepLoopDelayMs : kLoopDelayMs);
  }
}
