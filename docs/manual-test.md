# Manual Hardware Test

This checklist matches the current firmware scaffold in `src/main.cpp`.

## Flash

```bash
/home/lechat/.platformio/penv/bin/platformio run -t upload
```

## Expected Boot Behavior

1. Power on or reset the Pico.
2. The display should show `memory found` for about 3 seconds.
3. After the splash screen, the main screen should show:
   - four stack lines labeled `T:`, `Z:`, `Y:`, and `X:` or `X>`,
   - a status line showing the last key event or a calculator error,
   - a bottom legend line cycling through temporary key-help pages.

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
2. Confirm `evt:` updates to the matching key character and `PRESS`.
3. Confirm `act:` rises while the key is held and returns to `0` after release.
4. Hold the same key for more than 0.5 seconds.
5. Confirm `evt:` changes to `HOLD`.
6. Release the key.
7. Confirm `evt:` changes to `REL`.

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
- `e`: `ENTER`
- `d`: `EEX`
- `s`: `CHS` (change sign)
- `u`: `x↔y`
- `v`: `R↓` (stack roll-down)
- `x`: `CX`
- `q`: square root
- `r`: reciprocal (`1/x`)
- `p`: `pi`
- `c`: clear all
- `a` / `b`: backlight brighter / dimmer

1. Press `2`, then `e`, then `3`, then `+`.
2. Confirm the `X` register shows `5`.
3. Press `9`, then `e`, then `4`, then `/`.
4. Confirm the `X` register shows `2.25`.
5. Press `s`.
6. Confirm the `X` register changes sign.
7. Press `x`.
8. Confirm `X` resets to `0`.
9. Press `c`.
10. Confirm `X`, `Y`, `Z`, and `T` all reset to `0`.
11. Press `8`, then `e`, then `2`, then `u`.
12. Confirm `X` and `Y` swap values.
13. Press `3`, then `e`, then `4`, then `e`, then `5`.
14. Confirm the stack is `X=5`, `Y=4`, `Z=3`.
15. Press `v`.
16. Confirm the stack rolls down to `X=4`, `Y=3`, `Z=0`, `T=5`.
17. Press `9`, then `q`.
18. Confirm `X` becomes `3`.
19. Press `4`, then `r`.
20. Confirm `X` becomes `0.25`.
21. Press `p`.
22. Confirm `X` becomes approximately `3.141592654`.
23. Press `1`, then `d`, then `3`.
24. Confirm `X` becomes `1000`.
25. Press `1`, then `d`, then `s`, then `2`.
26. Confirm `X` becomes `0.01`.

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
- The bottom legend line on-screen now rotates through several short key-help pages. The full provisional mapping is listed above.
