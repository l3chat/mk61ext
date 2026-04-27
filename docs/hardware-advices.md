# Hardware Advices

This document collects practical hardware suggestions for the next mk61ext board revisions.
It is based on the current firmware behavior, wiring constraints, and recent hardware tests.

## Current Known-Good Baseline

- MCU: Raspberry Pi Pico / RP2040.
- LCD (ST7565) historical wiring: `SCK=GPIO20`, `MOSI=GPIO21`, `CS=GPIO17`, `DC=GPIO19`, `RESET=GPIO18`.
- LCD (ST7565) current migration firmware target: `SCK=GPIO26`, `MOSI=GPIO27`, `CS=GPIO17`, `DC=GPIO19`, `RESET=GPIO18`.
- Legacy LCD lines `GPIO20/21` are intentionally set to high-impedance input mode in firmware because the old wires remain physically connected to the LCD bus. This prevents the old Pico pins from driving against the new `GPIO26/27` SCK/MOSI signals.
- Backlight control: `GPIO16` through a P-channel MOSFET.
- EEPROM: AT24C256-compatible on `GPIO14/15` (`SDA/SCL`), I2C address `0x50`.

## Timing Context (Firmware)

- The active firmware intentionally throttles the main loop with `2 ms` delay in normal calculator/UI mode and `100 ms` in display-sleep mode.
- During active program execution (`RUN`), the loop now uses no intentional delay so execution can proceed at full speed between screen-refresh gates.
- Keypad polling is timer-driven (`~2 ms` in normal mode, `~10 ms` in active `RUN`) instead of relying on raw loop frequency.
- During `RUN`, execution is no longer constrained by a fixed per-loop slice budget; it runs continuously and yields at stop-key checks and run-screen refresh checkpoints.
- Hardware testing showed `12 MHz` can make the UI effectively unresponsive, so firmware treats that saved EEPROM CPU setting as unsafe and recovers to the default clock instead.
- This delay is not for arithmetic correctness; it is for CPU/power control and stable UI cadence in the single-loop architecture.
- Measured key-to-result time therefore includes loop polling, keypad debounce, and display refresh time, not only the math operation itself.

## Soldering Guide (Current Board, No-Cut Migration)

This is the recommended procedure for your current board when you do not want to cut old LCD wires.

### 1. Target pin map for this migration

- LCD `SCK` -> `GPIO26`
- LCD `MOSI` -> `GPIO27`
- LCD `CS` -> `GPIO17` (keep existing)
- LCD `DC` -> `GPIO19` (keep existing)
- LCD `RESET` -> `GPIO18` (keep existing)
- Legacy LCD wires on `GPIO20/21` stay physically connected but are disabled in firmware (high-impedance input) so only the new clock/data pins drive the bus.

### 2. What to solder

- Add one jumper wire from LCD `SCK` pin to Pico `GPIO26`.
- Add one jumper wire from LCD `SI/MOSI` pin to Pico `GPIO27`.
- Do not remove existing LCD wires from `GPIO17/18/19/20/21`.
- Do not add any new wire to `GPIO18` or `GPIO19`.

### 3. Safe sequence

- Flash firmware that uses `SCK=26`, `MOSI=27`, and sets old `20/21` to input mode.
- Power off.
- Solder the two new jumpers (`SCK->26`, `MOSI->27`).
- Power on and test display.

### 4. Important warning

- Do not connect LCD `SCK/MOSI` to `GPIO18/19` while old `DC/RESET` wiring still exists there.
- That creates pin-role overlap (`SPI` vs `DC/RESET`) and can cause bus contention or unstable display behavior.

### 5. `IC_*` pins (`10-13`) if screen is blank until touched

- Symptom: after reset, LCD stays blank until you touch pins `10-13` by hand.
- This usually means those lines are floating on that module revision.
- Practical fix:
- bridge `IC_CS` to `CS`,
- bridge `IC_SCL` to `SCL`,
- bridge `IC_SI` to `SI`,
- leave `IC_SO` unconnected.
- Keep these links short and reflow solder on LCD pins `1-9` at the same time.
- Add/verify a local `100nF` decoupling capacitor close to LCD `VDD/VSS`.

## High-Impact Suggestions

### 1. Move backlight control off `GPIO16`

- Reason: `GPIO16` overlaps with the default Pico SPI pin group (`18/19/16/17`) and complicates clean hardware-SPI routing.
- Suggested target: any free GPIO outside the SPI block used for LCD.

### 2. Add explicit USB/external-power sense

- Current behavior infers battery vs external power from `VSYS`, which is heuristic.
- Suggested: route divided VBUS to a GPIO/ADC-safe input for deterministic source detection.
- Reason: timeout/backlight behavior will be more robust and easier to tune.

### 3. Keep robust EEPROM I2C defaults

- Keep `A0/A1/A2` tied low (`0x50`) and `WP` tied to GND unless write-protect is needed.
- Use pull-ups on SDA/SCL (typically `4.7k`; do not exceed `10k`).
- Place a local `100nF` decoupling capacitor close to EEPROM VCC/GND.

## Reliability Suggestions

### 1. Add local decoupling near each subsystem

- LCD power pins: at least one `100nF` ceramic close to the module connector.
- EEPROM: `100nF` close to the IC.
- RP2040 board rail entry: add local bulk capacitance near display/backlight load transitions.

### 2. Add service-friendly test points

- Recommended points: `3V3`, `GND`, `SDA`, `SCL`, LCD `SCK/MOSI/CS/DC/RST`, and one spare UART/debug pin.
- Reason: faster bring-up and easier fault isolation during assembly and field debugging.

### 3. Keep SWD access in the PCB layout

- Even if not populated in all builds, keep reachable `SWDIO`, `SWCLK`, `GND`, and reset.
- Reason: recovery is much easier when USB boot flow is unavailable.

## Optional Future Improvements

### 1. Keypad anti-ghosting diodes

- Current matrix scanning is fine for now, but diodes improve predictability with simultaneous key presses.

### 2. Hardware power-latch path

- Firmware sleep is already useful, but true hard power gating can improve battery shelf life for long storage.

## New Board Bring-Up Checklist

Use this sequence to reduce risk when assembling and validating the next board revision.

### 1. Pre-power visual checks

- Confirm no shorts between `3V3` and `GND` with a multimeter.
- Confirm LCD pins are not mirrored/rotated at the connector.
- Confirm EEPROM `A0/A1/A2` are tied low and `WP` is tied to `GND` (unless intentionally write-protected).
- Confirm I2C pull-ups exist on `SDA` and `SCL`.
- Confirm BOOTSEL and RESET access is physically reachable.

### 2. First power-on checks

- Power from USB only first (no battery) and confirm stable `3V3`.
- Verify RP2040 enumerates over USB.
- Verify BOOTSEL mode appears as a mass-storage device with label `RPI-RP2`.
- Do not connect the LCD backlight MOSFET gate to floating logic; ensure a defined state at boot.

### 3. Minimal firmware smoke test

- Build: `/home/lechat/.platformio/penv/bin/platformio run`
- Upload: `/home/lechat/.platformio/penv/bin/platformio run -t upload`
- Expected upload fallback behavior:
- if serial CDC is visible, `scripts/upload_uf2.sh` toggles 1200 baud and waits for `RPI-RP2`,
- if serial is not visible, manually hold BOOTSEL while plugging USB and retry upload.

### 4. Peripheral bring-up order

- First: display only (draw fixed text).
- Second: keypad scan only (print key events).
- Third: EEPROM read/write sanity check.
- Fourth: full firmware with power-management and settings.

### 5. Failure isolation order

- If upload fails with `Could not find a Pico serial port or an RPI-RP2 boot drive.`:
- check USB cable (data-capable), USB connector soldering, and BOOTSEL switch wiring first.
- If LCD is blank/garbled:
- check `CS/DC/RESET` polarity and wiring before tuning firmware timing.
- If keypad misses keys:
- verify row/column continuity and pull behavior before touching debounce constants.

## Suggested Pin Plan For Next Revision (Clean PCB)

For a clean next board revision without legacy no-cut constraints:

- LCD `SCK` -> `GPIO18`
- LCD `MOSI` -> `GPIO19`
- LCD `CS` -> `GPIO17`
- LCD `DC` -> `GPIO20`
- LCD `RESET` -> `GPIO21`
- Backlight gate -> move away from `GPIO16` to a free non-SPI GPIO
- EEPROM `SDA/SCL` -> keep `GPIO14/GPIO15`
