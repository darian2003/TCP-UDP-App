#ifndef LIST
#define LIST

typedef struct cell
{
  void *element;
  struct cell *next;
} *list;

list addToList(void *element, list l);
list freeList(list l);

#endif