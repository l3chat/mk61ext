#include "CalculatorKeymap.h"

namespace {

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
