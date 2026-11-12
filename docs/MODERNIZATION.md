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

## Phase 4 — output safety

Defects 5–11 fixed. Explicit existing FIFO/file/symlink paths are preserved on
creation failure; only the created inode is unlinked. Default FIFO uses a
validated private XDG_RUNTIME_DIR or /tmp/gchd-UID directory with mode 0600,
O_NOFOLLOW and an advisory lock. Safe stale default FIFOs can be reused.
Writes handle partial writes/EINTR/EAGAIN; EPIPE, disk-full and UDP errors
propagate. UDP datagrams are never split on short writes. Socket connection
tries all addresses, frees addrinfo, and defaults to loopback as a destination.
Signal handlers only set sig_atomic_t; SIGHUP included, second signal _exit.
SIGPIPE disposition and masks are initialized. PID fd sentinel corrected.

A 256-buffer queue (at most 4 MiB plus in-flight buffers) and writer thread
separate synchronous USB reads from output latency. Saturation drops newest
buffers and counts losses; shutdown allows two seconds to drain before
cancelling nonblocking pipe/socket writes. A blocked kernel write to a regular
file on an unresponsive filesystem cannot be forcibly cancelled by this design.
Tests verify preserved files/symlinks, FIFO modes/cleanup, 1 MiB partial pipe
writes, EPIPE, /dev/full, bounded queue saturation/failure and SIGHUP.

## Phase 5 — CLI recording

Added gchd -, -o -, and -of stdout; only the selected sink is written.
Stdout is binary-only, rejects terminals, and restores pipe flags on close.
--duration accepts positive seconds or s/m/h suffixes and counts time after
initialization. Five-second stderr stats and final stats report USB/output
bytes, bitrate, dropped buffers/bytes, USB overflows and timeouts.
Help reflects runtime FIFO paths, hides disabled PID/frame-rate flags, and
adds input-no-stretch while preserving no-input-stretch. Numeric parsing
rejects non-finite/overflow values before unsafe casts. A related H.264 level
conversion swapped modf outputs; level 4.1 now converts to 41 correctly.
Tests include stdout byte fidelity and timed mocked streaming.

## Phase 6 — non-root use

Defect 12 fixed: udev appends elgato_gchd and uaccess tags, retaining the video
group/mode 0660 fallback. CMake installs the rule in an architecture-independent
lib/udev/rules.d directory; GCHD_UDEV_RULES_DIR can select /etc/udev/rules.d.
Actual seat ACL/device access remains unverified; WSL often lacks a desktop
seat or running udev. The WSL guide describes a narrow per-device permission
fallback and the need to reapply it after reattach.

## Phase 7 — documentation and recording wrapper

README and WSL2.md document build, official firmware extraction, non-root use,
new FIFO location, stdout/ffmpeg recording, reset and attach behavior. Windows
commands are separated into one-time admin bind and recurring non-admin attach.
No Windows commands were executed in this modernization task.

gchd-record.sh forwards stop signals as SIGINT to the driver, waits for EOF and
ffmpeg container finalization, refuses existing output, and checks both child
exit statuses. TS/MKV use stream copy; MP4/MOV preserve video and encode AAC audio.
Synthetic ffmpeg H.264/MPEG-audio fixtures verified MKV stream copy, MP4/AAC
output, stop forwarding and finalization, overwrite refusal, and cleanup after
a forced ffmpeg error. This is not a hardware capture test.

## Phase 8 — hardware acceptance remains outstanding

The original 0fd9:0044 enumerated during Phase 0 but was disconnected by the
time software hardening was complete. A second lsusb showed only root hubs.
The hardened driver was not run against the device. Firmware load, valid
30-second capture, real Ctrl+C/restart, unplug, reset recovery, permissions,
and 30+ minute USB/IP recording are all unverified.

The other agent shared upstream-only evidence: control EPIPE errors followed by
destructor aborts; after a power cycle, revision V1100090 was read but firmware
bulk upload hung. Its shared workspace logs were preserved and not modified.
That evidence does not isolate USB/IP from protocol/device compatibility and
does not demonstrate working capture.

Next: reattach/power-cycle with your Windows commands, then run the WSL guide's
acceptance sequence while collecting stderr and ffprobe output. Implement 4–8
async bulk-IN transfers only if measured latency/loss justifies them; no
experimental fork code has been copied and no async path is enabled here.

## Defect and verification summary

| Review defects | Result | Evidence |
| --- | --- | --- |
| 1–4 USB reads, cleanup, infinite waits, firmware I/O | Fixed in software | USB fault mocks, ASan/UBSan, static review |
| 5–6 FIFO ownership/default path | Fixed | Existing-file/symlink preservation, private mode/cleanup tests |
| 7–8 writes/backpressure | Fixed | Partial writes, EPIPE, /dev/full, queue saturation and bounded cancellation tests |
| 9 socket connection loop | Fixed | Reviewed address iteration and RAII cleanup; real network streaming unverified |
| 10–11 signals | Fixed | Initialized dispositions, SIGHUP and second-signal subprocess tests |
| 12 udev tags | Fixed | udevadm verify; real permissions unverified |

Additional fixes: explicit libusb context and automatic kernel-driver reattach;
firmware override/hash checks and no implicit cwd search; resolution/numeric
validation; matching CLI help and alias; PID descriptor sentinel.

Deferred/out of scope: V4L2/v4l2loopback, multi-card support, upstream dev merge,
analog fixes, HD60/HD60 S, mode detection, source changes on the fly, and
sub-480i inputs. The large protocol configuration class remains for later work.
