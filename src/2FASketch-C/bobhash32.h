#ifndef BOBHASH32_H
#define BOBHASH32_H

#include <stdint.h>
#include <stddef.h>

typedef struct BOBHash32 {
    uint32_t (*run)(const struct BOBHash32* self, const void* data, size_t len, uint32_t seed);
    void (*initialize)(struct BOBHash32* self, uint32_t seed);
    uint32_t prime32[6];  // Array of prime numbers used in hashing
    uint32_t seed;
    int prime32Num;       // Number of primes
    int prime32Byte;      // Byte size of primes
} BOBHash32;

// Function declarations
BOBHash32* BOBHash32_Create(uint32_t seed);
void BOBHash32_Destroy(BOBHash32* self);
void BOBHash32_Init(BOBHash32* self, uint32_t seed);
void BOBHash32_Free(BOBHash32* self);
uint32_t BOBHash32_Run(const BOBHash32* self, const void* buf, size_t len, uint32_t seed);
uint32_t bobhash32(const void* buf, size_t len, uint32_t seed);

#endif // BOBHASH32_H