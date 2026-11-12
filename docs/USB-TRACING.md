# Windows USB diagnostics without a capture-driver installation

Windows ETW/Logman was used successfully to capture the original 0fd9:0044
card's startup headers on 2026-11-11. No USBPcap installation or Windows restart
was needed. The provider settings follow [Microsoft's USB tracing guide](https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/how-to-capture-a-usb-event-trace).

Run scripts/trace-windows-usb.ps1 from an administrator PowerShell, using a
fresh output directory accessible to that elevated account. Copy the script to
a local drive first if an elevated session cannot see a mapped network drive.
The default duration is 20 seconds, with a 32 MiB circular-file limit.

```powershell
.\trace-windows-usb.ps1 -OutputDirectory "$env:TEMP\gchd-usb-new" -DurationSeconds 20
```

Open the official Elgato app while the trace is running. The helper stops its
own unique session in finally and exports XML with tracerpt. It enables only
Default, HeadersBusTrace and Rundown; no packet payload keywords are enabled.
The raw trace contains controller-wide metadata and device descriptors. Keep
it private and outside Git. Output files are refused if already present.

Export only the selected card's transfer metadata in Linux:

```sh
python3 tools/usb_etw.py /path/to/usb-headers.xml > /tmp/elgato-headers.json
```

The parser requires a matching VID/PID descriptor, never falls back to exporting
all devices, omits OS/device pointers, and never exports bulk/interrupt payload.
For a separate trace without rundown, use --descriptors with a header trace
from the same unchanged device connection. Raw ETL/XML files are local evidence;
only reviewed, sanitized findings belong in the repository.

The observed native warm startup submitted 649648 encoder bytes to endpoint
0x02 successfully (19 x 32768 plus 27056 bytes). No idle-image upload was present
in that warm trace. Its first nine control request headers matched the WSL
startup. The first B8 system command succeeded natively and stalled under WSL.
Header-only tracing cannot compare the six data bytes in that command. This
difference is evidence to investigate, not a confirmed USB/IP root cause.
