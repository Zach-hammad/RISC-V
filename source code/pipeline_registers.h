#ifndef __PIPELINE_REGISTERS_H__
#define __PIPELINE_REGISTERS_H__

#include "core.h"  // Ensure core.h is included to define control_signals_t

// IF/ID pipeline register structure
typedef struct {
    uint32_t instruction;  // Fetched instruction
    addr_t PC;             // Current program counter
} IF_ID_t;

// ID/EX pipeline register structure
typedef struct {
    uint32_t instruction;  // Decoded instruction
    addr_t PC;             // Program counter
    uint32_t rs1, rs2;     // Source register values
    uint32_t imm;          // Immediate value
    uint8_t rd; 
    uint8_t funct3;        // funct3 field for ALU operation
    uint8_t funct7;// Destination register
    control_signals_t control;  // Control signals for the EX stage
} ID_EX_t;

// EX/MEM pipeline register structure
typedef struct {
    uint32_t alu_result;   // Result from ALU
    uint32_t rs2;          // Second source register value (for stores)
    uint8_t rd;    // Destination register
    uint8_t zero;
    control_signals_t control;  // Control signals for the MEM stage
} EX_MEM_t;

// MEM/WB pipeline register structure
typedef struct {
    uint32_t data;         // Data from memory or ALU result
    uint8_t rd;            // Destination register
    control_signals_t control;  // Control signals for the WB stage
    uint32_t alu_result;   // Result from ALU
} MEM_WB_t;

#endif // __PIPELINE_REGISTERS_H__
