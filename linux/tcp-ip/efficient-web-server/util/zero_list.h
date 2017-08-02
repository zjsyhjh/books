#ifndef __ZERO_LIST_H__
#define __ZERO_LIST_H__

#include <assert.h>

struct list_head {
	struct list_head *prev, *next;
};


#define INIT_LIST_HEAD(ptr) do {\
    struct list_head *_ptr = (struct list_head *)ptr;   \
    (_ptr)->next = (_ptr); (_ptr->prev) = (_ptr);       \
} while(0)



/*
 * insert _new between prev and next
 */
static inline void __list_add(struct list_head *_new, struct list_head *prev, struct list_head *next) {
	_new->next = next;
	next->prev = _new;
	prev->next = _new;
	_new->prev = prev;
}


static inline void list_add(struct list_head *_new, struct list_head *head) {
	__list_add(_new, head, head->next);
}

static inline void list_add_tail(struct list_head *_new, struct list_head *head) {
	__list_add(_new, head->prev, head);
}


static inline void __list_del(struct list_head *prev, struct list_head *next) {
	prev->next = next;
	next->prev = prev;
}

/*
 * delete the entry from list
 */
static inline void list_del(struct list_head *head, struct list_head *entry) {
	assert(entry != head);

	__list_del(entry->prev, entry->next);
}



static inline bool list_empty(struct list_head *head) {
	return (head->next == head) && (head->prev == head);
}




#endif