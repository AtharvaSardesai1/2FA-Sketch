#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>
#include <random>
#include "HeavyPart.h"  // Assuming this includes all necessary headers

// Function to print bucket contents
void printBucket(const Bucket& bucket, int bucket_idx) {
    std::cout << "Bucket " << bucket_idx << ":\n";
    std::cout << "  Keys:   ";
    for (int i = 0; i < MAX_VALID_COUNTER; ++i) {
        std::cout << bucket.key[i] << "\t";
    }
    std::cout << "| Guard\n  Values: ";
    for (int i = 0; i < MAX_VALID_COUNTER; ++i) {
        std::cout << (bucket.val[i] & 0x7FFFFFFF) << "\t";
    }
    std::cout << "| " << bucket.val[MAX_VALID_COUNTER] << "\n\n";
}

// Function to create a key from integer
void makeKey(uint8_t* key, uint32_t value) {
    *reinterpret_cast<uint32_t*>(key) = value;
}

int main() {
    // Create a small sketch with 4 buckets for testing
    const int NUM_BUCKETS = 4;
    Elastic_2FA_HeavyPart<NUM_BUCKETS> sketch;
    
    // Test 1: Insert into empty bucket
    {
        std::cout << "=== Test 1: Insert into empty bucket ===\n";
        uint8_t key1[4];
        makeKey(key1, 0x12345678);
        
        int result = sketch.quick_insert(key1, 1, 5);  // thres_set = 5
        std::cout << "Insert result: " << result 
                  << " (0=success, 1=swap, 2=rehash, thres=threshold hit)\n";
        
        // Print all buckets
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            printBucket(sketch.buckets[i], i);
        }
    }
    
    // Test 2: Insert same key again (should increment counter)
    {
        std::cout << "\n=== Test 2: Insert existing key ===\n";
        uint8_t key1[4];
        makeKey(key1, 0x12345678);
        
        int result = sketch.quick_insert(key1, 1, 5);
        std::cout << "Insert result: " << result << "\n";
        printBucket(sketch.buckets[0], 0);  // Should be in first bucket
    }
    
    // Test 3: Fill up a bucket
    {
        std::cout << "\n=== Test 3: Fill up a bucket ===\n";
        for (int i = 1; i <= 7; ++i) {
            uint8_t key[4];
            makeKey(key, 0x1000 + i);  // Different keys
            int result = sketch.quick_insert(key, 1, 5);
            std::cout << "Insert key 0x" << std::hex << (0x1000 + i) 
                      << " result: " << result << "\n";
        }
        printBucket(sketch.buckets[0], 0);
    }
    
    // Test 4: Trigger threshold
    {
        std::cout << "\n=== Test 4: Trigger threshold ===\n";
        uint8_t key[4];
        makeKey(key, 0x2000);
        
        // First insert (should hit threshold)
        int result = sketch.quick_insert(key, 10, 5);  // thres_set = 5
        std::cout << "First insert result: " << result 
                  << " (should be 5, meaning threshold hit)\n";
                  
        // Second insert (should do actual insert)
        result = sketch.quick_insert(key, 10, 0);  // thres_set = 0 for second insert
        std::cout << "Second insert result: " << result 
                  << " (should be 0, meaning success)\n";
                  
        printBucket(sketch.buckets[0], 0);
    }
    
    // Test 5: Test eviction
    {
        std::cout << "\n=== Test 5: Test eviction ===\n";
        // Insert multiple keys to trigger evictions
        for (int i = 0; i < 20; ++i) {
            uint8_t key[4];
            makeKey(key, 0x3000 + i);
            sketch.quick_insert(key, 1, 3);  // Lower threshold to trigger more evictions
        }
        
        // Print all buckets to see eviction effects
        for (int i = 0; i < NUM_BUCKETS; ++i) {
            printBucket(sketch.buckets[i], i);
        }
    }
    
    std::cout << "Test completed. Check the output to understand the behavior.\n";
    return 0;
}