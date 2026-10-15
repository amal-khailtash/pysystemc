"""Validate native exports, fixed-point policies and TLM payload state."""
from amal.eda.systemc import sc_core as sc, sc_dt as dt, tlm

for namespace, names in (
    (sc, ('sc_time_unit', 'SC_NS', 'sc_port_policy', 'SC_ZERO_OR_MORE_BOUND',
          'sc_writer_policy', 'SC_MANY_WRITERS', 'sc_export', 'sc_mutex', 'sc_semaphore')),
    (dt, ('sc_q_mode', 'SC_TRN', 'sc_o_mode', 'SC_SAT', 'sc_logic', 'Log_X', 'sc_fixed')),
    (tlm, ('tlm_command', 'TLM_READ_COMMAND', 'tlm_response_status',
           'TLM_OK_RESPONSE', 'tlm_sync_enum', 'TLM_COMPLETED', 'tlm_generic_payload')),
):
    for name in names:
        assert name in dir(namespace), name
        assert name in namespace.__all__, name
        getattr(namespace, name)
    exported = {}
    exec(f'from {namespace.__name__} import *', exported)
    assert all(name in exported for name in names)

# Immediate channel notifications must execute in a simulation process.
mutex = sc.sc_mutex('namespace_mutex')
semaphore = sc.sc_semaphore('namespace_semaphore', 2)
channel_checks = []
def check_channels():
    """Verify native mutex ownership and semaphore count during evaluation."""
    assert mutex.trylock() == 0
    assert mutex.unlock() == 0
    assert semaphore.get_value() == 2
    assert semaphore.trywait() == 0
    assert semaphore.get_value() == 1
    assert semaphore.post() == 0
    assert semaphore.get_value() == 2
    channel_checks.append(True)
model = sc.sc_module('namespace_channels')
model.sc_method('check', check_channels)
sc.sc_start(sc.SC_ZERO_TIME)
assert channel_checks == [True]

# Native quantization truncates to the representable fractional precision;
# saturation and wrap remain selectable for arbitrary template parameters.
truncated = dt.sc_fixed[8, 4, dt.SC_TRN, dt.SC_SAT](1.1)
assert truncated.to_double() == 1.0625
saturated = dt.sc_fixed[8, 4, dt.SC_TRN, dt.SC_SAT](20)
assert saturated.to_double() == 7.9375
wrapped = dt.sc_fixed[8, 4, dt.SC_TRN, dt.SC_WRAP](20)
assert wrapped.to_double() == 4
assert dt.sc_fixed[73, 19](1.5).to_double() == 1.5

payload = tlm.tlm_generic_payload()
assert payload.get_response_status() == tlm.TLM_INCOMPLETE_RESPONSE
payload.set_command(tlm.TLM_WRITE_COMMAND)
payload.set_address(0x123456789)
payload.set_data_length(4)
payload.set_streaming_width(4)
payload.set_response_status(tlm.TLM_OK_RESPONSE)
assert payload.is_write() and not payload.is_read()
assert payload.get_address() == 0x123456789
assert payload.get_data_length() == payload.get_streaming_width() == 4
assert payload.is_response_ok()
payload.set_response_status(tlm.TLM_ADDRESS_ERROR_RESPONSE)
assert payload.is_response_error()
print('NAMESPACES_OK')

# Standard help() uses pydoc; verify useful output rather than just doc presence.
import pydoc
for namespace in (sc, dt, tlm):
    help_text = pydoc.render_doc(namespace, renderer=pydoc.plaintext)
    assert 'Native API discovery' in help_text
    assert 'Enums:' in help_text and 'Classes:' in help_text
assert 'SC_TRN=5' in pydoc.render_doc(dt, renderer=pydoc.plaintext)
assert 'SC_TRN' in pydoc.render_doc(dt.sc_q_mode, renderer=pydoc.plaintext)
assert 'TLM_OK_RESPONSE' in pydoc.render_doc(tlm.tlm_response_status, renderer=pydoc.plaintext)
assert 'Type[parameters]' in pydoc.render_doc(dt.sc_uint, renderer=pydoc.plaintext)
assert 'Native C++ class' in pydoc.render_doc(tlm.tlm_generic_payload, renderer=pydoc.plaintext)
assert 'sc_dt::sc_uint<17>' in pydoc.render_doc(dt.sc_uint[17], renderer=pydoc.plaintext)
print('HELP_OK')
