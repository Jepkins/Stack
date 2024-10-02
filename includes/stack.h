#ifndef STACK_H
#define STACK_H

#include <cstddef>
#include "protection.h"

typedef struct {
    const char* file;
    int line;
    const char* func;
} Code_position;
#define _POS_ {__FILE__, __LINE__, __func__}

#define NO_DATA_HASHING
#ifdef DEBUG
#define DO_HASH
#define DO_CANARY
#define _DBG(...) __VA_ARGS__
#define IF_DO_CANARY(...) __VA_ARGS__
#define IF_DO_HASH(...) __VA_ARGS__
#define _STACK_ASSERT_(stk) {stack_assert(stk, _POS_);}
#else
#define _DBG(...)
#define IF_DO_CANARY(...)
#define IF_DO_HASH(...)
#define _STACK_ASSERT_(stk)
#endif

typedef enum {
    CORRUPT_DATA    = -2,
    CORRUPT_STACK   = -1,
    OK              = 0,
    STK_NULL        = 1,
    ELM_WIDTH_NULL  = 2,
    BASE_CAP_NULL   = 3,
    DATA_NULL       = 4,
    CAP_NULL        = 5,
    STACK_OVERFLOW  = 6, // COOL
    TOO_BIG         = 7,
    STACK_UNDERFLOW = 8,
    REALLOC_NULL    = 9
} stack_err_t;

typedef struct {
    size_t size;
    size_t base_capacity;
    size_t capacity;
    size_t elm_width;
    void* data;
    stack_err_t err;

IF_DO_HASH
(
    uint64_t stack_hash;
    #ifndef NO_DATA_HASHING
        uint64_t data_hash;
    #endif // NO_DATA_HASHING
)
} stack_t;

void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity);
void stack_ctor (stack_t* stk, size_t elm_width);
void stack_dtor (stack_t* stk);

stack_err_t stack_err (stack_t* stk);

void stack_dump(stack_t* stk, Code_position pos);
void stack_assert (stack_t* stk, Code_position pos);

void stack_push (stack_t* stk, void* value);
void stack_pop (stack_t* stk, void* dst);

#endif // STACK_H
