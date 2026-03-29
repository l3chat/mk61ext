#include "RpnCalculator.h"

#include <cmath>

namespace {

bool isNegative(CalculatorValue value) {
  return std::signbit(value);
}

constexpr CalculatorValue kPi = 3.14159265358979323846;

}  // namespace

RpnCalculator::RpnCalculator() {
  reset();
}

void RpnCalculator::reset() {
  stack_ = {0.0, 0.0, 0.0, 0.0};
  entering_ = false;
  decimalMode_ = false;
  decimalScale_ = 0.1;
  enteringExponent_ = false;
  stackLiftEnabled_ = false;
  exponentMantissa_ = 0.0;
  lastX_ = 0.0;
  exponentValue_ = 0;
  exponentSign_ = 1;
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
    case CalculatorAction::EnterExponent:
      return pressEnterExponent();
    case CalculatorAction::ChangeSign:
      return toggleSign();
    case CalculatorAction::Add:
    case CalculatorAction::Subtract:
    case CalculatorAction::Multiply:
    case CalculatorAction::Divide:
      return performBinaryOperation(action);
    case CalculatorAction::Reciprocal:
      return reciprocal();
    case CalculatorAction::SquareRoot:
      return squareRoot();
    case CalculatorAction::Square:
      return square();
    case CalculatorAction::PowerXY:
      return powerXY();
    case CalculatorAction::Exp10:
      return exp10();
    case CalculatorAction::ExpE:
      return expE();
    case CalculatorAction::Log10:
      return log10Value();
    case CalculatorAction::NaturalLog:
      return naturalLog();
    case CalculatorAction::Sin:
      return sine();
    case CalculatorAction::Cos:
      return cosine();
    case CalculatorAction::Tan:
      return tangent();
    case CalculatorAction::Asin:
      return arcsine();
    case CalculatorAction::Acos:
      return arccosine();
    case CalculatorAction::Atan:
      return arctangent();
    case CalculatorAction::Pi:
      return setPi();
    case CalculatorAction::LastX:
      return recallLastX();
    case CalculatorAction::SwapXY:
      return swapXY();
    case CalculatorAction::RollDown:
      return rollDown();
    case CalculatorAction::IntegerPart:
      return integerPart();
    case CalculatorAction::FractionalPart:
      return fractionalPart();
    case CalculatorAction::AbsoluteValue:
      return absoluteValue();
    case CalculatorAction::Sign:
      return sign();
    case CalculatorAction::MaxXY:
      return maxXY();
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
    case CalculatorError::NegativeSquareRoot:
      return "negative sqrt";
    case CalculatorError::DomainError:
      return "domain error";
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

  if (enteringExponent_) {
    exponentValue_ = (exponentValue_ * 10) + digit;
    updateExponentValue();
    return true;
  }

  if (decimalMode_) {
    const CalculatorValue delta = static_cast<CalculatorValue>(digit) * decimalScale_;
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

  if (enteringExponent_) {
    return false;
  }

  if (decimalMode_) {
    return false;
  }

  decimalMode_ = true;
  decimalScale_ = 0.1;
  return true;
}

bool RpnCalculator::pressEnterExponent() {
  if (hasError()) {
    clearX();
  }

  if (enteringExponent_) {
    return false;
  }

  entering_ = true;
  decimalMode_ = false;
  enteringExponent_ = true;
  stackLiftEnabled_ = false;
  exponentMantissa_ = stack_[0];
  exponentValue_ = 0;
  exponentSign_ = 1;
  updateExponentValue();
  return true;
}

bool RpnCalculator::pressEnter() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  liftStack();
  stackLiftEnabled_ = false;
  return true;
}

bool RpnCalculator::toggleSign() {
  if (hasError()) {
    clearX();
  }

  rememberLastX();

  if (enteringExponent_) {
    exponentSign_ = -exponentSign_;
    updateExponentValue();
    return true;
  }

  stack_[0] = -stack_[0];
  stackLiftEnabled_ = !entering_;
  return true;
}

bool RpnCalculator::reciprocal() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if (stack_[0] == 0.0) {
    error_ = CalculatorError::DivideByZero;
    return false;
  }

  stack_[0] = 1.0 / stack_[0];
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::squareRoot() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if (stack_[0] < 0.0) {
    error_ = CalculatorError::NegativeSquareRoot;
    return false;
  }

  stack_[0] = std::sqrt(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::square() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] *= stack_[0];
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::powerXY() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue x = stack_[0];
  const CalculatorValue y = stack_[1];
  const CalculatorValue result = std::pow(x, y);

  if (std::isnan(result) || !std::isfinite(result)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = result;
  dropStack();
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::exp10() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue result = std::pow(10.0, stack_[0]);
  if (!std::isfinite(result)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = result;
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::expE() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue result = std::exp(stack_[0]);
  if (!std::isfinite(result)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = result;
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::log10Value() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if (stack_[0] <= 0.0) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = std::log10(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::naturalLog() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if (stack_[0] <= 0.0) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = std::log(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::sine() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = std::sin(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::cosine() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = std::cos(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::tangent() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue result = std::tan(stack_[0]);
  if (!std::isfinite(result)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = result;
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::arcsine() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if ((stack_[0] < -1.0) || (stack_[0] > 1.0)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = std::asin(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::arccosine() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if ((stack_[0] < -1.0) || (stack_[0] > 1.0)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = std::acos(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::arctangent() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = std::atan(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::setPi() {
  if (hasError()) {
    clearX();
  }

  rememberLastX();
  stack_[0] = kPi;
  finishEntry();
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::recallLastX() {
  if (hasError()) {
    clearX();
  }

  stack_[0] = lastX_;
  finishEntry();
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::swapXY() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue x = stack_[0];
  stack_[0] = stack_[1];
  stack_[1] = x;
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::rollDown() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  const CalculatorValue x = stack_[0];
  stack_[0] = stack_[1];
  stack_[1] = stack_[2];
  stack_[2] = stack_[3];
  stack_[3] = x;
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::integerPart() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = std::trunc(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::fractionalPart() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = stack_[0] - std::trunc(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::absoluteValue() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = std::fabs(stack_[0]);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::sign() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  if (stack_[0] > 0.0) {
    stack_[0] = 1.0;
  } else if (stack_[0] < 0.0) {
    stack_[0] = -1.0;
  } else {
    stack_[0] = 0.0;
  }

  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::maxXY() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = (stack_[0] > stack_[1]) ? stack_[0] : stack_[1];
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::clearX() {
  rememberLastX();
  stack_[0] = 0.0;
  finishEntry();
  clearError();
  stackLiftEnabled_ = false;
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
  rememberLastX();

  const CalculatorValue x = stack_[0];
  const CalculatorValue y = stack_[1];
  CalculatorValue result = 0.0;

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
  stackLiftEnabled_ = true;
  return true;
}

void RpnCalculator::rememberLastX() {
  lastX_ = stack_[0];
}

void RpnCalculator::startEntry() {
  if (stackLiftEnabled_) {
    liftStack();
  }

  stack_[0] = 0.0;
  entering_ = true;
  decimalMode_ = false;
  decimalScale_ = 0.1;
  enteringExponent_ = false;
  stackLiftEnabled_ = false;
  exponentMantissa_ = 0.0;
  exponentValue_ = 0;
  exponentSign_ = 1;
}

void RpnCalculator::finishEntry() {
  entering_ = false;
  decimalMode_ = false;
  decimalScale_ = 0.1;
  enteringExponent_ = false;
  exponentMantissa_ = 0.0;
  exponentValue_ = 0;
  exponentSign_ = 1;
}

void RpnCalculator::clearError() {
  error_ = CalculatorError::None;
}

void RpnCalculator::updateExponentValue() {
  stack_[0] = exponentMantissa_ * std::pow(10.0, exponentSign_ * exponentValue_);
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
