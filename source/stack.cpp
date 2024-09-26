#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include "stack.h"

static const size_t CAP_MULTIPLICATOR = 2;  // >= 2 !!!
static const size_t EXPONENTIAL_LIMIT = 1e4;
static const size_t MAX_CAP = 1e5;

#define CHECK_ERR(stk) {if(stack_err(stk)) return;}

static void stack_ifneed_resize (stack_t* stk, Cap_modification mode);

void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity)
{
    stack_dtor(stk);

    stk->size          = 0;
    stk->base_capacity = base_capacity;
    stk->capacity      = base_capacity;
    stk->elm_width     = elm_width;
    stk->err           = OK;

    stk->data = calloc(base_capacity, elm_width);
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

    if (!stk->data)
    {
        stk->err = DATA_NULL;
        return DATA_NULL;
    }
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
    if (!stk->capacity)
    {
        stk->err = CAP_NULL;
        return CAP_NULL;
    }
    if (stk->size > stk->capacity)
    {
        stk->err = OVERFLOW;
        return OVERFLOW;
    }

    return OK;
}

void stack_assert(stack_t* stk, const char* pos)
{
    stack_err_t err = stack_err(stk);
    if (err)
    {
        fprintf(stderr,
            "\nSTACK_ASSERT (called from %s):\n"
            "Assertion failed: err code = %d\n",
            pos, err
        );
        stack_dump(stk, "stack_assert");

        stack_dtor(stk);

        assert(0 && "STACK_ASSERT");
    }
}

void stack_dump(stack_t* stk, const char* pos)
{
    fprintf(stderr, "\nSTACK_DUMP (called from %s):\n", pos);

    if (!stk)
    {
        fprintf(stderr, "stk = nullptr!!!\n\n");
        return;
    }

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

    fprintf(stderr, "STACK BYTES: \n{\n");

    for (size_t b = 0; b < stk->elm_width * stk->size;)
    {
        fprintf(stderr, "\t| ");
        for (size_t i = 0; b < stk->elm_width * stk->size && i < stk->elm_width; i++, b++)
        {
            fprintf(stderr, "%.2X ", ((unsigned char*)stk->data)[b]);
        }
        fprintf(stderr, "|\n");
    }

    fprintf(stderr, "}\n\n");
}

void stack_ifneed_resize (stack_t* stk, Cap_modification mode)
{
    CHECK_ERR(stk)

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
                    stk->err = OVERFLOW;
                    return;
                }

                stk->capacity = new_cap;

                void* new_ptr = realloc(stk->data, new_cap * stk->elm_width);

                if (!new_ptr)
                {
                    stk->err = REALLOC_NULL;
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
    CHECK_ERR(stk)

    memcpy((char*)stk->data + stk->size*stk->elm_width, value, stk->elm_width);
    stk->size++;
}

void stack_pop (stack_t* stk, void* dst)
{
    CHECK_ERR(stk)
    if (stk->size == 0)
    {
        stk->err = POP_UNEXISTING;
        return;
    }

    memcpy(dst, (char*)stk->data + (stk->size - 1)*stk->elm_width, stk->elm_width);
    stk->size--;

    stack_ifneed_resize (stk, SHRINK);
}
