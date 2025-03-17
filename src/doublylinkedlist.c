/**
 * @authors
 * Steffen Viken Valvaag <steffenv@cs.uit.no>
 * Odin Bjerke <odin.bjerke@uit.no>
 */

#include <stdlib.h>

#include "defs.h"
#include "list.h"
#include "printing.h"


typedef struct lnode lnode_t;
struct lnode {
    lnode_t *next;
    lnode_t *prev;
    void *item;
};

struct list {
    lnode_t *head;
    lnode_t *tail;
    size_t length;
    cmp_fn cmpfn;
};

struct list_iter {
    list_t *list;
    lnode_t *node;
};


static lnode_t *newnode(void *item) {
    lnode_t *node = malloc(sizeof(lnode_t));
    if (!node) {
        pr_error("Cannot allocate memory\n");
        return NULL;
    }
    node->next = NULL;
    node->prev = NULL;
    node->item = item;

    return node;
}

list_t *list_create(cmp_fn cmpfn) {
    list_t *list = malloc(sizeof(list_t));
    if (!list) {
        pr_error("Cannot allocate memory\n");
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
    list->cmpfn = cmpfn;

    return list;
}

void list_destroy(list_t *list, free_fn item_free) {
    while (list->head != NULL) {
        lnode_t *next = list->head->next;
        if (item_free) {
            item_free(list->head->item);
        }
        free(list->head);
        list->head = next;
    }

    free(list);
}

size_t list_length(list_t *list) {
    return list->length;
}

int list_addfirst(list_t *list, void *item) {
    lnode_t *node = newnode(item);
    if (node == NULL) {
        return -1;
    }

    if (list->head == NULL) {
        list->head = list->tail = node;
    } else {
        list->head->prev = node;
        node->next = list->head;
        list->head = node;
    }
    list->length++;

    return 0;
}

int list_addlast(list_t *list, void *item) {
    lnode_t *node = newnode(item);
    if (node == NULL) {
        return -1;
    }

    if (list->head == NULL) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
        list->tail = node;
    }
    list->length++;

    return 0;
}

void *list_popfirst(list_t *list) {
    if (list->head == NULL) {
        PANIC("attempt to pop from empty list\n");
    }

    void *item = list->head->item;
    lnode_t *tmp = list->head;

    list->head = list->head->next;
    if (list->head == NULL) {
        list->tail = NULL;
    } else {
        list->head->prev = NULL;
    }

    list->length--;
    free(tmp);

    return item;
}

void *list_poplast(list_t *list) {
    if (list->head == NULL) {
        PANIC("attempt to pop from empty list\n");
    }

    void *item = list->tail->item;
    lnode_t *tmp = list->tail;
    list->tail = list->tail->prev;

    if (list->tail == NULL) {
        list->head = NULL;
    } else {
        list->tail->next = NULL;
    }

    free(tmp);
    list->length--;

    return item;
}

int list_contains(list_t *list, void *item) {
    lnode_t *node = list->head;

    while (node != NULL) {
        if (list->cmpfn(item, node->item) == 0) {
            return 1;
        }
        node = node->next;
    }

    return 0;
}

/*
 * Merges two sorted lists a and b using the given comparison function.
 * Only assigns the next pointers; the prev pointers will have to be
 * fixed by the caller.  Returns the head of the merged list.
 */
static lnode_t *merge(lnode_t *a, lnode_t *b, cmp_fn cmpfn) {
    lnode_t *head, *tail;

    /* Pick the smallest head node */
    if (cmpfn(a->item, b->item) < 0) {
        head = tail = a;
        a = a->next;
    } else {
        head = tail = b;
        b = b->next;
    }

    /* Now repeatedly pick the smallest head node */
    while (a && b) {
        if (cmpfn(a->item, b->item) < 0) {
            tail->next = a;
            tail = a;
            a = a->next;
        } else {
            tail->next = b;
            tail = b;
            b = b->next;
        }
    }

    /* Append the remaining non-empty list (if any) */
    if (a) {
        tail->next = a;
    } else {
        tail->next = b;
    }

    return head;
}

/**
 * Splits the given list in two halves, and returns the head of
 * the second half.
 */
static lnode_t *splitlist(lnode_t *head) {
    /* Move two pointers, a 'slow' one and a 'fast' one which moves
     * twice as fast.  When the fast one reaches the end of the list,
     * the slow one will be at the middle.
     */
    lnode_t *slow = head;
    lnode_t *fast = head->next;

    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }

    /* Now 'cut' the list and return the second half */
    lnode_t *half = slow->next;
    slow->next = NULL;

    return half;
}

/**
 * Recursive merge sort.  This function is named mergesort_ to avoid
 * collision with the mergesort function that is defined by the standard
 * library on some platforms.
 */
static lnode_t *mergesort_(lnode_t *head, cmp_fn cmpfn) {
    if (head->next == NULL) {
        return head;
    }

    lnode_t *half = splitlist(head);
    head = mergesort_(head, cmpfn);
    half = mergesort_(half, cmpfn);

    return merge(head, half, cmpfn);
}

void list_sort(list_t *list) {
    if (list->length < 2) {
        return;
    }

    /* Recursively sort the list */
    list->head = mergesort_(list->head, list->cmpfn);

    /* Fix the tail and prev links */
    lnode_t *prev = NULL;
    for (lnode_t *n = list->head; n != NULL; n = n->next) {
        n->prev = prev;
        prev = n;
    }
    list->tail = prev;
}

list_iter_t *list_createiter(list_t *list) {
    list_iter_t *iter = malloc(sizeof(list_iter_t));
    if (iter == NULL) {
        pr_error("Cannot allocate memory\n");
        return NULL;
    }

    iter->list = list;
    iter->node = list->head;

    return iter;
}

void list_destroyiter(list_iter_t *iter) {
    free(iter);
}

int list_hasnext(list_iter_t *iter) {
    return (iter->node == NULL) ? 0 : 1;
}

void *list_next(list_iter_t *iter) {
    if (iter->node == NULL) {
        pr_warn("Attempt to call list_next on exhausted iterator\n");
        return NULL;
    }

    void *item = iter->node->item;
    iter->node = iter->node->next;

    return item;
}

void list_resetiter(list_iter_t *iter) {
    iter->node = iter->list->head;
}
