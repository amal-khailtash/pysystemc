/** @file sc_core_constants.cpp
 * @brief Independent native public scalar/string/enum constant inventory.
 * @details Read static native storage without ownership transfer or GIL calls.
 */
#include <sysc/kernel/sc_constants.h>
#include <sysc/tracing/sc_tracing_ids.h>
#include <sysc/utils/sc_hash.h>

#include <iomanip>
#include <iostream>
#include <systemc>
int sc_main(int, char**)
{
  std::cout << std::setprecision(17);
  std::cout << "CORE_CONSTANT sc_core::PHASH_DEFAULT_GROW_FACTOR=" << sc_core::PHASH_DEFAULT_GROW_FACTOR << "\n";
  std::cout << "CORE_CONSTANT sc_core::PHASH_DEFAULT_INIT_TABLE_SIZE=" << sc_core::PHASH_DEFAULT_INIT_TABLE_SIZE
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::PHASH_DEFAULT_MAX_DENSITY=" << sc_core::PHASH_DEFAULT_MAX_DENSITY << "\n";
  std::cout << "CORE_CONSTANT sc_core::PHASH_DEFAULT_REORDER_FLAG=" << int(sc_core::PHASH_DEFAULT_REORDER_FLAG) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ABORT=" << static_cast<long long>(sc_core::SC_ABORT) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ALL_BOUND=" << static_cast<long long>(sc_core::SC_ALL_BOUND) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_BEFORE_END_OF_ELABORATION="
            << static_cast<long long>(sc_core::SC_BEFORE_END_OF_ELABORATION) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_BEFORE_TIMESTEP=" << static_cast<long long>(sc_core::SC_BEFORE_TIMESTEP)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_CACHE_REPORT=" << static_cast<long long>(sc_core::SC_CACHE_REPORT) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_CTHREAD_PROC_=" << static_cast<long long>(sc_core::SC_CTHREAD_PROC_) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEBUG=" << static_cast<long long>(sc_core::SC_DEBUG) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEFAULT_CATCH_ACTIONS="
            << static_cast<long long>(sc_core::SC_DEFAULT_CATCH_ACTIONS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEFAULT_ERROR_ACTIONS="
            << static_cast<long long>(sc_core::SC_DEFAULT_ERROR_ACTIONS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEFAULT_FATAL_ACTIONS="
            << static_cast<long long>(sc_core::SC_DEFAULT_FATAL_ACTIONS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEFAULT_INFO_ACTIONS="
            << static_cast<long long>(sc_core::SC_DEFAULT_INFO_ACTIONS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEFAULT_STACK_SIZE=" << sc_core::SC_DEFAULT_STACK_SIZE << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DEFAULT_WARNING_ACTIONS="
            << static_cast<long long>(sc_core::SC_DEFAULT_WARNING_ACTIONS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DISPLAY=" << static_cast<long long>(sc_core::SC_DISPLAY) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_DO_NOTHING=" << static_cast<long long>(sc_core::SC_DO_NOTHING) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ELABORATION=" << static_cast<long long>(sc_core::SC_ELABORATION) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_END_OF_ELABORATION=" << static_cast<long long>(sc_core::SC_END_OF_ELABORATION)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_END_OF_INITIALIZATION="
            << static_cast<long long>(sc_core::SC_END_OF_INITIALIZATION) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_END_OF_SIMULATION=" << static_cast<long long>(sc_core::SC_END_OF_SIMULATION)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_END_OF_UPDATE=" << static_cast<long long>(sc_core::SC_END_OF_UPDATE) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ERROR=" << static_cast<long long>(sc_core::SC_ERROR) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_EXIT_ON_STARVATION=" << static_cast<long long>(sc_core::SC_EXIT_ON_STARVATION)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_FATAL=" << static_cast<long long>(sc_core::SC_FATAL) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_FS=" << static_cast<long long>(sc_core::SC_FS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_FULL=" << static_cast<long long>(sc_core::SC_FULL) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_HIERARCHY_CHAR=" << int(sc_core::SC_HIERARCHY_CHAR) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_HIGH=" << static_cast<long long>(sc_core::SC_HIGH) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ABORT_=" << sc_core::SC_ID_ABORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ASSERTION_FAILED_=" << sc_core::SC_ID_ASSERTION_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ASSIGNMENT_FAILED_=" << sc_core::SC_ID_ASSIGNMENT_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ATTEMPT_TO_BIND_CLOCK_TO_OUTPUT_="
            << sc_core::SC_ID_ATTEMPT_TO_BIND_CLOCK_TO_OUTPUT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ATTEMPT_TO_WRITE_TO_CLOCK_=" << sc_core::SC_ID_ATTEMPT_TO_WRITE_TO_CLOCK_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_BACK_ON_EMPTY_LIST_=" << sc_core::SC_ID_BACK_ON_EMPTY_LIST_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_BAD_SC_MODULE_CONSTRUCTOR_=" << sc_core::SC_ID_BAD_SC_MODULE_CONSTRUCTOR_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_BIND_IF_TO_PORT_=" << sc_core::SC_ID_BIND_IF_TO_PORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_BIND_PORT_TO_PORT_=" << sc_core::SC_ID_BIND_PORT_TO_PORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CANNOT_CONVERT_=" << sc_core::SC_ID_CANNOT_CONVERT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CLOCK_HIGH_TIME_ZERO_=" << sc_core::SC_ID_CLOCK_HIGH_TIME_ZERO_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CLOCK_LOW_TIME_ZERO_=" << sc_core::SC_ID_CLOCK_LOW_TIME_ZERO_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CLOCK_PERIOD_ZERO_=" << sc_core::SC_ID_CLOCK_PERIOD_ZERO_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_COMPLETE_BINDING_=" << sc_core::SC_ID_COMPLETE_BINDING_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CONTEXT_BEGIN_FAILED_=" << sc_core::SC_ID_CONTEXT_BEGIN_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CONTEXT_END_FAILED_=" << sc_core::SC_ID_CONTEXT_END_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CONVERSION_FAILED_=" << sc_core::SC_ID_CONVERSION_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_CYCLE_MISSES_EVENTS_=" << sc_core::SC_ID_CYCLE_MISSES_EVENTS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_DEFAULT_TIME_UNIT_CHANGED_=" << sc_core::SC_ID_DEFAULT_TIME_UNIT_CHANGED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_DISABLE_WILL_ORPHAN_PROCESS_="
            << sc_core::SC_ID_DISABLE_WILL_ORPHAN_PROCESS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_DONT_INITIALIZE_=" << sc_core::SC_ID_DONT_INITIALIZE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_EMPTY_PROCESS_HANDLE_=" << sc_core::SC_ID_EMPTY_PROCESS_HANDLE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_END_MODULE_NOT_CALLED_=" << sc_core::SC_ID_END_MODULE_NOT_CALLED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_EVENT_LIST_FAILED_=" << sc_core::SC_ID_EVENT_LIST_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_EVENT_ON_NULL_PROCESS_=" << sc_core::SC_ID_EVENT_ON_NULL_PROCESS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_EXPORT_OUTSIDE_MODULE_=" << sc_core::SC_ID_EXPORT_OUTSIDE_MODULE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_FIND_EVENT_=" << sc_core::SC_ID_FIND_EVENT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_FRONT_ON_EMPTY_LIST_=" << sc_core::SC_ID_FRONT_ON_EMPTY_LIST_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_GEN_UNIQUE_NAME_=" << sc_core::SC_ID_GEN_UNIQUE_NAME_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_GET_IF_=" << sc_core::SC_ID_GET_IF_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_HALT_NOT_ALLOWED_=" << sc_core::SC_ID_HALT_NOT_ALLOWED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_HIER_NAME_INCORRECT_=" << sc_core::SC_ID_HIER_NAME_INCORRECT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_IEEE_1666_DEPRECATION_=" << sc_core::SC_ID_IEEE_1666_DEPRECATION_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ILLEGAL_CHARACTERS_=" << sc_core::SC_ID_ILLEGAL_CHARACTERS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_IMMEDIATE_NOTIFICATION_=" << sc_core::SC_ID_IMMEDIATE_NOTIFICATION_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_IMMEDIATE_SELF_NOTIFICATION_="
            << sc_core::SC_ID_IMMEDIATE_SELF_NOTIFICATION_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INCOMPATIBLE_TYPES_=" << sc_core::SC_ID_INCOMPATIBLE_TYPES_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INCOMPATIBLE_VECTORS_=" << sc_core::SC_ID_INCOMPATIBLE_VECTORS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INCONSISTENT_API_CONFIG_=" << sc_core::SC_ID_INCONSISTENT_API_CONFIG_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INIT_FAILED_=" << sc_core::SC_ID_INIT_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INSERT_EXPORT_=" << sc_core::SC_ID_INSERT_EXPORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INSERT_MODULE_=" << sc_core::SC_ID_INSERT_MODULE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INSERT_PORT_=" << sc_core::SC_ID_INSERT_PORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INSERT_PRIM_CHANNEL_=" << sc_core::SC_ID_INSERT_PRIM_CHANNEL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INSTANCE_EXISTS_=" << sc_core::SC_ID_INSTANCE_EXISTS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INTERNAL_ERROR_=" << sc_core::SC_ID_INTERNAL_ERROR_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_CTE_WL_=" << sc_core::SC_ID_INVALID_CTE_WL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_DIV_WL_=" << sc_core::SC_ID_INVALID_DIV_WL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_FIFO_SIZE_=" << sc_core::SC_ID_INVALID_FIFO_SIZE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_FX_VALUE_=" << sc_core::SC_ID_INVALID_FX_VALUE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_MAX_WL_=" << sc_core::SC_ID_INVALID_MAX_WL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_N_BITS_=" << sc_core::SC_ID_INVALID_N_BITS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_O_MODE_=" << sc_core::SC_ID_INVALID_O_MODE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_SEMAPHORE_VALUE_=" << sc_core::SC_ID_INVALID_SEMAPHORE_VALUE_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_INVALID_WL_=" << sc_core::SC_ID_INVALID_WL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_JOIN_ON_METHOD_HANDLE_=" << sc_core::SC_ID_JOIN_ON_METHOD_HANDLE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_KILL_PROCESS_WHILE_UNITIALIZED_="
            << sc_core::SC_ID_KILL_PROCESS_WHILE_UNITIALIZED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_LENGTH_MISMATCH_=" << sc_core::SC_ID_LENGTH_MISMATCH_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_LOGIC_X_TO_BOOL_=" << sc_core::SC_ID_LOGIC_X_TO_BOOL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_LOGIC_Z_TO_BOOL_=" << sc_core::SC_ID_LOGIC_Z_TO_BOOL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MAKE_SENSITIVE_=" << sc_core::SC_ID_MAKE_SENSITIVE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MAKE_SENSITIVE_NEG_=" << sc_core::SC_ID_MAKE_SENSITIVE_NEG_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MAKE_SENSITIVE_POS_=" << sc_core::SC_ID_MAKE_SENSITIVE_POS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_METHOD_TERMINATION_EVENT_=" << sc_core::SC_ID_METHOD_TERMINATION_EVENT_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MODULE_CTHREAD_AFTER_START_=" << sc_core::SC_ID_MODULE_CTHREAD_AFTER_START_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MODULE_METHOD_AFTER_START_=" << sc_core::SC_ID_MODULE_METHOD_AFTER_START_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MODULE_NAME_STACK_EMPTY_=" << sc_core::SC_ID_MODULE_NAME_STACK_EMPTY_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MODULE_THREAD_AFTER_START_=" << sc_core::SC_ID_MODULE_THREAD_AFTER_START_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MORE_THAN_ONE_FIFO_READER_=" << sc_core::SC_ID_MORE_THAN_ONE_FIFO_READER_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MORE_THAN_ONE_FIFO_WRITER_=" << sc_core::SC_ID_MORE_THAN_ONE_FIFO_WRITER_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_="
            << sc_core::SC_ID_MORE_THAN_ONE_SIGNAL_DRIVER_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NAME_EXISTS_=" << sc_core::SC_ID_NAME_EXISTS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NEXT_TRIGGER_NOT_ALLOWED_=" << sc_core::SC_ID_NEXT_TRIGGER_NOT_ALLOWED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NOTIFY_DELAYED_=" << sc_core::SC_ID_NOTIFY_DELAYED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NOT_EXPECTING_DYNAMIC_EVENT_NOTIFY_="
            << sc_core::SC_ID_NOT_EXPECTING_DYNAMIC_EVENT_NOTIFY_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NOT_IMPLEMENTED_=" << sc_core::SC_ID_NOT_IMPLEMENTED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_ASYNC_UPDATE_=" << sc_core::SC_ID_NO_ASYNC_UPDATE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_BOOL_RETURNED_=" << sc_core::SC_ID_NO_BOOL_RETURNED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_DEFAULT_EVENT_=" << sc_core::SC_ID_NO_DEFAULT_EVENT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_INT_RETURNED_=" << sc_core::SC_ID_NO_INT_RETURNED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_PROCESS_SEMANTICS_=" << sc_core::SC_ID_NO_PROCESS_SEMANTICS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_SC_LOGIC_RETURNED_=" << sc_core::SC_ID_NO_SC_LOGIC_RETURNED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_NO_SC_START_ACTIVITY_=" << sc_core::SC_ID_NO_SC_START_ACTIVITY_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_OPERAND_NOT_BOOL_=" << sc_core::SC_ID_OPERAND_NOT_BOOL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_OPERAND_NOT_SC_LOGIC_=" << sc_core::SC_ID_OPERAND_NOT_SC_LOGIC_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_OPERATION_FAILED_=" << sc_core::SC_ID_OPERATION_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_="
            << sc_core::SC_ID_OPERATION_ON_NON_SPECIALIZED_SIGNAL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_OUT_OF_BOUNDS_=" << sc_core::SC_ID_OUT_OF_BOUNDS_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_OUT_OF_RANGE_=" << sc_core::SC_ID_OUT_OF_RANGE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PHASE_CALLBACKS_UNSUPPORTED_="
            << sc_core::SC_ID_PHASE_CALLBACKS_UNSUPPORTED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PHASE_CALLBACK_FORBIDDEN_=" << sc_core::SC_ID_PHASE_CALLBACK_FORBIDDEN_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PHASE_CALLBACK_NOT_IMPLEMENTED_="
            << sc_core::SC_ID_PHASE_CALLBACK_NOT_IMPLEMENTED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PHASE_CALLBACK_REGISTER_=" << sc_core::SC_ID_PHASE_CALLBACK_REGISTER_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PORT_OUTSIDE_MODULE_=" << sc_core::SC_ID_PORT_OUTSIDE_MODULE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PROCESS_ALREADY_UNWINDING_=" << sc_core::SC_ID_PROCESS_ALREADY_UNWINDING_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_PROCESS_CONTROL_CORNER_CASE_="
            << sc_core::SC_ID_PROCESS_CONTROL_CORNER_CASE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_REGISTER_ID_FAILED_=" << sc_core::SC_ID_REGISTER_ID_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_REGISTER_ID_FAILED_=" << sc_core::SC_ID_REGISTER_ID_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_REMOVE_MODULE_=" << sc_core::SC_ID_REMOVE_MODULE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_REMOVE_PORT_=" << sc_core::SC_ID_REMOVE_PORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_REMOVE_PRIM_CHANNEL_=" << sc_core::SC_ID_REMOVE_PRIM_CHANNEL_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_RESET_PROCESS_WHILE_NOT_RUNNING_="
            << sc_core::SC_ID_RESET_PROCESS_WHILE_NOT_RUNNING_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_RESOLVED_PORT_NOT_BOUND_=" << sc_core::SC_ID_RESOLVED_PORT_NOT_BOUND_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_RETHROW_UNWINDING_=" << sc_core::SC_ID_RETHROW_UNWINDING_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_BV_CANNOT_CONTAIN_X_AND_Z_="
            << sc_core::SC_ID_SC_BV_CANNOT_CONTAIN_X_AND_Z_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_EXPORT_ALREADY_BOUND_=" << sc_core::SC_ID_SC_EXPORT_ALREADY_BOUND_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_EXPORT_HAS_NO_INTERFACE_=" << sc_core::SC_ID_SC_EXPORT_HAS_NO_INTERFACE_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_EXPORT_NOT_BOUND_AFTER_CONSTRUCTION_="
            << sc_core::SC_ID_SC_EXPORT_NOT_BOUND_AFTER_CONSTRUCTION_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_EXPORT_NOT_REGISTERED_=" << sc_core::SC_ID_SC_EXPORT_NOT_REGISTERED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_MODULE_NAME_REQUIRED_=" << sc_core::SC_ID_SC_MODULE_NAME_REQUIRED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SC_MODULE_NAME_USE_=" << sc_core::SC_ID_SC_MODULE_NAME_USE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SET_DEFAULT_TIME_UNIT_=" << sc_core::SC_ID_SET_DEFAULT_TIME_UNIT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SET_STACK_SIZE_=" << sc_core::SC_ID_SET_STACK_SIZE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SET_TIME_RESOLUTION_=" << sc_core::SC_ID_SET_TIME_RESOLUTION_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SIMULATION_START_AFTER_ERROR_="
            << sc_core::SC_ID_SIMULATION_START_AFTER_ERROR_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SIMULATION_START_AFTER_STOP_="
            << sc_core::SC_ID_SIMULATION_START_AFTER_STOP_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SIMULATION_START_UNEXPECTED_="
            << sc_core::SC_ID_SIMULATION_START_UNEXPECTED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SIMULATION_STOP_CALLED_TWICE_="
            << sc_core::SC_ID_SIMULATION_STOP_CALLED_TWICE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SIMULATION_TIME_OVERFLOW_=" << sc_core::SC_ID_SIMULATION_TIME_OVERFLOW_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_SIMULATION_UNCAUGHT_EXCEPTION_="
            << sc_core::SC_ID_SIMULATION_UNCAUGHT_EXCEPTION_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_STACK_SETUP_FAILED_=" << sc_core::SC_ID_STACK_SETUP_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_STOP_MODE_AFTER_START_=" << sc_core::SC_ID_STOP_MODE_AFTER_START_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_STRING_TOO_LONG_=" << sc_core::SC_ID_STRING_TOO_LONG_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_THROW_IT_IGNORED_=" << sc_core::SC_ID_THROW_IT_IGNORED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_THROW_IT_WHILE_NOT_RUNNING_=" << sc_core::SC_ID_THROW_IT_WHILE_NOT_RUNNING_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TIME_CONVERSION_FAILED_=" << sc_core::SC_ID_TIME_CONVERSION_FAILED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_ALREADY_INITIALIZED_="
            << sc_core::SC_ID_TRACING_ALREADY_INITIALIZED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_CLOSE_EMPTY_FILE_=" << sc_core::SC_ID_TRACING_CLOSE_EMPTY_FILE_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_FOPEN_FAILED_=" << sc_core::SC_ID_TRACING_FOPEN_FAILED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_INVALID_ENUM_VALUE_=" << sc_core::SC_ID_TRACING_INVALID_ENUM_VALUE_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_INVALID_TIMESCALE_UNIT_="
            << sc_core::SC_ID_TRACING_INVALID_TIMESCALE_UNIT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_OBJECT_IGNORED_=" << sc_core::SC_ID_TRACING_OBJECT_IGNORED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_OBJECT_NAME_FILTERED_="
            << sc_core::SC_ID_TRACING_OBJECT_NAME_FILTERED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_REVERSED_TIME_=" << sc_core::SC_ID_TRACING_REVERSED_TIME_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_TIMESCALE_DEFAULT_=" << sc_core::SC_ID_TRACING_TIMESCALE_DEFAULT_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_TIMESCALE_UNIT_=" << sc_core::SC_ID_TRACING_TIMESCALE_UNIT_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_VCD_DELTA_CYCLE_=" << sc_core::SC_ID_TRACING_VCD_DELTA_CYCLE_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_TRACING_VCD_TIME_RESOLUTION_="
            << sc_core::SC_ID_TRACING_VCD_TIME_RESOLUTION_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_UNKNOWN_ERROR_=" << sc_core::SC_ID_UNKNOWN_ERROR_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_UNKNOWN_PROCESS_TYPE_=" << sc_core::SC_ID_UNKNOWN_PROCESS_TYPE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VALUE_NOT_VALID_=" << sc_core::SC_ID_VALUE_NOT_VALID_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VC6_MAX_PROCESSES_EXCEEDED_=" << sc_core::SC_ID_VC6_MAX_PROCESSES_EXCEEDED_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VC6_PROCESS_HELPER_=" << sc_core::SC_ID_VC6_PROCESS_HELPER_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VECTOR_BIND_EMPTY_=" << sc_core::SC_ID_VECTOR_BIND_EMPTY_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_="
            << sc_core::SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VECTOR_INIT_CALLED_TWICE_=" << sc_core::SC_ID_VECTOR_INIT_CALLED_TWICE_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VECTOR_NONOBJECT_ELEMENTS_=" << sc_core::SC_ID_VECTOR_NONOBJECT_ELEMENTS_
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VECTOR_TOO_LONG_=" << sc_core::SC_ID_VECTOR_TOO_LONG_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_VECTOR_TOO_SHORT_=" << sc_core::SC_ID_VECTOR_TOO_SHORT_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WAIT_DURING_UNWINDING_=" << sc_core::SC_ID_WAIT_DURING_UNWINDING_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WAIT_NEGATIVE_CYCLES_=" << sc_core::SC_ID_WAIT_NEGATIVE_CYCLES_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WAIT_NOT_ALLOWED_=" << sc_core::SC_ID_WAIT_NOT_ALLOWED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WAIT_N_INVALID_=" << sc_core::SC_ID_WAIT_N_INVALID_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WATCHING_NOT_ALLOWED_=" << sc_core::SC_ID_WATCHING_NOT_ALLOWED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WITHOUT_MESSAGE_=" << sc_core::SC_ID_WITHOUT_MESSAGE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WRAP_SM_NOT_DEFINED_=" << sc_core::SC_ID_WRAP_SM_NOT_DEFINED_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_WRONG_VALUE_=" << sc_core::SC_ID_WRONG_VALUE_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ID_ZERO_LENGTH_=" << sc_core::SC_ID_ZERO_LENGTH_ << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_INCLUDE_DESCENDANTS="
            << static_cast<long long>(sc_core::SC_INCLUDE_DESCENDANTS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_INFO=" << static_cast<long long>(sc_core::SC_INFO) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_INTERRUPT=" << static_cast<long long>(sc_core::SC_INTERRUPT) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_INVALID_DESCENDANTS="
            << static_cast<long long>(sc_core::SC_INVALID_DESCENDANTS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_LOG=" << static_cast<long long>(sc_core::SC_LOG) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_LOW=" << static_cast<long long>(sc_core::SC_LOW) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_MANY_WRITERS=" << static_cast<long long>(sc_core::SC_MANY_WRITERS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_MAX_SEVERITY=" << static_cast<long long>(sc_core::SC_MAX_SEVERITY) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_MEDIUM=" << static_cast<long long>(sc_core::SC_MEDIUM) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_METHOD_PROC_=" << static_cast<long long>(sc_core::SC_METHOD_PROC_) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_MS=" << static_cast<long long>(sc_core::SC_MS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_NONE=" << static_cast<long long>(sc_core::SC_NONE) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_NO_DESCENDANTS=" << static_cast<long long>(sc_core::SC_NO_DESCENDANTS)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_NO_PROC_=" << static_cast<long long>(sc_core::SC_NO_PROC_) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_NS=" << static_cast<long long>(sc_core::SC_NS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ONE_OR_MORE_BOUND=" << static_cast<long long>(sc_core::SC_ONE_OR_MORE_BOUND)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ONE_WRITER=" << static_cast<long long>(sc_core::SC_ONE_WRITER) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_PAUSED=" << static_cast<long long>(sc_core::SC_PAUSED) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_PS=" << static_cast<long long>(sc_core::SC_PS) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_RUNNING=" << static_cast<long long>(sc_core::SC_RUNNING) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_RUN_TO_TIME=" << static_cast<long long>(sc_core::SC_RUN_TO_TIME) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_SEC=" << static_cast<long long>(sc_core::SC_SEC) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_SIM_ERROR=" << sc_core::SC_SIM_ERROR << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_SIM_OK=" << sc_core::SC_SIM_OK << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_SIM_USER_STOP=" << sc_core::SC_SIM_USER_STOP << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_START_OF_SIMULATION="
            << static_cast<long long>(sc_core::SC_START_OF_SIMULATION) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_STATUS_ANY=" << static_cast<long long>(sc_core::SC_STATUS_ANY) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_STATUS_LAST=" << static_cast<long long>(sc_core::SC_STATUS_LAST) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_STOP=" << static_cast<long long>(sc_core::SC_STOP) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_STOPPED=" << static_cast<long long>(sc_core::SC_STOPPED) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_STOP_FINISH_DELTA=" << static_cast<long long>(sc_core::SC_STOP_FINISH_DELTA)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_STOP_IMMEDIATE=" << static_cast<long long>(sc_core::SC_STOP_IMMEDIATE)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_THREAD_PROC_=" << static_cast<long long>(sc_core::SC_THREAD_PROC_) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_THROW=" << static_cast<long long>(sc_core::SC_THROW) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_UNCHECKED_WRITERS=" << static_cast<long long>(sc_core::SC_UNCHECKED_WRITERS)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_UNITIALIZED=" << static_cast<long long>(sc_core::SC_UNITIALIZED) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_UNSPECIFIED=" << static_cast<long long>(sc_core::SC_UNSPECIFIED) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_US=" << static_cast<long long>(sc_core::SC_US) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_WARNING=" << static_cast<long long>(sc_core::SC_WARNING) << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ZERO_OR_MORE_BOUND=" << static_cast<long long>(sc_core::SC_ZERO_OR_MORE_BOUND)
            << "\n";
  std::cout << "CORE_CONSTANT sc_core::SC_ZERO_TIME=" << sc_core::SC_ZERO_TIME.to_string() << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_copyright_string=" << sc_core::sc_copyright_string << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_is_prerelease=" << int(sc_core::sc_is_prerelease) << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_major=" << sc_core::sc_version_major << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_minor=" << sc_core::sc_version_minor << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_originator=" << sc_core::sc_version_originator << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_patch=" << sc_core::sc_version_patch << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_prerelease=" << sc_core::sc_version_prerelease << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_release_date=" << sc_core::sc_version_release_date << "\n";
  std::cout << "CORE_CONSTANT sc_core::sc_version_string=" << sc_core::sc_version_string << "\n";
  return 0;
}
