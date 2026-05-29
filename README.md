# DANDU MOD injecter

A beginner-friendly desktop app that installs Minecraft mod `.jar` files with drag-and-drop.

Built with C++17 and Qt Widgets for Windows.

## What It Does

1. Drag a mod `.jar` into the app.
2. Detect whether it is NeoForge, Fabric, or Quilt.
3. Read mod metadata (name, id, version, Minecraft version rule).
4. Choose a compatible launcher profile (or create one when possible).
5. Copy the mod into the correct `mods` folder.

Important: this tool installs files. It does not inject into running process memory.

## Features

- Drag-and-drop mod install flow
- Loader detection:
  - `quilt.mod.json` -> Quilt
  - `fabric.mod.json` -> Fabric
  - `META-INF/neoforge.mods.toml` or `META-INF/mods.toml` -> NeoForge
- Minecraft profile discovery from launcher JSON files
- Compatibility checks for loader and Minecraft version
- Auto-select profile and optional auto-create profile
- Animated Qt GUI

## Requirements

### For End Users

- Windows 10 or 11
- Minecraft Java Edition
- Correct loader installed first (NeoForge, Fabric, or Quilt)

### For Building From Source

- Qt 6 or Qt 5 with Widgets
- CMake 3.16+
- C++17 compiler (MinGW or MSVC)
- zlib (or toolchain that already provides compatible zlib)

## After You Install/Clone This Repo

Do this right away after downloading the repository.

### Option A: Run the Included Built App (Easiest)

If the repo already contains `build-qt\DANDU_MOD_injecter.exe` and Qt DLL files:

1. Open this folder:
   - `build-qt`
2. Double-click:
   - `DANDU_MOD_injecter.exe`
3. Go to **Quick Start (No Coding)** below and install your mod.

### Option B: Build It Yourself (If EXE Is Missing)

1. Open PowerShell in the repo folder.
2. Run:

```powershell
cmake -S . -B build-qt -G Ninja `
  -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/Ninja/ninja.exe" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/mingw_64" `
  -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build-qt
```

3. Start the app:
   - `build-qt\DANDU_MOD_injecter.exe`

### Important Next Step (Share With Other PCs)

If you plan to send the app to someone else, run this in Command Prompt inside `build-qt`:

```bat
"C:\Qt\6.11.1\mingw_64\bin\windeployqt.exe" --compiler-runtime DANDU_MOD_injecter.exe
```

Then share the full `build-qt` folder, not only the `.exe`.

## Quick Start (No Coding)

1. Open `DANDU_MOD_injecter.exe`.
2. Pick your Minecraft profile in the dropdown.
3. Drag one mod `.jar` onto the drop panel.
4. Review detected loader/version info.
5. Install (or let auto-install do it).
6. Start Minecraft with the same profile and verify the mod appears.

## Full Beginner Guide

### 1. Install Loader First

Before using this app, install the correct loader for the same Minecraft version as your mod:

- NeoForge mod -> install NeoForge
- Fabric mod -> install Fabric Loader
- Quilt mod -> install Quilt Loader

Run Minecraft once after loader install so the launcher creates required profiles/folders.

### 2. Open the App

- Launch `DANDU_MOD_injecter.exe`
- Recommended:
  - `Auto install after drop` ON
  - `Auto create profile if needed` ON

### 3. Drop the Mod

Drop exactly one `.jar` onto the card in the center.

The app shows:

- Mod name
- Mod id
- Mod version
- Loader type
- Minecraft version rule

### 4. Install

- If auto-install is ON, install happens immediately.
- If OFF, click `Install Mod`.

### 5. Launch Minecraft

Open Minecraft Launcher, select the profile used by the app, and run game.

If the mod is supported and dependencies exist, it will load.

## Build From Source

### MinGW + Qt Example

```powershell
cmake -S . -B build-qt -G Ninja `
  -DCMAKE_MAKE_PROGRAM="C:/Qt/Tools/Ninja/ninja.exe" `
  -DCMAKE_PREFIX_PATH="C:/Qt/6.11.1/mingw_64" `
  -DCMAKE_CXX_COMPILER="C:/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build-qt
```

Output:

- `build-qt\DANDU_MOD_injecter.exe`

### MSVC Example

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:/Qt/<version>/<msvc_kit>"
cmake --build build --config Release
```

Output:

- `build\Release\DANDU_MOD_injecter.exe`

## Deploy Qt Runtime Next to EXE

From Command Prompt in the executable folder:

```bat
"C:\Qt\6.11.1\mingw_64\bin\windeployqt.exe" --compiler-runtime DANDU_MOD_injecter.exe
```

This copies required Qt DLLs and plugin folders (`platforms`, `imageformats`, etc.).

## Troubleshooting

### Unknown Mod Type

The file may be non-mod, corrupted, or for unsupported loader format.

### No Compatible Profile

Install required loader + Minecraft version first, then run launcher once.

### Mod Installed But Not Loading

Check:

- Correct launcher profile selected
- Correct Minecraft version
- Required dependency mods installed
- Mod version matches loader/game version

### App Fails On Another PC

Run `windeployqt` so all Qt runtime files are packaged beside `.exe`.

## Project Layout

```text
.
|-- CMakeLists.txt
|-- README.md
`-- src
    |-- main.cpp
    |-- MainWindow.*
    |-- DropCard.*
    |-- ModTypes.h
    |-- ModAnalyzer.*
    |-- VersionMatcher.*
    |-- MinecraftManager.*
    `-- SimpleZipReader.*
```

## Security Notes

- The app copies files only.
- Download mods from trusted sources.
- Scan unknown mods before use.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

Add your chosen license file before public release (for example MIT).
