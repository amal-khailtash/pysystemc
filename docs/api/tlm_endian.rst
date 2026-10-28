TLM endian conversion
=====================

Native helpers are available through ``amal.eda.systemc.tlm``. Select the native
data word type as a template parameter and pass the bus width in bytes::

   tlm.tlm_to_hostendian_generic["unsigned short"](payload, 4)
   # Perform the transaction using its converted fields and buffers.
   tlm.tlm_from_hostendian(payload)

Use these helpers inside an initiator whose endianness differs from the host.
They assume arithmetic mode: bytes within each data word use host order. Native
context is attached to the payload, so conversions must not be cascaded through
multiple bridges. Use the same native word type and bus width for explicit
request/response pairs. The generic ``tlm_from_hostendian(payload)`` dispatcher
uses the response function stored by the preceding request conversion.

The Python generic request helper preserves the full 64-bit address window.
SystemC 2.3.4 uses an unsigned-int mask in that request converter; the binding
converts a bus-relative address and then restores the payload and native response
context to the original window. The response converter and dispatcher continue
to use native buffer-copy behavior.

Converter preconditions
-----------------------

All bus and data word sizes must be powers of two. Native helpers do not provide
Python validation of these requirements.

* ``tlm_to_hostendian_generic[Word]`` supports unaligned addresses and repeating
  byte enables. Data length must be a whole number of effective streaming widths,
  and streaming width must contain a whole number of data words. It allocates
  converted data and enable buffers and can expand address windows.
* ``tlm_to_hostendian_word[Word]`` requires data words no wider than the bus,
  no streaming, and byte enables uniform within each word. If enables are
  present, their length must equal the data length.
* ``tlm_to_hostendian_aligned[Word]`` adds bus-aligned address and a whole number
  of bus words to the word converter requirements. Equal data/bus widths take
  the native no-conversion path.
* ``tlm_to_hostendian_single[Word]`` requires one data word that does not cross
  a bus boundary. Streaming and repeating enable lengths are unsupported;
  enables must be uniform within the word. It converts the address in place.

Each family has a corresponding ``tlm_from_hostendian_*[Word](payload, bus)``
response function. Always complete the pair before consuming read results.

Ownership and response state
----------------------------

Retain original native data and byte-enable storage through both conversions.
The payload context manages temporary converted buffers. Read responses copy
converted enabled bytes into original data storage. Disabled bytes keep their
previous values. Response helpers do not generally restore payload pointers,
address, data length or streaming width to their original request values; retain
that metadata separately if needed. The single-word response helper is a native
no-op, and equal-width aligned conversion preserves original pointers.

Regression evidence
-------------------

``tests/models/tlm_endian_generic.py`` checks read/write word types, bus offsets,
unaligned transfers, streaming windows and repeating enables. An independent C++
reference compares the combined unaligned streaming/enable case's actual buffers.
Aligned and word probes check ordering and word-uniform response masking. The
single-word probe has an independent 27-case native address reference.
``tests/models/tlm_endian_public.py`` compares all four request/response families
and dispatcher pairs against native buffer results for 384 read/write cases.
These cover masking, streaming, unaligned addresses, pointer-preserving paths,
data word sizes from one through sixteen bytes, and a custom native word type.
Additional context-reuse checks cross 32-bit and 64-bit address windows while
preserving original enable buffers and previously completed read storage.
