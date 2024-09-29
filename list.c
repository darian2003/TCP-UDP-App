#include "list.h"
#include <stdlib.h>

list addToList(void *element, list l)
{
	list temp = (struct cell*)malloc(sizeof(struct cell));
	temp->element = element;
	temp->next = l;
	return temp;
}

list freeList(list l)
{
	list temp = l->next; 
	free(l);
	return temp;
}