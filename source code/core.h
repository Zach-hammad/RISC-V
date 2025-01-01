#ifndef __CORE_H__
#define __CORE_H__

#include "instruction_memory.h"  // Include necessary dependencies
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define BOOL bool
#define MEM_SIZE 1024       // Size of memory in bytes 
#define NUM_REGISTERS 32    // Size of register file 
#define NO_FORWARDING         0
#define FORWARD_FROM_EX_MEM   1
#define FORWARD_FROM_MEM_WB   2

typedef uint8_t byte_t;
typedef int64_t signal_t;
typedef int64_t register_t;

// Definition of the various control signals
typedef struct control_signals_s {
    signal_t Branch;
    signal_t MemRead;
    signal_t MemtoReg;
    signal_t ALUOp;
    signal_t MemWrite;
    signal_t ALUSrc;
    signal_t RegWrite;
} control_signals_t;  // Ensure control_signals_t is defined here

// Include pipeline_registers.h after defining control_signals_t
#include "pipeline_registers.h"

// Definition of the RISC-V core
struct core_s {
    tick_t clk;                         // Core clock
    addr_t PC;                          // Program counter
    instruction_memory_t *instr_mem;    // Instruction memory 
    byte_t data_mem[MEM_SIZE];          // Data memory
    register_t reg_file[NUM_REGISTERS]; // Register file.
    bool halt;
    bool inserted_NOP;
    bool flush_pipeline;
    int stall;

    bool (*tick)(struct core_s *core);  // Simulate function 

    // Pipeline registers
    IF_ID_t if_id;       
    ID_EX_t id_ex;       
    EX_MEM_t ex_mem;     
    MEM_WB_t mem_wb;    
    uint8_t forwardA;
    uint8_t forwardB;

    // Core control signals
    control_signals_t control; // Control signals for current instruction
};
typedef struct core_s core_t;

// Function prototypes for pipeline stages
void instruction_fetch(core_t *core, IF_ID_t *if_id);
void instruction_decode(core_t *core, IF_ID_t *if_id, ID_EX_t *id_ex);
void execute(core_t *core, ID_EX_t *id_ex, EX_MEM_t *ex_mem);
void memory_access(core_t *core, EX_MEM_t *ex_mem, MEM_WB_t *mem_wb);
void write_back(core_t *core, MEM_WB_t *mem_wb);

// Core initialization and utility functions
core_t *init_core(instruction_memory_t *i_mem);
bool tick_func(core_t *core);
void print_core_state(core_t *core);
void print_data_memory(core_t *core, unsigned int start, unsigned int end);
void control_unit(signal_t input, control_signals_t *signals);
signal_t ALU_control_unit(signal_t ALUOp, signal_t funct7, signal_t funct3);
signal_t imm_gen(signal_t input);
void ALU(signal_t input_0, signal_t input_1, signal_t ALU_ctrl_signal, signal_t *ALU_result, signal_t *zero);
signal_t MUX(signal_t sel, signal_t input_0, signal_t input_1);
signal_t Add(signal_t input_0, signal_t input_1);
signal_t ShiftLeft1(signal_t input);
void detect_and_forward(core_t *core);
void hazard_detection_unit(core_t *core, IF_ID_t *if_id, ID_EX_t *id_ex);
#endif // __CORE_H__
