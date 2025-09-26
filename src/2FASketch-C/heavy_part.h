#ifndef HEAVY_PART_H
#define HEAVY_PART_H

#include <stdint.h>
#include <stdbool.h>
#include "param.h"

// Forward declaration for BOBHash32
typedef struct BOBHash32 BOBHash32;

// Counter entry inside a bucket
typedef struct {
    uint32_t fp;       // fingerprint
    uint32_t key;      // real 32-bit flow ID
    uint32_t val;      // counter
    uint32_t guard;    // guard value
} Counter;

// Bucket = fixed number of counters
typedef struct {
    Counter slots[COUNTER_PER_BUCKET];
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

int Elastic_2FA_HeavyPart_insert(Elastic_2FA_HeavyPart* self, uint32_t flow_id,
                                 uint32_t f, uint32_t thres_set);
uint32_t Elastic_2FA_HeavyPart_query(const Elastic_2FA_HeavyPart* self, uint32_t flow_id,
                                     uint32_t thres_set);

void Elastic_2FA_HeavyPart_print_buckets(const Elastic_2FA_HeavyPart* self);
int Elastic_2FA_HeavyPart_get_bucket_num(const Elastic_2FA_HeavyPart* self);
double Elastic_2FA_HeavyPart_get_cnt_ratio(const Elastic_2FA_HeavyPart* self);
int Elastic_2FA_HeavyPart_get_cnt(const Elastic_2FA_HeavyPart* self);

void Elastic_2FA_HeavyPart_get_heavy_hitters(Elastic_2FA_HeavyPart* self, int threshold,
                                             uint32_t* out_keys, uint32_t* out_vals, int* out_num);

// Bucket position helpers
uint32_t CalculateBucketPos(uint32_t fp);
uint32_t CalculateBucketPos2(uint32_t fp);
int CalculateFP(const Elastic_2FA_HeavyPart* self, uint32_t flow_id,
                uint32_t* fp, bool use_second_hash);

#endif // HEAVY_PART_H
