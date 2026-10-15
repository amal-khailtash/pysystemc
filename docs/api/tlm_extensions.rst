Instance-specific TLM extensions
================================

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
checks the same lifecycle semantics. Complete carrier recycling and broad
lifetime coverage remain pending. These
checks do not establish complete extension support or M6A completion.
