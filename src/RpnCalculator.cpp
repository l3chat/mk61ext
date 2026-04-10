#include "RpnCalculator.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>

namespace {

bool isNegative(CalculatorValue value) {
  return std::signbit(value);
}

constexpr CalculatorValue kPi = 3.14159265358979323846;
constexpr CalculatorValue kDegreesPerCircle = 360.0;
constexpr CalculatorValue kGradiansPerCircle = 400.0;
constexpr CalculatorValue kMinuteLimit = 60.0;
constexpr CalculatorValue kSecondLimit = 60.0;
constexpr CalculatorValue kNormalizationEpsilon = 1e-9;

bool isIndirectPostDecrementRegister(uint8_t index) {
  return index <= 3;
}

bool isIndirectPreIncrementRegister(uint8_t index) {
  return (index >= 4) && (index <= 6);
}

bool coerceToBitwiseUint32(CalculatorValue value, uint32_t &result) {
  if (!std::isfinite(value)) {
    return false;
  }

  const CalculatorValue rounded = std::round(value);
  if (std::fabs(value - rounded) > kNormalizationEpsilon) {
    return false;
  }

  if ((rounded < 0.0) ||
      (rounded > static_cast<CalculatorValue>(std::numeric_limits<uint32_t>::max()))) {
    return false;
  }

  result = static_cast<uint32_t>(rounded);
  return true;
}

bool coerceToIndirectRegisterIndex(CalculatorValue value, uint8_t &result) {
  if (!std::isfinite(value)) {
    return false;
  }

  CalculatorValue wrapped =
      std::fmod(std::trunc(value), static_cast<CalculatorValue>(RpnCalculator::kRegisterCount));
  if (wrapped < 0.0) {
    wrapped += static_cast<CalculatorValue>(RpnCalculator::kRegisterCount);
  }

  result = static_cast<uint8_t>(wrapped);
  return true;
}

void formatNormalizedValue(CalculatorValue value, char *buffer, size_t bufferSize, int precision) {
  const CalculatorValue normalizedValue = (std::fabs(value) < 1e-12) ? 0.0 : value;
  std::snprintf(buffer, bufferSize, "%.*g", precision, normalizedValue);
}

CalculatorValue angleToRadians(CalculatorValue value, CalculatorAngleMode mode) {
  switch (mode) {
    case CalculatorAngleMode::Radians:
      return value;
    case CalculatorAngleMode::Gradians:
      return value * (2.0 * kPi / kGradiansPerCircle);
    case CalculatorAngleMode::Degrees:
      return value * (2.0 * kPi / kDegreesPerCircle);
  }

  return value;
}

CalculatorValue radiansToAngle(CalculatorValue value, CalculatorAngleMode mode) {
  switch (mode) {
    case CalculatorAngleMode::Radians:
      return value;
    case CalculatorAngleMode::Gradians:
      return value * (kGradiansPerCircle / (2.0 * kPi));
    case CalculatorAngleMode::Degrees:
      return value * (kDegreesPerCircle / (2.0 * kPi));
  }

  return value;
}

}  // namespace

RpnCalculator::RpnCalculator() {
  reset();
}

void RpnCalculator::reset() {
  registers_.fill(0.0);
  angleMode_ = CalculatorAngleMode::Radians;
  clearStackState();
}

bool RpnCalculator::recallRegister(uint8_t index) {
  if (hasError()) {
    return false;
  }

  if (!isValidRegisterIndex(index)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  finishEntry();
  rememberLastX();
  liftStack();
  stack_[0] = registers_[index];
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::storeRegister(uint8_t index) {
  if (hasError()) {
    return false;
  }

  if (!isValidRegisterIndex(index)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  finishEntry();
  registers_[index] = stack_[0];
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::recallIndirectRegister(uint8_t pointerIndex) {
  if (hasError()) {
    return false;
  }

  finishEntry();

  uint8_t targetIndex = 0;
  if (!prepareIndirectRegisterAccess(pointerIndex, targetIndex)) {
    return false;
  }

  rememberLastX();
  liftStack();
  stack_[0] = registers_[targetIndex];
  finishIndirectRegisterAccess(pointerIndex);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::storeIndirectRegister(uint8_t pointerIndex) {
  if (hasError()) {
    return false;
  }

  finishEntry();

  uint8_t targetIndex = 0;
  if (!prepareIndirectRegisterAccess(pointerIndex, targetIndex)) {
    return false;
  }

  registers_[targetIndex] = stack_[0];
  finishIndirectRegisterAccess(pointerIndex);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::readRegister(uint8_t index, CalculatorValue &value) const {
  if (hasError()) {
    return false;
  }

  if (!isValidRegisterIndex(index)) {
    return false;
  }

  value = registers_[index];
  return true;
}

bool RpnCalculator::writeRegister(uint8_t index, CalculatorValue value) {
  if (hasError()) {
    return false;
  }

  if (!isValidRegisterIndex(index)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  registers_[index] = value;
  return true;
}

void RpnCalculator::commitEntry() {
  if (hasError()) {
    return;
  }

  finishEntry();
}

void RpnCalculator::clearStackState() {
  stack_ = {0.0, 0.0, 0.0, 0.0};
  entering_ = false;
  decimalMode_ = false;
  enteringExponent_ = false;
  stackLiftEnabled_ = false;
  clearEntryBuffer();
  lastX_ = 0.0;
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
    case CalculatorAction::BitwiseAnd:
      return bitwiseAnd();
    case CalculatorAction::BitwiseOr:
      return bitwiseOr();
    case CalculatorAction::BitwiseXor:
      return bitwiseXor();
    case CalculatorAction::BitwiseNot:
      return bitwiseNot();
    case CalculatorAction::HourToHourMinute:
      return hourToHourMinute();
    case CalculatorAction::HourMinuteToHour:
      return hourMinuteToHour();
    case CalculatorAction::HourToHourMinuteSecond:
      return hourToHourMinuteSecond();
    case CalculatorAction::HourMinuteSecondToHour:
      return hourMinuteSecondToHour();
    case CalculatorAction::RandomValue:
      return randomValue();
    case CalculatorAction::ClearX:
      return clearX();
    case CalculatorAction::ClearAll:
      return clearAll();
  }

  return false;
}

void RpnCalculator::seedRandom(uint32_t seed) {
  randomState_ = (seed != 0) ? seed : 1;
}

CalculatorStack RpnCalculator::stack() const {
  return CalculatorStack{
      stack_[0],
      stack_[1],
      stack_[2],
      stack_[3],
  };
}

void RpnCalculator::formatXForDisplay(char *buffer, size_t bufferSize, int precision) const {
  if ((buffer == nullptr) || (bufferSize == 0)) {
    return;
  }

  if (entering_) {
    if (entryLength_ == 0) {
      formatNormalizedValue(stack_[0], buffer, bufferSize, precision);
    } else {
      std::snprintf(buffer, bufferSize, "%s", entryBuffer_.data());
    }
    return;
  }

  formatNormalizedValue(stack_[0], buffer, bufferSize, precision);
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

  const auto previousBuffer = entryBuffer_;
  const size_t previousLength = entryLength_;
  const CalculatorValue previousX = stack_[0];

  if (!enteringExponent_ && !decimalMode_ && mantissaIsSimpleZero()) {
    const size_t digitIndex = (entryLength_ > 0 && entryBuffer_[0] == '-') ? 1 : 0;
    entryBuffer_[digitIndex] = static_cast<char>('0' + digit);
  } else if (!appendEntryChar(static_cast<char>('0' + digit))) {
    return false;
  }

  refreshEntryFlags();
  if (mantissaSignificantDigitCount() > kMaxMantissaDigits) {
    entryBuffer_ = previousBuffer;
    entryLength_ = previousLength;
    stack_[0] = previousX;
    refreshEntryFlags();
    return false;
  }

  if (syncValueFromEntryBuffer()) {
    return true;
  }

  entryBuffer_ = previousBuffer;
  entryLength_ = previousLength;
  stack_[0] = previousX;
  refreshEntryFlags();
  return false;
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

  if (entryLength_ == 0) {
    if (!appendEntryChar('0') || !appendEntryChar('.')) {
      return false;
    }
  } else if ((entryLength_ == 1) && (entryBuffer_[0] == '-')) {
    if (!appendEntryChar('0') || !appendEntryChar('.')) {
      return false;
    }
  } else if (!appendEntryChar('.')) {
    return false;
  }

  refreshEntryFlags();
  if (!syncValueFromEntryBuffer()) {
    return false;
  }
  return true;
}

bool RpnCalculator::pressEnterExponent() {
  if (hasError()) {
    clearX();
  }

  if (enteringExponent_) {
    return false;
  }

  if (!entering_) {
    entering_ = true;
    stackLiftEnabled_ = false;
    seedEntryBufferFromCurrentX();
  }

  if (!appendEntryChar('e')) {
    return false;
  }

  refreshEntryFlags();
  if (!syncValueFromEntryBuffer()) {
    return false;
  }
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
    const size_t exponentMarker = exponentMarkerIndex();
    if (exponentMarker >= entryLength_) {
      return false;
    }

    const size_t signIndex = exponentMarker + 1;
    if ((signIndex < entryLength_) && (entryBuffer_[signIndex] == '-')) {
      removeEntryChar(signIndex);
    } else if (!insertEntryChar(signIndex, '-')) {
      return false;
    }

    refreshEntryFlags();
    syncValueFromEntryBuffer();
    return true;
  }

  if (entering_) {
    if (entryLength_ == 0) {
      if (!appendEntryChar('-') || !appendEntryChar('0')) {
        return false;
      }
    } else if (entryBuffer_[0] == '-') {
      removeEntryChar(0);
    } else if (!insertEntryChar(0, '-')) {
      return false;
    }

    refreshEntryFlags();
    syncValueFromEntryBuffer();
    stackLiftEnabled_ = false;
    return true;
  }

  stack_[0] = -stack_[0];
  stackLiftEnabled_ = true;
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
  stack_[0] = std::sin(angleToRadians(stack_[0], angleMode_));
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::cosine() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = std::cos(angleToRadians(stack_[0], angleMode_));
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::tangent() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue result = std::tan(angleToRadians(stack_[0], angleMode_));
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

  stack_[0] = radiansToAngle(std::asin(stack_[0]), angleMode_);
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

  stack_[0] = radiansToAngle(std::acos(stack_[0]), angleMode_);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::arctangent() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();
  stack_[0] = radiansToAngle(std::atan(stack_[0]), angleMode_);
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

bool RpnCalculator::bitwiseAnd() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  uint32_t x = 0;
  uint32_t y = 0;
  if (!coerceToBitwiseUint32(stack_[0], x) || !coerceToBitwiseUint32(stack_[1], y)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = static_cast<CalculatorValue>(y & x);
  dropStack();
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::bitwiseOr() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  uint32_t x = 0;
  uint32_t y = 0;
  if (!coerceToBitwiseUint32(stack_[0], x) || !coerceToBitwiseUint32(stack_[1], y)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = static_cast<CalculatorValue>(y | x);
  dropStack();
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::bitwiseXor() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  uint32_t x = 0;
  uint32_t y = 0;
  if (!coerceToBitwiseUint32(stack_[0], x) || !coerceToBitwiseUint32(stack_[1], y)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = static_cast<CalculatorValue>(y ^ x);
  dropStack();
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::bitwiseNot() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  uint32_t x = 0;
  if (!coerceToBitwiseUint32(stack_[0], x)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = static_cast<CalculatorValue>(~x);
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::hourToHourMinute() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue sign = isNegative(stack_[0]) ? -1.0 : 1.0;
  CalculatorValue magnitude = std::fabs(stack_[0]);
  CalculatorValue hours = std::trunc(magnitude);
  CalculatorValue minutes = (magnitude - hours) * 60.0;

  if (minutes >= (kMinuteLimit - kNormalizationEpsilon)) {
    hours += 1.0;
    minutes = 0.0;
  }

  stack_[0] = sign * (hours + (minutes / 100.0));
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::hourMinuteToHour() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue sign = isNegative(stack_[0]) ? -1.0 : 1.0;
  const CalculatorValue magnitude = std::fabs(stack_[0]);
  const CalculatorValue hours = std::trunc(magnitude);
  const CalculatorValue minuteValue = (magnitude - hours) * 100.0;

  if (minuteValue >= (kMinuteLimit + kNormalizationEpsilon)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = sign * (hours + (minuteValue / 60.0));
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::hourToHourMinuteSecond() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue sign = isNegative(stack_[0]) ? -1.0 : 1.0;
  const CalculatorValue magnitude = std::fabs(stack_[0]);
  CalculatorValue hours = std::trunc(magnitude);
  CalculatorValue minuteValue = (magnitude - hours) * 60.0;
  CalculatorValue minutes = std::trunc(minuteValue);
  CalculatorValue seconds = (minuteValue - minutes) * 60.0;

  if (seconds >= (kSecondLimit - kNormalizationEpsilon)) {
    minutes += 1.0;
    seconds = 0.0;
  }

  if (minutes >= (kMinuteLimit - kNormalizationEpsilon)) {
    hours += 1.0;
    minutes = 0.0;
  }

  stack_[0] = sign * (hours + (minutes / 100.0) + (seconds / 10000.0));
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::hourMinuteSecondToHour() {
  if (hasError()) {
    return false;
  }

  finishEntry();
  rememberLastX();

  const CalculatorValue sign = isNegative(stack_[0]) ? -1.0 : 1.0;
  const CalculatorValue magnitude = std::fabs(stack_[0]);
  const CalculatorValue hours = std::trunc(magnitude);
  const CalculatorValue minuteSecondValue = (magnitude - hours) * 100.0;
  const CalculatorValue minutes = std::trunc(minuteSecondValue);
  const CalculatorValue seconds = (minuteSecondValue - minutes) * 100.0;

  if ((minutes >= (kMinuteLimit + kNormalizationEpsilon)) ||
      (seconds >= (kSecondLimit + kNormalizationEpsilon))) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  stack_[0] = sign * (hours + (minutes / 60.0) + (seconds / 3600.0));
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::randomValue() {
  if (hasError()) {
    clearX();
  }

  finishEntry();
  rememberLastX();
  stack_[0] = static_cast<CalculatorValue>(nextRandomUint32()) / 4294967296.0;
  stackLiftEnabled_ = true;
  return true;
}

bool RpnCalculator::clearX() {
  if (entering_) {
    clearError();

    if (entryLength_ > 0) {
      removeEntryChar(entryLength_ - 1);
    }

    if ((entryLength_ == 0) || ((entryLength_ == 1) && (entryBuffer_[0] == '-'))) {
      stack_[0] = 0.0;
      entering_ = false;
      clearEntryBuffer();
      refreshEntryFlags();
      stackLiftEnabled_ = false;
      return true;
    }

    refreshEntryFlags();
    if (!syncValueFromEntryBuffer()) {
      stack_[0] = 0.0;
      entering_ = false;
      clearEntryBuffer();
      refreshEntryFlags();
    }
    stackLiftEnabled_ = false;
    return true;
  }

  rememberLastX();
  stack_[0] = 0.0;
  finishEntry();
  clearError();
  stackLiftEnabled_ = false;
  return true;
}

bool RpnCalculator::clearAll() {
  clearStackState();
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

bool RpnCalculator::isValidRegisterIndex(uint8_t index) const {
  return index < registers_.size();
}

bool RpnCalculator::prepareIndirectRegisterAccess(uint8_t pointerIndex, uint8_t &targetIndex) {
  if (!isValidRegisterIndex(pointerIndex)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  CalculatorValue pointerValue = registers_[pointerIndex];
  if (isIndirectPreIncrementRegister(pointerIndex)) {
    pointerValue += 1.0;
    registers_[pointerIndex] = pointerValue;
  }

  if (!coerceToIndirectRegisterIndex(pointerValue, targetIndex)) {
    error_ = CalculatorError::DomainError;
    return false;
  }

  return true;
}

void RpnCalculator::finishIndirectRegisterAccess(uint8_t pointerIndex) {
  if (isIndirectPostDecrementRegister(pointerIndex)) {
    registers_[pointerIndex] -= 1.0;
  }
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
  stackLiftEnabled_ = false;
  clearEntryBuffer();
  refreshEntryFlags();
}

void RpnCalculator::finishEntry() {
  if (entering_ && !syncValueFromEntryBuffer()) {
    error_ = CalculatorError::DomainError;
  }
  entering_ = false;
  clearEntryBuffer();
  refreshEntryFlags();
}

void RpnCalculator::clearError() {
  error_ = CalculatorError::None;
}

void RpnCalculator::clearEntryBuffer() {
  entryLength_ = 0;
  entryBuffer_[0] = '\0';
}

bool RpnCalculator::appendEntryChar(char ch) {
  if (entryLength_ + 1 >= entryBuffer_.size()) {
    return false;
  }

  entryBuffer_[entryLength_] = ch;
  entryLength_ += 1;
  entryBuffer_[entryLength_] = '\0';
  return true;
}

bool RpnCalculator::insertEntryChar(size_t index, char ch) {
  if ((index > entryLength_) || (entryLength_ + 1 >= entryBuffer_.size())) {
    return false;
  }

  std::memmove(entryBuffer_.data() + index + 1,
               entryBuffer_.data() + index,
               entryLength_ - index + 1);
  entryBuffer_[index] = ch;
  entryLength_ += 1;
  return true;
}

void RpnCalculator::removeEntryChar(size_t index) {
  if (index >= entryLength_) {
    return;
  }

  std::memmove(entryBuffer_.data() + index,
               entryBuffer_.data() + index + 1,
               entryLength_ - index);
  entryLength_ -= 1;
}

void RpnCalculator::refreshEntryFlags() {
  const size_t exponentMarker = exponentMarkerIndex();
  enteringExponent_ = exponentMarker < entryLength_;
  decimalMode_ = false;

  for (size_t index = 0; index < exponentMarker; ++index) {
    if (entryBuffer_[index] == '.') {
      decimalMode_ = true;
      break;
    }
  }
}

bool RpnCalculator::exponentHasDigits() const {
  const size_t exponentMarker = exponentMarkerIndex();
  if (exponentMarker >= entryLength_) {
    return false;
  }

  size_t digitIndex = exponentMarker + 1;
  if ((digitIndex < entryLength_) && (entryBuffer_[digitIndex] == '-')) {
    digitIndex += 1;
  }

  return digitIndex < entryLength_;
}

size_t RpnCalculator::exponentMarkerIndex() const {
  for (size_t index = 0; index < entryLength_; ++index) {
    if ((entryBuffer_[index] == 'e') || (entryBuffer_[index] == 'E')) {
      return index;
    }
  }

  return entryLength_;
}

bool RpnCalculator::mantissaIsSimpleZero() const {
  return ((entryLength_ == 1) && (entryBuffer_[0] == '0')) ||
         ((entryLength_ == 2) && (entryBuffer_[0] == '-') && (entryBuffer_[1] == '0'));
}

size_t RpnCalculator::mantissaSignificantDigitCount() const {
  const size_t exponentMarker = exponentMarkerIndex();
  bool seenNonZero = false;
  size_t digitCount = 0;

  for (size_t index = 0; index < exponentMarker; ++index) {
    const char ch = entryBuffer_[index];
    if ((ch == '-') || (ch == '.')) {
      continue;
    }

    if (!seenNonZero) {
      if (ch == '0') {
        continue;
      }
      seenNonZero = true;
    }

    digitCount += 1;
  }

  if (!seenNonZero && decimalMode_) {
    return 0;
  }

  return digitCount;
}

bool RpnCalculator::syncValueFromEntryBuffer() {
  if (entryLength_ == 0) {
    stack_[0] = 0.0;
    return true;
  }

  char parseBuffer[kEntryBufferSize];
  std::memcpy(parseBuffer, entryBuffer_.data(), entryLength_ + 1);

  const size_t exponentMarker = exponentMarkerIndex();
  size_t parseLength = entryLength_;
  if ((exponentMarker < entryLength_) && !exponentHasDigits()) {
    parseLength = exponentMarker;
  }

  parseBuffer[parseLength] = '\0';

  char *end = nullptr;
  errno = 0;
  const CalculatorValue parsedValue = std::strtod(parseBuffer, &end);
  if (end == parseBuffer) {
    stack_[0] = 0.0;
    return true;
  }

  if ((errno == ERANGE) || !std::isfinite(parsedValue)) {
    return false;
  }

  stack_[0] = parsedValue;
  return true;
}

void RpnCalculator::seedEntryBufferFromCurrentX() {
  clearEntryBuffer();

  char seedBuffer[kEntryBufferSize];
  formatNormalizedValue(stack_[0], seedBuffer, sizeof(seedBuffer), 15);
  const size_t seedLength = std::strlen(seedBuffer);
  std::memcpy(entryBuffer_.data(), seedBuffer, seedLength + 1);
  entryLength_ = seedLength;
  refreshEntryFlags();
}

uint32_t RpnCalculator::nextRandomUint32() {
  if (randomState_ == 0) {
    randomState_ = 1;
  }

  uint32_t state = randomState_;
  state ^= state << 13;
  state ^= state >> 17;
  state ^= state << 5;
  randomState_ = state;
  return randomState_;
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
