#ifndef PARAM_H
#define PARAM_H

#include <stdint.h>

// Key length definitions
#define KEY_LENGTH_4 4
#define KEY_LENGTH_8 8
#define KEY_LENGTH_13 13

// Bucket configuration
#define MAX_VALID_COUNTER 8
#define COUNTER_PER_BUCKET 8
#define HASH_NUM 3

// Memory alignment (reduced from 64 to 8 for non-SIMD)
#define ALIGNMENT 8

// Counter manipulation macros
#define GET_COUNTER(ptr, idx) (((uint32_t*)(ptr))[idx])
#define SET_COUNTER(ptr, idx, val) (((uint32_t*)(ptr))[idx] = (val))

// Function declarations for bucket position calculation
uint32_t CalculateBucketPos(uint32_t fp);
uint32_t CalculateBucketPos2(uint32_t fp);

// Counter manipulation
#define UPDATE_GUARD_VAL(x) ((x) + 1)
#define JUDGE_IF_SWAP(min_val, guard_val) ((min_val) < (guard_val))

// Counter value manipulation
#define GET_COUNTER_VAL(val) ((val) & 0x7FFFFFFF)
#define IS_HIGHEST_BIT_SET(val) ((val) & 0x80000000)

#endif // PARAM_H