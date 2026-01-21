#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"

mode="${1:-}"
demo="${2:-}"
file="${3:-}"

if [[ -z "${mode}" || -z "${demo}" ]]; then
  printf "usage:\n"
  printf "  tools/replayctl.sh run <demo>\n"
  printf "  tools/replayctl.sh record <demo> <out.pbr>\n"
  printf "  tools/replayctl.sh replay <demo> <in.pbr>\n"
  exit 2
fi

BIN=""
for d in "${ROOT}/build" "${ROOT}/build/bin" "${ROOT}/bin" "${ROOT}/out" "${ROOT}"; do
  if [[ -f "${d}/${demo}" ]]; then
    BIN="${d}/${demo}"
    break
  fi
done

if [[ -z "${BIN}" ]]; then
  printf "could not find demo '%s'\n" "${demo}"
  exit 1
fi

case "${mode}" in
  run)
    exec "${BIN}"
    ;;
  record)
    [[ -n "${file}" ]] || { printf "record needs <out.pbr>\n"; exit 2; }
    PLAYBOX_RECORD="${file}" exec "${BIN}"
    ;;
  replay)
    [[ -n "${file}" ]] || { printf "replay needs <in.pbr>\n"; exit 2; }
    PLAYBOX_REPLAY="${file}" exec "${BIN}"
    ;;
  *)
    printf "unknown mode: %s\n" "${mode}"
    exit 2
    ;;
esac
