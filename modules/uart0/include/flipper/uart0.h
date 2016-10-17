#ifndef __uart0_h__
#define __uart0_h__

/* Include all types and macros exposed by the Flipper Toolbox. */
#include <flipper/core.h>
#include <flipper/usart.h>

/* Declare the virtual interface for this modules. */
extern const struct _uart uart0;

#ifdef __private_include__

/* Declare the FMR overlay for this driver. */
enum { _uart0_configure, _uart0_enable, _uart0_disable, _uart0_ready, _uart0_put, _uart0_get, _uart0_push, _uart0_pull };

/* Declare each prototype for all functions within this driver. */
void uart0_configure(void);
void uart0_enable(void);
void uart0_disable(void);
uint8_t uart0_ready(void);
void uart0_put(uint8_t byte);
uint8_t uart0_get();
void uart0_push(void *source, uint32_t length);
void uart0_pull(void *destination, uint32_t length);

#endif
#endif