"""Port Accellera 16-point FFT source/transform/sink (Apache-2.0).

See upstream LICENSE/NOTICE. Inputs are working-directory in_real/in_imag;
outputs are out_real/out_imag. Native ports and guarded waits preserve the
upstream protocol, including the floating source's unusual data-valid loop.
"""

import ctypes
import math
from pathlib import Path
from amal.eda.systemc import sc_core as sc, sc_dt as dt


def f32(value):
    """Round a computation result to native IEEE single precision."""
    return ctypes.c_float(float(value)).value


def fixed(value, shift=0, sign=16):
    """Copy a native signed intermediate's sign and payload slice to sixteen bits."""
    value = int(value)
    bits = ((value >> shift) & 0x7FFF) | (((value >> sign) & 1) << 15)
    return dt.sc_int[16](bits)


def transform(samples, integer=False):
    """Perform the source DIF butterfly order with native numeric storage.

    Args:
        samples: Sixteen copied real/imaginary sample pairs.
        integer: Select signed fixed-point arithmetic and bit slicing.
    Returns:
        Sixteen complex pairs in source bit-reversed output order.
    """
    cast = dt.sc_int[16] if integer else f32
    values = [[int(cast(a)), int(cast(b))] if integer else [cast(a), cast(b)] for a, b in samples]
    weights = []
    real, imag = 1024 if integer else 1.0, 0
    theta = f32(f32(8 * f32(math.atan(1))) / 16)
    wr, wi = (942, -389) if integer else (f32(math.cos(theta)), f32(-math.sin(theta)))
    for _ in range(7):
        if integer:
            a = dt.sc_int[33](int(dt.sc_int[32](real * wr)) - int(dt.sc_int[32](imag * wi)))
            b = dt.sc_int[33](int(dt.sc_int[32](real * wi)) + int(dt.sc_int[32](imag * wr)))
            real, imag = int(fixed(a, 10, 32)), int(fixed(b, 10, 32))
        else:
            a = f32(f32(real * wr) - f32(imag * wi))
            imag = f32(f32(real * wi) + f32(imag * wr))
            real = a
        weights.append((real, imag))
    length, increment = 16, 1
    for _ in range(4):
        length //= 2
        for j in range(length):
            for i in range(j, 16, 2 * length):
                a, b = values[i], values[i + length]
                sums = [cast(a[k] + b[k]) for k in (0, 1)]
                if j == 0:
                    differences = [cast(a[k] - b[k]) for k in (0, 1)]
                elif integer:
                    dr = dt.sc_int[17](a[0] - b[0])
                    di = dt.sc_int[17](a[1] - b[1])
                    wr, wi = weights[j * increment - 1]
                    tr = dt.sc_int[34](int(dr) * wr - int(di) * wi)
                    ti = dt.sc_int[34](int(dr) * wi + int(di) * wr)
                    differences = [fixed(tr, 10, 33), fixed(ti, 10, 33)]
                    sums = [fixed(dt.sc_int[17](a[k] + b[k]), 0, 16) for k in (0, 1)]
                else:
                    dr, di = [f32(a[k] - b[k]) for k in (0, 1)]
                    wr, wi = weights[j * increment - 1]
                    differences = [f32(f32(dr * wr) - f32(di * wi)), f32(f32(dr * wi) + f32(di * wr))]
                values[i], values[i + length] = (
                    ([int(x) for x in sums], [int(x) for x in differences]) if integer else (sums, differences)
                )
        increment *= 2
    return [values[int(f"{i:04b}"[::-1], 2)] for i in range(16)]


class Process(sc.sc_module):
    """Own directional native ports and a single clocked callback."""

    def __init__(self, name, signals, directions, callback, clock, datatype):
        """Bind port directions before elaboration and retain the callback."""
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.clock = sc.sc_in[bool]("clock")
        self.clock.bind(clock)
        for key, direction in directions.items():
            port_type = sc.sc_in if direction == "in" else sc.sc_out
            port = port_type[datatype if "real" in key or "imag" in key else bool](key)
            port.bind(signals[key])
            setattr(self, key, port)
        self.sc_cthread("entry", callback, self.clock.pos())


def run(integer=False):
    """Run one upstream FFT variant using working-directory input/output files.

    Args:
        integer: Select the fixed-point source, protocol and transform.
    Returns:
        None after input exhaustion stops the native kernel and files close.
    Raises:
        OSError: Input files are absent or output files cannot be written.
        ValueError: An input token cannot be parsed in the native numeric format.
    """
    datatype = dt.sc_int[16] if integer else "float"
    clock = sc.sc_clock("CLOCK", 10, sc.SC_NS, 0.5, 0, sc.SC_NS)
    signals = {
        key: sc.sc_signal[datatype if "real" in key or "imag" in key else bool](key)
        for key in ("in_real", "in_imag", "out_real", "out_imag", "data_valid", "data_ack", "data_req", "data_ready")
    }
    parse = int if integer else f32
    inputs = [iter([parse(token) for token in Path(name).read_text().split()]) for name in ("in_real", "in_imag")]
    outputs = [Path(name).open("w") for name in ("out_real", "out_imag")]
    counts = [0, 0]

    def wait_until(predicate):
        """Preserve a native do/while wait: always suspend at least once."""
        while True:
            sc.wait()
            if predicate():
                return

    def source():
        """Feed samples on request and stop at the first exhausted component."""
        src.data_valid.write(False)
        while True:
            wait_until(lambda: src.data_req.read())
            try:
                src.in_real.write(next(inputs[0]))
                src.in_imag.write(next(inputs[1]))
            except StopIteration:
                print("End of Input Stream: Simulation Stops")
                sc.sc_stop()
                return
            counts[0] += 1
            src.data_valid.write(True)
            wait_until(lambda: not src.data_req.read())
            src.data_valid.write(False)
            sc.wait()

    def fft():
        """Read, compute and write each sixteen-sample frame in native order."""
        while True:
            fft_model.data_req.write(False)
            fft_model.data_ready.write(False)
            if integer:
                sc.wait()
            print("\nReading in the samples...")
            samples = []
            for _ in range(16):
                fft_model.data_req.write(True)
                wait_until(lambda: bool(fft_model.data_valid.read()) == integer)
                samples.append((fft_model.in_real.read(), fft_model.in_imag.read()))
                if integer:
                    samples[-1] = tuple(dt.sc_int[16](value) for value in samples[-1])
                fft_model.data_req.write(False)
                sc.wait()
            print("Computing...")
            values = transform(samples, integer)
            print("Writing the transform values...")
            for real, imag in values:
                fft_model.out_real.write(real)
                fft_model.out_imag.write(imag)
                fft_model.data_ready.write(True)
                wait_until(lambda: fft_model.data_ack.read())
                fft_model.data_ready.write(False)
                sc.wait()
            print("Done...")

    def sink():
        """Write all acknowledged output pairs with native numeric formatting."""
        dst.data_ack.write(False)
        while True:
            wait_until(lambda: dst.data_ready.read())
            for file, port in zip(outputs, (dst.out_real, dst.out_imag)):
                value = port.read()
                file.write(f"{int(value)}  \n" if integer else f"{value:e}  \n")
            counts[1] += 1
            dst.data_ack.write(True)
            wait_until(lambda: not dst.data_ready.read())
            dst.data_ack.write(False)

    fft_model = Process(
        "FFTPROCESS",
        signals,
        {key: "out" if key in ("out_real", "out_imag", "data_req", "data_ready") else "in" for key in signals},
        fft,
        clock,
        datatype,
    )
    src = Process(
        "SOURCEPROCESS",
        signals,
        {"in_real": "out", "in_imag": "out", "data_valid": "out", "data_req": "in"},
        source,
        clock,
        datatype,
    )
    dst = Process(
        "SINKPROCESS",
        signals,
        {"out_real": "in", "out_imag": "in", "data_ack": "out", "data_ready": "in"},
        sink,
        clock,
        datatype,
    )
    try:
        sc.sc_start()
        assert counts[0] > 0 and counts[1] > 0, counts
        assert sc.sc_end_of_simulation_invoked()
    finally:
        for file in outputs:
            file.close()


if __name__ == "__main__":
    run()
