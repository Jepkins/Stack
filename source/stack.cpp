#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "stack.h"

static const size_t CAP_MULTIPLICATOR = 2;  // >= 2 !!!
static const size_t EXPONENTIAL_LIMIT = 1e4;
static const size_t MAX_CAP = 1e5;
static const size_t DUMP_MAX_DATA = 1e3;

#define CHECK_ERR(stk) {if(stack_err(stk)) return;}

#ifdef DO_HASH
static uint64_t get_stack_hash (stack_t* stk);
static void save_stack_hash (stack_t* stk);
static bool check_stack_hash (stack_t* stk);

static uint64_t get_stack_hash (stack_t* stk)
{
    if (stk)
        return get_hash(stk, sizeof(stk->data) + sizeof(stk->size) + sizeof(stk->elm_width) +
                            sizeof(stk->base_capacity) + sizeof(stk->capacity) + sizeof(stk->err));

    else
        return (uint64_t)-1;
}

static bool check_stack_hash (stack_t* stk)
{
    if (stk)
        return stk->stack_hash == get_stack_hash(stk);

    else
        return false;
}
static void save_stack_hash (stack_t* stk)
{
    if (stk)
        stk->stack_hash = get_stack_hash(stk);
}

#ifndef NO_DATA_HASHING
static bool check_data_hash (stack_t* stk);
static uint64_t get_data_hash (stack_t* stk);
static void save_data_hash (stack_t* stk);

static uint64_t get_data_hash (stack_t* stk)
{
    if (stk && stk->data)
        return get_hash((char*)stk->data IF_DO_CANARY(- CANARY_THICKNESS), stk->capacity IF_DO_CANARY(+ 2*CANARY_THICKNESS));

    else
        return (uint64_t)-1;
}

static bool check_data_hash (stack_t* stk)
{
    if (stk && stk->data)
        return stk->data_hash == get_data_hash(stk);

    else
        return false;
}
static void save_data_hash (stack_t* stk)
{
    if (stk && stk->data)
        stk->data_hash = get_data_hash(stk);
}
#endif // NO_DATA_HASHING
#endif // DO_HASH

#ifdef NO_DATA_HASHING
#define HASH_SAVE(stk) IF_DO_HASH(save_stack_hash(stk);)
#else // notdef NO_DATA_HASHING
#define HASH_SAVE(stk) IF_DO_HASH(save_stack_hash(stk); save_data_hash(stk);)
#endif // NO_DATA_HASHING

/// @brief stack_ifneed_resize param
typedef enum {
    EXPAND = 0,
    SHRINK = 1
} Cap_modification;

static void stack_ifneed_resize (stack_t* stk, Cap_modification mode);

void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity)
{
    assert(stk && "stack_ctor(nullptr, ...)");

    stack_dtor(stk);

    base_capacity = (base_capacity)? base_capacity : 16;
    stk->elm_width     = elm_width;
    stk->base_capacity = base_capacity;
    stk->capacity      = base_capacity;
    stk->size          = 0;
    stk->err           = OK;

    stk->data = calloc(1, base_capacity*elm_width IF_DO_CANARY(+ 2*CANARY_THICKNESS));

    IF_DO_CANARY
    (
        if (stk->data)
        {
            place_canary(stk->data);
            stk->data = (char*)stk->data + CANARY_THICKNESS;
            place_canary((char*)stk->data + base_capacity*elm_width);
        }
    )

    HASH_SAVE(stk)

    _STACK_ASSERT_(stk)
}
void stack_ctor (stack_t* stk, size_t elm_width)
{
    stack_ctor(stk, elm_width, 0);
}

void stack_dtor (stack_t* stk)
{
    assert(stk && "stack_dtor(nullptr)");

    stk->size          = 0;
    stk->base_capacity = 0;
    stk->capacity      = 0;
    stk->elm_width     = 0;
    stk->err           = OK;
    IF_DO_HASH
    (
        stk->stack_hash = 0;
    #ifndef NO_DATA_HASHING
        stk->data_hash = 0;
    #endif // NO_DATA_HASHING
    )

    if (!stk->data)
        return;

    free((char*)stk->data IF_DO_CANARY(- CANARY_THICKNESS));
    stk->data = nullptr;
}

stack_err_t stack_err (stack_t* stk)
{
    if (!stk)
        return STK_NULL;

    IF_DO_HASH
    (
        if(!check_stack_hash(stk))
        {
            stk->err = CORRUPT_STACK;
            return CORRUPT_STACK;
        }
    )

    if (!stk->data)
    {
        stk->err = DATA_NULL;
        return DATA_NULL;
    }

    IF_DO_HASH
    (
        #ifndef NO_DATA_HASHING
            if(!check_data_hash(stk))
            {
                stk->err = CORRUPT_DATA;
                return CORRUPT_DATA;
            }
        #endif // NO_DATA_HASHING
    )
    IF_DO_CANARY
    (
        if (!check_canary((char*)stk->data - CANARY_THICKNESS) ||
            !check_canary((char*)stk->data + stk->capacity*stk->elm_width))
        {
            stk->err = CORRUPT_DATA;
            return CORRUPT_DATA;
        }
    )

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
        printf("Assertion failed (see stderr for more info)\n");
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

    IF_DO_HASH
    (
        fprintf(stderr, "stack_hash = %8llX (real = %8llX)\n", stk->stack_hash, get_stack_hash(stk));
    #ifndef NO_DATA_HASHING
        fprintf(stderr, "data_hash = %8llX (real = %8llX)\n", stk->data_hash, get_data_hash(stk));
    #endif // NO_DATA_HASHING
    )

    if (!stk->data)
    {
        fprintf(stderr, "stk->data = nullptr!!!\n\n");
        return;
    }

    fprintf(stderr, "data[%p]: \n{\n", stk->data);

    unsigned char byte = 0;
    IF_DO_CANARY
    (
        fprintf(stderr, "can1 = ");
        for (size_t i = 0; i < CANARY_THICKNESS; i++)
        {
            byte = ((unsigned char*)stk->data - CANARY_THICKNESS)[i];
            fprintf(stderr, "%.1X%.1X ", byte & 0x0f, (byte & 0xf0) >> 4);
        }
        fprintf(stderr, "\n");
    )

    if (stk->elm_width && stk->size)
    {
        size_t b = 0;
        const size_t max_b = std::min(DUMP_MAX_DATA, stk->elm_width*std::min(stk->size, stk->capacity));
        const int max_n_width = (int)ceil(log10((std::min(DUMP_MAX_DATA / stk->elm_width, std::min(stk->size, stk->capacity)))));

        while (b < max_b)
        {
            fprintf(stderr, "\t[%.*lld] = | ", max_n_width, b / stk->elm_width);
            for (size_t i = 0; i < stk->elm_width; i++, b++)
            {
                byte = ((unsigned char*)stk->data)[b];
                fprintf(stderr, "%.1X%.1X ", byte & 0x0f, (byte & 0xf0) >> 4);
            }
            fprintf(stderr, "|\n");
        }
        if (b >= DUMP_MAX_DATA)
        {
            fprintf(stderr, "\t... (too many members)\n");
        }
    }
    IF_DO_CANARY
    (
        fprintf(stderr, "can2 = ");
        for (size_t i = 0; i < CANARY_THICKNESS; i++)
        {
            byte = ((unsigned char*)stk->data + stk->capacity*stk->elm_width)[i];
            fprintf(stderr, "%.1X%.1X ", byte & 0x0f, (byte & 0xf0) >> 4);
        }
        fprintf(stderr, "\n");
    )

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
                size_t new_cap = 0;
                if (stk->capacity == 0)
                {
                    new_cap = 16;
                }
                else
                {
                    bool do_exp = stk->capacity < EXPONENTIAL_LIMIT;
                    new_cap = ((do_exp)? stk->capacity * CAP_MULTIPLICATOR : stk->capacity + EXPONENTIAL_LIMIT);
                }

                if (new_cap > MAX_CAP)
                {
                    stk->err = TOO_BIG;
                    _STACK_ASSERT_(stk)
                    break;
                }

                stk->capacity = new_cap;

                void* new_ptr = realloc((char*)stk->data IF_DO_CANARY(- CANARY_THICKNESS), new_cap * stk->elm_width IF_DO_CANARY(+ 2*CANARY_THICKNESS));

                if (!new_ptr)
                {
                    stk->err = REALLOC_NULL;
                    _STACK_ASSERT_(stk)
                    break;
                }
                IF_DO_CANARY
                (
                    place_canary(new_ptr);
                    new_ptr = (char*)new_ptr + CANARY_THICKNESS;
                    place_canary((char*)new_ptr + new_cap * stk->elm_width);
                )
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

                void* new_ptr = realloc((char*)stk->data IF_DO_CANARY(- CANARY_THICKNESS), new_cap * stk->elm_width IF_DO_CANARY(+ 2*CANARY_THICKNESS));

                if (!new_ptr)
                {
                    stk->err = REALLOC_NULL;
                    _STACK_ASSERT_(stk)
                    break;
                }
                IF_DO_CANARY
                (
                    place_canary(new_ptr);
                    new_ptr = (char*)new_ptr + CANARY_THICKNESS;
                    place_canary((char*)new_ptr + new_cap * stk->elm_width);
                )
                stk->data = new_ptr;
            }

            break;
        }
        default:
        {
            assert(0 && "stack_resize(): Cap_modification mode = <wtf?>");
        }
    }
    HASH_SAVE(stk)
}

void stack_push (stack_t* stk, void* value)
{
    stack_ifneed_resize (stk, EXPAND);
    _STACK_ASSERT_(stk)

    memcpy((char*)stk->data + stk->size*stk->elm_width, value, stk->elm_width);
    stk->size++;

    HASH_SAVE(stk)
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

    HASH_SAVE(stk)

    stack_ifneed_resize (stk, SHRINK);
}
