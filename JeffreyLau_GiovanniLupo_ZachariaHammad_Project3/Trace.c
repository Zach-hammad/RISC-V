#include "Trace.h"

TraceParser *initTraceParser(const char * mem_file)
{
    TraceParser *trace_parser = (TraceParser *)malloc(sizeof(TraceParser));

    trace_parser->fd = fopen(mem_file, "r");
    trace_parser->cur_req = (Request *)malloc(sizeof(Request));

    return trace_parser;
}

bool getRequest(TraceParser *mem_trace)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if ((read = getline(&line, &len, mem_trace->fd)) != -1)
    {
	char delim[] = " \n";

	char *ptr = strtok(line, delim);
        // Extract core ID
        int core_id = atoi(ptr);
        // Extract PC
        ptr = strtok(NULL, delim);
        uint64_t PC = convToUint64(ptr);
        // Extract Load or Store Address
        ptr = strtok(NULL, delim);
        uint64_t load_or_store_addr = convToUint64(ptr);
        // Extract Request Type
        ptr = strtok(NULL, delim);
        Request_Type req_type;
        if (strcmp(ptr, "L") == 0)
        {
            req_type = LOAD;
        }
        else if (strcmp(ptr, "S") == 0)
        {
            req_type = STORE;
        }

        mem_trace->cur_req->req_type = req_type;
        mem_trace->cur_req->load_or_store_addr = load_or_store_addr;
        mem_trace->cur_req->PC = PC;
        mem_trace->cur_req->core_id = core_id;

        free(line);
        line = NULL;
//        printMemRequest(mem_trace->cur_req);
	return true;
    }

    // Release memory
    free(line);

    fclose(mem_trace->fd);
    free(mem_trace->cur_req);
    free(mem_trace);
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
void printMemRequest(Request *req)
{
    printf("%d ", req->core_id);

    printf("%"PRIu64" ", req->PC);
    
    printf("%"PRIu64" ", req->load_or_store_addr);

    if (req->req_type == LOAD)
    {
        printf("L\n");
    }
    else if (req->req_type == STORE)
    {
        printf("S\n");
    }
}
