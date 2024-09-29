#ifndef QUEUE
#define QUEUE
#include<stdio.h>
#include<stdlib.h>
#include "list.h"

typedef struct queue
{
  list head;
  list tail;
} queue;

queue *queue_create(void);
void queue_enq(queue *q, void *element);
void *queue_deq(queue *q);
int queue_empty(queue *q);

#endif