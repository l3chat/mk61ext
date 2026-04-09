#ifndef PROGRAM_RECORDER_H
#define PROGRAM_RECORDER_H

#include <cstddef>
#include <cstdint>

#include "ProgramVm.h"

enum class ProgramRecorderError : uint8_t {
  None,
  InvalidKey,
  InvalidOperand,
  CommitFailed,
};

class ProgramRecorder {
public:
  ProgramRecorder();

  void reset();
  bool handleKey(ProgramVm &vm, uint8_t &editAddress, char keyPressed);

  bool hasPendingInput() const;
  bool hasPendingOperand() const;
  const char *activePrefixName() const;
  bool formatPendingPreview(char *bytesBuffer,
                            size_t bytesBufferSize,
                            char *decodedBuffer,
                            size_t decodedBufferSize) const;
  ProgramRecorderError error() const { return error_; }
  void clearError() { error_ = ProgramRecorderError::None; }

private:
  enum class PrefixState : uint8_t {
    None,
    F,
    K,
  };

  enum class PendingState : uint8_t {
    None,
    RegisterOperand,
    AddressHighNibble,
    AddressLowNibble,
  };

  struct CommandSpec {
    enum class Kind : uint8_t {
      None,
      Immediate,
      RegisterOperand,
      AddressOperand,
      MoveNext,
      MovePrevious,
      PrefixF,
      PrefixK,
    };

    Kind kind = Kind::None;
    uint8_t opcode = 0;
  };

  bool commitOpcode(ProgramVm &vm, uint8_t &editAddress, uint8_t opcode, bool hasOperand, uint8_t operand);
  bool decodeRegisterDesignator(char keyPressed, uint8_t &index) const;
  bool decodeHexNibble(char keyPressed, uint8_t &nibble) const;
  CommandSpec primaryCommandForKey(char keyPressed) const;
  CommandSpec shiftedCommandForKey(PrefixState prefix, char keyPressed) const;
  void clearPendingState();
  void setError(ProgramRecorderError error) { error_ = error; }

  PrefixState prefix_ = PrefixState::None;
  PendingState pending_ = PendingState::None;
  uint8_t pendingOpcode_ = 0;
  uint8_t pendingHighNibble_ = 0;
  ProgramRecorderError error_ = ProgramRecorderError::None;
};

#endif
