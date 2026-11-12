import subprocess, sys
binary = sys.argv[1]
for args, success in [(["--help"], True), (["--full-help"], True), (["--version"], True),
                       (["--invalid"], False), (["-or", "-1x480"], False),
                       (["-or", "1920x"], False), (["-of", "invalid"], False),
                       (["-o"], False)]:
    result = subprocess.run([binary, *args], capture_output=True, timeout=3)
    assert (result.returncode == 0) == success, (args, result.stderr)
    assert not result.stdout, (args, "diagnostics leaked to stdout")
