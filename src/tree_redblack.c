#include "tree.h"
#include "printing.h"
#include "compare.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


typedef struct treenode {
    struct treenode *parent;
    struct treenode *left;
    struct treenode *right;
    void *elem;
    int8_t black;
} treenode_t;

typedef struct tree {
    treenode_t *root;
    /* The sentinel-node (NIL) functions as a 'colored NULL-pointer' for leaf nodes 
     * eliminates a lot of edge-case conditions for the various rotations, etc. */
    treenode_t *NIL;
    cmpfunc_t cmpfunc;
    unsigned int size;
} tree_t;


tree_t *tree_create(cmpfunc_t cmpfunc) {
    tree_t *new_tree = malloc(sizeof(tree_t));
    treenode_t *sentinel = malloc(sizeof(treenode_t));
    if (new_tree == NULL || sentinel == NULL) {
        ERROR_PRINT("out of memory\n");
        return NULL;
    }
    
    /* create the sentinel */
    sentinel->left = NULL;
    sentinel->right = NULL;
    sentinel->parent = NULL;
    sentinel->elem = NULL;
    sentinel->black = 1;

    new_tree->NIL = sentinel;
    new_tree->root = sentinel;
    new_tree->cmpfunc = cmpfunc;
    new_tree->size = 0;

    return new_tree;
}

unsigned int tree_size(tree_t *tree) {
    return tree->size;
}

/* recursive part of tree_destroy */
static void node_destroy(treenode_t *node) {
    // postorder recursive call stack
    if (node->elem == NULL) return;  // ... if node == tree->NIL
    node_destroy(node->left);
    node_destroy(node->right);
    free(node);
}

void tree_destroy(tree_t *tree) {
    // call the recursive part first
    node_destroy(tree->root);
    // free sentinel (NIL-node), then tree itself
    free(tree->NIL);
    free(tree);
}

/* ---------------Insertion, searching, rotation-----------------
 * TODO: Deletion, tree copying
*/

/* rotate node counter-clockwise */
static void rotate_left(tree_t *tree, treenode_t *a) {
    treenode_t *b = a->right;
    treenode_t *c = a->right->left;

    /* fix root _or_ node a parents' left/right pointers */
    if (a == tree->root) {
        tree->root = b;
    } else {
        (a->parent->left == a) ? (a->parent->left = b) : (a->parent->right = b);
    }

    /* rotate parent-pointers */
    b->parent = a->parent;
    a->parent = b;
    if (c != tree->NIL) c->parent = a;     // c can be a NIL-node

    /* rotate left/right pointers */
    a->right = c;
    b->left = a;
}

/* rotate node clockwise */
static void rotate_right(tree_t *tree, treenode_t *a) {
    treenode_t *b = a->left;
    treenode_t *c = a->left->right;

    /* change root _or_ node a parents' left/right pointers */
    if (a == tree->root) {
        tree->root = b;
    } else {
        (a->parent->left == a) ? (a->parent->left = b) : (a->parent->right = b);
    }

    /* rotate parent-pointers */
    b->parent = a->parent;
    a->parent = b;
    if (c != tree->NIL) c->parent = a;      // c can be a NIL-node

    /* rotate left/right pointers */
    a->left = c;
    b->right = a;
}

int8_t tree_contains(tree_t *tree, void *elem) {
    treenode_t *curr = tree->root;
    int8_t direction;

    /* traverse until a NIL-node, or return is an equal element is found */
    while (curr != tree->NIL) {
        direction = tree->cmpfunc(elem, curr->elem);
        if (direction > 0) {
            curr = curr->right;
        } else if (direction < 0) {
            curr = curr->left;
        } else {
            // ... direction == 0, tree contains an equal element
            return 1;
        }
    }
    // tree does not contain an equal element
    return 0;
}

/* 
 * Iterative node adding
 * This approach is a bit ugly/spacious, but does not allocate any
 * memory before figuring out whether or not the element is a duplicate
 */
static treenode_t *node_add(tree_t *tree, void *elem) {
    treenode_t *curr = tree->root;
    int8_t direction;

    /* traverse until a NIL-node, or node with equal element is found */
    while ((direction = tree->cmpfunc(elem, curr->elem)) != 0) {
        if (direction > 0) {
            if (curr->right == tree->NIL) {
                treenode_t *newNode = malloc(sizeof(treenode_t));
                if (newNode == NULL) {
                    printf("out of memory\n");
                    return NULL;
                }
                newNode->elem = elem;
                newNode->left = tree->NIL;
                newNode->right = tree->NIL;
                newNode->black = 0;
                newNode->parent = curr;
                curr->right = newNode;
                // return the newly created node
                return newNode;
            }
            // else ... move right in tree
            curr = curr->right;
        } else {
            // ... direction < 0
            if (curr->left == tree->NIL) {
                // same as above, but curr->left
                treenode_t *newNode = malloc(sizeof(treenode_t));
                if (newNode == NULL) {
                    printf("out of memory\n");
                    return NULL;
                }
                newNode->elem = elem;
                newNode->left = tree->NIL;
                newNode->right = tree->NIL;
                newNode->black = 0;
                newNode->parent = curr;
                curr->left = newNode;
                return newNode;
            }
            // else ... move left in tree
            curr = curr->left;
        }
    }
    // tree has an item with the same value, don't add it
    return NULL;
}

void tree_add(tree_t *T, void *elem) {
    /* if root is not added, do so and return */
    if (T->size == 0) {
        treenode_t *newNode = malloc(sizeof(treenode_t));
        newNode->elem = elem;
        newNode->left = T->NIL;
        newNode->right = T->NIL;
        newNode->parent = T->NIL;
        newNode->black = 1;
        T->root = newNode;
        T->size = 1;
        return;
    }

    /* add to the tree, or abort if duplicate item */
    treenode_t *curr = node_add(T, elem);
    if (curr == NULL) return;
    T->size++;

    /* 
     * Balance and/or recolor the tree if needed
     * parent being red triggers the balancing 
     */
    int curr_is_leftchild, par_is_leftchild;
    treenode_t *par, *unc, *gp;

    while (!(curr->parent->black)) {
        // create pointers to parent, uncle and grandparent
        par = curr->parent;
        gp = par->parent;
        // determine uncle by parent/grandparent relation
        (gp->left == par) ? (unc = gp->right) : (unc = gp->left);

        // if uncle is red
        if (!(unc->black)) {
            par->black = 1;
            unc->black = 1;
            if (gp != T->root) gp->black = 0;
            // grandparent may have a red parent at this point, re-loop for gp
            curr = gp;
        } else {
            // determine currents' parent relation (what side)
            (gp->left == par) ? (par_is_leftchild = 1) : (par_is_leftchild = 0);

            // determine parents' relation to their parent (what side)
            (par->left == curr) ? (curr_is_leftchild = 1) : (curr_is_leftchild = 0);

            // Is currents' and uncles' parent relation equal?
            if (par_is_leftchild != curr_is_leftchild) {
                // rotate parent 'away'
                (curr_is_leftchild) ? (rotate_right(T, par)) : (rotate_left(T, par));
            } else {
                // rotate grandparent 'away'
                (curr_is_leftchild) ? (rotate_right(T, gp)) : (rotate_left(T, gp));
                // fix colors
                par->black = 1;
                if (gp != T->root) gp->black = 0;
            }
        }
    }
}

void tree_remove(tree_t *tree, void *elem) {
    return;
}

/* tree_t *tree_copy(tree_t *tree) {
    tree_t *copy = malloc(sizeof(tree_t));

    if (copy == NULL) {
        ERROR_PRINT("out of memory\n");
        return NULL;
    }

    // void *memcpy(void *dest, const void * src, size_t n)
    memcpy(copy, tree, sizeof(tree_t));
    return copy;
} */

/* -------------------------Iteration----------------------------
 * TODO: pre-order iterator for nxtfunc
*/

/* the type of 'next' function used by the iterator */
typedef struct treenode *(*nxtfunc_t)(tree_iter_t *);

typedef struct tree_iter {
    tree_t *tree;
    treenode_t *node;
    nxtfunc_t nxtfunc;
} tree_iter_t;

/* 
 * Morris traversal implementation
 * Works by creating 'links' between a subtrees 'far right leaf' and (sub)root
 */
static treenode_t *next_node_inorder(tree_iter_t *iter) {
    treenode_t *curr, *ret, *NIL;
    NIL = iter->tree->NIL;

    if (iter->node == NIL) return NIL;
    curr = iter->node;
    ret = NIL;          // node to be returned

    while (ret == NIL) {
        if (curr->left == NIL) {
            // can't move further left in current subtree, move right
            ret = curr;
            curr = curr->right;
        } else {
            // ... else, curr has a left child
            // create a predecessor pointer (pre), starting at currents left child
            // then traverse right with pre until we encounter either NIL or curr
            treenode_t *pre = curr->left;
            while (pre->right != NIL && pre->right != curr) {
                pre = pre->right;
            }
            // Determine if we hit a sentinel and need to form a new link,
            // Or whether we have already used the link using curr
            if (pre->right == NIL) {
                // We have reached a leaf, 
                // link it with curr before moving curr pointer left
                pre->right = curr;
                curr = curr->left;
            } else {
                /* ... (pre->right == curr) */
                // Delete the link and move curr pointer right
                pre->right = NIL;
                ret = curr;
                curr = curr->right;
            }
        }
    }
    /* increment iter to the next node */
    iter->node = curr;
    return ret;
}

tree_iter_t *tree_createiter(tree_t *tree, int inOrder) {
    tree_iter_t *new_iter = malloc(sizeof(tree_iter_t));
    if (new_iter == NULL) {
        ERROR_PRINT("new_iter == NULL");
        return NULL;
    }
    new_iter->tree = tree;
    new_iter->node = tree->root;

    /* decide what nxtfunc to point to */
    if (inOrder) {
        new_iter->nxtfunc = next_node_inorder;
        return new_iter;
    } else {
        // direct nxtfunc to a preorder iterator
        /* wasnt sure if i needed a different type of iterator as well, 
         * so future proofed a bit */
        ERROR_PRINT("preorder iteration is not yet implemented\n");
        return NULL;
    }
}

void tree_resetiter(tree_iter_t *iter) {
    if ((iter->node != iter->tree->NIL) && (iter->nxtfunc == next_node_inorder)) {
        treenode_t *curr = NULL;
        while (curr != iter->tree->NIL) {
            // finish the morris iterator process to avoid leaving any mutated leaves
            // this loop does nothing other than correctly finish the iteration process
            curr = next_node_inorder(iter);
        }
    }
    iter->node = iter->tree->root;
}

void tree_destroyiter(tree_iter_t *iter) {
    tree_resetiter(iter);
    free(iter);
}

void *tree_next(tree_iter_t *iter) {
    treenode_t *curr = iter->nxtfunc(iter);
    /* if end of tree is reached, curr->elem will be NULL */
    return curr->elem;
}



/* -------------------- end of active code --------------------*/



/* ---------Alternative/unused function implementations----------
 * TODO: check which is faster for iterative vs recursive versions
 * and/or Unused functions
*/

/* 
 * returns node that contains an equal elem, or NULL if none do
 * currently unused, intended for testing, could be useful for deletion..?
 */
/* static treenode_t *node_contains(tree_t *tree, treenode_t *node, void *elem) {
    // triggers if item is not in tree
    if (node == tree->NIL) return NULL;
    const int direction = tree->cmpfunc(node->elem, elem);

    if (direction > 0) {
        return node_contains(tree, node->right, elem);
    } else if (direction < 0) {
        return node_contains(tree, node->left, elem);
    } else {
        return node;
    }
} */

/*
 * Recursive variant of node_add
 */
/* static int node_add(tree_t *tree, treenode_t *root, treenode_t *newNode) {
    // compare current nodes' and newNodes' elem to figure out where to go next
    const int direction = tree->cmpfunc(root->elem, newNode->elem);
    switch (direction) {
        case (NONE):
            // tree already contains elem, return
            return 0;
        case (RIGHT):
            if (root->right == tree->NIL) {
                newNode->parent = root;
                root->right = newNode;
                return 1;
            }
            return node_add(tree, root->right, newNode);
        case (LEFT):
            if (root->left == tree->NIL) {
                newNode->parent = root;
                root->left = newNode;
                return 1;
            }
            return node_add(tree, root->left, newNode);
        default:
            ERROR_PRINT("cmpfunc should only ever return 0, 1 or -1\n");
            return 0;
    }
} */



/* --------------------------------------------------------------
 * Debugging & testing
 * Everything below is functions for printing, testing and debugging.
 * While it usually makes sense to have tests in a different file, 
 * for trees i found it nescessary to have very detailed prints to follow
 * what was happening, going wrong/right during the actual functions, as opposed to
 * just a unit test verifying results.
*/


/* --- the following comment bracket wraps the entire test program --- */ /*


#include "printing.h"
#include <stdio.h>

// LEN =  nums to be added to the tree (in ascending order [0, LEN>)
// MAX_PRINT = when to stop detailed rotation printing
// (use even integers)

#define LEN  20
#define MAX_PRINT  42

// misc implementation helper functions for QoL / readability
static int is_left_child(treenode_t *node) { if (node->parent->left == node) return 1; return 0; }
static void color_red(treenode_t *node) { node->black = 0; }
static void color_black(treenode_t *node) { node->black = 1; }
static int is_red(treenode_t *node) { if (node->black) return 0; return 1; }
static void debug_print_pass(char *msg) { fprintf(stderr, HGRN "[PASS]" reset " => %s", msg); }

// print node->elem -> (int)
static void p_node_int(treenode_t *node) {
    if (node == NULL) {
        printf("NULL");
    } else {
        void *element = node->elem;
        if (element == NULL) {
            printf(BLKB "NIL" reset "\n");
        } else {
            if (node->black) {
                printf(BLKHB " %d " reset "\n", *(int *)element);
            } else {
                printf(REDB " %d " reset "\n", *(int *)element);
            }
        }
    }
}

// uses either p_node_str or p_node_int
static void p_node_info(treenode_t *node, char *descr) {
    printf("%s : ", descr);
    p_node_int(node);
}

// recursive part of print_2D
static void print_2D_REC(tree_t *tree, treenode_t* root, int height) {
    // Modified version of https://www.geeksforgeeks.org/print-binary-tree-2-dimensions/
    if (root == NULL) return;
    height += 10;

    // call the bigger elements first //
    print_2D_REC(tree, root->right, height);

    // printing
    printf("\n");
    for (int i = 10; i < height; i++) printf(" ");

    // call node value printing function
    p_node_int(root);      // for tree of integers

    print_2D_REC(tree, root->left, height);
}

// calls print_2D_REC
static void print_2D(tree_t *tree, char *title) {
    printf("[print_2D]: %s\n", title);
    print_2D_REC(tree, tree->root, 0);
    printf("\n");
}

// tree add with step by step printing as coloring / rotations / cases occur
static void print_tree_add(tree_t *T, void *elem) {
    // if root is not added, do so and return
    if (T->size == 0) {
        treenode_t *newNode = malloc(sizeof(treenode_t));
        newNode->elem = elem;
        newNode->left = T->NIL;
        newNode->right = T->NIL;
        newNode->parent = T->NIL;
        newNode->black = 1;
        T->root = newNode;
        T->size = 1;
        return;
    }

    // add to the tree
    treenode_t *curr = node_add(T, elem);
    if (curr == NULL) {
        // element was duplicate, avoid adding and return
        return;
    }
    print_2D(T, "node_add finished");
    treenode_t *tmp = curr;
    treenode_t *par, *unc, *gp;

    // parent being red triggers the balancing
    while (is_red(curr->parent)) {
        // create pointers to parent, uncle and grandparent
        par = curr->parent;
        gp = curr->parent->parent;
        // determine uncle by parent/grandparent relation
        (gp->left == par) ? (unc = gp->right) : (unc = gp->left);
        
        p_node_info(tmp,  "Adding");
        p_node_info(curr, "Curr");
        p_node_info(par,  "    -> par");
        p_node_info(unc,  "    -> unc");
        p_node_info(gp,   "    -> gp ");

        if (is_red(unc)) {
            DEBUG_PRINT("1) uncle is red -> recoloring\n");
            // (if uncle is red, grandparent must be black)
            print_2D(T, "pre coloring");
            color_black(par);
            color_black(unc);
            if (gp != T->root) color_red(gp);
            print_2D(T, "post coloring");
            // grandparent may have a red parent at this point
            // change curr and repeat the case process
            curr = gp;
        } else {
            DEBUG_PRINT("1) uncle is black -> rotation\n");
            if ((is_left_child(par) != is_left_child(curr))) {
                DEBUG_PRINT("2) same-side uncle -> rotate parent\n");
                if (is_left_child(curr)) {
                    DEBUG_PRINT("3) uncle and curr are both left-children\n");
                    print_2D(T, "pre: [parent: rotate --> right]");
                    rotate_right(T, par);
                } else {
                    DEBUG_PRINT("3) uncle and curr are both right-children\n");
                    print_2D(T, "pre: [parent: rotate <-- left]");
                    rotate_left(T, par);
                }
                print_2D(T, "post rotation:");
            } else {
                DEBUG_PRINT("2) opposite-side uncle -> rotate grandparent\n");
                if (is_left_child(curr)) {
                    DEBUG_PRINT("3) curr is left child -> rotate grandparent right\n");
                    print_2D(T, "pre: [grandparent: rotate --> right]");
                    rotate_right(T, gp);
                } else {
                    DEBUG_PRINT("3) curr is right child -> rotate grandparent left\n");
                    print_2D(T, "pre: [grandparent: rotate --> left]");
                    rotate_left(T, gp);
                }
                // fix colors
                print_2D(T, "post rotation, pre coloring");
                color_black(par);
                if (gp != T->root) color_red(gp);
                print_2D(T, "post rotation and coloring");
            }
        }
    }
    T->size++;
    p_node_info(tmp, "[done adding]");
}

// checks that sentinel (tree->NIL) is as it should
static void check_sentinel_ok(tree_t *tree) {
    int sentinel_ok = 1;
    if (tree->NIL->right != NULL) {
        printf("OOPS: tree->NIL->right != NULL\n");
        sentinel_ok = 0;
        p_node_info(tree->NIL->right, "right");
    }
    if (tree->NIL->left != NULL) {
        printf("OOPS: tree->NIL->left != NULL\n");
        sentinel_ok = 0;
        p_node_info(tree->NIL->left, "left");
    }
    if (tree->NIL->parent != NULL) {
        printf("OOPS: tree->NIL->parent != NULL\n");
        p_node_info(tree->NIL->parent, "parent");
        sentinel_ok = 0;
    }
    if (tree->NIL->elem != NULL) {
        printf("OOPS: tree->NIL->elem != NULL\n");
        sentinel_ok = 0;
    }
    if (!tree->NIL->black) {
        printf("OOPS: tree->NIL is red\n");
        sentinel_ok = 0;
    }
    if (sentinel_ok) debug_print_pass("sentinel is not modified\n");
    else ERROR_PRINT("Sentinel is mutated :(\n");
}

static void debug_add(tree_t *tree, int *nums) {
    for (int i = 0; i < LEN; i++) {
        if (i == 0) DEBUG_PRINT("adding root...\n");
        else if (i == 1) DEBUG_PRINT("adding more nodes...\n");
        
        void *elem = &nums[i];
        if (LEN < MAX_PRINT) {
            print_tree_add(tree, elem);
        } else {
            tree_add(tree, elem);
        }
    }
    if (LEN < MAX_PRINT) printf("=> Done adding all. Scroll up to inspect tree\n");
    else printf("=> Done adding nodes.\n");
}

static void debug_contains(tree_t *tree) {
    int *morenums = calloc(LEN, sizeof(int));
    if (morenums == NULL) ERROR_PRINT("out of memory\n");

    // using LEN, create an array where exactly half of the
    // elements are contained within the tree
    for (int i = LEN-1; i >= 0; i--) {
        if (i % 2 == 0) morenums[i] = i;
        else morenums[i] = -i;
    }

    int correct_result = 0;
    int expected_correct = (int)((float)LEN/2.0);

    // loop morenums and check the tree for each number
    for (int i = 0; i < LEN; i++) {
        void *elem = &morenums[i];
        if (tree_contains(tree, elem) && (morenums[i] % 2 == 0)) {
            correct_result++;
        }
    }

    if (correct_result == expected_correct) {
        fprintf(stderr,
        HGRN "[PASS] " reset "=> Tree_contains works as expected -> %d/%d correct\n",
        correct_result, expected_correct);
    } else {
        fprintf(stderr,
        BRED "[ERROR] " reset "=> Tree_contains is not working as exptected -> %d/%d correct\n",
        correct_result, expected_correct);
    }
}

static void debug_iterate_full(tree_iter_t *iter) {
    printf("Expecting in-order:\n");
    void *element = tree_next(iter);
    int num = -1;
    int lastnum = -1;
    
    if (LEN < MAX_PRINT) {
        while (element != NULL) {
            num = *(int *)element;
            printf("%d < ", num);
            if (lastnum > num) ERROR_PRINT("last: %d > curr: %d", lastnum, num);
            lastnum = num;
            element = tree_next(iter);
        }
    } else {
        // lazy copy paste here but it works
        while (num < 10) {
            num = *(int *)element;
            printf("%d < ", num);
            if (lastnum > num) ERROR_PRINT("last: %d > curr: %d", lastnum, num);
            lastnum = num;
            element = tree_next(iter);
        }
        while (num != LEN-10) {
            num = *(int *)element;
            if (lastnum > num) ERROR_PRINT("last: %d > curr: %d", lastnum, num);
            lastnum = num;
            element = tree_next(iter);
        }
        printf(". . . < ");
        while (element != NULL) {
            num = *(int *)element;
            printf("%d < ", num);
            if (lastnum > num) ERROR_PRINT("last: %d > curr: %d", lastnum, num);
            lastnum = num;
            element = tree_next(iter);
        }
    }
    
    if (element == NULL && lastnum == LEN-1) {
        printf("END \n");
        debug_print_pass("all elements are iterated over in order until NULL\n");
    } else {
        ERROR_PRINT("not iterating to NULL\n");
    }
}

static void debug_iterate_partial(tree_iter_t *iter) {
    int STOP = MAX_PRINT;
    int num = 0;
    if (STOP >= LEN) STOP = LEN/2;
    printf("Expecting in-order, stopping at (%d) iterations:\n", STOP);
    while (num != STOP-1) {
        num = *(int *)tree_next(iter);
        printf("%d < ", num);
    }
    printf("(. . . < %d)\n", LEN-1);
    debug_print_pass("iterator works like expected\n");
}

// calls debug_xxxx functions
static void init_tests() {
    // tests assume LOG_LEVEL=0, ERROR_FATAL, @ printing.h
    if (LEN % 2 == 1) ERROR_PRINT("use an even number.\n");

    // DEBUG_PRINT("creating array,  [ 0, %d > ...\n", LEN);
    int *nums = calloc(LEN, sizeof(int));
    for (int i = 0; i < LEN; i++) nums[i] = i;
    if (nums == NULL) ERROR_PRINT("failed to create nums array\n");

    DEBUG_PRINT("creating tree...\n");
    tree_t *tree = tree_create(compare_ints);
    if (tree == NULL) ERROR_PRINT("tree == NULL");

    DEBUG_PRINT("adding nodes to the tree...\n");
    debug_add(tree, nums);

    DEBUG_PRINT("testing tree_contains...\n");
    debug_contains(tree);

    DEBUG_PRINT("creating tree iterator...\n");
    tree_iter_t *iter_one = tree_createiter(tree, 1);
    if (iter_one == NULL) ERROR_PRINT("iter == NULL");

    DEBUG_PRINT("testing full iteration through tree_next...\n");
    debug_iterate_full(iter_one);

    DEBUG_PRINT("destroying iterator...\n");
    tree_destroyiter(iter_one);

    DEBUG_PRINT("checking sentinel status...\n");
    check_sentinel_ok(tree);

    DEBUG_PRINT("creating new tree iterator...\n");
    tree_iter_t *iter_two = tree_createiter(tree, 1);
    if (iter_two == NULL) ERROR_PRINT("iter == NULL");

    DEBUG_PRINT("testing incomplete inorder iteration...\n");
    debug_iterate_partial(iter_two);

    DEBUG_PRINT("destroying iterator...\n");
    tree_destroyiter(iter_two);

    DEBUG_PRINT("checking sentinel status...\n");
    check_sentinel_ok(tree);

    DEBUG_PRINT("destroying tree...\n");
    tree_destroy(tree);

    printf("\n\t :^) HOORAY NO SEGFAULT :^)\n");
    debug_print_pass("all tests passed\n");
}

int main() {
    init_tests();
    return 0;
}

*/ /* <-- this comment wraps entire test program */
