/**
 * @authors
 * Odin Bjerke <odin.bjerke@uit.no>
 *
 * @details
 * Set implementation using red-black binary search tree.
 * In-order morris iterator
 */

#include <stdbool.h>

#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "defs.h"
#include "list.h"
#include "printing.h"
#include "set.h"

typedef enum tnode_color {
    RED = 0,
    BLACK,
} tnode_color_t;

typedef struct tnode tnode_t;
struct tnode {
    tnode_color_t color;
    void *elem;
    tnode_t *parent;
    tnode_t *left;
    tnode_t *right;
};

struct set {
    tnode_t *root;
    cmp_fn cmpfn;
    size_t length;
    int n_iterators;
};

static tnode_t sentinel = {.color = BLACK};

/**
 * The sentinel-node (NIL) functions as a 'colored NULL-pointer' for leaf nodes.
 * Eliminates a lot of edge-case conditions for the various rotations, etc.
 */
#define NIL &sentinel

/* --------------------Create, Destroy-------------------- */

set_t *set_create(cmp_fn cmpfn) {
    set_t *set = malloc(sizeof(set_t));
    if (set == NULL) {
        return NULL;
    }

    set->root = NIL;
    set->cmpfn = cmpfn;
    set->length = 0;
    set->n_iterators = 0;

    return set;
}

size_t set_length(set_t *set) {
    return set->length;
}

/**
 * @brief Recursive part of set_destroy. Probably some neat way to this without the overhead of
 * recursion, but whatever.
 */
static void rec_postorder_destroy(set_t *set, tnode_t *node, free_fn elem_freefn) {
    if (node == NIL) {
        return;
    }

    rec_postorder_destroy(set, node->left, elem_freefn);
    rec_postorder_destroy(set, node->right, elem_freefn);

    /**
     * Free self when returned from the recursive call stack. At this point, any children are also
     * freed. This results in the entire tree being freed in a bottom-up (postorder) manner, so
     * that no nodes depend on freed nodes for traversal a freed node depend on this node for
     * traversal free self when returned
     */
    if (elem_freefn) {
        elem_freefn(node->elem);
    }
    free(node);
}

void set_destroy(set_t *set, free_fn elem_freefn) {
    if (set->n_iterators != 0) {
        pr_error(
            "Mismatch in number of created vs. number of freed iterators."
            " The set still has %d active iterators, you likely forgot to destroy one.\n",
            set->n_iterators
        );
    }

    rec_postorder_destroy(set, set->root, elem_freefn);
    free(set);
}

/* ------------------------Rotation----------------------- */

/* rotate node counter-clockwise */
static inline void rotate_left(set_t *set, tnode_t *u) {
    tnode_t *v = u->right;

    u->right = v->left;
    if (v->left != NIL) {
        v->left->parent = u;
    }

    v->parent = u->parent;
    if (u->parent == NIL) {
        set->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }

    v->left = u;
    u->parent = v;
}

/* rotate node clockwise */
static inline void rotate_right(set_t *set, tnode_t *u) {
    tnode_t *v = u->left;

    u->left = v->right;
    if (v->right != NIL) {
        v->right->parent = u;
    }

    v->parent = u->parent;
    if (u->parent == NIL) {
        set->root = v;
    } else if (u == u->parent->right) {
        u->parent->right = v;
    } else {
        u->parent->left = v;
    }

    v->right = u;
    u->parent = v;
}

/* -----------------------Insertion----------------------- */

/* balance the tree (set) after adding a node */
static void post_insert_balance(set_t *set, tnode_t *added_node) {
    tnode_t *curr = added_node;

    while (curr->parent->color == RED) {
        tnode_t *par = curr->parent; // parent
        tnode_t *gp = par->parent;   // grandparent

        bool par_is_leftchild = (gp->left == par);
        tnode_t *unc = par_is_leftchild ? gp->right : gp->left; // uncle

        if (unc->color == RED) {
            /* case 1: red uncle - recolor and move up the tree */
            unc->color = BLACK;
            par->color = BLACK;
            gp->color = RED;
            curr = gp;
        } else {
            /* Case 2 & 3: black uncle - rotation needed */
            if (par_is_leftchild) {
                if (curr == par->right) {
                    /* Case 2a: Left-Right */
                    rotate_left(set, par);
                    curr = par;
                    par = curr->parent;
                }
                /* case 3a: Left-Left */
                rotate_right(set, gp);
            } else {
                if (curr == par->left) {
                    /* case 2b: Right-Left */
                    rotate_right(set, par);
                    curr = par;
                    par = curr->parent;
                }
                /* case 3b: Right-Right se */
                rotate_left(set, gp);
            }

            /* fix colors after rotation */
            par->color = BLACK;
            gp->color = RED;
            break;
        }
    }

    // ensure the root is always black
    set->root->color = BLACK;
}

static inline tnode_t *node_create(tnode_color_t color, void *elem, tnode_t *parent) {
    tnode_t *node = malloc(sizeof(tnode_t));

    if (node == NULL) {
        pr_error("Failed to allocate memory\n");
    } else {
        node->color = color;
        node->elem = elem;
        node->left = node->right = NIL;
        node->parent = parent;
    }

    return node;
}

/**
 * @returns
 * - NIL if already present, setting existing pointer
 *
 * - NULL on failure
 *
 * - new node on success
 *
 */
static tnode_t *node_insert(set_t *set, void *elem, tnode_t **duplicate) {
    tnode_t *curr = set->root;
    tnode_t **new_node_parent_ref = NULL;

    /* traverse until a NIL-node, or node with equal element is found */
    while (1) {
        int cmp = set->cmpfn(elem, curr->elem);

        if (cmp == 0) {
            *duplicate = curr;
            return NIL;
        } else if (cmp > 0) {
            if (curr->right == NIL) {
                new_node_parent_ref = &curr->right;
                break;
            }
            // a < b => move right in tree
            curr = curr->right;
        } else {
            // a > b => move left in tree
            if (curr->left == NIL) {
                new_node_parent_ref = &curr->left;
                break;
            }
            // else ... move left in tree
            curr = curr->left;
        }
    }

    tnode_t *new_node = node_create(RED, elem, curr);
    if (new_node == NULL) {
        return NULL;
    }

    *new_node_parent_ref = new_node; // will be either curr->right or curr->left

    return new_node;
}

void *set_insert(set_t *set, void *elem) {
    tnode_t *new_node = NULL;

    /* case: tree does not have a root yet */
    if (set->root == NIL) {
        new_node = node_create(BLACK, elem, NIL);
        if (new_node == NULL) {
            PANIC("Out of memory\n");
        }
        set->root = new_node;
        set->length += 1;
        return NULL;
    }

    tnode_t *duplicate = NULL;
    new_node = node_insert(set, elem, &duplicate);

    if (new_node == NULL) {
        PANIC("Failed to insert - likely out of memory\n");
    }
    if (new_node == NIL) {
        return duplicate->elem;
    }

    set->length += 1;
    post_insert_balance(set, new_node);

    // if (DEBUG_VALIDATE_BALANCED) {
    //     validate_rbtree(set);
    // }

    return NULL;
}

/* -------------------------Search------------------------ */

static inline tnode_t *node_search(set_t *set, void *elem) {
    tnode_t *curr = set->root;

    /* traverse until a NIL-node, or return is an equal element is found */
    while (curr != NIL) {
        int direction = set->cmpfn(elem, curr->elem);

        if (direction > 0) {
            /* a < b    => curr < target   => go right */
            curr = curr->right;
        } else if (direction < 0) {
            /* a > b    => curr > target   => go left */
            curr = curr->left;
        } else {
            /* current node holds the target element */
            break;
        }
    }

    /* NIL if we reached the end, otherwise the target */
    return curr;
}

void *set_get(set_t *set, void *elem) {
    tnode_t *node = node_search(set, elem);

    return node->elem;
}

/* ---------------------Set operations-------------------- */

/**
 * Recursive part of set_copy. Highly optimized.
 * Copies each node with no comparisons.
 */
static tnode_t *rec_set_copy(tnode_t *orig_node, tnode_t *parent) {
    if (orig_node == NIL) {
        return NIL;
    }

    tnode_t *new_node = malloc(sizeof(tnode_t));
    if (!new_node) {
        PANIC("Failed to allocate memory during set copy\n");
    }

    new_node->color = orig_node->color;
    new_node->elem = orig_node->elem;
    new_node->parent = parent;

    new_node->left = rec_set_copy(orig_node->left, new_node);
    new_node->right = rec_set_copy(orig_node->right, new_node);

    return new_node;
}

static set_t *set_copy(set_t *set) {
    set_t *set_cpy = malloc(sizeof(set_t));
    if (!set_cpy) {
        pr_error("Failed to allocate memory during set copy\n");
        return NULL;
    }

    set_cpy->cmpfn = set->cmpfn;
    set_cpy->length = set->length;
    set_cpy->root = rec_set_copy(set->root, NIL);

    return set_cpy;
}

/**
 * Recursive part of set_copy.
 * Copies each node with no comparisons.
 */
static void rec_set_merge(set_t *target, tnode_t *root) {
    if (root == NIL) {
        return;
    }

    rec_set_merge(target, root->right);
    rec_set_merge(target, root->left);
    set_insert(target, root->elem);
}

set_t *set_union(set_t *a, set_t *b) {
    /**
     * pick the smallest set to copy, given that they have the same compare function, otherwise we
     * have to stick with 'a' since since we are about to create a literal shallow copy of the set
     */
    if (a->length < b->length && a->cmpfn == b->cmpfn) {
        set_t *tmp = a;
        a = b;
        b = tmp;
    }

    set_t *c = set_copy(a);
    if (!c) {
        pr_error("Not enough memory to perform set union\n");
        return NULL;
    }

    /* if a is b, c == a || b, so no point in merging. return copy of a. */
    if (a != b) {
        rec_set_merge(c, b->root);
    }

    return c;
}

/**
 * Recursive part of set_intersection
 */
static void rec_set_intersection(set_t *c, set_t *b, tnode_t *root_a) {
    if (root_a == NIL) {
        return;
    }

    rec_set_intersection(c, b, root_a->left);
    rec_set_intersection(c, b, root_a->right);

    /* post order recursion here prevents items from being added in the worst-case fashion if
     * sorted */
    if (set_get(b, root_a->elem)) {
        set_insert(c, root_a->elem);
    }
}

set_t *set_intersection(set_t *a, set_t *b) {
    /* if a is b, c == a || b, so simply copy 'a' */
    if (a == b) {
        return set_copy(a);
    }

    set_t *c = set_create(a->cmpfn);
    if (!c) {
        return NULL;
    }

    /* pick the longest set as lead, as it might lead to drastically faster searches */
    if (a->length < b->length) {
        set_t *tmp = a;
        a = b;
        b = tmp;
    }

    rec_set_intersection(c, b, a->root);

    return c;
}

static void rec_set_difference(set_t *c, set_t *b, tnode_t *root_a) {
    if (root_a == NIL) {
        return;
    }

    rec_set_difference(c, b, root_a->left);
    rec_set_difference(c, b, root_a->right);

    /* post order recursion here prevents items from being added in the worst-case fashion if
     * sorted */
    if (set_get(b, root_a->elem) == NULL) {
        set_insert(c, root_a->elem);
    }
}

set_t *set_difference(set_t *a, set_t *b) {
    /**
     * This one is more tricky to optimize, as (a - b) != (b - a).
     * However, we do want to do it recursively, as our iterator is in-order,
     * which would lead to the worst-case insertion pattern.
     */

    set_t *c = set_create(a->cmpfn);
    if (!c) {
        return NULL;
    }

    /* if a is b, c == { Ø }, so no point in merging. Return empty set. */

    if (a != b) {
        rec_set_difference(c, b, a->root);
    }

    return c;
}


/* -----------------------Iteration----------------------- */

typedef struct set_iter {
    set_t *set;
    tnode_t *node;
} set_iter_t;

/*
 * Morris traversal implementation
 * See https://en.wikipedia.org/wiki/Tree_traversal#Morris_in-order_traversal_using_threading
 */
static tnode_t *next_node_inorder(set_iter_t *iter) {
    if (iter->node == NIL) {
        return NIL;
    }

    tnode_t *curr = iter->node;
    tnode_t *ret = NIL; // node to be returned

    while (ret == NIL) {
        if (curr->left == NIL) {
            /* can't move further left in current subtree, move right */
            ret = curr;
            curr = curr->right;
        } else {
            /* Predecessor; far right node in currents' left subtree */
            tnode_t *pre = curr->left;
            while (pre->right != NIL && pre->right != curr) {
                pre = pre->right;
            }

            /**
             * Determine if we hit a sentinel and need to form a new link
             * or if curr already used the link
             */
            if (pre->right == NIL) {
                /* link leaf node with current, then move left */
                pre->right = curr;
                curr = curr->left;
            } else {
                /* we hit a used link, clean up temporary pointer and move right */
                pre->right = NIL;
                ret = curr; // next in-order node
                curr = curr->right;
            }
        }
    }

    /* increment iter to the next node */
    iter->node = curr;
    return ret;
}

set_iter_t *set_createiter(set_t *set) {
    if (!set) {
        PANIC("Attempt to create iterator for set=NULL\n");
    }

    if (set->n_iterators != 0) {
        pr_warn("Having multiple active iterators for the same set may result in undefined "
                "behavior\n");
    }

    set_iter_t *iter = malloc(sizeof(set_iter_t));
    if (iter == NULL) {
        pr_error("Failed to allocate memory\n");
        return NULL;
    }

    set->n_iterators += 1;
    iter->set = set;
    iter->node = set->root;

    return iter;
}

int set_hasnext(set_iter_t *iter) {
    return (iter->node == NIL) ? 0 : 1;
}

void set_destroyiter(set_iter_t *iter) {
    while (set_hasnext(iter)) {
        // finish the morris iterator process to avoid leaving any mutated leaves
        next_node_inorder(iter);
    }

    iter->set->n_iterators -= 1;

    free(iter);
}

void *set_next(set_iter_t *iter) {
    tnode_t *curr = next_node_inorder(iter);
    /* if end of tree is reached, curr->elem will be NULL */
    return curr->elem;
}

/* ------------------Runtime validation------------------ */

/**
 * For debugging & verification. Verifies that the tree is in fact balanced.
 * Do not use during iteration.
 * See https://en.wikipedia.org/wiki/Red%E2%80%93black_tree#Properties
 */
ATTR_MAYBE_UNUSED
static void rec_validate_rbtree(tnode_t *node, int black_count, int *path_black_count) {
    /* prop 4: every path from a given node to any of its descendant NIL nodes goes through the
     * same number of black nodes */
    if (node == NIL) {
        if (*path_black_count == -1) {
            *path_black_count = black_count;
        }
        assertf(
            (black_count == *path_black_count),
            "expected count of %d, found %d\n",
            black_count,
            *path_black_count
        );
        return;
    }

    /* prop 3: a red node does not have a red child */
    if (node->color == RED) {
        assert(node != NIL);
        assert(node->left->color != RED && node->right->color != RED);
    } else {
        black_count++; // update black count to track at leaf level
    }

    /* Recursively verify left and right subtrees */
    rec_validate_rbtree(node->left, black_count, path_black_count);
    rec_validate_rbtree(node->right, black_count, path_black_count);
}

ATTR_MAYBE_UNUSED
static void validate_rbtree(set_t *set) {
    if (set->root == NIL) {
        return;
    }

    /* Property 1: Root must be black */
    assert(set->root->color == BLACK);

    int path_black_count = -1;
    rec_validate_rbtree(set->root, 0, &path_black_count);
}

/**
 *
 * ----- delete everything below for assignment, these tests were to confirm it was working ------
 *
 */

/* -------------------------Tests------------------------- */

static void print_int_node(tnode_t *node, int indent) {
    fprintf(
        stderr,
        "%*s%s %d %s\n",
        indent,
        "",
        (node->color == BLACK ? "\e[1;40m" : "\e[1;41m"),
        *((int *) node->elem),
        ANSI_COLOR_RESET
    );
}

static void
rec_print_rbtree(set_t *set, void (*node_print_fn)(tnode_t *, int), tnode_t *node, int curr_space) {
    if (node == NIL) {
        return;
    }
    static const int added_spacing = 8;

    rec_print_rbtree(set, node_print_fn, node->right, curr_space + added_spacing);
    node_print_fn(node, curr_space);
    rec_print_rbtree(set, node_print_fn, node->left, curr_space + added_spacing);
}

static void print_rbtree(set_t *set, void (*node_print_fn)(tnode_t *, int)) {
    fprintf(stderr, "-------------- tree of %zu elems --------------\n\n", set->length);
    rec_print_rbtree(set, node_print_fn, set->root, 0);
    fprintf(stderr, "\n\n<-root                                  leaf->\n");
    fprintf(stderr, "----------------------------------------------\n");
}

typedef struct test_vals {
    char *id;
    set_t *set;
    list_t *elems;
    list_iter_t *elems_iter;
    size_t n_elems;
    size_t n_inserted;
    size_t n_dups;
} test_vals_t;

static test_vals_t *test_vals_create(const char *id) {
    test_vals_t *T = malloc(sizeof(test_vals_t));
    if (!T) {
        PANIC("out of memory\n");
    }

    T->set = set_create((cmp_fn) compare_integers);
    T->elems = list_create((cmp_fn) compare_integers);
    T->id = strdup(id);

    if (!T->set || !T->elems || !T->id) {
        PANIC("Failed to create one or more structures\n");
    }

    T->elems_iter = list_createiter(T->elems);
    if (!T->elems_iter) {
        PANIC("Failed to create list iter\n");
    }

    T->n_elems = 0;
    T->n_dups = 0;
    T->n_inserted = 0;

    return T;
}

static void test_vals_destroy(test_vals_t *test) {
    list_resetiter(test->elems_iter);

    /* destroy only values that are not in set here */
    while (list_hasnext(test->elems_iter)) {
        int *elem = list_next(test->elems_iter);
        if (*elem < 0) {
            free(elem);
        }
    }
    list_destroyiter(test->elems_iter);
    list_destroy(test->elems, NULL);

    /* free the rest of the items using set API, to test that set_destroy works */
    set_destroy(test->set, free);

    free(test->id);
    free(test);
}

static void print_test_vals_info(test_vals_t *test) {
    pr_info("==== test vals \"%s\" ====\n", test->id);
    pr_info("%-20s %zu\n", "n_elems", test->n_elems);
    pr_info("%-20s %zu\n", "n_inserted", test->n_inserted);
    pr_info("%-20s %zu\n", "n_dups", test->n_dups);
}

static int generate_random(int x) {
    int y = rand() % (3 * x);
    return y;
}

static int generate_even(int x) {
    return 2 * x;
}

static int generate_odd(int x) {
    return (2 * x) + 1;
}

/**
 * @param test: pointer to test_vals_t object
 * @param n: number of elements to insert
 * @param generator_fn: function that takes in integer 'n_i' in a sequence of
 * { test->n_elems, ..., test->n_elems + n } and returns an integer. If NULL, the aforementioned
 * sequence is used directly.
 * @note Duplicate elems are multiplied by -1, all others valus will be >= 0.
 * val_mod of 0 -> disabled
 */
static void test_vals_insert(test_vals_t *test, size_t n, int (*generator_fn)(int)) {
    size_t next_i = test->n_elems;
    test->n_elems += n;

    for (; next_i < test->n_elems; next_i++) {
        int *elem = malloc(sizeof(int));
        if (!elem) {
            PANIC("Malloc failed\n");
        }

        if (generator_fn) {
            *elem = generator_fn((int) next_i);
        } else {
            *elem = (int) next_i;
        }

        if (list_addlast(test->elems, elem)) {
            PANIC("list_addlast failed\n");
        }

        int *rv = set_insert(test->set, (void *) elem);

        if (rv) {
            int *eq = set_get(test->set, elem);
            assert(compare_integers(elem, eq) == 0);
            assertf(elem != eq, "pointers should not be equal..\n");
            *elem = ((*elem) * -1);
            test->n_dups += 1;
        } else {
            test->n_inserted += 1;
        }
    }

    // print_rbtree(test->set, print_int_node);

    assertf(
        list_length(test->elems) == test->n_elems,
        "(%zu == %zu)\n",
        list_length(test->elems),
        test->n_elems
    );

    assertf(
        set_length(test->set) == test->n_inserted,
        "(%zu == %zu), n_elems=%zu, n_dups=%zu\n",
        set_length(test->set),
        test->n_inserted,
        test->n_elems,
        test->n_dups
    );

    validate_rbtree(test->set);
}

ATTR_MAYBE_UNUSED
void rbtree_test__set_get(size_t n_elems) {
    test_vals_t *T = test_vals_create(__func__);
    test_vals_insert(T, n_elems, generate_random);

    /**
     * Check that existing elements are found, and vv.
     */
    list_resetiter(T->elems_iter);
    for (size_t i = 0; i < n_elems; i++) {
        assert(list_hasnext(T->elems_iter));
        int *elem = list_next(T->elems_iter);
        int *rv = set_get(T->set, (void *) elem);

        if (*elem >= 0) {
            assert(rv != NULL);
            assert(compare_integers(rv, elem) == 0);
        } else {
            assert(rv == NULL);
        }
    }

    /**
     * Check for some more non-existing values
     */
    for (size_t i = 0; i < n_elems; i++) {
        int elem = (int) 1 + n_elems + (rand() - n_elems);
        assertf((set_get(T->set, &elem) == NULL), "... %d\n", elem);
    }

    // print_test_vals_info(T);
    test_vals_destroy(T);
    pr_debug("OK\n\n");
}

static void rbtree_test__inorder_iter(size_t n_elems) {
    if (n_elems < 10 || n_elems % 10 != 0) {
        PANIC("Must be > 10 and a multiple of 10\n");
    }

    test_vals_t *T = test_vals_create(__func__);

    static const size_t n_iter_runs = 10;
    size_t n_inserts_per_iter = n_elems / n_iter_runs;

    /* perform iteration m consecutive times, with n/m insertions inbetween */
    for (size_t i = 0; i < n_iter_runs; i++) {
        test_vals_insert(T, n_inserts_per_iter, generate_random);

        set_iter_t *iter = set_createiter(T->set);
        assert(iter);

        int *prev = NULL;
        size_t n_iterated = 0;

        while (set_hasnext(iter)) {
            int *elem = set_next(iter);

            assert(elem);
            assert(set_get(T->set, elem) == elem);

            /* verify in-order */
            if (prev) {
                assert(*elem < *prev);
            }

            n_iterated += 1;
        }
        set_destroyiter(iter);

        assertf(n_iterated == T->n_inserted, "(%zu == %zu)\n", T->n_inserted, n_iterated);
    }

    // print_test_vals_info(T);
    // print_rbtree(T->set, print_int_node);

    test_vals_destroy(T);
    pr_debug("OK\n\n");
}

static void rbtree_test__set_operations(size_t n_elems_each) {
    test_vals_t *A = test_vals_create("A"); // even
    test_vals_insert(A, n_elems_each, generate_even);

    test_vals_t *A_eq = test_vals_create("A_eq"); // equal to A
    test_vals_insert(A_eq, n_elems_each, generate_even);

    test_vals_t *B = test_vals_create("B"); // odd
    test_vals_insert(B, n_elems_each, generate_odd);

    test_vals_t *B_exp = test_vals_create("B_exp"); // odd, with twice the length
    test_vals_insert(B_exp, n_elems_each * 2, generate_odd);


    /* --- set_union --- */

    set_t *U = set_union(A->set, B->set);
    if (!U) {
        PANIC("Set union failed\n");
    }
    assert(set_length(U) == set_length(A->set) + set_length(B->set));

    set_iter_t *iter_U = set_createiter(U);
    assert(iter_U);

    size_t n_iterated = 0;
    while (set_hasnext(iter_U)) {
        int *elem = set_next(iter_U);

        if (set_get(A->set, elem) == NULL) {
            assert(set_get(B->set, elem));
        }
        n_iterated += 1;
    }
    assert(n_iterated == set_length(U));
    set_destroyiter(iter_U);
    set_destroy(U, NULL);
    pr_debug("OK: set_union\n");


    /* --- set_intersection --- */

    set_t *I = set_intersection(B->set, B_exp->set);
    if (!I) {
        PANIC("Set intersection failed\n");
    }
    assert(set_length(I) == set_length(B->set));

    set_iter_t *iter_I = set_createiter(I);
    assert(iter_I);

    n_iterated = 0;
    while (set_hasnext(iter_I)) {
        int *elem = set_next(iter_I);

        assert(set_get(B->set, elem));
        assert(set_get(B_exp->set, elem));

        n_iterated += 1;
    }
    assert(n_iterated == set_length(I));
    set_destroyiter(iter_I);
    set_destroy(I, NULL);
    pr_debug("OK: set_intersection\n");


    /* --- set_difference --- */

    set_t *D = set_difference(B_exp->set, B->set);
    if (!D) {
        PANIC("Set difference failed\n");
    }

    set_iter_t *iter_D = set_createiter(D);
    assert(iter_D);

    n_iterated = 0;
    while (set_hasnext(iter_D)) {
        int *elem = set_next(iter_D);
        assert(set_get(B_exp->set, elem));
        assert(set_get(B->set, elem) == NULL);

        n_iterated += 1;
    }
    assert(n_iterated == set_length(D));
    set_destroyiter(iter_D);

    set_t *D_empty_one = set_difference(A->set, A_eq->set);
    set_t *D_empty_two = set_difference(D, D);
    assert(set_length(D_empty_one) == 0);
    assert(set_length(D_empty_two) == 0);

    set_destroy(D_empty_one, NULL);
    set_destroy(D_empty_two, NULL);
    set_destroy(D, NULL);

    pr_debug("OK: set_difference\n");

    /* --- cleanup --- */

    test_vals_destroy(A);
    test_vals_destroy(A_eq);
    test_vals_destroy(B);
    test_vals_destroy(B_exp);
    pr_debug("OK\n\n");
}

ATTR_MAYBE_UNUSED
void set_ops_visual_test(size_t n_elems) {
    /* really can't bother with NULL checks or freeing here.
     * Will be sorted out on exit anyhow, and the program will light up with warnings if something
     * fails anyhow */

    set_t *even = set_create((cmp_fn) compare_integers);
    set_t *odd = set_create((cmp_fn) compare_integers);
    set_t *all = set_create((cmp_fn) compare_integers);

    int *elems = calloc(n_elems, sizeof(int));
    int **elems_p = calloc(n_elems, sizeof(int *));

    for (size_t i = 0; i < n_elems; i++) {
        elems[i] = (int) i;
        elems_p[i] = &elems[i];

        void *elem = (void *) elems_p[i];
        set_insert(all, elem);

        if (i % 2 == 0) {
            set_insert(even, elem);
        } else {
            set_insert(odd, elem);
        }
    }

    pr_info("\neven\n");
    print_rbtree(even, print_int_node);

    pr_info("\nodd\n");
    print_rbtree(odd, print_int_node);

    pr_info("\nall\n");
    print_rbtree(all, print_int_node);

    pr_info("even_odd union\n");
    set_t *even_odd = set_union(even, odd);
    print_rbtree(even_odd, print_int_node);

    pr_info("even_again\n");
    set_t *even_again = set_difference(even_odd, odd);
    print_rbtree(even_again, print_int_node);

    pr_info("nothing\n");
    set_t *nothing = set_difference(even_again, even);
    print_rbtree(nothing, print_int_node);

    pr_info("a\n");
    set_t *a = set_union(nothing, even);
    print_rbtree(a, print_int_node);

    pr_info("b\n");
    set_t *b = set_union(a, odd);
    print_rbtree(b, print_int_node);

    pr_info("inter_a_b\n");
    set_t *inter_a_b = set_intersection(a, b);
    print_rbtree(inter_a_b, print_int_node);

    pr_info("again\n");
    set_t *again = set_intersection(inter_a_b, even);
    print_rbtree(again, print_int_node);

    /* seems good, cba testing further */
}

int main() {
    srand(0xfff);

    // if (!DEBUG_ASSERT_PROPERTIES) {
    //     PANIC("enable DEBUG_ASSERT_PROPERTIES\n");
    // }

    rbtree_test__set_get(2000);
    rbtree_test__inorder_iter(2000);
    rbtree_test__set_operations(2000);
    // set_ops_visual_test(20);

    return EXIT_SUCCESS;
}
