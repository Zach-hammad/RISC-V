#ifndef __QUEUE_HH__
#define __QUEUE_HH__

#include <assert.h>

#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include "Request.h"

// Each Node stores one request
struct Node;
typedef struct Node Node;
typedef struct Node
{
    uint64_t mem_addr;
    Request_Type req_type; // Request type

    int bank_id; // Which bank the request targets to

    // Some timing informations.
    uint64_t begin_exe;
    uint64_t end_exe;
    Node *prev;
    Node *next;
}Node;

typedef struct Queue
{
    Node *first;
    Node *last;

    unsigned size; // Current size of the queue
}Queue;

Queue* initQueue()
{
    Queue *q = (Queue *)malloc(sizeof(Queue));

    q->first = NULL;
    q->last = NULL;
    q->size = 0;

    return q;
}	

// Push a request to the queue
void pushToQueue(Queue *q, Request *req)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node->mem_addr = req->memory_address;
    node->req_type = req->req_type;
    node->bank_id = req->bank_id;

    node->prev = NULL;
    node->next = NULL;

    // Check if the queue is empty.
    if (q->first == NULL && q->last == NULL)
    {
        q->first = node;
        q->last = node;
        q->size = 1;
    }
    else
    {
        // Now, the new node becomes the last node of the queue.
        node->prev = q->last;
        q->last->next = node;

        q->last = node;
        q->size = q->size + 1;
    }

    /*
    // print the queue
    Node *iter = q->first;
    while (iter != NULL)
    {
        printf("(""%"PRIu64, iter->mem_addr);
        printf(" : ");
        printf("%d)", iter->bank_id);

        iter = iter->next;
        if (iter != NULL)
        {
            printf(" -> ");
        }
    }
    printf("\n");
    */
}

void migrateToQueue(Queue *q, Node *_node)
{
    Node *node = (Node *)malloc(sizeof(Node));
    node->mem_addr = _node->mem_addr;
    node->req_type = _node->req_type;
    node->bank_id = _node->bank_id;
    node->begin_exe = _node->begin_exe;
    node->end_exe = _node->end_exe;

    node->prev = NULL;
    node->next = NULL;

    // Check if the queue is empty.
    if (q->first == NULL && q->last == NULL)
    {
        q->first = node;
        q->last = node;
        q->size = 1;
    }
    else
    {
        // Now, the new node becomes the last node of the queue.
        node->prev = q->last;
        q->last->next = node;

        q->last = node;
        q->size = q->size + 1;
    }

    /*
    // print the queue
    Node *iter = q->first;
    while (iter != NULL)
    {
        printf("(""%"PRIu64, iter->mem_addr);
        printf(" : ");
        printf("%d)", iter->bank_id);

        iter = iter->next;
        if (iter != NULL)
        {
            printf(" -> ");
        }
    }
    printf("\n");
    */
}

void deleteNode(Queue *q, Node *node)
{
    q->size = q->size - 1;

    if (node == q->first && node == q->last)
    {
        q->first = NULL;
        q->last = NULL;

        free(node);
    }
    else if (node == q->first && node != q->last)
    {
        q->first = node->next; // Node's next node becomes the first node
        q->first->prev = NULL;

        free(node);
    }
    else if (node != q->first && node == q->last)
    {
        q->last = node->prev; // Node's previous node becomes the last node
        q->last->next = NULL;

        free(node);
    }
    else
    {
        Node *prev = node->prev;
        Node *next = node->next;

        prev->next = next;
        next->prev = prev;

        free(node);
    }
}

#endif
