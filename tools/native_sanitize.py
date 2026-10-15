"""Run optional instrumented native references without modifying vendored sources."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main() -> None:
    """Compile and execute ASan/UBSan references with subprocess timeouts."""
    root = Path(__file__).resolve().parents[1]
    prefix = Path(os.environ.get("SYSTEMC_INSTALL_DIR", root / "build/deps/systemc-install"))
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    with tempfile.TemporaryDirectory() as directory:
        for stem in ("datatype_extensions", "channel_extensions"):
            executable = Path(directory) / stem
            subprocess.run(
                [
                    *compiler,
                    "-std=c++17",
                    "-DSC_INCLUDE_FX",
                    "-fsanitize=address,undefined",
                    "-fno-omit-frame-pointer",
                    "-g",
                    str(root / "tests/native" / f"{stem}.cpp"),
                    "-I" + str(prefix / "include"),
                    "-L" + str(prefix / "lib"),
                    "-Wl,-rpath," + str(prefix / "lib"),
                    "-lsystemc",
                    "-pthread",
                    "-o",
                    str(executable),
                ],
                check=True,
                timeout=60,
            )
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=60)
            assert result.returncode == 0, result.stdout + result.stderr
            assert stem.upper() + "_OK" in result.stdout, result.stdout
    print("Native ASan/UBSan references passed (dependency library is not instrumented)")


if __name__ == "__main__":
    main()
