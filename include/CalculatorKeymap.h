#ifndef CALCULATOR_KEYMAP_H
#define CALCULATOR_KEYMAP_H

#include <cstddef>

#include "RpnCalculator.h"

enum class CalculatorPrefix : uint8_t {
  None,
  F,
  K,
};

struct CalculatorKeyAssignment {
  char key;
  const char *primary;
  const char *fShifted;
  const char *kShifted;
};

CalculatorAction translateKeyToCalculatorAction(char keyPressed);
void resetCalculatorKeymapState();
CalculatorPrefix activeCalculatorPrefix();
const char *activeCalculatorPrefixName();
const char *calculatorKeyHelpLabel(char keyPressed, CalculatorPrefix prefix);
const char *calculatorKeyHelpDescription(char keyPressed, CalculatorPrefix prefix);
const char *calculatorLegend(uint8_t page);
uint8_t calculatorLegendPageCount();

// Planned full-keyboard assignment data for the mk61ext keypad.
// This is a design/reference table; the active firmware still implements only a run-mode subset.
const CalculatorKeyAssignment *plannedCalculatorKeyAssignments();
size_t plannedCalculatorKeyAssignmentCount();
const CalculatorKeyAssignment *plannedCalculatorKeyAssignment(char keyPressed);

#endif
