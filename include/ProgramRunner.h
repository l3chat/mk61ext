#ifndef PROGRAM_RUNNER_H
#define PROGRAM_RUNNER_H

#include <array>
#include <cstdint>

#include "ProgramVm.h"
#include "RpnCalculator.h"

enum class ProgramRunnerError : uint8_t {
  None,
  InvalidAddress,
  InvalidTarget,
  InvalidStep,
  UnsupportedControl,
  CallStackOverflow,
  CalculatorError,
};

const char *programRunnerErrorShortName(ProgramRunnerError error);
const char *programRunnerErrorText(ProgramRunnerError error);

class ProgramRunner {
public:
  ProgramRunner();

  void reset();
  bool start(const ProgramVm &vm);
  bool start(const ProgramVm &vm, RpnCalculator &calculator);
  void stop();
  void resetRunAddress();
  bool step(const ProgramVm &vm, RpnCalculator &calculator);

  uint8_t runAddress() const { return runAddress_; }
  bool isRunning() const { return running_; }
  ProgramRunnerError error() const { return error_; }
  bool hasError() const { return error_ != ProgramRunnerError::None; }
  void clearError() { error_ = ProgramRunnerError::None; }

private:
  static constexpr uint8_t kCallStackCapacity = 16;

  bool isValidTargetAddress(const ProgramVm &vm, uint8_t address) const;
  bool pushReturnAddress(uint8_t address);
  bool popReturnAddress(uint8_t &address);
  void fail(ProgramRunnerError error);

  std::array<uint8_t, kCallStackCapacity> callStack_{};
  uint8_t callDepth_ = 0;
  uint8_t runAddress_ = 0;
  bool running_ = false;
  ProgramRunnerError error_ = ProgramRunnerError::None;
};

#endif
