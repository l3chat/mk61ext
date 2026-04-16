# Program VM Draft

This note captures a proposed first design for mk61ext programmability.

The key correction is this:

- the user is holding a calculator
- programming happens by recording key sequences
- this is not a full-fledged programming environment

Any off-device text format, importer, or exporter should stay secondary to that calculator-first model.

## Core Principle

The primary user experience should be:

1. enter program mode
2. move a cursor through program memory
3. press keys to record program steps
4. run the stored program on the calculator

That means the design should start from recorded calculator actions, not from labels or a source-language-first workflow.

## Goals

- Keep programming faithful to the experience of using a programmable calculator.
- Reuse the existing calculator core as the execution/data engine.
- Keep room for compatibility with original MK-61-style listings later.
- Allow optional off-device tooling without making it the main model.
- Keep the stored program format compact, with every command encoded as either 1 byte or 2 bytes.

## User-Visible Model

The calculator should expose a small programming model, not a software-development environment.

Suggested visible concepts:

- program memory is a linear byte stream decoded as 1-byte and 2-byte commands
- `edit_pc` is the cursor while browsing/editing program memory
- `run_pc` is the execution cursor while the program runs
- `SST` and `BST` move only the edit cursor
- `SST` and `BST` move between decoded command starts, not individual bytes
- entering program mode changes what keypresses do: instead of acting on the current calculation, they are recorded into program memory
- addresses shown to the user are hexadecimal byte addresses

Important decision already confirmed:

- `SST` and `BST` do not execute code
- they are cursor movement only

## Encoding Constraint

Confirmed direction:

- every stored command is encoded as either 1 byte or 2 bytes
- program memory is addressed in base-16 bytes from `00` to `FF`
- maximum program size is therefore 256 bytes

That implies two separate views of the same program:

- a decoded command view for the human browsing program memory
- a compact byte-oriented encoding for storage and execution

So the calculator can remain step-oriented in the UI while the firmware stays byte-oriented internally.

The important compromise is:

- storage is byte-based
- browsing is command-based

## Program Browse UI

A practical single-line browse format could be:

```text
addr(:|>) op-code(s) decoded-op
```

Suggested meaning:

- `addr` is a 2-digit hexadecimal byte address
- `:` means an ordinary line
- `>` marks the currently active line
- `op-code(s)` shows the raw stored byte or bytes in hexadecimal
- `decoded-op` shows the human-readable meaning

Examples:

```text
00> 07        7
01: 1C        SIN
02: 41        MX 1
03: 51 20     GTO 20
```

In edit mode, `>` would normally mark `edit_pc`.

In run mode, `>` could mark `run_pc`.

If we later want to show both cursors at once, we can add a second marker, but that does not need to be part of v1.

The displayed `addr` should always be the byte address of the first byte of the decoded command shown on that line.

## What Gets Recorded

The user records a sequence of keypresses, but the firmware does not have to store raw keypad scan codes forever.

The better internal model is:

- the recorder consumes user keypresses
- prefix state and operand-entry state are resolved during recording
- the result is stored as a canonical program step with a fixed 1-byte or 2-byte encoding

So the user experience stays calculator-like, while the internal representation stays stable and executable.

Examples of front-panel input versus stored meaning:

| User presses in program mode | Stored meaning |
| --- | --- |
| `7` | digit-7 step |
| `k` then `7` | `sin` step |
| `q` then `1` | `MX 1` step |
| `r` then `x` | `XM b` step |
| `p` then `q` then `y` | `MXI c` step |
| `o` in program mode | halt step |

The key point is that the user still types keys, but the machine stores resolved program steps rather than temporary prefix keystrokes.

Under the hood, each committed command occupies one or two bytes in the program byte stream.

## Program Recorder

To make calculator-style entry work cleanly, program mode needs a small recorder state machine.

Suggested recorder states:

- idle
- waiting for `F`-shifted key
- waiting for `K`-shifted key
- waiting for register operand
- waiting for address high nibble
- waiting for address low nibble

Examples:

- pressing `k` in program mode should arm an `F`-recording prefix, not store a standalone `F` step
- pressing `q` should arm direct recall operand entry
- pressing `s` should arm jump-address entry
- when the needed trailing key or digits are entered, one resolved program step is committed at `edit_pc`

This keeps program memory meaningful and keeps `SST` / `BST` aligned with actual stored operations.

## Editing Semantics

Because program memory is byte-oriented but browsing is command-oriented, editing behavior needs to be explicit.

Recommended v1 rules:

- `edit_pc` always points to the first byte of a decoded command, or to the append position at the end of the program
- overwrite mode: entering a command at `edit_pc` replaces the decoded command currently under the cursor
- insert mode: entering a command at `edit_pc` inserts a new command at the cursor and shifts trailing bytes forward
- in insert mode, direct-address control-flow operands should be retargeted so existing `GTO` / `GSB` / direct conditional and `L0`-`L3` commands continue to point at the same logical destination after bytes shift
- if the new command is wider or narrower than the old one, the trailing bytes shift accordingly
- appending at the end inserts the new bytes at the current program length
- if a replacement or append would exceed `0x100` bytes total, the entry is rejected

This is the simplest model that matches the command-oriented UI while still respecting the byte-oriented storage format.

## Register Operand Entry

Register-bearing commands should reuse the existing mk61ext register-key mapping:

- `0`-`9` for registers `0`-`9`
- `.` for register `A`
- `x` for register `B`
- `y` for register `C`
- `z` for register `D`
- `v` for register `E`
- `u` for register `F`

Because direct and indirect register families fit in one byte, the recorder can show a pending nibble-style preview before commit.

Example direction:

```text
3A> 4_        MX _
3A> 4C        MX C
```

The first line is a pending entry after pressing `q`.

The second line is the committed result after pressing `y`.

## Address Operand Entry

Direct-address commands such as `GTO`, `GSB`, direct conditionals, and `Ln` need one full address byte.

Recommended v1 input model:

- the user enters the address as two hexadecimal nibbles
- high nibble first, then low nibble
- the resulting byte is stored as the second byte of the command

Recommended hex-digit key mapping:

- `0`-`9` map to `0`-`9`
- `.` maps to `A`
- `x` maps to `B`
- `y` maps to `C`
- `z` maps to `D`
- `v` maps to `E`
- `u` maps to `F`

This keeps address entry aligned with the active register-letter mapping, so the same `. x y z v u` sequence consistently means `A-F`.

Example direction:

```text
3E> 51 __     GTO __
3E> 51 A_     GTO A_
3E> 51 A4     GTO A4
```

The first line is the pending state after pressing `s`.

The second line is the pending state after entering the high nibble.

The third line is the committed 2-byte command after entering the low nibble.

## VM Direction

There is still a VM, but it should be thought of as a replay engine for recorded calculator steps, not as a standalone assembly-language machine.

Suggested ownership split:

- `RpnCalculator`: stack, numeric entry, registers, math, errors
- `ProgramVm`: stored program steps, edit cursor, run cursor, call stack, execution state

That means the VM executes the same kinds of actions the user can record from the keypad.

Those decoded actions should map directly onto compact 1-byte or 2-byte stored commands.

## VM State

Suggested minimum VM state:

- `program[]`
- `edit_pc`
- `run_pc`
- `running`
- `call_stack`
- `last_vm_error`
- reference or ownership of one `RpnCalculator`

Important separation:

- `edit_pc` is for browsing and editing program memory
- `run_pc` is for execution

Because stored commands are variable-width, both cursors should most likely be byte offsets into program memory rather than simple step indexes.

`SST` should decode the command at `edit_pc`, then advance by its width.

`BST` should locate the previous decoded command start. With only 256 bytes of program memory, a simple scan from `00` to `edit_pc` is acceptable in v1.

## Step Families

The stored program should contain calculator-style steps.

A practical split is:

- 1-byte commands for operations with no extra operand, and also for compact operand forms such as register-family opcodes
- 2-byte commands for operations that need a full extra byte, most notably absolute branch targets

### 1. Plain Calculator Steps

These correspond directly to existing calculator actions.

Examples:

- digit entry
- decimal point
- `ENTER`
- `EEX`
- `CHS`
- arithmetic operations
- trig and inverse trig
- stack ops
- clear ops

This is the easy part because the calculator core already knows how to execute most of them.

### 2. Register Steps

These need an explicit register operand.

Examples:

- `MX n`
- `XM n`
- `MXI n`
- `XMI n`

The user records them by pressing the command key and then the register key, but the stored step should remember the resolved register index.

These are strong candidates for 1-byte commands, not 2-byte commands.

Because there are now 16 exposed registers (`0`-`f`), a direct or indirect register operation can still fit naturally into a single byte by combining:

- the operation family
- the 4-bit register index

That makes register-bearing steps a good compression target.

### 3. Flow-Control Steps

These need an address operand or special control behavior.

Examples:

- `GTO addr`
- `GSB addr`
- `RTN`
- `R/S` as a stored halt/run-control step
- `NOP`

`RTN`, `R/S`, and `NOP` are natural 1-byte commands.

`GTO` and `GSB` are natural 2-byte commands because one opcode byte plus one absolute target-address byte is enough for a `00`-`FF` program space.

### 4. Conditional Steps

Examples:

- `JP X<0 addr`
- `JP X=0 addr`
- `JP X>=0 addr`
- `JP X<>0 addr`
- `Ln addr`

Again, the user enters these through keys, but the VM executes them as resolved conditional program steps.

These are also likely 2-byte commands.

For `Ln`, the register selector may be encodable in the opcode byte while the branch target stays in the second byte.

## Recommended Opcode Map

The opcode map should stay close to the original MK-61 wherever that does not conflict with the mk61ext design decisions already made.

In practice, "close to the original" should mean:

- preserve the original code neighborhoods for the major instruction families
- preserve original single-byte encodings where the corresponding operation still exists
- preserve the original register-family block layout
- preserve the original control and branch block around `0x50`
- use unused or extension space only where divergence is actually needed

### High-Level Layout

| Range | Proposed use | Direction |
| --- | --- | --- |
| `00`-`0E` | digits, decimal, sign, exponent, clear, enter | keep close to original |
| `0F`-`2F` | arithmetic, stack, `F`-layer scientific ops | keep close to original where practical |
| `30`-`3F` | `K`-layer utilities, time, boolean, random | keep close to original where practical |
| `40`-`4F` | direct `MX` family across registers `0-F` | extend family to full 16-register set |
| `50`-`5F` | halt / return / direct jump / direct conditional / `L0`-`L3` / invalid slots | match original neighborhood |
| `60`-`6F` | direct `XM` family across registers `0-F` | extend family to full 16-register set |
| `70`-`7F` | indirect/nonzero-jump family across registers `0-F` | extend selector space to full 16-register set |
| `80`-`8F` | indirect unconditional jump family across registers `0-F` | extend selector space to full 16-register set |
| `90`-`9F` | indirect/nonnegative-jump family across registers `0-F` | extend selector space to full 16-register set |
| `A0`-`AF` | indirect subroutine-call family across registers `0-F` | extend selector space to full 16-register set |
| `B0`-`BF` | indirect store family across registers `0-F` | extend family to full 16-register set |
| `C0`-`CF` | indirect/negative-jump family across registers `0-F` | extend selector space to full 16-register set |
| `D0`-`DF` | indirect recall family across registers `0-F` | extend family to full 16-register set |
| `E0`-`EF` | indirect/zero-jump family across registers `0-F` | extend selector space to full 16-register set |
| `F0`-`FF` | mk61ext-only extension space | reserve for explicit divergence |

### Low Region `00`-`3F`

For the low region, the recommendation should now be concrete for the operations we already implement.

The goal here is:

- use the original MK-61 codes where the meaning still matches
- define unimplemented or uncertain core slots as explicit `INVALID` opcodes rather than inventing new semantics too early
- leave the opcode map readable to anyone comparing it with original MK-61 references

| Opcode | Proposed meaning | Notes |
| --- | --- | --- |
| `00` | `0` | original match |
| `01` | `1` | original match |
| `02` | `2` | original match |
| `03` | `3` | original match |
| `04` | `4` | original match |
| `05` | `5` | original match |
| `06` | `6` | original match |
| `07` | `7` | original match |
| `08` | `8` | original match |
| `09` | `9` | original match |
| `0A` | `.` | original match |
| `0B` | `CHS` | original match |
| `0C` | `EEX` | original match |
| `0D` | `CX` | original match |
| `0E` | `ENTER` | original match |
| `0F` | `LASTX` | original match for stored `F ENTER` |
| `10` | `+` | original match |
| `11` | `-` | original match |
| `12` | `*` | original match |
| `13` | `/` | original match |
| `14` | `x<->y` | original match |
| `15` | `10^x` | original match |
| `16` | `e^x` | original match |
| `17` | `lg` | original match |
| `18` | `ln` | original match |
| `19` | `asin` | original match |
| `1A` | `acos` | original match |
| `1B` | `atan` | inferred original slot; keep this value |
| `1C` | `sin` | original match |
| `1D` | `cos` | original match |
| `1E` | `tan` | original match |
| `1F` | `INVALID` | defined invalid opcode in v1 |
| `20` | `pi` | original match |
| `21` | `sqrt` | original match |
| `22` | `x^2` | original match |
| `23` | `1/x` | original match |
| `24` | `x^y` | original match |
| `25` | `Rdown` | original match |
| `26` | `H.M->H` | original match |
| `27` | `INVALID` | keep original undefined feel, but define behavior |
| `28` | `INVALID` | keep original undefined feel, but define behavior |
| `29` | `INVALID` | keep original undefined feel, but define behavior |
| `2A` | `H.M.S->H` | original match |
| `2B` | `INVALID` | defined invalid opcode in v1 |
| `2C` | `INVALID` | defined invalid opcode in v1 |
| `2D` | `INVALID` | defined invalid opcode in v1 |
| `2E` | `INVALID` | defined invalid opcode in v1 |
| `2F` | `INVALID` | defined invalid opcode in v1 |
| `30` | `H->H.M.S` | original match |
| `31` | `|x|` | original match |
| `32` | `sign` | original match |
| `33` | `H->H.M` | original match |
| `34` | `INT` | original match |
| `35` | `FRAC` | original match |
| `36` | `max` | original match |
| `37` | `AND` | original match |
| `38` | `OR` | original match |
| `39` | `XOR` | original match |
| `3A` | `NOT` | original match |
| `3B` | `RND` | original match |
| `3C` | `INVALID` | defined invalid opcode in v1 |
| `3D` | `INVALID` | defined invalid opcode in v1 |
| `3E` | `INVALID` | defined invalid opcode in v1 |
| `3F` | `INVALID` | defined invalid opcode in v1 |

This gives mk61ext a very strong compatibility story in the single-byte instruction space used by ordinary calculator operations and the currently implemented `K`-layer utilities.

### Register Families

These should stay especially close to the original because the fit is excellent and the importer story becomes much simpler.

| Opcode | Proposed meaning |
| --- | --- |
| `40` | `MX 0` |
| `41` | `MX 1` |
| `42` | `MX 2` |
| `43` | `MX 3` |
| `44` | `MX 4` |
| `45` | `MX 5` |
| `46` | `MX 6` |
| `47` | `MX 7` |
| `48` | `MX 8` |
| `49` | `MX 9` |
| `4A` | `MX A` |
| `4B` | `MX B` |
| `4C` | `MX C` |
| `4D` | `MX D` |
| `4E` | `MX E` |
| `4F` | `MX F` |
| `60` | `XM 0` |
| `61` | `XM 1` |
| `62` | `XM 2` |
| `63` | `XM 3` |
| `64` | `XM 4` |
| `65` | `XM 5` |
| `66` | `XM 6` |
| `67` | `XM 7` |
| `68` | `XM 8` |
| `69` | `XM 9` |
| `6A` | `XM A` |
| `6B` | `XM B` |
| `6C` | `XM C` |
| `6D` | `XM D` |
| `6E` | `XM E` |
| `6F` | `XM F` |
| `B0` | `XMI 0` |
| `B1` | `XMI 1` |
| `B2` | `XMI 2` |
| `B3` | `XMI 3` |
| `B4` | `XMI 4` |
| `B5` | `XMI 5` |
| `B6` | `XMI 6` |
| `B7` | `XMI 7` |
| `B8` | `XMI 8` |
| `B9` | `XMI 9` |
| `BA` | `XMI A` |
| `BB` | `XMI B` |
| `BC` | `XMI C` |
| `BD` | `XMI D` |
| `BE` | `XMI E` |
| `BF` | `XMI F` |
| `D0` | `MXI 0` |
| `D1` | `MXI 1` |
| `D2` | `MXI 2` |
| `D3` | `MXI 3` |
| `D4` | `MXI 4` |
| `D5` | `MXI 5` |
| `D6` | `MXI 6` |
| `D7` | `MXI 7` |
| `D8` | `MXI 8` |
| `D9` | `MXI 9` |
| `DA` | `MXI A` |
| `DB` | `MXI B` |
| `DC` | `MXI C` |
| `DD` | `MXI D` |
| `DE` | `MXI E` |
| `DF` | `MXI F` |

That gives direct and indirect register operations a compact 1-byte representation, with the high nibble selecting the family and the low nibble carrying the register index.

### Control And Branch Block

This should remain intentionally recognizable to anyone familiar with MK-61 listings.

| Opcode | Proposed meaning | Width |
| --- | --- | --- |
| `50` | `HALT` | 1 byte |
| `51 aa` | `GTO aa` | 2 bytes |
| `52` | `RETURN` | 1 byte |
| `53 aa` | `GSB aa` | 2 bytes |
| `54` | `NOP` | 1 byte |
| `55` | `INVALID` | 1 byte |
| `56` | `INVALID` | 1 byte |
| `57 aa` | `JP X<>0 aa` | 2 bytes |
| `58 aa` | `L2 aa` | 2 bytes |
| `59 aa` | `JP X>=0 aa` | 2 bytes |
| `5A aa` | `L3 aa` | 2 bytes |
| `5B aa` | `L1 aa` | 2 bytes |
| `5C aa` | `JP X<0 aa` | 2 bytes |
| `5D aa` | `L0 aa` | 2 bytes |
| `5E aa` | `JP X=0 aa` | 2 bytes |
| `5F` | `INVALID` | 1 byte |

This is deliberately very close to the original MK-61 arrangement, except that branch targets are now hexadecimal byte addresses in the `00`-`FF` space instead of the original decimal-step display model.

### Indirect Jump Families

These also fit the original MK-61 organization very well.

| Opcode | Proposed meaning |
| --- | --- |
| `70` | `JPI X<>0 0` |
| `71` | `JPI X<>0 1` |
| `72` | `JPI X<>0 2` |
| `73` | `JPI X<>0 3` |
| `74` | `JPI X<>0 4` |
| `75` | `JPI X<>0 5` |
| `76` | `JPI X<>0 6` |
| `77` | `JPI X<>0 7` |
| `78` | `JPI X<>0 8` |
| `79` | `JPI X<>0 9` |
| `7A` | `JPI X<>0 A` |
| `7B` | `JPI X<>0 B` |
| `7C` | `JPI X<>0 C` |
| `7D` | `JPI X<>0 D` |
| `7E` | `JPI X<>0 E` |
| `7F` | `JPI X<>0 F` |
| `80` | `JPI 0` |
| `81` | `JPI 1` |
| `82` | `JPI 2` |
| `83` | `JPI 3` |
| `84` | `JPI 4` |
| `85` | `JPI 5` |
| `86` | `JPI 6` |
| `87` | `JPI 7` |
| `88` | `JPI 8` |
| `89` | `JPI 9` |
| `8A` | `JPI A` |
| `8B` | `JPI B` |
| `8C` | `JPI C` |
| `8D` | `JPI D` |
| `8E` | `JPI E` |
| `8F` | `JPI F` |
| `90` | `JPI X>=0 0` |
| `91` | `JPI X>=0 1` |
| `92` | `JPI X>=0 2` |
| `93` | `JPI X>=0 3` |
| `94` | `JPI X>=0 4` |
| `95` | `JPI X>=0 5` |
| `96` | `JPI X>=0 6` |
| `97` | `JPI X>=0 7` |
| `98` | `JPI X>=0 8` |
| `99` | `JPI X>=0 9` |
| `9A` | `JPI X>=0 A` |
| `9B` | `JPI X>=0 B` |
| `9C` | `JPI X>=0 C` |
| `9D` | `JPI X>=0 D` |
| `9E` | `JPI X>=0 E` |
| `9F` | `JPI X>=0 F` |
| `A0` | `GSBI 0` |
| `A1` | `GSBI 1` |
| `A2` | `GSBI 2` |
| `A3` | `GSBI 3` |
| `A4` | `GSBI 4` |
| `A5` | `GSBI 5` |
| `A6` | `GSBI 6` |
| `A7` | `GSBI 7` |
| `A8` | `GSBI 8` |
| `A9` | `GSBI 9` |
| `AA` | `GSBI A` |
| `AB` | `GSBI B` |
| `AC` | `GSBI C` |
| `AD` | `GSBI D` |
| `AE` | `GSBI E` |
| `AF` | `GSBI F` |
| `C0` | `JPI X<0 0` |
| `C1` | `JPI X<0 1` |
| `C2` | `JPI X<0 2` |
| `C3` | `JPI X<0 3` |
| `C4` | `JPI X<0 4` |
| `C5` | `JPI X<0 5` |
| `C6` | `JPI X<0 6` |
| `C7` | `JPI X<0 7` |
| `C8` | `JPI X<0 8` |
| `C9` | `JPI X<0 9` |
| `CA` | `JPI X<0 A` |
| `CB` | `JPI X<0 B` |
| `CC` | `JPI X<0 C` |
| `CD` | `JPI X<0 D` |
| `CE` | `JPI X<0 E` |
| `CF` | `JPI X<0 F` |
| `E0` | `JPI X=0 0` |
| `E1` | `JPI X=0 1` |
| `E2` | `JPI X=0 2` |
| `E3` | `JPI X=0 3` |
| `E4` | `JPI X=0 4` |
| `E5` | `JPI X=0 5` |
| `E6` | `JPI X=0 6` |
| `E7` | `JPI X=0 7` |
| `E8` | `JPI X=0 8` |
| `E9` | `JPI X=0 9` |
| `EA` | `JPI X=0 A` |
| `EB` | `JPI X=0 B` |
| `EC` | `JPI X=0 C` |
| `ED` | `JPI X=0 D` |
| `EE` | `JPI X=0 E` |
| `EF` | `JPI X=0 F` |

### Invalid Opcode Rule

For v1, every byte in the core `00`-`EF` space is now defined as either:

- a valid command opcode, or
- `INVALID`

`INVALID` means:

- the recorder never emits that byte
- the browser decodes it explicitly as invalid
- the VM stops with an opcode error if execution reaches it

That closes the core byte map without using placeholder "reserved" meanings outside the true mk61ext extension area.

### Extension Policy

Any genuinely mk61ext-only future features should prefer `F0`-`FF` first rather than disturbing the historically recognizable families above.

That keeps:

- the core calculator and programming map familiar
- import/export simpler
- hex dumps easier to compare against original MK-61 references

## Execution Semantics

Suggested rules:

- `SST` and `BST` affect only `edit_pc`
- execution uses only `run_pc`
- after a normal step, `run_pc` advances
- branch and subroutine steps modify `run_pc`
- subroutine call pushes a return address onto `call_stack`
- return pops from `call_stack`
- a stored halt step stops execution
- calculator errors or VM errors stop execution and leave `run_pc` on the failing step

Advancing either cursor means decoding the current command width and moving by 1 or 2 bytes.

This means the VM should execute command-aligned addresses only; jumps into the second byte of a 2-byte command must be treated as invalid program state.

## Textual Representation

This is where the previous draft was too aggressive.

Text should be treated as a secondary representation for:

- tests
- debugging
- import/export
- documentation

It should not be the primary user model.

So, instead of designing around a label-heavy assembly language first, the safer direction is:

1. define stored program steps
2. define how the keypad recorder creates them
3. define how the VM executes them
4. define the exact opcode map close to the original MK-61 layout
5. only then define an optional text listing format

## Listing Format Direction

If we want an off-device textual form later, it should probably look like a program listing rather than a conventional assembler source file.

Example direction:

```text
00: 1
01: ENTER
02: 2
03: *
04: MX 1
05: GTO 12
07: HALT
```

Because addresses are byte addresses, a 2-byte command consumes two addresses in the underlying stream even though it appears as one decoded line in the higher-level UI.

This kind of format matches the mental model of calculator program memory much better than a free-form source file with labels as the primary abstraction.

For debugging or low-level inspection, the listing can also optionally include raw bytes:

```text
00: 01      1
01: 0E      ENTER
02: 02      2
03: 12      *
04: 41      MX 1
05: 51 12   GTO 12
07: 50      HALT
```

Labels could still exist later as an optional convenience in tools, but they should not define the core design.

If needed, a lower-level byte dump can exist as a secondary export format, but the decoded listing should remain the default human-facing representation.

## Importer Direction

Yes, an importer still looks possible.

But the target of the importer should be:

- canonical stored program steps

not:

- a primary source-language environment

Likely useful importer inputs:

- original MK-61-style mnemonic listings
- mk61ext textual program listings
- exported step-by-step key sequences

A practical importer pipeline would be:

1. parse the external listing
2. resolve it into canonical stored program steps
3. encode those steps into the 1-byte or 2-byte command format
4. load that encoded stream into program memory

An exporter can do the reverse.

Keeping the opcode families close to the original MK-61 should make some imported commands map directly or almost directly into mk61ext bytes, especially in the register and branch neighborhoods.

The low `00`-`3F` region should also import cleanly for the currently implemented calculator/scientific/utility subset because the recommended v1 map now deliberately follows the original opcode values there too.

## Important Open Decision

One design point still needs care:

- should program memory store fully resolved steps only
- or should it preserve more literal key-sequence detail for some operations

My recommendation is:

- store resolved steps
- encode each resolved step as a 1-byte or 2-byte command
- keep the key-sequence behavior in the recorder, not in the VM format

That gives us:

- stable execution
- cleaner browsing with `SST` / `BST`
- easier import/export
- less coupling to future keyboard-label changes
- a compact program-memory representation

## Recommended v1

For a first implementation, I would keep it modest:

- a linear program memory
- separate `edit_pc` and `run_pc`
- `SST` / `BST` are cursor-only
- a recorder state machine for prefixes and operands
- stored canonical commands encoded as 1 or 2 bytes
- VM execution layered on top of `RpnCalculator`
- no source-language-first workflow
- text import/export deferred until the stored-step format is stable

## Next Design Step

The next useful document should pin down:

1. the exact 1-byte and 2-byte command encoding
2. the recorder state machine in program mode
3. address-entry behavior for jumps and calls
4. what the display shows while browsing and recording program steps
