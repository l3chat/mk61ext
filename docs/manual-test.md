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

1. Press `b` repeatedly.
2. Confirm the backlight dims in steps.
3. Press `a` repeatedly.
4. Confirm the backlight brightens in steps.
5. Press `e`.
6. Confirm the status bar changes to `MK61 HELP` and the stack view is replaced by help text.
7. Press `7`.
8. Confirm the help text explains digit `7` instead of changing the calculator stack.
9. Press `e` again.
10. Confirm the normal calculator screen returns.

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
- `k`, then `.`: `R↓` (stack roll-down)
- `k`, then `-`: square root
- `k`, then `/`: reciprocal (`1/x`)
- `k`, then `+`: `pi`
- `k`, then `*`: `x^2`
- `k`, then `0`: `10^x`
- `k`, then `1`: `e^x`
- `k`, then `2`: `lg`
- `k`, then `3`: `ln`
- `k`, then `7` / `8` / `9`: `sin` / `cos` / `tan`
- `k`, then `4` / `5` / `6`: `asin` / `acos` / `atan`
- `k`, then `u`: `x^y` (implemented as `X^Y`, matching the MK-61 notation)
- `k`, then `v`: `LAST X`
- `p`, then `7`: `INT`
- `p`, then `8`: `FRAC`
- `p`, then `9`: `max`
- `p`, then `4`: `|x|`
- `p`, then `5`: `sign`
- `p`, then `6`: `H->H.M`
- `p`, then `+`: `H.M->H`
- `p`, then `3`: `H->H.M.S`
- `p`, then `u`: `H.M.S->H`
- `p`, then `v`: `RND`
- `c`: clear all
- `a`: backlight brighter
- `b`: backlight dimmer
- `e`: toggle help mode

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

## Help Mode Check

This verifies that help mode can be entered with `e` and that keys show descriptions instead of acting while it is active.

1. Press `e`.
2. Confirm the left side of the top status bar shows `MK61 HELP`.
3. Press `k`.
4. Confirm the left side of the top status bar now shows `MK61 HELP F`.
5. Press `7`.
6. Confirm the screen shows help text for `sin` instead of changing the stack.
7. Press `e`.
8. Confirm the normal calculator stack screen returns.

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
29. Press `c`, then `2`, then `.`, then `5`, then `p`, then `6`.
30. Confirm `X` becomes `2.3`.
31. Press `c`, then `2`, then `.`, then `3`, then `0`, then `3`, then `0`, then `p`, then `+`.
32. Confirm `X` becomes approximately `2.505`.
33. Press `c`, then `2`, then `.`, then `5`, then `0`, then `8`, then `3`, then `3`, then `3`, then `p`, then `3`.
34. Confirm `X` becomes approximately `2.303`.
35. Press `c`, then `2`, then `.`, then `3`, then `0`, then `3`, then `0`, then `p`, then `u`.
36. Confirm `X` becomes approximately `2.508333`.
37. Press `c`, then `p`, then `v`.
38. Confirm `X` is greater than or equal to `0`.
39. Confirm `X` is less than `1`.

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
- The `a` and `b` keys now control backlight brighter/dimmer again, and `e` toggles the on-device help mode. While help mode is active, key presses show descriptions instead of performing their normal actions.
- The currently wired `F`-layer now includes trig, inverse trig, powers, exponentials, logarithms, `LAST X`, and the earlier `R↓`, `sqrt`, `1/x`, and `pi` functions.
- The currently wired `K`-layer now includes `INT`, `FRAC`, `max`, `|x|`, `sign`, the hour/minute conversion functions, and `RND`.
- The top status bar shows the current calculator mode (`RUN`, `ENT`, `EEX`, or `ERR`) on the left, optionally prefixed by `F` or `K` when a one-shot prefix is armed, and the most recent key/state event on the right. When the calculator is in an error state, the status bar shows the error text instead.
- The stack area now uses four uniform text rows without frames or separator lines.
