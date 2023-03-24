#pragma once

#include "kernel.h"
#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------------------------------------------
typedef struct linked_list_t {
    struct linked_list_t *prev;
    struct linked_list_t *next;
} linked_list_t;

// ------------------------------------------------------------------------------------------------
static inline void link_init(linked_list_t *x) {
    x->prev = x;
    x->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_after(linked_list_t *a, linked_list_t *x) {
    linked_list_t *p = a;
    linked_list_t *n = a->next;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_before(linked_list_t *a, linked_list_t *x) {
    linked_list_t *p = a->prev;
    linked_list_t *n = a;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_remove(linked_list_t *x) {
    linked_list_t *p = x->prev;
    linked_list_t *n = x->next;
    n->prev = p;
    p->next = n;
    x->next = 0;
    x->prev = 0;
}

// ------------------------------------------------------------------------------------------------
static inline void link_move_after(linked_list_t *a, linked_list_t *x) {
    linked_list_t *p = x->prev;
    linked_list_t *n = x->next;
    n->prev = p;
    p->next = n;

    p = a;
    n = a->next;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline void link_move_before(linked_list_t *a, linked_list_t *x) {
    linked_list_t *p = x->prev;
    linked_list_t *n = x->next;
    n->prev = p;
    p->next = n;

    p = a->prev;
    n = a;
    n->prev = x;
    x->next = n;
    x->prev = p;
    p->next = x;
}

// ------------------------------------------------------------------------------------------------
static inline bool list_is_empty(linked_list_t *x) {
    return x->next == x;
}

// ------------------------------------------------------------------------------------------------
#define link_data(link,T,m) (T *)((char *)(link) - (unsigned long)(&(((T*)0)->m)))

// ------------------------------------------------------------------------------------------------
#define list_for_each(it, list, m) \
    for (it = link_data((list).next, typeof(*it), m); \
        &it->m != &(list); \
        it = link_data(it->m.next, typeof(*it), m))

// ------------------------------------------------------------------------------------------------
#define list_for_each_safe(it, n, list, m) \
    for (it = link_data((list).next, typeof(*it), m), \
        n = link_data(it->m.next, typeof(*it), m); \
        &it->m != &(list); \
        it = n, \
        n = link_data(n->m.next, typeof(*it), m))

