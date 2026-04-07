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
   - a filled top status bar such as `MK61 RAD RUN`,
   - a supply-voltage readout such as `4.12V` on the right side of that top bar,
   - four equal-height stack lines labeled `T:`, `Z:`, `Y:`, and `X:` or `X>`,
   - no framed register boxes or separator borders in the stack area.

If the display instead shows `no memory found`, the external EEPROM on GPIO 14/15 is not responding.
The displayed voltage is measured from Pico `VSYS` through the board's built-in divider on `A3`, so it tracks battery voltage when the unit is battery-powered and the system supply when USB is attached.

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
7. Wait about 1.5 seconds without pressing anything.
8. Confirm the right side of the status bar returns to the voltage readout.

## Functional Key Checks

1. Press `e`.
2. Confirm the status bar changes to `MK61 SET` and the stack view is replaced by a settings screen.
3. Confirm the settings list is numbered and shows `1. Brightness`, `2. Backlight`, `3. Sleep`, `4. Angle`, `5. Labels`, and `6. CPU freq`.
4. Confirm the active `>` cursor is drawn inverted.
5. Press `b`.
6. Confirm the selection moves from `Brightness` to `Backlight`.
7. Confirm the description area says the backlight turns off after inactivity.
8. Press `d`.
9. Confirm the `Backlight` timeout value advances.
10. Press `b`.
11. Confirm the selection moves to `Sleep`.
12. Confirm the description area says the system enters energy-saving mode after inactivity.
13. Press `d`.
14. Confirm the `Sleep` timeout value advances.
15. Press `b` until `Angle` is selected.
16. Press `d`.
17. Confirm the `Angle` value advances to the next mode (`RAD` -> `GRD` -> `DEG` -> `RAD`).
18. Press `b` until `Labels` is selected.
19. Press `c`.
20. Confirm the `Labels` value toggles.
21. Press `b` until `CPU freq` is selected.
22. Press `d`.
23. Confirm the CPU-frequency value advances to the next option.
24. Confirm the available CPU-frequency values cycle through `12 MHz`, `24 MHz`, `48 MHz`, `96 MHz`, and `125 MHz`.
25. Press `f`.
26. Confirm the normal calculator screen returns and the visible stack labels revert to their original state.
27. Press `e` again, repeat the same `Labels` toggle, then press `e`.
28. Confirm the normal calculator screen returns and the visible stack labels now match the saved value.
29. Press `f`.
30. Confirm the status bar changes to `MK61 RAD HELP` and the stack view is replaced by help text.
31. Press `7`.
32. Confirm the help text explains digit `7` instead of changing the calculator stack.
33. Press `f` again.
34. Confirm the normal calculator screen returns.

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
- `p`, then `.` / `x` / `y`: `AND` / `OR` / `XOR` on unsigned 32-bit whole numbers
- `p`, then `z`: `NOT` on an unsigned 32-bit whole number
- `q`, then `0`-`9` or `.` / `x` / `y` / `z` / `v`: `RCL` register `0`-`e`
- `r`, then `0`-`9` or `.` / `x` / `y` / `z` / `v`: `STO` register `0`-`e`
- `p`, then `q`, then `0`-`9` or `.` / `x` / `y` / `z` / `v`: `RCLI` through a pointer register
- `p`, then `r`, then `0`-`9` or `.` / `x` / `y` / `z` / `v`: `STOI` through a pointer register
- `e`: open the settings screen
- while the settings screen is open:
  - `a` / `b`: select the previous / next setting
  - `c` / `d`: decrease / increase the selected setting (or toggle it)
  - `e`: save the staged settings to EEPROM and leave the screen
  - `f`: cancel the staged settings, restore the earlier values, and leave the screen
- `f`: toggle help mode

The calculator core stores stack values as 64-bit floating point numbers. The current screen now tries to show up to 15 significant digits while still fitting each value into the 128x64 stack layout.
Trigonometric functions now use the active saved angle mode (`RAD`, `GRD`, or `DEG`), and the status bar shows the current choice.
Indirect register access currently uses the whole-number part of the pointer register wrapped across registers `0`-`e`, with pointer registers `4`-`6` pre-incremented and `0`-`3` post-decremented.
While `ENT` or `EEX` is active, `CX` now works like a backspace key and removes the last entry character; outside entry it still clears `X`.
Mantissa entry is currently limited to 16 significant digits. Additional mantissa digits are rejected immediately so the calculator does not pretend to preserve a longer exact value than the numeric core can reasonably carry.
Backlight brightness, backlight timeout, sleep timeout, angle mode, stack-label visibility, and CPU frequency are staged in the settings screen, saved to EEPROM when you leave that screen with `e`, and restored again at boot. When the sleep timeout expires, the LCD enters power-save and the first key only wakes the screen.

## Idle Power Check

1. Open the settings screen with `e`.
2. Set `Brightness` to a visible non-`OFF` value.
3. Set `Backlight` to `5 s`.
4. Set `Sleep` to `15 s`.
5. Press `e` to save.
6. Leave the calculator idle on the main screen.
7. Confirm the backlight turns off after about 5 seconds.
8. Confirm the display enters sleep after about 15 seconds.
9. Press one key once.
10. Confirm the display wakes without executing that key's calculator action.
11. Press the same key again.
12. Confirm the key now executes normally.

## Supply Voltage Check

1. Leave the unit idle on the main calculator screen.
2. Confirm the right side of the status bar shows a voltage with two decimals, such as `4.12V`.
3. If the unit is powered only from a LiPo cell, confirm the value is plausible for the current battery state.
4. If USB is attached, expect the value to reflect Pico `VSYS`, which may be closer to the USB-backed system supply than to the battery alone.

1. Press `2`, then `v`, then `3`, then `+`.
2. Confirm the `X` register shows `5`.
3. Press `9`, then `v`, then `4`, then `/`.
4. Confirm the `X` register shows `2.25`.
5. Press `x`.
6. Confirm the `X` register changes sign.
7. Press `z`.
8. Confirm `X` resets to `0`.
9. Press `8`, then `v`, then `2`, then `u`.
10. Confirm `X` and `Y` swap values.
11. Press `3`, then `v`, then `4`, then `v`, then `5`.
12. Confirm the stack is `X=5`, `Y=4`, `Z=3`.
13. Press `k`, then `.`.
14. Confirm the stack rolls down to `X=4`, `Y=3`, `Z=0`, `T=5`.
15. Press `9`, then `k`, then `-`.
16. Confirm `X` becomes `3`.
17. Press `4`, then `k`, then `/`.
18. Confirm `X` becomes `0.25`.
19. Press `k`, then `+`.
20. Confirm `X` becomes approximately `3.14159265358979`.
21. Press `1`, then `y`, then `3`.
22. Confirm `X` becomes `1000`.
23. Press `1`, then `y`, then `x`, then `2`.
24. Confirm `X` becomes `0.01`.

## Prefix State Check

This verifies the new one-shot `F`/`K` prefix handling and the status-bar feedback.

1. Press `k`.
2. Confirm the left side of the top status bar shows `MK61 F RAD RUN` when the angle mode is `RAD`.
3. Press `.`.
4. Confirm the prefix is consumed and the status bar returns to plain `MK61 RAD RUN`.
5. Press `p`.
6. Confirm the left side of the top status bar shows `MK61 K RAD RUN`.
7. Press `4`.
8. Confirm the prefix is consumed and the status bar returns to plain `MK61 RAD RUN`.

## Help Mode Check

This verifies that help mode can be entered with `f` and that keys show descriptions instead of acting while it is active.

1. Press `f`.
2. Confirm the left side of the top status bar shows `MK61 RAD HELP`.
3. Press `k`.
4. Confirm the left side of the top status bar now shows `MK61 RAD HELP F`.
5. Press `7`.
6. Confirm the screen shows help text for `sin` instead of changing the stack.
7. Press `f`.
8. Confirm the normal calculator stack screen returns.

## Extended Operation Check

This verifies the newly implemented scientific and utility operations.

1. Press `0`, then `k`, then `7`.
2. Confirm `X` stays `0` (`sin(0) = 0`).
3. Press `0`, then `k`, then `8`.
4. Confirm `X` becomes `1` (`cos(0) = 1`).
5. Press `1`, then `k`, then `1`.
6. Confirm `X` becomes approximately `2.71828182845905`.
7. Press `2`, then `k`, then `0`.
8. Confirm `X` becomes `100`.
9. Press `1`, then `0`, then `0`, then `0`, then `k`, then `2`.
10. Confirm `X` becomes `3`.
11. Press `1`, then `0`, then `k`, then `3`.
12. Confirm `X` becomes approximately `2.30258509299405`.
13. Press `3`, then `k`, then `*`.
14. Confirm `X` becomes `9`.
15. Press `2`, then `v`, then `3`, then `k`, then `u`.
16. Confirm `X` becomes `9` (`X^Y`, so `3^2`).
17. Press `k`, then `v`.
18. Confirm `X` returns to `3` from `LAST X`.
19. Press `3`, then `x`, then `p`, then `4`.
20. Confirm `X` becomes `3`.
21. Press `3`, then `x`, then `p`, then `5`.
22. Confirm `X` becomes `-1`.
23. Press `1`, then `2`, then `.`, then `7`, then `5`, then `p`, then `7`.
24. Confirm `X` becomes `12`.
25. Press `1`, then `2`, then `.`, then `7`, then `5`, then `p`, then `8`.
26. Confirm `X` becomes `0.75`.
27. Press `2`, then `v`, then `5`, then `p`, then `9`.
28. Confirm `X` becomes `5`.
29. Press `2`, then `.`, then `5`, then `p`, then `6`.
30. Confirm `X` becomes `2.3`.
31. Press `2`, then `.`, then `3`, then `0`, then `3`, then `0`, then `p`, then `+`.
32. Confirm `X` becomes approximately `2.505`.
33. Press `2`, then `.`, then `5`, then `0`, then `8`, then `3`, then `3`, then `3`, then `p`, then `3`.
34. Confirm `X` becomes approximately `2.303`.
35. Press `2`, then `.`, then `3`, then `0`, then `3`, then `0`, then `p`, then `u`.
36. Confirm `X` becomes approximately `2.508333`.
37. Press `p`, then `v`.
38. Confirm `X` is greater than or equal to `0`.
39. Confirm `X` is less than `1`.
40. Press `1`, then `2`, then `v`, then `1`, then `0`, then `p`, then `.`.
41. Confirm `X` becomes `8`.
42. Press `1`, then `2`, then `v`, then `1`, then `0`, then `p`, then `x`.
43. Confirm `X` becomes `14`.
44. Press `1`, then `2`, then `v`, then `1`, then `0`, then `p`, then `y`.
45. Confirm `X` becomes `6`.
46. Press `0`, then `p`, then `z`.
47. Confirm `X` becomes `4294967295`, showing unsigned 32-bit `NOT`.
48. Press `2`, then `1`, then `4`, then `7`, then `4`, then `8`, then `3`, then `6`, then `4`, then `8`, then `v`, then `1`, then `p`, then `x`.
49. Confirm `X` becomes `2147483649`, showing the bitwise path treats values as unsigned instead of signed.
50. Press `1`, then `x`, then `p`, then `z`.
51. Confirm the calculator shows `domain error` because negative values are invalid for unsigned bitwise operations.
52. Press `1`, then `.`, then `5`, then `v`, then `3`, then `p`, then `.`.
53. Confirm the calculator shows `domain error` because bitwise ops require whole-number inputs.
54. Press `4`, then `2`, then `9`, then `4`, then `9`, then `6`, then `7`, then `2`, then `9`, then `6`, then `p`, then `z`.
55. Confirm the calculator shows `domain error` because values above `4294967295` are out of range.
56. Press `4`, then `2`, then `r`, then `5`.
57. Confirm `X` stays `42` after storing into register `5`.
58. Press `q`, then `5`.
59. Confirm `X` returns to `42` from register `5`.
60. Confirm `Y` keeps the prior `0`, showing direct recall lifts the stack instead of overwriting it.
61. Press `8`, then `8`, then `r`, then `v`.
62. Confirm `X` stays `88` after storing into register `e`.
63. Press `1`.
64. Confirm `X` changes to `1`, showing the stored register value is independent from the live stack.
65. Press `q`, then `v`.
66. Confirm `X` returns to `88` from register `e`.
67. Confirm `Y` becomes `1`, showing direct recall pushes the prior `X` value down the stack.
68. Press `6`, then `6`, then `r`, then `6`.
69. Confirm `X` stays `66` after storing into register `6`.
70. Press `6`, then `r`, then `7`.
71. Confirm `X` stays `6` after storing the pointer value into register `7`.
72. Press `p`, then `q`, then `7`.
73. Confirm `X` becomes `66`, showing indirect recall through register `7`.
74. Confirm `Y` keeps the prior `0`, showing indirect recall also lifts the stack.
75. Press `q`, then `7`.
76. Confirm `X` still shows `6`, showing pointer register `7` stays unchanged.
77. Confirm `Y` now shows `66`, showing direct recall pushes the prior `X` value down the stack.
78. Press `4`, then `4`, then `r`, then `6`.
79. Confirm `X` stays `44` after storing into register `6`.
80. Press `5`, then `r`, then `4`.
81. Confirm `X` stays `5` after storing the pointer value into register `4`.
82. Press `p`, then `q`, then `4`.
83. Confirm `X` becomes `44`, showing pointer register `4` pre-increments before use.
84. Press `q`, then `4`.
85. Confirm `X` becomes `6`, showing pointer register `4` was incremented.
86. Press `5`, then `5`, then `r`, then `5`.
87. Confirm `X` stays `55` after storing into register `5`.
88. Press `5`, then `r`, then `3`.
89. Confirm `X` stays `5` after storing the pointer value into register `3`.
90. Press `p`, then `q`, then `3`.
91. Confirm `X` becomes `55`, showing pointer register `3` uses its current value before changing.
92. Press `q`, then `3`.
93. Confirm `X` becomes `4`, showing pointer register `3` post-decrements after use.
94. Press `1`, then `2`, then `3`, then `r`, then `5`.
95. Confirm `X` stays `123` after storing into register `5`.
96. Press `2`, then `0`, then `.`, then `9`, then `r`, then `8`.
97. Confirm `X` stays `20.9` after storing the indirect pointer value into register `8`.
98. Press `p`, then `q`, then `8`.
99. Confirm `X` becomes `123`, showing indirect recall truncates the pointer value and wraps it across registers `0`-`e`.

## Entry Stack-Lift Check

This verifies the calculator now preserves the displayed `X` value when you begin typing a fresh number from display mode.

1. Press `2`, then `v`, then `3`, then `+`.
2. Confirm `X` shows `5`.
3. Press `4`.
4. Confirm `X` shows `4`.
5. Confirm `Y` still shows `5`.
6. Press `v`, then `6`.
7. Confirm the stack is `X=6`, `Y=4`, `Z=5`.

## 64-Bit Precision Spot Check

The firmware now fails to build if the toolchain does not provide a real 64-bit `double`, so a successful `pio run` already confirms the compiler-side requirement.

This extra keypad test is still useful on hardware because it checks that the calculator path is really preserving that precision end-to-end:

1. Press `1`, then `y`, then `8`.
2. Confirm `X` shows `1e+08`.
3. Press `v`, then `1`, then `+`.
4. Confirm `X` shows `100000001`.

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
- The `a` and `b` keys now control backlight brighter/dimmer again, `e` toggles the on-device help mode, and `c` is currently free for future extension work. While help mode is active, key presses show descriptions instead of performing their normal actions.
- The currently wired `F`-layer now includes trig, inverse trig, powers, exponentials, logarithms, `LAST X`, and the earlier `R↓`, `sqrt`, `1/x`, and `pi` functions.
- The currently wired `K`-layer now includes `INT`, `FRAC`, `max`, `|x|`, `sign`, the hour/minute conversion functions, `RND`, and the first indirect-register commands `RCLI` and `STOI`.
- The top status bar shows the current calculator mode (`RUN`, `ENT`, `EEX`, or `ERR`) on the left, optionally prefixed by `F` or `K` when a one-shot prefix is armed, and the most recent key/state event on the right. When the calculator is in an error state, the status bar shows the error text instead.
- The stack area now uses four uniform text rows without frames or separator lines.
