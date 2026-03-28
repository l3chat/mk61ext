#ifndef RPN_CALCULATOR_H
#define RPN_CALCULATOR_H

#include <array>
#include <cstdint>
#include <limits>

using CalculatorValue = double;

static_assert(sizeof(CalculatorValue) == 8, "mk61ext requires 64-bit floating-point values");
static_assert(std::numeric_limits<CalculatorValue>::digits == 53,
              "mk61ext expects IEEE-754 double precision");

enum class CalculatorAction : uint8_t {
  None,
  Digit0,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  Digit6,
  Digit7,
  Digit8,
  Digit9,
  DecimalPoint,
  Enter,
  EnterExponent,
  ChangeSign,
  Add,
  Subtract,
  Multiply,
  Divide,
  Reciprocal,
  SquareRoot,
  Pi,
  SwapXY,
  RollDown,
  ClearX,
  ClearAll,
};

enum class CalculatorError : uint8_t {
  None,
  DivideByZero,
  NegativeSquareRoot,
};

struct CalculatorStack {
  CalculatorValue x;
  CalculatorValue y;
  CalculatorValue z;
  CalculatorValue t;
};

class RpnCalculator {
public:
  RpnCalculator();

  void reset();
  bool apply(CalculatorAction action);

  CalculatorStack stack() const;
  bool isEntering() const { return entering_; }
  bool isEnteringExponent() const { return enteringExponent_; }
  bool hasError() const { return error_ != CalculatorError::None; }
  CalculatorError error() const { return error_; }
  const char *errorMessage() const;

private:
  bool enterDigit(uint8_t digit);
  bool enterDecimalPoint();
  bool pressEnter();
  bool pressEnterExponent();
  bool toggleSign();
  bool reciprocal();
  bool squareRoot();
  bool setPi();
  bool swapXY();
  bool rollDown();
  bool clearX();
  bool clearAll();
  bool performBinaryOperation(CalculatorAction action);

  void startEntry();
  void finishEntry();
  void clearError();
  void updateExponentValue();
  void liftStack();
  void dropStack();

  std::array<CalculatorValue, 4> stack_{{0.0, 0.0, 0.0, 0.0}};
  bool entering_ = false;
  bool decimalMode_ = false;
  bool enteringExponent_ = false;
  CalculatorValue decimalScale_ = 0.1;
  CalculatorValue exponentMantissa_ = 0.0;
  int exponentValue_ = 0;
  int exponentSign_ = 1;
  CalculatorError error_ = CalculatorError::None;
};

#endif
