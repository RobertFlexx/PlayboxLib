# Package

version       = "1.1.0"
author        = "PlayboxLib"
description   = "PlayboxLib Nim bindings — TUI raylib"
license       = "MIT"
srcDir        = "."
skipFiles     = @["demo.nim"]

# Dependencies

requires "nim >= 1.6.0"

task demo, "Build the Nim Playbox demo":
  exec "nim c -d:release -o:../../build/bin/pb_nim_demo demo.nim"
