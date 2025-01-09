#ifndef __BANK_HH__
#define __BANK_HH__

#define __STDC_FORMAT_MACROS
#include <inttypes.h> // uint64_t

#include <stdbool.h>

typedef struct Bank
{
    uint64_t cur_clk; // current memory clock
    uint64_t next_free; // the future memory clock that the bank is free
}Bank;

void initBank(Bank *bank)
{
    bank->cur_clk = 0;
    bank->next_free = 0;
}

#endif
