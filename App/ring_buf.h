//App/ring_buf.h

#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdbool.h>
#include <stdint.h>
#include "cmsis_gcc.h"

/* Single-producer single-consumer byte ring buffer.
 * Lock-free under SPSC assumption: producer only calls push,
 * consumer only calls pop. Mixing roles breaks the invariant.
 *
 * Size is fixed at compile time and must be a power of 2 for
 * efficient modulo via bitwise AND. */

#define RING_BUF_SIZE 256

typedef struct {
    uint8_t data[RING_BUF_SIZE];
    volatile uint16_t head;   /* producer writes, consumer reads */
    volatile uint16_t tail;   /* consumer writes, producer reads */
} ring_buf_t;

/* Reset to empty state. Call once before first use. */
void ring_buf_init(ring_buf_t *rb);

/* Producer side: push one byte.
 * Returns true if pushed, false if buffer was full.
 * Safe to call from ISR. */
bool ring_buf_push(ring_buf_t *rb, uint8_t byte);

/* Consumer side: pop one byte.
 * Returns true if popped (byte written to *out), false if empty.
 * Must NOT be called from ISR. */
bool ring_buf_pop(ring_buf_t *rb, uint8_t *out);

/* Query: is buffer empty?
 * Snapshot only - state can change immediately after return. */
bool ring_buf_is_empty(const ring_buf_t *rb);

#endif /* RING_BUF_H */