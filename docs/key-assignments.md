# Key Assignments

This file captures the planned full-keyboard assignment for the custom 8x5 `mk61ext` matrix.

It is intentionally separate from [manual-test.md](./manual-test.md): the manual test tracks what the firmware implements today, while this file records the broader target layout, including `F`- and `K`-shifted functions.

## Scope

- The current firmware now implements a growing calculator-mode subset of the matrix below, while most programming features are still planned only.
- The planned layout below is a design/reference table for future keypad and mode work.
- The original MK-61 keyboard is treated as a 6x5 block and is now mapped onto the bottom six rows of the custom 8x5 matrix.
- The top two rows, `a`-`j`, are reserved as mk61ext extension space.

## Label Direction

The short labels in this file are working mnemonics, not final cap text.

Assignments that are not yet implemented are marked in-place as *`like this`*.

The `a`-`j` extension block is still the long-term mk61ext extension area, but the current firmware currently uses `a`-`f` for a practical settings/help cluster.

That distinction matters because several labels are faithful to MK-61 shorthand but are not especially readable on first contact:

- `JP`, `JPI`, `DSNZ`, `RCLI`, `STOI`, and `GSBI` are compact programming mnemonics, but they hide a lot of behavior.
- `H->H.M`, `H.M->H`, `H->H.M.S`, and `H.M.S->H` are technically descriptive, but they are visually dense.
- `RUN`, `PRG`, and `CF` are probably too terse for final front-panel legends unless the surrounding mode model is very clear.
- The trigonometric labels `tg`, `asin`, `acos`, and `atan` are historically grounded, but we may still want a more modern labeling pass before printing or engraving anything.

## Planned Matrix

### Extension Rows

These rows are reserved for mk61ext-specific features in the long term, but a few keys still have temporary live firmware assignments:

| Raw key | Primary | Meaning |
| --- | --- | --- |
| `a` | `SET PREV` | Current firmware selects the previous item in the dedicated settings screen; the longer-term extension slot is still open. |
| `b` | `SET NEXT` | Current firmware selects the next item in the dedicated settings screen; the longer-term extension slot is still open. |
| `c` | `SET -` | Current firmware decreases or toggles the selected settings item; the longer-term extension slot is still open. |
| `d` | `SET +` | Current firmware increases or toggles the selected settings item; the longer-term extension slot is still open. |
| `e` | `SETTINGS` | Current firmware opens the settings screen; on that screen `e` saves staged brightness, idle-timeout, sleep-timeout, angle, label, and CPU-frequency changes to EEPROM. The longer-term extension slot is still open. |
| `f` | `HELP` | Current firmware toggles help mode on this key outside settings; while the settings screen is open, the same key cancels staged changes and restores the earlier values. The longer-term extension slot is still open. |
| `g` | *`EXT7`* | Reserved for mk61ext-specific features. |
| `h` | *`EXT8`* | Reserved for mk61ext-specific features. |
| `i` | *`EXT9`* | Reserved for mk61ext-specific features. |
| `j` | *`EXT10`* | Reserved for mk61ext-specific features. |

### Bottom Six Rows

This 6x5 block is the planned MK-61-style keyboard:

#### Row `k`-`o`

| Raw key | Primary | What it does | `F`-shifted | What it does | `K`-shifted | What it does |
| --- | --- | --- | --- | --- | --- | --- |
| `k` | `F` | Prefix key that selects the `F` layer for the next keypress. | `—` | No second-level assignment planned here. | `—` | No second-level assignment planned here. |
| `l` | *`SST`* | Step forward by one program step while inspecting program memory. | *`JP X<0`* | Conditional program branch keyed on `X<0`. | *`JPI X<0`* | Indirect conditional branch using an address/register reference. |
| `m` | *`BST`* | Step backward by one program step while inspecting program memory. | *`JP X=0`* | Conditional program branch keyed on `X=0`. | *`JPI X=0`* | Indirect conditional branch keyed on `X=0`. |
| `n` | *`RTN/0`* | Return from subroutine in program mode, or reset the program counter to `00` in calculator mode. | *`JP X>=0`* | Conditional branch keyed on `X>=0`. | *`JPI X>=0`* | Indirect conditional branch keyed on `X>=0`. |
| `o` | *`R/S`* | Run or stop a program; in program mode this is the halt command. | *`JP X<>0`* | Conditional branch keyed on `X<>0`. | *`JPI X<>0`* | Indirect conditional branch keyed on `X<>0`. |

#### Row `p`-`t`

| Raw key | Primary | What it does | `F`-shifted | What it does | `K`-shifted | What it does |
| --- | --- | --- | --- | --- | --- | --- |
| `p` | `K` | Prefix key that selects the `K` layer for the next keypress. | `—` | No second-level assignment planned here. | `—` | No second-level assignment planned here. |
| `q` | `RCL` | Recall a register value into `X` while lifting the stack. Use `0`-`9`, or `. x y z v` for registers `a`-`e`. | *`DSNZ0`* | Decrement register `0` and branch if it stays non-zero. | `RCLI` | Indirect register recall through a pointer register while lifting the stack. The pointer value uses its whole-number part wrapped across registers `0`-`e`; pointer registers `4`-`6` pre-increment and `0`-`3` post-decrement. |
| `r` | `STO` | Store `X` into a register. Use `0`-`9`, or `. x y z v` for registers `a`-`e`. | *`DSNZ1`* | Decrement register `1` and branch if it stays non-zero. | `STOI` | Indirect register store through a pointer register. The pointer value uses its whole-number part wrapped across registers `0`-`e`; pointer registers `4`-`6` pre-increment and `0`-`3` post-decrement. |
| `s` | *`GTO`* | Jump to the following program address. | *`DSNZ2`* | Decrement register `2` and branch if it stays non-zero. | *`JPI`* | Indirect jump using an address held in a register. |
| `t` | *`GSB/SST`* | In calculator mode, single-step a program; in program mode, call a subroutine. | *`DSNZ3`* | Decrement register `3` and branch if it stays non-zero. | *`GSBI`* | Indirect subroutine call using an address held in a register. |

#### Row `7`-`/`

| Raw key | Primary | What it does | `F`-shifted | What it does | `K`-shifted | What it does |
| --- | --- | --- | --- | --- | --- | --- |
| `7` | `7` | Enter digit `7`. | `sin` | Sine of `X`. | `INT` | Keep only the integer part of `X`. |
| `8` | `8` | Enter digit `8`. | `cos` | Cosine of `X`. | `FRAC` | Keep only the fractional part of `X`. |
| `9` | `9` | Enter digit `9`. | `tg` | Tangent of `X`. | `max` | Replace `X` with the larger of `X` and `Y` without dropping the stack. |
| `-` | `-` | Subtract `X` from `Y`. | `sqrt` | Square root of `X`. | `—` | Original MK-61 marks this as undefined and error-producing. |
| `/` | `/` | Divide `Y` by `X`. | `1/x` | Reciprocal of `X`. | `—` | Original MK-61 marks this as undefined and error-producing. |

#### Row `4`-`*`

| Raw key | Primary | What it does | `F`-shifted | What it does | `K`-shifted | What it does |
| --- | --- | --- | --- | --- | --- | --- |
| `4` | `4` | Enter digit `4`. | `asin` | Inverse sine of `X`. | `|x|` | Absolute value of `X`. |
| `5` | `5` | Enter digit `5`. | `acos` | Inverse cosine of `X`. | `sign` | Signum of `X`: `-1`, `0`, or `1`. |
| `6` | `6` | Enter digit `6`. | `atan` | Inverse tangent of `X`. | `H->H.M` | Convert decimal hours to `hours.minutes`. |
| `+` | `+` | Add `Y` and `X`. | `pi` | Load the constant pi into `X`. | `H.M->H` | Convert `hours.minutes` back to decimal hours. |
| `*` | `*` | Multiply `Y` by `X`. | `x^2` | Square `X`. | `—` | Original MK-61 marks this as undefined and error-producing. |

#### Row `1`-`v`

| Raw key | Primary | What it does | `F`-shifted | What it does | `K`-shifted | What it does |
| --- | --- | --- | --- | --- | --- | --- |
| `1` | `1` | Enter digit `1`. | `e^x` | Exponential of `X`. | `—` | No original MK-61 assignment on this shifted slot. |
| `2` | `2` | Enter digit `2`. | `lg` | Base-10 logarithm of `X`. | `—` | No original MK-61 assignment on this shifted slot. |
| `3` | `3` | Enter digit `3`. | `ln` | Natural logarithm of `X`. | `H->H.M.S` | Convert decimal hours to `hours.minutes.seconds`. |
| `u` | `x<->y` | Swap the `X` and `Y` registers. | `x^y` | Power operation using the stack operands. | `H.M.S->H` | Convert `hours.minutes.seconds` back to decimal hours. |
| `v` | `ENTER` | Push the stack and finish the current numeric entry. | `LAST X` | Recall the saved `Last X` value. | `RND` | Generate a random number in the interval `0<=X<1`. |

#### Row `0`-`z`

| Raw key | Primary | What it does | `F`-shifted | What it does | `K`-shifted | What it does |
| --- | --- | --- | --- | --- | --- | --- |
| `0` | `0` | Enter digit `0`. | `10^x` | Raise `10` to the power `X`. | *`NOP`* | Explicit no-operation program instruction. |
| `.` | `.` | Enter the decimal point while typing a number. | `Rdown` | Rotate the stack downward. | `AND` | Binary `AND` on decimal whole numbers from `0` to `4294967295`, using unsigned 32-bit integer semantics and a decimal result. |
| `x` | `CHS` | Change the sign of the current `X` value or exponent entry. | *`RUN`* | Leave programming mode and return to normal calculator mode. | `OR` | Binary `OR` on decimal whole numbers from `0` to `4294967295`, using unsigned 32-bit integer semantics and a decimal result. |
| `y` | `EEX` | Start exponent entry for the current number. | *`PRG`* | Enter programming mode. | `XOR` | Binary `XOR` on decimal whole numbers from `0` to `4294967295`, using unsigned 32-bit integer semantics and a decimal result. |
| `z` | `CX` | Delete the last entry character while entering a number; otherwise clear the `X` register. | *`CF`* | Clear the active prefix key or pending prefix state. | `NOT` | Unary binary `NOT` on a decimal whole number from `0` to `4294967295`, using unsigned 32-bit integer semantics and a decimal result. |

## Notes

- `F` and `K` are prefix keys in the original calculator model; their own shifted columns are intentionally blank.
- Blank shifted entries mean there is no direct original MK-61 assignment I want to claim yet.
- The `a`-`j` block is now explicitly reserved for mk61ext-specific extensions above the MK-61-style bottom six rows.
- The planned matrix is also available in code through `plannedCalculatorKeyAssignments()` in [CalculatorKeymap.cpp](../src/CalculatorKeymap.cpp).

## Label Review Hotspots

These are the places where I expect the labels to need another pass before they become final legends:

- `JP`, `JPI`, and `DSNZ` are historically accurate but opaque unless the user already thinks in program-control mnemonics.
- `RCLI`, `STOI`, and `GSBI` are accurate shorthand for indirect addressing, but they are poor first-contact labels.
- `H->H.M`, `H.M->H`, `H->H.M.S`, and `H.M.S->H` describe the conversions, but the punctuation-heavy format will be hard to read on a cramped key cap.
- `RUN`, `PRG`, and `CF` probably want more user-facing wording once the mode model is settled.
- `Rdown` is descriptive, but a future legend may want to align visually with the rest of the stack-operation terminology.
- `tg` mirrors the original terminology, but `tan` may be more immediately readable if we are not trying to stay strict to the original face labeling.

## References

- MK-61 command reference: https://thimet.de/CalcCollection/Calculators/Elektronika-MK-61/CmdRef.html
- English MK-61 operating instructions: https://www.wass.net/manuals/Elektronika%20MK-61%20English.pdf
