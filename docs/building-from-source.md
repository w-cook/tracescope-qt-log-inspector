# Building TraceScope from Source

This guide covers a local developer build of the TraceScope desktop application, its automated test executables, and the optional standalone Live-Log Generator. It describes the supported CMake/Qt configuration reflected in the repository rather than promising that every possible host-toolchain combination has been tested.

**Documentation baseline:** `phase-16-final-polish` at commit `8011035`. The reviewed repository and CI still identify the published prerelease as `v0.16.0`; check current project/workflow files again before creating or documenting the final v1.0 packages.

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Check build prerequisites and supported environments | [1. Requirements](#1-requirements) |
| Configure/build using Qt Creator | [2. Qt Creator build](#2-qt-creator-build) |
| Build on Windows using the command line | [3. Windows command-line build](#3-windows-command-line-build) |
| Build on Linux using the command line | [4. Linux command-line build](#4-linux-command-line-build) |
| Run tests or the standalone generator | [5. Tests and development utilities](#5-tests-and-development-utilities) |
| Understand packaging and investigate build problems | [6. Deployment and troubleshooting](#6-deployment-and-troubleshooting) |

## 1. Requirements

TraceScope's root [`CMakeLists.txt`](../CMakeLists.txt) declares a **C++17** project with **CMake 3.21+** and requires these **Qt 6** components:

- Qt Widgets
- Qt Charts
- Qt Concurrent
- Qt Test (for the repository's automated test executables)

The optional Live-Log Generator CLI uses Qt Core, and its separate launcher uses Qt Widgets. The root build enables both the generator subdirectory and the test subdirectory.

Use a matching **64-bit Qt kit and compiler**. The currently documented development baseline is Windows Qt **6.11.1** with 64-bit MinGW. The reviewed CI provides repeatable validation using **Qt 6.10.3**, matching MinGW on Windows and GCC on **Ubuntu 22.04**. These are documented environments—not a claim that the project requires exactly one Qt minor version or that other compiler combinations are verified.

For command-line builds, install Git, CMake, the matching C++ compiler, and a build tool such as Ninja. Select a Qt installation that includes **Qt Charts** and a kit compatible with the compiler you invoke. Building against one Qt installation and launching against an unrelated Qt runtime can produce misleading failures.

```sh
# From a normal shell after your tools are configured
git clone https://github.com/w-cook/tracescope-qt-log-inspector.git
cd tracescope-qt-log-inspector
```

To build a particular source revision, check out that revision locally. The project version in CMake and package names may lag an unreleased development branch until its release preparation is complete.

## 2. Qt Creator build

Qt Creator is the simplest route when the installed Qt kit already supplies the correct compiler, Qt Charts and CMake.

1. Open the repository root `CMakeLists.txt` in Qt Creator.
2. Select a **64-bit Qt 6 desktop kit** with Qt Charts installed (Windows: a kit matching your chosen MinGW toolchain; Linux: the corresponding GCC kit).
3. Configure the project. For release-candidate checks, choose a **Release** configuration; use a separate Debug build when investigating an issue.
4. Build the `TraceScope` target. The root project also exposes the generator, launcher and individual test targets.
5. Run `TraceScope` through Qt Creator so the selected kit supplies the expected Qt runtime environment.

If Qt Creator reports that `Qt6Charts` is missing, install the Charts module for the **same** Qt version/kit and reconfigure. Do not work around it by silently disabling features; Charts is a required project component.

## 3. Windows command-line build

Open a terminal with **CMake, Ninja and the matching MinGW tools** available on `PATH`. Use either a shell environment provided by your Qt/MinGW installation or a terminal configured for that exact kit; do not mix an unrelated system `g++` with a Qt build compiled using a different toolchain.

The following PowerShell example uses a generic Qt installation path; **replace the path** with the installed kit directory that contains `lib/cmake/Qt6`:

```powershell
# Run from the repository root
$qt = 'C:\Qt\<your-Qt-version>\mingw_64'  # Replace with your actual kit path

cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="$qt" `
  -DCMAKE_CXX_COMPILER=g++.exe

cmake --build build --parallel
```

For a single-configuration Ninja build, the development executable is normally `build/TraceScope.exe`. Run it with Qt Creator or with the matching Qt runtime on the library/plugin search path. Copying `TraceScope.exe` alone to another Windows machine is **not** a portable deployment: the needed Qt and compiler runtime dependencies must accompany it.

If you change the compiler or Qt family, use a **fresh build directory** rather than reusing cached toolchain locations.

## 4. Linux command-line build

Use a 64-bit GCC toolchain, CMake, and a compatible Qt 6 installation containing the required components. The CI reference environment is Ubuntu **22.04** with Qt **6.10.3**; installing distribution-provided packages may result in a different version, so verify dependencies rather than assuming parity.

For a Qt installation that CMake cannot discover automatically, provide its prefix explicitly:

```sh
# Run from the repository root; adjust this example path
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/<your-Qt-version>/gcc_64"

cmake --build build --parallel
./build/TraceScope
```

If your Qt packages are correctly exposed to CMake by the system, omit `CMAKE_PREFIX_PATH`. If the application fails to launch because a Qt platform plugin or display backend cannot initialize, check the active Qt library/plugin environment and that a graphical desktop session is available. The CI offscreen setting is appropriate for automated smoke tests, not normal interactive usage.

## 5. Tests and development utilities

### Run the registered test suite

The root CMake build configures the Qt Test programs automatically. From a configured project:

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

To discover/select test executables, use `ctest --test-dir build -N` and `ctest --test-dir build -R 'InvestigationSessionTests' --output-on-failure`. Headless execution can use `QT_QPA_PLATFORM=offscreen`, matching CI; see [Testing](testing.md) for platform-specific commands, test organization and release verification.

### Build the standalone Live-Log Generator

The same root CMake project includes two additional targets:

```sh
cmake --build build --target TraceScopeLiveLogGenerator
cmake --build build --target TraceScopeLiveLogGeneratorLauncher
```

The first is a Qt Core command-line executable. The second is a Qt Widgets launcher that invokes the CLI; it is **not** a separate TraceScope ingestion path. Consult [Live-Log Generator](live-log-generator.md) for scenario format, command-line flags and the supported renderer matrix. Its versioned scenarios reside under `samples/live/`.

For local Windows convenience deployment, the generator subproject defines an **explicit** target (not part of the normal build):

```powershell
cmake --build build --target TraceScopeLiveLogGeneratorPortable
```

The target assembles a generator/launcher directory using the active kit's `windeployqt`. This is a development convenience target, not a replacement for the application's normal platform-specific release pipeline.

## 6. Deployment and troubleshooting

### Developer build versus release artifact

Successful compilation is not proof that a build can run on another computer. TraceScope's reviewed [CI workflow](../.github/workflows/ci.yml) separately produces and checks three candidate artifacts:

- A portable Windows x64 ZIP, assembled with `windeployqt` and compiler-runtime dependencies, with a short startup smoke test after extraction.
- A Linux x86_64 AppImage, built using pinned `linuxdeploy` tooling, with AppDir file checks and an offscreen packaged startup smoke test.
- A platform-neutral ZIP containing repository samples and matching profiles.

The packaging recipes contain versioned names tied to `v0.16.0` in the reviewed tree. **Do not reuse those filenames or claim v1.0 packaging verification** until the workflow is updated and run against the final candidate. The workflow is the authoritative packaging recipe; this document is not an independent manual AppImage-packaging specification.

### Common build problems

| Symptom | What to check |
| --- | --- |
| CMake cannot find `Qt6Config.cmake` or Qt components | Confirm Qt is installed for the active compiler, set the correct `CMAKE_PREFIX_PATH`, and install the missing Qt modules. |
| `Qt6Charts` is missing | Install Qt Charts for the selected Qt version/kit, then clear/reconfigure the build if necessary. |
| Compiler/ABI or linker errors on Windows | Verify the actual `g++.exe` is from the matching MinGW toolchain, not a different installation earlier on `PATH`. |
| Executable runs in Qt Creator but not in a normal terminal | The Qt runtime/library and platform-plugin environment differs. Launch with the matching kit or use the verified portable deployment process. |
| A GUI test fails on a headless machine | Reproduce with `QT_QPA_PLATFORM=offscreen`; inspect the actual failure if the platform initializes successfully. |
| Build settings seem stale after switching kits | Remove or replace the old *build directory*, not any source files, and reconfigure using the intended kit. |
| The final executable builds but packaging fails | Inspect the packaging step's required-file and startup checks; a source build and portable distribution are different verification stages. |

For source-specific runtime problems, consult [Troubleshooting](troubleshooting.md). For changes affecting importer, state, or window ownership, see [Architecture](architecture.md). For performance evidence and its limits, see [Performance Notes](performance.md).

---

**Release follow-up:** Refresh this guide after finalizing the v1.0 CMake/project version, supported development toolchains, platform packaging and smoke tests. Avoid recording provisional CI configuration or planned generator distribution as already-released capabilities.
