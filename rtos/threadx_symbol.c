/*
 * Copyright 2025 wtcat
 */
#include <tx_api.h>
#include <base/symbol.h>

/* Event */
EXPORT_SYMBOL(_tx_event_flags_create);
EXPORT_SYMBOL(_tx_event_flags_delete);
EXPORT_SYMBOL(_tx_event_flags_get);
EXPORT_SYMBOL(_tx_event_flags_set);
EXPORT_SYMBOL(_tx_event_flags_set_notify);

/* Mutex */
EXPORT_SYMBOL(_tx_mutex_create);
EXPORT_SYMBOL(_tx_mutex_delete);
EXPORT_SYMBOL(_tx_mutex_get);
EXPORT_SYMBOL(_tx_mutex_prioritize);
EXPORT_SYMBOL(_tx_mutex_put);

/* Queue */
EXPORT_SYMBOL(_tx_queue_create);
EXPORT_SYMBOL(_tx_queue_delete);
EXPORT_SYMBOL(_tx_queue_flush);
EXPORT_SYMBOL(_tx_queue_receive);
EXPORT_SYMBOL(_tx_queue_send);
EXPORT_SYMBOL(_tx_queue_send_notify);
EXPORT_SYMBOL(_tx_queue_front_send);
EXPORT_SYMBOL(_tx_queue_prioritize);

/* Semaphore */
EXPORT_SYMBOL(_tx_semaphore_ceiling_put);
EXPORT_SYMBOL(_tx_semaphore_create);
EXPORT_SYMBOL(_tx_semaphore_delete);
EXPORT_SYMBOL(_tx_semaphore_get);
EXPORT_SYMBOL(_tx_semaphore_prioritize);
EXPORT_SYMBOL(_tx_semaphore_put);
EXPORT_SYMBOL(_tx_semaphore_put_notify);

/* Thread */
EXPORT_SYMBOL(_tx_thread_create);
EXPORT_SYMBOL(_tx_thread_delete);
EXPORT_SYMBOL(_tx_thread_identify);
EXPORT_SYMBOL(_tx_thread_preemption_change);
EXPORT_SYMBOL(_tx_thread_priority_change);
EXPORT_SYMBOL(_tx_thread_relinquish);
EXPORT_SYMBOL(_tx_thread_reset);
EXPORT_SYMBOL(_tx_thread_resume);
EXPORT_SYMBOL(_tx_thread_sleep);
EXPORT_SYMBOL(_tx_thread_suspend);
EXPORT_SYMBOL(_tx_thread_terminate);
EXPORT_SYMBOL(_tx_thread_time_slice_change);
EXPORT_SYMBOL(_tx_thread_wait_abort);

/* Timer */
EXPORT_SYMBOL(_tx_time_get);
EXPORT_SYMBOL(_tx_time_set);
EXPORT_SYMBOL(_tx_timer_activate);
EXPORT_SYMBOL(_tx_timer_change);
EXPORT_SYMBOL(_tx_timer_create);
EXPORT_SYMBOL(_tx_timer_deactivate);
EXPORT_SYMBOL(_tx_timer_delete);
