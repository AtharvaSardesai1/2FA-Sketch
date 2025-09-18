#include "2FASketch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "param.h"

Elastic_2FASketch* Elastic_2FASketch_create(int bucket_num, int thres_set) {
    Elastic_2FASketch* self = (Elastic_2FASketch*)malloc(sizeof(Elastic_2FASketch));
    if (!self) return NULL;

    self->heavy_part = Elastic_2FA_HeavyPart_create(bucket_num);
    if (!self->heavy_part) {
        free(self);
        return NULL;
    }

    self->thres_set = thres_set;
    return self;
}

void Elastic_2FASketch_destroy(Elastic_2FASketch* self) {
    if (!self) return;
    
    if (self->heavy_part) {
        Elastic_2FA_HeavyPart_destroy(self->heavy_part);
    }
    
    free(self);
}

void Elastic_2FASketch_clear(Elastic_2FASketch* self) {
    if (self && self->heavy_part) {
        Elastic_2FA_HeavyPart_clear(self->heavy_part);
    }
}

void Elastic_2FASketch_insert(Elastic_2FASketch* self, uint8_t* key, int f) {
    if (!self || !key) return;

    // First try quick insert
    int res = Elastic_2FA_HeavyPart_quick_insert(self->heavy_part, key, f, self->thres_set);
    
    // If quick insert returned the threshold, do a full insert
    if (res == self->thres_set) {
        Elastic_2FA_HeavyPart_quick_insert(self->heavy_part, key, f, 0);
    }
}

uint32_t Elastic_2FASketch_query(Elastic_2FASketch* self, uint8_t* key) {
    if (!self || !key) return 0;
    return Elastic_2FA_HeavyPart_query(self->heavy_part, key, self->thres_set);
}

// Helper function to swap two elements
static void swap(uint32_t* a, uint32_t* b) {
    uint32_t temp = *a;
    *a = *b;
    *b = temp;
}

// Simple quicksort implementation for heavy hitters
static void quicksort(uint32_t* arr, int low, int high) {
    if (low < high) {
        uint32_t pivot = arr[high];
        int i = low - 1;

        for (int j = low; j <= high - 1; j++) {
            if (arr[j] >= pivot) {
                i++;
                swap(&arr[i], &arr[j]);
            }
        }
        swap(&arr[i + 1], &arr[high]);
        int pi = i + 1;

        quicksort(arr, low, pi - 1);
        quicksort(arr, pi + 1, high);
    }
}

void Elastic_2FASketch_get_heavy_hitters(Elastic_2FASketch* self, int threshold, 
                                       uint32_t* results, int* num_results) {
    if (!self || !results || !num_results) return;

    int count = 0;
    int max_results = *num_results;
    *num_results = 0;

    // Iterate through all buckets
    int num_buckets = Elastic_2FA_HeavyPart_get_bucket_num(self->heavy_part);
    for (int i = 0; i < num_buckets; i++) {
        Bucket* bucket = &self->heavy_part->buckets[i];
        
        // Check each counter in the bucket
        for (int j = 0; j < COUNTER_PER_BUCKET; j++) {
            if (bucket->val[j] >= threshold) {
                if (count < max_results) {
                    results[count++] = bucket->key[j];
                } else {
                    // If we've filled the results array, sort and keep only top-k
                    quicksort(results, 0, count - 1);
                    if (bucket->val[j] > results[count - 1]) {
                        results[count - 1] = bucket->key[j];
                    }
                }
            }
        }
    }

    *num_results = count;
    
    // Sort the results
    if (count > 1) {
        quicksort(results, 0, count - 1);
    }
}