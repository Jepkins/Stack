#ifndef STACK_H
#define STACK_H

#include <cstddef>

typedef struct {
    const char* file;
    int line;
    const char* func;
} Code_position;
#define _POS_ {__FILE__, __LINE__, __func__}

#ifdef DEBUG
#define _DBG(...) __VA_ARGS__
#define _STACK_ASSERT_(stk) {stack_assert(stk, _POS_);}
#else
#define _DBG(...)
#define _STACK_ASSERT_(stk)
#endif

typedef enum {
    OK              = 0,
    STK_NULL        = 1,
    ELM_WIDTH_NULL  = 2,
    BASE_CAP_NULL   = 3,
    DATA_NULL       = 4,
    CAP_NULL        = 5,
    STACK_OVERFLOW  = 6, // COOL
    TOO_BIG         = 7,
    STACK_UNDERFLOW = 8, // FUCK
    REALLOC_NULL    = 9
} stack_err_t;

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

stack_err_t stack_err (stack_t* stk);

void stack_dump(stack_t* stk, Code_position pos);
void stack_assert (stack_t* stk, Code_position pos);

void stack_push (stack_t* stk, void* value);
void stack_pop (stack_t* stk, void* dst);

#endif // STACK_H
