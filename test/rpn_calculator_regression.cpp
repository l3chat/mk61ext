#include "RpnCalculator.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

namespace {

void fail(const char *message) {
  std::cerr << "FAIL: " << message << '\n';
  std::exit(1);
}

void expectTrue(bool condition, const char *message) {
  if (!condition) {
    fail(message);
  }
}

void expectFalse(bool condition, const char *message) {
  if (condition) {
    fail(message);
  }
}

void expectEqual(CalculatorValue actual, CalculatorValue expected, const char *message) {
  if (actual != expected) {
    std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
    std::exit(1);
  }
}

void expectNear(CalculatorValue actual,
                CalculatorValue expected,
                CalculatorValue tolerance,
                const char *message) {
  if (std::fabs(actual - expected) > tolerance) {
    std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual
              << ", tolerance " << tolerance << ")\n";
    std::exit(1);
  }
}

void expectError(const RpnCalculator &calculator, CalculatorError expected, const char *message) {
  if (calculator.error() != expected) {
    std::cerr << "FAIL: " << message << " (expected error " << static_cast<int>(expected)
              << ", got " << static_cast<int>(calculator.error()) << ")\n";
    std::exit(1);
  }
}

void press(RpnCalculator &calculator, CalculatorAction action, const char *message) {
  expectTrue(calculator.apply(action), message);
}

const char *virtualModeName(const RpnCalculator &calculator) {
  if (calculator.hasError()) {
    return "ERR";
  }

  if (calculator.isEnteringExponent()) {
    return "EEX";
  }

  if (calculator.isEntering()) {
    return "ENT";
  }

  return "RUN";
}

void expectVirtualDisplay(const RpnCalculator &calculator,
                          const char *expectedMode,
                          const char *expectedLabel,
                          const char *expectedX,
                          const char *message) {
  const char *actualMode = virtualModeName(calculator);
  if (std::strcmp(actualMode, expectedMode) != 0) {
    std::cerr << "FAIL: " << message << " (expected mode " << expectedMode << ", got " << actualMode
              << ")\n";
    std::exit(1);
  }

  const char *actualLabel = calculator.isEntering() ? "X>" : "X:";
  if (std::strcmp(actualLabel, expectedLabel) != 0) {
    std::cerr << "FAIL: " << message << " (expected label " << expectedLabel << ", got " << actualLabel
              << ")\n";
    std::exit(1);
  }

  char actualX[RpnCalculator::kEntryBufferSize];
  calculator.formatXForDisplay(actualX, sizeof(actualX), 15);
  if (std::strcmp(actualX, expectedX) != 0) {
    std::cerr << "FAIL: " << message << " (expected X text " << expectedX << ", got " << actualX
              << ")\n";
    std::exit(1);
  }
}

void enterText(RpnCalculator &calculator, const char *text) {
  for (const char *cursor = text; *cursor != '\0'; ++cursor) {
    switch (*cursor) {
      case '0':
        press(calculator, CalculatorAction::Digit0, "failed to enter digit 0");
        break;
      case '1':
        press(calculator, CalculatorAction::Digit1, "failed to enter digit 1");
        break;
      case '2':
        press(calculator, CalculatorAction::Digit2, "failed to enter digit 2");
        break;
      case '3':
        press(calculator, CalculatorAction::Digit3, "failed to enter digit 3");
        break;
      case '4':
        press(calculator, CalculatorAction::Digit4, "failed to enter digit 4");
        break;
      case '5':
        press(calculator, CalculatorAction::Digit5, "failed to enter digit 5");
        break;
      case '6':
        press(calculator, CalculatorAction::Digit6, "failed to enter digit 6");
        break;
      case '7':
        press(calculator, CalculatorAction::Digit7, "failed to enter digit 7");
        break;
      case '8':
        press(calculator, CalculatorAction::Digit8, "failed to enter digit 8");
        break;
      case '9':
        press(calculator, CalculatorAction::Digit9, "failed to enter digit 9");
        break;
      case '.':
        press(calculator, CalculatorAction::DecimalPoint, "failed to enter decimal point");
        break;
      case '-':
        press(calculator, CalculatorAction::ChangeSign, "failed to change sign");
        break;
      default:
        fail("unsupported test input character");
    }
  }
}

void clearAndEnter(RpnCalculator &calculator, const char *text) {
  press(calculator, CalculatorAction::ClearAll, "failed to clear calculator");
  enterText(calculator, text);
}

void testBitwiseBinaryOps() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "12");
  press(calculator, CalculatorAction::Enter, "failed to press ENTER for AND");
  enterText(calculator, "10");
  press(calculator, CalculatorAction::BitwiseAnd, "bitwise AND should succeed");
  expectEqual(calculator.stack().x, 8.0, "12 AND 10 should equal 8");

  clearAndEnter(calculator, "12");
  press(calculator, CalculatorAction::Enter, "failed to press ENTER for OR");
  enterText(calculator, "10");
  press(calculator, CalculatorAction::BitwiseOr, "bitwise OR should succeed");
  expectEqual(calculator.stack().x, 14.0, "12 OR 10 should equal 14");

  clearAndEnter(calculator, "12");
  press(calculator, CalculatorAction::Enter, "failed to press ENTER for XOR");
  enterText(calculator, "10");
  press(calculator, CalculatorAction::BitwiseXor, "bitwise XOR should succeed");
  expectEqual(calculator.stack().x, 6.0, "12 XOR 10 should equal 6");
}

void testUnsignedBehavior() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "0");
  press(calculator, CalculatorAction::BitwiseNot, "bitwise NOT should succeed for zero");
  expectEqual(calculator.stack().x, 4294967295.0, "NOT 0 should equal 4294967295");

  clearAndEnter(calculator, "4294967295");
  press(calculator, CalculatorAction::BitwiseNot, "bitwise NOT should succeed for UINT32_MAX");
  expectEqual(calculator.stack().x, 0.0, "NOT UINT32_MAX should equal 0");

  clearAndEnter(calculator, "2147483648");
  press(calculator, CalculatorAction::Enter, "failed to press ENTER for unsigned OR");
  enterText(calculator, "1");
  press(calculator, CalculatorAction::BitwiseOr, "unsigned OR should succeed");
  expectEqual(calculator.stack().x, 2147483649.0, "2147483648 OR 1 should equal 2147483649");
}

void testBitwiseValidation() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "1");
  press(calculator, CalculatorAction::ChangeSign, "failed to create negative test input");
  expectFalse(calculator.apply(CalculatorAction::BitwiseNot),
              "bitwise NOT should reject negative values");
  expectError(calculator, CalculatorError::DomainError,
              "negative values should trigger domain error");

  clearAndEnter(calculator, "1.5");
  press(calculator, CalculatorAction::Enter, "failed to press ENTER for fractional AND");
  enterText(calculator, "3");
  expectFalse(calculator.apply(CalculatorAction::BitwiseAnd),
              "bitwise AND should reject fractional values");
  expectError(calculator, CalculatorError::DomainError,
              "fractional values should trigger domain error");

  clearAndEnter(calculator, "4294967296");
  expectFalse(calculator.apply(CalculatorAction::BitwiseNot),
              "bitwise NOT should reject values above UINT32_MAX");
  expectError(calculator, CalculatorError::DomainError,
              "out-of-range values should trigger domain error");
}

void testAngleModes() {
  RpnCalculator calculator;

  calculator.setAngleMode(CalculatorAngleMode::Degrees);
  clearAndEnter(calculator, "90");
  press(calculator, CalculatorAction::Sin, "sine in degrees should succeed");
  expectNear(calculator.stack().x, 1.0, 1e-12, "sin 90 degrees should equal 1");

  calculator.reset();
  calculator.setAngleMode(CalculatorAngleMode::Gradians);
  clearAndEnter(calculator, "100");
  press(calculator, CalculatorAction::Sin, "sine in gradians should succeed");
  expectNear(calculator.stack().x, 1.0, 1e-12, "sin 100 gradians should equal 1");

  calculator.reset();
  calculator.setAngleMode(CalculatorAngleMode::Degrees);
  clearAndEnter(calculator, "1");
  press(calculator, CalculatorAction::Asin, "arcsine in degrees should succeed");
  expectNear(calculator.stack().x, 90.0, 1e-12, "asin 1 in degree mode should equal 90");

  calculator.reset();
  calculator.setAngleMode(CalculatorAngleMode::Gradians);
  clearAndEnter(calculator, "1");
  press(calculator, CalculatorAction::Atan, "arctangent in gradians should succeed");
  expectNear(calculator.stack().x, 50.0, 1e-12, "atan 1 in gradian mode should equal 50");
}

void testRegisterStoreRecall() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "42");
  expectTrue(calculator.storeRegister(5), "store register 5 should succeed");
  expectEqual(calculator.stack().x, 42.0, "storing should leave X unchanged");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallRegister(5), "recall register 5 should succeed");
  expectEqual(calculator.stack().x, 42.0, "recalling register 5 should restore 42");
  expectEqual(calculator.stack().y, 0.0, "recalling register 5 should push the prior X value into Y");

  clearAndEnter(calculator, "88");
  expectTrue(calculator.storeRegister(14), "store register e should succeed");

  clearAndEnter(calculator, "99");
  expectTrue(calculator.storeRegister(15), "store register f should succeed");

  press(calculator, CalculatorAction::ClearAll, "clear all should succeed");
  expectEqual(calculator.stack().x, 0.0, "clear all should clear X");
  expectTrue(calculator.recallRegister(14), "recall register e should succeed after clear all");
  expectEqual(calculator.stack().x, 88.0, "clear all should not erase stored registers");
  expectEqual(calculator.stack().y, 0.0, "recalling register e should push the prior X value into Y");
  expectTrue(calculator.recallRegister(15), "recall register f should succeed after clear all");
  expectEqual(calculator.stack().x, 99.0, "clear all should not erase register f");
  expectEqual(calculator.stack().y, 88.0, "recalling register f should push the prior X value into Y");
}

void testRegisterValidation() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "1");
  expectFalse(calculator.storeRegister(RpnCalculator::kRegisterCount),
              "store should reject out-of-range register indexes");
  expectError(calculator, CalculatorError::DomainError,
              "invalid register stores should trigger domain error");

  calculator.reset();
  expectFalse(calculator.recallRegister(RpnCalculator::kRegisterCount),
              "recall should reject out-of-range register indexes");
  expectError(calculator, CalculatorError::DomainError,
              "invalid register recalls should trigger domain error");
}

void testIndirectRegisterAccess() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "66");
  expectTrue(calculator.storeRegister(6), "store register 6 should succeed");
  clearAndEnter(calculator, "6");
  expectTrue(calculator.storeRegister(7), "store register 7 should succeed");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallIndirectRegister(7), "indirect recall through register 7 should succeed");
  expectEqual(calculator.stack().x, 66.0, "indirect recall through register 7 should load register 6");
  expectEqual(calculator.stack().y, 0.0,
              "indirect recall through register 7 should push the prior X value into Y");
  expectTrue(calculator.recallRegister(7), "direct recall of register 7 should succeed");
  expectEqual(calculator.stack().x, 6.0, "register 7 should stay unchanged during indirect recall");
  expectEqual(calculator.stack().y, 66.0, "direct recall should push the prior X value into Y");

  clearAndEnter(calculator, "99");
  expectTrue(calculator.storeIndirectRegister(7), "indirect store through register 7 should succeed");
  expectTrue(calculator.recallRegister(6), "direct recall of register 6 should succeed");
  expectEqual(calculator.stack().x, 99.0, "indirect store through register 7 should update register 6");
}

void testIndirectPointerAutoModification() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "44");
  expectTrue(calculator.storeRegister(6), "store register 6 should succeed");
  clearAndEnter(calculator, "5");
  expectTrue(calculator.storeRegister(4), "store register 4 should succeed");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallIndirectRegister(4), "indirect recall through register 4 should succeed");
  expectEqual(calculator.stack().x, 44.0, "register 4 should pre-increment and recall register 6");
  expectEqual(calculator.stack().y, 0.0,
              "indirect recall through register 4 should push the prior X value into Y");
  expectTrue(calculator.recallRegister(4), "direct recall of register 4 should succeed");
  expectEqual(calculator.stack().x, 6.0, "register 4 should be incremented to 6 after indirect recall");
  expectEqual(calculator.stack().y, 44.0, "direct recall should push the prior X value into Y");

  clearAndEnter(calculator, "55");
  expectTrue(calculator.storeRegister(5), "store register 5 should succeed");
  clearAndEnter(calculator, "5");
  expectTrue(calculator.storeRegister(3), "store register 3 should succeed");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallIndirectRegister(3), "indirect recall through register 3 should succeed");
  expectEqual(calculator.stack().x, 55.0, "register 3 should recall register 5 before post-decrement");
  expectEqual(calculator.stack().y, 0.0,
              "indirect recall through register 3 should push the prior X value into Y");
  expectTrue(calculator.recallRegister(3), "direct recall of register 3 should succeed");
  expectEqual(calculator.stack().x, 4.0, "register 3 should be decremented to 4 after indirect recall");
  expectEqual(calculator.stack().y, 55.0, "direct recall should push the prior X value into Y");
}

void testIndirectRegisterWrapping() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "123");
  expectTrue(calculator.storeRegister(5), "store register 5 should succeed");
  clearAndEnter(calculator, "21.9");
  expectTrue(calculator.storeRegister(8), "store register 8 should succeed");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallIndirectRegister(8), "indirect recall through register 8 should succeed");
  expectEqual(calculator.stack().x, 123.0,
              "indirect recall should truncate and wrap pointer values across registers 0-f");
  expectEqual(calculator.stack().y, 0.0,
              "indirect recall through register 8 should push the prior X value into Y");
}

void testIndirectRegisterValidation() {
  RpnCalculator calculator;

  expectFalse(calculator.recallIndirectRegister(RpnCalculator::kRegisterCount),
              "indirect recall should reject out-of-range pointer register indexes");
  expectError(calculator, CalculatorError::DomainError,
              "invalid indirect recalls should trigger domain error");

  calculator.reset();
  expectFalse(calculator.storeIndirectRegister(RpnCalculator::kRegisterCount),
              "indirect store should reject out-of-range pointer register indexes");
  expectError(calculator, CalculatorError::DomainError,
              "invalid indirect stores should trigger domain error");
}

void testDisplayFormattingDuringExponentEntry() {
  RpnCalculator calculator;

  press(calculator, CalculatorAction::Digit1, "failed to enter leading 1");
  expectVirtualDisplay(calculator, "ENT", "X>", "1", "display after entering 1 should show X> 1");

  press(calculator, CalculatorAction::DecimalPoint, "failed to enter decimal point");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.",
                       "display after entering decimal point should show X> 1.");

  press(calculator, CalculatorAction::Digit2, "failed to enter digit 2");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.2", "display after entering 2 should show X> 1.2");

  press(calculator, CalculatorAction::Digit3, "failed to enter digit 3");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.23",
                       "display after entering 3 should show X> 1.23");

  press(calculator, CalculatorAction::Digit4, "failed to enter digit 4");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.234",
                       "display after entering 4 should show X> 1.234");

  press(calculator, CalculatorAction::Digit5, "failed to enter digit 5");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.2345",
                       "display after entering 5 should show X> 1.2345");

  press(calculator, CalculatorAction::Digit6, "failed to enter digit 6");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.23456",
                       "display after entering 6 should show X> 1.23456");

  press(calculator, CalculatorAction::EnterExponent, "failed to enter exponent mode");
  expectVirtualDisplay(calculator, "EEX", "X>", "1.23456e",
                       "display after pressing EEX should show exponent-entry state");

  press(calculator, CalculatorAction::Digit1, "failed to enter exponent digit 1");
  expectVirtualDisplay(calculator, "EEX", "X>", "1.23456e1",
                       "display after entering exponent digit 1 should show X> 1.23456e1");

  press(calculator, CalculatorAction::Digit2, "failed to enter exponent digit 2");
  expectVirtualDisplay(calculator, "EEX", "X>", "1.23456e12",
                       "display after entering exponent digit 2 should show X> 1.23456e12");

  press(calculator, CalculatorAction::Digit3, "failed to enter exponent digit 3");
  expectVirtualDisplay(calculator, "EEX", "X>", "1.23456e123",
                       "display after entering exponent digit 3 should show X> 1.23456e123");
}

void testDisplayFormattingPreservesFractionalZeros() {
  RpnCalculator calculator;

  press(calculator, CalculatorAction::Digit1, "failed to enter leading 1 for fractional-zero test");
  expectVirtualDisplay(calculator, "ENT", "X>", "1",
                       "display after entering 1 should show X> 1 in fractional-zero test");

  press(calculator, CalculatorAction::DecimalPoint,
        "failed to enter decimal point for fractional-zero test");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.",
                       "display after entering decimal point should show X> 1. in fractional-zero test");

  press(calculator, CalculatorAction::Digit0, "failed to enter first fractional zero");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.0",
                       "display should preserve the first fractional zero");

  press(calculator, CalculatorAction::Digit0, "failed to enter second fractional zero");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.00",
                       "display should preserve the second fractional zero");

  press(calculator, CalculatorAction::Digit0, "failed to enter third fractional zero");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.000",
                       "display should preserve the third fractional zero");

  press(calculator, CalculatorAction::Digit0, "failed to enter fourth fractional zero");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.0000",
                       "display should preserve the fourth fractional zero");

  press(calculator, CalculatorAction::Digit1, "failed to enter trailing 1 for fractional-zero test");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.00001",
                       "display should preserve all fractional zeros before the final 1");
  expectEqual(calculator.stack().x, 1.00001, "fractional-zero test should keep the numeric value in X");
}

void testClearXBackspacesMantissaEntry() {
  RpnCalculator calculator;

  enterText(calculator, "1.23");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.23",
                       "mantissa backspace test should start with full entry text");

  press(calculator, CalculatorAction::ClearX, "failed to backspace mantissa digit 3");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.2",
                       "CX should delete the last mantissa digit while entering");

  press(calculator, CalculatorAction::ClearX, "failed to backspace mantissa digit 2");
  expectVirtualDisplay(calculator, "ENT", "X>", "1.",
                       "CX should preserve the trailing decimal point after deleting 2");

  press(calculator, CalculatorAction::ClearX, "failed to backspace decimal point");
  expectVirtualDisplay(calculator, "ENT", "X>", "1",
                       "CX should delete the decimal point when it is the last entry character");

  press(calculator, CalculatorAction::ClearX, "failed to exit mantissa entry");
  expectVirtualDisplay(calculator, "RUN", "X:", "0",
                       "CX should exit entry and leave X at 0 after deleting the final mantissa digit");
}

void testClearXBackspacesExponentEntry() {
  RpnCalculator calculator;

  press(calculator, CalculatorAction::Digit1, "failed to enter leading 1 for exponent backspace test");
  press(calculator, CalculatorAction::EnterExponent,
        "failed to enter exponent mode for exponent backspace test");
  press(calculator, CalculatorAction::Digit1, "failed to enter exponent digit 1");
  press(calculator, CalculatorAction::Digit0, "failed to enter exponent digit 0");
  expectVirtualDisplay(calculator, "EEX", "X>", "1e10",
                       "exponent backspace test should start with full exponent text");

  press(calculator, CalculatorAction::ClearX, "failed to backspace exponent digit 0");
  expectVirtualDisplay(calculator, "EEX", "X>", "1e1",
                       "CX should delete the last exponent digit while entering");

  press(calculator, CalculatorAction::ClearX, "failed to backspace exponent digit 1");
  expectVirtualDisplay(calculator, "EEX", "X>", "1e",
                       "CX should leave bare exponent-entry mode after deleting exponent digits");

  press(calculator, CalculatorAction::ClearX, "failed to exit exponent mode");
  expectVirtualDisplay(calculator, "ENT", "X>", "1",
                       "CX should delete the exponent marker and return to mantissa entry");
}

void testOverflowingExponentDigitIsRejected() {
  RpnCalculator calculator;

  press(calculator, CalculatorAction::Digit1, "failed to enter leading 1 for overflow test");
  press(calculator, CalculatorAction::EnterExponent, "failed to enter exponent mode for overflow test");
  press(calculator, CalculatorAction::Digit1, "failed to enter exponent digit 1 for overflow test");
  press(calculator, CalculatorAction::Digit0, "failed to enter exponent digit 0 for overflow test");
  press(calculator, CalculatorAction::Digit0, "failed to enter exponent digit 0 for overflow test");
  expectVirtualDisplay(calculator, "EEX", "X>", "1e100",
                       "overflow test should start from the largest still-finite staged exponent");

  expectFalse(calculator.apply(CalculatorAction::Digit0),
              "overflowing exponent digit should be rejected and rolled back");
  expectVirtualDisplay(calculator, "EEX", "X>", "1e100",
                       "overflowing exponent digit should leave the visible entry unchanged");
  expectEqual(calculator.stack().x, 1e100,
              "overflowing exponent digit should leave the numeric X value unchanged");
}

void testLongMantissaDigitsAreRejected() {
  RpnCalculator calculator;

  const char *acceptedPrefix = "1000000000000000";
  enterText(calculator, acceptedPrefix);
  expectVirtualDisplay(calculator, "ENT", "X>", acceptedPrefix,
                       "entry should preserve a 16-digit mantissa");

  expectFalse(calculator.apply(CalculatorAction::Digit0),
              "a seventeenth significant mantissa digit should be rejected");
  expectVirtualDisplay(calculator, "ENT", "X>", acceptedPrefix,
                       "rejected mantissa digits should leave the visible entry unchanged");
}

void testProblematicLongMixedMantissaStopsAtPrecisionLimit() {
  RpnCalculator calculator;

  const char *acceptedPrefix = "1000000000000000";
  enterText(calculator, acceptedPrefix);
  for (const char *cursor = "000001111111111111111111111111111111111111111111111"; *cursor != '\0'; ++cursor) {
    expectFalse(calculator.apply((*cursor == '0') ? CalculatorAction::Digit0 : CalculatorAction::Digit1),
                "digits beyond the mantissa precision limit should be rejected");
  }
  expectVirtualDisplay(calculator, "ENT", "X>", "1000000000000000",
                       "problematic long mantissa should stop at the supported 16-digit precision limit");
}

void testRecallPushesStack() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "5");
  expectTrue(calculator.storeRegister(1), "store register 1 should succeed");

  enterText(calculator, "3");
  press(calculator, CalculatorAction::Enter, "failed to press ENTER for recall-push sequence");
  enterText(calculator, "4");
  press(calculator, CalculatorAction::Multiply, "failed to multiply for recall-push sequence");

  expectEqual(calculator.stack().x, 12.0, "sequence should produce 12 in X before recall");
  expectEqual(calculator.stack().y, 5.0, "sequence should leave 5 in Y before recall");

  expectTrue(calculator.recallRegister(1), "recall register 1 should succeed in recall-push sequence");
  expectEqual(calculator.stack().x, 5.0, "recall-push sequence should load register 1 into X");
  expectEqual(calculator.stack().y, 12.0, "recall-push sequence should push the prior X value into Y");
  expectEqual(calculator.stack().z, 5.0, "recall-push sequence should preserve the older Y value in Z");
}

}  // namespace

int main() {
  testBitwiseBinaryOps();
  testUnsignedBehavior();
  testBitwiseValidation();
  testAngleModes();
  testRegisterStoreRecall();
  testRegisterValidation();
  testIndirectRegisterAccess();
  testIndirectPointerAutoModification();
  testIndirectRegisterWrapping();
  testIndirectRegisterValidation();
  testDisplayFormattingDuringExponentEntry();
  testDisplayFormattingPreservesFractionalZeros();
  testClearXBackspacesMantissaEntry();
  testClearXBackspacesExponentEntry();
  testOverflowingExponentDigitIsRejected();
  testLongMantissaDigitsAreRejected();
  testProblematicLongMixedMantissaStopsAtPrecisionLimit();
  testRecallPushesStack();
  std::cout << "RpnCalculator regression tests passed.\n";
  return 0;
}
