#include <time.h>
#include "protection.h"

static const uint64_t HWORD = (uint64_t)clock();

uint64_t get_hash(void* ptr, size_t len)
{
    unsigned char* byte_ptr = (unsigned char*) ptr;
    uint64_t hash = 0;

    for (size_t i = 0; i < len; i++)
    {
        hash = 31*hash + byte_ptr[i];
    }
    hash = hash ^ HWORD;
    return hash;
}
