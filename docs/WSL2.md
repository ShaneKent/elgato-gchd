# WSL 2 + usbipd-win recording

This targets the original Game Capture HD (`0fd9:0044`, `004e`, `0051`, `005d`).
USB enumeration was observed on Ubuntu 24.04 / WSL 2. A successful firmware load,
video/audio capture, restart, unplug handling and long recording with this fork
remain **unverified**. The device was disconnected before hardened-driver tests.

## Windows: once per installation/device

These are commands for you to run in Windows; the modernization task did not
execute Windows-side commands. Use an administrator PowerShell for installation
and binding. Replace `BUSID` with the current ID from `usbipd list` (the other
agent previously observed `9-4`, which may change):

```powershell
winget install --interactive --exact dorssel.usbipd-win
usbipd list
usbipd bind --busid=BUSID
```

Binding persists across reboots. Keep a WSL terminal open for the next step.
See Microsoft's [USB connection guide](https://learn.microsoft.com/en-us/windows/wsl/connect-usb)
and the [usbipd WSL guide](https://github.com/dorssel/usbipd-win/wiki/WSL-support).

## Windows: after reboot, replug, or WSL restart

Normal (non-administrator) PowerShell:

```powershell
usbipd list
usbipd attach --wsl --busid=BUSID
usbipd list
```

With supported versions, check `usbipd attach --help` and optionally use:

```powershell
usbipd attach --wsl --busid=BUSID --auto-attach
```

Auto-attach runs a foreground retry loop; keep its terminal open. It is not a
permanent service setting. Current usbipd implements an
[attach loop](https://github.com/dorssel/usbipd-win/blob/master/Usbipd/Wsl.cs).
While attached, Windows capture software cannot use the device. When done:

```powershell
usbipd detach --busid=BUSID
```

## WSL: verify USB and permissions

```sh
uname -r
lsusb -d 0fd9:
```

Use the [README](../README.md) to build, install firmware and configure udev.
WSL may have no running udev daemon or desktop seat, so `TAG+="uaccess"` alone
may not grant access. Reload rules before attaching if udev is available. If
reload reports that no daemon is running, your distro may support
`sudo service udev restart`; do not assume it is active.

For a narrow temporary fallback, find the actual bus/device number in lsusb:

```sh
# Example only: replace 001/002 with the numbers printed by lsusb.
sudo chgrp "$(id -gn)" /dev/bus/usb/001/002
sudo chmod 0660 /dev/bus/usb/001/002
```

This applies only to that device node and must be repeated after reattach or
re-enumeration. Avoid broad permissions on all USB nodes. Keep ffmpeg unprivileged.

## WSL: record and inspect

Prefer the Linux filesystem for builds and initial recording:

```sh
export GCHD_FIRMWARE_DIR="$HOME/.local/share/gchd/firmware"
build/src/gchd --duration 30s "$HOME/capture.ts"
ffprobe -v error -show_streams "$HOME/capture.ts"
scripts/gchd-record.sh "$HOME/capture.mp4" 10m
```

Or pipe directly, with shell pipeline failure propagation:

```sh
set -o pipefail
build/src/gchd --duration 90s - | ffmpeg -i - -c copy "$HOME/capture.mkv"
```

Stop with Ctrl+C once and wait. Disconnects should exit nonzero and cleanup
cannot throw; those paths are covered by USB fault mocks, not real unplug tests.
Stats appear every five seconds on stderr. Nonzero drop/overflow counts mean
loss; persistent timeouts with zero bytes do not establish a working stream.
If protocol readiness fails, capture stderr and try a physical power cycle;
do not repeatedly upload after an unresolved failure.

Normal firmware upload uses the already-open device and does not request a USB
reset; one attach is expected to suffice, but firmware re-enumeration behavior
is unverified. `gchd --reset` explicitly calls libusb_reset_device and may require
you to run the Windows attach command again. Hardware recovery is unverified.

## Hardware acceptance checklist

Still required on this fork: successful firmware load; 30-second capture with
ffprobe-confirmed video and audio; Ctrl+C followed by restart; real unplug during
streaming; and 30+ minutes over usbipd with loss counters and ffmpeg diagnostics.
If measurements implicate USB/IP round-trip latency, evaluate 4–8 async bulk-IN
transfers against FF-5's MIT-licensed experimental branch. Its existence is not
evidence of working capture. V4L2 and analog/mode-detection changes are deferred.
