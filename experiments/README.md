# Experiments

This directory preserves exploratory code outside the library and supported
examples. These probes document approaches and limitations; they are not public
APIs or release acceptance tests.

Run interface composition exploration from a configured checkout:

```sh
timeout 60s uv run python experiments/interfaces/interface_protocol_probe.py
```

The probe uses an explicit composition adapter and optional Protocol typing.
It checks native TLM transport/peek overloads, waits, callback exceptions,
implementation retention and missing-method rejection. The library has since
added automatic structural binding; this probe preserves the earlier design.
