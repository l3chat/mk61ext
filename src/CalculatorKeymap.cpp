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
    {'k', "F", "", ""},
    {'l', "SST", "JP X<0", "JPI X<0"},
    {'m', "BST", "JP X=0", "JPI X=0"},
    {'n', "RTN/0", "JP X>=0", "JPI X>=0"},
    {'o', "R/S", "JP X<>0", "JPI X<>0"},
    {'p', "K", "", ""},
    {'q', "RCL", "DSNZ0", "RCLI"},
    {'r', "STO", "DSNZ1", "STOI"},
    {'s', "GTO", "DSNZ2", "JPI"},
    {'t', "GSB/SST", "DSNZ3", "GSBI"},
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

CalculatorPrefix gActivePrefix = CalculatorPrefix::None;

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

CalculatorAction primaryActionForKey(char keyPressed) {
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
    case 'v':
      return CalculatorAction::Enter;
    case 'y':
      return CalculatorAction::EnterExponent;
    case 'x':
      return CalculatorAction::ChangeSign;
    case 'u':
      return CalculatorAction::SwapXY;
    case 'z':
      return CalculatorAction::ClearX;
    case 'c':
      return CalculatorAction::ClearAll;
    default:
      return CalculatorAction::None;
  }
}

CalculatorAction shiftedActionForKey(CalculatorPrefix prefix, char keyPressed) {
  switch (prefix) {
    case CalculatorPrefix::F:
      switch (keyPressed) {
        case '7':
          return CalculatorAction::Sin;
        case '8':
          return CalculatorAction::Cos;
        case '9':
          return CalculatorAction::Tan;
        case '.':
          return CalculatorAction::RollDown;
        case '4':
          return CalculatorAction::Asin;
        case '5':
          return CalculatorAction::Acos;
        case '6':
          return CalculatorAction::Atan;
        case '-':
          return CalculatorAction::SquareRoot;
        case '/':
          return CalculatorAction::Reciprocal;
        case '+':
          return CalculatorAction::Pi;
        case '*':
          return CalculatorAction::Square;
        case '1':
          return CalculatorAction::ExpE;
        case '2':
          return CalculatorAction::Log10;
        case '3':
          return CalculatorAction::NaturalLog;
        case 'u':
          return CalculatorAction::PowerXY;
        case 'v':
          return CalculatorAction::LastX;
        case '0':
          return CalculatorAction::Exp10;
        default:
          return CalculatorAction::None;
      }
    case CalculatorPrefix::K:
      switch (keyPressed) {
        case '7':
          return CalculatorAction::IntegerPart;
        case '8':
          return CalculatorAction::FractionalPart;
        case '9':
          return CalculatorAction::MaxXY;
        case '4':
          return CalculatorAction::AbsoluteValue;
        case '5':
          return CalculatorAction::Sign;
        default:
          return CalculatorAction::None;
      }
    case CalculatorPrefix::None:
      return CalculatorAction::None;
  }

  return CalculatorAction::None;
}

const char *shiftedAssignmentLabel(const CalculatorKeyAssignment *assignment, CalculatorPrefix prefix) {
  if (assignment == nullptr) {
    return "";
  }

  switch (prefix) {
    case CalculatorPrefix::F:
      return assignment->fShifted;
    case CalculatorPrefix::K:
      return assignment->kShifted;
    case CalculatorPrefix::None:
      return "";
  }

  return "";
}

bool hasPlannedShiftedAssignment(char keyPressed, CalculatorPrefix prefix) {
  const CalculatorKeyAssignment *assignment = plannedCalculatorKeyAssignment(keyPressed);
  const char *label = shiftedAssignmentLabel(assignment, prefix);
  return (label != nullptr) && (label[0] != '\0');
}

}  // namespace

CalculatorAction translateKeyToCalculatorAction(char keyPressed) {
  if (keyPressed == 'k') {
    gActivePrefix = CalculatorPrefix::F;
    return CalculatorAction::None;
  }

  if (keyPressed == 'p') {
    gActivePrefix = CalculatorPrefix::K;
    return CalculatorAction::None;
  }

  if (gActivePrefix != CalculatorPrefix::None) {
    const CalculatorPrefix prefix = gActivePrefix;
    gActivePrefix = CalculatorPrefix::None;

    const CalculatorAction shiftedAction = shiftedActionForKey(prefix, keyPressed);
    if (shiftedAction != CalculatorAction::None) {
      return shiftedAction;
    }

    if (hasPlannedShiftedAssignment(keyPressed, prefix)) {
      return CalculatorAction::None;
    }
  }

  return primaryActionForKey(keyPressed);
}

void resetCalculatorKeymapState() {
  gActivePrefix = CalculatorPrefix::None;
}

CalculatorPrefix activeCalculatorPrefix() {
  return gActivePrefix;
}

const char *activeCalculatorPrefixName() {
  switch (gActivePrefix) {
    case CalculatorPrefix::F:
      return "F";
    case CalculatorPrefix::K:
      return "K";
    case CalculatorPrefix::None:
      return "";
  }

  return "";
}

const char *calculatorLegend(uint8_t page) {
  switch (page) {
    case 0:
      return "vEnt yEex xChs zCx";
    case 1:
      return "uXy kF pK k.Srdn";
    case 2:
      return "k-Srt k/1/x k+Pi";
    case 3:
      return "k*Sq k0 10x kvLx";
    case 4:
      return "k7/8/9 trig k1/2/3";
    case 5:
      return "p7Int p8Frc p9Max";
    case 6:
      return "p4Abs p5Sign cClr";
    case 7:
      return "a+/b- trig=rad";
    default:
      return "";
  }
}

uint8_t calculatorLegendPageCount() {
  return 8;
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
