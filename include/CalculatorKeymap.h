#ifndef CALCULATOR_KEYMAP_H
#define CALCULATOR_KEYMAP_H

#include "RpnCalculator.h"

CalculatorAction translateKeyToCalculatorAction(char keyPressed);
const char *calculatorLegend(uint8_t page);
uint8_t calculatorLegendPageCount();

#endif
