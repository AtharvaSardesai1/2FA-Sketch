#include "bobhash32.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Internal function prototypes
static uint32_t bobhash32_run(const BOBHash32 *self, const void *data, size_t len, uint32_t seed);
static void bobhash32_initialize(BOBHash32 *self, uint32_t seed);

// Implementation of the BOBHash32 algorithm
static uint32_t bobhash32_run(const BOBHash32* self, const void* data, size_t len, uint32_t seed) {
    const uint8_t* input = (const uint8_t*)data;
    uint32_t h1 = seed ^ (uint32_t)len;
    uint32_t k1 = 0;
    
    // Process 4 bytes at a time
    const uint8_t* tail = input + (len - (len % 4));
    for (const uint8_t* p = input; p + 4 <= tail; p += 4) {
        k1 = *(uint32_t*)p;
        k1 *= self->prime32[1];
        k1 = (k1 << 15) | (k1 >> 17);
        k1 *= self->prime32[2];
        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> 19);
        h1 = h1 * 5 + self->prime32[3];
    }
    
    // Process remaining bytes
    k1 = 0;
    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16;
        case 2: k1 ^= tail[1] << 8;
        case 1: k1 ^= tail[0];
                k1 *= self->prime32[1];
                k1 = (k1 << 15) | (k1 >> 17);
                k1 *= self->prime32[2];
                h1 ^= k1;
    }
    
    // Finalize
    h1 ^= (uint32_t)len;
    h1 ^= h1 >> 16;
    h1 *= self->prime32[4];
    h1 ^= h1 >> 13;
    h1 *= self->prime32[5];
    h1 ^= h1 >> 16;
    
    return h1;
}

static void bobhash32_initialize(BOBHash32* self, uint32_t seed) {
    // Initialize prime numbers
    self->prime32[0] = 0x9E3779B1;
    self->prime32[1] = 0x85EBCA77;
    self->prime32[2] = 0xC2B2AE3D;
    self->prime32[3] = 0x27D4EB2F;
    self->prime32[4] = 0x165667B1;
    self->prime32[5] = 0x9E3779B9;
    
    // Set function pointers
    self->run = bobhash32_run;
    self->initialize = bobhash32_initialize;
    
    // Set other parameters
    self->seed = seed;
    self->prime32Num = 6;
    self->prime32Byte = 24;
}

// Constructor
void BOBHash32_Init(BOBHash32 *self, uint32_t seed) {
    bobhash32_initialize(self, seed);
}

// Destructor
void BOBHash32_Free(BOBHash32 *self) {
    // Nothing to free in this implementation
    (void)self;
}

// Create a new BOBHash32 instance
BOBHash32* BOBHash32_Create(uint32_t seed) {
    BOBHash32 *hash = (BOBHash32*)malloc(sizeof(BOBHash32));
    if (hash) {
        BOBHash32_Init(hash, seed);
    }
    return hash;
}

// Destroy a BOBHash32 instance
void BOBHash32_Destroy(BOBHash32 *self) {
    if (self) {
        BOBHash32_Free(self);
        free(self);
    }
}

// Main hash function
uint32_t BOBHash32_Run(const BOBHash32 *self, const void *buf, size_t len, uint32_t seed) {
    return self->run(self, buf, len, seed);
}

// Simplified interface
uint32_t bobhash32(const void *buf, size_t len, uint32_t seed) {
    BOBHash32 hash;
    BOBHash32_Init(&hash, seed);
    return BOBHash32_Run(&hash, buf, len, seed);
}
