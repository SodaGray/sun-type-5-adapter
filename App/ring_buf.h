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

/**
 * @brief Initialize the ring buffer storing raw data from UART.
 *
 * Reset to empty state. Call once before first use.
 * @param rb Ring buffer to initialize
 */
void ring_buf_init(ring_buf_t *rb);

/**
 * @brief Push a byte into the ring buffer.
 *
 * Producer-side operation. Safe to call from ISR?
 * @param rb Ring buffer to push into
 * @param byte The data byte to push
 * @return False if the buffer is full and the caller decides what to do.
 */
bool ring_buf_push(ring_buf_t *rb, uint8_t byte);

/**
 * @brief Pop one byte from the ring buffer.
 *
 * Consumer-side operation.
 * @param rb Ring buffer to pop.
 * @param out Where popped data to be stored.
 * @return False if no data to pop.
 * @note Must NOT be called from ISR?
 */
bool ring_buf_pop(ring_buf_t *rb, uint8_t *out);

/**
 * @brief Check if the buffer is empty
 * @param rb Ring buffer to check.
 * @return Whether it is empty.
 * @note Snapshot only - state can change immediately after return.
 */
bool ring_buf_is_empty(const ring_buf_t *rb);

#endif /* RING_BUF_H */