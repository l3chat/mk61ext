#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_BIN="/tmp/rpn_calculator_regression"

echo "Building host regression binary..."
g++ -std=c++17 \
  -I"${PROJECT_DIR}/include" \
  "${PROJECT_DIR}/test/rpn_calculator_regression.cpp" \
  "${PROJECT_DIR}/src/ProgramRunner.cpp" \
  "${PROJECT_DIR}/src/RpnCalculator.cpp" \
  "${PROJECT_DIR}/src/ProgramRecorder.cpp" \
  "${PROJECT_DIR}/src/ProgramVm.cpp" \
  -o "${OUTPUT_BIN}"

echo "Running host regression binary..."
"${OUTPUT_BIN}"
