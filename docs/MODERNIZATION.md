# Modernization report

Branch: `modernize`, based on upstream `e5bc6b9`.

## Phase 0 — setup

Isolated Linux checkout: `/home/shane/elgato-gchd-modernize`. The shared
Windows drive rejects Git chmod operations. Origin is ShaneKent/elgato-gchd;
upstream pushes are disabled. Reference remotes: ff5, eddyv73, upstream/dev.
Ubuntu 24.04, WSL 2 kernel 6.18.58.1, GCC 13.3, Clang 18.1, CMake 3.28,
libusb 1.0.27. Original Elgato 0fd9:0044 enumerated through lsusb.
Firmware binaries and downloaded installers are excluded from Git.

Hardware enumeration alone does not establish working firmware or capture.

## Phase 1 — build and CI

GCC 13 and Clang 18 strict builds pass, including Clang ASan/UBSan.
Regression suites cover CLI errors, resolution and address parsing, PAT/PMT/SIT
and descriptor generation. Constructor order, missing cstdint (credit PR #55),
ptr_fun, uninitialized state/descriptor fields, polymorphic destructors fixed.
CMake uses imported libusb, target options, install rules, and CTest.
Compared eddyv73 CMake (MIT); no fork code copied. CI gates correctness and
portability cppcheck diagnostics; performance suggestions are outside this gate.

## Phase 2 — USB robustness

Defects 1–4 fixed: initialized and checked streaming lengths/status, partial
timeout data retained, overflow data discarded, fatal errors propagated.
Explicit libusb context, auto detach/reattach, interface ownership tracked.
Only successful configuration sets initialized; partial configuration does not
attempt the full shutdown protocol. Cleanup never throws, skips removed devices,
and has a five-second budget plus at most one in-flight transfer.
Control, interrupt and firmware transfers use 1000 ms timeouts; polling loops
check cancellation with 15-second/2000-attempt limits. Firmware I/O and every
upload chunk are checked (no spurious zero-length final transfer).
Additive --reset performs a USB reset and exits; WSL may need reattach.
Mock fault tests cover timeout, overflow, I/O, disconnect, short/missing firmware,
interruption and non-throwing destruction. Hardware behavior remains unverified.

## Phase 3 — firmware

Added --firmware-dir (highest priority), GCHD_FIRMWARE_DIR, then system/Mac
paths. Explicit overrides are exclusive, and implicit current-directory search
is removed. Runtime verifies SHA-256 and exact length before each upload, on
the same open file. OpenSSL Crypto is now a build dependency.

Both approved official installers were downloaded over HTTPS and extracted
with 7z successfully. Eight versioned size/hash records are in
[firmware-hashes.json](firmware-hashes.json); H57/H58 files match across both
installers, M01 files differ. get-firmware.sh pins installer hashes, accepts
only the approved URLs or their exact local installers, verifies all four
files before installation, and was tested end-to-end for DMG and MSI.
The optional dmg2img fallback is untested. No firmware/installer is in Git.
