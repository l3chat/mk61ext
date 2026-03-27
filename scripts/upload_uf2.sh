#!/usr/bin/env bash

set -euo pipefail

uf2_path="${1:-}"
port_hint="${2:-}"

if [[ -z "$uf2_path" ]]; then
  echo "Usage: $0 <firmware.uf2> [upload-port-or-mount]" >&2
  exit 1
fi

if [[ ! -f "$uf2_path" ]]; then
  echo "UF2 file not found: $uf2_path" >&2
  exit 1
fi

find_boot_drive() {
  findmnt -rno TARGET,LABEL 2>/dev/null | awk '$2 == "RPI-RP2" { print $1; exit }'
}

is_serial_port() {
  [[ -n "${1:-}" && -c "$1" ]]
}

find_serial_port() {
  if is_serial_port "$port_hint"; then
    printf '%s\n' "$port_hint"
    return 0
  fi

  for candidate in /dev/ttyACM* /dev/ttyUSB*; do
    if [[ -c "$candidate" ]]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  return 1
}

touch_serial_port() {
  local port="$1"

  stty -F "$port" 1200 raw -echo -echoe -echok
  exec 3<>"$port"
  exec 3>&-
}

wait_for_boot_drive() {
  local timeout_seconds="${1:-15}"
  local deadline=$((SECONDS + timeout_seconds))
  local mount_point=""

  while (( SECONDS < deadline )); do
    mount_point="$(find_boot_drive || true)"
    if [[ -n "$mount_point" ]]; then
      printf '%s\n' "$mount_point"
      return 0
    fi
    sleep 1
  done

  return 1
}

boot_drive=""

if [[ -n "$port_hint" && -d "$port_hint" ]]; then
  boot_drive="$port_hint"
else
  boot_drive="$(find_boot_drive || true)"
fi

if [[ -z "$boot_drive" ]]; then
  serial_port="$(find_serial_port || true)"
  if [[ -z "${serial_port:-}" ]]; then
    echo "Could not find a Pico serial port or an RPI-RP2 boot drive." >&2
    exit 1
  fi

  echo "Resetting Pico on $serial_port into BOOTSEL mode..."
  touch_serial_port "$serial_port"
  boot_drive="$(wait_for_boot_drive 15 || true)"
fi

if [[ -z "$boot_drive" ]]; then
  echo "Timed out waiting for the RPI-RP2 boot drive." >&2
  exit 1
fi

echo "Copying $(basename "$uf2_path") to $boot_drive..."
cp "$uf2_path" "$boot_drive/"
sync

echo "UF2 upload complete."
