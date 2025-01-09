#ifndef __CONTROLLER_HH__
#define __CONTROLLER_HH__

#include "Bank.h"
#include "Queue.h"

// Bank
extern void initBank(Bank *bank);

// Queue operations
extern Queue* initQueue();
extern void pushToQueue(Queue *q, Request *req);
extern void migrateToQueue(Queue *q, Node *_node);
extern void deleteNode(Queue *q, Node *node);

// CONSTANTS
static unsigned MAX_WAITING_QUEUE_SIZE = 64;
static unsigned BLOCK_SIZE = 128; // cache block size
static unsigned NUM_OF_BANKS = 8; // number of banks

static unsigned nclks_read = 53;
static unsigned nclks_write = 53;

//#define FCFS
#define OOO

// Controller definition
typedef struct Controller
{
    // The memory controller needs to maintain records of all bank's status
    Bank *bank_status;

    // Current memory clock
    uint64_t cur_clk;

    // A queue contains all the requests that are waiting to be issued.
    Queue *waiting_queue;

    // A queue contains all the requests that have already been issued but are waiting to complete.
    Queue *pending_queue;

    /* For decoding */
    unsigned bank_shift;
    uint64_t bank_mask;
    uint64_t conflicts[64];
    unsigned conflict_count;
    unsigned bank_conflicts;

}Controller;

Controller *initController()
{
    Controller *controller = (Controller *)malloc(sizeof(Controller));
    controller->bank_status = (Bank *)malloc(NUM_OF_BANKS * sizeof(Bank));
    controller->conflict_count = 0;
    memset(controller->conflicts, 0, sizeof(controller->conflicts));
    for (int i = 0; i < NUM_OF_BANKS; i++)
    {
        initBank(&((controller->bank_status)[i]));
    }
    controller->cur_clk = 0;
    controller->bank_conflicts = 0;
    controller->waiting_queue = initQueue();
    controller->pending_queue = initQueue();

    controller->bank_shift = log2(BLOCK_SIZE);
    controller->bank_mask = (uint64_t)NUM_OF_BANKS - (uint64_t)1;

    return controller;
}

unsigned ongoingPendingRequests(Controller *controller)
{
    unsigned num_requests_left = controller->waiting_queue->size + 
                                 controller->pending_queue->size;

    return num_requests_left;
}

bool send(Controller *controller, Request *req)
{
    if (controller->waiting_queue->size == MAX_WAITING_QUEUE_SIZE)
    {
        return false;
    }

    // Decode the memory address
    req->bank_id = ((req->memory_address) >> controller->bank_shift) & controller->bank_mask;
    
    // Push to queue
    pushToQueue(controller->waiting_queue, req);

    return true;
}

void handleBankConflict(Controller *controller, Node *node)
{
    // Check if the conflict has already been counted
    bool already_counted = false;
    for (unsigned i = 0; i < controller->conflict_count; i++)
    {
        if (controller->conflicts[i] == node->mem_addr)
        {
            already_counted = true;
            break;
        }
    }

    if (!already_counted)
    {
        controller->bank_conflicts++;
        controller->conflicts[controller->conflict_count++] = node->mem_addr;
    }
}

void removeBankConflict(Controller *controller, Node *node)
{
    // Remove the request from the conflicted_requests list if it was there
    for (unsigned i = 0; i < controller->conflict_count; i++)
    {
        if (controller->conflicts[i] == node->mem_addr)
        {
            // Shift the remaining elements
            for (unsigned j = i; j < controller->conflict_count - 1; j++)
            {
                controller->conflicts[j] = controller->conflicts[j + 1];
            }
            controller->conflict_count--;
            break;
        }
    }
}


void tick(Controller *controller)
{
    // Step one, update system stats
    ++(controller->cur_clk);
    // printf("Clk: ""%"PRIu64"\n", controller->cur_clk);
    for (int i = 0; i < NUM_OF_BANKS; i++)
    {
        ++(controller->bank_status)[i].cur_clk;
        // printf("%"PRIu64"\n", (controller->bank_status)[i].cur_clk);
    }
    // printf("\n");

    // Step two, serve pending requests
    if (controller->pending_queue->size)
    {
        Node *first = controller->pending_queue->first;
        if (first->end_exe <= controller->cur_clk)
        {
            /*
            printf("Clk: ""%"PRIu64"\n", controller->cur_clk);
            printf("Address: ""%"PRIu64"\n", first->mem_addr);
            printf("Bank ID: %d\n", first->bank_id);
            printf("Begin execution: ""%"PRIu64"\n", first->begin_exe);
            printf("End execution: ""%"PRIu64"\n\n", first->end_exe);
            */
            deleteNode(controller->pending_queue, first);
        }
    }

    // Step three, find a request to schedule
    if (controller->waiting_queue->size)
    {
        #ifdef FCFS
        // Implementation One - FCFS
        Node *first = controller->waiting_queue->first;
        int target_bank_id = first->bank_id;

        if ((controller->bank_status)[target_bank_id].next_free <= controller->cur_clk)
        {
            first->begin_exe = controller->cur_clk;
            if (first->req_type == READ)
            {
                first->end_exe = first->begin_exe + (uint64_t)nclks_read;
            }
            else if (first->req_type == WRITE)
            {
                first->end_exe = first->begin_exe + (uint64_t)nclks_write;
            }
            // The target bank is no longer free until this request completes.
            (controller->bank_status)[target_bank_id].next_free = first->end_exe;

            migrateToQueue(controller->pending_queue, first);
            deleteNode(controller->waiting_queue, first);
        } else{
            handleBankConflict(controller,first);
        }
        #endif

        #ifdef OOO
        
        Node *current_node = controller->waiting_queue->first;

        while (current_node != NULL){
            Node *next_node = current_node->next;
            int target_bank_id = current_node->bank_id;

            if ((controller->bank_status)[target_bank_id].next_free <= controller->cur_clk)
            {
                current_node->begin_exe=controller->cur_clk;
                if (current_node->req_type == READ)
                {
                    current_node->end_exe = current_node->begin_exe + (uint64_t)nclks_read;
                } else if (current_node->req_type == WRITE)
                {
                    current_node->end_exe = current_node->begin_exe + (uint64_t)nclks_write;
                }
                (controller->bank_status)[target_bank_id].next_free = current_node->end_exe;

                removeBankConflict(controller,current_node);

                migrateToQueue(controller->pending_queue, current_node);
                deleteNode(controller->waiting_queue, current_node);

            }
            else{
                handleBankConflict(controller,current_node);
            }
            current_node = next_node;
        }
        
        #endif
    }
}


#endif
