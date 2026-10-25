TLM extensions and memory managers
==================================

Ordinary extensions
-------------------

Derive native extension types from ``tlm.tlm_extension[ExtensionType]`` and
implement the native ``clone()`` and ``copy_from()`` virtual contracts. Native
registration gives each type a stable ``ID`` for the process lifetime. Generic
payloads borrow manually installed extension storage until it is cleared or
freed under the payload's native ownership policy.

``clone()`` returns a new extension pointer whose lifetime must be managed by
the recipient; ``free()`` uses the extension's native deallocation contract.
Copying extension state does not transfer the source object's ownership.
Native endian contexts and instance-specific carriers intentionally return a
null clone and provide a no-op ``copy_from()`` implementation.

``tlm.tlm_mm_interface`` supplies the payload ``free(transaction)`` callback.
A Python override can observe payloads when their reference count reaches zero;
retain the manager while managed payloads can return to it. The callback receives
a borrowed payload with the GIL held. Its implementation determines whether to
recycle or destroy native storage.

Instance-specific extensions
----------------------------

``tlm_utils.instance_specific_extension_accessor`` isolates private extensions by
accessor instance and transaction. Define a native extension deriving from
``tlm_utils::instance_specific_extension<T>``; its ``priv_id`` identifies the
extension type. Extension storage is borrowed: retain its Python/native owner
until it has been cleared from every accessor that uses it.

The carrier type is registered lazily in cppyy. Resolve
``tlm_utils.instance_specific_extension_carrier.ID`` before constructing payloads,
and call ``payload.resize_extensions()`` if extension types were registered after
payload construction. Native payload extension access requires adequate array
capacity; violating that requirement can trigger a native assertion.

Obtain the per-accessor interface with ``accessor(payload)``. Its native methods
include ``set_extension(extension)``, ``get_extension(type.priv_id)`` and
``clear_extension(type.priv_id)``. ``set_extension`` returns the previous borrowed
pointer or a null pointer. The numeric ``get_extension`` overload returns an
``ispex_base`` pointer; use ``cppyy.bind_object(pointer, ExtensionType)`` to view
its concrete fields. Different accessors on the same payload cannot see one
another's extensions. The same accessor on another payload also starts empty.

Clearing the final private extension releases the carrier and its container.
All previously returned per-accessor references for that payload then become
invalid: discard them and reacquire with ``accessor(payload)`` before reuse.
Clearing does not delete the borrowed extension object. A reacquired accessor
starts empty and can attach a new extension.

``tlm_generic_payload.deep_copy_from`` copies ordinary payload state but omits
private information: the native carrier's ``clone()`` returns null. The copied
payload can acquire its own independent private extensions. Existing private
extensions on the original remain attached until explicitly cleared.

The regression in ``tests/models/tlm_instance_extensions.py`` verifies accessor
and transaction isolation, pointer identity, replacement, last-clear carrier
release, reattachment and deep-copy omission. An independent C++ reference in ``tests/native/tlm_instance_extensions.cpp``
checks the same lifecycle semantics. ``tests/models/tlm_extensions_public.py``
additionally compares native clone/copy/free counts, stable registration,
memory-manager callbacks, typed and numeric accessor overloads, public aliases
and concrete extension specializations. These checks preserve borrowed private
extension storage and distinguish its lifetime from the carrier/container.

Generic payload storage and copying
------------------------------------

``tlm.tlm_generic_payload`` exposes command, address, response, streaming-width,
byte-enable and payload-option fields with native numeric conversions. Data and
byte-enable pointers borrow caller storage; keep buffers alive while the payload
or any recipient uses them. ``deep_copy_from`` requires sufficiently allocated
destination buffers, copies payload fields and clones supported extensions.
``update_original_from`` copies read responses with native byte-enable masking;
write responses do not copy source data back.

Typed and indexed extension access preserve native pointer identity. Replacing
an extension returns the old pointer. ``clear_extension`` detaches without
freeing it, whereas ``release_extension`` follows the payload's native manager
and deferred-release policy. Auto extensions require a memory manager and are
released by native recycling/reset rules. ``reset()`` releases scheduled
auto extensions while preserving ordinary payload fields.

Keep a memory manager alive while its payloads are acquired. ``acquire()`` and
``release()`` follow the native reference count and invoke the manager's
``free`` callback when it reaches zero. Extension indices must be registered and
within the allocated extension array. Manager, reference-count and buffer-size
preconditions remain native requirements; violating them is not guaranteed to
produce a safe Python exception.
