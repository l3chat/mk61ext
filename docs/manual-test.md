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
   - a running timer at the top,
   - `act:<n> br:<value>` on the first diagnostics line,
   - `evt:<key> <state>` on the second diagnostics line.

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
2. Confirm the backlight brightens in steps and `br:` moves toward `256`.
3. Press `b` repeatedly.
4. Confirm the backlight dims in steps and `br:` drops toward `0`.
5. Press `d`.
6. Confirm the timer resets to near `00.00.00`.

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
- The current firmware is still a hardware bring-up scaffold, not the final calculator UI.
