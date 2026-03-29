#include "CalculatorKeymap.h"

namespace {

constexpr CalculatorKeyAssignment kPlannedAssignments[] = {
    {'a', "EXT1", "", ""},
    {'b', "EXT2", "", ""},
    {'c', "EXT3", "", ""},
    {'d', "EXT4", "", ""},
    {'e', "EXT5", "", ""},
    {'f', "EXT6", "", ""},
    {'g', "EXT7", "", ""},
    {'h', "EXT8", "", ""},
    {'i', "EXT9", "", ""},
    {'j', "EXT10", "", ""},
    {'k', "SST", "JP X<0", "JPI X<0"},
    {'l', "BST", "JP X=0", "JPI X=0"},
    {'m', "RTN/0", "JP X>=0", "JPI X>=0"},
    {'n', "R/S", "JP X<>0", "JPI X<>0"},
    {'o', "RCL", "DSNZ0", "RCLI"},
    {'p', "STO", "DSNZ1", "STOI"},
    {'q', "GTO", "DSNZ2", "JPI"},
    {'r', "GSB/SST", "DSNZ3", "GSBI"},
    {'s', "F", "", ""},
    {'t', "K", "", ""},
    {'7', "7", "sin", "INT"},
    {'8', "8", "cos", "FRAC"},
    {'9', "9", "tg", "max"},
    {'-', "-", "sqrt", ""},
    {'/', "/", "1/x", ""},
    {'4', "4", "asin", "|x|"},
    {'5', "5", "acos", "sign"},
    {'6', "6", "atan", "H->H.M"},
    {'+', "+", "pi", "H.M->H"},
    {'*', "*", "x^2", ""},
    {'1', "1", "e^x", ""},
    {'2', "2", "lg", ""},
    {'3', "3", "ln", "H->H.M.S"},
    {'u', "x<->y", "x^y", "H.M.S->H"},
    {'v', "ENTER", "LAST X", "RND"},
    {'0', "0", "10^x", "NOP"},
    {'.', ".", "Rdown", "AND"},
    {'x', "CHS", "RUN", "OR"},
    {'y', "EEX", "PRG", "XOR"},
    {'z', "CX", "CF", "NOT"},
};

CalculatorAction digitToAction(char keyPressed) {
  switch (keyPressed) {
    case '0':
      return CalculatorAction::Digit0;
    case '1':
      return CalculatorAction::Digit1;
    case '2':
      return CalculatorAction::Digit2;
    case '3':
      return CalculatorAction::Digit3;
    case '4':
      return CalculatorAction::Digit4;
    case '5':
      return CalculatorAction::Digit5;
    case '6':
      return CalculatorAction::Digit6;
    case '7':
      return CalculatorAction::Digit7;
    case '8':
      return CalculatorAction::Digit8;
    case '9':
      return CalculatorAction::Digit9;
    default:
      return CalculatorAction::None;
  }
}

}  // namespace

CalculatorAction translateKeyToCalculatorAction(char keyPressed) {
  const CalculatorAction digitAction = digitToAction(keyPressed);
  if (digitAction != CalculatorAction::None) {
    return digitAction;
  }

  switch (keyPressed) {
    case '.':
      return CalculatorAction::DecimalPoint;
    case '+':
      return CalculatorAction::Add;
    case '-':
      return CalculatorAction::Subtract;
    case '*':
      return CalculatorAction::Multiply;
    case '/':
      return CalculatorAction::Divide;
    case 'e':
      return CalculatorAction::Enter;
    case 'd':
      return CalculatorAction::EnterExponent;
    case 's':
      return CalculatorAction::ChangeSign;
    case 'q':
      return CalculatorAction::SquareRoot;
    case 'r':
      return CalculatorAction::Reciprocal;
    case 'p':
      return CalculatorAction::Pi;
    case 'x':
      return CalculatorAction::ClearX;
    case 'c':
      return CalculatorAction::ClearAll;
    case 'u':
      return CalculatorAction::SwapXY;
    case 'v':
      return CalculatorAction::RollDown;
    default:
      return CalculatorAction::None;
  }
}

const char *calculatorLegend(uint8_t page) {
  switch (page) {
    case 0:
      return "eEnt dEex uXy vRdn";
    case 1:
      return "sChs xCx qSrt r1/x";
    case 2:
      return "pPi cClr a+/b-";
    default:
      return "";
  }
}

uint8_t calculatorLegendPageCount() {
  return 3;
}

const CalculatorKeyAssignment *plannedCalculatorKeyAssignments() {
  return kPlannedAssignments;
}

size_t plannedCalculatorKeyAssignmentCount() {
  return sizeof(kPlannedAssignments) / sizeof(kPlannedAssignments[0]);
}

const CalculatorKeyAssignment *plannedCalculatorKeyAssignment(char keyPressed) {
  for (const CalculatorKeyAssignment &assignment : kPlannedAssignments) {
    if (assignment.key == keyPressed) {
      return &assignment;
    }
  }

  return nullptr;
}
