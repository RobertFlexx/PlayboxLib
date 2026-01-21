#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
DEMO="${1:-}"

if [[ -z "${DEMO}" ]]; then
  printf "usage: tools/dev.sh <demo_name>\n"
  exit 2
fi

"${ROOT}/tools/build.sh" all --debug

BIN=""
for d in "${ROOT}/build" "${ROOT}/build/bin" "${ROOT}/bin" "${ROOT}/out" "${ROOT}"; do
  if [[ -f "${d}/${DEMO}" ]]; then
    BIN="${d}/${DEMO}"
    break
  fi
done

if [[ -z "${BIN}" ]]; then
  printf "could not find demo '%s' in common bin dirs\n" "${DEMO}"
  exit 1
fi

exec "${BIN}"
