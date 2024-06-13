#include "list.h"


void list_init(struct list *plist) {
	plist->head.prev = NULL;
	plist->head.next = &plist->tail;
	plist->tail.prev = &plist->head;
	plist->tail.next = NULL;
}

bool list_empty(struct list *plist) {
	return plist->head.next == &plist->tail;
}

void list_insert_before(struct list_elem *pelem, struct list_elem *inserted) {
	inserted->next = pelem;
	inserted->prev = pelem->prev;
	pelem->prev->next = inserted;
	pelem->prev = inserted;
}

void list_remove(struct list_elem *pelem) {
	pelem->prev->next = pelem->next;
	pelem->next->prev = pelem->prev;
}

void list_push(struct list *plist, struct list_elem *pelem) {
	list_insert_before(plist->head.next, pelem);
}

void list_append(struct list *plist, struct list_elem *pelem) {
	list_insert_before(&plist->tail, pelem);
}

struct list_elem *list_pop(struct list *plist) {
	struct list_elem *pelem = plist->head.next;
	list_remove(plist->head.next);
	return pelem;
}

bool elem_find(struct list *plist, struct list_elem *pelem) {
	for (struct list_elem *ielem = plist->head.next; ielem != &plist->tail;
	     ielem = ielem->next) {
		if (ielem == pelem) {
			return true;
		}
	}
	return false;
}
