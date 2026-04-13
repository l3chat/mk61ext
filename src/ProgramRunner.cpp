#include "ProgramRunner.h"

#include <cmath>

namespace {

constexpr CalculatorValue kProgramConditionZeroEpsilon = 1e-12;

CalculatorAction calculatorActionForOpcode(uint8_t opcode) {
  switch (opcode) {
    case 0x00:
      return CalculatorAction::Digit0;
    case 0x01:
      return CalculatorAction::Digit1;
    case 0x02:
      return CalculatorAction::Digit2;
    case 0x03:
      return CalculatorAction::Digit3;
    case 0x04:
      return CalculatorAction::Digit4;
    case 0x05:
      return CalculatorAction::Digit5;
    case 0x06:
      return CalculatorAction::Digit6;
    case 0x07:
      return CalculatorAction::Digit7;
    case 0x08:
      return CalculatorAction::Digit8;
    case 0x09:
      return CalculatorAction::Digit9;
    case 0x0A:
      return CalculatorAction::DecimalPoint;
    case 0x0B:
      return CalculatorAction::ChangeSign;
    case 0x0C:
      return CalculatorAction::EnterExponent;
    case 0x0D:
      return CalculatorAction::ClearX;
    case 0x0E:
      return CalculatorAction::Enter;
    case 0x0F:
      return CalculatorAction::LastX;
    case 0x10:
      return CalculatorAction::Add;
    case 0x11:
      return CalculatorAction::Subtract;
    case 0x12:
      return CalculatorAction::Multiply;
    case 0x13:
      return CalculatorAction::Divide;
    case 0x14:
      return CalculatorAction::SwapXY;
    case 0x15:
      return CalculatorAction::Exp10;
    case 0x16:
      return CalculatorAction::ExpE;
    case 0x17:
      return CalculatorAction::Log10;
    case 0x18:
      return CalculatorAction::NaturalLog;
    case 0x19:
      return CalculatorAction::Asin;
    case 0x1A:
      return CalculatorAction::Acos;
    case 0x1B:
      return CalculatorAction::Atan;
    case 0x1C:
      return CalculatorAction::Sin;
    case 0x1D:
      return CalculatorAction::Cos;
    case 0x1E:
      return CalculatorAction::Tan;
    case 0x20:
      return CalculatorAction::Pi;
    case 0x21:
      return CalculatorAction::SquareRoot;
    case 0x22:
      return CalculatorAction::Square;
    case 0x23:
      return CalculatorAction::Reciprocal;
    case 0x24:
      return CalculatorAction::PowerXY;
    case 0x25:
      return CalculatorAction::RollDown;
    case 0x26:
      return CalculatorAction::HourMinuteToHour;
    case 0x2A:
      return CalculatorAction::HourMinuteSecondToHour;
    case 0x30:
      return CalculatorAction::HourToHourMinuteSecond;
    case 0x31:
      return CalculatorAction::AbsoluteValue;
    case 0x32:
      return CalculatorAction::Sign;
    case 0x33:
      return CalculatorAction::HourToHourMinute;
    case 0x34:
      return CalculatorAction::IntegerPart;
    case 0x35:
      return CalculatorAction::FractionalPart;
    case 0x36:
      return CalculatorAction::MaxXY;
    case 0x37:
      return CalculatorAction::BitwiseAnd;
    case 0x38:
      return CalculatorAction::BitwiseOr;
    case 0x39:
      return CalculatorAction::BitwiseXor;
    case 0x3A:
      return CalculatorAction::BitwiseNot;
    case 0x3B:
      return CalculatorAction::RandomValue;
    default:
      return CalculatorAction::None;
  }
}

CalculatorValue normalizedConditionValue(CalculatorValue value) {
  return (std::fabs(value) < kProgramConditionZeroEpsilon) ? 0.0 : value;
}

bool shouldTakeDirectConditional(uint8_t opcode, CalculatorValue xValue) {
  const CalculatorValue normalizedX = normalizedConditionValue(xValue);

  switch (opcode) {
    case 0x57:
      return normalizedX != 0.0;
    case 0x59:
      return normalizedX >= 0.0;
    case 0x5C:
      return normalizedX < 0.0;
    case 0x5E:
      return normalizedX == 0.0;
    default:
      return false;
  }
}

bool dsnzRegisterIndexForOpcode(uint8_t opcode, uint8_t &registerIndex) {
  switch (opcode) {
    case 0x5D:
      registerIndex = 0;
      return true;
    case 0x5B:
      registerIndex = 1;
      return true;
    case 0x58:
      registerIndex = 2;
      return true;
    case 0x5A:
      registerIndex = 3;
      return true;
    default:
      return false;
  }
}

bool coerceToIndirectProgramAddress(CalculatorValue value, uint8_t &address) {
  if (!std::isfinite(value)) {
    return false;
  }

  CalculatorValue wrapped =
      std::fmod(std::trunc(value), static_cast<CalculatorValue>(ProgramVm::kProgramCapacity));
  if (wrapped < 0.0) {
    wrapped += static_cast<CalculatorValue>(ProgramVm::kProgramCapacity);
  }

  address = static_cast<uint8_t>(wrapped);
  return true;
}

bool readIndirectTargetAddress(RpnCalculator &calculator, uint8_t selector, uint8_t &targetAddress) {
  CalculatorValue targetValue = 0.0;
  if (!calculator.readRegister(selector, targetValue)) {
    return false;
  }

  return coerceToIndirectProgramAddress(targetValue, targetAddress);
}

bool shouldTakeIndirectConditional(uint8_t opcode, CalculatorValue xValue) {
  if ((opcode >= 0x70) && (opcode <= 0x7F)) {
    return shouldTakeDirectConditional(0x57, xValue);
  }

  if ((opcode >= 0x90) && (opcode <= 0x9F)) {
    return shouldTakeDirectConditional(0x59, xValue);
  }

  if ((opcode >= 0xC0) && (opcode <= 0xCF)) {
    return shouldTakeDirectConditional(0x5C, xValue);
  }

  if ((opcode >= 0xE0) && (opcode <= 0xEF)) {
    return shouldTakeDirectConditional(0x5E, xValue);
  }

  return false;
}

}  // namespace

const char *programRunnerErrorShortName(ProgramRunnerError error) {
  switch (error) {
    case ProgramRunnerError::None:
      return "";
    case ProgramRunnerError::InvalidAddress:
      return "PC";
    case ProgramRunnerError::InvalidTarget:
      return "TGT";
    case ProgramRunnerError::InvalidStep:
      return "OPC";
    case ProgramRunnerError::UnsupportedControl:
      return "CTL";
    case ProgramRunnerError::CallStackOverflow:
      return "STK";
    case ProgramRunnerError::CalculatorError:
      return "CALC";
  }

  return "VM";
}

const char *programRunnerErrorText(ProgramRunnerError error) {
  switch (error) {
    case ProgramRunnerError::None:
      return "";
    case ProgramRunnerError::InvalidAddress:
      return "Execution cursor is not on a valid step boundary.";
    case ProgramRunnerError::InvalidTarget:
      return "Program control flow targeted an invalid step address.";
    case ProgramRunnerError::InvalidStep:
      return "Stored program step is invalid or incomplete.";
    case ProgramRunnerError::UnsupportedControl:
      return "This control-flow program step is not implemented yet.";
    case ProgramRunnerError::CallStackOverflow:
      return "Program call stack overflowed during execution.";
    case ProgramRunnerError::CalculatorError:
      return "Calculator execution failed while running the program.";
  }

  return "Unknown program runner error.";
}

ProgramRunner::ProgramRunner() {
  reset();
}

void ProgramRunner::reset() {
  callDepth_ = 0;
  runAddress_ = 0;
  running_ = false;
  pausedExecution_ = false;
  error_ = ProgramRunnerError::None;
}

bool ProgramRunner::start(const ProgramVm &vm) {
  if ((runAddress_ > vm.programLength()) || !vm.isStepBoundary(runAddress_)) {
    fail(ProgramRunnerError::InvalidAddress);
    return false;
  }

  for (uint8_t index = 0; index < callDepth_; ++index) {
    if (!isValidTargetAddress(vm, callStack_[index])) {
      fail(ProgramRunnerError::InvalidTarget);
      return false;
    }
  }

  error_ = ProgramRunnerError::None;
  running_ = true;
  pausedExecution_ = false;
  return true;
}

bool ProgramRunner::start(const ProgramVm &vm, RpnCalculator &calculator) {
  if (!pausedExecution_) {
    calculator.commitEntry();
    if (calculator.hasError()) {
      fail(ProgramRunnerError::CalculatorError);
      return false;
    }
  }

  return start(vm);
}

void ProgramRunner::stop() {
  if (running_) {
    running_ = false;
    pausedExecution_ = true;
  }
}

void ProgramRunner::resetRunAddress() {
  callDepth_ = 0;
  runAddress_ = 0;
  running_ = false;
  pausedExecution_ = false;
  error_ = ProgramRunnerError::None;
}

void ProgramRunner::clearPausedExecution() {
  pausedExecution_ = false;
}

bool ProgramRunner::singleStep(const ProgramVm &vm, RpnCalculator &calculator) {
  if (!start(vm, calculator)) {
    return false;
  }

  const bool result = step(vm, calculator);
  if (running_) {
    running_ = false;
    pausedExecution_ = true;
  }

  return result;
}

bool ProgramRunner::step(const ProgramVm &vm, RpnCalculator &calculator) {
  if (!running_) {
    return false;
  }

  const uint16_t programLength = vm.programLength();
  if (runAddress_ == programLength) {
    running_ = false;
    pausedExecution_ = false;
    callDepth_ = 0;
    return true;
  }

  if ((runAddress_ > programLength) || !vm.isStepBoundary(runAddress_)) {
    fail(ProgramRunnerError::InvalidAddress);
    return false;
  }

  const ProgramVm::DecodedStep step = vm.decodeAt(runAddress_);
  if (!step.inRange || !step.valid || step.extension || !step.complete) {
    fail(ProgramRunnerError::InvalidStep);
    return false;
  }

  const uint8_t nextAddress = vm.nextStepAddress(runAddress_);

  if (step.opcode < 0x40) {
    const CalculatorAction action = calculatorActionForOpcode(step.opcode);
    if (action == CalculatorAction::None) {
      fail(ProgramRunnerError::InvalidStep);
      return false;
    }

    if (!calculator.apply(action)) {
      fail(ProgramRunnerError::CalculatorError);
      return false;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0x4F) {
    if (!calculator.recallRegister(step.opcode & 0x0F)) {
      fail(ProgramRunnerError::CalculatorError);
      return false;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0x5F) {
    switch (step.opcode) {
      case 0x50:
        runAddress_ = nextAddress;
        running_ = false;
        pausedExecution_ = false;
        return true;
      case 0x52: {
        if (callDepth_ == 0) {
          runAddress_ = nextAddress;
          running_ = false;
          pausedExecution_ = false;
          return true;
        }

        uint8_t returnAddress = 0;
        if (!popReturnAddress(returnAddress) || !isValidTargetAddress(vm, returnAddress)) {
          fail(ProgramRunnerError::InvalidTarget);
          return false;
        }

        runAddress_ = returnAddress;
        return true;
      }
      case 0x51:
        if (!isValidTargetAddress(vm, step.operand)) {
          fail(ProgramRunnerError::InvalidTarget);
          return false;
        }

        runAddress_ = step.operand;
        return true;
      case 0x53:
        if (!isValidTargetAddress(vm, step.operand)) {
          fail(ProgramRunnerError::InvalidTarget);
          return false;
        }

        if (!pushReturnAddress(nextAddress)) {
          fail(ProgramRunnerError::CallStackOverflow);
          return false;
        }

        runAddress_ = step.operand;
        return true;
      case 0x54:
        runAddress_ = nextAddress;
        return true;
      case 0x57:
      case 0x59:
      case 0x5C:
      case 0x5E:
        if (shouldTakeDirectConditional(step.opcode, calculator.stack().x)) {
          if (!isValidTargetAddress(vm, step.operand)) {
            fail(ProgramRunnerError::InvalidTarget);
            return false;
          }

          runAddress_ = step.operand;
          return true;
        }

        runAddress_ = nextAddress;
        return true;
      case 0x58:
      case 0x5A:
      case 0x5B:
      case 0x5D: {
        uint8_t registerIndex = 0;
        if (!dsnzRegisterIndexForOpcode(step.opcode, registerIndex)) {
          fail(ProgramRunnerError::InvalidStep);
          return false;
        }

        CalculatorValue registerValue = 0.0;
        if (!calculator.readRegister(registerIndex, registerValue)) {
          fail(ProgramRunnerError::CalculatorError);
          return false;
        }

        registerValue -= 1.0;
        if (!calculator.writeRegister(registerIndex, registerValue)) {
          fail(ProgramRunnerError::CalculatorError);
          return false;
        }

        if (normalizedConditionValue(registerValue) != 0.0) {
          if (!isValidTargetAddress(vm, step.operand)) {
            fail(ProgramRunnerError::InvalidTarget);
            return false;
          }

          runAddress_ = step.operand;
          return true;
        }

        runAddress_ = nextAddress;
        return true;
      }
      default:
        fail(ProgramRunnerError::UnsupportedControl);
        return false;
    }
  }

  if (step.opcode <= 0x6F) {
    if (!calculator.storeRegister(step.opcode & 0x0F)) {
      fail(ProgramRunnerError::CalculatorError);
      return false;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0xAF) {
    const uint8_t selector = step.opcode & 0x0F;

    if ((step.opcode >= 0x80) && (step.opcode <= 0x8F)) {
      uint8_t targetAddress = 0;
      if (!readIndirectTargetAddress(calculator, selector, targetAddress)) {
        fail(ProgramRunnerError::CalculatorError);
        return false;
      }

      if (!isValidTargetAddress(vm, targetAddress)) {
        fail(ProgramRunnerError::InvalidTarget);
        return false;
      }

      runAddress_ = targetAddress;
      return true;
    }

    if ((step.opcode >= 0xA0) && (step.opcode <= 0xAF)) {
      uint8_t targetAddress = 0;
      if (!readIndirectTargetAddress(calculator, selector, targetAddress)) {
        fail(ProgramRunnerError::CalculatorError);
        return false;
      }

      if (!isValidTargetAddress(vm, targetAddress)) {
        fail(ProgramRunnerError::InvalidTarget);
        return false;
      }

      if (!pushReturnAddress(nextAddress)) {
        fail(ProgramRunnerError::CallStackOverflow);
        return false;
      }

      runAddress_ = targetAddress;
      return true;
    }

    if (shouldTakeIndirectConditional(step.opcode, calculator.stack().x)) {
      uint8_t targetAddress = 0;
      if (!readIndirectTargetAddress(calculator, selector, targetAddress)) {
        fail(ProgramRunnerError::CalculatorError);
        return false;
      }

      if (!isValidTargetAddress(vm, targetAddress)) {
        fail(ProgramRunnerError::InvalidTarget);
        return false;
      }

      runAddress_ = targetAddress;
      return true;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0xBF) {
    if (!calculator.storeIndirectRegister(step.opcode & 0x0F)) {
      fail(ProgramRunnerError::CalculatorError);
      return false;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0xCF) {
    if (shouldTakeIndirectConditional(step.opcode, calculator.stack().x)) {
      uint8_t targetAddress = 0;
      if (!readIndirectTargetAddress(calculator, step.opcode & 0x0F, targetAddress)) {
        fail(ProgramRunnerError::CalculatorError);
        return false;
      }

      if (!isValidTargetAddress(vm, targetAddress)) {
        fail(ProgramRunnerError::InvalidTarget);
        return false;
      }

      runAddress_ = targetAddress;
      return true;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0xDF) {
    if (!calculator.recallIndirectRegister(step.opcode & 0x0F)) {
      fail(ProgramRunnerError::CalculatorError);
      return false;
    }

    runAddress_ = nextAddress;
    return true;
  }

  if (step.opcode <= 0xEF) {
    if (shouldTakeIndirectConditional(step.opcode, calculator.stack().x)) {
      uint8_t targetAddress = 0;
      if (!readIndirectTargetAddress(calculator, step.opcode & 0x0F, targetAddress)) {
        fail(ProgramRunnerError::CalculatorError);
        return false;
      }

      if (!isValidTargetAddress(vm, targetAddress)) {
        fail(ProgramRunnerError::InvalidTarget);
        return false;
      }

      runAddress_ = targetAddress;
      return true;
    }

    runAddress_ = nextAddress;
    return true;
  }

  fail(ProgramRunnerError::UnsupportedControl);
  return false;
}

void ProgramRunner::fail(ProgramRunnerError error) {
  running_ = false;
  pausedExecution_ = false;
  error_ = error;
}

bool ProgramRunner::isValidTargetAddress(const ProgramVm &vm, uint8_t address) const {
  return (address <= vm.programLength()) && vm.isStepBoundary(address);
}

bool ProgramRunner::pushReturnAddress(uint8_t address) {
  if (callDepth_ >= kCallStackCapacity) {
    return false;
  }

  callStack_[callDepth_] = address;
  ++callDepth_;
  return true;
}

bool ProgramRunner::popReturnAddress(uint8_t &address) {
  if (callDepth_ == 0) {
    return false;
  }

  --callDepth_;
  address = callStack_[callDepth_];
  return true;
}
