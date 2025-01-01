#ifndef __INSTRUCTION_H__
#define __INSTRUCTION_H__

#include <stdint.h>

typedef uint64_t addr_t;
typedef uint64_t tick_t;

typedef struct
{
    // Byte-addressable address
    addr_t addr;
    // This is the translated binary format of assembly input
    unsigned int instruction;

} instruction_t;

#endif
