#ifndef __INSTRUCTION_HH__
#define __INSTRUCTION_HH__

#define __STDC_FORMAT_MACROS
#include <inttypes.h> // uint64_t

typedef enum Instruction_Type{EXE, BRANCH, LOAD, STORE}Instruction_Type;

// Instruction Format
typedef struct Instruction
{
    uint64_t PC; // Program Counter of the instruction

    Instruction_Type instr_type; // Instruction type

    uint64_t load_or_store_addr; // load or store address (for LOAD and STORE instructions)

    int size; // Size of data to be loaded/stored

    int taken; // If the instruction is a branch, what is the real direction (not the predicted).
                // You should reply on this field to determine the correctness of your predictions.
}Instruction;

#endif
