# Example Programs

These small programs are meant to smoke-test the current programmable-calculator stack end to end:

- program entry in `PRG` mode,
- decoded listing display,
- direct and indirect register access,
- direct and indirect control flow,
- `L0`-`L3` loop control,
- `GSB` / `RETURN`,
- and ordinary calculator arithmetic while a stored program is running.

They intentionally stay small enough to type in by hand on the current hardware.

## Before You Start

- Program memory lives in RAM while you edit. Press `g` to save the current program to EEPROM, and press `h` to restore the saved program when you want it back.
- Enter program mode with `k`, then `y` (`F PRG`).
- Return to calculator mode with `k`, then `x` (`F RUN`).
- Run the current program with `o`.
- Reset the run address to `00` with `n`.
- Program addresses are hexadecimal byte addresses.
- During address or register entry, `. x y z v u` mean `A B C D E F`.

Each listing below uses four columns:

- `Addr`: byte address shown by the program editor
- `Keys`: raw keys to press while recording that step
- `Bytes`: expected stored opcode bytes
- `Meaning`: decoded instruction text

## 1. Factorial `n!`

This version handles `0! = 1` and works for whole numbers `n >= 0`.

What it exercises:

- direct conditional branch
- `L0`
- `GTO`
- direct `XM` / `MX`
- multiplication

Setup before `RUN`:

- put `n` in `X`

Expected result:

- `X = n!`

| Addr | Keys | Bytes | Meaning |
| --- | --- | --- | --- |
| `00` | `k m 0 v` | `5E 0E` | `JP X=0 0E` |
| `02` | `r 0` | `60` | `XM 0` |
| `03` | `r 1` | `61` | `XM 1` |
| `04` | `k q 0 8` | `5D 08` | `L0 08` |
| `06` | `q 1` | `41` | `MX 1` |
| `07` | `o` | `50` | `HALT` |
| `08` | `q 1` | `41` | `MX 1` |
| `09` | `q 0` | `40` | `MX 0` |
| `0A` | `*` | `12` | `*` |
| `0B` | `r 1` | `61` | `XM 1` |
| `0C` | `s 0 4` | `51 04` | `GTO 04` |
| `0E` | `1` | `01` | `1` |
| `0F` | `o` | `50` | `HALT` |

Quick checks:

- `0` should produce `1`
- `1` should produce `1`
- `5` should produce `120`

## 2. Triangular Number `1 + 2 + ... + n`

This sums the integers from `1` through `n`. It returns `0` when `n = 0`.

What it exercises:

- direct conditional branch
- `L0`
- addition
- direct register storage

Setup before `RUN`:

- put `n` in `X`

Expected result:

- `X = n * (n + 1) / 2`

| Addr | Keys | Bytes | Meaning |
| --- | --- | --- | --- |
| `00` | `k m 0 y` | `5E 0C` | `JP X=0 0C` |
| `02` | `r 0` | `60` | `XM 0` |
| `03` | `0` | `00` | `0` |
| `04` | `r 1` | `61` | `XM 1` |
| `05` | `q 1` | `41` | `MX 1` |
| `06` | `q 0` | `40` | `MX 0` |
| `07` | `+` | `10` | `+` |
| `08` | `r 1` | `61` | `XM 1` |
| `09` | `k q 0 5` | `5D 05` | `L0 05` |
| `0B` | `q 1` | `41` | `MX 1` |
| `0C` | `o` | `50` | `HALT` |

Quick checks:

- `0` should produce `0`
- `1` should produce `1`
- `5` should produce `15`

## 3. Absolute Value `abs(x)`

This is a tiny direct-conditional smoke test.

What it exercises:

- direct `JP X<0`
- `CHS`

Setup before `RUN`:

- put the test value in `X`

Expected result:

- `X = abs(input)`

| Addr | Keys | Bytes | Meaning |
| --- | --- | --- | --- |
| `00` | `k l 0 3` | `5C 03` | `JP X<0 03` |
| `02` | `o` | `50` | `HALT` |
| `03` | `x` | `0B` | `CHS` |
| `04` | `o` | `50` | `HALT` |

Quick checks:

- `-3` should produce `3`
- `0` should stay `0`
- `7` should stay `7`

## 4. Indirect Jump Smoke Test

This is the smallest useful `JPI` example.

What it exercises:

- selector-register target lookup
- indirect unconditional jump

Setup before `RUN`:

1. In calculator mode, store `3` into register `2`.
2. Clear `X` back to `0` if you want a cleaner final stack display.

Expected result:

- execution jumps to address `03`
- `X` ends as `1`

| Addr | Keys | Bytes | Meaning |
| --- | --- | --- | --- |
| `00` | `p s 2` | `82` | `JPI 2` |
| `01` | `9` | `09` | `9` |
| `02` | `o` | `50` | `HALT` |
| `03` | `1` | `01` | `1` |
| `04` | `o` | `50` | `HALT` |

## 5. Indirect Subroutine Smoke Test

This is the smallest useful `GSBI` example.

What it exercises:

- selector-register target lookup
- indirect subroutine call
- `RETURN`

Setup before `RUN`:

1. In calculator mode, store `2` into register `1`.
2. Clear `X` back to `0` if you want a cleaner final stack display.

Expected result:

- the subroutine at address `02` runs
- `RETURN` comes back to the caller
- `X` ends as `1`

| Addr | Keys | Bytes | Meaning |
| --- | --- | --- | --- |
| `00` | `p t 1` | `A1` | `GSBI 1` |
| `01` | `o` | `50` | `HALT` |
| `02` | `1` | `01` | `1` |
| `03` | `n` | `52` | `RETURN` |

## 6. Indirect Sign Classifier

This example returns `-1`, `0`, or `1` using only indirect conditional jumps.

What it exercises:

- indirect `JPI X<0`
- indirect `JPI X=0`
- indirect `JPI X>=0`
- selector-register targets

Setup before `RUN`:

1. Store `4` into register `0`.
2. Store `7` into register `1`.
3. Store `9` into register `2`.
4. Enter the test value in `X`.

Expected result:

- negative input returns `-1`
- zero returns `0`
- positive input returns `1`

| Addr | Keys | Bytes | Meaning |
| --- | --- | --- | --- |
| `00` | `p l 0` | `C0` | `JPI X<0 0` |
| `01` | `p m 1` | `E1` | `JPI X=0 1` |
| `02` | `p n 2` | `92` | `JPI X>=0 2` |
| `03` | `o` | `50` | `HALT` |
| `04` | `1` | `01` | `1` |
| `05` | `x` | `0B` | `CHS` |
| `06` | `o` | `50` | `HALT` |
| `07` | `0` | `00` | `0` |
| `08` | `o` | `50` | `HALT` |
| `09` | `1` | `01` | `1` |
| `0A` | `o` | `50` | `HALT` |

Quick checks:

- input `-3` should produce `-1`
- input `0` should produce `0`
- input `5` should produce `1`

## Notes

- These examples are written against the current runner behavior, where the full current core branch families now execute.
- Direct and indirect branch targets must land on real decoded step boundaries. Jumping into the second byte of a 2-byte instruction still stops execution with a VM target error.
- Untaken conditional branches do not validate their targets, so a bad target in a selector register only matters if the branch is actually taken.
