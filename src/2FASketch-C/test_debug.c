#include "2FASketch.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

// Function prototypes
void print_menu();
void insert_key(Elastic_2FASketch* sketch);
void query_key(Elastic_2FASketch* sketch);
void print_stats(Elastic_2FASketch* sketch);
void run_auto_test(Elastic_2FASketch* sketch);

int main() {
    int bucket_num = 1000;
    int threshold = 10;
    int choice;
    
    printf("2FASketch Interactive Tester\n");
    printf("Enter number of buckets: ");
    scanf("%d", &bucket_num);
    printf("Enter threshold value: ");
    scanf("%d", &threshold);
    
    printf("\nCreating 2FASketch with %d buckets and threshold %d\n", bucket_num, threshold);
    Elastic_2FASketch* sketch = Elastic_2FASketch_create(bucket_num, threshold);
    
    if (!sketch) {
        printf("Failed to create 2FASketch. Exiting...\n");
        return 1;
    }
    
    printf("2FASketch created successfully at %p\n", (void*)sketch);
    
    if (!sketch->heavy_part) {
        printf("Error: heavy_part is NULL\n");
        Elastic_2FASketch_destroy(sketch);
        return 1;
    }
    printf("Heavy part created successfully at %p\n\n", (void*)sketch->heavy_part);

    do {
        print_menu();
        printf("\nEnter your choice: ");
        scanf("%d", &choice);
        printf("\n");
        
        switch(choice) {
            case 1:
                insert_key(sketch);
                break;
            case 2:
                query_key(sketch);
                break;
            case 3:
                print_stats(sketch);
                break;
            case 4:
                run_auto_test(sketch);
                break;
            case 5:
                printf("Clearing 2FASketch...\n");
                // Clear the sketch by destroying and recreating it
                Elastic_2FASketch_destroy(sketch);
                sketch = Elastic_2FASketch_create(bucket_num, threshold);
                if (!sketch) {
                    printf("Failed to recreate 2FASketch. Exiting...\n");
                    return 1;
                }
                printf("2FASketch cleared and recreated.\n");
                break;
            case 6:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
        
        printf("\n");
    } while (choice != 6);
    
    // Clean up
    if (sketch) {
        Elastic_2FASketch_destroy(sketch);
    }
    
    return 0;
}

void print_menu() {
    printf("\n=== 2FASketch Menu ===\n");
    printf("1. Insert a key\n");
    printf("2. Query a key\n");
    printf("3. Print statistics\n");
    printf("4. Run auto-test\n");
    printf("5. Clear 2FASketch\n");
    printf("6. Exit\n");
}

void insert_key(Elastic_2FASketch* sketch) {
    if (!sketch) {
        printf("Error: Sketch is NULL\n");
        return;
    }
    
    uint32_t key_val;
    printf("Enter key to insert (0-4294967295): ");
    if (scanf("%u", &key_val) != 1) {
        printf("Invalid input. Please enter a number.\n");
        // Clear input buffer
        while (getchar() != '\n');
        return;
    }
    
    uint8_t key[4];
    memcpy(key, &key_val, 4);
    
    printf("\nInserting key: 0x%08x (%u)\n", key_val, key_val);
    
    // Insert the key
    Elastic_2FASketch_insert(sketch, key, 1);
    printf("Key %u inserted.\n", key_val);
    
    // Print the bucket layout after insertion
    if (sketch && sketch->heavy_part) {
        Elastic_2FA_HeavyPart_print_buckets(sketch->heavy_part);
    } else {
        printf("Warning: Could not print bucket layout - heavy part not available.\n");
    }
}

void query_key(Elastic_2FASketch* sketch) {
    uint32_t key_val;
    printf("Enter key to query: ");
    scanf("%u", &key_val);
    
    uint8_t key[4];
    memcpy(key, &key_val, 4);
    
    uint32_t count = Elastic_2FASketch_query(sketch, key);
    printf("Key %u: count = %u\n", key_val, count);
}

void print_stats(Elastic_2FASketch* sketch) {
    if (!sketch) {
        printf("Error: Sketch is NULL\n");
        return;
    }
    
    printf("=== 2FASketch Statistics ===\n");
    printf("Heavy part address: %p\n", (void*)sketch->heavy_part);
    // Note: Accessing internal structure details might need additional getter functions
    // For now, we'll just print basic info
    printf("Structure size: %zu bytes\n", sizeof(Elastic_2FASketch));
}

void run_auto_test(Elastic_2FASketch* sketch) {
    if (!sketch) {
        printf("Error: Sketch is NULL\n");
        return;
    }
    
    int num_keys;
    printf("Enter number of random keys to insert: ");
    scanf("%d", &num_keys);
    
    if (num_keys <= 0) {
        printf("Invalid number of keys.\n");
        return;
    }
    
    srand(time(NULL));
    printf("Inserting %d random keys...\n", num_keys);
    
    for (int i = 0; i < num_keys; i++) {
        uint32_t key_val = rand() % 1000;  // Keys from 0-999
        uint8_t key[4];
        memcpy(key, &key_val, 4);
        
        Elastic_2FASketch_insert(sketch, key, 1);
        
        if ((i + 1) % 1000 == 0) {
            printf("Inserted %d keys...\n", i + 1);
        }
    }
    
    printf("\nAuto-test completed. %d keys inserted.\n", num_keys);
    
    // Print the bucket layout after auto-test
    if (sketch && sketch->heavy_part) {
        printf("\n=== Final Bucket Layout After Auto-Test ===\n");
        Elastic_2FA_HeavyPart_print_buckets(sketch->heavy_part);
    } else {
        printf("Warning: Could not print bucket layout - heavy part not available.\n");
    }
    
    printf("You can now query the keys or run more tests.\n");
}