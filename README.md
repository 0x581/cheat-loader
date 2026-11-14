# ImGui Loader Base

A modular C++23 desktop loader base built with **Dear ImGui** (DirectX 9 backend), featuring **KeyAuth** licensing integration, a comprehensive **Anti-Reverse Engineering / Anti-Debugging** layer, and a local payload encryption pipeline.

---

## Overview & Features

- **Modern GUI**: Responsive UI powered by Dear ImGui with custom dark styling, DPI scaling, and custom list box components.
- **Licensing & Authentication**: KeyAuth SDK integration supporting license verification, session validity checks, and account validation.
- **Anti-Reverse Engineering Suite**:
  - Debugger detection (`IsDebuggerPresent`, `CheckRemoteDebuggerPresent`, `NtGlobalFlag`, debug heap flags, debug object handles, analysis driver/module checks).
  - Anti-dumping and binary stripping (IAT/EAT clearing, section header corruption, relocation erasing).
  - Breakpoint & timing inspection (hardware breakpoints, RDTSC/QPC timing anomalies).
  - Configurable reaction levels: `PROTECTION_EXIT`, `PROTECTION_CRASH`, `PROTECTION_BSOD`, or `PROTECTION_SILENT`.
- **Payload Encryption & Delivery**:
  - Standalone `Encryptor` CLI tool to encrypt executable binaries into `.enc` files.
  - In-memory decryption and temporary execution pipeline with PE validation.
- **Developer-Friendly Configuration**: Easy offline/testing toggle (`DISABLE_KEYAUTH_CHECK`) to inspect and develop the UI without an active KeyAuth plan.

---

## Project Structure

```text
├── antiRe/                # Anti-debug, anti-dump and reverse-engineering protections
│   ├── anti_debug.cpp     # Anti-debug implementations and heuristics
│   ├── anti_debug.h       # Detection headers and prototypes
│   ├── lazy.h             # Lazy importer for stealth API resolution
│   ├── protection.h       # Central protection loop and response triggers
│   └── AntiProtection_Doku.md # Detailed German documentation of security checks
├── import/                # Dependencies and shared headers
│   ├── defines.hpp        # UI constants, colors, and styling metrics
│   ├── globals.hh         # Application state, product structures, and session data
│   ├── Main.h             # DirectX 9 device management and window handling
│   └── lib/
│       ├── fonts/         # Bundled UI fonts
│       ├── imgui/         # Dear ImGui library and DX9/Win32 backends
│       └── keyauth/       # KeyAuth C++ SDK, json parser, and string encryptor
├── src/                   # Application source code
│   ├── Main.cpp           # Program entry point, protection init, and main loop
│   ├── crypto/            # Payload decryption and execution helpers
│   └── ui/                # ImGui interface views (Login, Main, Loading, Error)
├── tools/
│   └── Encryptor.cpp      # CLI utility to encrypt executables for loader delivery
├── CMakeLists.txt         # Root CMake build definition
└── CMakePresets.json      # CMake configuration and build presets
```

---

## Requirements

- **Operating System**: Windows 10 / Windows 11 (64-bit)
- **Compiler**: Visual Studio 2022 (v143 toolset) with C++23 support
- **Build System**: CMake 3.20 or newer
- **SDK**: Windows SDK (DirectX 9 / d3d9 components included in standard Windows SDK)
- **ATL**: Microsoft Foundation Classes (MFC) and C++ ATL component (installable via Visual Studio Installer)

---

## Building the Project

### Option 1: Using CMake Presets (Command Line)

```powershell
# Configure for x64 Release
cmake --preset x64-release

# Build x64 Release
cmake --build --preset x64-release
```

Compiled binaries will be generated in `build/bin/Release/`:
- `ImGuiLoaderBase.exe` (Main application)
- `Encryptor.exe` (Payload encryption tool)
- `fonts/` (Copied automatically post-build)

### Option 2: Visual Studio 2022

1. Open Visual Studio 2022.
2. Select **Open a local folder** and choose the repository root.
3. Select `x64-Release` from the configuration dropdown.
4. Go to **Build** -> **Build All**.

---

## Configuration Guide

### 1. KeyAuth Credentials

Open [src/Main.cpp](src/Main.cpp) and replace the placeholder credentials with your KeyAuth dashboard values:

```cpp
std::string name    = skCrypt("YOUR_APP_NAME").decrypt();      // KeyAuth Application Name
std::string ownerid = skCrypt("YOUR_OWNER_ID").decrypt();      // KeyAuth Account / Owner ID
std::string secret  = skCrypt("YOUR_APP_SECRET").decrypt();    // KeyAuth Application Secret
std::string version = skCrypt("1.0").decrypt();                // Application Version
std::string url     = skCrypt("https://keyauth.win/api/1.3/").decrypt();
```

### 2. Testing / Offline Mode

To test or modify the user interface without connecting to KeyAuth, set the flag in [src/Main.cpp](src/Main.cpp):

```cpp
#define DISABLE_KEYAUTH_CHECK true
```

When enabled, the application skips license validation and jumps directly to the main menu.

### 3. Protection Mode

You can configure how the application reacts when debugging tools or memory tampering are detected:

```cpp
#define PROTECTION_MODE PROTECTION_EXIT
```

Available options (defined in `antiRe/protection.h`):
- `PROTECTION_EXIT`: Immediately terminates the process safely (recommended default).
- `PROTECTION_CRASH`: Deliberately induces a crash via memory fault.
- `PROTECTION_BSOD`: Triggers a system bugcheck via `NtRaiseHardError` (use with caution).
- `PROTECTION_SILENT`: Emits debug strings without terminating (useful for development).

---

## Payload Management & Encryption

The loader dynamically looks for encrypted payloads in a `products/` subfolder located next to the executable:

1. **Build the Encryptor tool**:
   ```powershell
   cmake --build --preset x64-release --target Encryptor
   ```

2. **Encrypt your target executable**:
   ```powershell
   # Syntax: Encryptor.exe <input_executable> <output_enc_path>
   build\bin\Release\Encryptor.exe "YourProgram.exe" "build\bin\Release\products\cs2.enc"
   ```

3. **Product Definition**:
   Product entries (names, versions, status indicators) are defined in [src/ui/ui.cc](src/ui/ui.cc) inside `ui::renderMain()`. Match the `.name` field with your encrypted file name (e.g. `cs2` -> `products/cs2.enc`).

---

## Disclaimer

This software is provided strictly for **educational, security research, and demonstration purposes**. It demonstrates GUI rendering, software licensing architecture, and defensive anti-tamper techniques. The authors and contributors do not condone, promote, or take responsibility for any misuse of this codebase.
