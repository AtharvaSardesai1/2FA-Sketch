#ifndef HEAVY_PART_H
#define HEAVY_PART_H

#include <stdint.h>
#include <stdbool.h>
#include "param.h"

// Forward declaration for BOBHash32
typedef struct BOBHash32 BOBHash32;

// Bucket structure for counters
typedef struct {
    uint32_t key[COUNTER_PER_BUCKET];
    uint32_t val[COUNTER_PER_BUCKET];
    uint32_t guard_val[COUNTER_PER_BUCKET];
} Bucket;

typedef struct {
    Bucket* buckets;
    BOBHash32* bobhash;
    int bucket_num;
    int cnt;
    int cnt_all;
} Elastic_2FA_HeavyPart;

// Function declarations
Elastic_2FA_HeavyPart* Elastic_2FA_HeavyPart_create(int bucket_num);
void Elastic_2FA_HeavyPart_destroy(Elastic_2FA_HeavyPart* self);
void Elastic_2FA_HeavyPart_clear(Elastic_2FA_HeavyPart* self);
int Elastic_2FA_HeavyPart_insert(Elastic_2FA_HeavyPart* self, const uint8_t* key, 
                               uint8_t* swap_key, uint32_t* swap_val, uint32_t f);
int Elastic_2FA_HeavyPart_quick_insert(Elastic_2FA_HeavyPart* self, const uint8_t* key, 
                                     uint32_t f, uint32_t thres_set);
uint32_t Elastic_2FA_HeavyPart_query(const Elastic_2FA_HeavyPart* self, const uint8_t* key, 
                                   uint32_t thres_set);
void Elastic_2FA_HeavyPart_print_buckets(const Elastic_2FA_HeavyPart* self);
int Elastic_2FA_HeavyPart_get_bucket_num(const Elastic_2FA_HeavyPart* self);
double Elastic_2FA_HeavyPart_get_cnt_ratio(const Elastic_2FA_HeavyPart* self);
int Elastic_2FA_HeavyPart_get_cnt(const Elastic_2FA_HeavyPart* self);

// Function declarations for bucket position calculation
uint32_t CalculateBucketPos(uint32_t fp);
uint32_t CalculateBucketPos2(uint32_t fp);

// Calculate fingerprint and bucket position
int CalculateFP(const Elastic_2FA_HeavyPart* self, const uint8_t* key, 
               uint32_t* fp, bool use_second_hash);

#endif // HEAVY_PART_H