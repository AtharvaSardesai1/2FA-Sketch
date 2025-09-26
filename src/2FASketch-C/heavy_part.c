#include "heavy_part.h"
#include "bobhash32.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

// Bucket position calculation
inline uint32_t CalculateBucketPos(uint32_t fp) {
    return fp & 0xFFFF;
}

inline uint32_t CalculateBucketPos2(uint32_t fp) {
    return (fp >> 16) & 0xFFFF;
}

// Calculate fingerprint + bucket position
inline int CalculateFP(const Elastic_2FA_HeavyPart* self, uint32_t flow_id,
                       uint32_t* fp, bool use_second_hash) {
    if (!self || !self->bobhash || !fp) return -1;
    *fp = BOBHash32_Run(self->bobhash, (uint8_t*)&flow_id, sizeof(uint32_t), 0);
    return use_second_hash ?
        CalculateBucketPos2(*fp) % self->bucket_num :
        CalculateBucketPos(*fp) % self->bucket_num;
}

Elastic_2FA_HeavyPart* Elastic_2FA_HeavyPart_create(int bucket_num) {
    Elastic_2FA_HeavyPart* self = calloc(1, sizeof(*self));
    if (!self) return NULL;
    self->buckets = calloc(bucket_num, sizeof(Bucket));
    if (!self->buckets) { free(self); return NULL; }
    self->bobhash = BOBHash32_Create(0);
    if (!self->bobhash) { free(self->buckets); free(self); return NULL; }
    self->bucket_num = bucket_num;
    return self;
}

void Elastic_2FA_HeavyPart_destroy(Elastic_2FA_HeavyPart* self) {
    if (!self) return;
    if (self->bobhash) BOBHash32_Destroy(self->bobhash);
    free(self->buckets);
    free(self);
}

void Elastic_2FA_HeavyPart_clear(Elastic_2FA_HeavyPart* self) {
    if (!self) return;
    self->cnt = 0; self->cnt_all = 0;
    memset(self->buckets, 0, sizeof(Bucket) * self->bucket_num);
}

// Insert flow_id with count f
int Elastic_2FA_HeavyPart_insert(Elastic_2FA_HeavyPart* self, uint32_t flow_id,
                                 uint32_t f, uint32_t thres_set) {
    uint32_t fp; int pos = CalculateFP(self, flow_id, &fp, false);
    if (pos < 0) return -1;
    Bucket* bucket = &self->buckets[pos];

    int min_idx = 0; uint32_t min_val = UINT_MAX; int empty = -1;

    for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
        Counter* c = &bucket->slots[i];
        if (c->key == flow_id && c->fp == fp) {
            c->val += f;
            return 0;
        }
        if (c->val == 0 && empty == -1) empty = i;
        if (c->val < min_val) { min_val = c->val; min_idx = i; }
    }

    if (empty >= 0) {
        bucket->slots[empty].fp = fp;
        bucket->slots[empty].key = flow_id;
        bucket->slots[empty].val = f;
        bucket->slots[empty].guard = 0;
        return 0;
    }

    if (thres_set > 0 && min_val >= thres_set) {
        self->cnt++;
        return thres_set;
    }

    Counter* victim = &bucket->slots[min_idx];
    victim->guard++;
    if (victim->guard > victim->val) {
        victim->fp = fp;
        victim->key = flow_id;
        victim->val = f;
        victim->guard = 0;
        self->cnt_all++;
        return 0;
    }
    return -1;
}

// Query flow_id
uint32_t Elastic_2FA_HeavyPart_query(const Elastic_2FA_HeavyPart* self, uint32_t flow_id,
                                     uint32_t thres_set) {
    uint32_t fp; int pos = CalculateFP(self, flow_id, &fp, false);
    if (pos < 0) return 0;
    const Bucket* bucket = &self->buckets[pos];

    uint32_t min_val = UINT_MAX;
    for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
        const Counter* c = &bucket->slots[i];
        if (c->key == flow_id && c->fp == fp) return c->val;
        if (c->val && c->val < min_val) min_val = c->val;
    }

    if (min_val >= thres_set) {
        pos = CalculateFP(self, flow_id, &fp, true);
        if (pos >= 0) {
            bucket = &self->buckets[pos];
            for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
                const Counter* c = &bucket->slots[i];
                if (c->key == flow_id && c->fp == fp) return c->val;
            }
        }
    }
    return 0;
}

int Elastic_2FA_HeavyPart_get_bucket_num(const Elastic_2FA_HeavyPart* self) {
    return self ? self->bucket_num : 0;
}

double Elastic_2FA_HeavyPart_get_cnt_ratio(const Elastic_2FA_HeavyPart* self) {
    return (self && self->bucket_num > 0) ?
           (double)self->cnt / (self->bucket_num * COUNTER_PER_BUCKET) : 0.0;
}

int Elastic_2FA_HeavyPart_get_cnt(const Elastic_2FA_HeavyPart* self) {
    return self ? self->cnt : 0;
}

void Elastic_2FA_HeavyPart_get_heavy_hitters(Elastic_2FA_HeavyPart* self, int threshold,
                                             uint32_t* out_keys, uint32_t* out_vals, int* out_num) {
    int count = 0, max = *out_num;
    for (int b = 0; b < self->bucket_num; b++) {
        Bucket* bucket = &self->buckets[b];
        for (int i = 0; i < COUNTER_PER_BUCKET; i++) {
            Counter* c = &bucket->slots[i];
            if (c->val >= (uint32_t)threshold) {
                if (count < max) {
                    out_keys[count] = c->key;
                    out_vals[count] = c->val;
                }
                count++;
            }
        }
    }
    *out_num = count;
}
