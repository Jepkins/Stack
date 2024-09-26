#ifndef STACK_H
#define STACK_H

#include <cstddef>

typedef enum {
    OK             = 0,
    STK_NULL       = 1,
    DATA_NULL      = 2,
    ELM_WIDTH_NULL = 3,
    BASE_CAP_NULL  = 4,
    CAP_NULL       = 5,
    OVERFLOW       = 6, // COOL
    POP_UNEXISTING = 7,
    REALLOC_NULL   = 8
} stack_err_t;

typedef enum {
    EXPAND = 0,
    SHRINK = 1
} Cap_modification;

typedef struct {
    void* data;
    size_t size;
    size_t base_capacity;
    size_t capacity;
    size_t elm_width;

    stack_err_t err;
} stack_t;

void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity);
void stack_dtor (stack_t* stk);
void stack_dump(stack_t* stk, const char* pos);

stack_err_t stack_err (stack_t* stk);
void stack_assert(stack_t* stk, const char* pos);

void stack_push (stack_t* stk, void* value);
void stack_pop (stack_t* stk, void* dst);

#endif // STACK_H
