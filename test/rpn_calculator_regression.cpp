#include "RpnCalculator.h"

#include <cstdint>
#include <cstdlib>
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

void testRegisterStoreRecall() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "42");
  expectTrue(calculator.storeRegister(5), "store register 5 should succeed");
  expectEqual(calculator.stack().x, 42.0, "storing should leave X unchanged");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallRegister(5), "recall register 5 should succeed");
  expectEqual(calculator.stack().x, 42.0, "recalling register 5 should restore 42");

  clearAndEnter(calculator, "88");
  expectTrue(calculator.storeRegister(14), "store register e should succeed");

  press(calculator, CalculatorAction::ClearAll, "clear all should succeed");
  expectEqual(calculator.stack().x, 0.0, "clear all should clear X");
  expectTrue(calculator.recallRegister(14), "recall register e should succeed after clear all");
  expectEqual(calculator.stack().x, 88.0, "clear all should not erase stored registers");
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
  expectTrue(calculator.recallRegister(7), "direct recall of register 7 should succeed");
  expectEqual(calculator.stack().x, 6.0, "register 7 should stay unchanged during indirect recall");

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
  expectTrue(calculator.recallRegister(4), "direct recall of register 4 should succeed");
  expectEqual(calculator.stack().x, 6.0, "register 4 should be incremented to 6 after indirect recall");

  clearAndEnter(calculator, "55");
  expectTrue(calculator.storeRegister(5), "store register 5 should succeed");
  clearAndEnter(calculator, "5");
  expectTrue(calculator.storeRegister(3), "store register 3 should succeed");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallIndirectRegister(3), "indirect recall through register 3 should succeed");
  expectEqual(calculator.stack().x, 55.0, "register 3 should recall register 5 before post-decrement");
  expectTrue(calculator.recallRegister(3), "direct recall of register 3 should succeed");
  expectEqual(calculator.stack().x, 4.0, "register 3 should be decremented to 4 after indirect recall");
}

void testIndirectRegisterWrapping() {
  RpnCalculator calculator;

  clearAndEnter(calculator, "123");
  expectTrue(calculator.storeRegister(5), "store register 5 should succeed");
  clearAndEnter(calculator, "20.9");
  expectTrue(calculator.storeRegister(8), "store register 8 should succeed");

  clearAndEnter(calculator, "0");
  expectTrue(calculator.recallIndirectRegister(8), "indirect recall through register 8 should succeed");
  expectEqual(calculator.stack().x, 123.0,
              "indirect recall should truncate and wrap pointer values across registers 0-e");
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

}  // namespace

int main() {
  testBitwiseBinaryOps();
  testUnsignedBehavior();
  testBitwiseValidation();
  testRegisterStoreRecall();
  testRegisterValidation();
  testIndirectRegisterAccess();
  testIndirectPointerAutoModification();
  testIndirectRegisterWrapping();
  testIndirectRegisterValidation();
  std::cout << "RpnCalculator regression tests passed.\n";
  return 0;
}
