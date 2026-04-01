# mk61ext

PlatformIO firmware for an extended MK-61 recreation on Raspberry Pi Pico hardware.

The active code lives in `src/` and `include/`. The `_archive/` directory is historical reference material from the original prototype and should not be treated as the active firmware tree.

## Build

In the VS Code integrated terminal for this workspace, `pio` is on `PATH`, so use:

```bash
pio run
```

## Regression Test

Run the host-side regression directly with:

```bash
bash scripts/run_regression.sh
```

Or run the same regression through PlatformIO with:

```bash
pio run -t regression
```

## Trace A Sequence

Trace a key sequence and print the stack after each key with:

```bash
bash scripts/trace_sequence.sh 5 STO 1 3 ENTER 4 '*' RCL 1
```

You can also feed the same kind of sequence through PlatformIO:

```bash
TRACE_SEQ="5 STO 1 3 ENTER 4 * RCL 1" pio run -t trace
```

The trace tool accepts raw keys like `q`, `r`, `v`, `x`, and `p`, plus a few convenience aliases such as `ENTER`, `EEX`, `CHS`, `RCL`, `STO`, `RCLI`, and `STOI`.

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

See [docs/key-assignments.md](docs/key-assignments.md) for the longer-term key-assignment plan, including the intended `F`- and `K`-shifted layers.

## EEPROM

The firmware expects an AT24C256C-compatible EEPROM on Pico `GPIO 14`/`GPIO 15` at I2C address `0x50`. The exact pin wiring is documented in [docs/manual-test.md](docs/manual-test.md). The current build stores the backlight brightness, angle mode (`RAD` / `GRD` / `DEG`), and stack-label visibility there so those settings survive a reboot.

## Current Firmware

The current scaffold now includes a hardware-independent RPN calculator core plus a calculator-style on-device screen with an inverted status line and four equal-height stack rows (`T`, `Z`, `Y`, `X`). The keypad mapping lives in a dedicated calculator keymap layer and currently follows an MK-61-inspired run-mode subset documented in [docs/manual-test.md](docs/manual-test.md). The active runtime subset now uses the bottom six keypad rows for calculator functions, uses `k`/`p` as one-shot `F`/`K` prefixes, uses `q`/`r` for direct `RCL`/`STO` register access with the original MK-61-style `a-e` mapping on `. x y z v`, and now also uses `p q` / `p r` for indirect `RCLI` / `STOI` through pointer registers with MK-61-style auto-modification (`4-6` pre-increment, `0-3` post-decrement). It uses `a`/`b` for backlight brighter/dimmer, `c` to cycle the saved angle mode, `d` to hide/show the saved stack labels, and `e` to toggle an on-device help mode. While help mode is active, key presses show descriptions instead of executing calculator actions, and you can still press `k` or `p` before another key to inspect `F`- or `K`-shifted meanings. The currently wired shifted subset now covers the earlier `R↓`, `sqrt`, `1/x`, and `pi` functions plus trig, inverse trig, powers, exponentials, logarithms, `LAST X`, the utility operations (`INT`, `FRAC`, `max`, `|x|`, `sign`), the hour/minute and hour/minute/second conversion functions, `RND`, indirect register access, and simplified decimal bitwise `AND`/`OR`/`XOR`/`NOT` on unsigned 32-bit whole numbers. Trigonometric functions now honor the active saved angle mode, which is shown in the status bar as `RAD`, `GRD`, or `DEG`. The stack renderer now tries to show up to 15 significant digits per value while still fitting the display width, fresh numeric entry from display mode now lifts the stack to preserve the previous `X` value, `CX` now backspaces the active entry text during `ENT` / `EEX`, and mantissa entry is limited to 16 significant digits so the calculator does not accept a longer exact decimal text than the numeric core can reasonably carry.

The calculator core now treats all register values as 64-bit floating point (`double`) and enforces that assumption with compile-time checks, so the build will fail on any toolchain that downgrades `double` precision.
