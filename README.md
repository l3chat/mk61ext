# mk61ext

PlatformIO firmware for an extended MK-61 recreation on Raspberry Pi Pico hardware.

The active code lives in `src/` and `include/`. The `_archive/` directory is historical reference material from the original prototype and should not be treated as the active firmware tree.

## Build

`pio` is not on `PATH` in this environment, so use:

```bash
/home/lechat/.platformio/penv/bin/platformio run
```

## Flash

```bash
/home/lechat/.platformio/penv/bin/platformio run -t upload
```

The custom upload helper will:

- reuse the `RPI-RP2` boot drive if the Pico is already in BOOTSEL mode,
- otherwise touch the Pico serial port at 1200 baud,
- wait for the `RPI-RP2` drive to appear,
- copy the generated UF2 automatically.

If auto-detection ever picks the wrong device, override the serial port explicitly:

```bash
/home/lechat/.platformio/penv/bin/platformio run -t upload --upload-port /dev/ttyACM0
```

## Manual Test

See [docs/manual-test.md](docs/manual-test.md) for the current hardware bring-up checklist and keypad diagnostics expectations.

## EEPROM

The firmware expects an AT24C256C-compatible EEPROM on Pico `GPIO 14`/`GPIO 15` at I2C address `0x50`. The exact pin wiring is documented in [docs/manual-test.md](docs/manual-test.md).

## Current Firmware

The current scaffold now includes a hardware-independent RPN calculator core plus a provisional on-device stack view. The keypad mapping is temporary, now lives in a dedicated calculator keymap layer, and is documented in [docs/manual-test.md](docs/manual-test.md).
