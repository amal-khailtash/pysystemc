"""PEP 517 backend: build native dependencies, then package through uv_build."""

from pathlib import Path
import os
import shutil
import subprocess
import sysconfig
import tarfile
import zipfile
import csv
import hashlib
import base64
import io
import uv_build

ROOT = Path(__file__).resolve().parents[1]
NATIVE = ROOT / "build/package-native"
VERSIONS = {"fmt": "11.2.0", "spdlog": "1.15.3", "systemc": "2.3.4"}


def _run(*args):
    """Execute a native build command and propagate failures."""
    subprocess.run(args, check=True)


def _source(name):
    """Fetch a pinned source archive into the local build cache."""
    version = VERSIONS[name]
    cache = ROOT / "build/deps"
    cache.mkdir(parents=True, exist_ok=True)
    directory = cache / f"{name}-{version}"
    if not directory.exists():
        repositories = {"fmt": "fmtlib/fmt", "spdlog": "gabime/spdlog", "systemc": "accellera-official/systemc"}
        tag = f"v{version}" if name == "spdlog" else version
        archive = cache / f"{name}-{version}.tar.gz"
        if not archive.exists():
            _run(
                "curl",
                "--fail",
                "--location",
                "--retry",
                "3",
                f"https://github.com/{repositories[name]}/archive/refs/tags/{tag}.tar.gz",
                "--output",
                str(archive),
            )
        with tarfile.open(archive) as stream:
            stream.extractall(cache, filter="data")
    return directory


def _native():
    """Compile/install fmt, spdlog and pthreads SystemC into package resources."""
    NATIVE.mkdir(parents=True, exist_ok=True)
    for name in VERSIONS:
        source = _source(name)
        build = ROOT / "build/deps" / f"package-{name}"
        options = [
            f"-DCMAKE_INSTALL_PREFIX={NATIVE}",
            "-DCMAKE_INSTALL_LIBDIR=lib",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DCMAKE_CXX_STANDARD=17",
            "-DCMAKE_POSITION_INDEPENDENT_CODE=ON",
            "-DCMAKE_SKIP_RPATH=ON",
            "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        ]
        if name == "systemc":
            options += ["-DENABLE_PTHREADS=ON", "-DBUILD_SHARED_LIBS=ON"]
        elif name == "fmt":
            options += ["-DFMT_TEST=OFF", "-DFMT_DOC=OFF"]
        else:
            options += [
                "-DSPDLOG_FMT_EXTERNAL=ON",
                f"-DCMAKE_PREFIX_PATH={NATIVE}",
                "-DSPDLOG_BUILD_TESTS=OFF",
                "-DSPDLOG_BUILD_EXAMPLE=OFF",
            ]
        _run("cmake", "-S", str(source), "-B", str(build), *options)
        _run("cmake", "--build", str(build), "-j", os.environ.get("BUILD_JOBS", "4"))
        _run("cmake", "--install", str(build))
        notices = NATIVE / "licenses" / name
        notices.mkdir(parents=True, exist_ok=True)
        for file in ("LICENSE", "LICENSE.rst", "NOTICE"):
            if (source / file).exists():
                shutil.copy2(source / file, notices / file)
        if name == "systemc":
            target = NATIVE / "src/sysc/kernel"
            target.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source / "src/sysc/kernel/sc_main_main.cpp", target)


def build_wheel(wheel_directory, config_settings=None, metadata_directory=None):
    """Build native resources and emit a platform-specific wheel with uv_build."""
    _native()
    original = uv_build.build_wheel(wheel_directory, config_settings, metadata_directory)
    path = Path(wheel_directory) / original
    tag = "py3-none-" + sysconfig.get_platform().replace("-", "_").replace(".", "_")
    target = path.with_name(original.replace("py3-none-any", tag))
    with zipfile.ZipFile(path) as archive:
        # Directory entries are ZIP structure, not installed files. Including
        # them in RECORD makes auditwheel attempt to read a directory as ELF.
        files = {name: archive.read(name) for name in archive.namelist() if not name.endswith(("/", "/RECORD"))}
    for resource in NATIVE.rglob("*"):
        if resource.is_file():
            # Wheels have no portable symlink installation contract. Store the
            # SystemC payload once under the name used by runtime discovery;
            # read_bytes follows the installation's unversioned symlink.
            if resource.parent == NATIVE / "lib" and resource.name.startswith("libsystemc.so."):
                continue
            # Installed CMake/pkg-config files refer to the build prefix and
            # are unnecessary for cppyy's runtime header/library discovery.
            if any(part in ("cmake", "pkgconfig") for part in resource.relative_to(NATIVE).parts):
                continue
            files["amal/eda/systemc/_native/" + resource.relative_to(NATIVE).as_posix()] = resource.read_bytes()
    for name in files:
        if name.endswith("/WHEEL"):
            files[name] = (
                files[name]
                .replace(b"Root-Is-Purelib: true", b"Root-Is-Purelib: false")
                .replace(b"py3-none-any", tag.encode())
            )
    records = io.StringIO()
    writer = csv.writer(records, lineterminator="\n")
    for name, data in files.items():
        digest = base64.urlsafe_b64encode(hashlib.sha256(data).digest()).rstrip(b"=").decode()
        writer.writerow([name, "sha256=" + digest, len(data)])
    record = next(name.rsplit("/", 1)[0] + "/RECORD" for name in files if name.endswith("/WHEEL"))
    writer.writerow([record, "", ""])
    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as archive:
        for name, data in files.items():
            archive.writestr(name, data)
        archive.writestr(record, records.getvalue())
    path.unlink()
    return target.name


def build_sdist(sdist_directory, config_settings=None):
    """Ship portable source/build instructions; compile on wheel construction."""
    return uv_build.build_sdist(sdist_directory, config_settings)


build_editable = uv_build.build_editable
prepare_metadata_for_build_wheel = uv_build.prepare_metadata_for_build_wheel
get_requires_for_build_wheel = uv_build.get_requires_for_build_wheel
get_requires_for_build_sdist = uv_build.get_requires_for_build_sdist
get_requires_for_build_editable = uv_build.get_requires_for_build_editable
