#include "Cache.h"


/* Constants */
#define SHP_TABLE_SIZE 1024 // Adjust the size as needed

/* Constants */
const unsigned block_size = 64; // Size of a cache line (in Bytes)
// TODO, you should try different sizes of cache, for example, 128KB, 256KB, 512KB, 1MB, 2MB
const unsigned cache_size = 128; // Size of a cache (in KB)
// TODO, you should try different associativity configurations, for example, 4, 8, 16
const unsigned assoc = 16;

Cache *initCache()
{
    Cache *cache = (Cache *)malloc(sizeof(Cache));
    cache->write_back_count = 0;
    cache->blk_mask = block_size - 1;
    cache->max_rrpv = 3; 
    unsigned num_blocks = cache_size * 1024 / block_size;
    cache->num_blocks = num_blocks;
    // printf("Num of blocks: %u\n", cache->num_blocks);

    // Initialize all cache blocks
    cache->blocks = (Cache_Block *)malloc(num_blocks * sizeof(Cache_Block));

    for (unsigned i = 0; i < num_blocks; i++)
    {
        cache->blocks[i].tag = UINT64_MAX; // Use UINT64_MAX for 64-bit tags
        cache->blocks[i].valid = false;
        cache->blocks[i].dirty = false;
        cache->blocks[i].when_touched = 0;
        cache->blocks[i].frequency = 0;
    }

    // Initialize Set-way variables
    unsigned num_sets = cache_size * 1024 / (block_size * assoc);
    cache->num_sets = num_sets;
    cache->num_ways = assoc;
    // printf("Num of sets: %u\n", cache->num_sets);

    unsigned set_shift = (unsigned)log2(block_size);
    cache->set_shift = set_shift;
    // printf("Set shift: %u\n", cache->set_shift);

    unsigned set_mask = num_sets - 1;
    cache->set_mask = set_mask;
    // printf("Set mask: %u\n", cache->set_mask);

    unsigned tag_shift = set_shift + (unsigned)log2(num_sets);
    cache->tag_shift = tag_shift;
    // printf("Tag shift: %u\n", cache->tag_shift);

    // Initialize Sets
    cache->sets = (Set *)malloc(num_sets * sizeof(Set));
    for (unsigned i = 0; i < num_sets; i++)
    {
        cache->sets[i].ways = (Cache_Block **)malloc(assoc * sizeof(Cache_Block *));
    }

    // Combine sets and blocks
    for (unsigned i = 0; i < num_blocks; i++)
    {
        Cache_Block *blk = &(cache->blocks[i]);

        unsigned set = i / assoc;
        unsigned way = i % assoc;

        blk->set = set;
        blk->way = way;

        cache->sets[set].ways[way] = blk;
    }

    // Initialize Signature Hit Predictor
    cache->shp_table = initSignatureHitPredictor(SHP_TABLE_SIZE);

    return cache;
}

bool accessBlock(Cache *cache, Request *req, uint64_t access_time)
{
    bool hit = false;

    uint64_t blk_aligned_addr = blkAlign(req->load_or_store_addr, cache->blk_mask);

    Cache_Block *blk = findBlock(cache, blk_aligned_addr);

    if (blk != NULL)
    {
        hit = true;

        // Update access time
        blk->when_touched = access_time;
        // Increment frequency counter
        ++blk->frequency;

        blk->rrpv = 0;

        // Set outcome bit to true
        blk->outcome = true;

        if (req->req_type == STORE)
        {
            blk->dirty = true;
        }
    } 
    else
    {
        // Cache miss, need to insert the block
        uint64_t wb_addr;
        bool wb_required = insertBlock(cache, req, access_time, &wb_addr);

        // Handle write-back if necessary
        if (wb_required)
        {
            // Write back dirty block to lower level (implement this function)
            cache->write_back_count++;
        }
    }

    return hit;
}

bool insertBlock(Cache *cache, Request *req, uint64_t access_time, uint64_t *wb_addr)
{
    // Step one: find a victim block
    uint64_t blk_aligned_addr = blkAlign(req->load_or_store_addr, cache->blk_mask);

    Cache_Block *victim = NULL;
#ifdef LRU
    bool wb_required = lru(cache, blk_aligned_addr, &victim, wb_addr);
#endif

#ifdef LFU
    bool wb_required = lfu(cache, blk_aligned_addr, &victim, wb_addr);
#endif

#ifdef SHP_REPLACEMENT
    bool wb_required = signature_hit_predictor(cache, blk_aligned_addr, &victim, wb_addr, req->PC);
#endif

    assert(victim != NULL);

    // Step two: insert the new block
    uint64_t tag = req->load_or_store_addr >> cache->tag_shift;
    victim->tag = tag;
    victim->valid = true;

    victim->when_touched = access_time;
    ++victim->frequency;

    if (req->req_type == STORE)
    {
        victim->dirty = true;
    }

    return wb_required;
    // printf("Inserted: %" PRIu64 "\n", req->load_or_store_addr);
}

// Helper Functions
inline uint64_t blkAlign(uint64_t addr, uint64_t mask)
{
    return addr & ~mask;
}

Cache_Block *findBlock(Cache *cache, uint64_t addr)
{
    // printf("Addr: %" PRIu64 "\n", addr);

    // Extract tag
    uint64_t tag = addr >> cache->tag_shift;
    // printf("Tag: %" PRIu64 "\n", tag);

    // Extract set index
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    // printf("Set: %" PRIu64 "\n", set_idx);

    Cache_Block **ways = cache->sets[set_idx].ways;
    for (unsigned i = 0; i < cache->num_ways; i++)
    {
        if (tag == ways[i]->tag && ways[i]->valid == true)
        {
            return ways[i];
        }
    }

    return NULL;
}

bool lru(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr)
{
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    // printf("Set: %" PRIu64 "\n", set_idx);
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one: try to find an invalid block.
    for (unsigned i = 0; i < cache->num_ways; i++)
    {
        if (ways[i]->valid == false)
        {
            *victim_blk = ways[i];
            return false; // No need to write-back
        }
    }

    // Step two: locate the LRU block
    Cache_Block *victim = ways[0];
    for (unsigned i = 1; i < cache->num_ways; i++)
    {
        if (ways[i]->when_touched < victim->when_touched)
        {
            victim = ways[i];
        }
    }

    // Step three: need to write-back the victim block if dirty
    bool wb_required = victim->dirty;

    if (wb_required)
    {
        *wb_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
    }

    // Invalidate victim
    victim->tag = UINT64_MAX;
    victim->valid = false;
    victim->dirty = false;
    victim->frequency = 0;
    victim->when_touched = 0;

    *victim_blk = victim;

    return wb_required; // Need to write-back if dirty
}

bool lfu(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr)
{
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    // printf("Set: %" PRIu64 "\n", set_idx);
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one: try to find an invalid block.
    for (unsigned i = 0; i < cache->num_ways; i++)
    {
        if (ways[i]->valid == false)
        {
            *victim_blk = ways[i];
            return false; // No need to write-back
        }
    }

    // Step two: locate the LFU block with the oldest timestamp in case of frequency tie
    Cache_Block *victim = ways[0];
    for (unsigned i = 1; i < cache->num_ways; i++)
    {
        if (ways[i]->frequency < victim->frequency)
        {
            victim = ways[i];
        }
        else if (ways[i]->frequency == victim->frequency)
        {
            if (ways[i]->when_touched < victim->when_touched)
            {
                victim = ways[i];
            }
        }
    }

    // Step three: need to write-back the victim block if dirty
    bool wb_required = victim->dirty;

    if (wb_required)
    {
        *wb_addr = (victim->tag << cache->tag_shift) | (victim->set << cache->set_shift);
    }

    // Invalidate victim
    victim->tag = UINT64_MAX;
    victim->valid = false;
    victim->dirty = false;
    victim->frequency = 0;
    victim->when_touched = 0;

    *victim_blk = victim;

    return wb_required; // Need to write-back if dirty
}

/* Signature Hit Predictor Implementation */


/* Predictor Functions */

// Hash function to generate index for the SHP table
uint32_t hashSignature(uint64_t signature, unsigned table_size)
{
    return (signature ^ (signature >> 5)) % table_size;
}

// Initialize the Signature Hit Predictor
SignatureHitPredictor *initSignatureHitPredictor(unsigned table_size)
{
    SignatureHitPredictor *shp = (SignatureHitPredictor *)malloc(sizeof(SignatureHitPredictor));
    shp->table_size = table_size;
    shp->entries = (SHP_Entry *)malloc(table_size * sizeof(SHP_Entry));

    for (unsigned i = 0; i < table_size; i++)
    {
        shp->entries[i].counter = 1;
    }
    return shp;
}

// Predict whether the next access will be a hit or miss
bool predictHit(SignatureHitPredictor *shp, uint64_t signature)
{
    uint32_t index = hashSignature(signature, shp->table_size);
    SHP_Entry *entry = &shp->entries[index];
    return entry->counter > 0;
}

// Update the Signature Hit Predictor based on the access result
void updateSignatureHitPredictor(SignatureHitPredictor *shp, uint64_t signature, bool hit)
{
    uint32_t index = hashSignature(signature, shp->table_size);
    SHP_Entry *entry = &shp->entries[index];

    if (hit)
    {
        if (entry->counter < MAX_PREDICTOR_COUNTER){
            entry->counter++;
        }
    }
    else
    {
        if (entry->counter > 0){
            entry->counter--;
        }
    }
}

// Signature Hit Predictor Replacement Policy
bool signature_hit_predictor(Cache *cache, uint64_t addr, Cache_Block **victim_blk, uint64_t *wb_addr, uint64_t signature)
{
    SignatureHitPredictor *shp = cache->shp_table;
    uint8_t max_rrpv = cache->max_rrpv; // e.g., 3 for 2-bit RRPV

    // Get predictor counter for the signature
    uint32_t index = hashSignature(signature, shp->table_size);
    uint8_t predictor_counter = shp->entries[index].counter;

    // Determine initial RRPV based on predictor
    uint8_t initial_rrpv;
    if (predictor_counter == 0)
    {
        // Not likely to be reused soon
        initial_rrpv = max_rrpv - 1; // e.g., 2
    }
    else
    {
        // Likely to be reused soon
        initial_rrpv = 0;
    }

    // Determine set index
    uint64_t set_idx = (addr >> cache->set_shift) & cache->set_mask;
    Cache_Block **ways = cache->sets[set_idx].ways;

    // Step one: try to find an invalid block.
    for (unsigned i = 0; i < cache->num_ways; i++)
    {
        if (!ways[i]->valid)
        {
            // Use this block
            Cache_Block *blk = ways[i];
            blk->rrpv = initial_rrpv;
            blk->signature = signature;
            blk->outcome = false; // Initialize outcome to false
            blk->valid = true;
            blk->tag = addr >> cache->tag_shift;
            blk->dirty = false; // Initialize dirty bit
            blk->when_touched = 0;
            blk->frequency = 0;

            *victim_blk = blk;
            return false; // No need to write-back
        }
    }

    // Step two: find a victim with RRPV == max_rrpv
    while (true)
    {
        for (unsigned i = 0; i < cache->num_ways; i++)
        {
            if (ways[i]->rrpv == max_rrpv)
            {
                // Victim found
                Cache_Block *victim = ways[i];

                // Update predictor based on outcome
                updateSignatureHitPredictor(shp, victim->signature, victim->outcome);

                // Prepare for write-back if necessary
                bool wb_required = victim->dirty;
                if (wb_required)
                {
                    *wb_addr = (victim->tag << cache->tag_shift) | (set_idx << cache->set_shift);
                }

                // Replace victim block with new block
                victim->rrpv = initial_rrpv;
                victim->signature = signature;
                victim->outcome = false; // Initialize outcome to false
                victim->valid = true;
                victim->tag = addr >> cache->tag_shift;
                victim->dirty = false; // Initialize dirty bit
                victim->when_touched = 0;
                victim->frequency = 0;

                *victim_blk = victim;
                return wb_required;
            }
        }

        // Increment RRPV of all blocks
        for (unsigned i = 0; i < cache->num_ways; i++)
        {
            if (ways[i]->rrpv < max_rrpv)
                ways[i]->rrpv++;
        }
    }
}

