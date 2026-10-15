import argparse
import os
import pathlib
import sys

os.environ.setdefault("EXTRA_CLING_ARGS", "-std=c++17")

import cppyy

__version__ = "0.1.0"

SYSTEMC_HOME = os.environ.get("SYSTEMC_HOME", str(pathlib.Path(__file__).resolve().parents[1] / "submodules/systemc"))


def load_systemc():
    """Load native SystemC and the external-fmt spdlog note adapter."""
    root = pathlib.Path(__file__).resolve().parents[1]
    for variable, default in (("FMT_HOME", "fmt-11.2.0"), ("SPDLOG_HOME", "spdlog-1.15.3")):
        prefix = pathlib.Path(os.environ.get(variable, str(root / "build/deps" / default)))
        if not (prefix / "include").is_dir():
            raise RuntimeError(f"{variable} headers missing; run make setup")
        cppyy.add_include_path(str(prefix / "include"))
    cppyy.add_include_path(str(root / "src/amal/eda/systemc/sc_core"))
    cppyy.include("runner_log.h")
    install = pathlib.Path(
        os.environ.get(
            "SYSTEMC_INSTALL_DIR", str(pathlib.Path(__file__).resolve().parents[1] / "build/deps/systemc-install")
        )
    )
    prefixes = [install, pathlib.Path(SYSTEMC_HOME)]
    headers = next(prefix / "include" for prefix in prefixes if (prefix / "include/systemc").is_file())
    cppyy.add_include_path(str(headers))
    override = os.environ.get("SYSTEMC_LIBRARY_DIR")
    directories = (
        [pathlib.Path(override)]
        if override
        else [prefix / name for prefix in prefixes for name in ("lib", "lib64", "lib-linux64")]
    )
    library = next(directory / "libsystemc.so" for directory in directories if (directory / "libsystemc.so").is_file())
    cppyy.add_library_path(str(library.parent))
    cppyy.load_library(str(library))

    cppyy.cppdef("#define SC_INCLUDE_DYNAMIC_PROCESSES")
    cppyy.include("systemc.h")


def run_sysc_cpp(source_files):
    """Compile sources and enter the native kernel startup wrapper."""
    cppyy.gbl.pysystemc.runner_note("Running SystemC C++ code")

    for src in source_files:
        if src == "++":
            break

        cppyy.gbl.pysystemc.runner_note(f"Reading {src}")
        source_path = pathlib.Path(src)
        cppyy.add_include_path(str(source_path.parent))

        with open(src) as fd:
            cppyy.cppdef(fd.read())

    # Recompile the pinned startup wrapper after defining the user's sc_main
    # so Cling resolves it to that JIT symbol, rather than a shared-library stub.
    startup = pathlib.Path(SYSTEMC_HOME) / "src/sysc/kernel/sc_main_main.cpp"
    cppyy.cppdef(startup.read_text())
    argc = 1
    argv = [sys.argv[0]]
    cppyy.gbl.pysystemc.runner_note(f"Starting sc_main with {argc} arguments: {argv}")
    sys.stdout.flush()
    cppyy.gbl.sc_core.sc_elab_and_sim.__release_gil__ = True
    return int(cppyy.gbl.sc_core.sc_elab_and_sim(argc, argv))


def main():
    parser = argparse.ArgumentParser(description="Run SystemC C++ code", prog="run_sysc_cpp")
    parser.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    parser.add_argument("source", nargs="+")
    args = parser.parse_args()

    load_systemc()

    return run_sysc_cpp(args.source)


if __name__ == "__main__":
    sys.exit(main())
