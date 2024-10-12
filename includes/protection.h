#ifndef PROTECTION_H
#define PROTECTION_H

#include <stdint.h>

typedef uint64_t hash_t;

static const size_t CANARY_THICKNESS = 16; // :8 (optional)

hash_t get_hash(void* ptr, size_t len);
bool check_hash (hash_t expected, void* ptr, size_t len);

void place_canary (void* dst);
bool check_canary (void* can);

#endif // PROTECTION_H
