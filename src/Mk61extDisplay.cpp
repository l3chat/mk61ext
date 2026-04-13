#include "Mk61extDisplay.h"

#include <Arduino.h>

#if __has_include(<hardware/clocks.h>) && __has_include(<hardware/pio.h>) && \
    __has_include(<hardware/pio_instructions.h>)
#include <hardware/clocks.h>
#include <hardware/pio.h>
#include <hardware/pio_instructions.h>
#define MK61EXT_HAS_PIO_LCD_TRANSPORT 1
#else
#define MK61EXT_HAS_PIO_LCD_TRANSPORT 0
#endif

#define MK61EXT_ENABLE_PIO_LCD_TRANSPORT 0

namespace {

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

struct PioLcdTransport {
  bool initialized = false;
  bool available = false;
  PIO pio = nullptr;
  uint sm = 0;
  uint programOffset = 0;
  uint8_t clockPin = U8X8_PIN_NONE;
  uint8_t dataPin = U8X8_PIN_NONE;
  uint32_t requestedBitClockHz = 0;
  uint32_t configuredBitClockHz = 0;
  uint32_t configuredSysClockHz = 0;
};

PioLcdTransport g_pioLcdTransport;

uint32_t transportPinMask() {
  return (1u << g_pioLcdTransport.clockPin) | (1u << g_pioLcdTransport.dataPin);
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

bool initializePioTransport(u8x8_t *u8x8, uint8_t initialDataLevel, uint8_t initialClockLevel) {
  if (g_pioLcdTransport.initialized) {
    return g_pioLcdTransport.available;
  }

  g_pioLcdTransport.initialized = true;
  g_pioLcdTransport.clockPin = u8x8->pins[U8X8_PIN_SPI_CLOCK];
  g_pioLcdTransport.dataPin = u8x8->pins[U8X8_PIN_SPI_DATA];
  g_pioLcdTransport.requestedBitClockHz = u8x8->bus_clock;

  const PIO candidates[] = {pio0, pio1};
  for (PIO candidate : candidates) {
    if (!pio_can_add_program(candidate, &kSt7565PioProgram)) {
      continue;
    }

    const int claimedSm = pio_claim_unused_sm(candidate, false);
    if (claimedSm < 0) {
      continue;
    }

    g_pioLcdTransport.pio = candidate;
    g_pioLcdTransport.sm = static_cast<uint>(claimedSm);
    g_pioLcdTransport.programOffset = pio_add_program(candidate, &kSt7565PioProgram);

    const uint32_t pinMask = transportPinMask();
    const uint32_t pinValues = (initialDataLevel ? (1u << g_pioLcdTransport.dataPin) : 0u) |
                               (initialClockLevel ? (1u << g_pioLcdTransport.clockPin) : 0u);

    pio_gpio_init(candidate, g_pioLcdTransport.dataPin);
    pio_gpio_init(candidate, g_pioLcdTransport.clockPin);

    const pio_sm_config config = st7565PioProgramConfig(
        g_pioLcdTransport.programOffset, g_pioLcdTransport.dataPin, g_pioLcdTransport.clockPin);
    pio_sm_init(candidate, g_pioLcdTransport.sm, g_pioLcdTransport.programOffset, &config);
    pio_sm_set_pindirs_with_mask(candidate, g_pioLcdTransport.sm, pinMask, pinMask);
    pio_sm_set_pins_with_mask(candidate, g_pioLcdTransport.sm, pinValues, pinMask);
    pio_sm_clear_fifos(candidate, g_pioLcdTransport.sm);
    pio_sm_restart(candidate, g_pioLcdTransport.sm);
    pio_sm_clkdiv_restart(candidate, g_pioLcdTransport.sm);
    pio_sm_set_enabled(candidate, g_pioLcdTransport.sm, true);

    g_pioLcdTransport.available = true;
    return true;
  }

  return false;
}

bool ensurePioTransportInitialized(u8x8_t *u8x8) {
  if (g_pioLcdTransport.initialized) {
    return g_pioLcdTransport.available;
  }

  return initializePioTransport(u8x8, 0, u8x8_GetSPIClockDefaultLevel(u8x8));
}

void updatePioBitClock(uint32_t requestedBitClockHz) {
  if (!g_pioLcdTransport.available) {
    return;
  }

  if (requestedBitClockHz == 0) {
    requestedBitClockHz = 1;
  }

  const uint32_t systemClockHz = clock_get_hz(clk_sys);
  const uint32_t maxBitClockHz = (systemClockHz >= kPioCyclesPerBit) ? (systemClockHz / kPioCyclesPerBit) : 1u;
  const uint32_t effectiveBitClockHz = min(min(requestedBitClockHz, kPioMaxStableBitClockHz), maxBitClockHz);

  if ((g_pioLcdTransport.configuredBitClockHz == effectiveBitClockHz) &&
      (g_pioLcdTransport.configuredSysClockHz == systemClockHz)) {
    return;
  }

  const float divider =
      static_cast<float>(systemClockHz) / (static_cast<float>(effectiveBitClockHz) * kPioCyclesPerBit);
  uint16_t dividerInt = 1;
  uint8_t dividerFrac = 0;
  pio_calculate_clkdiv_from_float(max(divider, 1.0f), &dividerInt, &dividerFrac);
  pio_sm_set_clkdiv_int_frac(g_pioLcdTransport.pio, g_pioLcdTransport.sm, dividerInt, dividerFrac);

  g_pioLcdTransport.requestedBitClockHz = requestedBitClockHz;
  g_pioLcdTransport.configuredBitClockHz = effectiveBitClockHz;
  g_pioLcdTransport.configuredSysClockHz = systemClockHz;
}

void sendPioBytes(const uint8_t *data, uint8_t count) {
  while (count > 0) {
    pio_sm_put_blocking(g_pioLcdTransport.pio,
                        g_pioLcdTransport.sm,
                        static_cast<uint32_t>(*data) << 24);
    ++data;
    --count;
  }
}

void waitForTransferComplete() {
  if (!g_pioLcdTransport.available) {
    return;
  }

  while (!pio_sm_is_tx_fifo_empty(g_pioLcdTransport.pio, g_pioLcdTransport.sm)) {
    tight_loop_contents();
  }

  const uint32_t drainDelayUs =
      (((kPioBitsPerByte + 2u) * 1000000u) + g_pioLcdTransport.configuredBitClockHz - 1u) /
      g_pioLcdTransport.configuredBitClockHz;
  delayMicroseconds(max<uint32_t>(2u, drainDelayUs + 2u));
}

extern "C" uint8_t u8x8_byte_mk61ext_pio_4wire(u8x8_t *u8x8,
                                               uint8_t msg,
                                               uint8_t arg_int,
                                               void *arg_ptr) {
  switch (msg) {
    case U8X8_MSG_BYTE_SEND:
      if (!ensurePioTransportInitialized(u8x8)) {
        return u8x8_byte_arduino_4wire_sw_spi(u8x8, msg, arg_int, arg_ptr);
      }
      sendPioBytes(static_cast<const uint8_t *>(arg_ptr), arg_int);
      break;

    case U8X8_MSG_BYTE_INIT:
      if (u8x8->bus_clock == 0) {
        u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
      }
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      u8x8_gpio_SetSPIData(u8x8, 0);
      u8x8_gpio_SetSPIClock(u8x8, u8x8_GetSPIClockDefaultLevel(u8x8));
      if (!initializePioTransport(u8x8, 0, u8x8_GetSPIClockDefaultLevel(u8x8))) {
        return u8x8_byte_arduino_4wire_sw_spi(u8x8, msg, arg_int, arg_ptr);
      }
      g_pioLcdTransport.requestedBitClockHz = u8x8->bus_clock;
      updatePioBitClock(g_pioLcdTransport.requestedBitClockHz);
      break;

    case U8X8_MSG_BYTE_SET_DC:
      if (!ensurePioTransportInitialized(u8x8)) {
        return u8x8_byte_arduino_4wire_sw_spi(u8x8, msg, arg_int, arg_ptr);
      }
      u8x8_gpio_SetDC(u8x8, arg_int);
      break;

    case U8X8_MSG_BYTE_START_TRANSFER:
      if (!ensurePioTransportInitialized(u8x8)) {
        return u8x8_byte_arduino_4wire_sw_spi(u8x8, msg, arg_int, arg_ptr);
      }
      updatePioBitClock(u8x8->bus_clock);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
      u8x8->gpio_and_delay_cb(
          u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, nullptr);
      break;

    case U8X8_MSG_BYTE_END_TRANSFER:
      if (!ensurePioTransportInitialized(u8x8)) {
        return u8x8_byte_arduino_4wire_sw_spi(u8x8, msg, arg_int, arg_ptr);
      }
      waitForTransferComplete();
      u8x8->gpio_and_delay_cb(
          u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, nullptr);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      break;

    default:
      return 0;
  }

  return 1;
}

#endif

}  // namespace

Mk61extDisplay::Mk61extDisplay(const u8g2_cb_t *rotation,
                               uint8_t clock,
                               uint8_t data,
                               uint8_t cs,
                               uint8_t dc,
                               uint8_t reset)
    : U8G2() {
#if MK61EXT_HAS_PIO_LCD_TRANSPORT && MK61EXT_ENABLE_PIO_LCD_TRANSPORT
  u8g2_Setup_st7565_erc12864_f(&u8g2, rotation, u8x8_byte_mk61ext_pio_4wire, u8x8_gpio_and_delay_arduino);
#else
  u8g2_Setup_st7565_erc12864_f(&u8g2, rotation, u8x8_byte_arduino_4wire_sw_spi, u8x8_gpio_and_delay_arduino);
#endif
  u8x8_SetPin_4Wire_SW_SPI(getU8x8(), clock, data, cs, dc, reset);
}
