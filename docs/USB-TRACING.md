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

## Broader capture result and next capture

The subsequent 20-second native startup trace enabled UCX Default,
HeadersBusTrace, PartialDataBusTrace and FullDataBusTrace (0x1c1), with
USBXHCI also using those keywords plus Rundown (0x81c1), and USBHUB3 using
headers/default/rundown (0x8041). UCX event-ID filtering included control
submissions/completions/data (20–25), bulk/interrupt headers (26–27) and
standard/class/vendor request headers (34–62), excluding bulk/interrupt
payload events. No payload-value predicates were applied in this final run.

The trace contained 20,010 selected-card transfer events: 9,751 vendor control
submissions, 9,751 successful control completions, 274 bulk/interrupt
submissions and 234 completions. Outstanding reads at the end of the window
are not evidence of dropped video. UCX emitted completion event 21 version 1;
no control-data event 22 or 25 and no TransferData field was present anywhere
in the exported trace. The user's working official-app preview is the hardware
baseline; these counts alone do not establish a valid Linux recording.

Microsoft documents payload tracing, but enabling its keywords did not produce
payloads on this host. The reason is unconfirmed. Do not treat absent bytes as
zero-valued data, infer SCMD contents from headers, or keep repeating narrow ETW
captures. Use contemporaneous device descriptors: device handles changed after
handoff, so an older rundown can select zero events from a later trace.

For a payload-capable capture, collect one complete sequence for this card:
app startup, working preview, app shutdown, handoff to WSL and the failing Linux
startup. Keep all target-card control bytes, transfer statuses and timing;
include endpoint/firmware-upload metadata and the device's descriptors. This
covers both initialization and shutdown without guessing which six bytes will
matter later. Keep raw captures private and outside Git; firmware payloads must
never be published. Do not capture unrelated USB devices when the tool supports
a device filter.

USBPcap is a possible next tool, not an installed dependency. Its
[official installation instructions](https://desowin.org/usbpcap/) require a
Windows restart. Installation/restart needs separate user approval. An
independent native-Linux test is another way to isolate the USB/IP transport,
but does not replace the missing official Windows command payloads.
