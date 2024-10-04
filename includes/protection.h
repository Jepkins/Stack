#ifndef PROTECTION_H
#define PROTECTION_H

#include <stdint.h>

static const size_t CANARY_THICKNESS = 24; // :8 (slightly slows down otherwise)

uint64_t get_hash(void* ptr, size_t len);

void place_canary (void* dst);
bool check_canary (void* can);

#endif // PROTECTION_H
