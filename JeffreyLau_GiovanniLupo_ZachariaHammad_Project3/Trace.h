#ifndef __TRACE_HH__
#define __TRACE_HH__

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Request.h"

typedef struct TraceParser
{
    FILE *fd; // file descriptor for the trace file

    Request *cur_req; // current instruction
}TraceParser;

// Define functions
TraceParser *initTraceParser(const char * mem_file);
bool getRequest(TraceParser *mem_trace);
uint64_t convToUint64(char *ptr);
void printMemRequest(Request *req);

#endif
