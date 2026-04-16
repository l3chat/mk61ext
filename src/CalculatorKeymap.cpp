#include "CalculatorKeymap.h"

#include <cstring>

namespace {

constexpr CalculatorKeyAssignment kPlannedAssignments[] = {
    {'a', "EXT1", "", ""},
    {'b', "EXT2", "", ""},
    {'c', "EXT3", "", ""},
    {'d', "EXT4", "", ""},
    {'e', "EXT5", "", ""},
    {'f', "EXT6", "", ""},
    {'g', "SAVE PRG", "", ""},
    {'h', "LOAD PRG", "", ""},
    {'i', "PRG INS", "", ""},
    {'j', "EXT10", "", ""},
    {'k', "F", "", ""},
    {'l', "SST", "JP X<0", "JPI X<0"},
    {'m', "BST", "JP X=0", "JPI X=0"},
    {'n', "RTN/0", "JP X>=0", "JPI X>=0"},
    {'o', "R/S", "JP X<>0", "JPI X<>0"},
    {'p', "K", "", ""},
    {'q', "MX", "L0", "MXI"},
    {'r', "XM", "L1", "XMI"},
    {'s', "GTO", "L2", "JPI"},
    {'t', "GSB/SST", "L3", "GSBI"},
    {'7', "7", "sin", "INT"},
    {'8', "8", "cos", "FRAC"},
    {'9', "9", "tg", "max"},
    {'-', "-", "sqrt", ""},
    {'/', "/", "1/x", ""},
    {'4', "4", "asin", "|x|"},
    {'5', "5", "acos", "sign"},
    {'6', "6", "atan", "H->H.M"},
    {'+', "+", "pi", "H.M->H"},
    {'*', "*", "x^2", "DROP"},
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

const char *shiftedAssignmentLabel(const CalculatorKeyAssignment *assignment, CalculatorPrefix prefix);

bool labelEquals(const char *label, const char *expected) {
  return std::strcmp(label, expected) == 0;
}

const char *helpLabelForKey(char keyPressed, CalculatorPrefix prefix) {
  if (keyPressed == 'a') {
    return "PREV";
  }

  if (keyPressed == 'b') {
    return "NEXT";
  }

  if (keyPressed == 'c') {
    return "DEC";
  }

  if (keyPressed == 'd') {
    return "INC";
  }

  if (keyPressed == 'e') {
    return "SETTINGS";
  }

  if (keyPressed == 'f') {
    return "HELP";
  }

  const CalculatorKeyAssignment *assignment = plannedCalculatorKeyAssignment(keyPressed);
  if (assignment == nullptr) {
    return "";
  }

  if (prefix == CalculatorPrefix::None) {
    return assignment->primary;
  }

  const char *label = shiftedAssignmentLabel(assignment, prefix);
  return (label != nullptr) ? label : "";
}

const char *helpDescriptionForLabel(const char *label) {
  if ((label == nullptr) || (label[0] == '\0')) {
    return "";
  }

  if (std::strncmp(label, "EXT", 3) == 0) {
    return "Reserved for a future mk61ext-specific function.";
  }

  if ((label[0] >= '0') && (label[0] <= '9') && (label[1] == '\0')) {
    static char digitDescriptions[][16] = {
        "Enter digit 0.",
        "Enter digit 1.",
        "Enter digit 2.",
        "Enter digit 3.",
        "Enter digit 4.",
        "Enter digit 5.",
        "Enter digit 6.",
        "Enter digit 7.",
        "Enter digit 8.",
        "Enter digit 9.",
    };
    return digitDescriptions[label[0] - '0'];
  }

  if (labelEquals(label, "PREV")) {
    return "While the settings screen is open, move the selection to the previous setting.";
  }
  if (labelEquals(label, "NEXT")) {
    return "While the settings screen is open, move the selection to the next setting.";
  }
  if (labelEquals(label, "DEC")) {
    return "While the settings screen is open, decrease or toggle the selected setting.";
  }
  if (labelEquals(label, "INC")) {
    return "While the settings screen is open, increase or toggle the selected setting.";
  }
  if (labelEquals(label, "SETTINGS")) {
    return "Open the settings screen. There you can preview brightness, backlight timeout, sleep timeout, angle mode, labels, and CPU frequency. Use a/b to select, c/d to change, e to save, and f to cancel.";
  }
  if (labelEquals(label, "HELP")) {
    return "Toggle help mode on or off. In help mode, key presses show descriptions instead of acting. Press f again to leave help. While the settings screen is open, f cancels and restores the earlier values.";
  }
  if (labelEquals(label, "SAVE PRG")) {
    return "Save the current program to EEPROM.";
  }
  if (labelEquals(label, "LOAD PRG")) {
    return "Restore the saved EEPROM program, replacing the current program.";
  }
  if (labelEquals(label, "PRG INS")) {
    return "In program mode, toggle between overwrite and insert editing. The status line shows OVR or INS. In insert mode, direct-address jumps and calls are retargeted after each insertion.";
  }
  if (labelEquals(label, "F")) {
    return "Arm the one-shot F prefix. Press the next key to inspect or use its F-shifted meaning.";
  }
  if (labelEquals(label, "K")) {
    return "Arm the one-shot K prefix. Press the next key to inspect or use its K-shifted meaning.";
  }
  if (labelEquals(label, "SST")) {
    return "Step forward by one program step while inspecting stored code.";
  }
  if (labelEquals(label, "BST")) {
    return "Step backward by one program step while inspecting stored code.";
  }
  if (labelEquals(label, "RTN/0")) {
    return "Return from a subroutine in program mode, or reset the program counter to 00 in run mode.";
  }
  if (labelEquals(label, "R/S")) {
    return "Run or stop a stored program.";
  }
  if (labelEquals(label, "MX")) {
    return "Arm direct memory-to-X recall. Press 0-9, or use . x y z v u for registers a-f. The recalled value lifts the stack into X.";
  }
  if (labelEquals(label, "XM")) {
    return "Arm direct X-to-memory store. Press 0-9, or use . x y z v u for registers a-f.";
  }
  if (labelEquals(label, "GTO")) {
    return "Jump to the specified program address.";
  }
  if (labelEquals(label, "GSB/SST")) {
    return "Execute one stored program step from the current run address. In program mode, record a GSB subroutine call.";
  }
  if (labelEquals(label, ".")) {
    return "Enter the decimal point while typing a number.";
  }
  if (labelEquals(label, "+")) {
    return "Add Y and X.";
  }
  if (labelEquals(label, "-")) {
    return "Subtract X from Y.";
  }
  if (labelEquals(label, "*")) {
    return "Multiply Y and X.";
  }
  if (labelEquals(label, "/")) {
    return "Divide Y by X.";
  }
  if (labelEquals(label, "sin")) {
    return "Take the sine of X using the active angle mode.";
  }
  if (labelEquals(label, "cos")) {
    return "Take the cosine of X using the active angle mode.";
  }
  if (labelEquals(label, "tg")) {
    return "Take the tangent of X using the active angle mode.";
  }
  if (labelEquals(label, "sqrt")) {
    return "Take the square root of X.";
  }
  if (labelEquals(label, "1/x")) {
    return "Replace X with its reciprocal.";
  }
  if (labelEquals(label, "asin")) {
    return "Take the inverse sine of X and express the result in the active angle mode.";
  }
  if (labelEquals(label, "acos")) {
    return "Take the inverse cosine of X and express the result in the active angle mode.";
  }
  if (labelEquals(label, "atan")) {
    return "Take the inverse tangent of X and express the result in the active angle mode.";
  }
  if (labelEquals(label, "pi")) {
    return "Load the constant pi into X.";
  }
  if (labelEquals(label, "x^2")) {
    return "Square X.";
  }
  if (labelEquals(label, "e^x")) {
    return "Raise e to the power of X.";
  }
  if (labelEquals(label, "lg")) {
    return "Take the base-10 logarithm of X.";
  }
  if (labelEquals(label, "ln")) {
    return "Take the natural logarithm of X.";
  }
  if (labelEquals(label, "x<->y")) {
    return "Swap the X and Y registers.";
  }
  if (labelEquals(label, "x^y")) {
    return "Raise one stack operand to the power of the other using the MK-61 stack order.";
  }
  if (labelEquals(label, "ENTER")) {
    return "Push the stack and finish the current numeric entry.";
  }
  if (labelEquals(label, "LAST X")) {
    return "Recall the saved Last X value from the previous operation.";
  }
  if (labelEquals(label, "10^x")) {
    return "Raise 10 to the power of X.";
  }
  if (labelEquals(label, "Rdown")) {
    return "Rotate the four-level stack downward.";
  }
  if (labelEquals(label, "CHS")) {
    return "Change the sign of X or of the exponent entry.";
  }
  if (labelEquals(label, "RUN")) {
    return "Return to normal run mode from programming mode.";
  }
  if (labelEquals(label, "EEX")) {
    return "Start entering the exponent for the current number.";
  }
  if (labelEquals(label, "PRG")) {
    return "Enter programming mode. Use g to save the current program and h to restore the saved program.";
  }
  if (labelEquals(label, "CX")) {
    return "Delete the last entry character while typing a number. Outside entry, clear the X register.";
  }
  if (labelEquals(label, "CF")) {
    return "Clear the currently armed prefix state.";
  }
  if (labelEquals(label, "INT")) {
    return "Keep only the integer part of X.";
  }
  if (labelEquals(label, "FRAC")) {
    return "Keep only the fractional part of X.";
  }
  if (labelEquals(label, "max")) {
    return "Replace X with the larger of X and Y without dropping the stack.";
  }
  if (labelEquals(label, "|x|")) {
    return "Replace X with its absolute value.";
  }
  if (labelEquals(label, "sign")) {
    return "Replace X with its sign: -1, 0, or 1.";
  }
  if (labelEquals(label, "H->H.M")) {
    return "Convert decimal hours to hours.minutes.";
  }
  if (labelEquals(label, "H.M->H")) {
    return "Convert hours.minutes back to decimal hours.";
  }
  if (labelEquals(label, "H->H.M.S")) {
    return "Convert decimal hours to hours.minutes.seconds.";
  }
  if (labelEquals(label, "H.M.S->H")) {
    return "Convert hours.minutes.seconds back to decimal hours.";
  }
  if (labelEquals(label, "RND")) {
    return "Generate a random number between 0 and 1.";
  }
  if (labelEquals(label, "NOP")) {
    return "Programming no-operation instruction.";
  }
  if (labelEquals(label, "AND")) {
    return "Bitwise AND on Y and X, using unsigned 32-bit whole-number values and a decimal result.";
  }
  if (labelEquals(label, "OR")) {
    return "Bitwise OR on Y and X, using unsigned 32-bit whole-number values and a decimal result.";
  }
  if (labelEquals(label, "XOR")) {
    return "Bitwise XOR on Y and X, using unsigned 32-bit whole-number values and a decimal result.";
  }
  if (labelEquals(label, "NOT")) {
    return "Bitwise NOT on X, using unsigned 32-bit whole-number values and a decimal result.";
  }
  if (labelEquals(label, "DROP")) {
    return "Drop X and shift Y to X, Z to Y, and T to Z.";
  }
  if (labelEquals(label, "JP X<0")) {
    return "Planned direct conditional jump when X is less than zero.";
  }
  if (labelEquals(label, "JP X=0")) {
    return "Planned direct conditional jump when X equals zero.";
  }
  if (labelEquals(label, "JP X>=0")) {
    return "Planned direct conditional jump when X is zero or positive.";
  }
  if (labelEquals(label, "JP X<>0")) {
    return "Planned direct conditional jump when X is not zero.";
  }
  if (labelEquals(label, "JPI X<0")) {
    return "Planned indirect conditional jump when X is less than zero.";
  }
  if (labelEquals(label, "JPI X=0")) {
    return "Planned indirect conditional jump when X equals zero.";
  }
  if (labelEquals(label, "JPI X>=0")) {
    return "Planned indirect conditional jump when X is zero or positive.";
  }
  if (labelEquals(label, "JPI X<>0")) {
    return "Planned indirect conditional jump when X is not zero.";
  }
  if (labelEquals(label, "L0")) {
    return "Planned loop control: decrement register 0 and branch while it stays non-zero.";
  }
  if (labelEquals(label, "L1")) {
    return "Planned loop control: decrement register 1 and branch while it stays non-zero.";
  }
  if (labelEquals(label, "L2")) {
    return "Planned loop control: decrement register 2 and branch while it stays non-zero.";
  }
  if (labelEquals(label, "L3")) {
    return "Planned loop control: decrement register 3 and branch while it stays non-zero.";
  }
  if (labelEquals(label, "MXI")) {
    return "Arm indirect memory-to-X recall. Pick a pointer register with 0-9 or . x y z v u; 4-6 pre-increment and 0-3 post-decrement. The recalled value lifts the stack into X.";
  }
  if (labelEquals(label, "XMI")) {
    return "Arm indirect X-to-memory store. Pick a pointer register with 0-9 or . x y z v u; 4-6 pre-increment and 0-3 post-decrement.";
  }
  if (labelEquals(label, "JPI")) {
    return "Planned indirect jump using an address held in a register.";
  }
  if (labelEquals(label, "GSBI")) {
    return "Planned indirect subroutine call using an address held in a register.";
  }

  return "Planned MK-61 key assignment.";
}

const char *missingShiftDescription(CalculatorPrefix prefix) {
  switch (prefix) {
    case CalculatorPrefix::F:
      return "No F-shifted assignment is planned for this key.";
    case CalculatorPrefix::K:
      return "No K-shifted assignment is planned for this key.";
    case CalculatorPrefix::None:
      return "No action is currently assigned to this key.";
  }

  return "No action is currently assigned to this key.";
}

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
        case '6':
          return CalculatorAction::HourToHourMinute;
        case '+':
          return CalculatorAction::HourMinuteToHour;
        case '3':
          return CalculatorAction::HourToHourMinuteSecond;
        case 'u':
          return CalculatorAction::HourMinuteSecondToHour;
        case 'v':
          return CalculatorAction::RandomValue;
        case '.':
          return CalculatorAction::BitwiseAnd;
        case 'x':
          return CalculatorAction::BitwiseOr;
        case 'y':
          return CalculatorAction::BitwiseXor;
        case 'z':
          return CalculatorAction::BitwiseNot;
        case '*':
          return CalculatorAction::Drop;
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

const char *calculatorKeyHelpLabel(char keyPressed, CalculatorPrefix prefix) {
  const char *label = helpLabelForKey(keyPressed, prefix);
  if (label[0] != '\0') {
    return label;
  }

  if (prefix != CalculatorPrefix::None) {
    return "n/a";
  }

  return "";
}

const char *calculatorKeyHelpDescription(char keyPressed, CalculatorPrefix prefix) {
  const char *label = helpLabelForKey(keyPressed, prefix);
  if (label[0] != '\0') {
    return helpDescriptionForLabel(label);
  }

  return missingShiftDescription(prefix);
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
      return "p4Abs p5Sign p*Drp";
    case 7:
      return "eSett fHelp";
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
