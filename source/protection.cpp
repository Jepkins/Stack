#include <time.h>
#include <string.h>
#include "protection.h"

static const uint64_t HWORD = 0xF00DBED;

static uint64_t CANARY_VALUE = 0xBEDEDA;
static const uint64_t CANARY_HASH  = get_hash(&CANARY_VALUE, sizeof(CANARY_VALUE));

static uint64_t my64_scramble(uint64_t val);

uint64_t get_hash(void* ptr, size_t len)
{
    unsigned char* byte_ptr = (unsigned char*) ptr;
    uint64_t hash = 0;

    for (size_t i = 0; i < len; i++)
    {
        hash = 31*hash + byte_ptr[i];
        hash = my64_scramble(hash);
    }
    hash = hash ^ HWORD;
    hash = my64_scramble(hash);
    return hash;
}
bool check_hash (uint64_t expected, void* ptr, size_t len)
{
    return expected == get_hash(ptr, len);
}

static uint64_t my64_scramble(uint64_t val)
{
    uint64_t ret = ((val >> 12) ^ HWORD) | (val << 20);
    ret ^= (ret >> 15) | (ret << 17);
    return ret;
}

void place_canary (void* dst)
{
    for (size_t i = 0; i <= CANARY_THICKNESS - sizeof(CANARY_HASH); i+=sizeof(CANARY_HASH))
        memcpy((char*)dst + i, &CANARY_HASH, sizeof(CANARY_HASH));

    size_t tail = CANARY_THICKNESS % sizeof(CANARY_HASH);
    memcpy((char*)dst + CANARY_THICKNESS - tail, &CANARY_HASH, tail);
}

bool check_canary (void* can)
{
    for (size_t i = 0; i <= CANARY_THICKNESS - sizeof(CANARY_HASH); i+=sizeof(CANARY_HASH))
    {
        if (memcmp((char*)can + i, &CANARY_HASH, sizeof(CANARY_HASH)) != 0)
            return false;
    }

    size_t tail = CANARY_THICKNESS % sizeof(CANARY_HASH);
    return memcmp((char*)can + CANARY_THICKNESS - tail, &CANARY_HASH, tail) == 0;
}
