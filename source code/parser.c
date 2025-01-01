#include <stdio.h>
#include <stdlib.h>
#include "parser.h"            
#include "instruction_memory.h" 

void load_instructions(instruction_memory_t *i_mem, const char *trace) {
    printf("Loading trace file: %s\n\n", trace);
    FILE *fd = fopen(trace, "r");
    if (fd == NULL) {
        perror("Cannot open trace file.\n");
        exit(EXIT_FAILURE);
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    addr_t PC = 0;
    int IMEM_index = 0;

    while ((read = getline(&line, &len, fd)) != -1) {
        if (IMEM_index >= IMEM_SIZE) {
            fprintf(stderr, "Error: Instruction memory overflow at index %d\n", IMEM_index);
            break;  // Prevent overflow if the instruction memory is full
        }

        // Set address for the current instruction
        i_mem->instructions[IMEM_index].addr = PC;

        // Get the raw instruction and parse
        char *raw_instr = strtok(line, " \t\n");  // Trim spaces, tabs, and newlines
        if (raw_instr == NULL) {
            fprintf(stderr, "Warning: Skipping empty or malformed line.\n");
            continue;
        }

        // Select parsing function based on instruction type
        if (strcmp(raw_instr, "add") == 0 ||
            strcmp(raw_instr, "sub") == 0 ||
            strcmp(raw_instr, "sll") == 0 ||
            strcmp(raw_instr, "srl") == 0 ||
            strcmp(raw_instr, "xor") == 0 ||
            strcmp(raw_instr, "or")  == 0 ||
            strcmp(raw_instr, "and") == 0 ||
            strcmp(raw_instr, "sra") == 0) {
            parse_R_type(raw_instr, &(i_mem->instructions[IMEM_index]));
        } else if (strcmp(raw_instr, "addi") == 0 || 
                   strcmp(raw_instr, "xori") == 0 ||
                   strcmp(raw_instr, "ori")  == 0 ||
                   strcmp(raw_instr, "andi") == 0 ||
                   strcmp(raw_instr, "slli") == 0 ||
                   strcmp(raw_instr, "srli") == 0 ||
                   strcmp(raw_instr, "lb") == 0 ||
                   strcmp(raw_instr, "lh") == 0 ||
                   strcmp(raw_instr, "lw") == 0 ||
                   strcmp(raw_instr, "ld") == 0 ||
                   strcmp(raw_instr, "lbu") == 0 ||
                   strcmp(raw_instr, "lhu") == 0 ||
                   strcmp(raw_instr, "lwu") == 0 ||
                   strcmp(raw_instr, "srai") == 0 ||
                   strcmp(raw_instr, "jalr") == 0) {
            parse_I_type(raw_instr, &(i_mem->instructions[IMEM_index]));
        } else if (strcmp(raw_instr, "beq") == 0 ||
                   strcmp(raw_instr, "bne") == 0 ||
                   strcmp(raw_instr, "blt") == 0 ||
                   strcmp(raw_instr, "bge") == 0 ||
                   strcmp(raw_instr, "bltu") == 0 ||
                   strcmp(raw_instr, "bgeu") == 0) {
            parse_SB_type(raw_instr, &(i_mem->instructions[IMEM_index]));
        } else if (strcmp(raw_instr, "sb") == 0 ||
                   strcmp(raw_instr, "sh") == 0 ||
                   strcmp(raw_instr, "sw") == 0 ||
                   strcmp(raw_instr, "sd") == 0) {
            parse_S_type(raw_instr, &(i_mem->instructions[IMEM_index]));
        } else {
            printf("Warning: Unknown instruction '%s'. Skipping.\n", raw_instr);
        }

        // Update the last instruction pointer
        i_mem->last = &(i_mem->instructions[IMEM_index]);
        printf("Loaded instruction: 0x%08X at address: 0x%08lX\n", 
            i_mem->instructions[IMEM_index].instruction, 
            i_mem->instructions[IMEM_index].addr);

        // Increment the index and PC
        IMEM_index++;
        PC += 4;  // Increment PC by 4 for each instruction (32-bit alignment)
    }

    free(line);
    fclose(fd);
}

void parse_R_type(char *opr, instruction_t *instr)
{
    instr->instruction = 0;
    unsigned opcode = 51;
    unsigned funct3 = 0;
    unsigned funct7 = 0;

    if (strcmp(opr, "add") == 0) {
        funct3 = 0;
        funct7 = 0;
    } else if (strcmp(opr, "sub") == 0) {
        funct3 = 0;
        funct7 = 32;
    } else if (strcmp(opr, "sll") == 0) {
        funct3 = 1;
        funct7 = 0;
    } else if (strcmp(opr, "srl") == 0) {
        funct3 = 5;
        funct7 = 0;
    } else if (strcmp(opr, "xor") == 0) {
        funct3 = 4;
        funct7 = 0;
    } else if (strcmp(opr, "or") == 0) {
        funct3 = 6;
        funct7 = 0;
    } else if (strcmp(opr, "and") == 0) {
        funct3 = 7;
        funct7 = 0;
    } else if (strcmp(opr, "sra") == 0) {
        funct3 = 3;
        funct7 = 0;
    }

    char *reg = strtok(NULL, ", ");
    unsigned rd = reg_index(reg);

    reg = strtok(NULL, ", ");
    unsigned rs_1 = reg_index(reg);

    reg = strtok(NULL, ", ");
    reg[strlen(reg) - 1] = '\0';
    unsigned rs_2 = reg_index(reg);

    instr->instruction |= opcode;
    instr->instruction |= (rd << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (rs_2 << (7 + 5 + 3 + 5));
    instr->instruction |= (funct7 << (7 + 5 + 3 + 5 + 5));
}

void parse_I_type(char *opr, instruction_t *instr)
{
    instr->instruction = 0;
    unsigned opcode = 0;
    unsigned funct3 = 0;

    if (strcmp(opr, "addi") == 0) {
        opcode = 19; 
        funct3 = 0;
    } else if (strcmp(opr, "xori") == 0) {
        opcode = 19;
        funct3 = 4;
    } else if (strcmp(opr, "ori") == 0) {
        opcode = 19;
        funct3 = 6;
    } else if (strcmp(opr, "andi") == 0) {
        opcode = 19;
        funct3 = 7;
    } else if (strcmp(opr, "slli") == 0) {
        opcode = 19;
        funct3 = 1;
    } else if (strcmp(opr, "srli") == 0) {
        opcode = 19;
        funct3 = 5;
    } else if (strcmp(opr, "lb") == 0) {
        opcode = 3;
        funct3 = 0;
    } else if (strcmp(opr, "lh") == 0) {
        opcode = 3;
        funct3 = 1;
    } else if (strcmp(opr, "lw") == 0) {
        opcode = 3;
        funct3 = 2;
    } else if (strcmp(opr, "ld") == 0) {
        opcode = 3;
        funct3 = 3;
    } else if (strcmp(opr, "lbu") == 0) {
        opcode = 3;
        funct3 = 4;
    } else if (strcmp(opr, "lhu") == 0) {
        opcode = 3;
        funct3 = 5;
    } else if (strcmp(opr, "lwu") == 0) {
        opcode = 3;
        funct3 = 6;
    } else if (strcmp(opr, "srai") == 0) {
        opcode = 19;
        funct3 = 5; 
    } else if (strcmp(opr, "jalr") == 0) {
        opcode = 103;
        funct3 = 0;
    }

    unsigned rd, rs_1, immediate;
    if (strcmp(opr, "ld") == 0 ||
        strcmp(opr, "lb") == 0 ||
        strcmp(opr, "lh") == 0 ||
        strcmp(opr, "lw") == 0 ||
        strcmp(opr, "lbu") == 0 ||
        strcmp(opr, "lhu") == 0 ||
        strcmp(opr, "lwu") == 0) {
        char *reg = strtok(NULL, ", ");
        rd = reg_index(reg);
        reg = strtok(NULL, "(");
        immediate = atoi(reg);

        reg = strtok(NULL, ")");
        rs_1 = reg_index(reg);
    } else {
        char *reg = strtok(NULL, ", ");
        rd = reg_index(reg);
        reg = strtok(NULL, ", ");

        rs_1 = reg_index(reg);
        reg = strtok(NULL, ", ");
        immediate = atoi(reg);
    }

    instr->instruction |= opcode;
    instr->instruction |= (rd << 7);
    instr->instruction |= (funct3 << (7 + 5));
    instr->instruction |= (rs_1 << (7 + 5 + 3));
    instr->instruction |= (immediate << (7 + 5 + 3 + 5));
}

void parse_SB_type(char *opr, instruction_t *instr)
{
    instr->instruction = 0;
    unsigned opcode = 99;
    unsigned funct3 = 0;

    if (strcmp(opr, "beq") == 0) {
        funct3 = 0;
    } else if (strcmp(opr, "bne") == 0) {
        funct3 = 1;
    } else if (strcmp(opr, "blt") == 0) {
        funct3 = 4;
    } else if (strcmp(opr, "bge") == 0) {
        funct3 = 5;
    } else if (strcmp(opr, "bltu") == 0) {
        funct3 = 6;
    } else if (strcmp(opr, "bgeu") == 0) {
        funct3 = 7;
    }

    char *reg = strtok(NULL, ", ");
    unsigned rs_1 = reg_index(reg);
    reg = strtok(NULL, ", ");
    unsigned rs_2 = reg_index(reg);
    reg = strtok(NULL, ", ");
    int immediate = atoi(reg); 

    if (immediate & (1 << 11)) { 
        immediate |= 0xFFFFF000;
    }

    unsigned imm12 = (immediate >> 12) & 0x1;
    unsigned imm11 = (immediate >> 11) & 0x1;
    unsigned imm10_5 = (immediate >> 5) & 0x3F;
    unsigned imm4_1 = (immediate >> 1) & 0xF;

    instr->instruction |= opcode;   
    instr->instruction |= (imm11 << 7);
    instr->instruction |= (imm4_1 << 8);
    instr->instruction |= (funct3 << 12);
    instr->instruction |= (rs_1 << 15);
    instr->instruction |= (rs_2 << 20);
    instr->instruction |= (imm10_5 << 25);
    instr->instruction |= (imm12 << 31); 
}

void parse_S_type(char *opr, instruction_t *instr)
{
    instr->instruction = 0;
    unsigned opcode = 35;
    unsigned funct3 = 0;

    if (strcmp(opr, "sb") == 0) {
        funct3 = 0;
    } else if (strcmp(opr, "sh") == 0) {
        funct3 = 1;
    } else if (strcmp(opr, "sw") == 0) {
        funct3 = 2;
    } else if (strcmp(opr, "sd") == 0) {
        funct3 = 3;
    }

    char *reg = strtok(NULL, ", ");
    unsigned rs_2 = reg_index(reg);

    reg = strtok(NULL, "(");
    int immediate = atoi(reg);

    reg = strtok(NULL, ")");
    unsigned rs_1 = reg_index(reg);

    unsigned imm11_5 = (immediate >> 5) & 0x7F;
    unsigned imm4_0 = immediate & 0x1F;

    instr->instruction |= opcode;
    instr->instruction |= (imm4_0 << 7);
    instr->instruction |= (funct3 << 12);
    instr->instruction |= (rs_1 << 15);
    instr->instruction |= (rs_2 << 20);
    instr->instruction |= (imm11_5 << 25);
}

int reg_index(char *reg)
{
    unsigned i = 0;

    for (i; i < NUM_OF_REGS; i++) {
        if (strcmp(REGISTER_NAME[i], reg) == 0)
            break;
    }

    return i;
}
