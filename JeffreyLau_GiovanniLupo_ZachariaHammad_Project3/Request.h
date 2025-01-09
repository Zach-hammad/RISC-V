#ifndef __REQUEST_HH__
#define __REQUEST_HH__

#define __STDC_FORMAT_MACROS
#include <inttypes.h> // uint64_t

typedef enum Request_Type{LOAD, STORE}Request_Type;

// Instruction Format
typedef struct Request
{
    Request_Type req_type; // Request Type

    uint64_t load_or_store_addr; // Load or Store Address

    // Advanced Features
    uint64_t PC; // The instruction that brings the cache block

    int core_id; // The core of PC is running on

}Request;

#endif
