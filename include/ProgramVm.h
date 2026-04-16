#ifndef PROGRAM_VM_H
#define PROGRAM_VM_H

#include <array>
#include <cstddef>
#include <cstdint>

struct ProgramOpcodeInfo {
  uint8_t opcode = 0;
  uint8_t width = 1;
  bool valid = false;
  bool extension = false;
  const char *mnemonic = "INVALID";
};

ProgramOpcodeInfo describeProgramOpcode(uint8_t opcode);
char selectorNibbleToChar(uint8_t nibble);

class ProgramVm {
public:
  static constexpr uint16_t kProgramCapacity = 256;

  struct DecodedStep {
    uint8_t address = 0;
    uint8_t opcode = 0;
    uint8_t width = 1;
    bool inRange = false;
    bool valid = false;
    bool extension = false;
    bool complete = false;
    uint8_t operand = 0;
  };

  ProgramVm();

  void reset();
  bool appendByte(uint8_t byte);
  bool loadProgram(const uint8_t *bytes, uint16_t length);
  bool setProgramByte(uint8_t address, uint8_t byte);
  bool setProgramLength(uint16_t length);

  uint16_t programLength() const { return programLength_; }
  uint8_t programByte(uint16_t address) const;

  bool isStepBoundary(uint8_t address) const;
  uint8_t nextStepAddress(uint8_t address) const;
  bool previousStepAddress(uint8_t address, uint8_t &previousAddress) const;
  bool insertStepAt(uint8_t address, uint8_t opcode, uint8_t operand = 0);
  void relocateDirectTargetsAfterInsert(uint8_t insertionAddress, uint8_t insertedWidth);
  bool replaceStepAt(uint8_t address, uint8_t opcode, uint8_t operand = 0);

  DecodedStep decodeAt(uint8_t address) const;
  bool formatStep(const DecodedStep &step, char *buffer, size_t bufferSize) const;
  bool formatListingLine(uint8_t address, char *buffer, size_t bufferSize) const;

private:
  std::array<uint8_t, kProgramCapacity> program_{};
  uint16_t programLength_ = 0;
};

#endif
