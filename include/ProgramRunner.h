#ifndef PROGRAM_RUNNER_H
#define PROGRAM_RUNNER_H

#include <cstdint>

#include "ProgramVm.h"
#include "RpnCalculator.h"

enum class ProgramRunnerError : uint8_t {
  None,
  InvalidAddress,
  InvalidStep,
  UnsupportedControl,
  CalculatorError,
};

const char *programRunnerErrorShortName(ProgramRunnerError error);
const char *programRunnerErrorText(ProgramRunnerError error);

class ProgramRunner {
public:
  ProgramRunner();

  void reset();
  bool start(const ProgramVm &vm);
  void stop();
  void resetRunAddress();
  bool step(const ProgramVm &vm, RpnCalculator &calculator);

  uint8_t runAddress() const { return runAddress_; }
  bool isRunning() const { return running_; }
  ProgramRunnerError error() const { return error_; }
  bool hasError() const { return error_ != ProgramRunnerError::None; }
  void clearError() { error_ = ProgramRunnerError::None; }

private:
  void fail(ProgramRunnerError error);

  uint8_t runAddress_ = 0;
  bool running_ = false;
  ProgramRunnerError error_ = ProgramRunnerError::None;
};

#endif
