#ifndef __TRACE_HH__
#define __TRACE_HH__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Instruction.h"

typedef struct TraceParser
{
    FILE *fd; // file descriptor for the trace file

    Instruction *cur_instr; // current instruction
}TraceParser;

// Define functions
TraceParser *initTraceParser(const char * trace_file);
bool getInstruction(TraceParser *cpu_trace);
uint64_t convToUint64(char *ptr);
void printInstruction(Instruction *instr);

#endif
