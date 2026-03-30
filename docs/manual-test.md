# Manual Hardware Test

This checklist matches the current firmware scaffold in `src/main.cpp`.

## Flash

```bash
pio run -t upload
```

## Expected Boot Behavior

1. Power on or reset the Pico.
2. The display should show `memory found` for about 3 seconds.
3. After the splash screen, the main screen should show:
   - a filled top status bar such as `MK61 RUN`,
   - four equal-height stack lines labeled `T:`, `Z:`, `Y:`, and `X:` or `X>`,
   - no framed register boxes or separator borders in the stack area.

If the display instead shows `no memory found`, the external EEPROM on GPIO 14/15 is not responding.

## EEPROM Wiring Reference

The current firmware expects an AT24C256C-compatible EEPROM at I2C address `0x50`.

1. Connect pin 1 `A0` to `GND`.
2. Connect pin 2 `A1` to `GND`.
3. Connect pin 3 `A2` to `GND`.
4. Connect pin 4 `GND` to system ground.
5. Connect pin 5 `SDA` to Pico `GPIO 14`.
6. Connect pin 6 `SCL` to Pico `GPIO 15`.
7. Connect pin 7 `WP` to `GND` for normal operation.
8. Connect pin 8 `VCC` to `3v3`.

Also add I2C pull-ups from `SDA` to `3v3` and from `SCL` to `3v3`. The AT24C256C datasheet says the pull-ups should not exceed `10 kOhm`; `4.7 kOhm` is a typical choice.

## Keypad And Backlight Test

1. Press any keypad button once.
2. Confirm the right side of the top status bar updates to the matching key character and `PRESS`.
3. Hold the same key for more than 0.5 seconds.
4. Confirm the status bar changes to `HOLD`.
5. Release the key.
6. Confirm the status bar changes to `REL`.

## Functional Key Checks

1. Press `a` repeatedly.
2. Confirm the backlight brightens in steps.
3. Press `b` repeatedly.
4. Confirm the backlight dims in steps.

## Calculator Smoke Test

The current provisional MK-61-inspired run-mode subset is:

- digits: `0`-`9`
- decimal point: `.`
- operations: `+`, `-`, `*`, `/`
- `v`: `ENTER`
- `y`: `EEX`
- `x`: `CHS` (change sign)
- `u`: `x↔y`
- `z`: `CX`
- `s`, then `.`: `R↓` (stack roll-down)
- `s`, then `-`: square root
- `s`, then `/`: reciprocal (`1/x`)
- `s`, then `+`: `pi`
- `s`, then `*`: `x^2`
- `s`, then `0`: `10^x`
- `s`, then `1`: `e^x`
- `s`, then `2`: `lg`
- `s`, then `3`: `ln`
- `s`, then `7` / `8` / `9`: `sin` / `cos` / `tan`
- `s`, then `4` / `5` / `6`: `asin` / `acos` / `atan`
- `s`, then `u`: `x^y` (implemented as `X^Y`, matching the MK-61 notation)
- `s`, then `v`: `LAST X`
- `t`, then `7`: `INT`
- `t`, then `8`: `FRAC`
- `t`, then `9`: `max`
- `t`, then `4`: `|x|`
- `t`, then `5`: `sign`
- `c`: clear all
- `a` / `b`: backlight brighter / dimmer

The calculator core stores stack values as 64-bit floating point numbers. The current screen now tries to show up to 15 significant digits while still fitting each value into the 128x64 stack layout.
Trigonometric functions currently use radians until an angle-mode feature exists.

1. Press `2`, then `v`, then `3`, then `+`.
2. Confirm the `X` register shows `5`.
3. Press `9`, then `v`, then `4`, then `/`.
4. Confirm the `X` register shows `2.25`.
5. Press `x`.
6. Confirm the `X` register changes sign.
7. Press `z`.
8. Confirm `X` resets to `0`.
9. Press `c`.
10. Confirm `X`, `Y`, `Z`, and `T` all reset to `0`.
11. Press `8`, then `v`, then `2`, then `u`.
12. Confirm `X` and `Y` swap values.
13. Press `3`, then `v`, then `4`, then `v`, then `5`.
14. Confirm the stack is `X=5`, `Y=4`, `Z=3`.
15. Press `k`, then `.`.
16. Confirm the stack rolls down to `X=4`, `Y=3`, `Z=0`, `T=5`.
17. Press `9`, then `k`, then `-`.
18. Confirm `X` becomes `3`.
19. Press `4`, then `k`, then `/`.
20. Confirm `X` becomes `0.25`.
21. Press `k`, then `+`.
22. Confirm `X` becomes approximately `3.14159265358979`.
23. Press `1`, then `y`, then `3`.
24. Confirm `X` becomes `1000`.
25. Press `1`, then `y`, then `x`, then `2`.
26. Confirm `X` becomes `0.01`.

## Prefix State Check

This verifies the new one-shot `F`/`K` prefix handling and the status-bar feedback.

1. Press `k`.
2. Confirm the left side of the top status bar shows `MK61 F RUN`.
3. Press `.`.
4. Confirm the prefix is consumed and the status bar returns to plain `MK61 RUN`.
5. Press `p`.
6. Confirm the left side of the top status bar shows `MK61 K RUN`.
7. Press `4`.
8. Confirm the prefix is consumed and the status bar returns to plain `MK61 RUN`.

## Extended Operation Check

This verifies the newly implemented scientific and utility operations.

1. Press `c`, then `0`, then `k`, then `7`.
2. Confirm `X` stays `0` (`sin(0) = 0`).
3. Press `c`, then `0`, then `k`, then `8`.
4. Confirm `X` becomes `1` (`cos(0) = 1`).
5. Press `c`, then `1`, then `k`, then `1`.
6. Confirm `X` becomes approximately `2.71828182845905`.
7. Press `c`, then `2`, then `k`, then `0`.
8. Confirm `X` becomes `100`.
9. Press `c`, then `1`, then `0`, then `0`, then `0`, then `k`, then `2`.
10. Confirm `X` becomes `3`.
11. Press `c`, then `1`, then `0`, then `k`, then `3`.
12. Confirm `X` becomes approximately `2.30258509299405`.
13. Press `c`, then `3`, then `k`, then `*`.
14. Confirm `X` becomes `9`.
15. Press `c`, then `2`, then `v`, then `3`, then `k`, then `u`.
16. Confirm `X` becomes `9` (`X^Y`, so `3^2`).
17. Press `k`, then `v`.
18. Confirm `X` returns to `3` from `LAST X`.
19. Press `c`, then `-`, then `3`, then `p`, then `4`.
20. Confirm `X` becomes `3`.
21. Press `c`, then `-`, then `3`, then `p`, then `5`.
22. Confirm `X` becomes `-1`.
23. Press `c`, then `1`, then `2`, then `.`, then `7`, then `5`, then `p`, then `7`.
24. Confirm `X` becomes `12`.
25. Press `c`, then `1`, then `2`, then `.`, then `7`, then `5`, then `p`, then `8`.
26. Confirm `X` becomes `0.75`.
27. Press `c`, then `2`, then `v`, then `5`, then `p`, then `9`.
28. Confirm `X` becomes `5`.

## Entry Stack-Lift Check

This verifies the calculator now preserves the displayed `X` value when you begin typing a fresh number from display mode.

1. Press `c`.
2. Press `2`, then `v`, then `3`, then `+`.
3. Confirm `X` shows `5`.
4. Press `4`.
5. Confirm `X` shows `4`.
6. Confirm `Y` still shows `5`.
7. Press `v`, then `6`.
8. Confirm the stack is `X=6`, `Y=4`, `Z=5`.

## 64-Bit Precision Spot Check

The firmware now fails to build if the toolchain does not provide a real 64-bit `double`, so a successful `pio run` already confirms the compiler-side requirement.

This extra keypad test is still useful on hardware because it checks that the calculator path is really preserving that precision end-to-end:

1. Press `c`.
2. Press `1`, then `y`, then `8`.
3. Confirm `X` shows `1e+08`.
4. Press `v`, then `1`, then `+`.
5. Confirm `X` shows `100000001`.

If `X` stays at `100000000` instead, the calculator is behaving like a 32-bit float path somewhere.

## Multi-Key Hold Regression Check

This verifies the recent keypad timing fix.

1. Press and hold one key.
2. While still holding it, press and hold a second key.
3. Keep both pressed for more than 0.5 seconds.
4. Confirm each key can independently produce a `HOLD` event rather than sharing one timer.

## Notes

- The active key map is:
  - row 0: `a b c d e`
  - row 1: `f g h i j`
  - row 2: `k l m n o`
  - row 3: `p q r s t`
  - row 4: `7 8 9 - /`
  - row 5: `4 5 6 + *`
  - row 6: `1 2 3 u v`
  - row 7: `0 . x y z`
- This is still a prototype keypad layout over the custom 8x5 matrix, but it now follows a more deliberate MK-61-inspired run-mode subset.
- The longer-term full assignment draft, including intended `F`- and `K`-shifted layers, is tracked in `docs/key-assignments.md`.
- The active runtime subset now uses the bottom six rows of the matrix for calculator keys and begins to follow the planned MK-61 prefix model with `k` = `F` and `p` = `K`.
- The currently wired `F`-layer now includes trig, inverse trig, powers, exponentials, logarithms, `LAST X`, and the earlier `R↓`, `sqrt`, `1/x`, and `pi` functions.
- The currently wired `K`-layer now includes `INT`, `FRAC`, `max`, `|x|`, and `sign`.
- The top status bar shows the current calculator mode (`RUN`, `ENT`, `EEX`, or `ERR`) on the left, optionally prefixed by `F` or `K` when a one-shot prefix is armed, and the most recent key/state event on the right. When the calculator is in an error state, the status bar shows the error text instead.
- The stack area now uses four uniform text rows without frames or separator lines.
