#ifndef __CACHE_H__
#define __CACHE_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

#include "Cache_Blk.h"
#include "Request.h"

// Uncomment the replacement policy you want to use
//#define LRU
//#define LFU
#define SHP_REPLACEMENT

#define MAX_PREDICTOR_COUNTER 3

/* Cache */
typedef struct Set
{
    Cache_Block **ways; // Block ways within a set
} Set;

/* Signature Hit Predictor Structures */
typedef struct
{
    uint8_t counter;
} SHP_Entry;


typedef struct SignatureHitPredictor
{
    SHP_Entry *entries;
    unsigned table_size;
} SignatureHitPredictor;

typedef struct Cache
{
    uint64_t blk_mask;
    unsigned num_blocks;

    Cache_Block *blocks; // All cache blocks

    /* Set-Associative Information */
    unsigned num_sets;  // Number of sets
    unsigned num_ways;  // Number of ways within a set

    unsigned set_shift;
    unsigned set_mask;  // To extract set index
    unsigned tag_shift; // To extract tag

    Set *sets; // All the sets of a cache

    /* Signature Hit Predictor */
    SignatureHitPredictor *shp_table;

    unsigned max_rrpv;
    uint64_t write_back_count;

} Cache;

// Function Definitions
Cache *initCache();
bool accessBlock(Cache *cache, Request *req, uint64_t access_time);
bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr);
void freeCache(Cache *cache);

// Helper Functions
uint64_t blkAlign(uint64_t addr, uint64_t mask);
Cache_Block *findBlock(Cache *cache, uint64_t addr);

// Replacement Policies
bool lru(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr);
bool lfu(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr);
bool signature_hit_predictor(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr, uint64_t signature);

/* Signature Hit Predictor Function Declarations */
SignatureHitPredictor *initSignatureHitPredictor(unsigned table_size);
void updateSignatureHitPredictor(SignatureHitPredictor *shp, uint64_t signature, bool hit);
bool predictHit(SignatureHitPredictor *shp, uint64_t signature);

#endif /* __CACHE_H__ */