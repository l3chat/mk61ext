#include "Mk61extDisplay.h"

#include <Arduino.h>
#include <SPI.h>

#if __has_include(<SoftwareSPI.h>)
#include <SoftwareSPI.h>
#define MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI 1
#else
#define MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI 0
#endif

#if __has_include(<hardware/clocks.h>) && __has_include(<hardware/pio.h>) && \
    __has_include(<hardware/pio_instructions.h>)
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/pio_instructions.h>
#define MK61EXT_HAS_PIO_LCD_TRANSPORT 1
#else
#define MK61EXT_HAS_PIO_LCD_TRANSPORT 0
#endif

namespace {

struct DisplayTransportState {
  bool initialized = false;
  uint8_t clockPin = U8X8_PIN_NONE;
  uint8_t dataPin = U8X8_PIN_NONE;
  uint8_t csPin = U8X8_PIN_NONE;
  uint8_t dcPin = U8X8_PIN_NONE;
  uint8_t resetPin = U8X8_PIN_NONE;

#if MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI
  SoftwareSPI *softwareSpi = nullptr;
#endif

#if MK61EXT_HAS_PIO_LCD_TRANSPORT
  bool pioReady = false;
  PIO pio = nullptr;
  uint sm = 0;
  uint programOffset = 0;
  uint32_t configuredBitClockHz = 0;
  uint32_t configuredSysClockHz = 0;
#endif
};

DisplayTransportState g_transport;

void cachePins(u8x8_t *u8x8) {
  g_transport.clockPin = u8x8->pins[U8X8_PIN_SPI_CLOCK];
  g_transport.dataPin = u8x8->pins[U8X8_PIN_SPI_DATA];
  g_transport.csPin = u8x8->pins[U8X8_PIN_CS];
  g_transport.dcPin = u8x8->pins[U8X8_PIN_DC];
  g_transport.resetPin = u8x8->pins[U8X8_PIN_RESET];
}

void setChipSelect(u8x8_t *u8x8, uint8_t level) {
  if (g_transport.csPin != U8X8_PIN_NONE) {
    digitalWrite(g_transport.csPin, level ? HIGH : LOW);
    return;
  }

  u8x8_gpio_SetCS(u8x8, level);
}

void setDataCommand(u8x8_t *u8x8, uint8_t level) {
  if (g_transport.dcPin != U8X8_PIN_NONE) {
    digitalWrite(g_transport.dcPin, level ? HIGH : LOW);
    return;
  }

  u8x8_gpio_SetDC(u8x8, level);
}

#if MK61EXT_HAS_PIO_LCD_TRANSPORT

constexpr uint8_t kPioProgramLength = 5;
constexpr uint8_t kPioWrapTarget = 0;
constexpr uint8_t kPioWrap = kPioProgramLength - 1;
constexpr uint8_t kPioBitsPerByte = 8;
constexpr uint8_t kPioCyclesPerBit = 2;
constexpr uint32_t kPioMaxStableBitClockHz = 1000000u;

const uint16_t kSt7565PioProgramInstructions[kPioProgramLength] = {
    static_cast<uint16_t>(pio_encode_pull(false, true) | pio_encode_sideset(1, 0)),
    static_cast<uint16_t>(pio_encode_set(pio_x, kPioBitsPerByte - 1) | pio_encode_sideset(1, 0)),
    static_cast<uint16_t>(pio_encode_out(pio_pins, 1) | pio_encode_sideset(1, 0)),
    static_cast<uint16_t>(pio_encode_nop() | pio_encode_sideset(1, 1)),
    static_cast<uint16_t>(pio_encode_jmp_x_dec(2) | pio_encode_sideset(1, 0)),
};

const pio_program_t kSt7565PioProgram = {
    kSt7565PioProgramInstructions,
    kPioProgramLength,
    -1,
};

uint32_t transportPinMask() {
  return (1u << g_transport.clockPin) | (1u << g_transport.dataPin);
}

pio_sm_config st7565PioProgramConfig(uint programOffset, uint dataPin, uint clockPin) {
  pio_sm_config config = pio_get_default_sm_config();
  sm_config_set_wrap(&config, programOffset + kPioWrapTarget, programOffset + kPioWrap);
  sm_config_set_out_pins(&config, dataPin, 1);
  sm_config_set_sideset_pins(&config, clockPin);
  sm_config_set_sideset(&config, 1, false, false);
  sm_config_set_out_shift(&config, false, false, 32);
  sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
  return config;
}

bool initializePioTransport() {
  const PIO candidates[] = {pio0, pio1};
  for (PIO candidate : candidates) {
    if (!pio_can_add_program(candidate, &kSt7565PioProgram)) {
      continue;
    }

    const int claimedSm = pio_claim_unused_sm(candidate, false);
    if (claimedSm < 0) {
      continue;
    }

    g_transport.pio = candidate;
    g_transport.sm = static_cast<uint>(claimedSm);
    g_transport.programOffset = pio_add_program(candidate, &kSt7565PioProgram);

    pio_gpio_init(candidate, g_transport.dataPin);
    pio_gpio_init(candidate, g_transport.clockPin);

    const pio_sm_config config =
        st7565PioProgramConfig(g_transport.programOffset, g_transport.dataPin, g_transport.clockPin);
    pio_sm_init(candidate, g_transport.sm, g_transport.programOffset, &config);
    pio_sm_set_pindirs_with_mask(candidate, g_transport.sm, transportPinMask(), transportPinMask());
    pio_sm_set_pins_with_mask(candidate, g_transport.sm, 0u, transportPinMask());
    pio_sm_clear_fifos(candidate, g_transport.sm);
    pio_sm_restart(candidate, g_transport.sm);
    pio_sm_clkdiv_restart(candidate, g_transport.sm);
    pio_sm_set_enabled(candidate, g_transport.sm, true);

    g_transport.pioReady = true;
    return true;
  }

  return false;
}

void updatePioBitClock(uint32_t requestedBitClockHz) {
  if (!g_transport.pioReady) {
    return;
  }

  if (requestedBitClockHz == 0) {
    requestedBitClockHz = 1;
  }

  const uint32_t systemClockHz = clock_get_hz(clk_sys);
  const uint32_t maxBitClockHz = (systemClockHz >= kPioCyclesPerBit) ? (systemClockHz / kPioCyclesPerBit) : 1u;
  const uint32_t effectiveBitClockHz = min(min(requestedBitClockHz, kPioMaxStableBitClockHz), maxBitClockHz);
  if ((g_transport.configuredBitClockHz == effectiveBitClockHz) &&
      (g_transport.configuredSysClockHz == systemClockHz)) {
    return;
  }

  const float divider =
      static_cast<float>(systemClockHz) / (static_cast<float>(effectiveBitClockHz) * kPioCyclesPerBit);
  uint16_t dividerInt = 1;
  uint8_t dividerFrac = 0;
  pio_calculate_clkdiv_from_float(max(divider, 1.0f), &dividerInt, &dividerFrac);
  pio_sm_set_clkdiv_int_frac(g_transport.pio, g_transport.sm, dividerInt, dividerFrac);
  g_transport.configuredBitClockHz = effectiveBitClockHz;
  g_transport.configuredSysClockHz = systemClockHz;
}

void sendPioBytes(const uint8_t *data, uint8_t count) {
  while (count > 0) {
    pio_sm_put_blocking(g_transport.pio, g_transport.sm, static_cast<uint32_t>(*data) << 24);
    ++data;
    --count;
  }
}

void waitForPioTransferComplete() {
  if (!g_transport.pioReady) {
    return;
  }

  while (!pio_sm_is_tx_fifo_empty(g_transport.pio, g_transport.sm)) {
    tight_loop_contents();
  }

  if (g_transport.configuredBitClockHz == 0) {
    return;
  }

  const uint32_t drainDelayUs = (((kPioBitsPerByte + 2u) * 1000000u) + g_transport.configuredBitClockHz - 1u) /
                                g_transport.configuredBitClockHz;
  delayMicroseconds(max<uint32_t>(2u, drainDelayUs + 2u));
}

#endif

bool initializeTransport(u8x8_t *u8x8) {
  if (g_transport.initialized) {
    return true;
  }

  cachePins(u8x8);
  if (u8x8->bus_clock == 0) {
    u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
  }

  if (g_transport.csPin != U8X8_PIN_NONE) {
    pinMode(g_transport.csPin, OUTPUT);
  }
  if (g_transport.dcPin != U8X8_PIN_NONE) {
    pinMode(g_transport.dcPin, OUTPUT);
  }
  if (g_transport.resetPin != U8X8_PIN_NONE) {
    pinMode(g_transport.resetPin, OUTPUT);
    digitalWrite(g_transport.resetPin, HIGH);
  }

  setDataCommand(u8x8, 0);
  setChipSelect(u8x8, u8x8->display_info->chip_disable_level);

#if MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI
  // Prefer Arduino-Pico SoftwareSPI when the core provides it.
  g_transport.softwareSpi = new SoftwareSPI(g_transport.clockPin, NOPIN, g_transport.dataPin);
  if (g_transport.softwareSpi == nullptr) {
    return false;
  }
  g_transport.softwareSpi->begin();
  g_transport.initialized = true;
  return true;
#elif MK61EXT_HAS_PIO_LCD_TRANSPORT
  if (!initializePioTransport()) {
    return false;
  }
  updatePioBitClock(u8x8->bus_clock);
  g_transport.initialized = true;
  return true;
#else
  (void)u8x8;
  return false;
#endif
}

extern "C" uint8_t u8x8_byte_mk61ext_spi_4wire(u8x8_t *u8x8,
                                               uint8_t msg,
                                               uint8_t arg_int,
                                               void *arg_ptr) {
  switch (msg) {
    case U8X8_MSG_BYTE_INIT:
      return initializeTransport(u8x8) ? 1 : 0;

    case U8X8_MSG_BYTE_SET_DC:
      if (!initializeTransport(u8x8)) {
        return 0;
      }
      setDataCommand(u8x8, arg_int);
      return 1;

    case U8X8_MSG_BYTE_START_TRANSFER:
      if (!initializeTransport(u8x8)) {
        return 0;
      }
#if MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI
      g_transport.softwareSpi->beginTransaction(SPISettings(u8x8->bus_clock, MSBFIRST, SPI_MODE0));
#elif MK61EXT_HAS_PIO_LCD_TRANSPORT
      updatePioBitClock(u8x8->bus_clock);
#endif
      setChipSelect(u8x8, u8x8->display_info->chip_enable_level);
      u8x8->gpio_and_delay_cb(
          u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, nullptr);
      return 1;

    case U8X8_MSG_BYTE_SEND:
      if (!initializeTransport(u8x8)) {
        return 0;
      }
#if MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI
      for (uint8_t index = 0; index < arg_int; ++index) {
        g_transport.softwareSpi->transfer(static_cast<const uint8_t *>(arg_ptr)[index]);
      }
#elif MK61EXT_HAS_PIO_LCD_TRANSPORT
      sendPioBytes(static_cast<const uint8_t *>(arg_ptr), arg_int);
#endif
      return 1;

    case U8X8_MSG_BYTE_END_TRANSFER:
      if (!initializeTransport(u8x8)) {
        return 0;
      }
#if MK61EXT_HAS_PIO_LCD_TRANSPORT && !MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI
      waitForPioTransferComplete();
#endif
      u8x8->gpio_and_delay_cb(
          u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, nullptr);
      setChipSelect(u8x8, u8x8->display_info->chip_disable_level);
#if MK61EXT_HAS_ARDUINO_PICO_SOFTWARE_SPI
      g_transport.softwareSpi->endTransaction();
#endif
      return 1;

    default:
      return 0;
  }
}

}  // namespace

Mk61extDisplay::Mk61extDisplay(const u8g2_cb_t *rotation,
                               uint8_t clock,
                               uint8_t data,
                               uint8_t cs,
                               uint8_t dc,
                               uint8_t reset)
    : U8G2() {
  u8g2_Setup_st7565_erc12864_f(&u8g2, rotation, u8x8_byte_mk61ext_spi_4wire, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_4Wire_SW_SPI(getU8x8(), clock, data, cs, dc, reset);
}

