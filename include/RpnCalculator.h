#ifndef RPN_CALCULATOR_H
#define RPN_CALCULATOR_H

#include <array>
#include <cstdint>

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
  ChangeSign,
  Add,
  Subtract,
  Multiply,
  Divide,
  SwapXY,
  Drop,
  ClearX,
  ClearAll,
};

enum class CalculatorError : uint8_t {
  None,
  DivideByZero,
};

struct CalculatorStack {
  double x;
  double y;
  double z;
  double t;
};

class RpnCalculator {
public:
  RpnCalculator();

  void reset();
  bool apply(CalculatorAction action);

  CalculatorStack stack() const;
  bool isEntering() const { return entering_; }
  bool hasError() const { return error_ != CalculatorError::None; }
  CalculatorError error() const { return error_; }
  const char *errorMessage() const;

private:
  bool enterDigit(uint8_t digit);
  bool enterDecimalPoint();
  bool pressEnter();
  bool toggleSign();
  bool swapXY();
  bool drop();
  bool clearX();
  bool clearAll();
  bool performBinaryOperation(CalculatorAction action);

  void startEntry();
  void finishEntry();
  void clearError();
  void liftStack();
  void dropStack();

  std::array<double, 4> stack_{{0.0, 0.0, 0.0, 0.0}};
  bool entering_ = false;
  bool decimalMode_ = false;
  double decimalScale_ = 0.1;
  CalculatorError error_ = CalculatorError::None;
};

#endif
