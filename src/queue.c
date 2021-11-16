#include <stdio.h>
#include <stdlib.h>
#include "../headers/queue.h"
 
struct Node {
    void * item;
    struct Node* next;
};
 
struct Node* createNode(void * item)
{
    struct Node* temp = (struct Node*) malloc(sizeof(struct Node));
    temp->item = item;
    temp->next = NULL;
    return temp;
}
 
Queue* CreateQueue()
{
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue->out = queue->in = NULL;
    queue->size = 0;
    return queue;
}

int GetQueueSize(Queue * q)
{
    return q->size;
}
 
void enQueue(Queue* q, void * item)
{
    struct Node* temp = createNode(item);
    
    q->size++;
 
    if (q->in == NULL) {
        q->out = q->in = temp;
        return;
    }
 
    q->in->next = temp;
    q->in = temp;
}
 
void * deQueue(Queue* q)
{
    if (q->out == NULL)
        return NULL;
 
    struct Node* temp = q->out;

    q->size--;
 
    q->out = q->out->next;
 
    if (q->out == NULL)
        q->in = NULL;

 
    return temp->item;
}
