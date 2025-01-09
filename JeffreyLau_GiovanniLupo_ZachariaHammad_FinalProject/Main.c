#include "Trace.h"

#include "Controller.h"

extern TraceParser *initTraceParser(const char * mem_file);
extern bool getRequest(TraceParser *mem_trace);

extern Controller *initController();
extern unsigned ongoingPendingRequests(Controller *controller);
extern bool send(Controller *controller, Request *req);
extern void tick(Controller *controller);

int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<mem-file>");

        return 0;
    }

    // Initialize a CPU trace parser
    TraceParser *mem_trace = initTraceParser(argv[1]);

    // Initialize a Controller
    Controller *controller = initController();
    // printf("%u\n", controller->bank_shift);
    // printf("%u\n", controller->bank_mask);

    uint64_t cycles = 0;

    bool stall = false;
    bool end = false;

    while (!end || ongoingPendingRequests(controller))
    {
        if (!end && !stall)
        {
            end = !(getRequest(mem_trace));
        }

        if (!end)
        {
            stall = !(send(controller, mem_trace->cur_req));
        }

        tick(controller);
        ++cycles;
    }

    free(controller->bank_status);
    free(controller->waiting_queue);
    free(controller->pending_queue);
    free(controller);
    printf("End Execution Time: ""%"PRIu64"\n", cycles);
    printf("Num Bank Conflicts: %u\n", controller->bank_conflicts);
}
