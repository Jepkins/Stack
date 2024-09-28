#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "stack.h"

static const size_t CAP_MULTIPLICATOR = 2;  // >= 2 !!!
static const size_t EXPONENTIAL_LIMIT = 1e6;
static const size_t MAX_CAP = 1e8;
static const size_t DUMP_MAX_DATA = 1e3;

#define CHECK_ERR(stk) {if(stack_err(stk)) return;}

typedef enum {
    EXPAND = 0,
    SHRINK = 1
} Cap_modification;

static void stack_ifneed_resize (stack_t* stk, Cap_modification mode);

void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity)
{
    assert(stk);

    stack_dtor(stk);

    stk->elm_width     = elm_width;
    stk->base_capacity = base_capacity;
    stk->capacity      = base_capacity;
    stk->size          = 0;
    stk->err           = OK;

    stk->data = calloc(base_capacity, elm_width);

    _STACK_ASSERT_(stk)
}

void stack_dtor (stack_t* stk)
{
    stk->size          = 0;
    stk->base_capacity = 0;
    stk->capacity      = 0;
    stk->elm_width     = 0;
    stk->err           = OK;

    free(stk->data);
    stk->data = nullptr;
}

stack_err_t stack_err (stack_t* stk)
{
    if (!stk)
        return STK_NULL;

    if (stk->err)
        return stk->err;

    if (!stk->elm_width)
    {
        stk->err = ELM_WIDTH_NULL;
        return ELM_WIDTH_NULL;
    }
    if (!stk->base_capacity)
    {
        stk->err = BASE_CAP_NULL;
        return BASE_CAP_NULL;
    }
    if (!stk->data)
    {
        stk->err = DATA_NULL;
        return DATA_NULL;
    }
    if (!stk->capacity)
    {
        stk->err = CAP_NULL;
        return CAP_NULL;
    }
    if (stk->size > stk->capacity)
    {
        stk->err = STACK_OVERFLOW;
        return STACK_OVERFLOW;
    }

    return OK;
}

void stack_assert (stack_t* stk, Code_position pos)
{
    stack_err_t err = stack_err(stk);
    if (err)
    {
        fprintf(stderr,
            "STACK_ASSERT (called from %s:%d in function %s):\n"
            "Assertion failed: err code = %d\n",
            pos.file, pos.line, pos.func, err
        );
        stack_dump(stk, _POS_);

        stack_dtor(stk);

        assert(0 && "STACK_ASSERT");
    }
}

void stack_dump(stack_t* stk, Code_position pos)
{
    stack_err(stk);
    fprintf(stderr, "STACK_DUMP (called from %s:%d in function %s):\n", pos.file, pos.line, pos.func);

    if (!stk)
    {
        fprintf(stderr, "stk = nullptr!!!\n\n");
        return;
    }

    fprintf(stderr, "stack_t <stk>[%p]:\n", stk);

    fprintf(stderr, "elements width = %lld\n", stk->elm_width);
    fprintf(stderr, "base capacity = %lld\n", stk->base_capacity);
    fprintf(stderr, "capacity = %lld\n", stk->capacity);
    fprintf(stderr, "size = %lld\n", stk->size);
    fprintf(stderr, "err code = %d\n", stk->err);

    if (!stk->data)
    {
        fprintf(stderr, "stk->data = nullptr!!!\n\n");
        return;
    }

    fprintf(stderr, "data[%p]: \n{\n", stk->data);
    if (stk->elm_width && stk->size)
    {
        size_t b = 0;
        const size_t max_b = std::min(DUMP_MAX_DATA, stk->elm_width*std::min(stk->size, stk->capacity));
        const int max_n_width = (int)ceil(log10((std::min(DUMP_MAX_DATA / stk->elm_width, stk->size))));

        while (b < stk->elm_width * stk->size && b < DUMP_MAX_DATA)
        {
            fprintf(stderr, "\t[%.*lld] = | ", max_n_width, b / stk->elm_width);
            for (size_t i = 0; b < stk->elm_width * stk->size && i < stk->elm_width; i++, b++)
            {
                fprintf(stderr, "%.2X ", ((unsigned char*)stk->data)[b]);
            }
            fprintf(stderr, "|\n");
        }
        if (b >= DUMP_MAX_DATA)
        {
            fprintf(stderr, "\t... (too many members)\n");
        }
    }

    fprintf(stderr, "}\n\n");
}

void stack_ifneed_resize (stack_t* stk, Cap_modification mode)
{
    _STACK_ASSERT_(stk)

    if (stk->size < stk->base_capacity)
        return;

    switch (mode)
    {
        case EXPAND:
        {
            if (stk->size == stk->capacity)
            {
                bool do_exp = stk->capacity < EXPONENTIAL_LIMIT;
                size_t new_cap = ((do_exp)? stk->capacity * CAP_MULTIPLICATOR : stk->capacity + EXPONENTIAL_LIMIT);

                if (new_cap > MAX_CAP)
                {
                    stk->err = TOO_BIG;
                    _STACK_ASSERT_(stk)
                    return;
                }

                stk->capacity = new_cap;

                void* new_ptr = realloc(stk->data, new_cap * stk->elm_width);

                if (!new_ptr)
                {
                    stk->err = REALLOC_NULL;
                    _STACK_ASSERT_(stk)
                    return;
                }
                stk->data = new_ptr;
            }

            break;
        }
        case SHRINK:
        {
            size_t size_target = 1;
            size_t new_cap = 1;

            if (stk->capacity < CAP_MULTIPLICATOR*EXPONENTIAL_LIMIT)
            {
                size_target = stk->capacity / (CAP_MULTIPLICATOR * CAP_MULTIPLICATOR);
                new_cap = stk->capacity / CAP_MULTIPLICATOR;
            }
            else
            {
                size_target = stk->capacity - 2*EXPONENTIAL_LIMIT;
                new_cap = stk->capacity - EXPONENTIAL_LIMIT;
            }

            if (stk->size <= size_target)
            {
                stk->capacity = new_cap;

                void* new_ptr = realloc(stk->data, new_cap * stk->elm_width);

                if (!new_ptr)
                {
                    stk->err = REALLOC_NULL;
                    _STACK_ASSERT_(stk)
                    return;
                }
                stk->data = new_ptr;
            }

            break;
        }
        default:
        {
            assert(0 && "stack_resize(): Cap_modification mode = <wtf?>");
        }
    }
}

void stack_push (stack_t* stk, void* value)
{
    stack_ifneed_resize (stk, EXPAND);
    _STACK_ASSERT_(stk)

    memcpy((char*)stk->data + stk->size*stk->elm_width, value, stk->elm_width);
    stk->size++;
}

void stack_pop (stack_t* stk, void* dst)
{
    _STACK_ASSERT_(stk)
    if (stk->size == 0)
    {
        stk->err = STACK_UNDERFLOW;
        _STACK_ASSERT_(stk)
        return;
    }

    memcpy(dst, (char*)stk->data + (stk->size - 1)*stk->elm_width, stk->elm_width);
    stk->size--;

    stack_ifneed_resize (stk, SHRINK);
}
