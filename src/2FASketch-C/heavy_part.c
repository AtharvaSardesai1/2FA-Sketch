#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include "heavy_part.h"
#include "bobhash32.h"
#include "param.h"
#include <stdio.h>

// Bucket position calculation
uint32_t CalculateBucketPos(uint32_t fp) {
    return fp & 0xFFFF;  // Use lower 16 bits
}

uint32_t CalculateBucketPos2(uint32_t fp) {
    return (fp >> 16) & 0xFFFF;  // Use upper 16 bits
}

// Calculate fingerprint and bucket position
int CalculateFP(const Elastic_2FA_HeavyPart* self, const uint8_t* key, 
               uint32_t* fp, bool use_second_hash) {
    if (!self || !self->bobhash || !key || !fp) {
        return -1;  // Error: invalid parameters
    }
    
    *fp = BOBHash32_Run(self->bobhash, key, KEY_LENGTH_4, 0);
    return use_second_hash ? 
        CalculateBucketPos2(*fp) % self->bucket_num : 
        CalculateBucketPos(*fp) % self->bucket_num;
}

Elastic_2FA_HeavyPart* Elastic_2FA_HeavyPart_create(int bucket_num) {
    if (bucket_num <= 0) {
        return NULL;
    }

    // Allocate main structure
    Elastic_2FA_HeavyPart* self = (Elastic_2FA_HeavyPart*)calloc(1, sizeof(Elastic_2FA_HeavyPart));
    if (!self) {
        return NULL;
    }

    // Allocate buckets (using calloc to zero initialize)
    self->buckets = (Bucket*)calloc(bucket_num, sizeof(Bucket));
    if (!self->buckets) {
        free(self);
        return NULL;
    }

    // Initialize BOBHash32
    self->bobhash = BOBHash32_Create(0);  // Using seed 0
    if (!self->bobhash) {
        free(self->buckets);
        free(self);
        return NULL;
    }

    self->bucket_num = bucket_num;
    self->cnt = 0;
    self->cnt_all = 0;

    return self;
}

void Elastic_2FA_HeavyPart_destroy(Elastic_2FA_HeavyPart* self) {
    if (!self) return;
    
    // Destroy the hash function
    if (self->bobhash) {
        BOBHash32_Destroy(self->bobhash);
        self->bobhash = NULL;
    }
    
    // Free the buckets
    if (self->buckets) {
        free(self->buckets);
        self->buckets = NULL;
    }
    
    // Free the main structure
    free(self);
}

void Elastic_2FA_HeavyPart_clear(Elastic_2FA_HeavyPart* self) {
    if (!self) return;
    
    // Reset counters
    self->cnt = 0;
    self->cnt_all = 0;
    
    // Clear all buckets if they exist
    if (self->buckets && self->bucket_num > 0) {
        memset(self->buckets, 0, sizeof(Bucket) * self->bucket_num);
    }
}


int Elastic_2FA_HeavyPart_quick_insert(Elastic_2FA_HeavyPart* self, const uint8_t* key, 
                                     uint32_t f, uint32_t thres_set) {
    if (!self || !key || !self->buckets || !self->bobhash) {
        return -1;  // Invalid parameters
    }

    uint32_t fp;
    int pos = CalculateFP(self, key, &fp, false);
    if (pos < 0 || pos >= self->bucket_num) {
        return -1;  // Invalid position
    }

    Bucket* bucket = &self->buckets[pos];
    int min_counter_idx = 0;
    uint32_t min_counter = UINT32_MAX;
    int empty_slot = -1;
    int matched_idx = -1;

    // First pass: check for existing key and find min counter
    for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
        if (bucket->key[i] == 0) {
            // Found an empty slot
            bucket->key[i] = fp;
            bucket->val[i] = f;
            bucket->guard_val[i] = 0;
            self->cnt++;
            self->cnt_all++;
            return 0;
        } else if (bucket->key[i] == fp) {
            // Key already exists, increment counter
            bucket->val[i] += f;
            return 0;
        } else if (bucket->val[i] < min_counter) {
            min_counter = bucket->val[i];
            min_counter_idx = i;
        }
    }

    // If there's an empty slot, use it
    if (empty_slot >= 0) {
        bucket->key[empty_slot] = fp;
        bucket->val[empty_slot] = f;
        bucket->guard_val[empty_slot] = 0;
        self->cnt++;
        self->cnt_all++;
        return 0;
    }

    // No empty slots, check if we can replace the minimum counter
    if (thres_set > 0 && min_counter > thres_set) {
        self->cnt++;
        return thres_set;
    }

    // Update guard value (using last slot for guard value)
    uint32_t guard_val = bucket->val[COUNTER_PER_BUCKET - 1];
    guard_val = (guard_val + 1) & 0x7FFFFFFF;  // Simple increment, keep highest bit clear
    bucket->val[COUNTER_PER_BUCKET - 1] = guard_val;

    // Check if we should replace the minimum counter
    if (f > min_counter) {
        // Replace the minimum counter with the new item
        bucket->key[min_counter_idx] = fp;
        bucket->val[min_counter_idx] = f;
        self->cnt_all++;
        return 0;
    }

    // Couldn't insert the item
    return -1;
}

uint32_t Elastic_2FA_HeavyPart_query(const Elastic_2FA_HeavyPart* self, const uint8_t* key, 
                                   uint32_t thres_set) {
    if (!self || !key || !self->buckets || !self->bobhash) {
        return 0;  // Invalid parameters
    }

    uint32_t fp;
    int pos = CalculateFP(self, key, &fp, false);
    if (pos < 0 || pos >= self->bucket_num) {
        return 0;  // Invalid position
    }

    uint32_t res = 0;
    const Bucket* bucket = &self->buckets[pos];
    
    // Check all counters in the bucket for matching fingerprint
    for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
        if (bucket->key[i] == fp) {
            res += bucket->val[i];
        }
    }
    
    // If we found a match, return it
    if (res > 0) {
        return res;
    }
    
    // No match found in first position, try second hash position
    pos = CalculateFP(self, key, &fp, true); // true = use second hash
    if (pos >= 0 && pos < self->bucket_num) {
        bucket = &self->buckets[pos];
        for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
            if (bucket->key[i] == fp) {
                res += bucket->val[i];
            }
        }
    }
    
    return res;
}

int Elastic_2FA_HeavyPart_get_bucket_num(const Elastic_2FA_HeavyPart* self) {
    return (self && self->buckets) ? self->bucket_num : 0;
}

double Elastic_2FA_HeavyPart_get_cnt_ratio(const Elastic_2FA_HeavyPart* self) {
    if (!self || !self->buckets || self->bucket_num == 0) {
        return 0.0;
    }
    return (double)self->cnt / (self->bucket_num * COUNTER_PER_BUCKET);
}

int Elastic_2FA_HeavyPart_get_cnt(const Elastic_2FA_HeavyPart* self) {
    return (self) ? self->cnt : 0;
}

void Elastic_2FA_HeavyPart_print_buckets(const Elastic_2FA_HeavyPart* self) {
    if (!self || !self->buckets) {
        printf("Heavy part is not initialized.\n");
        return;
    }
    
    printf("\n=== Bucket Layout ===\n");
    printf("Total buckets: %d\n", self->bucket_num);
    printf("Counters per bucket: %d\n", COUNTER_PER_BUCKET);
    printf("Total counters: %d\n", self->bucket_num * COUNTER_PER_BUCKET);
    printf("Non-zero counters: %d\n", self->cnt);
    printf("\n");
    
    for (int i = 0; i < self->bucket_num; i++) {
        const Bucket* bucket = &self->buckets[i];
        printf("Bucket %d:\n", i);
        
        for (int j = 0; j < COUNTER_PER_BUCKET; j++) {
            if (bucket->key[j] != 0 || bucket->val[j] != 0 || bucket->guard_val[j] != 0) {
                printf("  Slot %d: key=0x%08x, value=%u, guard=%u\n", 
                       j, bucket->key[j], bucket->val[j], bucket->guard_val[j]);
            } else {
                printf("  Slot %d: <empty>\n", j);
            }
        }
        printf("\n");
    }
    printf("=== End of Bucket Layout ===\n\n");
}

int Elastic_2FA_HeavyPart_insert(Elastic_2FA_HeavyPart* self, const uint8_t* key, 
                               uint8_t* swap_key, uint32_t* swap_val, uint32_t f) {
    // This is a simplified version of quick_insert that handles swap values
    if (!self || !key || !self->buckets || !self->bobhash) {
        return -1;  // Invalid parameters
    }

    // Try quick insert first
    int result = Elastic_2FA_HeavyPart_quick_insert(self, key, f, 0);
    if (result >= 0) {
        return result;  // Success or threshold reached
    }

    // If quick insert failed, we need to handle swapping
    uint32_t fp;
    int pos = CalculateFP(self, key, &fp, false);
    if (pos < 0 || pos >= self->bucket_num) {
        return -1;  // Invalid position
    }

    Bucket* bucket = &self->buckets[pos];
    uint32_t min_counter = UINT32_MAX;
    int min_counter_idx = 0;
    for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
        if (bucket->val[i] < min_counter) {
            min_counter = bucket->val[i];
            min_counter_idx = i;
        }
    }

    // If we have swap values, store the evicted key and value
    if (swap_key && swap_val) {
        memcpy(swap_key, &bucket->key[min_counter_idx], KEY_LENGTH_4);
        *swap_val = bucket->val[min_counter_idx];
    }

    // Replace the minimum counter
    bucket->key[min_counter_idx] = fp;
    bucket->val[min_counter_idx] = f;
    self->cnt_all++;
    
    return 0;  // Success with swap
}