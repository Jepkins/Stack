#ifndef PROTECTION_H
#define PROTECTION_H

#include <stdint.h>

static const size_t CANARY_THICKNESS = 24; // :8 (slightly slows down otherwise)

/// @brief     gets hash
/// @param ptr pointer to a buffer for hashing
/// @param len byte legth of a field for hashing
/// @return desired hash
uint64_t get_hash(void* ptr, size_t len);

/// @brief places canary to dst (must have at least CANARY_THICKNESS bytes)
/// @param dst pointer to a field for placing
void place_canary (void* dst);

/// @brief  checks if *can bytes are untouched
bool check_canary (void* can);

#endif // PROTECTION_H
