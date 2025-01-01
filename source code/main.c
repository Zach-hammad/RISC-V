#include <stdio.h>
#include <stdlib.h>
#include "core.h"               // Core definitions, including control signals and pipeline registers
#include "parser.h"             // For parsing and loading instructions

int main(int argc, const char **argv) {
    if (argc != 2) {
        printf("Usage: %s <trace-file>\n", argv[0]);
        exit(EXIT_SUCCESS);
    }

    // Initialize instruction memory and load instructions from trace file
    instruction_memory_t instr_mem;
    instr_mem.last = NULL;
    load_instructions(&instr_mem, argv[1]);

    // Initialize the core
    core_t *core = init_core(&instr_mem);
    if (core == NULL) {
        fprintf(stderr, "Error: Failed to initialize core.\n");
        return EXIT_FAILURE;
    }

    // Main simulation loop
    while (tick_func(core)) {
        // Each call to tick_func simulates a single clock cycle
    }

    printf("Simulation complete.\n");
    print_core_state(core);
    print_data_memory(core, 0, 32);

    // Clean up
    free(core);
    return 0;
}

