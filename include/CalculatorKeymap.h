#ifndef CALCULATOR_KEYMAP_H
#define CALCULATOR_KEYMAP_H

#include "RpnCalculator.h"

CalculatorAction translateKeyToCalculatorAction(char keyPressed);
const char *calculatorPrimaryLegend();
const char *calculatorSecondaryLegend();

#endif
