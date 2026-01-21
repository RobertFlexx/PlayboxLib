.PHONY: all meson cmake clean distclean install examples help

all: meson

meson:
	@printf "\033[1;36m==> Building with Meson\033[0m\n"
	@meson setup build/meson --wipe >/dev/null 2>&1 || meson setup build/meson
	@meson compile -C build/meson

cmake:
	@printf "\033[1;36m==> Building with CMake\033[0m\n"
	@cmake -S . -B build/cmake -DCMAKE_BUILD_TYPE=Release
	@cmake --build build/cmake

examples:
	@printf "\033[1;36m==> Building examples (Meson preferred)\033[0m\n"
	@meson setup build/meson --wipe >/dev/null 2>&1 || true
	@meson compile -C build/meson

install:
	@printf "\033[1;32m==> Installing (Meson preferred)\033[0m\n"
	@meson install -C build/meson

clean:
	@printf "\033[1;33m==> Cleaning build artifacts\033[0m\n"
	@rm -rf build/meson build/cmake

distclean: clean
	@printf "\033[1;31m==> Distclean complete\033[0m\n"

help:
	@printf "\n"
	@printf "PlayboxLib build targets:\n"
	@printf "  make            Build with Meson (default)\n"
	@printf "  make meson      Build using Meson\n"
	@printf "  make cmake      Build using CMake\n"
	@printf "  make examples   Build examples\n"
	@printf "  make install    Install library\n"
	@printf "  make clean      Remove build directories\n"
	@printf "\n"
