"""Exercise public native and typed sc_plist list families through cppyy."""

from amal.eda.systemc import sc_core as sc

import cppyy
import pathlib
import threading

cppyy.include(str(pathlib.Path(__file__).resolve().parents[1] / "native" / "sc_core_lists_probe.h"))
probe = cppyy.gbl.sc_core_lists_probe


def read_base_values(list_proxy):
    """Return borrowed integer payload values using the native base iterator.

    Args:
        list_proxy: A live ``sc_plist_base`` whose payloads remain alive.

    Returns:
        Integer payloads in forward list order.
    """
    values = []
    cursor = sc.sc_plist_base_iter(list_proxy)
    while not cursor.empty():
        values.append(probe.read_integer(cursor.get()))
        cursor.__postinc__(0)
    return values


base = sc.sc_plist_base()
assert base.empty() and base.size() == 0
h20 = base.push_back(probe.integer_pointer(3))
h10 = base.push_front(probe.integer_pointer(1))
h30 = base.push_back(probe.integer_pointer(4))
h16 = base.insert_before(h20, probe.integer_pointer(2))
h35 = base.insert_after(h20, probe.integer_pointer(5))
h40 = base.insert_before(None, probe.integer_pointer(6))
h5 = base.insert_after(None, probe.integer_pointer(0))
assert base.size() == 7 and probe.read_integer(base.front()) == 5
assert probe.read_integer(base.back()) == 40
base.set(h16, probe.integer_pointer(7))
assert probe.read_integer(base.get(h16)) == 16
assert read_base_values(base) == [5, 10, 16, 20, 35, 30, 40]

base_cursor = sc.sc_plist_base_iter(base)
base_cursor.set_handle(h35)
assert base_cursor.get_handle() == h35 and probe.read_integer(base_cursor.get()) == 35
base_cursor.set(probe.integer_pointer(7))
assert probe.read_integer(base_cursor.get()) == 16
base_cursor.remove()
assert base_cursor.get_handle() == h30 and probe.read_integer(base_cursor.get()) == 30
base_cursor.remove(1)
assert base_cursor.get_handle() == h40 and probe.read_integer(base_cursor.get()) == 40
base_cursor.set_handle(h20)
base_cursor.remove(-1)
assert base_cursor.get_handle() == h16 and probe.read_integer(base_cursor.get()) == 16
assert read_base_values(base) == [5, 10, 16, 40]
base_cursor.reset(base, True)
backward = []
while not base_cursor.empty():
    backward.append(probe.read_integer(base_cursor.get()))
    if backward[-1] == 5:
        break
    base_cursor.__postdec__(0)
assert backward == [40, 16, 10, 5]
assert probe.read_integer(base.remove(h16)) == 16
assert probe.read_integer(base.pop_front()) == 5
assert probe.read_integer(base.pop_back()) == 40
assert base.size() == 1 and probe.read_integer(base.front()) == 10 and probe.read_integer(base.back()) == 10
base.erase_all()
assert base.empty() and base.size() == 0
assert [probe.read_integer(probe.integer_pointer(index)) for index in (0, 6, 7)] == [5, 40, 16]

item_pointer_type = "sc_core_lists_probe::Item*"
typed_list_type = sc.sc_plist[item_pointer_type]
typed_iterator_type = typed_list_type.iterator
typed = typed_list_type()
h_item0 = typed.push_back(probe.item_pointer(0))
h_item1 = typed.push_front(probe.item_pointer(1))
h_item2 = typed.push_back(probe.item_pointer(2))
h_item4 = typed.insert_after(h_item0, probe.item_pointer(4))
h_item3 = typed.insert_before(h_item0, probe.item_pointer(3))
typed.set(h_item0, probe.item_pointer(5))
assert typed.size() == 5
assert probe.read_item_identity(typed.front()) == 1 and probe.read_item_identity(typed.back()) == 2
assert probe.read_item_identity(typed.get(h_item0)) == 5
assert probe.read_item_identity(typed.get(h_item3)) == 3

typed_cursor = typed_iterator_type(typed, True)
typed_pointer_cursor = typed_iterator_type(probe.item_list_pointer(typed), True)
assert probe.read_item_identity(typed_pointer_cursor.get()) == 2
typed_pointer_cursor.reset(probe.item_list_pointer(typed), False)
assert probe.read_item_identity(typed_pointer_cursor.get()) == 1
assert probe.read_item_identity(typed_cursor.get()) == 2
typed_cursor.__postdec__(0)
assert probe.read_item_identity(typed_cursor.__deref__()) == 4
typed_cursor.set(probe.item_pointer(0))
assert probe.read_item_identity(typed.get(h_item4)) == 0
typed_cursor.reset(typed, False)
assert probe.read_item_identity(typed_cursor.get()) == 1
typed_cursor.__postinc__(0)
assert probe.read_item_identity(typed_cursor.get()) == 3
typed_cursor.remove(1)
assert probe.read_item_identity(typed_cursor.get()) == 5
typed_cursor.reset(typed, True)
assert probe.read_item_identity(typed_cursor.get()) == 2
typed_cursor.reset(typed, False)
assert probe.read_item_identity(typed_cursor.get()) == 1
typed_cursor.reset(typed, True)
assert probe.read_item_identity(typed_cursor.get()) == 2
assert probe.read_item_identity(typed.pop_front()) == 1
assert probe.read_item_identity(typed.pop_back()) == 2
assert typed.size() == 2
assert probe.read_item_identity(typed.front()) == 5 and probe.read_item_identity(typed.back()) == 0
assert probe.read_item_identity(typed.remove(h_item4)) == 0 and typed.size() == 1
typed.erase_all()
assert typed.empty() and typed.size() == 0
assert [probe.read_item_identity(probe.item_pointer(index)) for index in range(6)] == [0, 1, 2, 3, 4, 5]

null_typed = typed_list_type()
null_typed.insert_before(None, probe.item_pointer(0))
null_typed.insert_after(None, probe.item_pointer(1))
assert null_typed.size() == 2
assert probe.read_item_identity(null_typed.front()) == 1
assert probe.read_item_identity(null_typed.back()) == 0
null_typed.erase_all()
assert null_typed.empty()

mapped = sc.sc_plist_base()
for index in range(3):
    mapped.push_back(probe.integer_pointer(index))
callback_values = []
callback_contexts = []
callback_threads = []


def map_callback(value_pointer, context_pointer):
    """Read each borrowed value during synchronous mapcar callback dispatch.

    cppyy invokes this Python callback on the calling thread with the GIL
    available. The local callable remains live for the complete native call.

    Args:
        value_pointer: Borrowed payload pointer for the current list node.
        context_pointer: Caller-owned opaque context forwarded by mapcar.

    Returns:
        None. This callback does not escape the synchronous mapcar call.
    """
    callback_values.append(probe.read_integer(value_pointer))
    callback_contexts.append(probe.is_map_state(context_pointer))
    callback_threads.append(threading.get_ident())


mapped.mapcar(map_callback, probe.map_state_pointer())
assert callback_values == [5, 10, 15]
assert callback_contexts == [True, True, True]
assert callback_threads and set(callback_threads) == {threading.get_ident()}
assert [probe.read_integer(probe.integer_pointer(index)) for index in range(3)] == [5, 10, 15]

print("SC_PLIST_REFERENCE base=5,10,16,40 typed=1,5 map=30,3 borrowed=1")
