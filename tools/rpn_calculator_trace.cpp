#include "CalculatorKeymap.h"
#include "RpnCalculator.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

enum class PendingRegisterOperation : uint8_t {
  None,
  DirectRecall,
  DirectStore,
  IndirectRecall,
  IndirectStore,
};

PendingRegisterOperation gPendingRegisterOperation = PendingRegisterOperation::None;

const char *pendingRegisterOperationName() {
  switch (gPendingRegisterOperation) {
    case PendingRegisterOperation::DirectRecall:
      return "RCL";
    case PendingRegisterOperation::DirectStore:
      return "STO";
    case PendingRegisterOperation::IndirectRecall:
      return "RCLI";
    case PendingRegisterOperation::IndirectStore:
      return "STOI";
    case PendingRegisterOperation::None:
      return "";
  }

  return "";
}

void clearPendingRegisterOperation() {
  gPendingRegisterOperation = PendingRegisterOperation::None;
}

void armPendingRegisterOperation(PendingRegisterOperation operation) {
  gPendingRegisterOperation = operation;
  resetCalculatorKeymapState();
}

bool decodeRegisterKey(char keyPressed, uint8_t &index) {
  if ((keyPressed >= '0') && (keyPressed <= '9')) {
    index = static_cast<uint8_t>(keyPressed - '0');
    return true;
  }

  switch (keyPressed) {
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

bool handleRegisterOperationKey(RpnCalculator &calculator, char keyPressed) {
  const CalculatorPrefix prefix = activeCalculatorPrefix();

  if ((prefix == CalculatorPrefix::None) && (keyPressed == 'q')) {
    armPendingRegisterOperation(PendingRegisterOperation::DirectRecall);
    return true;
  }

  if ((prefix == CalculatorPrefix::None) && (keyPressed == 'r')) {
    armPendingRegisterOperation(PendingRegisterOperation::DirectStore);
    return true;
  }

  if ((prefix == CalculatorPrefix::K) && (keyPressed == 'q')) {
    armPendingRegisterOperation(PendingRegisterOperation::IndirectRecall);
    return true;
  }

  if ((prefix == CalculatorPrefix::K) && (keyPressed == 'r')) {
    armPendingRegisterOperation(PendingRegisterOperation::IndirectStore);
    return true;
  }

  if (gPendingRegisterOperation == PendingRegisterOperation::None) {
    return false;
  }

  const PendingRegisterOperation operation = gPendingRegisterOperation;
  clearPendingRegisterOperation();

  uint8_t registerIndex = 0;
  if (!decodeRegisterKey(keyPressed, registerIndex)) {
    return true;
  }

  switch (operation) {
    case PendingRegisterOperation::DirectRecall:
      (void)calculator.recallRegister(registerIndex);
      return true;
    case PendingRegisterOperation::DirectStore:
      (void)calculator.storeRegister(registerIndex);
      return true;
    case PendingRegisterOperation::IndirectRecall:
      (void)calculator.recallIndirectRegister(registerIndex);
      return true;
    case PendingRegisterOperation::IndirectStore:
      (void)calculator.storeIndirectRegister(registerIndex);
      return true;
    case PendingRegisterOperation::None:
      return false;
  }

  return false;
}

const char *calculatorModeName(const RpnCalculator &calculator) {
  if (calculator.hasError()) {
    return "ERR";
  }

  if (calculator.isEnteringExponent()) {
    return "EEX";
  }

  if (calculator.isEntering()) {
    return "ENT";
  }

  return "RUN";
}

std::string traceStatus(const RpnCalculator &calculator) {
  if (calculator.hasError()) {
    return std::string("ERR ") + calculator.errorMessage();
  }

  if (gPendingRegisterOperation != PendingRegisterOperation::None) {
    return pendingRegisterOperationName();
  }

  const char *prefixName = activeCalculatorPrefixName();
  if (prefixName[0] != '\0') {
    return std::string(prefixName) + " " + calculatorModeName(calculator);
  }

  return calculatorModeName(calculator);
}

void formatNumericStackValue(CalculatorValue value, char *buffer, size_t bufferSize) {
  const CalculatorValue normalizedValue = (std::fabs(value) < 1e-12) ? 0.0 : value;
  std::snprintf(buffer, bufferSize, "%.15g", normalizedValue);
}

void printState(size_t stepIndex, char rawKey, const RpnCalculator &calculator) {
  char xBuffer[RpnCalculator::kEntryBufferSize];
  char yBuffer[32];
  char zBuffer[32];
  char tBuffer[32];
  const CalculatorStack stack = calculator.stack();

  calculator.formatXForDisplay(xBuffer, sizeof(xBuffer), 15);
  formatNumericStackValue(stack.y, yBuffer, sizeof(yBuffer));
  formatNumericStackValue(stack.z, zBuffer, sizeof(zBuffer));
  formatNumericStackValue(stack.t, tBuffer, sizeof(tBuffer));

  std::cout << stepIndex << ". key `" << rawKey << "`" << '\n';
  std::cout << "   status: " << traceStatus(calculator) << '\n';
  std::cout << "   T: " << tBuffer << '\n';
  std::cout << "   Z: " << zBuffer << '\n';
  std::cout << "   Y: " << yBuffer << '\n';
  std::cout << "   " << (calculator.isEntering() ? "X>" : "X:") << ' ' << xBuffer << '\n';
}

bool appendRegisterKey(std::vector<char> &rawKeys, char registerDesignator) {
  switch (static_cast<char>(std::tolower(static_cast<unsigned char>(registerDesignator)))) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      rawKeys.push_back(registerDesignator);
      return true;
    case 'a':
      rawKeys.push_back('.');
      return true;
    case 'b':
      rawKeys.push_back('x');
      return true;
    case 'c':
      rawKeys.push_back('y');
      return true;
    case 'd':
      rawKeys.push_back('z');
      return true;
    case 'e':
      rawKeys.push_back('v');
      return true;
    case 'f':
      rawKeys.push_back('u');
      return true;
    default:
      return false;
  }
}

std::string uppercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return value;
}

bool expandAliasToken(const std::string &token, std::vector<char> &rawKeys) {
  const std::string upper = uppercase(token);

  if ((token.size() == 1) &&
      std::strchr("0123456789.+-*/abcdefghijklmnopqrstuvwxyz", token[0])) {
    rawKeys.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(token[0]))));
    return true;
  }

  if (upper == "ENTER") {
    rawKeys.push_back('v');
    return true;
  }
  if (upper == "EEX") {
    rawKeys.push_back('y');
    return true;
  }
  if (upper == "CHS") {
    rawKeys.push_back('x');
    return true;
  }
  if ((upper == "SWAP") || (upper == "XY") || (upper == "X<->Y") || (upper == "X<>Y")) {
    rawKeys.push_back('u');
    return true;
  }
  if ((upper == "CX") || (upper == "CLX")) {
    rawKeys.push_back('z');
    return true;
  }
  if (upper == "F") {
    rawKeys.push_back('k');
    return true;
  }
  if (upper == "K") {
    rawKeys.push_back('p');
    return true;
  }
  if (upper == "RCL") {
    rawKeys.push_back('q');
    return true;
  }
  if (upper == "STO") {
    rawKeys.push_back('r');
    return true;
  }
  if (upper == "RCLI") {
    rawKeys.push_back('p');
    rawKeys.push_back('q');
    return true;
  }
  if (upper == "STOI") {
    rawKeys.push_back('p');
    rawKeys.push_back('r');
    return true;
  }

  auto expandPrefixedRegisterToken = [&](const char *prefix, const std::vector<char> &leadKeys) {
    const size_t prefixLength = std::strlen(prefix);
    if ((upper.size() != prefixLength + 1) || (upper.rfind(prefix, 0) != 0)) {
      return false;
    }

    rawKeys.insert(rawKeys.end(), leadKeys.begin(), leadKeys.end());
    return appendRegisterKey(rawKeys, token.back());
  };

  if (expandPrefixedRegisterToken("RCLI", {'p', 'q'})) {
    return true;
  }
  if (expandPrefixedRegisterToken("STOI", {'p', 'r'})) {
    return true;
  }
  if (expandPrefixedRegisterToken("RCL", {'q'})) {
    return true;
  }
  if (expandPrefixedRegisterToken("STO", {'r'})) {
    return true;
  }

  return false;
}

void printUsage(const char *argv0) {
  std::cerr << "Usage: " << argv0 << " <token> [<token> ...]\n";
  std::cerr << "Examples:\n";
  std::cerr << "  " << argv0 << " 5 STO 1 3 ENTER 4 '*' RCL 1\n";
  std::cerr << "  " << argv0 << " 5 r 1 3 v 4 '*' q 1\n";
  std::cerr << "  " << argv0 << " 6 6 STO 6 6 STO 7 RCLI 7\n";
  std::cerr << "Supported aliases: ENTER, EEX, CHS, CX, SWAP, RCL, STO, RCLI, STOI, F, K,\n";
  std::cerr << "and combined register forms like STO1, RCLA, RCLIE.\n";
}

}  // namespace

int main(int argc, char **argv) {
  if (argc <= 1) {
    printUsage(argv[0]);
    return 1;
  }

  std::vector<char> rawKeys;
  for (int index = 1; index < argc; ++index) {
    if (!expandAliasToken(argv[index], rawKeys)) {
      std::cerr << "Unsupported token: " << argv[index] << '\n';
      printUsage(argv[0]);
      return 2;
    }
  }

  resetCalculatorKeymapState();
  clearPendingRegisterOperation();

  RpnCalculator calculator;
  std::cout << "Expanded raw keys:";
  for (char rawKey : rawKeys) {
    std::cout << ' ' << rawKey;
  }
  std::cout << "\n\n";

  size_t stepIndex = 1;
  for (char rawKey : rawKeys) {
    if (!handleRegisterOperationKey(calculator, rawKey)) {
      (void)calculator.apply(translateKeyToCalculatorAction(rawKey));
    }
    printState(stepIndex, rawKey, calculator);
    ++stepIndex;
  }

  return calculator.hasError() ? 3 : 0;
}
