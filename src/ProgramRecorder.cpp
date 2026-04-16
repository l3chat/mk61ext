#include "ProgramRecorder.h"

#include <cctype>
#include <cstdio>

namespace {

char normalizeKey(char keyPressed) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(keyPressed)));
}

}  // namespace

ProgramRecorder::ProgramRecorder() {
  reset();
}

void ProgramRecorder::reset() {
  prefix_ = PrefixState::None;
  clearPendingState();
  insertModeEnabled_ = false;
  error_ = ProgramRecorderError::None;
}

bool ProgramRecorder::hasPendingInput() const {
  return (prefix_ != PrefixState::None) || (pending_ != PendingState::None);
}

bool ProgramRecorder::hasPendingOperand() const {
  return pending_ != PendingState::None;
}

const char *ProgramRecorder::activePrefixName() const {
  switch (prefix_) {
    case PrefixState::F:
      return "F";
    case PrefixState::K:
      return "K";
    case PrefixState::None:
      return "";
  }

  return "";
}

bool ProgramRecorder::formatPendingPreview(char *bytesBuffer,
                                           size_t bytesBufferSize,
                                           char *decodedBuffer,
                                           size_t decodedBufferSize) const {
  if ((bytesBuffer == nullptr) || (bytesBufferSize == 0) || (decodedBuffer == nullptr) ||
      (decodedBufferSize == 0)) {
    return false;
  }

  bytesBuffer[0] = '\0';
  decodedBuffer[0] = '\0';

  if (pending_ == PendingState::None) {
    return false;
  }

  const ProgramOpcodeInfo info = describeProgramOpcode(pendingOpcode_);
  const char *mnemonic = (info.valid && !info.extension) ? info.mnemonic : "OP";

  switch (pending_) {
    case PendingState::RegisterOperand:
      std::snprintf(bytesBuffer, bytesBufferSize, "%X_", pendingOpcode_ >> 4);
      std::snprintf(decodedBuffer, decodedBufferSize, "%s _", mnemonic);
      return true;
    case PendingState::AddressHighNibble:
      std::snprintf(bytesBuffer, bytesBufferSize, "%02X __", pendingOpcode_);
      std::snprintf(decodedBuffer, decodedBufferSize, "%s __", mnemonic);
      return true;
    case PendingState::AddressLowNibble:
      std::snprintf(bytesBuffer, bytesBufferSize, "%02X %X_", pendingOpcode_, pendingHighNibble_);
      std::snprintf(decodedBuffer, decodedBufferSize, "%s %X_", mnemonic, pendingHighNibble_);
      return true;
    case PendingState::None:
      return false;
  }

  return false;
}

bool ProgramRecorder::handleKey(ProgramVm &vm, uint8_t &editAddress, char keyPressed) {
  const char key = normalizeKey(keyPressed);

  if (pending_ != PendingState::None) {
    switch (pending_) {
      case PendingState::RegisterOperand: {
        uint8_t registerIndex = 0;
        if (!decodeRegisterDesignator(key, registerIndex)) {
          clearPendingState();
          setError(ProgramRecorderError::InvalidOperand);
          return false;
        }

        if (!commitOpcode(vm, editAddress, static_cast<uint8_t>(pendingOpcode_ | registerIndex), false, 0)) {
          clearPendingState();
          setError(ProgramRecorderError::CommitFailed);
          return false;
        }

        clearPendingState();
        setError(ProgramRecorderError::None);
        return true;
      }
      case PendingState::AddressHighNibble: {
        uint8_t nibble = 0;
        if (!decodeHexNibble(key, nibble)) {
          clearPendingState();
          setError(ProgramRecorderError::InvalidOperand);
          return false;
        }

        pendingHighNibble_ = nibble;
        pending_ = PendingState::AddressLowNibble;
        setError(ProgramRecorderError::None);
        return true;
      }
      case PendingState::AddressLowNibble: {
        uint8_t nibble = 0;
        if (!decodeHexNibble(key, nibble)) {
          clearPendingState();
          setError(ProgramRecorderError::InvalidOperand);
          return false;
        }

        const uint8_t addressOperand = static_cast<uint8_t>((pendingHighNibble_ << 4) | nibble);
        if (!commitOpcode(vm, editAddress, pendingOpcode_, true, addressOperand)) {
          clearPendingState();
          setError(ProgramRecorderError::CommitFailed);
          return false;
        }

        clearPendingState();
        setError(ProgramRecorderError::None);
        return true;
      }
      case PendingState::None:
        break;
    }
  }

  if (key == 'k') {
    prefix_ = PrefixState::F;
    setError(ProgramRecorderError::None);
    return true;
  }

  if (key == 'p') {
    prefix_ = PrefixState::K;
    setError(ProgramRecorderError::None);
    return true;
  }

  CommandSpec command{};
  if (prefix_ == PrefixState::None) {
    command = primaryCommandForKey(key);
  } else {
    const PrefixState activePrefix = prefix_;
    prefix_ = PrefixState::None;
    command = shiftedCommandForKey(activePrefix, key);
  }

  switch (command.kind) {
    case CommandSpec::Kind::None:
      setError(ProgramRecorderError::InvalidKey);
      return false;
    case CommandSpec::Kind::Immediate:
      if (!commitOpcode(vm, editAddress, command.opcode, false, 0)) {
        setError(ProgramRecorderError::CommitFailed);
        return false;
      }
      setError(ProgramRecorderError::None);
      return true;
    case CommandSpec::Kind::RegisterOperand:
      pending_ = PendingState::RegisterOperand;
      pendingOpcode_ = command.opcode;
      pendingHighNibble_ = 0;
      setError(ProgramRecorderError::None);
      return true;
    case CommandSpec::Kind::AddressOperand:
      pending_ = PendingState::AddressHighNibble;
      pendingOpcode_ = command.opcode;
      pendingHighNibble_ = 0;
      setError(ProgramRecorderError::None);
      return true;
    case CommandSpec::Kind::MoveNext:
      if (!vm.isStepBoundary(editAddress)) {
        setError(ProgramRecorderError::CommitFailed);
        return false;
      }
      editAddress = vm.nextStepAddress(editAddress);
      setError(ProgramRecorderError::None);
      return true;
    case CommandSpec::Kind::MovePrevious: {
      if (!vm.isStepBoundary(editAddress)) {
        setError(ProgramRecorderError::CommitFailed);
        return false;
      }

      uint8_t previousAddress = 0;
      if (vm.previousStepAddress(editAddress, previousAddress)) {
        editAddress = previousAddress;
      }
      setError(ProgramRecorderError::None);
      return true;
    }
    case CommandSpec::Kind::ToggleInsertMode:
      insertModeEnabled_ = !insertModeEnabled_;
      setError(ProgramRecorderError::None);
      return true;
    case CommandSpec::Kind::PrefixF:
      prefix_ = PrefixState::F;
      setError(ProgramRecorderError::None);
      return true;
    case CommandSpec::Kind::PrefixK:
      prefix_ = PrefixState::K;
      setError(ProgramRecorderError::None);
      return true;
  }

  setError(ProgramRecorderError::InvalidKey);
  return false;
}

bool ProgramRecorder::commitOpcode(ProgramVm &vm,
                                   uint8_t &editAddress,
                                   uint8_t opcode,
                                   bool hasOperand,
                                   uint8_t operand) {
  const uint8_t commitAddress = editAddress;
  bool committed = false;

  if (insertModeEnabled_) {
    const ProgramOpcodeInfo info = describeProgramOpcode(opcode);
    committed = hasOperand ? vm.insertStepAt(commitAddress, opcode, operand) : vm.insertStepAt(commitAddress, opcode);
    if (!committed) {
      return false;
    }

    vm.relocateDirectTargetsAfterInsert(commitAddress, info.width);
  } else {
    committed =
        hasOperand ? vm.replaceStepAt(commitAddress, opcode, operand) : vm.replaceStepAt(commitAddress, opcode);
  }

  if (!committed) {
    return false;
  }

  editAddress = vm.nextStepAddress(commitAddress);
  return true;
}

bool ProgramRecorder::decodeRegisterDesignator(char keyPressed, uint8_t &index) const {
  const char key = normalizeKey(keyPressed);
  if ((key >= '0') && (key <= '9')) {
    index = static_cast<uint8_t>(key - '0');
    return true;
  }

  switch (key) {
    case '.':
      index = 10;
      return true;
    case 'x':
      index = 11;
      return true;
    case 'y':
      index = 12;
      return true;
    case 'z':
      index = 13;
      return true;
    case 'v':
      index = 14;
      return true;
    case 'u':
      index = 15;
      return true;
    default:
      return false;
  }
}

bool ProgramRecorder::decodeHexNibble(char keyPressed, uint8_t &nibble) const {
  return decodeRegisterDesignator(keyPressed, nibble);
}

ProgramRecorder::CommandSpec ProgramRecorder::primaryCommandForKey(char keyPressed) const {
  CommandSpec command{};
  const char key = normalizeKey(keyPressed);

  if ((key >= '0') && (key <= '9')) {
    command.kind = CommandSpec::Kind::Immediate;
    command.opcode = static_cast<uint8_t>(key - '0');
    return command;
  }

  switch (key) {
    case '.':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x0A;
      return command;
    case 'x':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x0B;
      return command;
    case 'y':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x0C;
      return command;
    case 'z':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x0D;
      return command;
    case 'v':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x0E;
      return command;
    case '+':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x10;
      return command;
    case '-':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x11;
      return command;
    case '*':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x12;
      return command;
    case '/':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x13;
      return command;
    case 'u':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x14;
      return command;
    case 'q':
      command.kind = CommandSpec::Kind::RegisterOperand;
      command.opcode = 0x40;
      return command;
    case 'r':
      command.kind = CommandSpec::Kind::RegisterOperand;
      command.opcode = 0x60;
      return command;
    case 's':
      command.kind = CommandSpec::Kind::AddressOperand;
      command.opcode = 0x51;
      return command;
    case 't':
      command.kind = CommandSpec::Kind::AddressOperand;
      command.opcode = 0x53;
      return command;
    case 'n':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x52;
      return command;
    case 'o':
      command.kind = CommandSpec::Kind::Immediate;
      command.opcode = 0x50;
      return command;
    case 'l':
      command.kind = CommandSpec::Kind::MoveNext;
      return command;
    case 'm':
      command.kind = CommandSpec::Kind::MovePrevious;
      return command;
    case 'i':
      command.kind = CommandSpec::Kind::ToggleInsertMode;
      return command;
    case 'k':
      command.kind = CommandSpec::Kind::PrefixF;
      return command;
    case 'p':
      command.kind = CommandSpec::Kind::PrefixK;
      return command;
    default:
      return command;
  }
}

ProgramRecorder::CommandSpec ProgramRecorder::shiftedCommandForKey(PrefixState prefix, char keyPressed) const {
  CommandSpec command{};
  const char key = normalizeKey(keyPressed);

  switch (prefix) {
    case PrefixState::F:
      switch (key) {
        case '7':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x1C;
          return command;
        case '8':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x1D;
          return command;
        case '9':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x1E;
          return command;
        case '.':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x25;
          return command;
        case '4':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x19;
          return command;
        case '5':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x1A;
          return command;
        case '6':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x1B;
          return command;
        case '-':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x21;
          return command;
        case '/':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x23;
          return command;
        case '+':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x20;
          return command;
        case '*':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x22;
          return command;
        case '1':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x16;
          return command;
        case '2':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x17;
          return command;
        case '3':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x18;
          return command;
        case 'u':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x24;
          return command;
        case 'v':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x0F;
          return command;
        case '0':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x15;
          return command;
        case 'l':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x5C;
          return command;
        case 'm':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x5E;
          return command;
        case 'n':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x59;
          return command;
        case 'o':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x57;
          return command;
        case 'q':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x5D;
          return command;
        case 'r':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x5B;
          return command;
        case 's':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x58;
          return command;
        case 't':
          command.kind = CommandSpec::Kind::AddressOperand;
          command.opcode = 0x5A;
          return command;
        default:
          return command;
      }
    case PrefixState::K:
      switch (key) {
        case '7':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x34;
          return command;
        case '8':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x35;
          return command;
        case '9':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x36;
          return command;
        case '4':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x31;
          return command;
        case '5':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x32;
          return command;
        case '6':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x33;
          return command;
        case '+':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x26;
          return command;
        case '3':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x30;
          return command;
        case 'u':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x2A;
          return command;
        case 'v':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x3B;
          return command;
        case '0':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x54;
          return command;
        case '.':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x37;
          return command;
        case 'x':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x38;
          return command;
        case 'y':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x39;
          return command;
        case 'z':
          command.kind = CommandSpec::Kind::Immediate;
          command.opcode = 0x3A;
          return command;
        case 'q':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0xD0;
          return command;
        case 'r':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0xB0;
          return command;
        case 's':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0x80;
          return command;
        case 't':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0xA0;
          return command;
        case 'l':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0xC0;
          return command;
        case 'm':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0xE0;
          return command;
        case 'n':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0x90;
          return command;
        case 'o':
          command.kind = CommandSpec::Kind::RegisterOperand;
          command.opcode = 0x70;
          return command;
        default:
          return command;
      }
    case PrefixState::None:
      return command;
  }

  return command;
}

void ProgramRecorder::clearPendingState() {
  pending_ = PendingState::None;
  pendingOpcode_ = 0;
  pendingHighNibble_ = 0;
}
