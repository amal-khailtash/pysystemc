import argparse
import os
import pathlib
import sys

os.environ.setdefault("EXTRA_CLING_ARGS", "-std=c++17")

import cppyy

__version__ = "0.1.0"

SYSTEMC_HOME = os.environ.get("SYSTEMC_HOME", str(pathlib.Path(__file__).resolve().parents[1] / "submodules/systemc"))


def load_systemc():
    install = pathlib.Path(os.environ.get("SYSTEMC_INSTALL_DIR",
        str(pathlib.Path(__file__).resolve().parents[1] / "build/deps/systemc-install")))
    prefixes = [install, pathlib.Path(SYSTEMC_HOME)]
    headers = next(prefix / "include" for prefix in prefixes
                   if (prefix / "include/systemc").is_file())
    cppyy.add_include_path(str(headers))
    override = os.environ.get("SYSTEMC_LIBRARY_DIR")
    directories = ([pathlib.Path(override)] if override else
                   [prefix / name for prefix in prefixes
                    for name in ("lib", "lib64", "lib-linux64")])
    library = next(directory / "libsystemc.so" for directory in directories
                   if (directory / "libsystemc.so").is_file())
    cppyy.add_library_path(str(library.parent))
    cppyy.load_library(str(library))

    cppyy.cppdef("#define SC_INCLUDE_DYNAMIC_PROCESSES")
    cppyy.include("systemc.h")


def run_sysc_cpp(source_files):
    print("-" * 80)
    print("Running SystemC C++ code ...")

    for src in source_files:
        if src == "++":
            break

        print(f"  Reading '{src}' ...")
        source_path = pathlib.Path(src)
        cppyy.add_include_path(str(source_path.parent))

        with open(src, "r") as fd:
            cppyy.cppdef(fd.read())

    argc = len(sys.argv) - len(source_files)
    argv = [sys.argv[0]]
    print("-" * 80)
    print(f"Running sc_main ({argc}, {argv}) ...")
    print("-" * 80)
    cppyy.gbl.sc_main(argc, argv)


def main():
    parser = argparse.ArgumentParser(description="Run SystemC C++ code", prog="run_sysc_cpp")
    parser.add_argument("--version", action="version", version=f"%(prog)s {__version__}")
    parser.add_argument("source", nargs="+")
    args = parser.parse_args()

    load_systemc()

    run_sysc_cpp(args.source)


if __name__ == "__main__":
    main()
