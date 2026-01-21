#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
MODE="release"
JOBS="${JOBS:-0}"
CC_BIN="${CC:-cc}"
CXX_BIN="${CXX:-c++}"

t_reset="$(printf '\033[0m')"
t_bold="$(printf '\033[1m')"
t_dim="$(printf '\033[2m')"
t_red="$(printf '\033[31m')"
t_grn="$(printf '\033[32m')"
t_yel="$(printf '\033[33m')"
t_blu="$(printf '\033[34m')"
t_mag="$(printf '\033[35m')"
t_cyn="$(printf '\033[36m')"

say() { printf "%b\n" "$*"; }
ok() { say "${t_grn}${t_bold}ok${t_reset}  $*"; }
info() { say "${t_cyn}${t_bold}info${t_reset} $*"; }
warn() { say "${t_yel}${t_bold}warn${t_reset} $*"; }
die() { say "${t_red}${t_bold}fail${t_reset} $*"; exit 1; }

hr() { say "${t_dim}------------------------------------------------------------${t_reset}"; }

need() {
  command -v "$1" >/dev/null 2>&1 || die "missing tool: $1"
}

jobs_auto() {
  if [[ "${JOBS}" -gt 0 ]]; then
    echo "${JOBS}"
    return
  fi
  if command -v nproc >/dev/null 2>&1; then
    nproc
    return
  fi
  sysctl -n hw.ncpu 2>/dev/null || echo 4
}

usage() {
  cat <<EOF
playbox build tool

usage:
  tools/build.sh build [--debug|--release] [--sanitize] [--jobs N]
  tools/build.sh clean
  tools/build.sh install --prefix <dir>
  tools/build.sh rust [--debug|--release]
  tools/build.sh all [--debug|--release]
  tools/build.sh doctor

notes:
  - builds libplaybox.a and libplaybox.so into build/lib
  - builds C++ wrapper into build/lib (if cpp/playbox.cpp exists)
  - builds rust CLI into build/bin (if tools/rust exists)
EOF
}

mode_flags() {
  if [[ "${MODE}" == "debug" ]]; then
    echo "-O0 -g3"
  else
    echo "-O2 -DNDEBUG"
  fi
}

sanitize_flags() {
  if [[ "${1:-}" == "1" ]]; then
    echo "-fsanitize=address,undefined -fno-omit-frame-pointer"
  else
    echo ""
  fi
}

pick_input_src() {
  if [[ -f "${ROOT}/src/pb_input_posix.c" ]]; then
    echo "${ROOT}/src/pb_input_posix.c"
  else
    echo "${ROOT}/src/pb_input.c"
  fi
}

build_lib() {
  local sanitize="${1:-0}"
  local cflags
  local sflags
  local j
  j="$(jobs_auto)"

  mkdir -p "${BUILD}/obj" "${BUILD}/lib" "${BUILD}/bin"

  cflags="$(mode_flags)"
  sflags="$(sanitize_flags "${sanitize}")"

  local inc="-I${ROOT}/include -I${ROOT}/include/playbox"
  local base="-std=c11 -Wall -Wextra -Wpedantic ${cflags} ${sflags} ${inc}"

  local src=()
  src+=("${ROOT}/src/pb_app.c")
  src+=("${ROOT}/src/pb_fb.c")
  src+=("$(pick_input_src)")
  src+=("${ROOT}/src/pb_renderer.c")
  src+=("${ROOT}/src/pb_term_posix.c")
  src+=("${ROOT}/src/pb_time.c")
  src+=("${ROOT}/src/pb_utf8.c")
  [[ -f "${ROOT}/src/pb_replay.c" ]] && src+=("${ROOT}/src/pb_replay.c")

  local obj=()
  for f in "${src[@]}"; do
    [[ -f "$f" ]] || die "missing source: $f"
    obj+=("${BUILD}/obj/$(basename "${f%.c}").o")
  done

  hr
  info "building playbox core (${MODE})"
  info "cc=${CC_BIN}"
  info "jobs=${j}"
  hr

  local i=0
  for f in "${src[@]}"; do
    local o="${obj[$i]}"
    i=$((i+1))
    printf "%b\n" "${t_mag}${t_bold}cc${t_reset}   ${t_dim}${f##${ROOT}/}${t_reset}"
    "${CC_BIN}" ${base} -fPIC -c "$f" -o "$o"
  done

  printf "%b\n" "${t_mag}${t_bold}ar${t_reset}   ${t_dim}lib/libplaybox.a${t_reset}"
  ar rcs "${BUILD}/lib/libplaybox.a" "${obj[@]}"

  printf "%b\n" "${t_mag}${t_bold}ld${t_reset}   ${t_dim}lib/libplaybox.so${t_reset}"
  "${CC_BIN}" -shared ${sflags} -o "${BUILD}/lib/libplaybox.so" "${obj[@]}"

  ok "core built -> build/lib"
}

build_cpp() {
  [[ -f "${ROOT}/cpp/playbox.cpp" ]] || return 0

  mkdir -p "${BUILD}/obj" "${BUILD}/lib"

  local cflags
  cflags="$(mode_flags)"
  local inc="-I${ROOT}/include -I${ROOT}/include/playbox -I${ROOT}/cpp"
  local base="-std=c++17 -Wall -Wextra -Wpedantic ${cflags} ${inc}"

  hr
  info "building playbox C++ wrapper (${MODE})"
  info "cxx=${CXX_BIN}"
  hr

  printf "%b\n" "${t_mag}${t_bold}cxx${t_reset}  ${t_dim}cpp/playbox.cpp${t_reset}"
  "${CXX_BIN}" ${base} -c "${ROOT}/cpp/playbox.cpp" -o "${BUILD}/obj/playbox_cpp.o"

  printf "%b\n" "${t_mag}${t_bold}ar${t_reset}   ${t_dim}lib/libplayboxpp.a${t_reset}"
  ar rcs "${BUILD}/lib/libplayboxpp.a" "${BUILD}/obj/playbox_cpp.o"

  ok "C++ wrapper built -> build/lib"
}

build_rust() {
  [[ -d "${ROOT}/tools/rust" ]] || { warn "tools/rust not found, skipping rust"; return 0; }
  need cargo
  local prof="--release"
  [[ "${MODE}" == "debug" ]] && prof=""

  hr
  info "building rust CLI (${MODE})"
  hr

  (cd "${ROOT}/tools/rust" && cargo build ${prof})

  local bin
  if [[ "${MODE}" == "debug" ]]; then
    bin="${ROOT}/tools/rust/target/debug/playbox"
  else
    bin="${ROOT}/tools/rust/target/release/playbox"
  fi

  [[ -f "${bin}" ]] || die "rust build finished but playbox binary not found"
  cp -f "${bin}" "${BUILD}/bin/playbox"

  ok "rust CLI built -> build/bin/playbox"
}

clean_all() {
  rm -rf "${BUILD}"
  ok "cleaned build/"
}

install_prefix() {
  local prefix="${1:-}"
  [[ -n "${prefix}" ]] || die "install needs --prefix <dir>"

  mkdir -p "${prefix}/include" "${prefix}/lib" "${prefix}/bin"

  rsync -a --delete "${ROOT}/include/" "${prefix}/include/"
  [[ -f "${BUILD}/lib/libplaybox.a" ]] && cp -f "${BUILD}/lib/libplaybox.a" "${prefix}/lib/"
  [[ -f "${BUILD}/lib/libplaybox.so" ]] && cp -f "${BUILD}/lib/libplaybox.so" "${prefix}/lib/"
  [[ -f "${BUILD}/lib/libplayboxpp.a" ]] && cp -f "${BUILD}/lib/libplayboxpp.a" "${prefix}/lib/"
  [[ -f "${BUILD}/bin/playbox" ]] && cp -f "${BUILD}/bin/playbox" "${prefix}/bin/"

  ok "installed to ${prefix}"
}

doctor() {
  hr
  info "doctor report"
  hr
  say "root: ${ROOT}"
  say "cc:   ${CC_BIN}"
  say "cxx:  ${CXX_BIN}"
  say "mode: ${MODE}"
  say "term: ${TERM:-unset}"
  say "cols: ${COLUMNS:-unset}"
  say "rows: ${LINES:-unset}"
  say "input: $(basename "$(pick_input_src)")"
  [[ -f "${ROOT}/src/pb_replay.c" ]] && say "replay: enabled (pb_replay.c present)" || say "replay: missing (add pb_replay.c)"
  ok "done"
}

cmd="${1:-}"
shift || true

sanitize=0
prefix=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --debug) MODE="debug"; shift ;;
    --release) MODE="release"; shift ;;
    --sanitize) sanitize=1; shift ;;
    --jobs) JOBS="${2:-0}"; shift 2 ;;
    --prefix) prefix="${2:-}"; shift 2 ;;
    *) die "unknown option: $1" ;;
  esac
done

case "${cmd}" in
  build) build_lib "${sanitize}"; build_cpp ;;
  rust) build_rust ;;
  all) build_lib "${sanitize}"; build_cpp; build_rust ;;
  clean) clean_all ;;
  install) install_prefix "${prefix}" ;;
  doctor) doctor ;;
  ""|help|-h|--help) usage ;;
  *) die "unknown command: ${cmd}" ;;
esac
