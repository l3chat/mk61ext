#include "RpnCalculator.h"
#include "ProgramRecorder.h"
#include "ProgramRunner.h"
#include "ProgramVm.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <initializer_list>
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

void expectEqualByte(uint8_t actual, uint8_t expected, const char *message) {
  if (actual != expected) {
    std::cerr << "FAIL: " << message << " (expected " << static_cast<unsigned>(expected)
              << ", got " << static_cast<unsigned>(actual) << ")\n";
    std::exit(1);
  }
}

void expectEqualU16(uint16_t actual, uint16_t expected, const char *message) {
  if (actual != expected) {
    std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
    std::exit(1);
  }
}

void expectString(const char *actual, const char *expected, const char *message) {
  if ((actual == nullptr) || (std::strcmp(actual, expected) != 0)) {
    std::cerr << "FAIL: " << message << " (expected \"" << expected << "\", got \""
              << ((actual != nullptr) ? actual : "(null)") << "\")\n";
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

void expectRecorderError(const ProgramRecorder &recorder,
                         ProgramRecorderError expected,
                         const char *message) {
  if (recorder.error() != expected) {
    std::cerr << "FAIL: " << message << " (expected error " << static_cast<int>(expected)
              << ", got " << static_cast<int>(recorder.error()) << ")\n";
    std::exit(1);
  }
}

void expectRunnerError(const ProgramRunner &runner,
                       ProgramRunnerError expected,
                       const char *message) {
  if (runner.error() != expected) {
    std::cerr << "FAIL: " << message << " (expected error " << static_cast<int>(expected)
              << ", got " << static_cast<int>(runner.error()) << ")\n";
    std::exit(1);
  }
}

void expectRegisterValue(const RpnCalculator &calculator,
                         uint8_t index,
                         CalculatorValue expected,
                         const char *message) {
  CalculatorValue actual = 0.0;
  if (!calculator.readRegister(index, actual)) {
    std::cerr << "FAIL: " << message << " (failed to read register "
              << static_cast<unsigned>(index) << ")\n";
    std::exit(1);
  }

  if (actual != expected) {
    std::cerr << "FAIL: " << message << " (expected register value " << expected
              << ", got " << actual << ")\n";
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
  if ((text != nullptr) && (text[0] == '-') && (text[1] != '\0')) {
    enterText(calculator, text + 1);
    press(calculator, CalculatorAction::ChangeSign, "failed to apply leading minus sign");
    return;
  }

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

void testProgramVmOpcodeInfo() {
  ProgramOpcodeInfo digitInfo = describeProgramOpcode(0x07);
  expectTrue(digitInfo.valid, "opcode 07 should be valid");
  expectEqual(digitInfo.width, 1.0, "opcode 07 should be one byte");
  expectString(digitInfo.mnemonic, "7", "opcode 07 should decode as digit 7");

  ProgramOpcodeInfo mxFInfo = describeProgramOpcode(0x4F);
  expectTrue(mxFInfo.valid, "opcode 4F should be valid after enabling register F");
  expectString(mxFInfo.mnemonic, "MX", "opcode 4F should be in the MX family");

  ProgramOpcodeInfo xmiFInfo = describeProgramOpcode(0xBF);
  expectTrue(xmiFInfo.valid, "opcode BF should be valid after enabling indirect register F");
  expectString(xmiFInfo.mnemonic, "XMI", "opcode BF should be in the XMI family");

  ProgramOpcodeInfo mxiFInfo = describeProgramOpcode(0xDF);
  expectTrue(mxiFInfo.valid, "opcode DF should be valid after enabling indirect register F");
  expectString(mxiFInfo.mnemonic, "MXI", "opcode DF should be in the MXI family");

  ProgramOpcodeInfo gtoInfo = describeProgramOpcode(0x51);
  expectTrue(gtoInfo.valid, "opcode 51 should be valid");
  expectEqual(gtoInfo.width, 2.0, "opcode 51 should be two bytes");
  expectString(gtoInfo.mnemonic, "GTO", "opcode 51 should decode as GTO");

  ProgramOpcodeInfo invalidInfo = describeProgramOpcode(0x55);
  expectFalse(invalidInfo.valid, "opcode 55 should remain invalid");
  expectString(invalidInfo.mnemonic, "INVALID", "opcode 55 should decode as INVALID");

  ProgramOpcodeInfo extensionInfo = describeProgramOpcode(0xF0);
  expectFalse(extensionInfo.valid, "opcode F0 should not be valid in core v1");
  expectTrue(extensionInfo.extension, "opcode F0 should be marked as extension-space");
  expectString(extensionInfo.mnemonic, "EXT", "opcode F0 should decode as extension");
}

void testProgramVmFormattingAndListing() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x0E, 0x02, 0x12, 0x4F, 0x51, 0x12, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)), "ProgramVm should load a valid byte sequence");
  expectEqual(vm.programLength(), 8.0, "ProgramVm should report the loaded program length");

  ProgramVm::DecodedStep gtoStep = vm.decodeAt(5);
  expectTrue(gtoStep.inRange, "decodeAt should report in-range for existing addresses");
  expectTrue(gtoStep.valid, "GTO opcode should decode as valid");
  expectEqual(gtoStep.width, 2.0, "GTO step should decode as two-byte");
  expectTrue(gtoStep.complete, "GTO step should have its operand available");
  expectEqual(gtoStep.operand, 0x12, "GTO operand should decode from the next byte");

  char label[32];
  expectTrue(vm.formatStep(gtoStep, label, sizeof(label)), "formatStep should succeed for valid steps");
  expectString(label, "GTO 12", "formatStep should render two-byte control operations with operands");

  ProgramVm::DecodedStep mxFStep = vm.decodeAt(4);
  expectTrue(vm.formatStep(mxFStep, label, sizeof(label)), "formatStep should succeed for register steps");
  expectString(label, "MX F", "formatStep should expose register-F opcodes");

  ProgramVm indirectVm;
  const uint8_t indirectProgram[] = {0xBF, 0xDF};
  expectTrue(indirectVm.loadProgram(indirectProgram, sizeof(indirectProgram)),
             "ProgramVm should load indirect register-family opcodes");
  ProgramVm::DecodedStep xmiFStep = indirectVm.decodeAt(0);
  expectTrue(indirectVm.formatStep(xmiFStep, label, sizeof(label)),
             "formatStep should succeed for indirect store steps");
  expectString(label, "XMI F", "formatStep should expose indirect X-to-memory opcodes");

  ProgramVm::DecodedStep mxiFStep = indirectVm.decodeAt(1);
  expectTrue(indirectVm.formatStep(mxiFStep, label, sizeof(label)),
             "formatStep should succeed for indirect recall steps");
  expectString(label, "MXI F", "formatStep should expose indirect memory-to-X opcodes");

  char line[64];
  expectTrue(vm.formatListingLine(5, line, sizeof(line)),
             "formatListingLine should succeed for in-range steps");
  expectString(line, "05: 51 12   GTO 12",
               "listing output should include address, bytes, and decoded mnemonic");

  ProgramVm truncatedVm;
  const uint8_t truncated[] = {0x51};
  expectTrue(truncatedVm.loadProgram(truncated, sizeof(truncated)),
             "ProgramVm should load a truncated control opcode for diagnostics");

  ProgramVm::DecodedStep truncatedStep = truncatedVm.decodeAt(0);
  expectTrue(truncatedStep.valid, "truncated GTO should still decode as a known opcode");
  expectFalse(truncatedStep.complete, "truncated GTO should report missing operand");
  expectTrue(truncatedVm.formatStep(truncatedStep, label, sizeof(label)),
             "formatStep should still work for truncated steps");
  expectString(label, "GTO __", "truncated two-byte steps should show placeholder operand text");

  expectTrue(truncatedVm.formatListingLine(0, line, sizeof(line)),
             "formatListingLine should work for truncated steps");
  expectString(line, "00: 51 __   GTO __",
               "listing output should make truncated two-byte opcodes obvious");
}

void testProgramVmStepNavigation() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x51, 0x20, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "ProgramVm should load navigation test bytes");

  expectTrue(vm.isStepBoundary(0), "address 00 should be a step boundary");
  expectTrue(vm.isStepBoundary(1), "address 01 should be a step boundary");
  expectFalse(vm.isStepBoundary(2), "address 02 should be inside a two-byte step");
  expectTrue(vm.isStepBoundary(3), "address 03 should be a step boundary");
  expectTrue(vm.isStepBoundary(4), "program end should be a boundary");

  expectEqualByte(vm.nextStepAddress(0), 1, "next step after 00 should be 01");
  expectEqualByte(vm.nextStepAddress(1), 3, "next step after 01 should skip to 03");
  expectEqualByte(vm.nextStepAddress(3), 4, "next step after 03 should be program end");
  expectEqualByte(vm.nextStepAddress(4), 4, "next step at end should stay at end");
  expectEqualByte(vm.nextStepAddress(2), 2,
                  "next step should refuse to advance from non-boundary addresses");

  uint8_t previous = 0;
  expectFalse(vm.previousStepAddress(0, previous),
              "there should be no previous step before address 00");
  expectTrue(vm.previousStepAddress(1, previous), "previous step before 01 should exist");
  expectEqualByte(previous, 0, "previous step before 01 should be 00");
  expectTrue(vm.previousStepAddress(3, previous), "previous step before 03 should exist");
  expectEqualByte(previous, 1, "previous step before 03 should be 01");
  expectTrue(vm.previousStepAddress(4, previous), "previous step before end should exist");
  expectEqualByte(previous, 3, "previous step before end should be 03");
  expectFalse(vm.previousStepAddress(2, previous),
              "previous step lookup should reject non-boundary addresses");
}

void testProgramVmStepReplacement() {
  ProgramVm vm;
  const uint8_t initialProgram[] = {0x01, 0x0E, 0x02, 0x12, 0x41, 0x51, 0x12, 0x50};
  expectTrue(vm.loadProgram(initialProgram, sizeof(initialProgram)),
             "ProgramVm should load replace-step baseline program");

  expectTrue(vm.replaceStepAt(4, 0x51, 0x34),
             "replacing a 1-byte step with a 2-byte step should succeed");
  expectEqualU16(vm.programLength(), 9, "program length should grow by one byte");
  const uint8_t expandedProgram[] = {0x01, 0x0E, 0x02, 0x12, 0x51, 0x34, 0x51, 0x12, 0x50};
  for (uint8_t index = 0; index < sizeof(expandedProgram); ++index) {
    expectEqualByte(vm.programByte(index), expandedProgram[index],
                    "expanded replacement should preserve and shift trailing bytes");
  }

  expectTrue(vm.replaceStepAt(4, 0x6F),
             "replacing a 2-byte step with a 1-byte step should succeed");
  expectEqualU16(vm.programLength(), 8, "program length should shrink by one byte");
  const uint8_t shrunkProgram[] = {0x01, 0x0E, 0x02, 0x12, 0x6F, 0x51, 0x12, 0x50};
  for (uint8_t index = 0; index < sizeof(shrunkProgram); ++index) {
    expectEqualByte(vm.programByte(index), shrunkProgram[index],
                    "shrunk replacement should keep trailing bytes aligned");
  }

  expectTrue(vm.replaceStepAt(8, 0x50), "appending at program end should succeed");
  expectEqualU16(vm.programLength(), 9, "appending a one-byte step should increase length by one");
  expectEqualByte(vm.programByte(8), 0x50, "appended step should be written at the end");

  expectFalse(vm.replaceStepAt(5, 0x55), "invalid opcodes should be rejected by replacement");
  expectFalse(vm.replaceStepAt(5, 0xF0),
              "extension-space opcodes should be rejected until explicitly supported");
  expectFalse(vm.replaceStepAt(6, 0x50),
              "replacement should reject non-boundary addresses inside two-byte steps");
}

void testProgramVmReplacementOverflow() {
  ProgramVm fullVm;
  expectTrue(fullVm.setProgramLength(ProgramVm::kProgramCapacity),
             "setting a full-length program should succeed");
  expectFalse(fullVm.replaceStepAt(0, 0x51, 0x00),
              "expanding a full program should fail due to capacity limit");

  ProgramVm nearlyFullVm;
  expectTrue(nearlyFullVm.setProgramLength(255),
             "setting a nearly full program should succeed");
  expectFalse(nearlyFullVm.replaceStepAt(255, 0x51, 0x00),
              "appending a two-byte step at 255 should overflow");
  expectTrue(nearlyFullVm.replaceStepAt(255, 0x50),
             "appending a one-byte step at 255 should still fit");
  expectEqualU16(nearlyFullVm.programLength(), 256, "successful append should fill program capacity");
}

void testProgramRecorderBasicFlow() {
  ProgramVm vm;
  ProgramRecorder recorder;
  uint8_t editAddress = 0;

  expectTrue(recorder.handleKey(vm, editAddress, '7'),
             "recording digit 7 should append opcode 07");
  expectEqualU16(vm.programLength(), 1, "recording first step should grow program length to 1");
  expectEqualByte(vm.programByte(0), 0x07, "digit 7 should encode as opcode 07");
  expectEqualByte(editAddress, 1, "cursor should advance after recording a one-byte step");

  expectTrue(recorder.handleKey(vm, editAddress, 'm'),
             "BST should move to the previous step boundary");
  expectEqualByte(editAddress, 0, "BST from program end should move back to address 00");

  expectTrue(recorder.handleKey(vm, editAddress, 'v'),
             "recording ENTER over an existing step should replace it");
  expectEqualU16(vm.programLength(), 1, "replacing a one-byte step should keep program length stable");
  expectEqualByte(vm.programByte(0), 0x0E, "ENTER should encode as opcode 0E");
  expectEqualByte(editAddress, 1, "cursor should advance after replacement");

  expectTrue(recorder.handleKey(vm, editAddress, 'l'),
             "SST at append position should keep the cursor at program end");
  expectEqualByte(editAddress, 1, "SST at end should not move past program length");
}

void testProgramRecorderRegisterAndAddressOperands() {
  ProgramVm vm;
  ProgramRecorder recorder;
  uint8_t editAddress = 0;

  expectTrue(recorder.handleKey(vm, editAddress, 'q'),
             "MX should arm register-operand entry");
  expectTrue(recorder.hasPendingInput(), "MX should create pending recorder state");
  expectTrue(recorder.handleKey(vm, editAddress, 'u'),
             "MX followed by u should commit MX F");
  expectFalse(recorder.hasPendingInput(), "successful register commit should clear pending state");

  expectTrue(recorder.handleKey(vm, editAddress, 'r'),
             "XM should arm register-operand entry");
  expectTrue(recorder.handleKey(vm, editAddress, 'y'),
             "XM followed by y should commit XM C");

  expectTrue(recorder.handleKey(vm, editAddress, 's'),
             "GTO should arm address entry");
  expectTrue(recorder.handleKey(vm, editAddress, '.'),
             "first address nibble . should map to hexadecimal A");
  expectTrue(recorder.handleKey(vm, editAddress, 'u'),
             "second address nibble u should map to hexadecimal F");

  expectTrue(recorder.handleKey(vm, editAddress, 't'),
             "GSB should arm address entry");
  expectTrue(recorder.handleKey(vm, editAddress, '1'),
             "GSB high nibble should accept digit 1");
  expectTrue(recorder.handleKey(vm, editAddress, '2'),
             "GSB low nibble should accept digit 2");

  expectEqualU16(vm.programLength(), 6, "recorded register/address sequence should use 6 bytes");
  const uint8_t expected[] = {0x4F, 0x6C, 0x51, 0xAF, 0x53, 0x12};
  for (uint8_t index = 0; index < sizeof(expected); ++index) {
    expectEqualByte(vm.programByte(index), expected[index],
                    "register/address recorder output should match canonical opcodes");
  }
  expectEqualByte(editAddress, 6, "cursor should end at append position after commits");
}

void testProgramRecorderShiftedFamilies() {
  ProgramVm vm;
  ProgramRecorder recorder;
  uint8_t editAddress = 0;

  expectTrue(recorder.handleKey(vm, editAddress, 'k'),
             "F prefix should arm shifted recording");
  expectTrue(recorder.handleKey(vm, editAddress, '7'),
             "F 7 should record SIN");

  expectTrue(recorder.handleKey(vm, editAddress, 'k'),
             "F prefix should re-arm for L recording");
  expectTrue(recorder.handleKey(vm, editAddress, 'q'),
             "F q should arm L0 address entry");
  expectTrue(recorder.handleKey(vm, editAddress, '2'),
             "L0 high nibble should accept 2");
  expectTrue(recorder.handleKey(vm, editAddress, '0'),
             "L0 low nibble should accept 0");

  expectTrue(recorder.handleKey(vm, editAddress, 'p'),
             "K prefix should arm shifted recording");
  expectTrue(recorder.handleKey(vm, editAddress, 'q'),
             "K q should arm MXI register entry");
  expectTrue(recorder.handleKey(vm, editAddress, 'u'),
             "K q u should commit MXI F");

  expectTrue(recorder.handleKey(vm, editAddress, 'p'),
             "K prefix should arm indirect jump recording");
  expectTrue(recorder.handleKey(vm, editAddress, 's'),
             "K s should arm JPI register entry");
  expectTrue(recorder.handleKey(vm, editAddress, 'v'),
             "K s v should commit JPI E");

  expectTrue(recorder.handleKey(vm, editAddress, 'k'),
             "F prefix should arm before prefix switch test");
  expectTrue(recorder.handleKey(vm, editAddress, 'p'),
             "pressing p after k should switch to K prefix");
  expectTrue(recorder.handleKey(vm, editAddress, '4'),
             "K 4 should record ABS");

  expectTrue(recorder.handleKey(vm, editAddress, 'p'),
             "K prefix should arm before NOP");
  expectTrue(recorder.handleKey(vm, editAddress, '0'),
             "K 0 should record NOP");

  const uint8_t expected[] = {0x1C, 0x5D, 0x20, 0xDF, 0x8E, 0x31, 0x54};
  expectEqualU16(vm.programLength(), sizeof(expected),
                 "shifted recorder sequence should produce expected byte count");
  for (uint8_t index = 0; index < sizeof(expected); ++index) {
    expectEqualByte(vm.programByte(index), expected[index],
                    "shifted recorder mapping should match the recommended opcode map");
  }
}

void testProgramRecorderErrors() {
  ProgramVm vm;
  ProgramRecorder recorder;
  uint8_t editAddress = 0;

  expectFalse(recorder.handleKey(vm, editAddress, 'a'),
              "unsupported primary key should be rejected");
  expectRecorderError(recorder, ProgramRecorderError::InvalidKey,
                      "unsupported key should report InvalidKey");

  expectTrue(recorder.handleKey(vm, editAddress, 'q'),
             "MX should arm register-operand state before invalid operand test");
  expectFalse(recorder.handleKey(vm, editAddress, 'a'),
              "invalid register designator should be rejected");
  expectRecorderError(recorder, ProgramRecorderError::InvalidOperand,
                      "invalid register designator should report InvalidOperand");
  expectFalse(recorder.hasPendingInput(),
              "invalid register designator should clear pending recorder state");

  ProgramVm nearlyFullVm;
  expectTrue(nearlyFullVm.setProgramLength(255),
             "setting a nearly full program should succeed for overflow test");
  ProgramRecorder overflowRecorder;
  uint8_t overflowAddress = 255;

  expectTrue(overflowRecorder.handleKey(nearlyFullVm, overflowAddress, 's'),
             "GTO should arm address entry at the final byte");
  expectTrue(overflowRecorder.handleKey(nearlyFullVm, overflowAddress, '0'),
             "first address nibble should still be accepted near capacity");
  expectFalse(overflowRecorder.handleKey(nearlyFullVm, overflowAddress, '0'),
              "committing a two-byte command at 255 should overflow");
  expectRecorderError(overflowRecorder, ProgramRecorderError::CommitFailed,
                      "overflowing commit should report CommitFailed");
  expectFalse(overflowRecorder.hasPendingInput(),
              "failed overflow commit should clear pending recorder state");
  expectEqualU16(nearlyFullVm.programLength(), 255,
                 "overflowed two-byte append should leave program length unchanged");
}

void recordProgramFromSteps(ProgramVm &vm, std::initializer_list<const char *> steps) {
  ProgramRecorder recorder;
  uint8_t editAddress = 0;
  uint8_t stepIndex = 0;

  for (const char *step : steps) {
    for (const char *cursor = step; *cursor != '\0'; ++cursor) {
      if (*cursor == ' ') {
        continue;
      }

      if (!recorder.handleKey(vm, editAddress, *cursor)) {
        std::cerr << "FAIL: example program recording failed at step "
                  << static_cast<unsigned>(stepIndex) << " key '" << *cursor
                  << "' (error " << static_cast<int>(recorder.error()) << ")\n";
        std::exit(1);
      }
    }

    ++stepIndex;
  }

  expectFalse(recorder.hasPendingInput(),
              "example program recording should not leave pending input");
}

void expectProgramBytes(const ProgramVm &vm,
                        const uint8_t *expected,
                        size_t length,
                        const char *message) {
  if (vm.programLength() != length) {
    std::cerr << "FAIL: " << message << " (expected program length " << length
              << ", got " << vm.programLength() << ")\n";
    std::exit(1);
  }

  for (size_t index = 0; index < length; ++index) {
    const uint8_t actual = vm.programByte(static_cast<uint16_t>(index));
    if (actual != expected[index]) {
      std::cerr << "FAIL: " << message << " (byte " << index << ": expected "
                << static_cast<unsigned>(expected[index]) << ", got "
                << static_cast<unsigned>(actual) << ")\n";
      std::exit(1);
    }
  }
}

void runProgramUntilStop(ProgramRunner &runner,
                         const ProgramVm &vm,
                         RpnCalculator &calculator,
                         uint16_t maxSteps,
                         const char *message) {
  uint16_t steps = 0;
  while (runner.isRunning() && (steps < maxSteps)) {
    (void)runner.step(vm, calculator);
    ++steps;
  }

  if (runner.isRunning()) {
    fail(message);
  }
}

void testProgramRunnerLinearExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x0E, 0x02, 0x10, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading linear execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start on a valid program");
  runProgramUntilStop(runner, vm, calculator, 16, "linear program should stop promptly");

  expectFalse(runner.hasError(), "linear program should finish without runner errors");
  expectEqualByte(runner.runAddress(), 5, "HALT should leave run address after the halt step");
  expectEqual(calculator.stack().x, 3.0, "1 ENTER 2 + should leave 3 in X");
}

void testProgramRunnerSingleStepExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x02, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading single-step digit program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.singleStep(vm, calculator), "SST should execute the first step");
  expectFalse(runner.isRunning(), "SST should pause after one step");
  expectEqualByte(runner.runAddress(), 1, "SST should advance to the second step");
  expectTrue(calculator.isEntering(), "SST of a digit should leave numeric entry active");
  expectEqual(calculator.stack().x, 1.0, "first SST should enter digit 1");

  expectTrue(runner.singleStep(vm, calculator), "second SST should execute the second digit");
  expectFalse(runner.isRunning(), "second SST should pause after one step");
  expectEqualByte(runner.runAddress(), 2, "second SST should advance to HALT");
  expectTrue(calculator.isEntering(), "second SST should keep numeric entry active");
  expectEqual(calculator.stack().x, 12.0,
              "second SST should continue the program's active numeric entry");

  expectTrue(runner.singleStep(vm, calculator), "third SST should execute HALT");
  expectFalse(runner.isRunning(), "SST of HALT should leave the runner stopped");
  expectEqualByte(runner.runAddress(), 3, "HALT should leave the run address after the halt step");
  expectFalse(runner.hasError(), "SST sequence should finish without runner errors");
}

void testProgramRunnerResumeAfterSingleStepKeepsProgramEntry() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x02, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading resume-after-SST program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.singleStep(vm, calculator), "SST should execute the first digit before resume");
  expectTrue(runner.start(vm, calculator), "R/S should resume after SST");
  runProgramUntilStop(runner, vm, calculator, 8, "resumed program should halt");

  expectFalse(runner.hasError(), "resumed program should finish without runner errors");
  expectEqual(calculator.stack().x, 12.0,
              "R/S after SST should continue the program's active numeric entry");
}

void testProgramRunnerSingleStepCommitsInitialEntry() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading initial-entry SST program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;
  enterText(calculator, "2");

  expectTrue(runner.singleStep(vm, calculator), "SST should start from an active user entry");
  expectFalse(runner.isRunning(), "SST should pause after executing one step");
  expectEqual(calculator.stack().x, 1.0, "SST should enter the program digit in X");
  expectEqual(calculator.stack().y, 0.0,
              "SST should commit the user's pending entry instead of appending the program digit to it");
}

void testProgramRunnerSingleStepSubroutineExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x53, 0x03, 0x50, 0x01, 0x52};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading single-step subroutine program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.singleStep(vm, calculator), "SST should execute GSB");
  expectFalse(runner.isRunning(), "SST should pause after GSB");
  expectEqualByte(runner.runAddress(), 3, "GSB under SST should move to the subroutine target");

  expectTrue(runner.singleStep(vm, calculator), "SST should execute the subroutine body");
  expectFalse(runner.isRunning(), "SST should pause inside the subroutine");
  expectEqualByte(runner.runAddress(), 4, "SST should advance to RETURN inside the subroutine");
  expectEqual(calculator.stack().x, 1.0, "subroutine body should enter digit 1");

  expectTrue(runner.singleStep(vm, calculator), "SST should execute RETURN");
  expectFalse(runner.isRunning(), "SST should pause after RETURN");
  expectEqualByte(runner.runAddress(), 2, "RETURN under SST should restore the caller address");

  expectTrue(runner.singleStep(vm, calculator), "SST should execute the caller HALT");
  expectFalse(runner.hasError(), "single-step subroutine path should not set runner errors");
  expectEqualByte(runner.runAddress(), 3, "HALT should leave the run address after the halt step");
}

void testProgramRunnerDirectJumpExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x51, 0x04, 0x09, 0x50, 0x01, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading direct-jump execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start for direct-jump execution");
  runProgramUntilStop(runner, vm, calculator, 16, "direct-jump program should halt");

  expectFalse(runner.hasError(), "direct GTO execution should not set runner error");
  expectEqualByte(runner.runAddress(), 6,
                  "final HALT after a direct jump should leave run address after the halt step");
  expectEqual(calculator.stack().x, 1.0, "direct GTO should skip over the unreachable digit 9 path");
}

void testProgramRunnerDirectConditionalExecution() {
  {
    ProgramVm vm;
    const uint8_t program[] = {0x00, 0x0E, 0x5E, 0x06, 0x09, 0x50, 0x01, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading JP X=0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(runner.start(vm), "runner should start for JP X=0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "JP X=0 true-path program should halt");

    expectFalse(runner.hasError(), "JP X=0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 8, "JP X=0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 1.0, "JP X=0 should jump when X is zero");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x01, 0x0E, 0x5E, 0x03, 0x09, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading JP X=0 false-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(runner.start(vm), "runner should start for JP X=0 false-path");
    runProgramUntilStop(runner, vm, calculator, 16, "JP X=0 false-path program should halt");

    expectFalse(runner.hasError(), "JP X=0 false-path should ignore the invalid untaken target");
    expectEqualByte(runner.runAddress(), 6, "JP X=0 false-path HALT should end at program length");
    expectEqual(calculator.stack().x, 9.0, "JP X=0 should fall through when X is nonzero");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x01, 0x0B, 0x0E, 0x5C, 0x07, 0x09, 0x50, 0x02, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading JP X<0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(runner.start(vm), "runner should start for JP X<0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "JP X<0 true-path program should halt");

    expectFalse(runner.hasError(), "JP X<0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 9, "JP X<0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 2.0, "JP X<0 should jump when X is negative");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x00, 0x0E, 0x59, 0x06, 0x09, 0x50, 0x03, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading JP X>=0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(runner.start(vm), "runner should start for JP X>=0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "JP X>=0 true-path program should halt");

    expectFalse(runner.hasError(), "JP X>=0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 8, "JP X>=0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 3.0, "JP X>=0 should jump when X is zero or positive");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x00, 0x0E, 0x57, 0x03, 0x09, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading JP X<>0 false-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(runner.start(vm), "runner should start for JP X<>0 false-path");
    runProgramUntilStop(runner, vm, calculator, 16, "JP X<>0 false-path program should halt");

    expectFalse(runner.hasError(), "JP X<>0 false-path should ignore the invalid untaken target");
    expectEqualByte(runner.runAddress(), 6, "JP X<>0 false-path HALT should end at program length");
    expectEqual(calculator.stack().x, 9.0, "JP X<>0 should fall through when X is zero");
  }
}

void testProgramRunnerIndirectJumpExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x82, 0x09, 0x50, 0x01, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading indirect-jump execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(calculator.writeRegister(2, 259.7), "writing wrapped indirect jump target should succeed");
  expectTrue(runner.start(vm), "runner should start for indirect-jump execution");
  runProgramUntilStop(runner, vm, calculator, 16, "indirect-jump program should halt");

  expectFalse(runner.hasError(), "indirect JPI execution should not set runner error");
  expectEqualByte(runner.runAddress(), 5,
                  "final HALT after an indirect jump should leave run address after the halt step");
  expectEqual(calculator.stack().x, 1.0,
              "indirect JPI should truncate and wrap the target register into byte-address space");
}

void testProgramRunnerIndirectConditionalExecution() {
  {
    ProgramVm vm;
    const uint8_t program[] = {0x01, 0x0E, 0x72, 0x09, 0x50, 0x02, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading indirect JP X<>0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(2, 5.0), "writing indirect JP X<>0 target should succeed");
    expectTrue(runner.start(vm), "runner should start for indirect JP X<>0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "indirect JP X<>0 true-path program should halt");

    expectFalse(runner.hasError(), "indirect JP X<>0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 7, "indirect JP X<>0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 2.0, "indirect JP X<>0 should jump when X is nonzero");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x00, 0x0E, 0x90, 0x09, 0x50, 0x03, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading indirect JP X>=0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(0, 5.0), "writing indirect JP X>=0 target should succeed");
    expectTrue(runner.start(vm), "runner should start for indirect JP X>=0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "indirect JP X>=0 true-path program should halt");

    expectFalse(runner.hasError(), "indirect JP X>=0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 7, "indirect JP X>=0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 3.0, "indirect JP X>=0 should jump when X is zero or positive");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x01, 0x0B, 0x0E, 0xC3, 0x09, 0x50, 0x04, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading indirect JP X<0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(3, 6.0), "writing indirect JP X<0 target should succeed");
    expectTrue(runner.start(vm), "runner should start for indirect JP X<0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "indirect JP X<0 true-path program should halt");

    expectFalse(runner.hasError(), "indirect JP X<0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 8, "indirect JP X<0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 4.0, "indirect JP X<0 should jump when X is negative");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x00, 0x0E, 0xE4, 0x09, 0x50, 0x05, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading indirect JP X=0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(4, 5.0), "writing indirect JP X=0 target should succeed");
    expectTrue(runner.start(vm), "runner should start for indirect JP X=0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "indirect JP X=0 true-path program should halt");

    expectFalse(runner.hasError(), "indirect JP X=0 true-path should not set runner error");
    expectEqualByte(runner.runAddress(), 7, "indirect JP X=0 true-path HALT should end at program length");
    expectEqual(calculator.stack().x, 5.0, "indirect JP X=0 should jump when X is zero");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x00, 0x0E, 0x70, 0x51, 0x05, 0x09, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading indirect JP X<>0 false-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(0, 4.0),
               "writing untaken invalid indirect conditional target should succeed");
    expectTrue(runner.start(vm), "runner should start for indirect JP X<>0 false-path");
    runProgramUntilStop(runner, vm, calculator, 16, "indirect JP X<>0 false-path program should halt");

    expectFalse(runner.hasError(),
                "indirect untaken conditional should ignore an invalid target in the selector register");
    expectEqualByte(runner.runAddress(), 7, "indirect JP X<>0 false-path HALT should end at program length");
    expectEqual(calculator.stack().x, 9.0,
                "indirect JP X<>0 should fall through when X is zero even if the selector register holds an invalid target");
  }
}

void testProgramRunnerRegisterExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x05, 0x61, 0x03, 0x0E, 0x04, 0x12, 0x41, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading register execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start for register execution");
  runProgramUntilStop(runner, vm, calculator, 16, "register execution program should halt");

  const CalculatorStack stack = calculator.stack();
  expectFalse(runner.hasError(), "register execution should not set runner error");
  expectEqual(stack.x, 5.0, "MX 1 should recall 5 into X");
  expectEqual(stack.y, 12.0, "MX 1 should lift the previous result into Y");
}

void testProgramRunnerIndirectSubroutineExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0xA1, 0x50, 0x01, 0x52};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading indirect subroutine execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(calculator.writeRegister(1, -254.2),
             "writing wrapped indirect subroutine target should succeed");
  expectTrue(runner.start(vm), "runner should start for indirect subroutine execution");
  runProgramUntilStop(runner, vm, calculator, 16, "indirect subroutine program should halt");

  expectFalse(runner.hasError(), "indirect GSBI execution should not set runner error");
  expectEqualByte(runner.runAddress(), 2, "HALT after an indirect subroutine should leave run address after the halt");
  expectEqual(calculator.stack().x, 1.0,
              "indirect GSBI should truncate and wrap the selector-register target before calling");
}

void testProgramRunnerDsnzExecution() {
  {
    ProgramVm vm;
    const uint8_t program[] = {0x5D, 0x04, 0x09, 0x50, 0x01, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading L0 true-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(0, 2.0), "writing register 0 before L0 should succeed");
    expectTrue(runner.start(vm), "runner should start for L0 true-path");
    runProgramUntilStop(runner, vm, calculator, 16, "L0 true-path program should halt");

    expectFalse(runner.hasError(), "L0 true-path should not set runner error");
    expectEqual(calculator.stack().x, 1.0, "L0 should jump while the decremented counter stays nonzero");
    expectRegisterValue(calculator, 0, 1.0, "L0 should decrement register 0 before jumping");
  }

  {
    ProgramVm vm;
    const uint8_t program[] = {0x5D, 0x01, 0x09, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading L0 false-path program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(0, 1.0), "writing register 0 before L0 false-path should succeed");
    expectTrue(runner.start(vm), "runner should start for L0 false-path");
    runProgramUntilStop(runner, vm, calculator, 16, "L0 false-path program should halt");

    expectFalse(runner.hasError(), "L0 false-path should ignore the invalid untaken target");
    expectEqual(calculator.stack().x, 9.0, "L0 should fall through when the decremented counter reaches zero");
    expectRegisterValue(calculator, 0, 0.0, "L0 should decrement register 0 to zero on the fall-through path");
  }

  const struct {
    uint8_t opcode;
    uint8_t index;
  } cases[] = {
      {0x5B, 1},
      {0x58, 2},
      {0x5A, 3},
  };

  for (const auto &caseData : cases) {
    ProgramVm vm;
    const uint8_t program[] = {caseData.opcode, 0x04, 0x09, 0x50, 0x01, 0x50};
    expectTrue(vm.loadProgram(program, sizeof(program)),
               "loading L register-selection program should succeed");

    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(caseData.index, 2.0),
               "writing L register-selection counter should succeed");
    expectTrue(runner.start(vm), "runner should start for L register-selection test");
    runProgramUntilStop(runner, vm, calculator, 16, "L register-selection program should halt");

    expectFalse(runner.hasError(), "L register-selection should not set runner error");
    expectEqual(calculator.stack().x, 1.0,
                "L register-selection should jump while the decremented counter stays nonzero");
    expectRegisterValue(calculator, caseData.index, 1.0,
                        "L register-selection should decrement the expected counter register");
  }
}

void testProgramRunnerSubroutineExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x53, 0x03, 0x50, 0x01, 0x52};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading direct subroutine execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start for subroutine execution");
  runProgramUntilStop(runner, vm, calculator, 16, "direct subroutine program should halt");

  expectFalse(runner.hasError(), "direct subroutine execution should not set runner error");
  expectEqualByte(runner.runAddress(), 3, "HALT after a subroutine should leave run address after the halt");
  expectEqual(calculator.stack().x, 1.0, "GSB should execute the subroutine body before returning to HALT");
}

void testProgramRunnerNestedSubroutineExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x53, 0x03, 0x50, 0x02, 0x0E, 0x53, 0x08, 0x52, 0x03, 0x10, 0x52};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading nested subroutine execution program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start for nested subroutine execution");
  runProgramUntilStop(runner, vm, calculator, 32, "nested subroutine program should halt");

  expectFalse(runner.hasError(), "nested subroutine execution should not set runner error");
  expectEqualByte(runner.runAddress(), 3,
                  "HALT after nested subroutines should leave run address after the halt");
  expectEqual(calculator.stack().x, 5.0,
              "nested GSB/RETURN execution should preserve return order and compute 2 + 3");
}

void testProgramRunnerReturnStopsExecution() {
  ProgramVm vm;
  const uint8_t program[] = {0x01, 0x52, 0x02};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading RETURN stop program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start for RETURN stop test");
  runProgramUntilStop(runner, vm, calculator, 8, "RETURN stop program should halt");

  expectFalse(runner.hasError(), "RETURN should stop cleanly in the basic runner");
  expectEqualByte(runner.runAddress(), 2, "RETURN should leave run address after the return step");
  expectEqual(calculator.stack().x, 1.0, "RETURN should stop before the following digit executes");
}

void testProgramRunnerInvalidTargetStops() {
  ProgramVm vm;
  const uint8_t program[] = {0x51, 0x01, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading invalid-target program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start before invalid-target test");
  expectFalse(runner.step(vm, calculator), "invalid control-flow target should stop execution with an error");
  expectFalse(runner.isRunning(), "runner should stop after invalid target");
  expectRunnerError(runner, ProgramRunnerError::InvalidTarget,
                    "invalid control-flow target should report InvalidTarget");
  expectEqualByte(runner.runAddress(), 0, "invalid target should leave run address on the failing step");
}

void testProgramRunnerIndirectInvalidTargetStops() {
  ProgramVm vm;
  const uint8_t program[] = {0x82, 0x51, 0x04, 0x50, 0x01, 0x50};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading indirect invalid-target program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(calculator.writeRegister(2, 2.9), "writing indirect invalid target should succeed");
  expectTrue(runner.start(vm), "runner should start before indirect invalid-target test");
  expectFalse(runner.step(vm, calculator),
              "invalid indirect control-flow target should stop execution with an error");
  expectFalse(runner.isRunning(), "runner should stop after invalid indirect target");
  expectRunnerError(runner, ProgramRunnerError::InvalidTarget,
                    "invalid indirect control-flow target should report InvalidTarget");
  expectEqualByte(runner.runAddress(), 0,
                  "invalid indirect target should leave run address on the failing step");
}

void testProgramRunnerCallStackOverflowStops() {
  ProgramVm vm;
  const uint8_t program[] = {0x53, 0x00};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading recursive GSB program should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start before call-stack overflow test");
  for (uint8_t step = 0; step < 64; ++step) {
    if (!runner.step(vm, calculator)) {
      break;
    }
  }

  expectFalse(runner.isRunning(), "runner should stop after call-stack overflow");
  expectRunnerError(runner, ProgramRunnerError::CallStackOverflow,
                    "recursive GSB should report CallStackOverflow");
  expectEqualByte(runner.runAddress(), 0, "call-stack overflow should leave run address on the failing step");
}

void testProgramRunnerInvalidStepStops() {
  ProgramVm vm;
  const uint8_t program[] = {0x51};
  expectTrue(vm.loadProgram(program, sizeof(program)),
             "loading truncated program step should succeed");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(runner.start(vm), "runner should start before invalid-step test");
  expectFalse(runner.step(vm, calculator), "truncated step should stop execution with an error");
  expectFalse(runner.isRunning(), "runner should stop after invalid step");
  expectRunnerError(runner, ProgramRunnerError::InvalidStep,
                    "truncated step should report InvalidStep");
  expectEqualByte(runner.runAddress(), 0, "invalid step should leave run address on failing step");
}

void testExampleFactorialProgram() {
  ProgramVm vm;
  recordProgramFromSteps(vm, {"km0v", "r0", "r1", "kq08", "q1", "o", "q1", "q0", "*", "r1", "s04", "1", "o"});

  const uint8_t expected[] = {
      0x5E, 0x0E, 0x60, 0x61, 0x5D, 0x08, 0x41, 0x50,
      0x41, 0x40, 0x12, 0x61, 0x51, 0x04, 0x01, 0x50,
  };
  expectProgramBytes(vm, expected, sizeof(expected),
                     "factorial example should record the documented byte sequence");

  const struct {
    const char *input;
    CalculatorValue expected;
  } cases[] = {
      {"0", 1.0},
      {"1", 1.0},
      {"5", 120.0},
  };

  for (const auto &caseData : cases) {
    ProgramRunner runner;
    RpnCalculator calculator;

    clearAndEnter(calculator, caseData.input);
    expectTrue(runner.start(vm, calculator), "runner should start for factorial example");
    runProgramUntilStop(runner, vm, calculator, 128, "factorial example should halt");

    expectFalse(runner.hasError(), "factorial example should finish without runner errors");
    expectNear(calculator.stack().x, caseData.expected, 1e-9,
               "factorial example should produce the documented result");
  }
}

void testExampleTriangularProgram() {
  ProgramVm vm;
  recordProgramFromSteps(vm, {"km0y", "r0", "0", "r1", "q1", "q0", "+", "r1", "kq05", "q1", "o"});

  const uint8_t expected[] = {
      0x5E, 0x0C, 0x60, 0x00, 0x61, 0x41, 0x40,
      0x10, 0x61, 0x5D, 0x05, 0x41, 0x50,
  };
  expectProgramBytes(vm, expected, sizeof(expected),
                     "triangular-number example should record the documented byte sequence");

  const struct {
    const char *input;
    CalculatorValue expected;
  } cases[] = {
      {"0", 0.0},
      {"1", 1.0},
      {"5", 15.0},
  };

  for (const auto &caseData : cases) {
    ProgramRunner runner;
    RpnCalculator calculator;

    clearAndEnter(calculator, caseData.input);
    expectTrue(runner.start(vm, calculator), "runner should start for triangular-number example");
    runProgramUntilStop(runner, vm, calculator, 128, "triangular-number example should halt");

    expectFalse(runner.hasError(),
                "triangular-number example should finish without runner errors");
    expectNear(calculator.stack().x, caseData.expected, 1e-9,
               "triangular-number example should produce the documented result");
  }
}

void testExampleAbsoluteValueProgram() {
  ProgramVm vm;
  recordProgramFromSteps(vm, {"kl03", "o", "x", "o"});

  const uint8_t expected[] = {0x5C, 0x03, 0x50, 0x0B, 0x50};
  expectProgramBytes(vm, expected, sizeof(expected),
                     "absolute-value example should record the documented byte sequence");

  const struct {
    const char *input;
    CalculatorValue expected;
  } cases[] = {
      {"-3", 3.0},
      {"0", 0.0},
      {"7", 7.0},
  };

  for (const auto &caseData : cases) {
    ProgramRunner runner;
    RpnCalculator calculator;

    clearAndEnter(calculator, caseData.input);
    expectTrue(runner.start(vm, calculator), "runner should start for absolute-value example");
    runProgramUntilStop(runner, vm, calculator, 32, "absolute-value example should halt");

    expectFalse(runner.hasError(), "absolute-value example should finish without runner errors");
    expectNear(calculator.stack().x, caseData.expected, 1e-9,
               "absolute-value example should produce the documented result");
  }
}

void testExampleIndirectJumpProgram() {
  ProgramVm vm;
  recordProgramFromSteps(vm, {"ps2", "9", "o", "1", "o"});

  const uint8_t expected[] = {0x82, 0x09, 0x50, 0x01, 0x50};
  expectProgramBytes(vm, expected, sizeof(expected),
                     "indirect-jump example should record the documented byte sequence");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(calculator.writeRegister(2, 3.0),
             "writing the documented indirect-jump selector target should succeed");
  expectTrue(runner.start(vm, calculator), "runner should start for indirect-jump example");
  runProgramUntilStop(runner, vm, calculator, 32, "indirect-jump example should halt");

  expectFalse(runner.hasError(), "indirect-jump example should finish without runner errors");
  expectEqual(calculator.stack().x, 1.0,
              "indirect-jump example should land on the documented target");
}

void testExampleIndirectSubroutineProgram() {
  ProgramVm vm;
  recordProgramFromSteps(vm, {"pt1", "o", "1", "n"});

  const uint8_t expected[] = {0xA1, 0x50, 0x01, 0x52};
  expectProgramBytes(vm, expected, sizeof(expected),
                     "indirect-subroutine example should record the documented byte sequence");

  ProgramRunner runner;
  RpnCalculator calculator;

  expectTrue(calculator.writeRegister(1, 2.0),
             "writing the documented indirect-subroutine selector target should succeed");
  expectTrue(runner.start(vm, calculator), "runner should start for indirect-subroutine example");
  runProgramUntilStop(runner, vm, calculator, 32, "indirect-subroutine example should halt");

  expectFalse(runner.hasError(),
              "indirect-subroutine example should finish without runner errors");
  expectEqual(calculator.stack().x, 1.0,
              "indirect-subroutine example should return to the caller after running the body");
}

void testExampleIndirectSignClassifierProgram() {
  ProgramVm vm;
  recordProgramFromSteps(vm, {"pl0", "pm1", "pn2", "o", "1", "x", "o", "0", "o", "1", "o"});

  const uint8_t expected[] = {
      0xC0, 0xE1, 0x92, 0x50, 0x01, 0x0B, 0x50, 0x00, 0x50, 0x01, 0x50,
  };
  expectProgramBytes(vm, expected, sizeof(expected),
                     "indirect sign-classifier example should record the documented byte sequence");

  const struct {
    const char *input;
    CalculatorValue expected;
  } cases[] = {
      {"-3", -1.0},
      {"0", 0.0},
      {"5", 1.0},
  };

  for (const auto &caseData : cases) {
    ProgramRunner runner;
    RpnCalculator calculator;

    expectTrue(calculator.writeRegister(0, 4.0),
               "writing the documented negative-target selector should succeed");
    expectTrue(calculator.writeRegister(1, 7.0),
               "writing the documented zero-target selector should succeed");
    expectTrue(calculator.writeRegister(2, 9.0),
               "writing the documented positive-target selector should succeed");
    clearAndEnter(calculator, caseData.input);

    expectTrue(runner.start(vm, calculator),
               "runner should start for indirect sign-classifier example");
    runProgramUntilStop(runner, vm, calculator, 32, "indirect sign-classifier example should halt");

    expectFalse(runner.hasError(),
                "indirect sign-classifier example should finish without runner errors");
    expectNear(calculator.stack().x, caseData.expected, 1e-9,
               "indirect sign-classifier example should produce the documented result");
  }
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
  testProgramVmOpcodeInfo();
  testProgramVmFormattingAndListing();
  testProgramVmStepNavigation();
  testProgramVmStepReplacement();
  testProgramVmReplacementOverflow();
  testProgramRecorderBasicFlow();
  testProgramRecorderRegisterAndAddressOperands();
  testProgramRecorderShiftedFamilies();
  testProgramRecorderErrors();
  testProgramRunnerLinearExecution();
  testProgramRunnerSingleStepExecution();
  testProgramRunnerResumeAfterSingleStepKeepsProgramEntry();
  testProgramRunnerSingleStepCommitsInitialEntry();
  testProgramRunnerSingleStepSubroutineExecution();
  testProgramRunnerDirectJumpExecution();
  testProgramRunnerDirectConditionalExecution();
  testProgramRunnerIndirectJumpExecution();
  testProgramRunnerIndirectConditionalExecution();
  testProgramRunnerRegisterExecution();
  testProgramRunnerIndirectSubroutineExecution();
  testProgramRunnerDsnzExecution();
  testProgramRunnerSubroutineExecution();
  testProgramRunnerNestedSubroutineExecution();
  testProgramRunnerReturnStopsExecution();
  testProgramRunnerInvalidTargetStops();
  testProgramRunnerIndirectInvalidTargetStops();
  testProgramRunnerCallStackOverflowStops();
  testProgramRunnerInvalidStepStops();
  testExampleFactorialProgram();
  testExampleTriangularProgram();
  testExampleAbsoluteValueProgram();
  testExampleIndirectJumpProgram();
  testExampleIndirectSubroutineProgram();
  testExampleIndirectSignClassifierProgram();
  std::cout << "RpnCalculator regression tests passed.\n";
  return 0;
}
