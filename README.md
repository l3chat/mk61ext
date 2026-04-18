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
bash scripts/trace_sequence.sh 5 XM 1 3 ENTER 4 '*' MX 1
```

You can also feed the same kind of sequence through PlatformIO:

```bash
TRACE_SEQ="5 XM 1 3 ENTER 4 * MX 1" pio run -t trace
```

The trace tool accepts raw keys like `q`, `r`, `v`, `x`, and `p`, plus a few convenience aliases such as `ENTER`, `EEX`, `CHS`, `MX`, `XM`, `MXI`, and `XMI`.

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

## Loop Timing

The main loop intentionally includes a small delay in `src/main.cpp`.

- Program-run mode delay: no intentional `delay(...)` while `programRunner.isRunning()`.
- Active calculator/UI mode delay: `kLoopDelayMs = 2 ms`.
- Sleep mode delay: `kSleepLoopDelayMs = 100 ms`.
- Program-run execution is now time-sliced: each loop runs VM steps until either `~1.5 ms` budget is used or `512` steps are executed, then returns to input/UI tasks.
- Program-run target validation now uses a cached step-boundary map inside `ProgramRunner` instead of repeated full boundary scans.

This delay is intentional and serves three purposes:

- It prevents a busy-spin loop that would otherwise run at 100% CPU all the time.
- It keeps UI behavior and keypad polling cadence stable in this cooperative single-loop design.
- It limits unnecessary full-screen LCD update pressure (the current display path is software SPI), which helps responsiveness and power usage.
- During active program execution, that intentional delay is skipped so the VM can run as fast as possible between screen refresh slots.

Latency impact:

- Any key is handled on the next loop pass, so loop delay contributes up to one loop period of wait.
- The keypad layer also applies debounce (`5 ms`), so measured end-to-end key-to-visible latency is not pure math execution time.
- Arithmetic itself (for example `X * Y`) is very fast; most perceived delay is input polling + debounce + display refresh timing.

## Manual Test

See [docs/manual-test.md](docs/manual-test.md) for the current hardware bring-up checklist and keypad diagnostics expectations.

See [docs/key-assignments.md](docs/key-assignments.md) for the longer-term key-assignment plan, including the intended `F`- and `K`-shifted layers.

See [docs/program-examples.md](docs/program-examples.md) for small hand-entered example programs that exercise the current program recorder and runner.

## EEPROM

The firmware expects an AT24C256C-compatible EEPROM on Pico `GPIO 14`/`GPIO 15` at I2C address `0x50`. The exact pin wiring is documented in [docs/manual-test.md](docs/manual-test.md). EEPROM addresses `0x0000`-`0x00ff` are reserved for settings, and `0x0100`-`0x01ff` are reserved for the current 256-byte program image. The current build stages brightness, backlight timeout, sleep timeout, angle mode (`RAD` / `GRD` / `DEG`), stack-label visibility, and CPU frequency in a dedicated settings screen. The timeout list now includes `OFF`, `5 s`, `15 s`, `30 s`, `1 min`, `2 min`, `5 min`, and `15 min`, and those idle timeouts are applied only while the unit is running on battery power. The current CPU-frequency choices are `12 MHz`, `24 MHz`, `48 MHz`, `96 MHz`, and `125 MHz`. `e` saves the staged values and `f` cancels them, and the saved settings survive a reboot. Program save/restore is explicit: `g` saves the current program image to EEPROM, and `h` restores the saved image on demand.

## Current Firmware

The current scaffold now includes a hardware-independent RPN calculator core plus a calculator-style on-device screen with an inverted status line and four equal-height stack rows (`T`, `Z`, `Y`, `X`). The status line now starts normal calculator states with `PCxx` so single-step execution visibly tracks the next program address. The keypad mapping lives in a dedicated calculator keymap layer and currently follows an MK-61-inspired run-mode subset documented in [docs/manual-test.md](docs/manual-test.md). The active runtime subset now uses the bottom six keypad rows for calculator functions, uses `k`/`p` as one-shot `F`/`K` prefixes, uses `o` to run or stop stored programs, `n` to reset the run address, `t` to single-step one stored program instruction, and `q`/`r` for direct `MX`/`XM` register access with the original MK-61-style `a-f` mapping on `. x y z v u`, and now also uses `p q` / `p r` for indirect `MXI` / `XMI` through pointer registers with MK-61-style auto-modification (`4-6` pre-increment, `0-3` post-decrement). Program EEPROM persistence is on demand: `g` saves the current program image and `h` restores the saved image. It opens a dedicated settings screen on `e`; inside that screen, `a`/`b` move between settings, `c`/`d` change the selected setting, `e` saves the staged values to EEPROM, and `f` cancels the staged changes and restores the earlier ones. The list is now numbered, the active `>` cursor is inverted, and the screen shows a short description for the selected row. Those settings now include brightness, backlight timeout, sleep timeout, angle mode, stack-label visibility, and CPU frequency. The timeout list now extends up to `15 min`, and the backlight/sleep timeout behavior is only active while the unit is running on battery power. After a battery-only sleep timeout, the LCD enters power-save and the first key wakes it without executing a calculator action. Help mode toggles on `f` outside the settings screen. While help mode is active, key presses show descriptions instead of executing calculator actions, and you can still press `k` or `p` before another key to inspect `F`- or `K`-shifted meanings. The currently wired shifted subset now covers the earlier `R↓`, `sqrt`, `1/x`, and `pi` functions plus trig, inverse trig, powers, exponentials, logarithms, `LAST X`, the utility operations (`INT`, `FRAC`, `max`, `|x|`, `sign`), the hour/minute and hour/minute/second conversion functions, `RND`, indirect register access, and simplified decimal bitwise `AND`/`OR`/`XOR`/`NOT` on unsigned 32-bit whole numbers. Trigonometric functions now honor the active saved angle mode, which is shown in the status bar as `RAD`, `GRD`, or `DEG`. The stack renderer now tries to show up to 15 significant digits per value while still fitting the display width, fresh numeric entry from display mode now lifts the stack to preserve the previous `X` value, `CX` now backspaces the active entry text during `ENT` / `EEX`, and mantissa entry is limited to 16 significant digits so the calculator does not accept a longer exact decimal text than the numeric core can reasonably carry.

The calculator core now treats all register values as 64-bit floating point (`double`) and enforces that assumption with compile-time checks, so the build will fail on any toolchain that downgrades `double` precision.
