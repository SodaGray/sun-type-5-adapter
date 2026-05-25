//App/ring_buf.c
//The move of pointers to head and tail has been processed here.
#include "ring_buf.h"

void ring_buf_init(ring_buf_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
}

bool ring_buf_push(ring_buf_t *rb, uint8_t byte)
{
    uint16_t next_head = (rb->head + 1) & (RING_BUF_SIZE - 1);
    if (next_head == rb->tail) return (false);
    rb->data[rb->head] = byte;
    __DMB();
    rb->head = next_head;
    return (true);
}

bool ring_buf_pop(ring_buf_t *rb, uint8_t *out)
{
    if (rb->head == rb->tail) return (false);
    *out = rb->data[rb->tail];
    __DMB();
    rb->tail = (rb->tail + 1) & (RING_BUF_SIZE - 1);
    return (true);
}

bool ring_buf_is_empty(const ring_buf_t *rb)
{
    if (rb->head == rb->tail) return (true);
    return (false);
}