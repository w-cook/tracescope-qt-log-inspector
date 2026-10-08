# Building TraceScope from Source

This guide covers local source builds of the TraceScope desktop application, its automated test programs, and the standalone Live-Log Generator CLI and launcher. It describes the repository's supported CMake/Qt build structure and the environments used for continuous integration.

For release packaging, the repository workflow is the authoritative recipe. A successful local source build and a verified portable release artifact are related but separate results.

## Find the information you need

| I want to... | Go to |
| --- | --- |
| Check build prerequisites and reference environments | [1. Requirements](#1-requirements) |
| Configure and build with Qt Creator | [2. Qt Creator build](#2-qt-creator-build) |
| Build from the Windows command line | [3. Windows command-line build](#3-windows-command-line-build) |
| Build from the Linux command line | [4. Linux command-line build](#4-linux-command-line-build) |
| Run tests or build the Live-Log Generator | [5. Tests and development utilities](#5-tests-and-development-utilities) |
| Understand release packaging or diagnose build problems | [6. Deployment and troubleshooting](#6-deployment-and-troubleshooting) |

## 1. Requirements

The root [`CMakeLists.txt`](../CMakeLists.txt) defines TraceScope as a **C++17** project requiring **CMake 3.21 or later** and **Qt 6**.

The configured build requires these Qt components:

- Qt Core
- Qt Widgets
- Qt Charts
- Qt Concurrent
- Qt Test

Qt Core is used throughout the application and by the Live-Log Generator CLI. TraceScope itself uses Widgets, Charts, and Concurrent. Qt Test is required for the repository's automated test programs, which are configured as part of the root CMake project.

The root project includes:

- the `TraceScope` desktop application
- the standalone `TraceScopeLiveLogGenerator` CLI
- the `TraceScopeLiveLogGeneratorLauncher` Qt Widgets launcher
- the registered automated test programs under `tests/`

### Reference environments

The v1.0 continuous-integration workflow provides repeatable validation on:

| Platform | Reference environment |
| --- | --- |
| Windows x64 | GitHub-hosted Windows, Qt 6.10.3, Qt-compatible 64-bit MinGW, CMake, and Ninja |
| Linux x86_64 | Ubuntu 22.04, Qt 6.10.3, GCC, and CMake |

These are **validated reference environments**, not additional CMake requirements. Other compatible Qt 6 desktop kits and C++17 toolchains may build the project, but they are not established by the CI matrix merely because CMake can configure them.

Use a **matching 64-bit Qt installation and compiler**. In particular, do not combine a Qt build produced for one compiler or ABI with an unrelated compiler from another installation.

Qt Charts must be installed for the same Qt version and kit used to configure the project.

### Clone the repository

```sh
git clone https://github.com/w-cook/tracescope-qt-log-inspector.git
cd tracescope-qt-log-inspector
```

To reproduce the v1.0 release source exactly:

```sh
git checkout v1.0.0
```

Normal development can instead use the current branch being changed.

## 2. Qt Creator build

Qt Creator is the simplest development path when the installed Qt kit already provides the correct compiler, CMake integration, and required Qt modules.

1. Open the repository's root `CMakeLists.txt` in Qt Creator.
2. Select a **64-bit Qt 6 desktop kit** with Qt Charts installed.
3. Configure the project.
4. Choose the desired build configuration:
   - **Debug** for debugging and development diagnostics
   - **Release** for optimized builds, release-like behavior, or preparation for packaging verification
5. Build the `TraceScope` target.
6. Run `TraceScope` through the configured kit.

The same CMake project exposes the Live-Log Generator, launcher, and individual test targets, so a separate project configuration is not required for those components.

### Changing kits

CMake caches compiler and Qt discovery information in the build directory. After switching to an incompatible Qt installation or compiler family, use a new build directory or clear the existing CMake configuration before reconfiguring.

If Qt Creator reports that `Qt6Charts` is unavailable, install the Charts module for the **same Qt installation and kit** rather than disabling the feature. Charts is part of the required application build.

## 3. Windows command-line build

Use a terminal where CMake, Ninja, and the compiler associated with the selected Qt kit are available.

Before configuring, ensure that the selected Qt kit's compatible MinGW `bin` directory is on `PATH` and takes precedence over unrelated compiler installations. The `g++.exe` selected by CMake must match the compiler ABI expected by the Qt installation. If multiple MinGW toolchains are installed, provide the matching compiler's full path rather than relying on `PATH` ordering.

The following PowerShell example uses a placeholder Qt installation path. Replace it with the directory for the installed 64-bit kit that contains `lib/cmake/Qt6`.

```powershell
# Run from the repository root.

$qt = 'C:\Qt\<Qt-version>\<kit-directory>'

cmake -S . -B build -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="$qt" `
  -DCMAKE_CXX_COMPILER=g++.exe

cmake --build build --parallel
```

If the selected Qt/MinGW environment already exposes the correct Qt package location to CMake, an explicit `CMAKE_PREFIX_PATH` may not be necessary.

For the single-configuration Ninja generator, the application executable is produced in the configured build tree. Running that executable locally still requires the matching Qt runtime and platform plugins to be discoverable.

### Windows toolchain consistency

A common source of Windows build failures is selecting a valid Qt installation but invoking a different `g++.exe` earlier on `PATH`.

Check the active tools when diagnosing configuration or linker problems:

```powershell
cmake --version
g++ --version
Get-Command g++ | Select-Object -ExpandProperty Source
qmake --version
```

The compiler reported by the shell should be compatible with the Qt kit supplied to CMake.

A locally built `TraceScope.exe` by itself is **not** a portable Windows distribution. Qt libraries, platform plugins, and compiler-runtime dependencies are assembled separately by the release packaging workflow.

## 4. Linux command-line build

Use a 64-bit C++17-capable GCC toolchain, CMake 3.21 or later, and a compatible Qt 6 installation containing the required modules.

The CI reference environment is Ubuntu 22.04 with Qt 6.10.3, but the local commands do not depend on that exact Qt minor version.

If CMake does not discover Qt automatically, provide the Qt prefix explicitly:

```sh
# Run from the repository root.
# Adjust the Qt path for the installed kit.

cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/<Qt-version>/gcc_64"

cmake --build build --parallel
./build/TraceScope
```

If the system or shell environment already exposes the correct Qt installation to CMake, omit `CMAKE_PREFIX_PATH`.

A Debug configuration can be created in a separate build directory:

```sh
cmake -S . -B build-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="$HOME/Qt/<Qt-version>/gcc_64"

cmake --build build-debug --parallel
```

### Linux runtime environment

Normal application use requires a graphical desktop environment and a working Qt platform plugin.

If the executable builds but fails during startup with a platform-plugin or display error, check:

- which Qt libraries are being loaded
- whether the correct platform plugins belong to that Qt installation
- whether the current environment has access to a graphical session
- whether system libraries required by the Qt platform backend are installed

`QT_QPA_PLATFORM=offscreen` is used for automated tests and smoke tests. It is not the normal interactive runtime configuration.

## 5. Tests and development utilities

### Run the automated test suite

The root CMake project enables testing and configures the registered CTest test programs declared in [`tests/CMakeLists.txt`](../tests/CMakeLists.txt).

After configuring and building:

```sh
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

List the registered CTest test programs:

```sh
ctest --test-dir build -N
```

Run a selected group:

```sh
ctest --test-dir build \
  -R 'LiveSessionFollowCoordinatorTests|LiveFileFollowerTests' \
  --output-on-failure
```

GUI-dependent automated tests can run headlessly using the same offscreen Qt platform used by CI.

```sh
QT_QPA_PLATFORM=offscreen \
  ctest --test-dir build --output-on-failure
```

PowerShell:

```powershell
$env:QT_QPA_PLATFORM = 'offscreen'
ctest --test-dir build --output-on-failure
```

See [Testing](testing.md) for the test organization, focused-test examples, manual scenario verification, and release-level acceptance checks.

### Build the Live-Log Generator

The Live-Log Generator is part of the root CMake project but remains a separate application from TraceScope.

Build the CLI:

```sh
cmake --build build --target TraceScopeLiveLogGenerator
```

Build the Qt Widgets launcher:

```sh
cmake --build build --target TraceScopeLiveLogGeneratorLauncher
```

The launcher invokes the standalone generator; it is not an alternate TraceScope ingestion path. TraceScope observes only the physical files produced by the utility.

See [Live-Log Generator](live-log-generator.md) for the scenario schema, renderer families, command-line options, launcher workflow, and example scenarios under `samples/live/`.

### Windows generator convenience deployment

On Windows, the generator subproject also provides an explicit development deployment target:

```powershell
cmake --build build --target TraceScopeLiveLogGeneratorPortable
```

This target:

- builds the generator CLI
- builds the Qt Widgets launcher
- creates a portable staging directory
- runs the active Qt installation's `windeployqt`
- includes the required compiler/runtime dependencies

The target is intentionally **not** part of the normal default build, so routine TraceScope development does not pay the deployment cost on every compile.

This local convenience target is separate from the versioned Live-Log Generator packages produced and verified by the release workflow.

## 6. Deployment and troubleshooting

### Developer build versus release artifact

A successful source build establishes that the selected source revision compiled successfully with the configured toolchain and dependencies. It does not, by itself, verify automated tests, interactive application behavior, or deployment completeness. It does not establish that the executable is portable to another machine.

The v1.0 [GitHub Actions workflow](../.github/workflows/ci.yml) is configured to build, package, and verify the following release-oriented artifacts:

- **Windows x64 TraceScope package** — portable ZIP assembled with `windeployqt` and compiler-runtime dependencies
- **Linux x86_64 TraceScope package** — AppImage assembled using pinned `linuxdeploy` tooling
- **Windows Live-Log Generator package** — standalone generator and launcher convenience distribution
- **Linux Live-Log Generator package** — standalone generator and launcher convenience distribution
- **Samples package** — platform-neutral archive containing the repository's sample logs and matching profiles

The application packages include the relevant bundled samples and documentation files.

During CI execution, the workflow checks required deployment files, performs startup smoke tests against the assembled packages rather than only build-tree executables, and uploads the resulting artifacts when the relevant steps succeed.

The Windows application package is extracted before its smoke test. The Linux AppImage is launched from the packaged artifact with the offscreen Qt platform for CI startup verification.

Release package names carry the v1.0 version identity. The workflow itself is the authoritative source for exact artifact names, deployment contents, pinned packaging tools, and release automation.

### Why release packaging is not documented as manual copy steps

Windows Qt deployment requires the appropriate Qt libraries, plugins, and compiler runtime. Linux AppImage construction similarly depends on the packaging toolchain and runtime contents.

Duplicating those recipes as hand-maintained shell instructions here would create a second packaging specification that could drift from CI. Contributors who need the release process should inspect [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) and the files under `packaging/`.

For ordinary development, build and run from the configured Qt environment instead.

### Common build problems

| Symptom | What to check |
| --- | --- |
| CMake cannot find `Qt6Config.cmake` | Confirm that the intended Qt kit is installed and discoverable, or provide its prefix with `CMAKE_PREFIX_PATH`. |
| CMake finds Qt but reports `Qt6Charts` missing | Install Qt Charts for the same Qt version and compiler kit, then reconfigure. |
| CMake reports another required Qt component missing | Verify that the selected Qt installation contains Widgets, Charts, Concurrent, and Test rather than mixing partial installations. |
| Windows linker or ABI errors | Confirm that the active `g++.exe` belongs to the MinGW toolchain compatible with the selected Qt kit. |
| Build behaves strangely after switching Qt kits or compilers | Configure into a fresh build directory so stale CMake cache entries do not point to the old toolchain. |
| Application runs in Qt Creator but not from a normal terminal | The runtime environment is not finding the same Qt libraries/plugins. Run from the correct kit environment or use the verified packaged distribution. |
| Linux application reports a Qt platform-plugin/display error | Verify the graphical session, Qt platform plugins, and required system libraries for the selected Qt build. |
| GUI-dependent tests fail on a headless host | Retry with `QT_QPA_PLATFORM=offscreen` and then inspect any remaining test failure normally. |
| `TraceScopeLiveLogGeneratorPortable` is unavailable | That convenience deployment target is Windows-specific; build the CLI and launcher targets normally on other platforms. |
| Application compiles but packaging fails | Treat packaging as a separate verification stage. Inspect the workflow's missing-file, deployment, and startup-smoke-test failure rather than assuming the source build proves the package is complete. |

For application/runtime symptoms after a successful build, see [Troubleshooting](troubleshooting.md). For subsystem ownership and extension points, see [Architecture](architecture.md). For the automated and manual verification model, see [Testing](testing.md). For measured resource behavior, see [Performance Notes](performance.md).
