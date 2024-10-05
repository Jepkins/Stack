#ifndef STACK_H
#define STACK_H

#include <cstddef>
#include "protection.h"

/// @brief defines code position (for tracking <func1 called from file:line in func2>)
typedef struct {
    const char* file;
    int line;
    const char* func;
} Code_position;
#define _POS_ {__FILE__, __LINE__, __func__}

/// @brief to stop cheaters
#define DEBUG
#ifdef NO_DATA_HASHING
    #undef NO_DATA_HASHING
#endif // NO_DATA_HASHING
// #define NO_DATA_HASHING

#ifdef DEBUG
    #define DO_HASH
    #define DO_CANARY
    #define _STACK_ASSERT_(stk) {stack_assert(stk, _POS_);}
#else // notdef DEBUG
    #define _STACK_ASSERT_(stk)
#endif // DEBUG

#ifdef DO_HASH
    #define IF_DO_HASH(...) __VA_ARGS__
#else // notdef DO_HASH
    #define IF_DO_HASH(...)
#endif // DO_HASH

#ifdef DO_CANARY
    #define IF_DO_CANARY(...) __VA_ARGS__
#else // notdef DO_CANARY
    #define IF_DO_CANARY(...)
#endif // DO_CANARY

/// @brief possible errors (checked in stack_error(), saved in stack_t)
typedef enum {
    CORRUPT_DATA    = -2,
    CORRUPT_STACK   = -1,
    OK              = 0,
    STK_NULL        = 1,
    ELM_WIDTH_NULL  = 2,
    BASE_CAP_NULL   = 3,
    DATA_NULL       = 4,
    STACK_OVERFLOW  = 5, // COOL
    TOO_BIG         = 6,
    STACK_UNDERFLOW = 7,
    REALLOC_NULL    = 8
} stack_err_t;

/// @brief stack itself
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

/// @brief               makes stack consistent
/// @param stk           pointer to a stack
/// @param elm_width     size of elements in bytes (same for all elements, can be changed only by destructing and constructing again)
/// @param base_capacity staring capacity (optional param, default value = 16)
void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity);
void stack_ctor (stack_t* stk, size_t elm_width);

/// @brief destructs stack
void stack_dtor (stack_t* stk);

/// @brief checks errors (does not return OK for inconsistent stack!)
stack_err_t stack_err (stack_t* stk);

/// @brief prints all stack fields to stderr, also shows pos fields (file:line in func) (optimal call: stack_dump(&stk, _POS_))
void stack_dump(stack_t* stk, Code_position pos);
/// @brief checks error, if not ok: calls stack_dump and finishes proccess
void stack_assert (stack_t* stk, Code_position pos);

/// @brief       copies stk->elm_width bytes from *value to stk->data
/// @param stk   pointer to a stack
/// @param value pointer to a value to save in stack (byte size must be equal to or greater than stk->elm_width, undefined behaviour otherwise)
void stack_push (stack_t* stk, void* value);

/// @brief     copies stk->elm_width bytes from stk->data to *dst
/// @param stk pointer to a stack
/// @param dst pointer to a buffer for popped value (size must be at least elm_width)
void stack_pop (stack_t* stk, void* dst);

#endif // STACK_H
