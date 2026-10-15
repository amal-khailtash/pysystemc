# Small SystemC examples

These original examples demonstrate introductory topics covered by
[ASIC World](https://www.asic-world.com/examples/systemc/index.html) and
[Learn SystemC](https://www.learnsystemc.com/). They do not copy tutorial code.

- `event_cancel.py`: event notification/cancellation, static sensitivity,
  `dont_initialize`, and timed thread waits.
- `mixed_gate.py`: a C++ AND gate with ports connected to Python-owned signals;
  Python drives and verifies every input combination.

Run each in its own interpreter with `uv run examples/toy/<filename>.py`.
Each example checks values and simulation times and exits after a finite run.
