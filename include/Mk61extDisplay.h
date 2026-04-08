#pragma once

#include <U8g2lib.h>

class Mk61extDisplay : public U8G2 {
 public:
  Mk61extDisplay(const u8g2_cb_t *rotation,
                 uint8_t clock,
                 uint8_t data,
                 uint8_t cs,
                 uint8_t dc,
                 uint8_t reset = U8X8_PIN_NONE);
};
