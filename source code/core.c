#include <stdlib.h>
#include <string.h>
#include "core.h"
#include "pipeline_registers.h"

core_t *init_core(instruction_memory_t *i_mem)
{
    printf("Initializing core...\n");
    core_t *core = (core_t *)malloc(sizeof(core_t));
    if (core == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for core.\n");
        exit(EXIT_FAILURE);
    }

    core->clk = 0;
    core->PC = 0;
    core->instr_mem = i_mem;
    core->tick = tick_func;
    core->halt = false;

    memset(core->data_mem, 0, MEM_SIZE);
    memset(core->reg_file, 0, NUM_REGISTERS * sizeof(register_t));
    
    core->if_id = (IF_ID_t){0};
    core->id_ex = (ID_EX_t){0};
    core->ex_mem = (EX_MEM_t){0};
    core->mem_wb = (MEM_WB_t){0};

    printf("Core initialized. Starting PC: %lu, Clock cycle: %lu\n", core->PC, core->clk);
    return core;
}

bool tick_func(core_t *core)
{
    printf("\nClock Cycle: %lu\n", core->clk);

    // Backup current pipeline register states
    IF_ID_t next_if_id = core->if_id;
    ID_EX_t next_id_ex = core->id_ex;
    EX_MEM_t next_ex_mem = core->ex_mem;
    MEM_WB_t next_mem_wb = core->mem_wb;

    // 1. Fetch the next instruction and update IF/ID pipeline register
    instruction_fetch(core, &next_if_id);
    if (core->halt) return false;  // Halt if PC is out of bounds or misaligned

    // 2. Detect hazards using the updated IF/ID and existing ID/EX stages
    hazard_detection_unit(core, &next_if_id, &core->id_ex);

    // 3. Check for stall before proceeding with instruction decode
    if (core->stall) {
        printf("Debug: Pipeline stalled due to load-use hazard.\n");
        // Clear control signals in ID/EX to prevent register updates
        next_if_id = core->if_id;
        core->id_ex.control = (control_signals_t){0}; 
        ++core->clk; // Increment the clock even during a stall
        return true; // Return early due to stall; no stage updates
    }

    // 4. Decode the fetched instruction if no stall is detected
    instruction_decode(core, &next_if_id, &next_id_ex);
    if (core->halt) return false;

    // 5. Perform data forwarding only if no stall is detected
    detect_and_forward(core);

    // 6. Execute the ALU operation based on the decoded instruction
    execute(core, &next_id_ex, &next_ex_mem);
    if (core->halt) return false;

    // 7. Access memory if required (load/store instructions)
    memory_access(core, &next_ex_mem, &next_mem_wb);
    if (core->halt) return false;

    // 8. Write the result back to the register file
    write_back(core, &next_mem_wb);
    if (core->halt) return false;

    // Update all pipeline registers after each stage has completed for this cycle
    core->if_id = next_if_id;
    core->id_ex = next_id_ex;
    core->ex_mem = next_ex_mem;
    core->mem_wb = next_mem_wb;

    // Increment the clock cycle at the end of all stage updates
    ++core->clk;

    // Only clear the stall flag if the hazard is resolved
    core->stall = 0;

    // Check for the end of the program
    if (core->PC >= core->instr_mem->last->addr + 4) {
        printf("End of program. Final PC: %lu\n", core->PC);
        return false;
    }
    return true;
}

void hazard_detection_unit(core_t *core, IF_ID_t *if_id, ID_EX_t *id_ex) {
    // Initialize stall signal to 0
    core->stall = 0;

    // Check if the previous instruction is a load (MemRead control signal is set)
    if (id_ex->control.MemRead) {
        // Extract destination register from ID/EX (load instruction)
        signal_t rd = id_ex->rd;  // Destination register of the load instruction

        // Extract source registers from IF/ID (current instruction in decode stage)
        signal_t rs1 = (if_id->instruction >> 15) & 0x1F;  // Source register 1
        signal_t rs2 = (if_id->instruction >> 20) & 0x1F;  // Source register 2


        // Check if the destination register of the load matches any source registers of the next instruction
        if (rd != 0 && (rd == rs1 || rd == rs2)) {
            // Load-use hazard detected; set the stall signal
            core->stall = 1;
        }
    } else {
        // Debug: No load-use hazard detected
        printf("Debug: No load-use hazard detected. Continuing without stall.\n");
    }
}


void control_unit(signal_t input, control_signals_t *signals)
{
    if (input == 51) {
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 2;
    } else if (input == 3) {
        signals->ALUSrc = 1;
        signals->MemtoReg = 1;
        signals->RegWrite = 1;
        signals->MemRead = 1;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 35) {
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 1;
        signals->Branch = 0;
        signals->ALUOp = 0;
    } else if (input == 99) {
        signals->ALUSrc = 0;
        signals->MemtoReg = 0;
        signals->RegWrite = 0;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 1;
        signals->ALUOp = 1;
    } else if (input == 19) {
        signals->ALUSrc = 1;
        signals->MemtoReg = 0;
        signals->RegWrite = 1;
        signals->MemRead = 0;
        signals->MemWrite = 0;
        signals->Branch = 0;
        signals->ALUOp = 0;
    }
}

signal_t ALU_control_unit(signal_t ALUOp, signal_t Funct7, signal_t Funct3)
{
    switch (ALUOp) {
        case 0:
            if (Funct3 == 1) {
                return 3;
            }
            return 2;

        case 1:
            return 6;

        case 2:
            if (Funct7 == 0 && Funct3 == 0) {
                return 2;
            } else if (Funct7 == 32 && Funct3 == 0) {
                return 6;
            } else if (Funct7 == 0 && Funct3 == 7) {
                return 0;
            } else if (Funct7 == 0 && Funct3 == 6) {
                return 1;
            } else if (Funct7 == 0 && Funct3 == 1) {
                return 3;
            }
            break;

        default:
            return 0;
    }
}

signal_t imm_gen(signal_t input) {
    signal_t imm;
    signal_t opcode = input & 0x7F;

    if (opcode == 0x13 || opcode == 0x03 || opcode == 0x73) {
        imm = (input >> 20) & 0xFFF;
        if (imm & 0x800) {
            imm |= 0xFFFFFFFFFFFFF000ULL;
        }
    } else if (opcode == 0x23) {
        imm = ((input >> 25) & 0x7F) << 5;
        imm |= (input >> 7) & 0x1F;
        if (imm & 0x800) {
            imm |= 0xFFFFFFFFFFFFF000ULL;
        }
    } else if (opcode == 0x63) {
        imm = ((input >> 31) & 0x1) << 12;
        imm |= ((input >> 7) & 0x1) << 11;
        imm |= ((input >> 25) & 0x3F) << 5;
        imm |= ((input >> 8) & 0xF) << 1;
        if (imm & 0x1000) {
            imm |= 0xFFFFFFFFFFFFE000ULL;
        }
    } else if (opcode == 0x37 || opcode == 0x17) {
        imm = input & 0xFFFFF000ULL;
    } else if (opcode == 0x6F) {
        imm = ((input >> 31) & 0x1) << 20;
        imm |= ((input >> 12) & 0xFF) << 12;
        imm |= ((input >> 20) & 0x1) << 11;
        imm |= ((input >> 21) & 0x3FF) << 1;
        if (imm & 0x100000) {
            imm |= 0xFFFFFFFFFFE00000ULL;
        }
    }
    printf("Debug: Immediate value for branch: 0x%08lX\n", imm);
    return imm;
}

void ALU(signal_t input_0, signal_t input_1, signal_t ALU_ctrl_signal, signal_t *ALU_result, signal_t *zero)
{
    switch (ALU_ctrl_signal) {
        case 2:
            *ALU_result = input_0 + input_1;
            break;
        case 6:
            *ALU_result = input_0 - input_1;
            break;
        case 0:
            *ALU_result = input_0 & input_1;
            break;
        case 1:
            *ALU_result = input_0 | input_1;
            break;
        case 3:
            *ALU_result = input_0 << (input_1 & 0x1F);
            break;
        case 7:
            *ALU_result = (input_0 < input_1) ? 1 : 0;
            break;
        default:
            *ALU_result = 0;
            break;
    }

    *zero = (*ALU_result == 0) ? 1 : 0;
}

signal_t MUX(signal_t sel, signal_t input_0, signal_t input_1)
{
    return (sel == 0) ? input_0 : input_1;
}

void print_core_state(core_t *core)
{
    printf("Register file\n");
    for (int i = 0; i < NUM_REGISTERS; i++)
        printf("x%d \t: %ld\n", i, core->reg_file[i]);
}

void print_data_memory(core_t *core, unsigned int start, unsigned int end)
{
    if (start >= MEM_SIZE || end > MEM_SIZE) {
        printf("Address range [%d, %d) is invalid\n", start, end);
        return;
    }

    printf("Data memory: bytes (in hex) within address range [%d, %d)\n", start, end);
    for (unsigned int i = start; i < end; i++)
        printf("%d: \t %02x\n", i, core->data_mem[i]);
}

void instruction_fetch(core_t *core, IF_ID_t *if_id) {
    // Calculate the number of instructions loaded
    int num_instructions = core->instr_mem->last - core->instr_mem->instructions + 1;

    // Debug statement: Print the range of instruction memory
    printf("Debug: Instruction memory range: 0x%08lX to 0x%08lX\n",
           (unsigned long) core->instr_mem->instructions,
           (unsigned long) core->instr_mem->last);

    // Check for stall signal
    if (core->stall) {
        // Debug statement: Stalling instruction fetch
        printf("Debug: Stall detected. Halting instruction fetch.\n");
        // Do not increment PC or fetch a new instruction, keep current state
        return;
    }

    // Ensure PC is within the bounds of the loaded instructions
    if (core->PC / 4 >= num_instructions) {
        fprintf(stderr, "Error: PC out of bounds at PC: 0x%08lX. Halting simulation.\n", core->PC);
        core->halt = true;
        return;
    }

    // Ensure the PC is correctly aligned to 4-byte boundaries
    if (core->PC % 4 != 0) {
        fprintf(stderr, "Error: Misaligned PC at PC: 0x%08lX. Halting simulation.\n", core->PC);
        core->halt = true;
        return;
    }

    // Fetch the instruction at the current PC
    uint32_t instruction_index = core->PC / 4;  // Index in instruction memory

    // Debug statement: Print the instruction index
    printf("Debug: Instruction index: %u\n", instruction_index);

    // Check if instruction_index is valid
    if (instruction_index >= num_instructions) {
        fprintf(stderr, "Error: Instruction index out of bounds at index: %u. Halting simulation.\n", instruction_index);
        core->halt = true;
        return;
    }

    instruction_t *instruction = &core->instr_mem->instructions[instruction_index];

    // Load the instruction into the IF/ID pipeline register
    if_id->instruction = instruction->instruction;
    if_id->PC = core->PC;

    // Debug statement: Print the fetched instruction
    printf("Debug (IF): PC: 0x%08lX, Fetched instruction: 0x%08X\n", core->PC, if_id->instruction);

    // Increment PC to the next instruction
    core->PC += 4;
}


void instruction_decode(core_t *core, IF_ID_t *if_id, ID_EX_t *id_ex) {
    uint32_t instruction = if_id->instruction;

    // Decode the instruction
    id_ex->instruction = instruction;
    id_ex->PC = if_id->PC;

    // Extract source registers, destination register, and immediate
    id_ex->rs1 = core->reg_file[(instruction >> 15) & 0x1F];  // rs1
    id_ex->rs2 = core->reg_file[(instruction >> 20) & 0x1F];  // rs2
    id_ex->rd = (instruction >> 7) & 0x1F;                    // rd
    id_ex->imm = imm_gen(instruction);                        // Generate immediate

    // Extract funct3 and funct7 for R-type and other relevant instructions
    id_ex->funct3 = (instruction >> 12) & 0x7;   // funct3
    id_ex->funct7 = (instruction >> 25) & 0x7F;  // funct7

    // Generate control signals
    signal_t opcode = instruction & 0x7F; // Extract opcode
    control_unit(opcode, &id_ex->control);

    printf("Debug (ID): Decoding instruction at PC: 0x%08lX\n", if_id->PC);
    printf("    rs1: x%d = 0x%08X, rs2: x%d = 0x%08X, rd: x%d\n",
           (instruction >> 15) & 0x1F, id_ex->rs1,
           (instruction >> 20) & 0x1F, id_ex->rs2,
           id_ex->rd);
}
void detect_and_forward(core_t *core) {
    // Initialize forwarding signals
    core->forwardA = NO_FORWARDING;
    core->forwardB = NO_FORWARDING;
    if (core->stall) {
        printf("Debug: Stall detected, skipping forwarding decisions.\n");
        return; // Exit early since stall should take priority over forwarding
    }

    // Forwarding from EX/MEM stage (newest data)
    if (core->ex_mem.control.RegWrite && core->ex_mem.rd != 0) {
        if (core->ex_mem.rd == ((core->id_ex.instruction >> 15) & 0x1F)) {
            core->forwardA = FORWARD_FROM_EX_MEM;
            printf("Debug: Forwarding operand1 from EX/MEM - ex_mem.rd: %d, id_ex.rs1: %d\n", core->ex_mem.rd, (core->id_ex.instruction >> 15) & 0x1F);
        }
        if (core->ex_mem.rd == ((core->id_ex.instruction >> 20) & 0x1F)) {
            core->forwardB = FORWARD_FROM_EX_MEM;
            printf("Debug: Forwarding operand2 from EX/MEM - ex_mem.rd: %d, id_ex.rs2: %d\n", core->ex_mem.rd, (core->id_ex.instruction >> 20) & 0x1F);
        }
    }

    // Forwarding from MEM/WB stage (older data)
    if (core->mem_wb.control.RegWrite && core->mem_wb.rd != 0) {
        if (core->forwardA == NO_FORWARDING && core->mem_wb.rd == ((core->id_ex.instruction >> 15) & 0x1F)) {
            core->forwardA = FORWARD_FROM_MEM_WB;
            printf("Debug: Forwarding operand1 from MEM/WB - mem_wb.rd: %d, id_ex.rs1: %d\n", core->mem_wb.rd, (core->id_ex.instruction >> 15) & 0x1F);
        }
        if (core->forwardB == NO_FORWARDING && core->mem_wb.rd == ((core->id_ex.instruction >> 20) & 0x1F)) {
            core->forwardB = FORWARD_FROM_MEM_WB;
            printf("Debug: Forwarding operand2 from MEM/WB - mem_wb.rd: %d, id_ex.rs2: %d\n", core->mem_wb.rd, (core->id_ex.instruction >> 20) & 0x1F);
        }
    }
}


void execute(core_t *core, ID_EX_t *id_ex, EX_MEM_t *ex_mem) {
    signal_t alu_result;
    signal_t zero;
    if (id_ex->control.RegWrite == 0 && id_ex->control.ALUSrc == 0 &&
        id_ex->control.MemRead == 0 && id_ex->control.MemWrite == 0 &&
        id_ex->control.MemtoReg == 0 && id_ex->control.Branch == 0) {
        printf("Debug: Execution stage stalled due to control signals being zero.\n");
        return; // Skip execution due to stall
    }

    // Determine the source of operand1 based on forwarding
    signal_t operand1;
    if (core->forwardA == FORWARD_FROM_EX_MEM) {
        operand1 = core->ex_mem.alu_result; // Forward from EX/MEM stage
        printf("Debug: Forwarding operand1 from EX/MEM - Value: 0x%08lX\n", operand1);
    } else if (core->forwardA == FORWARD_FROM_MEM_WB) {
        operand1 = core->mem_wb.alu_result; // Forward from MEM/WB stage
        printf("Debug: Forwarding operand1 from MEM/WB - Value: 0x%08lX\n", operand1);
    } else {
        operand1 = id_ex->rs1; // Use value from the ID/EX stage
        printf("Debug: Using operand1 from ID/EX - Value: 0x%08lX\n", operand1);
    }

    // Determine the source of operand2 based on forwarding logic and ALUSrc control signal
    signal_t operand2;
    if (core->forwardB == FORWARD_FROM_EX_MEM) {
        operand2 = core->ex_mem.alu_result; // Forward from EX/MEM stage
        printf("Debug: Forwarding operand2 from EX/MEM - Value: 0x%08lX\n", operand2);
    } else if (core->forwardB == FORWARD_FROM_MEM_WB) {
        operand2 = core->mem_wb.alu_result; // Forward from MEM/WB stage
        printf("Debug: Forwarding operand2 from MEM/WB - Value: 0x%08lX\n", operand2);
    } else if (id_ex->control.ALUSrc) {
        operand2 = id_ex->imm; // Use the immediate value if ALUSrc is set
        printf("Debug: Using immediate value for operand2 - Value: 0x%08lX\n", operand2);
    } else {
        operand2 = id_ex->rs2; // Use value from the ID/EX stage
        printf("Debug: Using operand2 from ID/EX - Value: 0x%08lX\n", operand2);
    }

    // Generate ALU control signal using funct7 and funct3
    signal_t alu_ctrl = ALU_control_unit(id_ex->control.ALUOp, id_ex->funct7, id_ex->funct3);

    // Execute ALU operation
    ALU(operand1, operand2, alu_ctrl, &alu_result, &zero);

    // Update EX/MEM pipeline register with ALU results and control signals
    ex_mem->alu_result = alu_result;
    ex_mem->zero = zero; // Store the zero flag in EX/MEM for use in branch decisions
    ex_mem->rs2 = id_ex->rs2;
    ex_mem->rd = id_ex->rd;
    ex_mem->control = id_ex->control;
}




void memory_access(core_t *core, EX_MEM_t *ex_mem, MEM_WB_t *mem_wb) {
    if (ex_mem->control.RegWrite == 0 && ex_mem->control.MemRead == 0 &&
        ex_mem->control.MemWrite == 0 && ex_mem->control.MemtoReg == 0) {
        printf("Debug: Memory access stage stalled due to control signals being zero.\n");
        return; // Skip memory access due to stall
    }
    if (ex_mem->control.MemRead) {
        // Load data from memory
        mem_wb->data = *(uint64_t *)&core->data_mem[ex_mem->alu_result];
        printf("Debug: Loaded data 0x%08X from address 0x%08X\n", mem_wb->data, ex_mem->alu_result);
    } else if (ex_mem->control.MemWrite) {
        // Store data to memory
        *(uint64_t *)&core->data_mem[ex_mem->alu_result] = ex_mem->rs2;
        printf("Debug: Stored data 0x%08X to address 0x%08X\n", ex_mem->rs2, ex_mem->alu_result);
    }

    // Pass ALU result forward if not a load/store
    mem_wb->alu_result = ex_mem->alu_result;
    mem_wb->rd = ex_mem->rd;
    mem_wb->control = ex_mem->control; // Propagate control signals
}

void write_back(core_t *core, MEM_WB_t *mem_wb) {
    if (mem_wb->control.RegWrite) {
        uint32_t write_data = mem_wb->control.MemtoReg ? mem_wb->data : mem_wb->alu_result;
        if (mem_wb->rd != 0) {
            core->reg_file[mem_wb->rd] = write_data;
            printf("Debug: Write Back Stage - Register x%d updated with value 0x%08X\n", mem_wb->rd, write_data);
        } else {
            printf("Debug: Write Back Stage - Attempted to write to x0, ignored.\n");
        }
    } else {
        printf("Debug: Write Back Stage - No register update due to RegWrite being disabled.\n");
    }
}
