#include "queue.h"

queue* queue_create(void)
{
  queue *q = (queue*)malloc(sizeof(struct queue));
  q -> head = q -> tail = NULL;
  return q;
}

int queue_empty(queue *q)
{
  return q -> head == NULL;
}

void queue_enq(queue *q, void *element)
{
  if(queue_empty(q))
    q -> head = q->tail = addToList(element, NULL);
  else
    {
      q->tail->next = addToList(element, NULL);
      q->tail = q->tail->next;
    }
}

void *queue_deq(queue *q)
{
  if (queue_empty(q))
    return NULL;
  {
    void *temp = q->head->element;
    q -> head = freeList(q->head);
    return temp;
  }
}