#include "RpnCalculator.h"

#include <cmath>

namespace {

bool isNegative(double value) {
  return std::signbit(value);
}

}  // namespace

RpnCalculator::RpnCalculator() {
  reset();
}

void RpnCalculator::reset() {
  stack_ = {0.0, 0.0, 0.0, 0.0};
  entering_ = false;
  decimalMode_ = false;
  decimalScale_ = 0.1;
  error_ = CalculatorError::None;
}

bool RpnCalculator::apply(CalculatorAction action) {
  switch (action) {
    case CalculatorAction::None:
      return false;
    case CalculatorAction::Digit0:
      return enterDigit(0);
    case CalculatorAction::Digit1:
      return enterDigit(1);
    case CalculatorAction::Digit2:
      return enterDigit(2);
    case CalculatorAction::Digit3:
      return enterDigit(3);
    case CalculatorAction::Digit4:
      return enterDigit(4);
    case CalculatorAction::Digit5:
      return enterDigit(5);
    case CalculatorAction::Digit6:
      return enterDigit(6);
    case CalculatorAction::Digit7:
      return enterDigit(7);
    case CalculatorAction::Digit8:
      return enterDigit(8);
    case CalculatorAction::Digit9:
      return enterDigit(9);
    case CalculatorAction::DecimalPoint:
      return enterDecimalPoint();
    case CalculatorAction::Enter:
      return pressEnter();
    case CalculatorAction::ChangeSign:
      return toggleSign();
    case CalculatorAction::Add:
    case CalculatorAction::Subtract:
    case CalculatorAction::Multiply:
    case CalculatorAction::Divide:
      return performBinaryOperation(action);
    case CalculatorAction::SwapXY:
      return swapXY();
    case CalculatorAction::Drop:
      return drop();
    case CalculatorAction::ClearX:
      return clearX();
    case CalculatorAction::ClearAll:
      return clearAll();
  }

  return false;
}

CalculatorStack RpnCalculator::stack() const {
  return CalculatorStack{
      stack_[0],
      stack_[1],
      stack_[2],
      stack_[3],
  };
}

const char *RpnCalculator::errorMessage() const {
  switch (error_) {
    case CalculatorError::None:
      return "";
    case CalculatorError::DivideByZero:
      return "divide by zero";
  }

  return "unknown error";
}

bool RpnCalculator::enterDigit(uint8_t digit) {
  if (hasError()) {
    clearX();
  }

  if (!entering_) {
    startEntry();
  }

  if (decimalMode_) {
    const double delta = static_cast<double>(digit) * decimalScale_;
    stack_[0] = isNegative(stack_[0]) ? stack_[0] - delta : stack_[0] + delta;
    decimalScale_ /= 10.0;
    return true;
  }

  if (isNegative(stack_[0])) {
    stack_[0] = (stack_[0] * 10.0) - digit;
  } else {
    stack_[0] = (stack_[0] * 10.0) + digit;
  }

  return true;
}

bool RpnCalculator::enterDecimalPoint() {
  if (hasError()) {
    clearX();
  }

  if (!entering_) {
    startEntry();
  }

  if (decimalMode_) {
    return false;
  }

  decimalMode_ = true;
  decimalScale_ = 0.1;
  return true;
}

bool RpnCalculator::pressEnter() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  liftStack();
  return true;
}

bool RpnCalculator::toggleSign() {
  if (hasError()) {
    clearX();
  }

  stack_[0] = -stack_[0];
  return true;
}

bool RpnCalculator::swapXY() {
  if (hasError()) {
    return false;
  }

  finishEntry();

  const double x = stack_[0];
  stack_[0] = stack_[1];
  stack_[1] = x;
  return true;
}

bool RpnCalculator::drop() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  stack_[0] = stack_[1];
  dropStack();
  return true;
}

bool RpnCalculator::clearX() {
  stack_[0] = 0.0;
  finishEntry();
  clearError();
  return true;
}

bool RpnCalculator::clearAll() {
  reset();
  return true;
}

bool RpnCalculator::performBinaryOperation(CalculatorAction action) {
  if (hasError()) {
    return false;
  }

  finishEntry();

  const double x = stack_[0];
  const double y = stack_[1];
  double result = 0.0;

  switch (action) {
    case CalculatorAction::Add:
      result = y + x;
      break;
    case CalculatorAction::Subtract:
      result = y - x;
      break;
    case CalculatorAction::Multiply:
      result = y * x;
      break;
    case CalculatorAction::Divide:
      if (x == 0.0) {
        error_ = CalculatorError::DivideByZero;
        return false;
      }
      result = y / x;
      break;
    default:
      return false;
  }

  stack_[0] = result;
  dropStack();
  return true;
}

void RpnCalculator::startEntry() {
  stack_[0] = 0.0;
  entering_ = true;
  decimalMode_ = false;
  decimalScale_ = 0.1;
}

void RpnCalculator::finishEntry() {
  entering_ = false;
  decimalMode_ = false;
  decimalScale_ = 0.1;
}

void RpnCalculator::clearError() {
  error_ = CalculatorError::None;
}

void RpnCalculator::liftStack() {
  stack_[3] = stack_[2];
  stack_[2] = stack_[1];
  stack_[1] = stack_[0];
}

void RpnCalculator::dropStack() {
  stack_[1] = stack_[2];
  stack_[2] = stack_[3];
}
