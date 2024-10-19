#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include "stack.h"

static const size_t CAP_MULTIPLICATOR = 2;  // >= 2 !!!
static const size_t EXPONENTIAL_LIMIT = 1e6;
static const size_t MAX_CAP = 1e8;
static const size_t DUMP_MAX_DATA = 1e3;

struct stack{
    size_t size;
    size_t base_capacity;
    size_t capacity;
    size_t elm_width;
    void* data;
    stack_err_t err;

IF_DO_HASH
(
    hash_t stack_hash;
    #ifndef NO_DATA_HASHING
        hash_t data_hash;
    #endif // NO_DATA_HASHING
)
};

#define CHECK_ERR(stk) do {if(stack_err(stk)) return;} while(0)

#ifdef DO_HASH
static uint64_t get_stack_hash (stack_t* stk);
static void save_stack_hash (stack_t* stk);
static bool check_stack_hash (stack_t* stk);

static uint64_t get_stack_hash (stack_t* stk)
{
    if (!stk)
        return (uint64_t)-1;

    hash_t orig_stack_hash = stk->stack_hash;
    hash_t orig_data_hash = stk->data_hash;

    stk->stack_hash = 0;
  #ifndef NO_DATA_HASHING
    stk->data_hash = 0;
  #endif // NO_DATA_HASHING

    hash_t res = get_hash(stk, sizeof(*stk));

    stk->stack_hash = orig_stack_hash;
  #ifndef NO_DATA_HASHING
    stk->data_hash = orig_data_hash;
  #endif // NO_DATA_HASHING

    return res;
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
        return get_hash((char*)stk->data IF_DO_CANARY(- CANARY_THICKNESS), stk->capacity * stk->elm_width IF_DO_CANARY(+ 2*CANARY_THICKNESS));

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

typedef enum {
    EXPAND = 0,
    SHRINK = 1
} Cap_modification;

static void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity = DEFAULT_CAP);
static void stack_dtor (stack_t* stk);
static void stack_ifneed_resize (stack_t* stk, Cap_modification mode);
static void stack_resize(stack_t* stk, size_t new_cap);

stack_t* stack_new (size_t elm_width, size_t base_capacity)
{
    stack_t* stk = nullptr;
    stk = (stack_t*)calloc(1, sizeof(stack_t));
    if (!stk)
    {
        fprintf(stderr, "Error: allocating memory for stack_t failed.\n");
        return nullptr;
    }
    stack_ctor(stk, elm_width, base_capacity);
    return stk;
}
static void stack_ctor (stack_t* stk, size_t elm_width, size_t base_capacity)
{
    assert(stk && "stack_ctor(nullptr, ...)");

    stack_dtor(stk);

    base_capacity = (base_capacity)? base_capacity : DEFAULT_CAP;
    stk->elm_width     = elm_width;
    stk->base_capacity = base_capacity;
    stk->capacity      = base_capacity;
    stk->size          = 0;
    stk->err           = STACK_OK;

    stk->data = calloc(base_capacity*elm_width IF_DO_CANARY(+ 2*CANARY_THICKNESS), sizeof(char));

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

void stack_delete (stack_t* stk)
{
    stack_dtor(stk);
    free(stk);
}
static void stack_dtor (stack_t* stk)
{
    assert(stk && "stack_dtor(nullptr)");
    stk->size          = 0;
    stk->base_capacity = 0;
    stk->capacity      = 0;
    stk->elm_width     = 0;
    stk->err           = STACK_OK;
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
    if (!stk->data)
    {
        stk->err = DATA_NULL;
        return DATA_NULL;
    }
    if (stk->size > stk->capacity)
    {
        stk->err = STACK_OVERFLOW;
        return STACK_OVERFLOW;
    }

    return STACK_OK;
}

void stack_assert (stack_t* stk, code_position_t pos)
{
    stack_err_t err = stack_err(stk);
    if (err != STACK_OK)
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

void stack_dump(stack_t* stk, code_position_t pos)
{
    fprintf(stderr, "STACK_DUMP (called from %s:%d in function %s):\n", pos.file, pos.line, pos.func);

    if (!stk)
    {
        fprintf(stderr, "stk = nullptr!!!\n\n");
        return;
    }

    fprintf(stderr, "stack_t <stk>[%p]:\n", stk);

    fprintf(stderr, "elements width = %ld\n", stk->elm_width);
    fprintf(stderr, "base capacity = %ld\n", stk->base_capacity);
    fprintf(stderr, "capacity = %ld\n", stk->capacity);
    fprintf(stderr, "size = %ld\n", stk->size);
    fprintf(stderr, "err code = %d\n", stk->err);

    IF_DO_HASH
    (
        fprintf(stderr, "stack_hash = %8lX (real = %8lX)\n", stk->stack_hash, get_stack_hash(stk));
    #ifndef NO_DATA_HASHING
        fprintf(stderr, "data_hash = %8lX (real = %8lX)\n", stk->data_hash, get_data_hash(stk));
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
            fprintf(stderr, "%.2X ", byte);
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
            fprintf(stderr, "\t[%.*ld] = | ", max_n_width, b / stk->elm_width);
            for (size_t i = 0; i < stk->elm_width; i++, b++)
            {
                byte = ((unsigned char*)stk->data)[b];
                fprintf(stderr, "%.2X ", byte);
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
            fprintf(stderr, "%.2X ", byte);
        }
        fprintf(stderr, "\n");
    )

    fprintf(stderr, "}\n\n");
}

static void stack_ifneed_resize (stack_t* stk, Cap_modification mode)
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
                    break;
                }

                stk->capacity = new_cap;

                stack_resize(stk, new_cap);
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
                stack_resize(stk, new_cap);
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

static void stack_resize(stack_t* stk, size_t new_cap)
{
    stk->capacity = new_cap;

    void* new_ptr = realloc((char*)stk->data IF_DO_CANARY(- CANARY_THICKNESS), new_cap * stk->elm_width IF_DO_CANARY(+ 2*CANARY_THICKNESS));

    if (!new_ptr)
    {
        stk->err = REALLOC_NULL;
        _STACK_ASSERT_(stk)
    }
    IF_DO_CANARY
    (
        place_canary(new_ptr);
        new_ptr = (char*)new_ptr + CANARY_THICKNESS;
        place_canary((char*)new_ptr + new_cap * stk->elm_width);
    )
    stk->data = new_ptr;
}

void stack_push (stack_t* stk, const void* value)
{
    _STACK_ASSERT_(stk)
    assert(value && "stack_push(): &value = nullptr!!!!!\n");
    stack_ifneed_resize (stk, EXPAND);
    _STACK_ASSERT_(stk)

    memcpy((char*)stk->data + stk->size*stk->elm_width, value, stk->elm_width);
    stk->size++;

    HASH_SAVE(stk)
}

void stack_pop (stack_t* stk, void* dst)
{
    assert(dst && "stack_pop(): dst = nullptr!!!!!\n");
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
