#ifndef __INSTRUCTION_MEMORY_H__
#define __INSTRUCTION_MEMORY_H__

#include "instruction.h"

#define IMEM_SIZE 256
typedef struct {
    instruction_t instructions[IMEM_SIZE];
    instruction_t *last; // Points to the last instruction
} instruction_memory_t;

#endif
