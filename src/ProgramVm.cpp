#include "ProgramVm.h"

#include <cstdio>
#include <cstring>

namespace {

struct LowOpcodeEntry {
  const char *mnemonic;
  bool valid;
};

constexpr LowOpcodeEntry kLowOpcodeTable[0x40] = {
    {"0", true},        // 00
    {"1", true},        // 01
    {"2", true},        // 02
    {"3", true},        // 03
    {"4", true},        // 04
    {"5", true},        // 05
    {"6", true},        // 06
    {"7", true},        // 07
    {"8", true},        // 08
    {"9", true},        // 09
    {".", true},        // 0A
    {"CHS", true},      // 0B
    {"EEX", true},      // 0C
    {"CX", true},       // 0D
    {"ENTER", true},    // 0E
    {"LASTX", true},    // 0F
    {"+", true},        // 10
    {"-", true},        // 11
    {"*", true},        // 12
    {"/", true},        // 13
    {"X<->Y", true},    // 14
    {"10^X", true},     // 15
    {"E^X", true},      // 16
    {"LG", true},       // 17
    {"LN", true},       // 18
    {"ASIN", true},     // 19
    {"ACOS", true},     // 1A
    {"ATAN", true},     // 1B
    {"SIN", true},      // 1C
    {"COS", true},      // 1D
    {"TAN", true},      // 1E
    {"INVALID", false}, // 1F
    {"PI", true},       // 20
    {"SQRT", true},     // 21
    {"X^2", true},      // 22
    {"1/X", true},      // 23
    {"X^Y", true},      // 24
    {"RDOWN", true},    // 25
    {"H.M->H", true},   // 26
    {"INVALID", false}, // 27
    {"INVALID", false}, // 28
    {"INVALID", false}, // 29
    {"H.M.S->H", true}, // 2A
    {"INVALID", false}, // 2B
    {"INVALID", false}, // 2C
    {"INVALID", false}, // 2D
    {"INVALID", false}, // 2E
    {"INVALID", false}, // 2F
    {"H->H.M.S", true}, // 30
    {"ABS", true},      // 31
    {"SIGN", true},     // 32
    {"H->H.M", true},   // 33
    {"INT", true},      // 34
    {"FRAC", true},     // 35
    {"MAX", true},      // 36
    {"AND", true},      // 37
    {"OR", true},       // 38
    {"XOR", true},      // 39
    {"NOT", true},      // 3A
    {"RND", true},      // 3B
    {"INVALID", false}, // 3C
    {"INVALID", false}, // 3D
    {"INVALID", false}, // 3E
    {"INVALID", false}, // 3F
};

void copyLabel(const char *label, char *buffer, size_t bufferSize) {
  if ((buffer == nullptr) || (bufferSize == 0)) {
    return;
  }

  std::snprintf(buffer, bufferSize, "%s", (label != nullptr) ? label : "");
}

bool isDirectControlFlowOpcode(uint8_t opcode) {
  switch (opcode) {
    case 0x51:  // GTO
    case 0x53:  // GSB
    case 0x57:  // JP X<>0
    case 0x58:  // L2
    case 0x59:  // JP X>=0
    case 0x5A:  // L3
    case 0x5B:  // L1
    case 0x5C:  // JP X<0
    case 0x5D:  // L0
    case 0x5E:  // JP X=0
      return true;
    default:
      return false;
  }
}

}  // namespace

char selectorNibbleToChar(uint8_t nibble) {
  if (nibble < 10) {
    return static_cast<char>('0' + nibble);
  }
  if (nibble < 16) {
    return static_cast<char>('A' + (nibble - 10));
  }
  return '?';
}

ProgramOpcodeInfo describeProgramOpcode(uint8_t opcode) {
  ProgramOpcodeInfo info{};
  info.opcode = opcode;

  if (opcode < 0x40) {
    const LowOpcodeEntry &entry = kLowOpcodeTable[opcode];
    info.valid = entry.valid;
    info.mnemonic = entry.mnemonic;
    return info;
  }

  if (opcode <= 0x4F) {
    info.valid = true;
    info.mnemonic = "MX";
    return info;
  }

  if (opcode <= 0x5F) {
    switch (opcode) {
      case 0x50:
        info.valid = true;
        info.mnemonic = "HALT";
        return info;
      case 0x51:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "GTO";
        return info;
      case 0x52:
        info.valid = true;
        info.mnemonic = "RETURN";
        return info;
      case 0x53:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "GSB";
        return info;
      case 0x54:
        info.valid = true;
        info.mnemonic = "NOP";
        return info;
      case 0x57:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "JP X<>0";
        return info;
      case 0x58:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "L2";
        return info;
      case 0x59:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "JP X>=0";
        return info;
      case 0x5A:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "L3";
        return info;
      case 0x5B:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "L1";
        return info;
      case 0x5C:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "JP X<0";
        return info;
      case 0x5D:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "L0";
        return info;
      case 0x5E:
        info.valid = true;
        info.width = 2;
        info.mnemonic = "JP X=0";
        return info;
      default:
        info.valid = false;
        info.mnemonic = "INVALID";
        return info;
    }
  }

  if (opcode <= 0x6F) {
    info.valid = true;
    info.mnemonic = "XM";
    return info;
  }

  if (opcode <= 0x7F) {
    info.valid = true;
    info.mnemonic = "JPI X<>0";
    return info;
  }

  if (opcode <= 0x8F) {
    info.valid = true;
    info.mnemonic = "JPI";
    return info;
  }

  if (opcode <= 0x9F) {
    info.valid = true;
    info.mnemonic = "JPI X>=0";
    return info;
  }

  if (opcode <= 0xAF) {
    info.valid = true;
    info.mnemonic = "GSBI";
    return info;
  }

  if (opcode <= 0xBF) {
    info.valid = true;
    info.mnemonic = "XMI";
    return info;
  }

  if (opcode <= 0xCF) {
    info.valid = true;
    info.mnemonic = "JPI X<0";
    return info;
  }

  if (opcode <= 0xDF) {
    info.valid = true;
    info.mnemonic = "MXI";
    return info;
  }

  if (opcode <= 0xEF) {
    info.valid = true;
    info.mnemonic = "JPI X=0";
    return info;
  }

  info.extension = true;
  info.valid = false;
  info.mnemonic = "EXT";
  return info;
}

ProgramVm::ProgramVm() {
  reset();
}

void ProgramVm::reset() {
  program_.fill(0);
  programLength_ = 0;
}

bool ProgramVm::appendByte(uint8_t byte) {
  if (programLength_ >= kProgramCapacity) {
    return false;
  }

  program_[programLength_] = byte;
  ++programLength_;
  return true;
}

bool ProgramVm::loadProgram(const uint8_t *bytes, uint16_t length) {
  if (length > kProgramCapacity) {
    return false;
  }

  program_.fill(0);
  if ((bytes != nullptr) && (length > 0)) {
    std::memcpy(program_.data(), bytes, length);
  }

  programLength_ = length;
  return true;
}

bool ProgramVm::setProgramByte(uint8_t address, uint8_t byte) {
  if (address >= kProgramCapacity) {
    return false;
  }

  program_[address] = byte;
  if (address >= programLength_) {
    programLength_ = static_cast<uint16_t>(address + 1);
  }

  return true;
}

bool ProgramVm::setProgramLength(uint16_t length) {
  if (length > kProgramCapacity) {
    return false;
  }

  if (length > programLength_) {
    std::memset(program_.data() + programLength_, 0, length - programLength_);
  }

  programLength_ = length;
  return true;
}

uint8_t ProgramVm::programByte(uint16_t address) const {
  if (address >= programLength_) {
    return 0;
  }

  return program_[address];
}

bool ProgramVm::isStepBoundary(uint8_t address) const {
  if (address > programLength_) {
    return false;
  }

  uint16_t cursor = 0;
  while (cursor < programLength_) {
    if (cursor == address) {
      return true;
    }

    const DecodedStep step = decodeAt(static_cast<uint8_t>(cursor));
    uint16_t width = (step.width > 0) ? step.width : 1;
    if (cursor + width > programLength_) {
      cursor = programLength_;
    } else {
      cursor += width;
    }
  }

  return address == programLength_;
}

uint8_t ProgramVm::nextStepAddress(uint8_t address) const {
  if (address >= programLength_) {
    return static_cast<uint8_t>(programLength_);
  }

  if (!isStepBoundary(address)) {
    return address;
  }

  const DecodedStep step = decodeAt(address);
  uint16_t next = static_cast<uint16_t>(address) + static_cast<uint16_t>((step.width > 0) ? step.width : 1);
  if (next > programLength_) {
    next = programLength_;
  }

  return static_cast<uint8_t>(next);
}

bool ProgramVm::previousStepAddress(uint8_t address, uint8_t &previousAddress) const {
  if ((address == 0) || !isStepBoundary(address)) {
    return false;
  }

  uint8_t cursor = 0;
  uint8_t lastBoundary = 0;
  bool foundBoundary = false;

  while (cursor < address) {
    lastBoundary = cursor;
    foundBoundary = true;

    const uint8_t next = nextStepAddress(cursor);
    if (next <= cursor) {
      break;
    }
    cursor = next;
  }

  if (!foundBoundary) {
    return false;
  }

  previousAddress = lastBoundary;
  return true;
}

bool ProgramVm::insertStepAt(uint8_t address, uint8_t opcode, uint8_t operand) {
  if (!isStepBoundary(address)) {
    return false;
  }

  const ProgramOpcodeInfo newStepInfo = describeProgramOpcode(opcode);
  if (!newStepInfo.valid || newStepInfo.extension) {
    return false;
  }

  const uint8_t newWidth = newStepInfo.width;
  const int32_t newLength = static_cast<int32_t>(programLength_) + static_cast<int32_t>(newWidth);
  if ((newLength < 0) || (newLength > kProgramCapacity)) {
    return false;
  }

  const uint16_t trailingStart = static_cast<uint16_t>(address);
  const uint16_t trailingCount = static_cast<uint16_t>(programLength_ - trailingStart);
  std::memmove(program_.data() + trailingStart + newWidth, program_.data() + trailingStart, trailingCount);

  program_[address] = opcode;
  if (newWidth == 2) {
    program_[static_cast<uint16_t>(address) + 1u] = operand;
  }

  programLength_ = static_cast<uint16_t>(newLength);
  return true;
}

void ProgramVm::relocateDirectTargetsAfterInsert(uint8_t insertionAddress, uint8_t insertedWidth) {
  if ((insertedWidth == 0) || (programLength_ == 0)) {
    return;
  }

  uint16_t cursor = 0;
  while (cursor < programLength_) {
    const DecodedStep step = decodeAt(static_cast<uint8_t>(cursor));
    const uint8_t width = (step.width > 0) ? step.width : 1;

    if ((cursor != insertionAddress) && step.complete && isDirectControlFlowOpcode(step.opcode) && (width == 2)) {
      const uint16_t operandIndex = cursor + 1u;
      const uint8_t target = program_[operandIndex];
      if (target >= insertionAddress) {
        const uint16_t shifted = static_cast<uint16_t>(target) + insertedWidth;
        if (shifted <= 0xFFu) {
          program_[operandIndex] = static_cast<uint8_t>(shifted);
        }
      }
    }

    const uint16_t next = cursor + width;
    if ((next <= cursor) || (next > programLength_)) {
      break;
    }
    cursor = next;
  }
}

bool ProgramVm::replaceStepAt(uint8_t address, uint8_t opcode, uint8_t operand) {
  if (!isStepBoundary(address)) {
    return false;
  }

  const ProgramOpcodeInfo newStepInfo = describeProgramOpcode(opcode);
  if (!newStepInfo.valid || newStepInfo.extension) {
    return false;
  }

  uint8_t oldWidth = 0;
  if (address < programLength_) {
    const DecodedStep oldStep = decodeAt(address);
    oldWidth = (oldStep.width > 0) ? oldStep.width : 1;
    if (static_cast<uint16_t>(address) + oldWidth > programLength_) {
      oldWidth = static_cast<uint8_t>(programLength_ - address);
    }
  }

  const uint8_t newWidth = newStepInfo.width;
  const int32_t newLength =
      static_cast<int32_t>(programLength_) - static_cast<int32_t>(oldWidth) + static_cast<int32_t>(newWidth);
  if ((newLength < 0) || (newLength > kProgramCapacity)) {
    return false;
  }

  const uint16_t trailingStart = static_cast<uint16_t>(address) + static_cast<uint16_t>(oldWidth);
  const uint16_t trailingCount = static_cast<uint16_t>(programLength_ - trailingStart);
  const uint16_t originalLength = programLength_;

  if (newWidth > oldWidth) {
    const uint16_t shift = static_cast<uint16_t>(newWidth - oldWidth);
    std::memmove(program_.data() + trailingStart + shift, program_.data() + trailingStart, trailingCount);
  } else if (newWidth < oldWidth) {
    const uint16_t shift = static_cast<uint16_t>(oldWidth - newWidth);
    std::memmove(program_.data() + trailingStart - shift, program_.data() + trailingStart, trailingCount);
  }

  program_[address] = opcode;
  if (newWidth == 2) {
    program_[static_cast<uint16_t>(address) + 1u] = operand;
  }

  programLength_ = static_cast<uint16_t>(newLength);
  if (programLength_ < originalLength) {
    std::memset(program_.data() + programLength_, 0, originalLength - programLength_);
  }

  return true;
}

ProgramVm::DecodedStep ProgramVm::decodeAt(uint8_t address) const {
  DecodedStep step{};
  step.address = address;

  if (address >= programLength_) {
    return step;
  }

  step.inRange = true;
  step.opcode = program_[address];

  const ProgramOpcodeInfo info = describeProgramOpcode(step.opcode);
  step.width = info.width;
  step.valid = info.valid;
  step.extension = info.extension;

  if (step.width == 2) {
    if ((address + 1) < programLength_) {
      step.complete = true;
      step.operand = program_[address + 1];
    } else {
      step.complete = false;
    }
  } else {
    step.complete = true;
  }

  return step;
}

bool ProgramVm::formatStep(const DecodedStep &step, char *buffer, size_t bufferSize) const {
  if ((buffer == nullptr) || (bufferSize == 0)) {
    return false;
  }

  if (!step.inRange) {
    copyLabel("", buffer, bufferSize);
    return false;
  }

  const ProgramOpcodeInfo info = describeProgramOpcode(step.opcode);

  if (info.extension) {
    std::snprintf(buffer, bufferSize, "EXT %02X", step.opcode);
    return true;
  }

  if (!info.valid) {
    copyLabel("INVALID", buffer, bufferSize);
    return true;
  }

  if (((step.opcode >= 0x40) && (step.opcode <= 0x4F)) ||
      ((step.opcode >= 0x60) && (step.opcode <= 0x6F)) ||
      ((step.opcode >= 0x70) && (step.opcode <= 0x7F)) ||
      ((step.opcode >= 0x80) && (step.opcode <= 0x8F)) ||
      ((step.opcode >= 0x90) && (step.opcode <= 0x9F)) ||
      ((step.opcode >= 0xA0) && (step.opcode <= 0xAF)) ||
      ((step.opcode >= 0xB0) && (step.opcode <= 0xBF)) ||
      ((step.opcode >= 0xC0) && (step.opcode <= 0xCF)) ||
      ((step.opcode >= 0xD0) && (step.opcode <= 0xDF)) ||
      ((step.opcode >= 0xE0) && (step.opcode <= 0xEF))) {
    const char selector = selectorNibbleToChar(step.opcode & 0x0F);
    std::snprintf(buffer, bufferSize, "%s %c", info.mnemonic, selector);
    return true;
  }

  if (info.width == 2) {
    if (step.complete) {
      std::snprintf(buffer, bufferSize, "%s %02X", info.mnemonic, step.operand);
    } else {
      std::snprintf(buffer, bufferSize, "%s __", info.mnemonic);
    }
    return true;
  }

  copyLabel(info.mnemonic, buffer, bufferSize);
  return true;
}

bool ProgramVm::formatListingLine(uint8_t address, char *buffer, size_t bufferSize) const {
  if ((buffer == nullptr) || (bufferSize == 0)) {
    return false;
  }

  const DecodedStep step = decodeAt(address);
  if (!step.inRange) {
    copyLabel("", buffer, bufferSize);
    return false;
  }

  char decodedLabel[32];
  (void)formatStep(step, decodedLabel, sizeof(decodedLabel));

  if (step.width == 2) {
    if (step.complete) {
      std::snprintf(buffer, bufferSize, "%02X: %02X %02X   %s", address, step.opcode, step.operand, decodedLabel);
    } else {
      std::snprintf(buffer, bufferSize, "%02X: %02X __   %s", address, step.opcode, decodedLabel);
    }
  } else {
    std::snprintf(buffer, bufferSize, "%02X: %02X      %s", address, step.opcode, decodedLabel);
  }

  return true;
}
