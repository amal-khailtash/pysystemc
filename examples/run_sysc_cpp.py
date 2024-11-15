import argparse
import os
import pathlib
import sys

import cppyy

__version__ = "0.1.0"

SYSTEMC_HOME = os.environ["SYSTEMC_HOME"]


def load_systemc():
    cppyy.add_include_path(f"{SYSTEMC_HOME}/include")
    cppyy.add_library_path(f"{SYSTEMC_HOME}/lib-linux64")

    cppyy.load_library("systemc")
    # # cppyy.load_library(f"{SYSTEMC_HOME}/lib-linux64/libsystemc.so")
    # cppyy.load_reflection_info(f"{SYSTEMC_HOME}/lib-linux64/libsystemc.so")

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
