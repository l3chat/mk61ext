#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
OUTPUT_BIN="/tmp/rpn_calculator_trace"

if [[ $# -eq 0 ]]; then
  if [[ -n "${TRACE_SEQ:-}" ]]; then
    read -r -a TRACE_ARGS <<< "${TRACE_SEQ}"
  else
    echo "Usage: bash scripts/trace_sequence.sh <token> [<token> ...]" >&2
    echo "Example: bash scripts/trace_sequence.sh 5 XM 1 3 ENTER 4 '*' MX 1" >&2
    echo "Or: TRACE_SEQ=\"5 XM 1 3 ENTER 4 * MX 1\" pio run -t trace" >&2
    exit 1
  fi
else
  TRACE_ARGS=("$@")
fi

echo "Building host trace binary..."
g++ -std=c++17 \
  -I"${PROJECT_DIR}/include" \
  "${PROJECT_DIR}/tools/rpn_calculator_trace.cpp" \
  "${PROJECT_DIR}/src/CalculatorKeymap.cpp" \
  "${PROJECT_DIR}/src/RpnCalculator.cpp" \
  -o "${OUTPUT_BIN}"

echo "Running host trace binary..."
"${OUTPUT_BIN}" "${TRACE_ARGS[@]}"
