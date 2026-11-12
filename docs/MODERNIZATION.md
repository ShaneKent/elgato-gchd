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
