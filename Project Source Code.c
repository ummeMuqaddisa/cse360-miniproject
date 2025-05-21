#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

// Cache configuration
#define L1_SIZE 16
#define L2_SIZE 64
#define WORD_SIZE 4      // 4 bytes per word
#define WORDS_PER_LINE 4 // 4 words per cache line (16 bytes per line)
#define BLOCK_SIZE (WORDS_PER_LINE * WORD_SIZE)  // 16 bytes per cache line
#define ADDRESS_SPACE 0x1000  // 4096 bytes address space
#define MAIN_MEMORY_SIZE ADDRESS_SPACE  // Main memory size equals address space
#define L1_ACCESS_COST 1
#define L2_ACCESS_COST 10
#define MEMORY_ACCESS_COST 100
#define MAIN_MEMORY_BLOCKS (MAIN_MEMORY_SIZE / BLOCK_SIZE)


// Additional definitions for fully associative cache
#define L1_FULLY_ASSOCIATIVE 1  // Flag to indicate fully associative cache
#define L2_FULLY_ASSOCIATIVE 1  // Flag to indicate fully associative cache

// Additional definitions for set associative cache
#define L1_ASSOCIATIVITY 2  // 2-way set associative for L1
#define L2_ASSOCIATIVITY 4  // 4-way set associative for L2
#define L1_SETS (L1_SIZE / L1_ASSOCIATIVITY)  // Number of sets in L1
#define L2_SETS (L2_SIZE / L2_ASSOCIATIVITY)  // Number of sets in L2



// Cache line structure
typedef struct {
    int tag;
    bool valid;
    unsigned int address;
} CacheLine;


// fully associative cache line structure
typedef struct {
    int tag;
    bool valid;
    unsigned int address;
    int lru_counter;
} FullyAssociativeCacheLine;


// set associative cache line structure
typedef struct {
    int tag;
    bool valid;
    unsigned int address;
    int lru_counter;
} AssociativeCacheLine;


// cache stat structure
typedef struct {
    int l1_hits;
    int l2_hits;
    int memory_accesses;
    long total_cost;
    float hit_rate;
    float avg_access_time;
} CacheStats;


// Structure to store hit address information
typedef struct {
    unsigned int address;
    int cache_level;  // 1 for L1 hit, 2 for L2 hit
} HitInfo;



// Animation for cache cheching
void animateCheckL1() {
    printf("\nChecking in L1");
    fflush(stdout);
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        usleep(50000);
    }
    printf("\n\n");
}

void animateCheckL2() {
    printf("\nChecking in L2");
    fflush(stdout);
    for (int i = 0; i < 3; i++) {
        printf(".");
        fflush(stdout);
        usleep(500000);
    }
   printf("\n\n");
}

void animateCheckMM() {
    printf("\nAccessing Main Memory");
    fflush(stdout);
    for (int i = 0; i < 5; i++) {
        printf(".");
        fflush(stdout);
        usleep(1000000);
    }
    printf("\n\n");
}

// Introduce a small delay for visual effect
void simulateDelay() {
    usleep(500000); // 500ms delay
}


//-- functions for direct mapping--


// Display summary of all hit addresses
void displayHitSummary(HitInfo *hitInfoArray, int hitCount) {
    int displayCount =  hitCount;

    printf("Summary of Cache Hits (showing first %d out of %d hits)\n", displayCount, hitCount);
    printf("---------------------------------------------------\n");
    printf("Address  | Cache | TAG  | SET  | WORD | BYTE\n");
    printf("-------- | ----- | ---- | ---- | ---- | ----\n");

    for (int i = 0; i < displayCount; i++) {
        unsigned int address = hitInfoArray[i].address;
        int cache_level = hitInfoArray[i].cache_level;


        unsigned int tag, set;

        if (cache_level == 1) {
            // L1 cache hit
            tag = address / (L1_SIZE * WORDS_PER_LINE * WORD_SIZE);
            set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE;
        } else {
            // L2 cache hit
            tag = address / (L2_SIZE * WORDS_PER_LINE * WORD_SIZE);
            set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SIZE;
        }

        unsigned int word = (address / WORD_SIZE) % WORDS_PER_LINE;
        unsigned int byte = address % WORD_SIZE;

        printf("0x%04X   | L%d    | 0x%02X | 0x%01X  | 0x%01X  | 0x%01X\n",
               address, cache_level, tag, set, word, byte);
    }

    if (hitCount > displayCount) {
        printf("... and %d more hits (not shown)\n", hitCount - displayCount);
    }
    printf("\n");
}

// Initialize cache with invalid lines
void initializeCache(CacheLine *cache, int size) {
    for (int i = 0; i < size; i++) {
        cache[i].valid = false;
        cache[i].tag = -1;
        cache[i].address = 0;
    }
}

// Generate random memory addresses
void generateAddresses(unsigned int *addresses, int numAccesses) {
    for (int i = 0; i < numAccesses; i++) {
        addresses[i] = (rand() % (ADDRESS_SPACE / WORD_SIZE)) * WORD_SIZE;
    }
}

// Check if an address is in the cache
bool checkCache(CacheLine *cache, int cacheSize, unsigned int address, int *tag, int *index) {
    *index = (address / (WORDS_PER_LINE * WORD_SIZE)) % cacheSize;
    *tag = address / (cacheSize * WORDS_PER_LINE * WORD_SIZE);

    return (cache[*index].valid && cache[*index].tag == *tag);
}

// Update cache with new address
void updateCache(CacheLine *cache, int index, int tag, unsigned int address) {
    cache[index].valid = true;
    cache[index].tag = tag;
    cache[index].address = address;
}


// Display  cache content
void displayCacheContents(CacheLine *cache, int size, const char *cacheName) {
    int displayLimit = size;

printf("%s Contents (showing first %d lines):\n", cacheName, displayLimit);
    printf("SET(idx) | Valid | TAG  | Address  | Word Offset\n");
    printf("-------- | ----- | ---- | -------- | -----------\n");
    for (int i = 0; i < displayLimit; i++) {
        if (cache[i].valid) {
            printf("   0x%01X   |   %d   | 0x%02X | 0x%04X   | 0x%01X\n",
                i, cache[i].valid, cache[i].tag, cache[i].address,
                (cache[i].address / WORD_SIZE) % WORDS_PER_LINE);
        } else {
            printf("   0x%01X   |   %d   | ---  | -------- | ---\n", i, cache[i].valid);
        }
    }

    if (size > displayLimit) {
        printf("... and %d more sets (not shown)\n", size - displayLimit);
    }
    printf("\n");
}


// Print detailed address breakdown with TAG, SET, WORD
void printAddressBreakdown(unsigned int address) {
    unsigned int l1_tag = address/(L1_SIZE * WORDS_PER_LINE * WORD_SIZE);
    unsigned int l1_set = (address/(WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE;
    unsigned int l2_tag = address/(L2_SIZE * WORDS_PER_LINE * WORD_SIZE);
    unsigned int l2_set = (address/(WORDS_PER_LINE * WORD_SIZE)) % L2_SIZE;
    unsigned int word_offset = (address/WORD_SIZE) % WORDS_PER_LINE;
    unsigned int byte_offset = address % WORD_SIZE;

    printf("Address Breakdown (0x%04X):\n", address);
    printf("----------------------------------------\n");
    printf("Memory Architecture:\n");
    printf("- Word Size: %d bytes\n", WORD_SIZE);
    printf("- Words per Cache Line: %d words\n", WORDS_PER_LINE);
    printf("- Cache Line Size: %d bytes\n\n", WORD_SIZE * WORDS_PER_LINE);

    printf("Binary breakdown: ");
    for (int i = 15; i >= 0; i--) {
        int bit = (address >> i) & 1;
        if (i == 7) printf(" | ");
        if (i == 4) printf(" | ");
        if (i == 2) printf(" | ");
        printf("%d", bit);
    }
    printf("\n                  TAG | SET | WORD | BYTE\n\n");

    printf("L1 Cache Mapping:\n");
    printf("  TAG: 0x%02X  SET: 0x%01X  WORD: 0x%01X  BYTE: 0x%01X\n\n",
           l1_tag, l1_set, word_offset, byte_offset);

    printf("L2 Cache Mapping:\n");
    printf("  TAG: 0x%01X  SET: 0x%01X  WORD: 0x%01X  BYTE: 0x%01X\n\n",
           l2_tag, l2_set, word_offset, byte_offset);
}


// Clear the console screen
void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}


int cacheSimulation(int t){
   // Seed random number generator
    srand(time(NULL));

    int numAccesses;
    int i;

    // Statistics variables
    int l1Hits = 0, l1Misses = 0;
    int l2Hits = 0, l2Misses = 0;
    long totalCycles = 0;  // Use long for potential large values

    // Create cache structures
    CacheLine l1Cache[L1_SIZE];
    CacheLine l2Cache[L2_SIZE];

    // Initialize caches
    initializeCache(l1Cache, L1_SIZE);
    initializeCache(l2Cache, L2_SIZE);

 // User input
    printf("Memory Hierarchy Simulator (Direct Mapping) with TAG/SET/WORD Breakdown\n");
    printf("-------------------------------------------------------------------\n");
    printf("Enter the number of memory access attempts to simulate: ");
    scanf("%d", &numAccesses);
    // Allocate memory for addresses
    unsigned int *addresses = (unsigned int *)malloc(numAccesses * sizeof(unsigned int));
    if (addresses == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        return 1;
    }

    // Create array to store hit information
    HitInfo *hitInfoArray = (HitInfo *)malloc(numAccesses * sizeof(HitInfo));
    if (hitInfoArray == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        free(addresses);
        return 1;
    }
    int hitCount = 0;

    // Generate random addresses
    generateAddresses(addresses, numAccesses);
    // Simulate memory accesses
    for (i = 0; i < numAccesses; i++) {
        unsigned int address = addresses[i];
        int tag, index;
        bool l1Hit, l2Hit;


        if(t==1){
            clearScreen();

        printf("Memory Access #%d\n", i + 1);
        printf("------------------\n");
        printf("Accessing address: 0x%04X\n\n", address);

        // Print address breakdown with TAG/SET/WORD
        printAddressBreakdown(address);

        }

        // Check L1 Cache
        l1Hit = checkCache(l1Cache, L1_SIZE, address, &tag, &index);
      if(t==1)  animateCheckL1();
        if (l1Hit) {
            // L1 Cache hit
            l1Hits++;
            totalCycles += L1_ACCESS_COST;

            // Record hit information
            hitInfoArray[hitCount].address = address;
            hitInfoArray[hitCount].cache_level = 1;  // L1 hit
            hitCount++;

            if(t==1){
                printf("L1 CACHE HIT!\n");
            printf("  TAG: 0x%02X  SET: 0x%01X  WORD: 0x%01X\n",
                   address / (L1_SIZE * WORDS_PER_LINE * WORD_SIZE),
                   (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE,
                   (address / WORD_SIZE) % WORDS_PER_LINE);
            printf("Access cost: %d cycles\n\n", L1_ACCESS_COST);

            displayCacheContents(l1Cache, L1_SIZE, "L1 Cache");
            if (hitCount > 0) {
                displayHitSummary(hitInfoArray, hitCount);
            } else {
                printf("No cache hits recorded during this simulation.\n");
            }

            printf("\nPress Enter to continue to next memory access...");
            while (getchar() != '\n'); // Clear input buffer
            getchar();
            } // Wait for user input
        } else {
            // L1 Cache miss
            l1Misses++;


            if(t==1){
                printf("L1 CACHE MISS!\n");

           printf("  Attempted to find TAG: 0x%02X in SET: 0x%01X\n",
                   address / (L1_SIZE * WORDS_PER_LINE * WORD_SIZE),
                   (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE);

            // Check L2 Cache
          usleep(500000);
            }


            l2Hit = checkCache(l2Cache, L2_SIZE, address, &tag, &index);
         if(t==1)   animateCheckL2();
            if (l2Hit) {
                // L2 Cache hit
                l2Hits++;
                totalCycles += L2_ACCESS_COST;

                // Record hit information
                hitInfoArray[hitCount].address = address;
                hitInfoArray[hitCount].cache_level = 2;  // L2 hit
                hitCount++;


                if(t==1){

                printf("\nL2 CACHE HIT!\n");
                printf("  TAG: 0x%02X  SET: 0x%01X  WORD: 0x%01X\n",
                       address / (L2_SIZE * WORDS_PER_LINE * WORD_SIZE),
                       (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SIZE,
                       (address / WORD_SIZE) % WORDS_PER_LINE);
                printf("Access cost: %d cycles\n", L2_ACCESS_COST);

                // Update L1 Cache
                updateCache(l1Cache, (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE,
                           address / (L1_SIZE * WORDS_PER_LINE * WORD_SIZE), address);
                printf("Data loaded from L2 to L1\n\n");

                displayCacheContents(l2Cache, L2_SIZE, "L2 Cache");
                if (hitCount > 0) {
                    displayHitSummary(hitInfoArray, hitCount);
                } else {
                    printf("No cache hits recorded during this simulation.\n");
                }

                printf("\nPress Enter to continue to next memory access...");
               while (getchar() != '\n'); // Clear input buffer
                getchar(); // Wait for user input
                }

            } else {
                // L2 Cache miss
                l2Misses++;
                totalCycles += MEMORY_ACCESS_COST;


                if(t==1){
                      printf("L2 CACHE MISS!\n");

                printf("  Attempted to find TAG: 0x%02X in SET: 0x%01X\n",
                       address / (L2_SIZE * WORDS_PER_LINE * WORD_SIZE),
                       (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SIZE);


                animateCheckMM();


                printf("Access cost: %d cycles\n", MEMORY_ACCESS_COST);

                }
                // Update L1 and L2 Caches (inclusive cache policy)
                updateCache(l1Cache, (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE,
                           address / (L1_SIZE * WORDS_PER_LINE * WORD_SIZE), address);
                updateCache(l2Cache, (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SIZE,
                           address / (L2_SIZE * WORDS_PER_LINE * WORD_SIZE), address);
                           if(t==1){
                printf("Data loaded from Main Memory to L1 and L2.\n\n");

                // Display cache contents after update
                displayCacheContents(l1Cache, L1_SIZE, "L1 Cache");
                displayCacheContents(l2Cache, L2_SIZE, "L2 Cache");

                printf("\nPress Enter to continue to next memory access...");
                while (getchar() != '\n'); // Clear input buffer
                getchar(); // Wait for user input

                }

            }
        }
    }

    // Calculate final statistics
    float l1HitRatio = (float)l1Hits / numAccesses;
    float l2HitRatio = (l1Misses > 0) ? (float)l2Hits / l1Misses : 0;
    float amat = L1_ACCESS_COST + (1 - l1HitRatio) * (L2_ACCESS_COST + (1 - l2HitRatio) * MEMORY_ACCESS_COST);

    // Display final statistics
    clearScreen();
    printf("Memory Hierarchy Simulation Complete\n");
    printf("------------------------------------\n\n");
    printf("Cache Architecture:\n");
    printf("------------------\n");
    printf("  Cache Policy: Inclusive (L2 contains all entries in L1)\n");
    printf("  L1 Cache: %d sets, %d-byte lines (%d words per line)\n", L1_SIZE, WORDS_PER_LINE * WORD_SIZE, WORDS_PER_LINE);
    printf("  L2 Cache: %d sets, %d-byte lines (%d words per line)\n\n", L2_SIZE, WORDS_PER_LINE * WORD_SIZE, WORDS_PER_LINE);

    printf("Simulation Results:\n");
    printf("------------------\n");
    printf("Total memory accesses: %d\n\n", numAccesses);

    printf("L1 Cache Statistics:\n");
    printf("  Hits: %d (%.2f%%)\n", l1Hits, l1HitRatio * 100);
    printf("  Misses: %d (%.2f%%)\n\n", l1Misses, (1 - l1HitRatio) * 100);

    printf("L2 Cache Statistics:\n");
    printf("  Hits: %d (%.2f%%)\n", l2Hits, l2HitRatio * 100);
    printf("  Misses: %d (%.2f%%)\n\n", l2Misses, (1 - l2HitRatio) * 100);

    printf("Performance Metrics:\n");
    printf("  Total Cycle Cost: %ld cycles\n", totalCycles);
    printf("  Average Memory Access Time (AMAT): %.2f cycles\n\n", amat);

    // Display hit address summary
    if (hitCount > 0) {
        displayHitSummary(hitInfoArray, hitCount);
    } else {
        printf("No cache hits recorded during this simulation.\n");
    }

    // Free allocated memory
    free(addresses);
    free(hitInfoArray);
     printf("\n===============================================\n");
    printf("Direct Mapping analysis complete.\n");
    printf("Press Enter to return to main menu...");
    getchar();
    getchar();
}


//-- functions for fully associative mapping--


// Function to initialize fully associative cache
void initializeFullyAssociativeCache(FullyAssociativeCacheLine *cache, int size) {
    for (int i = 0; i < size; i++) {
        cache[i].valid = false;
        cache[i].tag = -1;
        cache[i].address = 0;
        cache[i].lru_counter = 0;
    }
}

// Check if address is in fully associative cache
bool checkFullyAssociativeCache(FullyAssociativeCacheLine *cache, int size, unsigned int address, int *tag, int *way) {
    *tag = address / (WORDS_PER_LINE * WORD_SIZE);
    for (int i = 0; i < size; i++) {
        if (cache[i].valid && cache[i].tag == *tag) {
            *way = i;
            return true;
        }
    }

    return false;
}

// Update LRU counters for fully associative cache
void updateFullyAssociativeLRU(FullyAssociativeCacheLine *cache, int size, int accessedWay) {

    for (int i = 0; i < size; i++) {
        if (cache[i].valid) {
            cache[i].lru_counter++;
        }
    }

    // Reset counter
    cache[accessedWay].lru_counter = 0;
}

// Find the least recently used way in fully associative cache
int findFullyAssociativeLRU(FullyAssociativeCacheLine *cache, int size) {
    int lruWay = 0;
    int maxCounter = -1;

    for (int i = 0; i < size; i++) {

        if (!cache[i].valid) {
            return i;
        }

        if (cache[i].lru_counter > maxCounter) {
            maxCounter = cache[i].lru_counter;
            lruWay = i;
        }
    }

    return lruWay;
}

// Update fully associative cache with new address
void updateFullyAssociativeCache(FullyAssociativeCacheLine *cache, int size, int way, int tag, unsigned int address) {
    cache[way].valid = true;
    cache[way].tag = tag;
    cache[way].address = address;
    updateFullyAssociativeLRU(cache, size, way);
}

// Display the contents of the fully associative cache
void displayFullyAssociativeCacheContents(FullyAssociativeCacheLine *cache, int size, const char *cacheName) {
    int displayLimit = size;

    printf("%s Contents (showing first %d entries):\n", cacheName, displayLimit);
    printf("ENTRY | Valid | TAG  | Address  | LRU | Word Offset\n");
    printf("----- | ----- | ---- | -------- | --- | -----------\n");

    for (int i = 0; i < displayLimit; i++) {
        if (cache[i].valid) {
            printf("0x%02X |   %d   | 0x%02X | 0x%04X   | %3d | 0x%01X\n",
                i, cache[i].valid, cache[i].tag, cache[i].address,
                cache[i].lru_counter, (cache[i].address / WORD_SIZE) % WORDS_PER_LINE);
        } else {
            printf("0x%02X |   %d   | ---  | -------- | --- | ---\n", i, cache[i].valid);
        }
    }

    if (size > displayLimit) {
        printf("... and %d more entries (not shown)\n", size - displayLimit);
    }
    printf("\n");
}

// Print address breakdown for fully associative cache
void printFullyAssociativeAddressBreakdown(unsigned int address) {
    unsigned int tag = address / (WORDS_PER_LINE * WORD_SIZE);
    unsigned int word_offset = (address / WORD_SIZE) % WORDS_PER_LINE;
    unsigned int byte_offset = address % WORD_SIZE;

    printf("Address Breakdown (0x%04X):\n", address);
    printf("----------------------------------------\n");
    printf("Memory Architecture (Fully Associative):\n");
    printf("- Word Size: %d bytes\n", WORD_SIZE);
    printf("- Words per Cache Line: %d words\n", WORDS_PER_LINE);
    printf("- Cache Line Size: %d bytes\n", WORD_SIZE * WORDS_PER_LINE);
    printf("- L1: Fully associative (%d entries)\n", L1_SIZE);
    printf("- L2: Fully associative (%d entries)\n\n", L2_SIZE);

    printf("Binary breakdown: ");

    for (int i = 15; i >= 0; i--) {
        int bit = (address >> i) & 1;

        if (i == 4) printf(" | ");
        if (i == 2) printf(" | ");

        printf("%d", bit);
    }
    printf("\n                        TAG       │  WORD  │  BYTE \n\n");

    printf("L1 & L2 Cache Mapping (Fully Associative):\n");
    printf("  TAG: 0x%03X  WORD: 0x%01X  BYTE: 0x%01X\n\n",
           tag, word_offset, byte_offset);
}

// Main function for fully associative cache simulation
int cacheSimulationFullyAssociative(int t) {
    srand(time(NULL));

    int numAccesses;
    int i;
    int l1Hits = 0, l1Misses = 0;
    int l2Hits = 0, l2Misses = 0;
    long totalCycles = 0;

    // Create cache structures for fully associative caches
    FullyAssociativeCacheLine l1Cache[L1_SIZE];
    FullyAssociativeCacheLine l2Cache[L2_SIZE];

    // Initialize caches
    initializeFullyAssociativeCache(l1Cache, L1_SIZE);
    initializeFullyAssociativeCache(l2Cache, L2_SIZE);

    // User input
    printf("Memory Hierarchy Simulator (Fully Associative) with TAG/OFFSET Breakdown\n");
    printf("-------------------------------------------------------------------\n");
    printf("L1 Cache: Fully associative with %d entries\n", L1_SIZE);
    printf("L2 Cache: Fully associative with %d entries\n", L2_SIZE);
    printf("Enter the number of memory access attempts to simulate: ");
    scanf("%d", &numAccesses);

    // Allocate memory for addresses
    unsigned int *addresses = (unsigned int *)malloc(numAccesses * sizeof(unsigned int));
    if (addresses == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        return 1;
    }

    // Create array to store hit information
    HitInfo *hitInfoArray = (HitInfo *)malloc(numAccesses * sizeof(HitInfo));
    if (hitInfoArray == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        free(addresses);
        return 1;
    }
    int hitCount = 0;

    // Generate random addresses
    generateAddresses(addresses, numAccesses);

    // Simulate memory accesses
    for (i = 0; i < numAccesses; i++) {
        unsigned int address = addresses[i];
        int tag, way;
        bool l1Hit, l2Hit;

        if(t==1){
             clearScreen();

        printf("Memory Access #%d (Fully Associative)\n", i + 1);
        printf("----------------------------------\n");
        printf("Accessing address: 0x%04X\n\n", address);

        // Print address breakdown
        printFullyAssociativeAddressBreakdown(address);

        }

        // Check L1 Cache - in fully associative, we search all entries
        l1Hit = checkFullyAssociativeCache(l1Cache, L1_SIZE, address, &tag, &way);
if(t==1)  animateCheckL1();
        if (l1Hit) {
            // L1 Cache hit
            l1Hits++;
            totalCycles += L1_ACCESS_COST;

            // Record hit information
            hitInfoArray[hitCount].address = address;
            hitInfoArray[hitCount].cache_level = 1;  // L1 hit
            hitCount++;


            if(t==1){
                 printf("L1 CACHE HIT!\n");
            printf("  TAG: 0x%03X  ENTRY: 0x%02X  WORD: 0x%01X\n",
                   tag, way, (address / WORD_SIZE) % WORDS_PER_LINE);
            printf("Access cost: %d cycles\n\n", L1_ACCESS_COST);

            }

            // Update LRU counters
            updateFullyAssociativeLRU(l1Cache, L1_SIZE, way);

            if(t==1){

                displayFullyAssociativeCacheContents(l1Cache, L1_SIZE, "L1 Cache");
            if (hitCount > 0) {
                displayHitSummary(hitInfoArray, hitCount);
            } else {
                printf("No cache hits recorded during this simulation.\n");
            }

            printf("\nPress Enter to continue to next memory access...");
            while (getchar() != '\n'); // Clear input buffer
            getchar(); // Wait for user input

            }

        } else {
            // L1 Cache miss
            l1Misses++;
            if(t==1){
                 printf("L1 CACHE MISS!\n");
            printf("  Attempted to find TAG: 0x%03X in fully associative L1\n", tag);
           usleep(500000);

            }


            // Check L2 Cache - search all entries
            l2Hit = checkFullyAssociativeCache(l2Cache, L2_SIZE, address, &tag, &way);
if(t==1)  animateCheckL2();
            if (l2Hit) {
                // L2 Cache hit
                l2Hits++;
                totalCycles += L2_ACCESS_COST;

                // Record hit information
                hitInfoArray[hitCount].address = address;
                hitInfoArray[hitCount].cache_level = 2;  // L2 hit
                hitCount++;

                if(t==1){

                printf("\nL2 CACHE HIT!\n");
                printf("  TAG: 0x%03X  ENTRY: 0x%02X  WORD: 0x%01X\n",
                       tag, way, (address / WORD_SIZE) % WORDS_PER_LINE);
                printf("Access cost: %d cycles\n", L2_ACCESS_COST);

                }
                // Update LRU counters for L2
                updateFullyAssociativeLRU(l2Cache, L2_SIZE, way);

                // Find location in L1 to place this data
                int l1Way = findFullyAssociativeLRU(l1Cache, L1_SIZE);

                // Update L1 Cache (tag is the same for fully associative)
                updateFullyAssociativeCache(l1Cache, L1_SIZE, l1Way, tag, address);

               if(t==1){
                 printf("Data loaded from L2 to L1 (placed in entry 0x%02X)\n\n", l1Way);

                displayFullyAssociativeCacheContents(l2Cache, L2_SIZE, "L2 Cache");
                if (hitCount > 0) {
                    displayHitSummary(hitInfoArray, hitCount);
                } else {
                    printf("No cache hits recorded during this simulation.\n");
                }

                printf("\nPress Enter to continue to next memory access...");
                while (getchar() != '\n'); // Clear input buffer
                getchar(); // Wait for user input
               }
            } else {
                // L2 Cache miss
                l2Misses++;
                totalCycles += MEMORY_ACCESS_COST;
                if(t==1){

                printf("\nL2 CACHE MISS!\n");
                printf("  Attempted to find TAG: 0x%03X in fully associative L2\n", tag);

                if(t==1)  animateCheckMM();
                printf("Access cost: %d cycles\n", MEMORY_ACCESS_COST);
                }

                // Find locations in L1 and L2 to place this data
                int l1Way = findFullyAssociativeLRU(l1Cache, L1_SIZE);
                int l2Way = findFullyAssociativeLRU(l2Cache, L2_SIZE);

                // Update L1 and L2 Caches (inclusive cache policy)
                updateFullyAssociativeCache(l1Cache, L1_SIZE, l1Way, tag, address);
                updateFullyAssociativeCache(l2Cache, L2_SIZE, l2Way, tag, address);

              if(t==1){
                  printf("Data loaded from Main Memory to L1 (entry 0x%02X) and L2 (entry 0x%02X)\n\n",
                       l1Way, l2Way);

                // Display cache contents after update
                displayFullyAssociativeCacheContents(l1Cache, L1_SIZE, "L1 Cache");
                displayFullyAssociativeCacheContents(l2Cache, L2_SIZE, "L2 Cache");

                printf("\nPress Enter to continue to next memory access...");
                while (getchar() != '\n'); // Clear input buffer
                getchar(); // Wait for user input
              }
            }
        }
    }

    // Calculate final statistics
    float l1HitRatio = (float)l1Hits / numAccesses;
    float l2HitRatio = (l1Misses > 0) ? (float)l2Hits / l1Misses : 0;
    float amat = L1_ACCESS_COST + (1 - l1HitRatio) * (L2_ACCESS_COST + (1 - l2HitRatio) * MEMORY_ACCESS_COST);

    // Display final statistics
    clearScreen();
    printf("Memory Hierarchy Simulation Complete (Fully Associative)\n");
    printf("----------------------------------------------------\n\n");
    printf("Cache Architecture:\n");
    printf("------------------\n");
    printf("  Cache Policy: Inclusive (L2 contains all entries in L1)\n");
    printf("  L1 Cache: Fully associative with %d entries, %d-byte lines (%d words per line)\n",
           L1_SIZE, WORDS_PER_LINE * WORD_SIZE, WORDS_PER_LINE);
    printf("  L2 Cache: Fully associative with %d entries, %d-byte lines (%d words per line)\n\n",
           L2_SIZE, WORDS_PER_LINE * WORD_SIZE, WORDS_PER_LINE);

    printf("Simulation Results:\n");
    printf("------------------\n");
    printf("Total memory accesses: %d\n\n", numAccesses);

    printf("L1 Cache Statistics:\n");
    printf("  Hits: %d (%.2f%%)\n", l1Hits, l1HitRatio * 100);
    printf("  Misses: %d (%.2f%%)\n\n", l1Misses, (1 - l1HitRatio) * 100);

    printf("L2 Cache Statistics:\n");
    printf("  Hits: %d (%.2f%%)\n", l2Hits, l2HitRatio * 100);
    printf("  Misses: %d (%.2f%%)\n\n", l2Misses, (1 - l2HitRatio) * 100);

    printf("Performance Metrics:\n");
    printf("  Total Cycle Cost: %ld cycles\n", totalCycles);
    printf("  Average Memory Access Time (AMAT): %.2f cycles\n\n", amat);

    // Display hit address summary
    if (hitCount > 0) {
        displayHitSummary(hitInfoArray, hitCount);
    } else {
        printf("No cache hits recorded during this simulation.\n");
    }

    // Free allocated memory
    free(addresses);
    free(hitInfoArray);
    printf("\n===============================================\n");
    printf("Fully Associative Mapping analysis complete.\n");
    printf("Press Enter to return to main menu...");
    getchar();
    getchar();
    return 0;
}




//-- functions for set associative mapping--


// Function to initialize associative cache
void initializeAssociativeCache(AssociativeCacheLine *cache, int sets, int ways) {
    for (int i = 0; i < sets; i++) {
        for (int j = 0; j < ways; j++) {
            int index = i * ways + j;
            cache[index].valid = false;
            cache[index].tag = -1;
            cache[index].address = 0;
            cache[index].lru_counter = 0;
        }
    }
}

// Check if address is in associative cache
bool checkAssociativeCache(AssociativeCacheLine *cache, int sets, int ways, unsigned int address, int *tag, int *set, int *way) {
    // Calculate set and tag
    *set = (address / (WORDS_PER_LINE * WORD_SIZE)) % sets;
    *tag = address / (sets * WORDS_PER_LINE * WORD_SIZE);

    // Check all ways in the set
    for (int w = 0; w < ways; w++) {
        int index = (*set) * ways + w;
        if (cache[index].valid && cache[index].tag == *tag) {
            *way = w;
            return true;
        }
    }

    return false;
}

// Update LRU counters for all ways in a set
void updateLRUCounters(AssociativeCacheLine *cache, int set, int ways, int accessedWay) {
    // Increment all counters in the set
    for (int w = 0; w < ways; w++) {
        int index = set * ways + w;
        if (cache[index].valid) {
            cache[index].lru_counter++;
        }
    }

    // Reset counter for the accessed way
    int accessedIndex = set * ways + accessedWay;
    cache[accessedIndex].lru_counter = 0;
}

// Find the least recently used way in a set
int findLRUWay(AssociativeCacheLine *cache, int set, int ways) {
    int lruWay = 0;
    int maxCounter = -1;

    for (int w = 0; w < ways; w++) {
        int index = set * ways + w;

        // If way is invalid, use it immediately
        if (!cache[index].valid) {
            return w;
        }

        // Otherwise find the way with highest LRU counter
        if (cache[index].lru_counter > maxCounter) {
            maxCounter = cache[index].lru_counter;
            lruWay = w;
        }
    }

    return lruWay;
}

// Update associative cache with new address
void updateAssociativeCache(AssociativeCacheLine *cache, int set, int way, int ways, int tag, unsigned int address) {
    int index = set * ways + way;
    cache[index].valid = true;
    cache[index].tag = tag;
    cache[index].address = address;

    // Reset LRU counter for this way and update others
    updateLRUCounters(cache, set, ways, way);
}

// Display the contents of the set associative cache
void displayAssociativeCacheContents(AssociativeCacheLine *cache, int sets, int ways, const char *cacheName) {
    int displayLimit =  sets ;

    printf("%s Contents (showing first %d sets):\n", cacheName, displayLimit);
    printf("SET  | WAY | Valid | TAG  | Address  | LRU | Word Offset\n");
    printf("----- | --- | ----- | ---- | -------- | --- | -----------\n");

    for (int i = 0; i < displayLimit; i++) {
        for (int j = 0; j < ways; j++) {
            int index = i * ways + j;
            if (cache[index].valid) {
                printf("0x%02X | %2d  |   %d   | 0x%02X | 0x%04X   | %3d | 0x%01X\n",
                    i, j, cache[index].valid, cache[index].tag, cache[index].address,
                    cache[index].lru_counter, (cache[index].address / WORD_SIZE) % WORDS_PER_LINE);
            } else {
                printf("0x%02X | %2d  |   %d   | ---  | -------- | --- | ---\n", i, j, cache[index].valid);
            }
        }
        // Add a separator between sets
        if (i < displayLimit - 1) printf("-----------------------------------------------------\n");
    }

    if (sets > displayLimit) {
        printf("... and %d more sets (not shown)\n", sets - displayLimit);
    }
    printf("\n");
}

// Print address breakdown for set associative cache
void printAssociativeAddressBreakdown(unsigned int address) {
    unsigned int l1_tag = address / (L1_SETS * WORDS_PER_LINE * WORD_SIZE);
    unsigned int l1_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
    unsigned int l2_tag = address / (L2_SETS * WORDS_PER_LINE * WORD_SIZE);
    unsigned int l2_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SETS;
    unsigned int word_offset = (address / WORD_SIZE) % WORDS_PER_LINE;
    unsigned int byte_offset = address % WORD_SIZE;

    printf("Address Breakdown (0x%04X):\n", address);
    printf("----------------------------------------\n");
    printf("Memory Architecture (Set Associative):\n");
    printf("- Word Size: %d bytes\n", WORD_SIZE);
    printf("- Words per Cache Line: %d words\n", WORDS_PER_LINE);
    printf("- Cache Line Size: %d bytes\n", WORD_SIZE * WORDS_PER_LINE);
    printf("- L1: %d-way set associative (%d sets)\n", L1_ASSOCIATIVITY, L1_SETS);
    printf("- L2: %d-way set associative (%d sets)\n\n", L2_ASSOCIATIVITY, L2_SETS);

    printf("Binary breakdown: ");
    // Show in binary with clear separations between TAG, SET, WORD parts
    for (int i = 15; i >= 0; i--) {
        int bit = (address >> i) & 1;

        // Add visual separators based on L1 cache
        if (i == 8) printf(" | "); // Between tag and set (adjusted for set associative)
        if (i == 4) printf(" | "); // Between set and word
        if (i == 2) printf(" | "); // Between word and byte

        printf("%d", bit);
    }
    printf("\n                  TAG | SET | WORD | BYTE\n\n");

    printf("L1 Cache Mapping (%d-way):\n", L1_ASSOCIATIVITY);
    printf("  TAG: 0x%02X  SET: 0x%01X  WORD: 0x%01X  BYTE: 0x%01X\n\n",
           l1_tag, l1_set, word_offset, byte_offset);

    printf("L2 Cache Mapping (%d-way):\n", L2_ASSOCIATIVITY);
    printf("  TAG: 0x%01X  SET: 0x%01X  WORD: 0x%01X  BYTE: 0x%01X\n\n",
           l2_tag, l2_set, word_offset, byte_offset);
}

// Main function for set associative cache simulation
int cacheSimulationSetAssociative(int t) {
    // Seed random number generator
    srand(time(NULL));

    int numAccesses;
    int i;

    // Statistics variables
    int l1Hits = 0, l1Misses = 0;
    int l2Hits = 0, l2Misses = 0;
    long totalCycles = 0;  // Use long for potential large values

    // Create cache structures for set associative caches
    AssociativeCacheLine l1Cache[L1_SETS * L1_ASSOCIATIVITY];
    AssociativeCacheLine l2Cache[L2_SETS * L2_ASSOCIATIVITY];

    // Initialize caches
    initializeAssociativeCache(l1Cache, L1_SETS, L1_ASSOCIATIVITY);
    initializeAssociativeCache(l2Cache, L2_SETS, L2_ASSOCIATIVITY);

    // User input
    printf("Memory Hierarchy Simulator (Set Associative) with TAG/SET/WORD Breakdown\n");
    printf("-------------------------------------------------------------------\n");
    printf("L1 Cache: %d-way set associative with %d sets\n", L1_ASSOCIATIVITY, L1_SETS);
    printf("L2 Cache: %d-way set associative with %d sets\n", L2_ASSOCIATIVITY, L2_SETS);
    printf("Enter the number of memory access attempts to simulate: ");
    scanf("%d", &numAccesses);

    // Allocate memory for addresses
    unsigned int *addresses = (unsigned int *)malloc(numAccesses * sizeof(unsigned int));
    if (addresses == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        return 1;
    }

    // Create array to store hit information
    HitInfo *hitInfoArray = (HitInfo *)malloc(numAccesses * sizeof(HitInfo));
    if (hitInfoArray == NULL) {
        printf("Memory allocation failed. Exiting...\n");
        free(addresses);
        return 1;
    }
    int hitCount = 0;

    // Generate random addresses
    generateAddresses(addresses, numAccesses);

    // Simulate memory accesses
    for (i = 0; i < numAccesses; i++) {
        unsigned int address = addresses[i];
        int tag, set, way;
        bool l1Hit, l2Hit;

        if(t==1){
            clearScreen();

        printf("Memory Access #%d (Set Associative)\n", i + 1);
        printf("----------------------------------\n");
        printf("Accessing address: 0x%04X\n\n", address);

        // Print address breakdown with TAG/SET/WORD
        printAssociativeAddressBreakdown(address);

        }

        // Check L1 Cache
        l1Hit = checkAssociativeCache(l1Cache, L1_SETS, L1_ASSOCIATIVITY, address, &tag, &set, &way);
        if(t==1) animateCheckL1();
        if (l1Hit) {
            // L1 Cache hit
            l1Hits++;
            totalCycles += L1_ACCESS_COST;

            // Record hit information
            hitInfoArray[hitCount].address = address;
            hitInfoArray[hitCount].cache_level = 1;  // L1 hit
            hitCount++;

           if(t==1){
             printf("L1 CACHE HIT!\n");
            printf("  TAG: 0x%02X  SET: 0x%01X  WAY: %d  WORD: 0x%01X\n",
                   tag, set, way, (address / WORD_SIZE) % WORDS_PER_LINE);
            printf("Access cost: %d cycles\n\n", L1_ACCESS_COST);

           }
            // Update LRU counters
            updateLRUCounters(l1Cache, set, L1_ASSOCIATIVITY, way);

            if(t==1){
                displayAssociativeCacheContents(l1Cache, L1_SETS, L1_ASSOCIATIVITY, "L1 Cache");
            if (hitCount > 0) {
                displayHitSummary(hitInfoArray, hitCount);
            } else {
                printf("No cache hits recorded during this simulation.\n");
            }

            printf("\nPress Enter to continue to next memory access...");
            while (getchar() != '\n'); // Clear input buffer
            getchar(); // Wait for user input
            }
        } else {
            // L1 Cache miss
            l1Misses++;

            if(t==1){
                printf("L1 CACHE MISS!\n");
            printf("  Attempted to find TAG: 0x%02X in SET: 0x%01X\n",
                   tag, set);

            }
            // Check L2 Cache
            l2Hit = checkAssociativeCache(l2Cache, L2_SETS, L2_ASSOCIATIVITY, address, &tag, &set, &way);
            if(t==1) animateCheckL2();
            if (l2Hit) {
                // L2 Cache hit
                l2Hits++;
                totalCycles += L2_ACCESS_COST;

                // Record hit information
                hitInfoArray[hitCount].address = address;
                hitInfoArray[hitCount].cache_level = 2;  // L2 hit
                hitCount++;

              if(t==1){
                  printf("\nL2 CACHE HIT!\n");
                printf("  TAG: 0x%02X  SET: 0x%01X  WAY: %d  WORD: 0x%01X\n",
                       tag, set, way, (address / WORD_SIZE) % WORDS_PER_LINE);
                printf("Access cost: %d cycles\n", L2_ACCESS_COST);
              }

                // Update LRU counters for L2
                updateLRUCounters(l2Cache, set, L2_ASSOCIATIVITY, way);

                // Calculate L1 parameters for update
                int l1Tag = address / (L1_SETS * WORDS_PER_LINE * WORD_SIZE);
                int l1Set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
                int l1Way = findLRUWay(l1Cache, l1Set, L1_ASSOCIATIVITY);

                // Update L1 Cache
                updateAssociativeCache(l1Cache, l1Set, l1Way, L1_ASSOCIATIVITY, l1Tag, address);
               if(t==1){
                 printf("Data loaded from L2 to L1 (placed in set 0x%01X, way %d)\n\n", l1Set, l1Way);

                displayAssociativeCacheContents(l2Cache, L2_SETS, L2_ASSOCIATIVITY, "L2 Cache");
                if (hitCount > 0) {
                    displayHitSummary(hitInfoArray, hitCount);
                } else {
                    printf("No cache hits recorded during this simulation.\n");
                }

                printf("\nPress Enter to continue to next memory access...");
                while (getchar() != '\n'); // Clear input buffer
                getchar(); // Wait for user input
               }
            } else {
                // L2 Cache miss
                l2Misses++;
                totalCycles += MEMORY_ACCESS_COST;

             if(t==1){
                   printf("\nL2 CACHE MISS!\n");
                printf("  Attempted to find TAG: 0x%02X in SET: 0x%01X\n",
                       tag, set);
                printf("ACCESSING MAIN MEMORY...\n");
                printf("Access cost: %d cycles\n", MEMORY_ACCESS_COST);
             }

                // Calculate L1 parameters for update
                int l1Tag = address / (L1_SETS * WORDS_PER_LINE * WORD_SIZE);
                int l1Set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
                int l1Way = findLRUWay(l1Cache, l1Set, L1_ASSOCIATIVITY);

                // Calculate L2 parameters for update
                int l2Tag = address / (L2_SETS * WORDS_PER_LINE * WORD_SIZE);
                int l2Set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SETS;
                int l2Way = findLRUWay(l2Cache, l2Set, L2_ASSOCIATIVITY);

                // Update L1 and L2 Caches (inclusive cache policy)
                updateAssociativeCache(l1Cache, l1Set, l1Way, L1_ASSOCIATIVITY, l1Tag, address);
                updateAssociativeCache(l2Cache, l2Set, l2Way, L2_ASSOCIATIVITY, l2Tag, address);
                if(t==1){

                printf("Data loaded from Main Memory to L1 (set 0x%01X, way %d) and L2 (set 0x%01X, way %d)\n\n",
                       l1Set, l1Way, l2Set, l2Way);

                // Display cache contents after update
                displayAssociativeCacheContents(l1Cache, L1_SETS, L1_ASSOCIATIVITY, "L1 Cache");
                displayAssociativeCacheContents(l2Cache, L2_SETS, L2_ASSOCIATIVITY, "L2 Cache");

                printf("\nPress Enter to continue to next memory access...");
               while (getchar() != '\n'); // Clear input buffer
               getchar(); // Wait for user input
                }
            }
        }
    }

    // Calculate final statistics
    float l1HitRatio = (float)l1Hits / numAccesses;
    float l2HitRatio = (l1Misses > 0) ? (float)l2Hits / l1Misses : 0;
    float amat = L1_ACCESS_COST + (1 - l1HitRatio) * (L2_ACCESS_COST + (1 - l2HitRatio) * MEMORY_ACCESS_COST);

    // Display final statistics
    clearScreen();
    printf("Memory Hierarchy Simulation Complete (Set Associative)\n");
    printf("---------------------------------------------------\n\n");
    printf("Cache Architecture:\n");
    printf("------------------\n");
    printf("  Cache Policy: Inclusive (L2 contains all entries in L1)\n");
    printf("  L1 Cache: %d-way set associative with %d sets, %d-byte lines (%d words per line)\n",
           L1_ASSOCIATIVITY, L1_SETS, WORDS_PER_LINE * WORD_SIZE, WORDS_PER_LINE);
    printf("  L2 Cache: %d-way set associative with %d sets, %d-byte lines (%d words per line)\n\n",
           L2_ASSOCIATIVITY, L2_SETS, WORDS_PER_LINE * WORD_SIZE, WORDS_PER_LINE);

    printf("Simulation Results:\n");
    printf("------------------\n");
    printf("Total memory accesses: %d\n\n", numAccesses);

    printf("L1 Cache Statistics:\n");
    printf("  Hits: %d (%.2f%%)\n", l1Hits, l1HitRatio * 100);
    printf("  Misses: %d (%.2f%%)\n\n", l1Misses, (1 - l1HitRatio) * 100);

    printf("L2 Cache Statistics:\n");
    printf("  Hits: %d (%.2f%%)\n", l2Hits, l2HitRatio * 100);
    printf("  Misses: %d (%.2f%%)\n\n", l2Misses, (1 - l2HitRatio) * 100);

    printf("Performance Metrics:\n");
    printf("  Total Cycle Cost: %ld cycles\n", totalCycles);
    printf("  Average Memory Access Time (AMAT): %.2f cycles\n\n", amat);

    // Display hit address summary
    if (hitCount > 0) {
        displayHitSummary(hitInfoArray, hitCount);
    } else {
        printf("No cache hits recorded during this simulation.\n");
    }

    // Free allocated memory
    free(addresses);
    free(hitInfoArray);
  printf("\n===============================================\n");
    printf("Set Associative Mapping analysis complete.\n");
    printf("Press Enter to return to main menu...");
    getchar();
    getchar();
    return 0;
}






// Function to run comparative analysis between all three mappinh
void compareAllCacheMappings(int numAccesses) {
    // Statistics for each cache type
    int dm_l1_hits = 0, dm_l2_hits = 0, dm_memory_accesses = 0;
    int fa_l1_hits = 0, fa_l2_hits = 0, fa_memory_accesses = 0;
    int sa_l1_hits = 0, sa_l2_hits = 0, sa_memory_accesses = 0;

    // Access costs
    int dm_total_cost = 0, fa_total_cost = 0, sa_total_cost = 0;

    // Addresses to use (same for all three schemes)
    unsigned int *addresses = malloc(numAccesses * sizeof(unsigned int));
    if (!addresses) {
        printf("Memory allocation failed!\n");
        return;
    }

    // Generate random addresses for testing
    srand(time(NULL));
    generateAddresses(addresses, numAccesses);

    // Arrays to store hit information for each cache scheme
    HitInfo *dm_hits = malloc(numAccesses * sizeof(HitInfo));
    HitInfo *fa_hits = malloc(numAccesses * sizeof(HitInfo));
    HitInfo *sa_hits = malloc(numAccesses * sizeof(HitInfo));

    if (!dm_hits || !fa_hits || !sa_hits) {
        printf("Memory allocation failed!\n");
        free(addresses);
        if (dm_hits) free(dm_hits);
        if (fa_hits) free(fa_hits);
        if (sa_hits) free(sa_hits);
        return;
    }

    int dm_hit_count = 0, fa_hit_count = 0, sa_hit_count = 0;

    printf("Comparing cache mapping schemes with %d memory accesses\n", numAccesses);
    printf("-------------------------------------------------------\n\n");

    // 1. Set up Direct-Mapped Caches
    CacheLine l1_cache_dm[L1_SIZE];
    CacheLine l2_cache_dm[L2_SIZE];
    initializeCache(l1_cache_dm, L1_SIZE);
    initializeCache(l2_cache_dm, L2_SIZE);

    // 2. Set up Fully Associative Caches
    FullyAssociativeCacheLine l1_cache_fa[L1_SIZE];
    FullyAssociativeCacheLine l2_cache_fa[L2_SIZE];
    initializeFullyAssociativeCache(l1_cache_fa, L1_SIZE);
    initializeFullyAssociativeCache(l2_cache_fa, L2_SIZE);

    // 3. Set up Set-Associative Caches
    AssociativeCacheLine l1_cache_sa[L1_SIZE]; // Total size remains the same
    AssociativeCacheLine l2_cache_sa[L2_SIZE]; // Total size remains the same
    initializeAssociativeCache(l1_cache_sa, L1_SETS, L1_ASSOCIATIVITY);
    initializeAssociativeCache(l2_cache_sa, L2_SETS, L2_ASSOCIATIVITY);

    // Run the simulation for each address
    for (int i = 0; i < numAccesses; i++) {
        unsigned int address = addresses[i];
        int tag, index, way, set;

        // Direct-Mapped Cache Simulation

        // Check L1 cache
        bool l1_hit_dm = checkCache(l1_cache_dm, L1_SIZE, address, &tag, &index);

        if (l1_hit_dm) {
            // L1 hit
            dm_l1_hits++;
            dm_total_cost += L1_ACCESS_COST;
            dm_hits[dm_hit_count].address = address;
            dm_hits[dm_hit_count].cache_level = 1;
            dm_hit_count++;
        } else {
            // Check L2 cache
            bool l2_hit_dm = checkCache(l2_cache_dm, L2_SIZE, address, &tag, &index);

            if (l2_hit_dm) {
                // L2 hit
                dm_l2_hits++;
                dm_total_cost += (L1_ACCESS_COST + L2_ACCESS_COST);
                dm_hits[dm_hit_count].address = address;
                dm_hits[dm_hit_count].cache_level = 2;
                dm_hit_count++;

                // Update L1 cache
                int l1_tag, l1_index;
                checkCache(l1_cache_dm, L1_SIZE, address, &l1_tag, &l1_index);
                updateCache(l1_cache_dm, l1_index, l1_tag, address);
            } else {
                // Cache miss - access main memory
                dm_memory_accesses++;
                dm_total_cost += (L1_ACCESS_COST + L2_ACCESS_COST + MEMORY_ACCESS_COST);

                // Update L2 cache
                int l2_tag, l2_index;
                checkCache(l2_cache_dm, L2_SIZE, address, &l2_tag, &l2_index);
                updateCache(l2_cache_dm, l2_index, l2_tag, address);

                // Update L1 cache
                int l1_tag, l1_index;
                checkCache(l1_cache_dm, L1_SIZE, address, &l1_tag, &l1_index);
                updateCache(l1_cache_dm, l1_index, l1_tag, address);
            }
        }


        // Fully Associative Cache Simulation

        // Check L1 cache
        bool l1_hit_fa = checkFullyAssociativeCache(l1_cache_fa, L1_SIZE, address, &tag, &way);

        if (l1_hit_fa) {
            // L1 hit
            fa_l1_hits++;
            fa_total_cost += L1_ACCESS_COST;
            fa_hits[fa_hit_count].address = address;
            fa_hits[fa_hit_count].cache_level = 1;
            fa_hit_count++;

            // Update LRU
            updateFullyAssociativeLRU(l1_cache_fa, L1_SIZE, way);
        } else {
            // Check L2 cache
            bool l2_hit_fa = checkFullyAssociativeCache(l2_cache_fa, L2_SIZE, address, &tag, &way);

            if (l2_hit_fa) {
                // L2 hit
                fa_l2_hits++;
                fa_total_cost += (L1_ACCESS_COST + L2_ACCESS_COST);
                fa_hits[fa_hit_count].address = address;
                fa_hits[fa_hit_count].cache_level = 2;
                fa_hit_count++;

                // Update LRU for L2
                updateFullyAssociativeLRU(l2_cache_fa, L2_SIZE, way);

                // Update L1 cache - need to find a place in L1 using LRU
                int l1_way = findFullyAssociativeLRU(l1_cache_fa, L1_SIZE);
                int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE);
                updateFullyAssociativeCache(l1_cache_fa, L1_SIZE, l1_way, l1_tag, address);
            } else {
                // Cache miss - access main memory
                fa_memory_accesses++;
                fa_total_cost += (L1_ACCESS_COST + L2_ACCESS_COST + MEMORY_ACCESS_COST);

                // Update L2 cache - need to find a place in L2 using LRU
                int l2_way = findFullyAssociativeLRU(l2_cache_fa, L2_SIZE);
                int l2_tag = address / (WORDS_PER_LINE * WORD_SIZE);
                updateFullyAssociativeCache(l2_cache_fa, L2_SIZE, l2_way, l2_tag, address);

                // Update L1 cache - need to find a place in L1 using LRU
                int l1_way = findFullyAssociativeLRU(l1_cache_fa, L1_SIZE);
                int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE);
                updateFullyAssociativeCache(l1_cache_fa, L1_SIZE, l1_way, l1_tag, address);
            }
        }


        // Set-Associative Cache Simulation

        // Check L1 cache
        bool l1_hit_sa = checkAssociativeCache(l1_cache_sa, L1_SETS, L1_ASSOCIATIVITY, address, &tag, &set, &way);

        if (l1_hit_sa) {
            // L1 hit
            sa_l1_hits++;
            sa_total_cost += L1_ACCESS_COST;
            sa_hits[sa_hit_count].address = address;
            sa_hits[sa_hit_count].cache_level = 1;
            sa_hit_count++;

            // Update LRU
            updateLRUCounters(l1_cache_sa, set, L1_ASSOCIATIVITY, way);
        } else {
            // Check L2 cache
            bool l2_hit_sa = checkAssociativeCache(l2_cache_sa, L2_SETS, L2_ASSOCIATIVITY, address, &tag, &set, &way);

            if (l2_hit_sa) {
                // L2 hit
                sa_l2_hits++;
                sa_total_cost += (L1_ACCESS_COST + L2_ACCESS_COST);
                sa_hits[sa_hit_count].address = address;
                sa_hits[sa_hit_count].cache_level = 2;
                sa_hit_count++;

                // Update LRU for L2
                updateLRUCounters(l2_cache_sa, set, L2_ASSOCIATIVITY, way);

                // Update L1 cache
                int l1_tag, l1_set;
                l1_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
                l1_tag = address / (L1_SETS * WORDS_PER_LINE * WORD_SIZE);
                int l1_way = findLRUWay(l1_cache_sa, l1_set, L1_ASSOCIATIVITY);
                updateAssociativeCache(l1_cache_sa, l1_set, l1_way, L1_ASSOCIATIVITY, l1_tag, address);
            } else {
                // Cache miss - access main memory
                sa_memory_accesses++;
                sa_total_cost += (L1_ACCESS_COST + L2_ACCESS_COST + MEMORY_ACCESS_COST);

                // Update L2 cache
                int l2_tag, l2_set;
                l2_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SETS;
                l2_tag = address / (L2_SETS * WORDS_PER_LINE * WORD_SIZE);
                int l2_way = findLRUWay(l2_cache_sa, l2_set, L2_ASSOCIATIVITY);
                updateAssociativeCache(l2_cache_sa, l2_set, l2_way, L2_ASSOCIATIVITY, l2_tag, address);

                // Update L1 cache
                int l1_tag, l1_set;
                l1_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
                l1_tag = address / (L1_SETS * WORDS_PER_LINE * WORD_SIZE);
                int l1_way = findLRUWay(l1_cache_sa, l1_set, L1_ASSOCIATIVITY);
                updateAssociativeCache(l1_cache_sa, l1_set, l1_way, L1_ASSOCIATIVITY, l1_tag, address);
            }
        }
    }

    // Display the results
    clearScreen();

    printf("Cache Comparison Results (%d accesses):\n", numAccesses);
    printf("=======================================\n\n");
    printf("1. Direct-Mapped Cache Performance:\n");
    printf("----------------------------------\n");
    printf("L1 Cache Hits: %d (%.2f%%)\n", dm_l1_hits, (float)dm_l1_hits/numAccesses*100);
    printf("L2 Cache Hits: %d (%.2f%%)\n", dm_l2_hits, (float)dm_l2_hits/numAccesses*100);
    printf("Memory Accesses: %d (%.2f%%)\n", dm_memory_accesses, (float)dm_memory_accesses/numAccesses*100);
    printf("Total Hit Rate: %.2f%%\n", (float)(dm_l1_hits + dm_l2_hits)/numAccesses*100);
    printf("Total Access Cost: %d\n", dm_total_cost);
    printf("Average Access Time: %.2f cycles/access\n\n", (float)dm_total_cost/numAccesses);
    printf("\n2. Fully Associative Cache Performance:\n");
    printf("---------------------------------------\n");
    printf("L1 Cache Hits: %d (%.2f%%)\n", fa_l1_hits, (float)fa_l1_hits/numAccesses*100);
    printf("L2 Cache Hits: %d (%.2f%%)\n", fa_l2_hits, (float)fa_l2_hits/numAccesses*100);
    printf("Memory Accesses: %d (%.2f%%)\n", fa_memory_accesses, (float)fa_memory_accesses/numAccesses*100);
    printf("Total Hit Rate: %.2f%%\n", (float)(fa_l1_hits + fa_l2_hits)/numAccesses*100);
    printf("Total Access Cost: %d\n", fa_total_cost);
    printf("Average Access Time: %.2f cycles/access\n\n", (float)fa_total_cost/numAccesses);
    printf("\n3. Set-Associative Cache Performance:\n");
    printf("-------------------------------------\n");
    printf("L1 Cache Hits: %d (%.2f%%)\n", sa_l1_hits, (float)sa_l1_hits/numAccesses*100);
    printf("L2 Cache Hits: %d (%.2f%%)\n", sa_l2_hits, (float)sa_l2_hits/numAccesses*100);
    printf("Memory Accesses: %d (%.2f%%)\n", sa_memory_accesses, (float)sa_memory_accesses/numAccesses*100);
    printf("Total Hit Rate: %.2f%%\n", (float)(sa_l1_hits + sa_l2_hits)/numAccesses*100);
    printf("Total Access Cost: %d\n", sa_total_cost);
    printf("Average Access Time: %.2f cycles/access\n\n", (float)sa_total_cost/numAccesses);
    printf("\nComparative Analysis:\n");
    printf("--------------------\n");

    // Find the best performer
    int best_hit_rate = -1;
    char *best_scheme = NULL;

    float dm_hit_rate = (float)(dm_l1_hits + dm_l2_hits)/numAccesses*100;
    float fa_hit_rate = (float)(fa_l1_hits + fa_l2_hits)/numAccesses*100;
    float sa_hit_rate = (float)(sa_l1_hits + sa_l2_hits)/numAccesses*100;

    if (dm_hit_rate >= fa_hit_rate && dm_hit_rate >= sa_hit_rate) {
        best_hit_rate = dm_hit_rate;
        best_scheme = "Direct-Mapped";
    } else if (fa_hit_rate >= dm_hit_rate && fa_hit_rate >= sa_hit_rate) {
        best_hit_rate = fa_hit_rate;
        best_scheme = "Fully Associative";
    } else {
        best_hit_rate = sa_hit_rate;
        best_scheme = "Set-Associative";
    }

    int best_cost = -1;
    char *best_cost_scheme = NULL;

    if (dm_total_cost <= fa_total_cost && dm_total_cost <= sa_total_cost) {
        best_cost = dm_total_cost;
        best_cost_scheme = "Direct-Mapped";
    } else if (fa_total_cost <= dm_total_cost && fa_total_cost <= sa_total_cost) {
        best_cost = fa_total_cost;
        best_cost_scheme = "Fully Associative";
    } else {
        best_cost = sa_total_cost;
        best_cost_scheme = "Set-Associative";
    }

  //  printf("Best Hit Rate: %s (%.2f%%)\n", best_scheme, best_hit_rate);
  //  printf("Best Access Cost: %s (%d cycles)\n\n", best_cost_scheme, best_cost);

    printf("Hit Rate Comparison:\n");
    printf("- Direct-Mapped: %.2f%%\n", dm_hit_rate);
    printf("- Fully Associative: %.2f%%\n", fa_hit_rate);
    printf("- Set-Associative: %.2f%%\n\n", sa_hit_rate);

    printf("Average Access Time Comparison:\n");
    printf("- Direct-Mapped: %.2f cycles/access\n", (float)dm_total_cost/numAccesses);
    printf("- Fully Associative: %.2f cycles/access\n", (float)fa_total_cost/numAccesses);
    printf("- Set-Associative: %.2f cycles/access\n\n", (float)sa_total_cost/numAccesses);

    // Clean up memory
    free(addresses);
    free(dm_hits);
    free(fa_hits);
    free(sa_hits);
}

void runPredefinedAddressPattern( int numAccesses) {
    clearScreen();
    printf("Running Cache Comparisons with Specific Address Patterns\n");
    printf("=====================================================\n\n");

    // Create a simpler comparison function specifically for address patterns
    void compareWithPattern(unsigned int *addresses, int numAccesses, const char *patternName) {
        // Statistics for each cache type
        CacheStats dmStats = {0}, faStats = {0}, saStats = {0};

        // Initialize caches
        CacheLine *l1_cache_dm = calloc(L1_SIZE, sizeof(CacheLine));
        CacheLine *l2_cache_dm = calloc(L2_SIZE, sizeof(CacheLine));
        FullyAssociativeCacheLine *l1_cache_fa = calloc(L1_SIZE, sizeof(FullyAssociativeCacheLine));
        FullyAssociativeCacheLine *l2_cache_fa = calloc(L2_SIZE, sizeof(FullyAssociativeCacheLine));
        AssociativeCacheLine *l1_cache_sa = calloc(L1_SIZE, sizeof(AssociativeCacheLine));
        AssociativeCacheLine *l2_cache_sa = calloc(L2_SIZE, sizeof(AssociativeCacheLine));

        // Check for memory allocation failures
        if (!l1_cache_dm || !l2_cache_dm || !l1_cache_fa || !l2_cache_fa || !l1_cache_sa || !l2_cache_sa) {
            fprintf(stderr, "Memory allocation failed for pattern analysis caches!\n");
            goto cleanup;
        }

        // Initialize caches
        initializeCache(l1_cache_dm, L1_SIZE);
        initializeCache(l2_cache_dm, L2_SIZE);
        initializeFullyAssociativeCache(l1_cache_fa, L1_SIZE);
        initializeFullyAssociativeCache(l2_cache_fa, L2_SIZE);
        initializeAssociativeCache(l1_cache_sa, L1_SETS, L1_ASSOCIATIVITY);
        initializeAssociativeCache(l2_cache_sa, L2_SETS, L2_ASSOCIATIVITY);

        // Process each memory access for each cache type
        for (int i = 0; i < numAccesses; i++) {
            unsigned int address = addresses[i];
            int tag, index, way, set;

            // Direct-Mapped Cache Simulation
            bool l1_hit_dm = checkCache(l1_cache_dm, L1_SIZE, address, &tag, &index);
            if (l1_hit_dm) {
                dmStats.l1_hits++;
                dmStats.total_cost += L1_ACCESS_COST;
            } else {
                bool l2_hit_dm = checkCache(l2_cache_dm, L2_SIZE, address, &tag, &index);
                if (l2_hit_dm) {
                    dmStats.l2_hits++;
                    dmStats.total_cost += (L1_ACCESS_COST + L2_ACCESS_COST);
                    int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE * L1_SIZE);
                    int l1_index = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE;
                    updateCache(l1_cache_dm, l1_index, l1_tag, address);
                } else {
                    dmStats.memory_accesses++;
                    dmStats.total_cost += (L1_ACCESS_COST + L2_ACCESS_COST + MEMORY_ACCESS_COST);
                    int l2_tag = address / (WORDS_PER_LINE * WORD_SIZE * L2_SIZE);
                    int l2_index = (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SIZE;
                    updateCache(l2_cache_dm, l2_index, l2_tag, address);
                    int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE * L1_SIZE);
                    int l1_index = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SIZE;
                    updateCache(l1_cache_dm, l1_index, l1_tag, address);
                }
            }

            // Fully Associative Cache Simulation
            bool l1_hit_fa = checkFullyAssociativeCache(l1_cache_fa, L1_SIZE, address, &tag, &way);
            if (l1_hit_fa) {
                faStats.l1_hits++;
                faStats.total_cost += L1_ACCESS_COST;
                updateFullyAssociativeLRU(l1_cache_fa, L1_SIZE, way);
            } else {
                bool l2_hit_fa = checkFullyAssociativeCache(l2_cache_fa, L2_SIZE, address, &tag, &way);
                if (l2_hit_fa) {
                    faStats.l2_hits++;
                    faStats.total_cost += (L1_ACCESS_COST + L2_ACCESS_COST);
                    updateFullyAssociativeLRU(l2_cache_fa, L2_SIZE, way);
                    int l1_way = findFullyAssociativeLRU(l1_cache_fa, L1_SIZE);
                    int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE);
                    updateFullyAssociativeCache(l1_cache_fa, L1_SIZE, l1_way, l1_tag, address);
                } else {
                    faStats.memory_accesses++;
                    faStats.total_cost += (L1_ACCESS_COST + L2_ACCESS_COST + MEMORY_ACCESS_COST);
                    int l2_way = findFullyAssociativeLRU(l2_cache_fa, L2_SIZE);
                    int l2_tag = address / (WORDS_PER_LINE * WORD_SIZE);
                    updateFullyAssociativeCache(l2_cache_fa, L2_SIZE, l2_way, l2_tag, address);
                    int l1_way = findFullyAssociativeLRU(l1_cache_fa, L1_SIZE);
                    int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE);
                    updateFullyAssociativeCache(l1_cache_fa, L1_SIZE, l1_way, l1_tag, address);
                }
            }

            // Set-Associative Cache Simulation
            bool l1_hit_sa = checkAssociativeCache(l1_cache_sa, L1_SETS, L1_ASSOCIATIVITY, address, &tag, &set, &way);
            if (l1_hit_sa) {
                saStats.l1_hits++;
                saStats.total_cost += L1_ACCESS_COST;
                updateLRUCounters(l1_cache_sa, set, L1_ASSOCIATIVITY, way);
            } else {
                bool l2_hit_sa = checkAssociativeCache(l2_cache_sa, L2_SETS, L2_ASSOCIATIVITY, address, &tag, &set, &way);
                if (l2_hit_sa) {
                    saStats.l2_hits++;
                    saStats.total_cost += (L1_ACCESS_COST + L2_ACCESS_COST);
                    updateLRUCounters(l2_cache_sa, set, L2_ASSOCIATIVITY, way);
                    int l1_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
                    int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE * L1_SETS);
                    int l1_way = findLRUWay(l1_cache_sa, l1_set, L1_ASSOCIATIVITY);
                    updateAssociativeCache(l1_cache_sa, l1_set, l1_way, L1_ASSOCIATIVITY, l1_tag, address);
                } else {
                    saStats.memory_accesses++;
                    saStats.total_cost += (L1_ACCESS_COST + L2_ACCESS_COST + MEMORY_ACCESS_COST);
                    int l2_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L2_SETS;
                    int l2_tag = address / (WORDS_PER_LINE * WORD_SIZE * L2_SETS);
                    int l2_way = findLRUWay(l2_cache_sa, l2_set, L2_ASSOCIATIVITY);
                    updateAssociativeCache(l2_cache_sa, l2_set, l2_way, L2_ASSOCIATIVITY, l2_tag, address);
                    int l1_set = (address / (WORDS_PER_LINE * WORD_SIZE)) % L1_SETS;
                    int l1_tag = address / (WORDS_PER_LINE * WORD_SIZE * L1_SETS);
                    int l1_way = findLRUWay(l1_cache_sa, l1_set, L1_ASSOCIATIVITY);
                    updateAssociativeCache(l1_cache_sa, l1_set, l1_way, L1_ASSOCIATIVITY, l1_tag, address);
                }
            }
        }

        // Calculate hit rates and average access times
        dmStats.hit_rate = (float)(dmStats.l1_hits + dmStats.l2_hits) / numAccesses * 100;
        dmStats.avg_access_time = (float)dmStats.total_cost / numAccesses;

        faStats.hit_rate = (float)(faStats.l1_hits + faStats.l2_hits) / numAccesses * 100;
        faStats.avg_access_time = (float)faStats.total_cost / numAccesses;

        saStats.hit_rate = (float)(saStats.l1_hits + saStats.l2_hits) / numAccesses * 100;
        saStats.avg_access_time = (float)saStats.total_cost / numAccesses;

        // Print results for this address pattern
        printf("\n\nResults for %s Pattern (%d accesses):\n", patternName, numAccesses);
        printf("-----------------------------------------\n");

        printf("                     | Direct-Mapped | Fully Associative | Set-Associative |\n");
        printf("---------------------------------------------------------------------\n");
        printf("L1 Hit Rate          | %6.2f%%      | %6.2f%%          | %6.2f%%        |\n",
               (float)dmStats.l1_hits/numAccesses*100,
               (float)faStats.l1_hits/numAccesses*100,
               (float)saStats.l1_hits/numAccesses*100);
        printf("L2 Hit Rate          | %6.2f%%      | %6.2f%%          | %6.2f%%        |\n",
               (float)dmStats.l2_hits/numAccesses*100,
               (float)faStats.l2_hits/numAccesses*100,
               (float)saStats.l2_hits/numAccesses*100);
        printf("Total Hit Rate       | %6.2f%%      | %6.2f%%          | %6.2f%%        |\n",
               dmStats.hit_rate, faStats.hit_rate, saStats.hit_rate);
        printf("Avg Access Time      | %6.2f cycles | %6.2f cycles     | %6.2f cycles   |\n",
               dmStats.avg_access_time, faStats.avg_access_time, saStats.avg_access_time);

        // Identify best performing strategy for this pattern
        printf("\nBest cache for %s pattern: ", patternName);

        if (dmStats.avg_access_time <= faStats.avg_access_time && dmStats.avg_access_time <= saStats.avg_access_time) {
            printf("Direct-Mapped (%.2f cycles/access)\n", dmStats.avg_access_time);
        } else if (faStats.avg_access_time <= dmStats.avg_access_time && faStats.avg_access_time <= saStats.avg_access_time) {
            printf("Fully Associative (%.2f cycles/access)\n", faStats.avg_access_time);
        } else {
            printf("Set-Associative (%.2f cycles/access)\n", saStats.avg_access_time);
        }

    cleanup:
        // Free memory
        free(l1_cache_dm);
        free(l2_cache_dm);
        free(l1_cache_fa);
        free(l2_cache_fa);
        free(l1_cache_sa);
        free(l2_cache_sa);
    }



    printf("Testing each cache mapping scheme with three common memory access patterns:\n");
    printf("1. Sequential Access: Accessing consecutive memory addresses\n");
    printf("2. Random Access: Accessing memory randomly\n");
    printf("3. Repeated Access: Repeatedly accessing a small set of addresses\n\n");

    // 1. Sequential access pattern (good spatial locality)
    unsigned int *seqAddresses = malloc(numAccesses * sizeof(unsigned int));
    if (seqAddresses) {
        printf("Generating sequential access pattern...\n");
        for (int i = 0; i < numAccesses; i++) {
            seqAddresses[i] = (i * WORD_SIZE) % ADDRESS_SPACE;
             // Sequential addresses
        }
        compareWithPattern(seqAddresses, numAccesses, "Sequential");
        free(seqAddresses);
    } else {
        fprintf(stderr, "Memory allocation failed for sequential addresses!\n");
    }



    // 2. Random access pattern (poor locality)
    unsigned int *randomAddresses = malloc(numAccesses * sizeof(unsigned int));
    if (randomAddresses) {
        printf("\nGenerating random access pattern...\n");
        srand((unsigned int)time(NULL));
        for (int i = 0; i < numAccesses; i++) {
            randomAddresses[i] = (rand() % (ADDRESS_SPACE / WORD_SIZE)) * WORD_SIZE;
             // Random addresses
        }
        compareWithPattern(randomAddresses, numAccesses, "Random");
        free(randomAddresses);
    } else {
        fprintf(stderr, "Memory allocation failed for random addresses!\n");
    }

    // 3. Repeated access pattern (tests temporal locality)
    unsigned int *repeatedAddresses = malloc(numAccesses * sizeof(unsigned int));
    if (repeatedAddresses) {
        printf("\nGenerating repeated access pattern...\n");
        int numUniqueAddresses = 20;
        unsigned int *uniqueAddresses = malloc(numUniqueAddresses * sizeof(unsigned int));
        if (uniqueAddresses) {
            // Generate a small set of unique addresses
            for (int i = 0; i < numUniqueAddresses; i++) {
                uniqueAddresses[i] = (rand() % (ADDRESS_SPACE / WORD_SIZE)) * WORD_SIZE;
            }
            // Create a pattern that repeatedly accesses these addresses
            for (int i = 0; i < numAccesses; i++) {
                repeatedAddresses[i] = uniqueAddresses[i % numUniqueAddresses];
            }
            free(uniqueAddresses);
            compareWithPattern(repeatedAddresses, numAccesses, "Repeated");
        } else {
            fprintf(stderr, "Memory allocation failed for unique addresses!\n");
        }
        free(repeatedAddresses);
    } else {
        fprintf(stderr, "Memory allocation failed for repeated addresses!\n");
    }

    printf("\n===============================================\n");
    printf("Address pattern analysis complete.\n");
    printf("Press Enter to return to main menu...");
    getchar();  // Wait for user input
}

void runCacheMappingComparison() {
    clearScreen();
    printf("Cache Mapping Comparison Utility\n");
    printf("================================\n\n");

    printf("This program compares the performance of three cache mapping technique:\n");
    printf("1. Direct-Mapped Cache\n");
    printf("2. Fully Associative Cache\n");
    printf("3. Set-Associative Cache\n\n");

    printf("Cache Parameters:\n");
    printf("- L1 Cache Size: %d entries\n", L1_SIZE);
    printf("- L2 Cache Size: %d entries\n", L2_SIZE);
    printf("- Block Size: %d bytes\n", BLOCK_SIZE);
    printf("- Word Size: %d bytes\n", WORD_SIZE);
    printf("- L1 Set Associativity: %d-way\n", L1_ASSOCIATIVITY);
    printf("- L2 Set Associativity: %d-way\n", L2_ASSOCIATIVITY);
    printf("- L1 Access Cost: %d cycles\n", L1_ACCESS_COST);
    printf("- L2 Access Cost: %d cycles\n", L2_ACCESS_COST);
    printf("- Memory Access Cost: %d cycles\n\n", MEMORY_ACCESS_COST);

    // Ask the user for number of memory accesses to simulate
    int numAccesses;  // Default value
    printf("Enter number of memory accesses to simulate: ");
     //   printf("Enter the number of Test Addresses:");
    scanf("%d",&numAccesses);
    char input[20];
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (sscanf(input, "%d", &numAccesses) != 1) {
          //  numAccesses = 1000;  // Use default if invalid input
        }
    }

    printf("\nRunning cache comparison with %d memory accesses...\n\n", numAccesses);
    simulateDelay();

    // Run the comparison
    compareAllCacheMappings(numAccesses);

    // Optionally run predefined patterns
    printf("\nWould you like to run additional analysis with predefined address patterns? (y/n): ");
    if (fgets(input, sizeof(input), stdin) != NULL) {
        if (input[0] == 'y' || input[0] == 'Y') {
            runPredefinedAddressPattern(numAccesses);
        }
    }

    printf("\nCache mapping comparison complete.\n");
}




int main() {
    while (true) {
        clearScreen();
        int choice;

        printf("\n============================================================\n");
        printf("                 MEMORY HIERARCHY SIMULATION        \n");
        printf("============================================================\n");
        printf("  [1] Simulate Direct Mapping\n");
        printf("  [2] Simulate Fully Associative Mapping\n");
        printf("  [3] Simulate Set Associative Mapping\n");
        printf("  [4] Comparison and Analysis of All Three Mapping Techniques\n\n");
        printf("  [0] Exit\n");
        printf("============================================================\n");
        printf("Choose an option: ");
        scanf("%d", &choice);

        if (choice == 1) {
            while (true) {
                clearScreen();
                int subChoice;

                printf("\n============================================\n");
                printf("         DIRECT MAPPING MECHANISM           \n");
                printf("============================================\n");
                printf("  [1] Simulate Direct Mapping Mechanism\n");
                printf("  [2] Analyze Cache Performance\n\n");
                printf("  [0] Back \n");
                printf("============================================\n");
                printf("Choose an option: ");
                scanf("%d", &subChoice);

                if (subChoice == 1) {
                    cacheSimulation(1);
                } else if (subChoice == 2) {
                    cacheSimulation(2);
                } else if (subChoice == 0) {
                    break;
                } else {
                    printf("Invalid option! Press Enter to try again...\n");
                    getchar(); getchar();
                }
            }
        }

        else if (choice == 2) {
            while (true) {
                clearScreen();
                int subChoice;

                printf("\n============================================\n");
                printf("     FULLY ASSOCIATIVE MAPPING MECHANISM    \n");
                printf("============================================\n");
                printf("  [1] Simulate Fully Associative Mechanism\n");
                printf("  [2] Analyze Cache Performance\n\n");
                printf("  [0] Back \n");
                printf("============================================\n");
                printf("Choose an option: ");
                scanf("%d", &subChoice);

                if (subChoice == 1) {
                    cacheSimulationFullyAssociative(1);
                } else if (subChoice == 2) {
                    cacheSimulationFullyAssociative(2);
                } else if (subChoice == 0) {
                    break;
                } else {
                    printf("Invalid option! Press Enter to try again...\n");
                    getchar(); getchar();
                }
            }
        }

        else if (choice == 3) {
            while (true) {
                clearScreen();
                int subChoice;

                printf("\n============================================\n");
                printf("       SET ASSOCIATIVE MAPPING MECHANISM    \n");
                printf("============================================\n");
                printf("  [1] Simulate Set Associative Mechanism\n");
                printf("  [2] Analyze Cache Performance\n\n");
                printf("  [0] Back\n");
                printf("============================================\n");
                printf("Choose an option: ");
                scanf("%d", &subChoice);

                if (subChoice == 1) {
                    cacheSimulationSetAssociative(1);
                } else if (subChoice == 2) {
                    cacheSimulationSetAssociative(2);
                } else if (subChoice == 0) {
                    break;
                } else {
                    printf("Invalid option! Press Enter to try again...\n");
                    getchar(); getchar();
                }
            }
        }

        else if (choice == 4) {
            runCacheMappingComparison();
        }

        else if (choice == 0) {
            printf("\nExiting the simulation. Goodbye!\n");
            break;
        }

        else {
            printf("Invalid option! Press Enter to try again...\n");
            getchar(); getchar();
        }
    }

    return 0;
}
