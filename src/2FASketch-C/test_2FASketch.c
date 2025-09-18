#include "2FASketch.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define NUM_KEYS 10000
#define BUCKET_NUM 1000
#define THRESHOLD 10

int main() {
    // Create a new 2FASketch
    Elastic_2FASketch* sketch = Elastic_2FASketch_create(BUCKET_NUM, THRESHOLD);
    if (!sketch) {
        printf("Failed to create 2FASketch\n");
        return 1;
    }

    // Generate some test data
    srand(time(NULL));
    uint32_t test_keys[NUM_KEYS];
    for (int i = 0; i < NUM_KEYS; i++) {
        test_keys[i] = rand() % 100; // Keys from 0-99
        Elastic_2FASketch_insert(sketch, (uint8_t*)&test_keys[i], 1);
    }

    // Query some keys
    printf("Querying some keys:\n");
    for (int i = 0; i < 10; i++) {
        uint32_t key = rand() % 100;
        uint32_t count = Elastic_2FASketch_query(sketch, (uint8_t*)&key);
        printf("Key %u: count = %u\n", key, count);
    }

    // Get heavy hitters
    uint32_t heavy_hitters[20];
    int num_hitters = 20;
    Elastic_2FASketch_get_heavy_hitters(sketch, THRESHOLD, heavy_hitters, &num_hitters);
    
    printf("\nTop %d heavy hitters (threshold=%d):\n", num_hitters, THRESHOLD);
    for (int i = 0; i < num_hitters; i++) {
        printf("Key %u: %u\n", heavy_hitters[i], 
               Elastic_2FASketch_query(sketch, (uint8_t*)&heavy_hitters[i]));
    }

    // Clean up
    Elastic_2FASketch_destroy(sketch);
    return 0;
}