"""Exercise the six public FIFO interface families over a native FIFO."""

from amal.eda.systemc import sc_core as sc, sc_dt
import cppyy


cppyy.cppdef(
    """
    /** Exercise FIFO input output-reference calls without retaining storage.
     * The referenced channels and local output values live through each call.
     */
    #include <cstdint>
    namespace fifo_interfaces_public {
    std::uintptr_t combined_in_address(sc_core::sc_fifo<int>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_in_if<int>*>(&channel));
    }
    std::uintptr_t combined_out_address(sc_core::sc_fifo<int>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_out_if<int>*>(&channel));
    }
    std::uintptr_t blocking_in_address(sc_core::sc_fifo<int>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_blocking_in_if<int>*>(&channel));
    }
    std::uintptr_t blocking_out_address(sc_core::sc_fifo<int>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_blocking_out_if<int>*>(&channel));
    }
    std::uintptr_t nonblocking_in_address(sc_core::sc_fifo<int>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_nonblocking_in_if<int>*>(&channel));
    }
    std::uintptr_t nonblocking_out_address(sc_core::sc_fifo<int>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_nonblocking_out_if<int>*>(&channel));
    }
    using wide_type = sc_dt::sc_biguint<137>;
    std::uintptr_t wide_combined_in_address(sc_core::sc_fifo<wide_type>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_in_if<wide_type>*>(&channel));
    }
    std::uintptr_t wide_combined_out_address(sc_core::sc_fifo<wide_type>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_out_if<wide_type>*>(&channel));
    }
    std::uintptr_t wide_blocking_in_address(sc_core::sc_fifo<wide_type>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_blocking_in_if<wide_type>*>(&channel));
    }
    std::uintptr_t wide_blocking_out_address(sc_core::sc_fifo<wide_type>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_blocking_out_if<wide_type>*>(&channel));
    }
    std::uintptr_t wide_nonblocking_in_address(sc_core::sc_fifo<wide_type>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_nonblocking_in_if<wide_type>*>(&channel));
    }
    std::uintptr_t wide_nonblocking_out_address(sc_core::sc_fifo<wide_type>& channel) {
      return reinterpret_cast<std::uintptr_t>(
          static_cast<sc_core::sc_fifo_nonblocking_out_if<wide_type>*>(&channel));
    }
    std::pair<bool, int> nb_read(sc_core::sc_fifo_nonblocking_in_if<int>& channel,
                                 int initial) {
      int value = initial;
      bool success = channel.nb_read(value);
      return {success, value};
    }
    int read_ref(sc_core::sc_fifo_blocking_in_if<int>& channel) {
      int value = -1;
      channel.read(value);
      return value;
    }
    }

    /** Native derived implementations expose protected combined constructors.
     * Each object is created and destroyed through its Python cppyy proxy.
     */
    namespace fifo_interface_destruction {
    struct Input : sc_core::sc_fifo_in_if<int> {
      sc_core::sc_event written;
      static int destructions;
      Input() : sc_core::sc_fifo_in_if<int>() {}
      ~Input() override { ++destructions; }
      bool nb_read(int&) override { return false; }
      const sc_core::sc_event& data_written_event() const override { return written; }
      void read(int&) override {}
      int read() override { return 0; }
      int num_available() const override { return 0; }
    };
    int Input::destructions = 0;

    struct Output : sc_core::sc_fifo_out_if<int> {
      sc_core::sc_event read_event;
      static int destructions;
      Output() : sc_core::sc_fifo_out_if<int>() {}
      ~Output() override { ++destructions; }
      int num_free() const override { return 0; }
      bool nb_write(const int&) override { return false; }
      const sc_core::sc_event& data_read_event() const override { return read_event; }
      void write(const int&) override {}
    };
    int Output::destructions = 0;

    struct BlockingInput : sc_core::sc_fifo_blocking_in_if<int> {
      static int destructions;
      ~BlockingInput() override { ++destructions; }
      void read(int&) override {}
      int read() override { return 0; }
    };
    int BlockingInput::destructions = 0;

    struct BlockingOutput : sc_core::sc_fifo_blocking_out_if<int> {
      static int destructions;
      ~BlockingOutput() override { ++destructions; }
      void write(const int&) override {}
    };
    int BlockingOutput::destructions = 0;

    struct NonblockingInput : sc_core::sc_fifo_nonblocking_in_if<int> {
      sc_core::sc_event written;
      static int destructions;
      ~NonblockingInput() override { ++destructions; }
      bool nb_read(int&) override { return false; }
      const sc_core::sc_event& data_written_event() const override { return written; }
    };
    int NonblockingInput::destructions = 0;

    struct NonblockingOutput : sc_core::sc_fifo_nonblocking_out_if<int> {
      sc_core::sc_event read_event;
      static int destructions;
      ~NonblockingOutput() override { ++destructions; }
      bool nb_write(const int&) override { return false; }
      const sc_core::sc_event& data_read_event() const override { return read_event; }
    };
    int NonblockingOutput::destructions = 0;
    }
    """
)


class Observer(sc.sc_module):
    """Record borrowed FIFO activity events during simulation."""

    def __init__(self, name, written_event, read_event):
        """Register non-initializing methods on borrowed FIFO events.

        Args:
            name: SystemC module name.
            written_event: FIFO-owned event that must outlive this observer.
            read_event: FIFO-owned event that must outlive this observer.
        """
        module_name = sc.sc_module_name(name)
        super().__init__(module_name)
        self.records = []
        self.sc_method("written", self.written)
        self.sensitive(written_event)
        self.dont_initialize()
        self.sc_method("read", self.read)
        self.sensitive(read_event)
        self.dont_initialize()

    def written(self):
        """Append the current simulation time and delta for a write event."""
        self.records.append(("W", sc.sc_time_stamp().value(), sc.sc_delta_count()))

    def read(self):
        """Append the current simulation time and delta for a read event."""
        self.records.append(("R", sc.sc_time_stamp().value(), sc.sc_delta_count()))


module_name = sc.sc_module_name("top")
top = sc.sc_module(module_name)
fifo = sc.sc_fifo[int]("fifo", 2)
interfaces = {
    "combined_in": sc.sc_fifo_in_if[int],
    "combined_out": sc.sc_fifo_out_if[int],
    "blocking_in": sc.sc_fifo_blocking_in_if[int],
    "blocking_out": sc.sc_fifo_blocking_out_if[int],
    "nonblocking_in": sc.sc_fifo_nonblocking_in_if[int],
    "nonblocking_out": sc.sc_fifo_nonblocking_out_if[int],
}
addresses = {name: getattr(cppyy.gbl.fifo_interfaces_public, f"{name}_address")(fifo) for name in interfaces}
channels = {name: cppyy.bind_object(addresses[name], interface) for name, interface in interfaces.items()}
del module_name
observer = Observer(
    "observer",
    channels["combined_in"].data_written_event(),
    channels["combined_out"].data_read_event(),
)
temporary = sc.sc_fifo[int]("fifo_lifetime", 1)
temporary_name = str(temporary.name())
assert sc.sc_find_object(temporary_name)
temporary.__destruct__()
assert not sc.sc_find_object(temporary_name)
print("FIFO_IFACE_LIFETIME 1")

destruction_probes = tuple(
    getattr(cppyy.gbl.fifo_interface_destruction, name)()
    for name in (
        "Input",
        "Output",
        "BlockingInput",
        "BlockingOutput",
        "NonblockingInput",
        "NonblockingOutput",
    )
)
for probe in destruction_probes:
    probe.__destruct__()
destruction_counts = tuple(
    int(getattr(getattr(cppyy.gbl.fifo_interface_destruction, name), "destructions"))
    for name in (
        "Input",
        "Output",
        "BlockingInput",
        "BlockingOutput",
        "NonblockingInput",
        "NonblockingOutput",
    )
)
assert destruction_counts == (1, 1, 1, 1, 1, 1), destruction_counts
print("FIFO_IFACE_DESTRUCTION", *destruction_counts)

wide_type = sc_dt.sc_biguint[137]
wide_fifo = sc.sc_fifo[wide_type]("wide_fifo", 3)
wide_names = (
    "combined_in",
    "combined_out",
    "blocking_in",
    "blocking_out",
    "nonblocking_in",
    "nonblocking_out",
)
wide_interfaces = {
    "combined_in": sc.sc_fifo_in_if[wide_type],
    "combined_out": sc.sc_fifo_out_if[wide_type],
    "blocking_in": sc.sc_fifo_blocking_in_if[wide_type],
    "blocking_out": sc.sc_fifo_blocking_out_if[wide_type],
    "nonblocking_in": sc.sc_fifo_nonblocking_in_if[wide_type],
    "nonblocking_out": sc.sc_fifo_nonblocking_out_if[wide_type],
}
wide_channels = {
    name: cppyy.bind_object(
        getattr(cppyy.gbl.fifo_interfaces_public, f"wide_{name}_address")(wide_fifo),
        wide_interfaces[name],
    )
    for name in wide_names
}
assert (
    len({cppyy.addressof(wide_channels[name].data_written_event()) for name in ("combined_in", "nonblocking_in")}) == 1
)
assert (
    len({cppyy.addressof(wide_channels[name].data_read_event()) for name in ("combined_out", "nonblocking_out")}) == 1
)

wide_output = wide_type(73)
wide_empty_read = wide_channels["nonblocking_in"].nb_read(wide_output)
wide_values = (
    wide_type("174224571863520493293247799005065324265"),
    wide_type("174224571863520493293247799005065324267"),
    wide_type("174224571863520493293247799005065324271"),
)
wide_write = wide_channels["nonblocking_out"].nb_write(wide_values[0])
wide_channels["blocking_out"].write(wide_values[1])
wide_channels["combined_out"].write(wide_values[2])
wide_before_update = (
    wide_channels["combined_in"].num_available(),
    wide_channels["combined_out"].num_free(),
)
assert not wide_empty_read and wide_output.to_string() == "73"
assert wide_write and wide_before_update == (0, 0)

written_events = {cppyy.addressof(channels[name].data_written_event()) for name in ("combined_in", "nonblocking_in")}
read_events = {cppyy.addressof(channels[name].data_read_event()) for name in ("combined_out", "nonblocking_out")}
assert len(written_events) == len(read_events) == 1

initial = (
    channels["combined_in"].num_available(),
    channels["combined_out"].num_free(),
    cppyy.gbl.fifo_interfaces_public.nb_read(channels["nonblocking_in"], 91),
)
assert initial[:2] == (0, 2), initial
assert (initial[2].first, initial[2].second) == (False, 91), initial

first_write = channels["nonblocking_out"].nb_write(17)
channels["blocking_out"].write(23)
rejected_write = channels["combined_out"].nb_write(29)
before_update = (
    channels["combined_in"].num_available(),
    channels["combined_out"].num_free(),
)
assert first_write and not rejected_write and before_update == (0, 0)

sc.sc_start(1, sc.SC_NS)
readable = channels["combined_in"].num_available()
first_read = cppyy.gbl.fifo_interfaces_public.read_ref(channels["blocking_in"])
second_read = channels["combined_in"].read()
after_reads = channels["combined_in"].num_available()
before_read_update = channels["combined_out"].num_free()
assert (readable, first_read, second_read, after_reads, before_read_update) == (
    2,
    17,
    23,
    0,
    0,
)
wide_readable = wide_channels["combined_in"].num_available()
assert wide_readable == 3
assert wide_channels["nonblocking_in"].nb_read(wide_output)
wide_first = wide_output.to_string()
wide_channels["blocking_in"].read(wide_output)
wide_second = wide_output.to_string()
wide_third = wide_channels["combined_in"].read().to_string()
wide_after_reads = wide_channels["combined_in"].num_available()
wide_free_before_update = wide_channels["combined_out"].num_free()
assert (
    wide_first,
    wide_second,
    wide_third,
    wide_after_reads,
    wide_free_before_update,
) == tuple(value.to_string() for value in wide_values) + (0, 0)
sc.sc_start(1, sc.SC_NS)
after_update = channels["combined_out"].num_free()
assert after_update == 2
wide_free_after_update = wide_channels["combined_out"].num_free()
assert wide_free_after_update == 3

for kind, time_value, delta in observer.records:
    print("FIFO_IFACE_EVENT", kind, time_value, delta)
print(
    "FIFO_IFACE_VALUES",
    *initial[:2],
    int(initial[2][0]),
    initial[2][1],
    int(first_write),
    int(rejected_write),
    *before_update,
    readable,
    first_read,
    second_read,
    after_reads,
    before_read_update,
    after_update,
)
assert [record[0] for record in observer.records] == ["W", "R"]
print(
    "FIFO_IFACE_WIDE",
    int(wide_empty_read),
    int(wide_write),
    *wide_before_update,
    wide_readable,
    wide_first,
    wide_second,
    wide_third,
    wide_after_reads,
    wide_free_before_update,
    wide_free_after_update,
)

sc.sc_stop()
