#include "Trace.h"
#include "Branch_Predictor.h"

extern TraceParser *initTraceParser(const char * trace_file);
extern bool getInstruction(TraceParser *cpu_trace);

extern Branch_Predictor *initBranchPredictor();
extern bool predict(Branch_Predictor *branch_predictor, Instruction *instr);

int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<trace-file>");

        return 0;
    }

    // Initialize a CPU trace parser
    TraceParser *cpu_trace = initTraceParser(argv[1]);

    // Initialize a branch predictor
    Branch_Predictor *branch_predictor = initBranchPredictor();

    // Running the trace
    uint64_t num_of_instructions = 0;
    uint64_t num_of_branches = 0;
    uint64_t num_of_correct_predictions = 0;
    uint64_t num_of_incorrect_predictions = 0;

    while (getInstruction(cpu_trace))
    {
        // We are only interested in BRANCH instruction
        if (cpu_trace->cur_instr->instr_type == BRANCH)
        {
            ++num_of_branches;

            if (predict(branch_predictor, cpu_trace->cur_instr))
            {
                ++num_of_correct_predictions;
            }
            else
            {
                ++num_of_incorrect_predictions;
            }
        }
        ++num_of_instructions;
    }

//    printf("Number of instructions: %"PRIu64"\n", num_of_instructions);
//    printf("Number of branches: %"PRIu64"\n", num_of_branches);
    printf("Number of correct predictions: %"PRIu64"\n", num_of_correct_predictions);
    printf("Number of incorrect predictions: %"PRIu64"\n", num_of_incorrect_predictions);

    float performance = (float)num_of_correct_predictions / (float)num_of_branches * 100;
    printf("Predictor Correctness: %f%%\n", performance);
}
