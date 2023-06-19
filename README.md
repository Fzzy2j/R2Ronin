# RoninLauncher
![Build Status](https://github.com/R2Ronin/RoninLauncher/actions/workflows/ci.yml/badge.svg)

Launcher used to modify Titanfall 2 to allow Ronin mods and custom content to be loaded.

## Build

Check [BUILD.md](https://github.com/R2Ronin/RoninLauncher/blob/main/BUILD.md) for instructions on how to compile, you can also download [binaries built by GitHub Actions](https://github.com/R2Ronin/RoninLauncher/actions).

## Format

This project uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html), make sure you run `clang-format -i --style=file RoninLauncher/*.cpp RoninLauncher/*.h RoninDLL/*.cpp RoninDLL/*.h` when opening a Pull Request. Check the tool's website for instructions on how to integrate it with your IDE.
