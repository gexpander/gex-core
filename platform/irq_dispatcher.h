//
// Created by MightyPork on 2018/01/14.
//
// Provides a trampoline system for redirecting IRQ calls to assigned callbacks.
//

#ifndef GEX_CORE_IRQ_DISPATCHER_H
#define GEX_CORE_IRQ_DISPATCHER_H

// Dummy peripherals for use with the
extern void * const EXTIS[16];

/**
 * Initialize the interrupt dispatcher
 */
void irqd_init(void);

/** Typedef for a IRQ callback run by the dispatcher */
typedef void (*IrqCallback)(void*);

/**
 * Attach a callback to a IRQ handler.
 * Pass NULL to remove the handler.
 *
 * NOTE: The handler is responsible for clearing any interrupt status flags.
 * NOTE: It is not guaranteed that the particular peripheral caused the interrupt when
 *       the function is called; some interrupt vectors are shared. Make sure to check the flags.
 *
 * @param periph - peripheral we're attaching to
 * @param callback - callback to fire on IRQ
 * @param data - data passed to the callback (user context)
 */
void irqd_attach(void *periph, IrqCallback callback, void *data);


/**
 * Remove a callback
 *
 * @param periph - peripheral we're attaching to
 * @param callback - callback to remove, if it doesn't match, do nothing
 * @return the arg, if any
 */
void* irqd_detach(void *periph, IrqCallback callback);

#endif //GEX_CORE_IRQ_DISPATCHER_H
