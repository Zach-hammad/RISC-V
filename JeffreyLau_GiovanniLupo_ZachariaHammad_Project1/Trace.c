#include "Trace.h"

TraceParser *initTraceParser(const char * trace_file)
{
    TraceParser *trace_parser = (TraceParser *)malloc(sizeof(TraceParser));

    trace_parser->fd = fopen(trace_file, "r");
    trace_parser->cur_instr = (Instruction *)malloc(sizeof(Instruction));

    return trace_parser;
}

bool getInstruction(TraceParser *cpu_trace)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if ((read = getline(&line, &len, cpu_trace->fd)) != -1)
    {
	char delim[] = " \n";

        // This is the PC
	char *ptr = strtok(line, delim);
	cpu_trace->cur_instr->PC = convToUint64(ptr);

        // This is the instruction type
        ptr = strtok(NULL, delim);
        if (strcmp(ptr, "B") == 0)
	{
            cpu_trace->cur_instr->instr_type = BRANCH;
        }
        else if (strcmp(ptr, "E") == 0)
        {
            cpu_trace->cur_instr->instr_type = EXE;
        }
        else if (strcmp(ptr, "L") == 0)
        {
            cpu_trace->cur_instr->instr_type = LOAD;
        }
        else if (strcmp(ptr, "S") == 0)
        {
            cpu_trace->cur_instr->instr_type = STORE;
        }

        // More info
        if (strcmp(ptr, "B") == 0)
        {
            ptr = strtok(NULL, delim);

            cpu_trace->cur_instr->taken = atoi(ptr);
        }

        if (strcmp(ptr, "L") == 0 || strcmp(ptr, "S") == 0)
        {
            ptr = strtok(NULL, delim);
            
            cpu_trace->cur_instr->load_or_store_addr = convToUint64(ptr);

            ptr = strtok(NULL, delim);
            
            cpu_trace->cur_instr->size = atoi(ptr);
        }

        free(line);
        line = NULL;
        // printInstruction(cpu_trace->cur_instr);
	return true;
    }

    // Release memory
    free(line);

    fclose(cpu_trace->fd);
    free(cpu_trace->cur_instr);
    free(cpu_trace);
    return false;
}

// convert a string to a uint64_t number
uint64_t convToUint64(char *ptr)
{
    uint64_t ret = 0;

    // Step one, determine the number of chars
    int i = 0;
    char iter = ptr[i];
    while (iter != '\0')
    {
        ++i;
        iter = ptr[i];
    }

    int size = i; // size of the char array

    // Step two, convert to decimal
    uint64_t scale = 1;
    for (i = size - 1; i >= 0; i--)
    {
        ret += (ptr[i] - 48) * scale;
        
	scale *= 10;
    }

    return ret;
}

// print instruction (debugging)
void printInstruction(Instruction *instr)
{
    printf("%"PRIu64" ", instr->PC);

    if (instr->instr_type == BRANCH)
    {
        printf("B ");
        printf("%d\n", instr->taken);
    }
    else if (instr->instr_type == LOAD)
    {
        printf("L ");
        printf("%"PRIu64" ", instr->load_or_store_addr);
        printf("%d\n", instr->size);
    }
    else if (instr->instr_type == STORE)
    {
        printf("S ");
        printf("%"PRIu64" ", instr->load_or_store_addr);
        printf("%d\n", instr->size);
    }
    else if (instr->instr_type == EXE)
    {
        printf("E\n");
    }
}
