# mk61ext

PlatformIO firmware for an extended MK-61 recreation on Raspberry Pi Pico hardware.

The active code lives in `src/` and `include/`. The `_archive/` directory is historical reference material from the original prototype and should not be treated as the active firmware tree.

## Build

In the VS Code integrated terminal for this workspace, `pio` is on `PATH`, so use:

```bash
pio run
```

## Flash

```bash
pio run -t upload
```

The custom upload helper will:

- reuse the `RPI-RP2` boot drive if the Pico is already in BOOTSEL mode,
- otherwise touch the Pico serial port at 1200 baud,
- wait for the `RPI-RP2` drive to appear,
- copy the generated UF2 automatically.

If auto-detection ever picks the wrong device, override the serial port explicitly:

```bash
pio run -t upload --upload-port /dev/ttyACM0
```

## Manual Test

See [docs/manual-test.md](docs/manual-test.md) for the current hardware bring-up checklist and keypad diagnostics expectations.

## EEPROM

The firmware expects an AT24C256C-compatible EEPROM on Pico `GPIO 14`/`GPIO 15` at I2C address `0x50`. The exact pin wiring is documented in [docs/manual-test.md](docs/manual-test.md).

## Current Firmware

The current scaffold now includes a hardware-independent RPN calculator core plus a calculator-style on-device screen with an inverted status line and four equal-height stack rows (`T`, `Z`, `Y`, `X`). The keypad mapping lives in a dedicated calculator keymap layer and currently follows an MK-61-inspired run-mode subset documented in [docs/manual-test.md](docs/manual-test.md). The stack renderer now tries to show up to 15 significant digits per value while still fitting the display width, and fresh numeric entry from display mode now lifts the stack to preserve the previous `X` value.

The calculator core now treats all register values as 64-bit floating point (`double`) and enforces that assumption with compile-time checks, so the build will fail on any toolchain that downgrades `double` precision.
