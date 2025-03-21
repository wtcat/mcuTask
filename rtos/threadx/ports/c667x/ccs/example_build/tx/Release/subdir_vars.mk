################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
ASM_SRCS += \
../tx_thread_context_restore.asm \
../tx_thread_context_save.asm \
../tx_thread_interrupt_control.asm \
../tx_thread_schedule.asm \
../tx_thread_stack_build.asm \
../tx_thread_system_return.asm \
../tx_timer_interrupt.asm 

C_SRCS += \
../tx_block_allocate.c \
../tx_block_pool_cleanup.c \
../tx_block_pool_create.c \
../tx_block_pool_delete.c \
../tx_block_pool_info_get.c \
../tx_block_pool_initialize.c \
../tx_block_pool_performance_info_get.c \
../tx_block_pool_performance_system_info_get.c \
../tx_block_pool_prioritize.c \
../tx_block_release.c \
../tx_byte_allocate.c \
../tx_byte_pool_cleanup.c \
../tx_byte_pool_create.c \
../tx_byte_pool_delete.c \
../tx_byte_pool_info_get.c \
../tx_byte_pool_initialize.c \
../tx_byte_pool_performance_info_get.c \
../tx_byte_pool_performance_system_info_get.c \
../tx_byte_pool_prioritize.c \
../tx_byte_pool_search.c \
../tx_byte_release.c \
../tx_event_flags_cleanup.c \
../tx_event_flags_create.c \
../tx_event_flags_delete.c \
../tx_event_flags_get.c \
../tx_event_flags_info_get.c \
../tx_event_flags_initialize.c \
../tx_event_flags_performance_info_get.c \
../tx_event_flags_performance_system_info_get.c \
../tx_event_flags_set.c \
../tx_event_flags_set_notify.c \
../tx_initialize_high_level.c \
../tx_initialize_kernel_enter.c \
../tx_initialize_kernel_setup.c \
../tx_mutex_cleanup.c \
../tx_mutex_create.c \
../tx_mutex_delete.c \
../tx_mutex_get.c \
../tx_mutex_info_get.c \
../tx_mutex_initialize.c \
../tx_mutex_performance_info_get.c \
../tx_mutex_performance_system_info_get.c \
../tx_mutex_prioritize.c \
../tx_mutex_priority_change.c \
../tx_mutex_put.c \
../tx_queue_cleanup.c \
../tx_queue_create.c \
../tx_queue_delete.c \
../tx_queue_flush.c \
../tx_queue_front_send.c \
../tx_queue_info_get.c \
../tx_queue_initialize.c \
../tx_queue_performance_info_get.c \
../tx_queue_performance_system_info_get.c \
../tx_queue_prioritize.c \
../tx_queue_receive.c \
../tx_queue_send.c \
../tx_queue_send_notify.c \
../tx_semaphore_ceiling_put.c \
../tx_semaphore_cleanup.c \
../tx_semaphore_create.c \
../tx_semaphore_delete.c \
../tx_semaphore_get.c \
../tx_semaphore_info_get.c \
../tx_semaphore_initialize.c \
../tx_semaphore_performance_info_get.c \
../tx_semaphore_performance_system_info_get.c \
../tx_semaphore_prioritize.c \
../tx_semaphore_put.c \
../tx_semaphore_put_notify.c \
../tx_thread_create.c \
../tx_thread_delete.c \
../tx_thread_entry_exit_notify.c \
../tx_thread_identify.c \
../tx_thread_info_get.c \
../tx_thread_initialize.c \
../tx_thread_performance_info_get.c \
../tx_thread_performance_system_info_get.c \
../tx_thread_preemption_change.c \
../tx_thread_priority_change.c \
../tx_thread_relinquish.c \
../tx_thread_reset.c \
../tx_thread_resume.c \
../tx_thread_shell_entry.c \
../tx_thread_sleep.c \
../tx_thread_stack_analyze.c \
../tx_thread_stack_error_handler.c \
../tx_thread_stack_error_notify.c \
../tx_thread_suspend.c \
../tx_thread_system_preempt_check.c \
../tx_thread_system_resume.c \
../tx_thread_system_suspend.c \
../tx_thread_terminate.c \
../tx_thread_time_slice.c \
../tx_thread_time_slice_change.c \
../tx_thread_timeout.c \
../tx_thread_wait_abort.c \
../tx_time_get.c \
../tx_time_set.c \
../tx_timer_activate.c \
../tx_timer_change.c \
../tx_timer_create.c \
../tx_timer_deactivate.c \
../tx_timer_delete.c \
../tx_timer_expiration_process.c \
../tx_timer_info_get.c \
../tx_timer_initialize.c \
../tx_timer_performance_info_get.c \
../tx_timer_performance_system_info_get.c \
../tx_timer_system_activate.c \
../tx_timer_system_deactivate.c \
../tx_timer_thread_entry.c \
../tx_trace_buffer_full_notify.c \
../tx_trace_disable.c \
../tx_trace_enable.c \
../tx_trace_event_filter.c \
../tx_trace_event_unfilter.c \
../tx_trace_initialize.c \
../tx_trace_interrupt_control.c \
../tx_trace_isr_enter_insert.c \
../tx_trace_isr_exit_insert.c \
../tx_trace_object_register.c \
../tx_trace_object_unregister.c \
../tx_trace_user_event_insert.c \
../txe_block_allocate.c \
../txe_block_pool_create.c \
../txe_block_pool_delete.c \
../txe_block_pool_info_get.c \
../txe_block_pool_prioritize.c \
../txe_block_release.c \
../txe_byte_allocate.c \
../txe_byte_pool_create.c \
../txe_byte_pool_delete.c \
../txe_byte_pool_info_get.c \
../txe_byte_pool_prioritize.c \
../txe_byte_release.c \
../txe_event_flags_create.c \
../txe_event_flags_delete.c \
../txe_event_flags_get.c \
../txe_event_flags_info_get.c \
../txe_event_flags_set.c \
../txe_event_flags_set_notify.c \
../txe_mutex_create.c \
../txe_mutex_delete.c \
../txe_mutex_get.c \
../txe_mutex_info_get.c \
../txe_mutex_prioritize.c \
../txe_mutex_put.c \
../txe_queue_create.c \
../txe_queue_delete.c \
../txe_queue_flush.c \
../txe_queue_front_send.c \
../txe_queue_info_get.c \
../txe_queue_prioritize.c \
../txe_queue_receive.c \
../txe_queue_send.c \
../txe_queue_send_notify.c \
../txe_semaphore_ceiling_put.c \
../txe_semaphore_create.c \
../txe_semaphore_delete.c \
../txe_semaphore_get.c \
../txe_semaphore_info_get.c \
../txe_semaphore_prioritize.c \
../txe_semaphore_put.c \
../txe_semaphore_put_notify.c \
../txe_thread_create.c \
../txe_thread_delete.c \
../txe_thread_entry_exit_notify.c \
../txe_thread_info_get.c \
../txe_thread_preemption_change.c \
../txe_thread_priority_change.c \
../txe_thread_relinquish.c \
../txe_thread_reset.c \
../txe_thread_resume.c \
../txe_thread_suspend.c \
../txe_thread_terminate.c \
../txe_thread_time_slice_change.c \
../txe_thread_wait_abort.c \
../txe_timer_activate.c \
../txe_timer_change.c \
../txe_timer_create.c \
../txe_timer_deactivate.c \
../txe_timer_delete.c \
../txe_timer_info_get.c 

C_DEPS += \
./tx_block_allocate.d \
./tx_block_pool_cleanup.d \
./tx_block_pool_create.d \
./tx_block_pool_delete.d \
./tx_block_pool_info_get.d \
./tx_block_pool_initialize.d \
./tx_block_pool_performance_info_get.d \
./tx_block_pool_performance_system_info_get.d \
./tx_block_pool_prioritize.d \
./tx_block_release.d \
./tx_byte_allocate.d \
./tx_byte_pool_cleanup.d \
./tx_byte_pool_create.d \
./tx_byte_pool_delete.d \
./tx_byte_pool_info_get.d \
./tx_byte_pool_initialize.d \
./tx_byte_pool_performance_info_get.d \
./tx_byte_pool_performance_system_info_get.d \
./tx_byte_pool_prioritize.d \
./tx_byte_pool_search.d \
./tx_byte_release.d \
./tx_event_flags_cleanup.d \
./tx_event_flags_create.d \
./tx_event_flags_delete.d \
./tx_event_flags_get.d \
./tx_event_flags_info_get.d \
./tx_event_flags_initialize.d \
./tx_event_flags_performance_info_get.d \
./tx_event_flags_performance_system_info_get.d \
./tx_event_flags_set.d \
./tx_event_flags_set_notify.d \
./tx_initialize_high_level.d \
./tx_initialize_kernel_enter.d \
./tx_initialize_kernel_setup.d \
./tx_mutex_cleanup.d \
./tx_mutex_create.d \
./tx_mutex_delete.d \
./tx_mutex_get.d \
./tx_mutex_info_get.d \
./tx_mutex_initialize.d \
./tx_mutex_performance_info_get.d \
./tx_mutex_performance_system_info_get.d \
./tx_mutex_prioritize.d \
./tx_mutex_priority_change.d \
./tx_mutex_put.d \
./tx_queue_cleanup.d \
./tx_queue_create.d \
./tx_queue_delete.d \
./tx_queue_flush.d \
./tx_queue_front_send.d \
./tx_queue_info_get.d \
./tx_queue_initialize.d \
./tx_queue_performance_info_get.d \
./tx_queue_performance_system_info_get.d \
./tx_queue_prioritize.d \
./tx_queue_receive.d \
./tx_queue_send.d \
./tx_queue_send_notify.d \
./tx_semaphore_ceiling_put.d \
./tx_semaphore_cleanup.d \
./tx_semaphore_create.d \
./tx_semaphore_delete.d \
./tx_semaphore_get.d \
./tx_semaphore_info_get.d \
./tx_semaphore_initialize.d \
./tx_semaphore_performance_info_get.d \
./tx_semaphore_performance_system_info_get.d \
./tx_semaphore_prioritize.d \
./tx_semaphore_put.d \
./tx_semaphore_put_notify.d \
./tx_thread_create.d \
./tx_thread_delete.d \
./tx_thread_entry_exit_notify.d \
./tx_thread_identify.d \
./tx_thread_info_get.d \
./tx_thread_initialize.d \
./tx_thread_performance_info_get.d \
./tx_thread_performance_system_info_get.d \
./tx_thread_preemption_change.d \
./tx_thread_priority_change.d \
./tx_thread_relinquish.d \
./tx_thread_reset.d \
./tx_thread_resume.d \
./tx_thread_shell_entry.d \
./tx_thread_sleep.d \
./tx_thread_stack_analyze.d \
./tx_thread_stack_error_handler.d \
./tx_thread_stack_error_notify.d \
./tx_thread_suspend.d \
./tx_thread_system_preempt_check.d \
./tx_thread_system_resume.d \
./tx_thread_system_suspend.d \
./tx_thread_terminate.d \
./tx_thread_time_slice.d \
./tx_thread_time_slice_change.d \
./tx_thread_timeout.d \
./tx_thread_wait_abort.d \
./tx_time_get.d \
./tx_time_set.d \
./tx_timer_activate.d \
./tx_timer_change.d \
./tx_timer_create.d \
./tx_timer_deactivate.d \
./tx_timer_delete.d \
./tx_timer_expiration_process.d \
./tx_timer_info_get.d \
./tx_timer_initialize.d \
./tx_timer_performance_info_get.d \
./tx_timer_performance_system_info_get.d \
./tx_timer_system_activate.d \
./tx_timer_system_deactivate.d \
./tx_timer_thread_entry.d \
./tx_trace_buffer_full_notify.d \
./tx_trace_disable.d \
./tx_trace_enable.d \
./tx_trace_event_filter.d \
./tx_trace_event_unfilter.d \
./tx_trace_initialize.d \
./tx_trace_interrupt_control.d \
./tx_trace_isr_enter_insert.d \
./tx_trace_isr_exit_insert.d \
./tx_trace_object_register.d \
./tx_trace_object_unregister.d \
./tx_trace_user_event_insert.d \
./txe_block_allocate.d \
./txe_block_pool_create.d \
./txe_block_pool_delete.d \
./txe_block_pool_info_get.d \
./txe_block_pool_prioritize.d \
./txe_block_release.d \
./txe_byte_allocate.d \
./txe_byte_pool_create.d \
./txe_byte_pool_delete.d \
./txe_byte_pool_info_get.d \
./txe_byte_pool_prioritize.d \
./txe_byte_release.d \
./txe_event_flags_create.d \
./txe_event_flags_delete.d \
./txe_event_flags_get.d \
./txe_event_flags_info_get.d \
./txe_event_flags_set.d \
./txe_event_flags_set_notify.d \
./txe_mutex_create.d \
./txe_mutex_delete.d \
./txe_mutex_get.d \
./txe_mutex_info_get.d \
./txe_mutex_prioritize.d \
./txe_mutex_put.d \
./txe_queue_create.d \
./txe_queue_delete.d \
./txe_queue_flush.d \
./txe_queue_front_send.d \
./txe_queue_info_get.d \
./txe_queue_prioritize.d \
./txe_queue_receive.d \
./txe_queue_send.d \
./txe_queue_send_notify.d \
./txe_semaphore_ceiling_put.d \
./txe_semaphore_create.d \
./txe_semaphore_delete.d \
./txe_semaphore_get.d \
./txe_semaphore_info_get.d \
./txe_semaphore_prioritize.d \
./txe_semaphore_put.d \
./txe_semaphore_put_notify.d \
./txe_thread_create.d \
./txe_thread_delete.d \
./txe_thread_entry_exit_notify.d \
./txe_thread_info_get.d \
./txe_thread_preemption_change.d \
./txe_thread_priority_change.d \
./txe_thread_relinquish.d \
./txe_thread_reset.d \
./txe_thread_resume.d \
./txe_thread_suspend.d \
./txe_thread_terminate.d \
./txe_thread_time_slice_change.d \
./txe_thread_wait_abort.d \
./txe_timer_activate.d \
./txe_timer_change.d \
./txe_timer_create.d \
./txe_timer_deactivate.d \
./txe_timer_delete.d \
./txe_timer_info_get.d 

OBJS += \
./tx_block_allocate.obj \
./tx_block_pool_cleanup.obj \
./tx_block_pool_create.obj \
./tx_block_pool_delete.obj \
./tx_block_pool_info_get.obj \
./tx_block_pool_initialize.obj \
./tx_block_pool_performance_info_get.obj \
./tx_block_pool_performance_system_info_get.obj \
./tx_block_pool_prioritize.obj \
./tx_block_release.obj \
./tx_byte_allocate.obj \
./tx_byte_pool_cleanup.obj \
./tx_byte_pool_create.obj \
./tx_byte_pool_delete.obj \
./tx_byte_pool_info_get.obj \
./tx_byte_pool_initialize.obj \
./tx_byte_pool_performance_info_get.obj \
./tx_byte_pool_performance_system_info_get.obj \
./tx_byte_pool_prioritize.obj \
./tx_byte_pool_search.obj \
./tx_byte_release.obj \
./tx_event_flags_cleanup.obj \
./tx_event_flags_create.obj \
./tx_event_flags_delete.obj \
./tx_event_flags_get.obj \
./tx_event_flags_info_get.obj \
./tx_event_flags_initialize.obj \
./tx_event_flags_performance_info_get.obj \
./tx_event_flags_performance_system_info_get.obj \
./tx_event_flags_set.obj \
./tx_event_flags_set_notify.obj \
./tx_initialize_high_level.obj \
./tx_initialize_kernel_enter.obj \
./tx_initialize_kernel_setup.obj \
./tx_mutex_cleanup.obj \
./tx_mutex_create.obj \
./tx_mutex_delete.obj \
./tx_mutex_get.obj \
./tx_mutex_info_get.obj \
./tx_mutex_initialize.obj \
./tx_mutex_performance_info_get.obj \
./tx_mutex_performance_system_info_get.obj \
./tx_mutex_prioritize.obj \
./tx_mutex_priority_change.obj \
./tx_mutex_put.obj \
./tx_queue_cleanup.obj \
./tx_queue_create.obj \
./tx_queue_delete.obj \
./tx_queue_flush.obj \
./tx_queue_front_send.obj \
./tx_queue_info_get.obj \
./tx_queue_initialize.obj \
./tx_queue_performance_info_get.obj \
./tx_queue_performance_system_info_get.obj \
./tx_queue_prioritize.obj \
./tx_queue_receive.obj \
./tx_queue_send.obj \
./tx_queue_send_notify.obj \
./tx_semaphore_ceiling_put.obj \
./tx_semaphore_cleanup.obj \
./tx_semaphore_create.obj \
./tx_semaphore_delete.obj \
./tx_semaphore_get.obj \
./tx_semaphore_info_get.obj \
./tx_semaphore_initialize.obj \
./tx_semaphore_performance_info_get.obj \
./tx_semaphore_performance_system_info_get.obj \
./tx_semaphore_prioritize.obj \
./tx_semaphore_put.obj \
./tx_semaphore_put_notify.obj \
./tx_thread_context_restore.obj \
./tx_thread_context_save.obj \
./tx_thread_create.obj \
./tx_thread_delete.obj \
./tx_thread_entry_exit_notify.obj \
./tx_thread_identify.obj \
./tx_thread_info_get.obj \
./tx_thread_initialize.obj \
./tx_thread_interrupt_control.obj \
./tx_thread_performance_info_get.obj \
./tx_thread_performance_system_info_get.obj \
./tx_thread_preemption_change.obj \
./tx_thread_priority_change.obj \
./tx_thread_relinquish.obj \
./tx_thread_reset.obj \
./tx_thread_resume.obj \
./tx_thread_schedule.obj \
./tx_thread_shell_entry.obj \
./tx_thread_sleep.obj \
./tx_thread_stack_analyze.obj \
./tx_thread_stack_build.obj \
./tx_thread_stack_error_handler.obj \
./tx_thread_stack_error_notify.obj \
./tx_thread_suspend.obj \
./tx_thread_system_preempt_check.obj \
./tx_thread_system_resume.obj \
./tx_thread_system_return.obj \
./tx_thread_system_suspend.obj \
./tx_thread_terminate.obj \
./tx_thread_time_slice.obj \
./tx_thread_time_slice_change.obj \
./tx_thread_timeout.obj \
./tx_thread_wait_abort.obj \
./tx_time_get.obj \
./tx_time_set.obj \
./tx_timer_activate.obj \
./tx_timer_change.obj \
./tx_timer_create.obj \
./tx_timer_deactivate.obj \
./tx_timer_delete.obj \
./tx_timer_expiration_process.obj \
./tx_timer_info_get.obj \
./tx_timer_initialize.obj \
./tx_timer_interrupt.obj \
./tx_timer_performance_info_get.obj \
./tx_timer_performance_system_info_get.obj \
./tx_timer_system_activate.obj \
./tx_timer_system_deactivate.obj \
./tx_timer_thread_entry.obj \
./tx_trace_buffer_full_notify.obj \
./tx_trace_disable.obj \
./tx_trace_enable.obj \
./tx_trace_event_filter.obj \
./tx_trace_event_unfilter.obj \
./tx_trace_initialize.obj \
./tx_trace_interrupt_control.obj \
./tx_trace_isr_enter_insert.obj \
./tx_trace_isr_exit_insert.obj \
./tx_trace_object_register.obj \
./tx_trace_object_unregister.obj \
./tx_trace_user_event_insert.obj \
./txe_block_allocate.obj \
./txe_block_pool_create.obj \
./txe_block_pool_delete.obj \
./txe_block_pool_info_get.obj \
./txe_block_pool_prioritize.obj \
./txe_block_release.obj \
./txe_byte_allocate.obj \
./txe_byte_pool_create.obj \
./txe_byte_pool_delete.obj \
./txe_byte_pool_info_get.obj \
./txe_byte_pool_prioritize.obj \
./txe_byte_release.obj \
./txe_event_flags_create.obj \
./txe_event_flags_delete.obj \
./txe_event_flags_get.obj \
./txe_event_flags_info_get.obj \
./txe_event_flags_set.obj \
./txe_event_flags_set_notify.obj \
./txe_mutex_create.obj \
./txe_mutex_delete.obj \
./txe_mutex_get.obj \
./txe_mutex_info_get.obj \
./txe_mutex_prioritize.obj \
./txe_mutex_put.obj \
./txe_queue_create.obj \
./txe_queue_delete.obj \
./txe_queue_flush.obj \
./txe_queue_front_send.obj \
./txe_queue_info_get.obj \
./txe_queue_prioritize.obj \
./txe_queue_receive.obj \
./txe_queue_send.obj \
./txe_queue_send_notify.obj \
./txe_semaphore_ceiling_put.obj \
./txe_semaphore_create.obj \
./txe_semaphore_delete.obj \
./txe_semaphore_get.obj \
./txe_semaphore_info_get.obj \
./txe_semaphore_prioritize.obj \
./txe_semaphore_put.obj \
./txe_semaphore_put_notify.obj \
./txe_thread_create.obj \
./txe_thread_delete.obj \
./txe_thread_entry_exit_notify.obj \
./txe_thread_info_get.obj \
./txe_thread_preemption_change.obj \
./txe_thread_priority_change.obj \
./txe_thread_relinquish.obj \
./txe_thread_reset.obj \
./txe_thread_resume.obj \
./txe_thread_suspend.obj \
./txe_thread_terminate.obj \
./txe_thread_time_slice_change.obj \
./txe_thread_wait_abort.obj \
./txe_timer_activate.obj \
./txe_timer_change.obj \
./txe_timer_create.obj \
./txe_timer_deactivate.obj \
./txe_timer_delete.obj \
./txe_timer_info_get.obj 

ASM_DEPS += \
./tx_thread_context_restore.d \
./tx_thread_context_save.d \
./tx_thread_interrupt_control.d \
./tx_thread_schedule.d \
./tx_thread_stack_build.d \
./tx_thread_system_return.d \
./tx_timer_interrupt.d 

OBJS__QUOTED += \
"tx_block_allocate.obj" \
"tx_block_pool_cleanup.obj" \
"tx_block_pool_create.obj" \
"tx_block_pool_delete.obj" \
"tx_block_pool_info_get.obj" \
"tx_block_pool_initialize.obj" \
"tx_block_pool_performance_info_get.obj" \
"tx_block_pool_performance_system_info_get.obj" \
"tx_block_pool_prioritize.obj" \
"tx_block_release.obj" \
"tx_byte_allocate.obj" \
"tx_byte_pool_cleanup.obj" \
"tx_byte_pool_create.obj" \
"tx_byte_pool_delete.obj" \
"tx_byte_pool_info_get.obj" \
"tx_byte_pool_initialize.obj" \
"tx_byte_pool_performance_info_get.obj" \
"tx_byte_pool_performance_system_info_get.obj" \
"tx_byte_pool_prioritize.obj" \
"tx_byte_pool_search.obj" \
"tx_byte_release.obj" \
"tx_event_flags_cleanup.obj" \
"tx_event_flags_create.obj" \
"tx_event_flags_delete.obj" \
"tx_event_flags_get.obj" \
"tx_event_flags_info_get.obj" \
"tx_event_flags_initialize.obj" \
"tx_event_flags_performance_info_get.obj" \
"tx_event_flags_performance_system_info_get.obj" \
"tx_event_flags_set.obj" \
"tx_event_flags_set_notify.obj" \
"tx_initialize_high_level.obj" \
"tx_initialize_kernel_enter.obj" \
"tx_initialize_kernel_setup.obj" \
"tx_mutex_cleanup.obj" \
"tx_mutex_create.obj" \
"tx_mutex_delete.obj" \
"tx_mutex_get.obj" \
"tx_mutex_info_get.obj" \
"tx_mutex_initialize.obj" \
"tx_mutex_performance_info_get.obj" \
"tx_mutex_performance_system_info_get.obj" \
"tx_mutex_prioritize.obj" \
"tx_mutex_priority_change.obj" \
"tx_mutex_put.obj" \
"tx_queue_cleanup.obj" \
"tx_queue_create.obj" \
"tx_queue_delete.obj" \
"tx_queue_flush.obj" \
"tx_queue_front_send.obj" \
"tx_queue_info_get.obj" \
"tx_queue_initialize.obj" \
"tx_queue_performance_info_get.obj" \
"tx_queue_performance_system_info_get.obj" \
"tx_queue_prioritize.obj" \
"tx_queue_receive.obj" \
"tx_queue_send.obj" \
"tx_queue_send_notify.obj" \
"tx_semaphore_ceiling_put.obj" \
"tx_semaphore_cleanup.obj" \
"tx_semaphore_create.obj" \
"tx_semaphore_delete.obj" \
"tx_semaphore_get.obj" \
"tx_semaphore_info_get.obj" \
"tx_semaphore_initialize.obj" \
"tx_semaphore_performance_info_get.obj" \
"tx_semaphore_performance_system_info_get.obj" \
"tx_semaphore_prioritize.obj" \
"tx_semaphore_put.obj" \
"tx_semaphore_put_notify.obj" \
"tx_thread_context_restore.obj" \
"tx_thread_context_save.obj" \
"tx_thread_create.obj" \
"tx_thread_delete.obj" \
"tx_thread_entry_exit_notify.obj" \
"tx_thread_identify.obj" \
"tx_thread_info_get.obj" \
"tx_thread_initialize.obj" \
"tx_thread_interrupt_control.obj" \
"tx_thread_performance_info_get.obj" \
"tx_thread_performance_system_info_get.obj" \
"tx_thread_preemption_change.obj" \
"tx_thread_priority_change.obj" \
"tx_thread_relinquish.obj" \
"tx_thread_reset.obj" \
"tx_thread_resume.obj" \
"tx_thread_schedule.obj" \
"tx_thread_shell_entry.obj" \
"tx_thread_sleep.obj" \
"tx_thread_stack_analyze.obj" \
"tx_thread_stack_build.obj" \
"tx_thread_stack_error_handler.obj" \
"tx_thread_stack_error_notify.obj" \
"tx_thread_suspend.obj" \
"tx_thread_system_preempt_check.obj" \
"tx_thread_system_resume.obj" \
"tx_thread_system_return.obj" \
"tx_thread_system_suspend.obj" \
"tx_thread_terminate.obj" \
"tx_thread_time_slice.obj" \
"tx_thread_time_slice_change.obj" \
"tx_thread_timeout.obj" \
"tx_thread_wait_abort.obj" \
"tx_time_get.obj" \
"tx_time_set.obj" \
"tx_timer_activate.obj" \
"tx_timer_change.obj" \
"tx_timer_create.obj" \
"tx_timer_deactivate.obj" \
"tx_timer_delete.obj" \
"tx_timer_expiration_process.obj" \
"tx_timer_info_get.obj" \
"tx_timer_initialize.obj" \
"tx_timer_interrupt.obj" \
"tx_timer_performance_info_get.obj" \
"tx_timer_performance_system_info_get.obj" \
"tx_timer_system_activate.obj" \
"tx_timer_system_deactivate.obj" \
"tx_timer_thread_entry.obj" \
"tx_trace_buffer_full_notify.obj" \
"tx_trace_disable.obj" \
"tx_trace_enable.obj" \
"tx_trace_event_filter.obj" \
"tx_trace_event_unfilter.obj" \
"tx_trace_initialize.obj" \
"tx_trace_interrupt_control.obj" \
"tx_trace_isr_enter_insert.obj" \
"tx_trace_isr_exit_insert.obj" \
"tx_trace_object_register.obj" \
"tx_trace_object_unregister.obj" \
"tx_trace_user_event_insert.obj" \
"txe_block_allocate.obj" \
"txe_block_pool_create.obj" \
"txe_block_pool_delete.obj" \
"txe_block_pool_info_get.obj" \
"txe_block_pool_prioritize.obj" \
"txe_block_release.obj" \
"txe_byte_allocate.obj" \
"txe_byte_pool_create.obj" \
"txe_byte_pool_delete.obj" \
"txe_byte_pool_info_get.obj" \
"txe_byte_pool_prioritize.obj" \
"txe_byte_release.obj" \
"txe_event_flags_create.obj" \
"txe_event_flags_delete.obj" \
"txe_event_flags_get.obj" \
"txe_event_flags_info_get.obj" \
"txe_event_flags_set.obj" \
"txe_event_flags_set_notify.obj" \
"txe_mutex_create.obj" \
"txe_mutex_delete.obj" \
"txe_mutex_get.obj" \
"txe_mutex_info_get.obj" \
"txe_mutex_prioritize.obj" \
"txe_mutex_put.obj" \
"txe_queue_create.obj" \
"txe_queue_delete.obj" \
"txe_queue_flush.obj" \
"txe_queue_front_send.obj" \
"txe_queue_info_get.obj" \
"txe_queue_prioritize.obj" \
"txe_queue_receive.obj" \
"txe_queue_send.obj" \
"txe_queue_send_notify.obj" \
"txe_semaphore_ceiling_put.obj" \
"txe_semaphore_create.obj" \
"txe_semaphore_delete.obj" \
"txe_semaphore_get.obj" \
"txe_semaphore_info_get.obj" \
"txe_semaphore_prioritize.obj" \
"txe_semaphore_put.obj" \
"txe_semaphore_put_notify.obj" \
"txe_thread_create.obj" \
"txe_thread_delete.obj" \
"txe_thread_entry_exit_notify.obj" \
"txe_thread_info_get.obj" \
"txe_thread_preemption_change.obj" \
"txe_thread_priority_change.obj" \
"txe_thread_relinquish.obj" \
"txe_thread_reset.obj" \
"txe_thread_resume.obj" \
"txe_thread_suspend.obj" \
"txe_thread_terminate.obj" \
"txe_thread_time_slice_change.obj" \
"txe_thread_wait_abort.obj" \
"txe_timer_activate.obj" \
"txe_timer_change.obj" \
"txe_timer_create.obj" \
"txe_timer_deactivate.obj" \
"txe_timer_delete.obj" \
"txe_timer_info_get.obj" 

C_DEPS__QUOTED += \
"tx_block_allocate.d" \
"tx_block_pool_cleanup.d" \
"tx_block_pool_create.d" \
"tx_block_pool_delete.d" \
"tx_block_pool_info_get.d" \
"tx_block_pool_initialize.d" \
"tx_block_pool_performance_info_get.d" \
"tx_block_pool_performance_system_info_get.d" \
"tx_block_pool_prioritize.d" \
"tx_block_release.d" \
"tx_byte_allocate.d" \
"tx_byte_pool_cleanup.d" \
"tx_byte_pool_create.d" \
"tx_byte_pool_delete.d" \
"tx_byte_pool_info_get.d" \
"tx_byte_pool_initialize.d" \
"tx_byte_pool_performance_info_get.d" \
"tx_byte_pool_performance_system_info_get.d" \
"tx_byte_pool_prioritize.d" \
"tx_byte_pool_search.d" \
"tx_byte_release.d" \
"tx_event_flags_cleanup.d" \
"tx_event_flags_create.d" \
"tx_event_flags_delete.d" \
"tx_event_flags_get.d" \
"tx_event_flags_info_get.d" \
"tx_event_flags_initialize.d" \
"tx_event_flags_performance_info_get.d" \
"tx_event_flags_performance_system_info_get.d" \
"tx_event_flags_set.d" \
"tx_event_flags_set_notify.d" \
"tx_initialize_high_level.d" \
"tx_initialize_kernel_enter.d" \
"tx_initialize_kernel_setup.d" \
"tx_mutex_cleanup.d" \
"tx_mutex_create.d" \
"tx_mutex_delete.d" \
"tx_mutex_get.d" \
"tx_mutex_info_get.d" \
"tx_mutex_initialize.d" \
"tx_mutex_performance_info_get.d" \
"tx_mutex_performance_system_info_get.d" \
"tx_mutex_prioritize.d" \
"tx_mutex_priority_change.d" \
"tx_mutex_put.d" \
"tx_queue_cleanup.d" \
"tx_queue_create.d" \
"tx_queue_delete.d" \
"tx_queue_flush.d" \
"tx_queue_front_send.d" \
"tx_queue_info_get.d" \
"tx_queue_initialize.d" \
"tx_queue_performance_info_get.d" \
"tx_queue_performance_system_info_get.d" \
"tx_queue_prioritize.d" \
"tx_queue_receive.d" \
"tx_queue_send.d" \
"tx_queue_send_notify.d" \
"tx_semaphore_ceiling_put.d" \
"tx_semaphore_cleanup.d" \
"tx_semaphore_create.d" \
"tx_semaphore_delete.d" \
"tx_semaphore_get.d" \
"tx_semaphore_info_get.d" \
"tx_semaphore_initialize.d" \
"tx_semaphore_performance_info_get.d" \
"tx_semaphore_performance_system_info_get.d" \
"tx_semaphore_prioritize.d" \
"tx_semaphore_put.d" \
"tx_semaphore_put_notify.d" \
"tx_thread_create.d" \
"tx_thread_delete.d" \
"tx_thread_entry_exit_notify.d" \
"tx_thread_identify.d" \
"tx_thread_info_get.d" \
"tx_thread_initialize.d" \
"tx_thread_performance_info_get.d" \
"tx_thread_performance_system_info_get.d" \
"tx_thread_preemption_change.d" \
"tx_thread_priority_change.d" \
"tx_thread_relinquish.d" \
"tx_thread_reset.d" \
"tx_thread_resume.d" \
"tx_thread_shell_entry.d" \
"tx_thread_sleep.d" \
"tx_thread_stack_analyze.d" \
"tx_thread_stack_error_handler.d" \
"tx_thread_stack_error_notify.d" \
"tx_thread_suspend.d" \
"tx_thread_system_preempt_check.d" \
"tx_thread_system_resume.d" \
"tx_thread_system_suspend.d" \
"tx_thread_terminate.d" \
"tx_thread_time_slice.d" \
"tx_thread_time_slice_change.d" \
"tx_thread_timeout.d" \
"tx_thread_wait_abort.d" \
"tx_time_get.d" \
"tx_time_set.d" \
"tx_timer_activate.d" \
"tx_timer_change.d" \
"tx_timer_create.d" \
"tx_timer_deactivate.d" \
"tx_timer_delete.d" \
"tx_timer_expiration_process.d" \
"tx_timer_info_get.d" \
"tx_timer_initialize.d" \
"tx_timer_performance_info_get.d" \
"tx_timer_performance_system_info_get.d" \
"tx_timer_system_activate.d" \
"tx_timer_system_deactivate.d" \
"tx_timer_thread_entry.d" \
"tx_trace_buffer_full_notify.d" \
"tx_trace_disable.d" \
"tx_trace_enable.d" \
"tx_trace_event_filter.d" \
"tx_trace_event_unfilter.d" \
"tx_trace_initialize.d" \
"tx_trace_interrupt_control.d" \
"tx_trace_isr_enter_insert.d" \
"tx_trace_isr_exit_insert.d" \
"tx_trace_object_register.d" \
"tx_trace_object_unregister.d" \
"tx_trace_user_event_insert.d" \
"txe_block_allocate.d" \
"txe_block_pool_create.d" \
"txe_block_pool_delete.d" \
"txe_block_pool_info_get.d" \
"txe_block_pool_prioritize.d" \
"txe_block_release.d" \
"txe_byte_allocate.d" \
"txe_byte_pool_create.d" \
"txe_byte_pool_delete.d" \
"txe_byte_pool_info_get.d" \
"txe_byte_pool_prioritize.d" \
"txe_byte_release.d" \
"txe_event_flags_create.d" \
"txe_event_flags_delete.d" \
"txe_event_flags_get.d" \
"txe_event_flags_info_get.d" \
"txe_event_flags_set.d" \
"txe_event_flags_set_notify.d" \
"txe_mutex_create.d" \
"txe_mutex_delete.d" \
"txe_mutex_get.d" \
"txe_mutex_info_get.d" \
"txe_mutex_prioritize.d" \
"txe_mutex_put.d" \
"txe_queue_create.d" \
"txe_queue_delete.d" \
"txe_queue_flush.d" \
"txe_queue_front_send.d" \
"txe_queue_info_get.d" \
"txe_queue_prioritize.d" \
"txe_queue_receive.d" \
"txe_queue_send.d" \
"txe_queue_send_notify.d" \
"txe_semaphore_ceiling_put.d" \
"txe_semaphore_create.d" \
"txe_semaphore_delete.d" \
"txe_semaphore_get.d" \
"txe_semaphore_info_get.d" \
"txe_semaphore_prioritize.d" \
"txe_semaphore_put.d" \
"txe_semaphore_put_notify.d" \
"txe_thread_create.d" \
"txe_thread_delete.d" \
"txe_thread_entry_exit_notify.d" \
"txe_thread_info_get.d" \
"txe_thread_preemption_change.d" \
"txe_thread_priority_change.d" \
"txe_thread_relinquish.d" \
"txe_thread_reset.d" \
"txe_thread_resume.d" \
"txe_thread_suspend.d" \
"txe_thread_terminate.d" \
"txe_thread_time_slice_change.d" \
"txe_thread_wait_abort.d" \
"txe_timer_activate.d" \
"txe_timer_change.d" \
"txe_timer_create.d" \
"txe_timer_deactivate.d" \
"txe_timer_delete.d" \
"txe_timer_info_get.d" 

ASM_DEPS__QUOTED += \
"tx_thread_context_restore.d" \
"tx_thread_context_save.d" \
"tx_thread_interrupt_control.d" \
"tx_thread_schedule.d" \
"tx_thread_stack_build.d" \
"tx_thread_system_return.d" \
"tx_timer_interrupt.d" 

C_SRCS__QUOTED += \
"../tx_block_allocate.c" \
"../tx_block_pool_cleanup.c" \
"../tx_block_pool_create.c" \
"../tx_block_pool_delete.c" \
"../tx_block_pool_info_get.c" \
"../tx_block_pool_initialize.c" \
"../tx_block_pool_performance_info_get.c" \
"../tx_block_pool_performance_system_info_get.c" \
"../tx_block_pool_prioritize.c" \
"../tx_block_release.c" \
"../tx_byte_allocate.c" \
"../tx_byte_pool_cleanup.c" \
"../tx_byte_pool_create.c" \
"../tx_byte_pool_delete.c" \
"../tx_byte_pool_info_get.c" \
"../tx_byte_pool_initialize.c" \
"../tx_byte_pool_performance_info_get.c" \
"../tx_byte_pool_performance_system_info_get.c" \
"../tx_byte_pool_prioritize.c" \
"../tx_byte_pool_search.c" \
"../tx_byte_release.c" \
"../tx_event_flags_cleanup.c" \
"../tx_event_flags_create.c" \
"../tx_event_flags_delete.c" \
"../tx_event_flags_get.c" \
"../tx_event_flags_info_get.c" \
"../tx_event_flags_initialize.c" \
"../tx_event_flags_performance_info_get.c" \
"../tx_event_flags_performance_system_info_get.c" \
"../tx_event_flags_set.c" \
"../tx_event_flags_set_notify.c" \
"../tx_initialize_high_level.c" \
"../tx_initialize_kernel_enter.c" \
"../tx_initialize_kernel_setup.c" \
"../tx_mutex_cleanup.c" \
"../tx_mutex_create.c" \
"../tx_mutex_delete.c" \
"../tx_mutex_get.c" \
"../tx_mutex_info_get.c" \
"../tx_mutex_initialize.c" \
"../tx_mutex_performance_info_get.c" \
"../tx_mutex_performance_system_info_get.c" \
"../tx_mutex_prioritize.c" \
"../tx_mutex_priority_change.c" \
"../tx_mutex_put.c" \
"../tx_queue_cleanup.c" \
"../tx_queue_create.c" \
"../tx_queue_delete.c" \
"../tx_queue_flush.c" \
"../tx_queue_front_send.c" \
"../tx_queue_info_get.c" \
"../tx_queue_initialize.c" \
"../tx_queue_performance_info_get.c" \
"../tx_queue_performance_system_info_get.c" \
"../tx_queue_prioritize.c" \
"../tx_queue_receive.c" \
"../tx_queue_send.c" \
"../tx_queue_send_notify.c" \
"../tx_semaphore_ceiling_put.c" \
"../tx_semaphore_cleanup.c" \
"../tx_semaphore_create.c" \
"../tx_semaphore_delete.c" \
"../tx_semaphore_get.c" \
"../tx_semaphore_info_get.c" \
"../tx_semaphore_initialize.c" \
"../tx_semaphore_performance_info_get.c" \
"../tx_semaphore_performance_system_info_get.c" \
"../tx_semaphore_prioritize.c" \
"../tx_semaphore_put.c" \
"../tx_semaphore_put_notify.c" \
"../tx_thread_create.c" \
"../tx_thread_delete.c" \
"../tx_thread_entry_exit_notify.c" \
"../tx_thread_identify.c" \
"../tx_thread_info_get.c" \
"../tx_thread_initialize.c" \
"../tx_thread_performance_info_get.c" \
"../tx_thread_performance_system_info_get.c" \
"../tx_thread_preemption_change.c" \
"../tx_thread_priority_change.c" \
"../tx_thread_relinquish.c" \
"../tx_thread_reset.c" \
"../tx_thread_resume.c" \
"../tx_thread_shell_entry.c" \
"../tx_thread_sleep.c" \
"../tx_thread_stack_analyze.c" \
"../tx_thread_stack_error_handler.c" \
"../tx_thread_stack_error_notify.c" \
"../tx_thread_suspend.c" \
"../tx_thread_system_preempt_check.c" \
"../tx_thread_system_resume.c" \
"../tx_thread_system_suspend.c" \
"../tx_thread_terminate.c" \
"../tx_thread_time_slice.c" \
"../tx_thread_time_slice_change.c" \
"../tx_thread_timeout.c" \
"../tx_thread_wait_abort.c" \
"../tx_time_get.c" \
"../tx_time_set.c" \
"../tx_timer_activate.c" \
"../tx_timer_change.c" \
"../tx_timer_create.c" \
"../tx_timer_deactivate.c" \
"../tx_timer_delete.c" \
"../tx_timer_expiration_process.c" \
"../tx_timer_info_get.c" \
"../tx_timer_initialize.c" \
"../tx_timer_performance_info_get.c" \
"../tx_timer_performance_system_info_get.c" \
"../tx_timer_system_activate.c" \
"../tx_timer_system_deactivate.c" \
"../tx_timer_thread_entry.c" \
"../tx_trace_buffer_full_notify.c" \
"../tx_trace_disable.c" \
"../tx_trace_enable.c" \
"../tx_trace_event_filter.c" \
"../tx_trace_event_unfilter.c" \
"../tx_trace_initialize.c" \
"../tx_trace_interrupt_control.c" \
"../tx_trace_isr_enter_insert.c" \
"../tx_trace_isr_exit_insert.c" \
"../tx_trace_object_register.c" \
"../tx_trace_object_unregister.c" \
"../tx_trace_user_event_insert.c" \
"../txe_block_allocate.c" \
"../txe_block_pool_create.c" \
"../txe_block_pool_delete.c" \
"../txe_block_pool_info_get.c" \
"../txe_block_pool_prioritize.c" \
"../txe_block_release.c" \
"../txe_byte_allocate.c" \
"../txe_byte_pool_create.c" \
"../txe_byte_pool_delete.c" \
"../txe_byte_pool_info_get.c" \
"../txe_byte_pool_prioritize.c" \
"../txe_byte_release.c" \
"../txe_event_flags_create.c" \
"../txe_event_flags_delete.c" \
"../txe_event_flags_get.c" \
"../txe_event_flags_info_get.c" \
"../txe_event_flags_set.c" \
"../txe_event_flags_set_notify.c" \
"../txe_mutex_create.c" \
"../txe_mutex_delete.c" \
"../txe_mutex_get.c" \
"../txe_mutex_info_get.c" \
"../txe_mutex_prioritize.c" \
"../txe_mutex_put.c" \
"../txe_queue_create.c" \
"../txe_queue_delete.c" \
"../txe_queue_flush.c" \
"../txe_queue_front_send.c" \
"../txe_queue_info_get.c" \
"../txe_queue_prioritize.c" \
"../txe_queue_receive.c" \
"../txe_queue_send.c" \
"../txe_queue_send_notify.c" \
"../txe_semaphore_ceiling_put.c" \
"../txe_semaphore_create.c" \
"../txe_semaphore_delete.c" \
"../txe_semaphore_get.c" \
"../txe_semaphore_info_get.c" \
"../txe_semaphore_prioritize.c" \
"../txe_semaphore_put.c" \
"../txe_semaphore_put_notify.c" \
"../txe_thread_create.c" \
"../txe_thread_delete.c" \
"../txe_thread_entry_exit_notify.c" \
"../txe_thread_info_get.c" \
"../txe_thread_preemption_change.c" \
"../txe_thread_priority_change.c" \
"../txe_thread_relinquish.c" \
"../txe_thread_reset.c" \
"../txe_thread_resume.c" \
"../txe_thread_suspend.c" \
"../txe_thread_terminate.c" \
"../txe_thread_time_slice_change.c" \
"../txe_thread_wait_abort.c" \
"../txe_timer_activate.c" \
"../txe_timer_change.c" \
"../txe_timer_create.c" \
"../txe_timer_deactivate.c" \
"../txe_timer_delete.c" \
"../txe_timer_info_get.c" 

ASM_SRCS__QUOTED += \
"../tx_thread_context_restore.asm" \
"../tx_thread_context_save.asm" \
"../tx_thread_interrupt_control.asm" \
"../tx_thread_schedule.asm" \
"../tx_thread_stack_build.asm" \
"../tx_thread_system_return.asm" \
"../tx_timer_interrupt.asm" 


