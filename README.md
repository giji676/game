# Game Engine / OpenGL Learning Project

A small game project built to learn modern OpenGL and real-time rendering techniques. The project includes a custom game engine written from scratch and serves as a sandbox for experimenting with graphics programming, engine architecture, and game development concepts.

## Features

* OpenGL rendering pipeline
* Custom asset management system
* Model and texture loading
* Material and shader system
* Scene graph with hierarchical structure
* Basic lighting and shading
* Camera controls
* Custom input handling

## Project Goals

The primary goal of this project is educational. Rather than relying on existing engines, most systems are implemented from scratch to gain a deeper understanding of:

* Graphics programming
* Rendering pipelines
* Scene management
* Resource management
* Mathematics for 3D graphics
* Engine architecture

## Dependencies

Windows/MinGW third-party trees are not committed. Fetch them (and init gj-lib
submodules) from the repo root:

```bash
./scripts/fetch-deps.sh          # all deps
./scripts/fetch-deps.sh sdl      # SDL2 MinGW devel only
./scripts/fetch-deps.sh freetype # cross-build FreeType for MinGW
./scripts/fetch-deps.sh submodules
```

Linux builds still use system SDL2/FreeType via the linker and `pkg-config`.

For a Windows cross-build, runtime DLLs are copied into `dist/windows/` automatically
(SDL2, FreeType, and MinGW runtimes).

## Building

```bash
make                         # native (linux/macos) -> dist/<os>/
make TARGET_OS=windows       # MinGW cross-compile (after fetch-deps)
make all-targets             # every platform this host can build
make clean-all               # remove all build/ and dist/ outputs
```

On Linux with MinGW installed, `all-targets` builds `linux` and `windows`.
macOS is only included when building on Darwin.

## Running

```bash
./dist/linux/gj-engine
wine ./dist/windows/gj-engine.exe
```

## Status

This project is actively developed and used as a learning platform for experimenting with new rendering and engine features.
