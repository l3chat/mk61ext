#ifndef RPN_CALCULATOR_H
#define RPN_CALCULATOR_H

#include <array>
#include <cstddef>
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
  Square,
  PowerXY,
  Exp10,
  ExpE,
  Log10,
  NaturalLog,
  Sin,
  Cos,
  Tan,
  Asin,
  Acos,
  Atan,
  Pi,
  LastX,
  SwapXY,
  RollDown,
  IntegerPart,
  FractionalPart,
  AbsoluteValue,
  Sign,
  MaxXY,
  BitwiseAnd,
  BitwiseOr,
  BitwiseXor,
  BitwiseNot,
  Drop,
  HourToHourMinute,
  HourMinuteToHour,
  HourToHourMinuteSecond,
  HourMinuteSecondToHour,
  RandomValue,
  ClearX,
  ClearAll,
};

enum class CalculatorError : uint8_t {
  None,
  DivideByZero,
  NegativeSquareRoot,
  DomainError,
};

enum class CalculatorAngleMode : uint8_t {
  Radians,
  Gradians,
  Degrees,
};

struct CalculatorStack {
  CalculatorValue x;
  CalculatorValue y;
  CalculatorValue z;
  CalculatorValue t;
};

class RpnCalculator {
public:
  static constexpr uint8_t kRegisterCount = 16;
  static constexpr size_t kEntryBufferSize = 64;
  static constexpr size_t kMaxMantissaDigits = 16;

  RpnCalculator();

  void reset();
  bool apply(CalculatorAction action);
  bool recallRegister(uint8_t index);
  bool storeRegister(uint8_t index);
  bool recallIndirectRegister(uint8_t pointerIndex);
  bool storeIndirectRegister(uint8_t pointerIndex);
  bool readRegister(uint8_t index, CalculatorValue &value) const;
  bool writeRegister(uint8_t index, CalculatorValue value);
  void commitEntry();
  void seedRandom(uint32_t seed);
  void setAngleMode(CalculatorAngleMode mode) { angleMode_ = mode; }
  CalculatorAngleMode angleMode() const { return angleMode_; }

  CalculatorStack stack() const;
  void formatXForDisplay(char *buffer, size_t bufferSize, int precision) const;
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
  bool square();
  bool powerXY();
  bool exp10();
  bool expE();
  bool log10Value();
  bool naturalLog();
  bool sine();
  bool cosine();
  bool tangent();
  bool arcsine();
  bool arccosine();
  bool arctangent();
  bool setPi();
  bool recallLastX();
  bool swapXY();
  bool rollDown();
  bool integerPart();
  bool fractionalPart();
  bool absoluteValue();
  bool sign();
  bool maxXY();
  bool bitwiseAnd();
  bool bitwiseOr();
  bool bitwiseXor();
  bool bitwiseNot();
  bool drop();
  bool hourToHourMinute();
  bool hourMinuteToHour();
  bool hourToHourMinuteSecond();
  bool hourMinuteSecondToHour();
  bool randomValue();
  bool clearX();
  bool clearAll();
  bool performBinaryOperation(CalculatorAction action);

  bool isValidRegisterIndex(uint8_t index) const;
  bool prepareIndirectRegisterAccess(uint8_t pointerIndex, uint8_t &targetIndex);
  void finishIndirectRegisterAccess(uint8_t pointerIndex);
  void rememberLastX();
  void clearStackState();
  void startEntry();
  void finishEntry();
  void clearError();
  void clearEntryBuffer();
  bool appendEntryChar(char ch);
  bool insertEntryChar(size_t index, char ch);
  void removeEntryChar(size_t index);
  void refreshEntryFlags();
  bool exponentHasDigits() const;
  size_t exponentMarkerIndex() const;
  bool mantissaIsSimpleZero() const;
  size_t mantissaSignificantDigitCount() const;
  bool syncValueFromEntryBuffer();
  void seedEntryBufferFromCurrentX();
  uint32_t nextRandomUint32();
  void liftStack();
  void dropStack();

  std::array<CalculatorValue, 4> stack_{{0.0, 0.0, 0.0, 0.0}};
  bool entering_ = false;
  bool decimalMode_ = false;
  bool enteringExponent_ = false;
  bool stackLiftEnabled_ = false;
  std::array<char, kEntryBufferSize> entryBuffer_{{}};
  size_t entryLength_ = 0;
  CalculatorValue lastX_ = 0.0;
  std::array<CalculatorValue, kRegisterCount> registers_{{}};
  CalculatorAngleMode angleMode_ = CalculatorAngleMode::Radians;
  CalculatorError error_ = CalculatorError::None;
  uint32_t randomState_ = 1;
};

#endif
