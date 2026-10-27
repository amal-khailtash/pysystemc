"""Preserve native FIFO overloads while guarding Python scheduler unwind."""

import cppyy

_bridges = []


class FifoTemplate:
    """Instantiate unrestricted native FIFO types with guarded blocking calls."""

    def __init__(self, template):
        """Retain the native template without precompiling value specializations."""
        self.template = template

    def __getitem__(self, datatype):
        """Return a native class whose blocking calls preserve reset/kill cleanup."""
        native = self.template[datatype]
        if getattr(native, "_guarded_fifo", False):
            return native
        return guard_fifo_class(native, datatype)

    def __call__(self, datatype):
        """Support alternate cppyy template specialization syntax."""
        return self[datatype]


def guard_fifo_class(native, datatype):
    """Guard a native FIFO specialization or borrowed blocking interface class."""
    if getattr(native, "_guarded_fifo", False):
        return native
    value = native.__cpp_name__.split("<", 1)[1].removesuffix(">")
    identifier = f"GuardedFifo{len(_bridges)}"
    methods = ""
    if hasattr(native, "read"):
        methods += f"""
      /** Guard the native value-returning read overload. */
      static std::pair<std::optional<{value}>, bool> read(sc_core::sc_fifo_blocking_in_if<{value}>& channel) {{
        return sc_core::python_fifo_read(channel);
      }}
      /** Guard the native output-parameter read using an initial staged value. */
      static std::pair<std::optional<{value}>, bool> read_output(sc_core::sc_fifo_blocking_in_if<{value}>& channel, const {value}& initial) {{
        return sc_core::python_fifo_read_output(channel, initial);
      }}
      /** Copy a completed sample into independent native return storage. */
      static {value} copy_value(const std::pair<std::optional<{value}>, bool>& result) {{
        return result.first.value();
      }}
        """
    if hasattr(native, "write"):
        methods += f"""
      /** Guard a borrowed blocking FIFO write; retain input and channel.
       * The caller releases GIL; Python reset/kill unwind is returned by bool.
       */
      static bool write(sc_core::sc_fifo_blocking_out_if<{value}>& channel, const {value}& value) {{
        return sc_core::python_fifo_write(channel, value);
      }}
        """
    cppyy.cppdef(f"""
    #include <optional>
    /** Guard borrowed native FIFO calls; callers release GIL and retain channel/input.
     * False completion requires Python unwind before original scheduler rethrow.
     */
    namespace pysystemc_interfaces {{ struct {identifier} {{
      {methods}
    }}; }}
    """)
    bridge = getattr(cppyy.gbl.pysystemc_interfaces, identifier)
    _bridges.append(bridge)
    read_bridge = getattr(bridge, "read", None)
    read_output_bridge = getattr(bridge, "read_output", None)
    copy_value_bridge = getattr(bridge, "copy_value", None)
    write_bridge = getattr(bridge, "write", None)
    for operation in (read_bridge, read_output_bridge, write_bridge):
        if operation is not None:
            operation.__release_gil__ = True

    def read(self, output=None):
        """Read one copyable value and assign a supplied output after completion.

        Args:
            output: Optional mutable target. It must expose ``__assign__`` or a
                writable ``value`` attribute when supplied.
        Returns:
            An independently owned native value when output is omitted;
            otherwise None after assigning the completed sample.
        Raises:
            _ProcessUnwind: Native reset or kill interrupted the blocking read.
            TypeError: The initial output cannot convert to T or the completed
                sample cannot be assigned to output.
            Native exception: The native read or value copy failed.

        The native value-return overload requires T to be copy-constructible.
        A supplied output selects the native ``read(T&)`` overload and requires
        its current value to convert to T before the read begins. The output
        remains unchanged until the read completes; a later Python assignment
        error does not put the consumed sample back into the channel. Calls
        release the GIL while blocked and retain the borrowed channel.
        """
        from . import _ProcessUnwind

        channel = self.get_interface() if hasattr(self, "get_interface") else self
        if output is None:
            result = read_bridge(channel)
        else:
            initial = output if hasattr(output, "__assign__") else output.value
            result = read_output_bridge(channel, initial)
        if not result.second:
            raise _ProcessUnwind()
        sample = copy_value_bridge(result)
        if output is None:
            return sample
        if hasattr(output, "__assign__"):
            output.__assign__(sample)
        else:
            output.value = sample

    def write(self, value):
        """Write a sample, propagating reset/kill through Python cleanup."""
        from . import _ProcessUnwind

        if not write_bridge(self.get_interface() if hasattr(self, "get_interface") else self, value):
            raise _ProcessUnwind()

    if hasattr(native, "read"):
        native.read = read
    if hasattr(native, "write"):
        native.write = write
    if native.__cpp_name__.startswith("sc_core::sc_fifo<"):

        def assign(self, sample):
            """Write through native assignment with guarded reset/kill cleanup."""
            write(self, sample)
            return self

        def convert(self):
            """Consume one sample through the guarded native conversion operator."""
            return read(self)

        stream_bridge = cppyy.gbl.sc_core.python_fifo_stream[value].insert

        def stream_insert(self, stream):
            """Print queued samples to a borrowed stream without consuming them."""
            result = stream_bridge(stream, self)
            if not result.first:
                raise OSError(str(result.second))
            return stream

        native._fifo_stream_insert = stream_insert
        native.__assign__ = assign
        setattr(native, "operator " + value, convert)
        if value in (
            "int",
            "unsigned int",
            "long",
            "unsigned long",
            "long long",
            "unsigned long long",
            "short",
            "unsigned short",
            "char",
            "signed char",
            "unsigned char",
        ):

            def integer(self):
                """Consume a native integer sample with guarded scheduler unwind."""
                sample = read(self)
                return ord(sample) if isinstance(sample, str) else int(sample)

            native.__int__ = integer
        elif value in ("float", "double", "long double"):

            def floating(self):
                """Consume a native floating sample with guarded scheduler unwind."""
                return float(read(self))

            native.__float__ = floating
        elif value == "bool":

            def boolean(self):
                """Consume a native bool sample with guarded scheduler unwind."""
                return bool(read(self))

            native.__bool__ = boolean
    native._guarded_fifo = True
    return native
