/*
 * Copyright 2024 wtcat
 */

#ifndef TX_USER_H_
#define TX_USER_H_


/* Define various build options for the ThreadX port.  The application should either make changes
   here by commenting or un-commenting the conditional compilation defined OR supply the defines
   though the compiler's equivalent of the -D option.

   For maximum speed, the following should be defined:

        TX_MAX_PRIORITIES                       32
        TX_DISABLE_PREEMPTION_THRESHOLD
        TX_DISABLE_REDUNDANT_CLEARING
        TX_DISABLE_NOTIFY_CALLBACKS
        TX_NOT_INTERRUPTABLE
        TX_TIMER_PROCESS_IN_ISR
        TX_REACTIVATE_INLINE
        TX_DISABLE_STACK_FILLING
        TX_INLINE_THREAD_RESUME_SUSPEND

   For minimum size, the following should be defined:

        TX_MAX_PRIORITIES                       32
        TX_DISABLE_PREEMPTION_THRESHOLD
        TX_DISABLE_REDUNDANT_CLEARING
        TX_DISABLE_NOTIFY_CALLBACKS
        TX_NO_FILEX_POINTER
        TX_NOT_INTERRUPTABLE
        TX_TIMER_PROCESS_IN_ISR

   Of course, many of these defines reduce functionality and/or change the behavior of the
   system in ways that may not be worth the trade-off. For example, the TX_TIMER_PROCESS_IN_ISR
   results in faster and smaller code, however, it increases the amount of processing in the ISR.
   In addition, some services that are available in timers are not available from ISRs and will
   therefore return an error if this option is used. This may or may not be desirable for a
   given application.  */


/* Override various options with default values already assigned in tx_port.h. Please also refer
   to tx_port.h for descriptions on each of these options.  */


#define TX_MAX_PRIORITIES                       32
#define TX_MINIMUM_STACK                        1024
#define TX_THREAD_USER_EXTENSION                
#define TX_TIMER_THREAD_STACK_SIZE              4096
#define TX_TIMER_THREAD_PRIORITY                10


/* Define the common timer tick reference for use by other middleware components. The default
   value is 10ms (i.e. 100 ticks, defined in tx_api.h), but may be replaced by a port-specific
   version in tx_port.h or here.
   Note: the actual hardware timer value may need to be changed (usually in tx_initialize_low_level).  */

#define TX_TIMER_TICKS_PER_SECOND       (1000UL)


/* Determine if there is a FileX pointer in the thread control block.
   By default, the pointer is there for legacy/backwards compatibility.
   The pointer must also be there for applications using FileX.
   Define this to save space in the thread control block.
*/

/*
#define TX_NO_FILEX_POINTER
*/

/* Determine if timer expirations (application timers, timeouts, and tx_thread_sleep calls
   should be processed within the a system timer thread or directly in the timer ISR.
   By default, the timer thread is used. When the following is defined, the timer expiration
   processing is done directly from the timer ISR, thereby eliminating the timer thread control
   block, stack, and context switching to activate it.  */

#define TX_TIMER_PROCESS_IN_ISR


/* Determine if in-line timer reactivation should be used within the timer expiration processing.
   By default, this is disabled and a function call is used. When the following is defined,
   reactivating is performed in-line resulting in faster timer processing but slightly larger
   code size.  */

#define TX_REACTIVATE_INLINE

/* Determine is stack filling is enabled. By default, ThreadX stack filling is enabled,
   which places an 0xEF pattern in each byte of each thread's stack.  This is used by
   debuggers with ThreadX-awareness and by the ThreadX run-time stack checking feature.  */

/*
#define TX_DISABLE_STACK_FILLING
*/

/* Determine whether or not stack checking is enabled. By default, ThreadX stack checking is
   disabled. When the following is defined, ThreadX thread stack checking is enabled.  If stack
   checking is enabled (TX_ENABLE_STACK_CHECKING is defined), the TX_DISABLE_STACK_FILLING
   define is negated, thereby forcing the stack fill which is necessary for the stack checking
   logic.  */

/*
#define TX_ENABLE_STACK_CHECKING
*/

/* Determine if random number is used for stack filling. By default, ThreadX uses a fixed
   pattern for stack filling. When the following is defined, ThreadX uses a random number
   for stack filling. This is effective only when TX_ENABLE_STACK_CHECKING is defined.  */ 

/*
#define TX_ENABLE_RANDOM_NUMBER_STACK_FILLING
*/

/* Determine if preemption-threshold should be disabled. By default, preemption-threshold is
   enabled. If the application does not use preemption-threshold, it may be disabled to reduce
   code size and improve performance.  */

/*
#define TX_DISABLE_PREEMPTION_THRESHOLD
*/

/* Determine if global ThreadX variables should be cleared. If the compiler startup code clears
   the .bss section prior to ThreadX running, the define can be used to eliminate unnecessary
   clearing of ThreadX global variables.  */

#define TX_DISABLE_REDUNDANT_CLEARING

/* Determine if no timer processing is required. This option will help eliminate the timer
   processing when not needed. The user will also have to comment out the call to
   tx_timer_interrupt, which is typically made from assembly language in
   tx_initialize_low_level. Note: if TX_NO_TIMER is used, the define TX_TIMER_PROCESS_IN_ISR
   must also be used and tx_timer_initialize must be removed from ThreadX library.  */

/*
#define TX_NO_TIMER
#ifndef TX_TIMER_PROCESS_IN_ISR
#define TX_TIMER_PROCESS_IN_ISR
#endif
*/

/* Determine if the notify callback option should be disabled. By default, notify callbacks are
   enabled. If the application does not use notify callbacks, they may be disabled to reduce
   code size and improve performance.  */

/*
#define TX_DISABLE_NOTIFY_CALLBACKS
*/


/* Determine if the tx_thread_resume and tx_thread_suspend services should have their internal
   code in-line. This results in a larger image, but improves the performance of the thread
   resume and suspend services.  */

#define TX_INLINE_THREAD_RESUME_SUSPEND


/* Determine if the internal ThreadX code is non-interruptable. This results in smaller code
   size and less processing overhead, but increases the interrupt lockout time.  */

/*
#define TX_NOT_INTERRUPTABLE
*/


/* Determine if the trace event logging code should be enabled. This causes slight increases in
   code size and overhead, but provides the ability to generate system trace information which
   is available for viewing in TraceX.  */

/*
#define TX_ENABLE_EVENT_TRACE
*/


/* Determine if block pool performance gathering is required by the application. When the following is
   defined, ThreadX gathers various block pool performance information. */

/*
#define TX_BLOCK_POOL_ENABLE_PERFORMANCE_INFO
*/

/* Determine if byte pool performance gathering is required by the application. When the following is
   defined, ThreadX gathers various byte pool performance information. */

/*
#define TX_BYTE_POOL_ENABLE_PERFORMANCE_INFO
*/

/* Determine if event flags performance gathering is required by the application. When the following is
   defined, ThreadX gathers various event flags performance information. */

/*
#define TX_EVENT_FLAGS_ENABLE_PERFORMANCE_INFO
*/

/* Determine if mutex performance gathering is required by the application. When the following is
   defined, ThreadX gathers various mutex performance information. */

/*
#define TX_MUTEX_ENABLE_PERFORMANCE_INFO
*/

/* Determine if queue performance gathering is required by the application. When the following is
   defined, ThreadX gathers various queue performance information. */

/*
#define TX_QUEUE_ENABLE_PERFORMANCE_INFO
*/

/* Determine if semaphore performance gathering is required by the application. When the following is
   defined, ThreadX gathers various semaphore performance information. */

/*
#define TX_SEMAPHORE_ENABLE_PERFORMANCE_INFO
*/

/* Determine if thread performance gathering is required by the application. When the following is
   defined, ThreadX gathers various thread performance information. */

/*
#define TX_THREAD_ENABLE_PERFORMANCE_INFO
*/

/* Determine if timer performance gathering is required by the application. When the following is
   defined, ThreadX gathers various timer performance information. */

/*
#define TX_TIMER_ENABLE_PERFORMANCE_INFO
*/

/*  Override options for byte pool searches of multiple blocks. */

/*
#define TX_BYTE_POOL_MULTIPLE_BLOCK_SEARCH    20
*/

/*  Override options for byte pool search delay to avoid thrashing. */

/*
#define TX_BYTE_POOL_DELAY_VALUE              3
*/

#define TX_MSEC(n) (n * 1000 / TX_TIMER_TICKS_PER_SECOND)
#define TX_DISABLE_ERROR_CHECKING


#ifdef __cplusplus
extern "C"{
#endif

#ifdef TX_API_H
#ifdef BASEWORK_OS_ZEPHYR_OS_BASE_H_
#error "xxxx"
#endif
#include "basework/generic.h"
#include "basework/linker.h"

#if defined(__GNUC__) || defined(__clang__)
#include "basework/cleanup.h"

typedef struct TX_MUTEX_STRUCT TX_MUTEX;
DEFINE_GUARD(tx_mutex, TX_MUTEX *, tx_mutex_get(_T, 0xFFFFFFFFUL), tx_mutex_put(_T))
#endif /* __GNUC__ ||  __clang__ */

/*
 * Custom define section
 */
#define __fastcode  __rte_section(".itcm")
#define __fastbss   __rte_section(".fastbss")
#define __fastdata  __rte_section(".fastdata")

/*
 * System intitialize 
 */
struct sysinit_item {
   int (*handler)(void);
   const char *name;
};

/* system initialize order */
#define SI_DRIVER_ORDER        50
#define SI_APPLICATION_ORDER   150

#define SYSINIT(_handler, _order) \
    enum { __enum_##_handler = _order}; \
    __SYSINIT(_handler, _order)

#define __SYSINIT(_handler, _order) \
   LINKER_ROSET_ITEM_ORDERED(sysinit, struct sysinit_item, entry, _order) = { \
      .handler = _handler, \
      .name = #_handler \
   }
    
/*
 * Platform interface
 */
int printk(const char *fmt, ...) __rte_printf(1, 2);
int request_irq(int irq, void (*handler)(void *), void *arg);
int remove_irq(int irq, void (*handler)(void *), void *arg);

/*
 * uart driver
 */
int stm32_uart_open(const char *name, void **pdev);
int stm32_uart_close(void *dev);
int stm32_uart_setup(void *dev, int baudrate, int ndata, int nstop, 
    bool parity, bool odd);
int stm32_uart_write(void *dev, const char *buf, size_t len);
int stm32_uart_read(void *dev, char *buf, size_t len);
void stm32_uart_putc(char c);

#endif /* TX_API_H */

#ifdef __cplusplus
    extern "C"{
#endif
#endif /* TX_USER_H_ */

