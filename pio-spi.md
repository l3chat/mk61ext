Work in the currently opened Arduino project and complete this task end-to-end.

Task:
Migrate the display transport from u8g2 built-in software SPI to Raspberry Pi Pico PIO-based SPI using Arduino-Pico SoftwareSPI, while preserving the existing display type, pins, rotation, and high-level drawing code.

Current display constructor to find:
U8G2_ST7565_ERC12864_F_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/ 20, /* data=*/ 21, /* cs=*/ 17, /* dc=*/ 19, /* reset=*/ 18);

Target result:
- Stop using u8g2’s built-in _4W_SW_SPI transport
- Use a custom u8x8 byte callback for SPI transfer
- Send bytes through Arduino-Pico SoftwareSPI
- Keep manual GPIO control for CS
- Keep the same display model, rotation, and pins unless there is a strong technical reason not to
- Keep all high-level u8g2 drawing code unchanged as much as possible

Platform/context:
- Board: Raspberry Pi Pico / RP2040
- Framework: Arduino
- Display library: u8g2
- Desired SPI transport: Arduino-Pico PIO-based SoftwareSPI
- Desired CS handling: manual GPIO CS control

Known pin mapping:
- SCK / clock = 20
- MOSI / data = 21
- CS = 17
- DC = 19
- RESET = 18

Project rules:
- Make the smallest reliable patch
- Do not refactor unrelated code
- Do not rename unrelated symbols
- Do not change application logic unless required for this migration
- Preserve existing drawing calls and buffer usage
- Prefer explicit GPIO handling for CS, DC, and RESET
- Add only short comments where necessary
- If there are multiple valid approaches, choose the simplest reliable one and explain why

First step: inspect the repository.
Search for:
- U8G2_ST7565_ERC12864_F_4W_SW_SPI
- U8G2_
- U8X8_
- u8g2.begin
- sendBuffer
- clearBuffer
- drawStr
- drawUTF8
- setup()
- loop()
- SPI
- SoftwareSPI
- display pin definitions
- any custom display init/reset/helper code

Before editing:
Give me a short plan based on the actual files you found. The plan must say:
- which file defines the display object
- where display initialization happens
- whether there are already helper functions for display pins, reset, delays, or GPIO
- the minimum viable migration strategy

Then apply the patch directly.

Implementation requirements:
1. Replace the existing constructor-based u8g2 SW SPI transport with a custom u8x8 byte callback transport.
2. The custom byte callback must handle at least:
   - U8X8_MSG_BYTE_INIT
   - U8X8_MSG_BYTE_SET_DC
   - U8X8_MSG_BYTE_START_TRANSFER
   - U8X8_MSG_BYTE_SEND
   - U8X8_MSG_BYTE_END_TRANSFER
3. Use Arduino-Pico SoftwareSPI for byte transfer.
4. Use GPIO for:
   - CS on pin 17
   - DC on pin 19
   - RESET on pin 18
5. Keep the display type and setup behavior equivalent to:
   - ST7565_ERC12864_F
   - U8G2_R0
6. Keep high-level u8g2 usage unchanged as much as possible:
   - begin/init
   - clearBuffer
   - draw calls
   - sendBuffer
7. Reuse existing helper code if it is already clean and appropriate.
8. Avoid changing pin assignments unless required by a proven constraint.

Architecture preference:
- Have a global or file-local SoftwareSPI instance configured for pins 20 and 21
- Manage CS manually with GPIO, not automatic SPI CS
- Manage DC explicitly in the byte callback
- Keep RESET in the GPIO/delay callback or existing init path, whichever is cleaner in this project
- Keep the patch readable and localized

Verification:
After patching, verify the result in the smallest practical way available in this repo:
- If there is an Arduino CLI / PlatformIO / existing build task, use it to compile
- Otherwise do a careful static sanity check and clearly state what was and was not verified

Deliverables:
When done, provide:
- a short summary of what changed
- the list of modified files
- the final transport architecture
- any assumptions made
- any compile/runtime risks still visible
- if compilation was not possible, explain exactly why

Important:
- Do not stop after analysis
- After the short plan, apply the patch directly
- Prefer a minimal patch over a broad rewrite
- If API differences force a design adjustment, make the smallest safe change and explain it
