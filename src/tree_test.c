#include "tree.h"
#include "printing.h"
#include "compare.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* -------------------Includes & Definitions---------------------
 * actual program starts at Debugging & testing
*/

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

/* the type of 'next' function used by the iterator */
typedef struct treenode *(*nxtfunc_t)(tree_iter_t *);

typedef struct tree_iter {
    tree_t *tree;
    treenode_t *node;
    nxtfunc_t nxtfunc;
} tree_iter_t;


/* copies of some internal functions for print_tree_add() */

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


/* ---------------------Debugging & testing----------------------
 * LEN =  nums to be added to the tree (in ascending order [0, LEN>)
 * MAX_PRINT = when to stop detailed rotation printing
 * (use even integers for both)
 * PRINT_TREE_2D controls whether or not to print the tree in terminal
 */

#define LEN  10
#define MAX_PRINT  42
#define PRINT_TREE_2D  0

// misc implementation helper functions for QoL / readability
static int is_left_child(treenode_t *node) { if (node->parent->left == node) return 1; return 0; }
static void color_red(treenode_t *node) { node->black = 0; }
static void color_black(treenode_t *node) { node->black = 1; }
static int is_red(treenode_t *node) { if (node->black) return 0; return 1; }
static void debug_print_pass(char *msg) { fprintf(stderr, HGRN "[PASS]" reset " => %s", msg); }


// print node->elem as an integer
static void p_node_int(treenode_t *node) {
    if (node == NULL) {
        printf("NULL\n");
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

static void p_node_int_inline(treenode_t *node) {
    if (node == NULL) {
        printf("NULL");
    } else {
        void *element = node->elem;
        if (element == NULL) {
            printf(BLKB "NIL" reset "");
        } else {
            if (node->black) {
                printf(BLKHB " %d " reset "", *(int *)element);
            } else {
                printf(REDB " %d " reset "", *(int *)element);
            }
        }
    }
}

// uses either p_node_str or p_node_int
static void p_node_info(char *descr, treenode_t *node) {
    printf("%s : ", descr);
    p_node_int(node);
}

static void p_node_relation(treenode_t *a, char *descr, treenode_t *b) {
    p_node_int_inline(a);
    printf("->%-9s : ", descr);
    p_node_int(b);
}

// recursive part of print_2D
static void print_2D_REC(tree_t *tree, treenode_t* root, int height) {
    if (root == NULL) return;
    height += 10;

    // call the bigger elements first
    print_2D_REC(tree, root->right, height);

    // printing
    printf("\n");
    for (int i = 10; i < height; i++) printf(" ");
    p_node_int(root);      // for tree of integers

    print_2D_REC(tree, root->left, height);
}

// calls print_2D_REC
static void print_2D(tree_t *tree, char *title) {
    if (PRINT_TREE_2D) {
        printf("[print_2D]: %s\n", title);
        print_2D_REC(tree, tree->root, 0);
        printf("\n");
    }
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
    int n_loops = 0;
    // parent being red triggers the balancing
    while (is_red(curr->parent)) {
        if (curr != tmp) {
            p_node_info("(balancing after adding)", tmp);
        }
        // create pointers to parent, uncle and grandparent
        par = curr->parent;
        gp = curr->parent->parent;
        // determine uncle by parent/grandparent relation
        (gp->left == par) ? (unc = gp->right) : (unc = gp->left);

        printf("\ncurr(");
        p_node_int_inline(curr);
        printf(") -> parent(");
        p_node_int_inline(par);
        printf(") is red. Balancing (#%d) triggered\n", n_loops);
        p_node_relation(curr, "uncle", unc);
        p_node_relation(curr, "grandpar.", gp);

        if (is_red(unc)) {
            CASE_PRINT("1) uncle is red -> recolor\n");
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
            INFO_PRINT("2) uncle is black -> rotation based on rel. uncle position\n");
            
            if ((is_left_child(par) != is_left_child(curr))) {
                INFO_PRINT("2_a) same-side uncle, \n");
                if (is_left_child(curr)) {
                    CASE_PRINT("2_a_1) uncle and curr are both left-children\n");
                    print_2D(T, "pre: [parent: rotate --> right]");
                    rotate_right(T, par);
                } 
                else {
                    CASE_PRINT("2_a_2) uncle and curr are both right-children\n");
                    print_2D(T, "pre: [parent: rotate <-- left]");
                    rotate_left(T, par);
                }
                print_2D(T, "post rotation:");
            }
            else {
                INFO_PRINT("2_b) opposite-side uncle\n");
                if (is_left_child(curr)) {
                    CASE_PRINT("2_b_1) curr is left child -> rotate grandparent right\n");
                    print_2D(T, "pre: [grandparent: rotate --> right]");
                    rotate_right(T, gp);
                } 
                else {
                    CASE_PRINT("2_b_2) curr is right child -> rotate grandparent left\n");
                    print_2D(T, "pre: [grandparent: rotate --> left]");
                    rotate_left(T, gp);
                }
                // fix colors
                INFO_PRINT("2_b_x) also requires coloring:\n");
                print_2D(T, "post rotation, pre coloring");
                color_black(par);
                if (gp != T->root) color_red(gp);
                print_2D(T, "post rotation & coloring");
            }
        }
        n_loops++;
    }

    T->size++;
    if (n_loops == 0) {
        printf("[no balancing needed] : ");
        p_node_int_inline(curr);
        printf("\n");
    } else {
        p_node_info("[done balancing after adding]", tmp);
    }
    printf("tree size: %d", T->size);
    printf("\n");
}

// checks that sentinel (tree->NIL) is as it should
static void check_sentinel_ok(tree_t *tree) {
    int sentinel_ok = 1;
    if (tree->NIL->right != NULL) {
        DEBUG_PRINT("OOPS: tree->NIL->right != NULL\n");
        sentinel_ok = 0;
        p_node_info("right", tree->NIL->right);
    }
    if (tree->NIL->left != NULL) {
        DEBUG_PRINT("OOPS: tree->NIL->left != NULL\n");
        sentinel_ok = 0;
        p_node_info("left", tree->NIL->left);
    }
    if (tree->NIL->parent != NULL) {
        DEBUG_PRINT("OOPS: tree->NIL->parent != NULL\n");
        p_node_info("parent", tree->NIL->parent);
        sentinel_ok = 0;
    }
    if (tree->NIL->elem != NULL) {
        DEBUG_PRINT("OOPS: tree->NIL->elem != NULL\n");
        sentinel_ok = 0;
    }
    if (!tree->NIL->black) {
        DEBUG_PRINT("OOPS: tree->NIL is red\n");
        sentinel_ok = 0;
    }
    if (sentinel_ok) debug_print_pass("sentinel is not modified\n");
    else ERROR_PRINT("Sentinel is mutated :(\n");
}

static void debug_add(tree_t *tree, int *nums) {
    for (int i = 0; i < LEN; i++) {
        if (i == 0)
            DEBUG_PRINT("adding root...\n");
        else if (i == 1)
            DEBUG_PRINT("adding more nodes...\n");
        
        void *elem = &nums[i];
        if (LEN < MAX_PRINT)
            print_tree_add(tree, elem);
        else
            tree_add(tree, elem);
    }
    if (LEN < MAX_PRINT)
        printf("> Done adding all. Scroll up to inspect tree\n");
    else 
        printf("=> Done adding nodes.\n");
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

/* 
 * returns node that contains an equal elem, or NULL if none do
 */
static treenode_t *node_contains(tree_t *tree, void *elem) {
    treenode_t *curr = tree->root;
    int8_t direction;

    /* traverse until a NIL-node, or node with equal element is found */
    while (curr != tree->NIL) {
        direction = tree->cmpfunc(elem, curr->elem);
        if (direction > 0) {
            // move right
            curr = curr->right;
        } else if (direction < 0) {
            // move left
            curr = curr->left;
        } else {
            // node found
            return curr;
        }
    }
    // no nodes contain the elem
    return NULL;
}

/* 
 * replaces a node to be removed with its successor
 * returns a pointer to the successor node (tree->NIL if the tree is now empty)
 */
static treenode_t *node_replace(tree_t *tree, treenode_t *node) {
    treenode_t *NIL, *par;
    int node_is_leftchild, node_is_root;

    NIL = tree->NIL;
    par = node->parent;

    if (node == tree->root) {
        node_is_root = 1;
    } else {
        node_is_root = 0;
        // determine parent relation
        (par->left == node) ? (node_is_leftchild = 1) : (node_is_leftchild = 0);
    }

    /* if node does not have two children */
    if ((node->left == NIL) || (node->right == NIL)) {
        if ((node->left == NIL) && (node->right == NIL)) {
            /* node has no children */
            if (node_is_root) {
                tree->root = NIL;
            } else {
                (node_is_leftchild) ? (par->left = NIL) : (par->right = NIL);
            }
        }
        /* node has only one child, replace the node with its child */
        else if (node->left == NIL) {
            /* replace node with its right child */
            if (node_is_root) {
                tree->root = node->right;
            } else {
                (node_is_leftchild) ? (par->left = node->right) : (par->right = node->right);
            }
            node->right->parent = par;
        }
        else {  /* ... (node->right == NIL) */
            /* replace node with its left child */
            if (node_is_root) {
                tree->root = node->left;
            } else {
                (node_is_leftchild) ? (par->left = node->left) : (par->right = node->left);
            }
            node->left->parent = par;
        }
    }
    else { /* ... node has two children */
        /* replace node with its inorder successor, i.e., the left-most node in the right subtree */
        treenode_t *curr = node->right;
        while (curr->left != NIL) {
            curr = curr->left;
        }

        /* if successor is not right child of node */
        if (node->right != curr) {
            curr->parent->left = NIL;
            curr->right = node->right;
        } else {
            // retain curr->right
            CASE_PRINT("successor is right child of node\n");
        }


        if (node_is_root) {
            tree->root = curr;
        } else {
            (node_is_leftchild) ? (par->left = curr) : (par->right = curr);
        }
        
        // inherit the nodes' relations
        curr->parent = node->parent;
        curr->left = node->left;
        curr->left->parent = curr;
        curr->right->parent = curr;
    }

    if (node_is_root) {
        return tree->root;
    } else if (node_is_leftchild) {
        return par->left;
    } else {
        return par->right;
    }
}

void tree_remove(tree_t *tree, void *elem) {
    printf("\nattempting to remove node with '%d'\n", *(int*)elem);
    treenode_t *node = node_contains(tree, elem);
    if (node == NULL) {
        DEBUG_PRINT("elem is not in the tree\n");
        return;
    }

    /* replace node with its successor */
    treenode_t *succ = node_replace(tree, node);
    p_node_relation(node, "successor", succ);

    /* free the node with the item */
    free(node);
    tree->size--;

    /* if tree is empty, don't try to balance it */
    // could also check for succ == NIL for same test
    if (tree->size == 0) return;

    p_node_relation(succ, "parent", succ->parent);
    p_node_relation(succ, "left", succ->left);
    p_node_relation(succ, "right", succ->right);

    /* balancing / recoloring */
}

static void debug_remove(tree_t *tree) {
    print_2D(tree, "pre removal");
    int to_remove = 7;
    void *elem = &to_remove;
    tree_remove(tree, elem);
    print_2D(tree, "post removal");
    to_remove = 8;
    tree_remove(tree, elem);
    print_2D(tree, "post removal");
}

// calls debug_xxxx functions
static void init_tests() {
    if (LEN % 2 == 1) ERROR_PRINT("use an even number.\n");

    printf("> creating array to store node values,  [ 0, %d > ...\n", LEN);
    int *nums = calloc(LEN, sizeof(int));
    if (nums == NULL) ERROR_PRINT("failed to create nums array\n");
    for (int i = 0; i < LEN; i++) nums[i] = i;

    printf("> creating tree...\n");
    tree_t *tree = tree_create(compare_ints);
    if (tree == NULL) ERROR_PRINT("tree == NULL");

    printf("> adding nodes to the tree...\n");
    debug_add(tree, nums);

    TEST_PRINT("testing tree_contains...\n");
    debug_contains(tree);

    printf("> creating tree iterator...\n");
    tree_iter_t *iter_one = tree_createiter(tree, 1);
    if (iter_one == NULL) ERROR_PRINT("iter == NULL");

    TEST_PRINT("testing full iteration through tree_next...\n");
    debug_iterate_full(iter_one);

    printf("> destroying iterator...\n");
    tree_destroyiter(iter_one);

    TEST_PRINT("checking sentinel status...\n");
    check_sentinel_ok(tree);

    printf("> creating new tree iterator...\n");
    tree_iter_t *iter_two = tree_createiter(tree, 1);
    if (iter_two == NULL) ERROR_PRINT("iter == NULL");

    TEST_PRINT("testing incomplete inorder iteration...\n");
    debug_iterate_partial(iter_two);

    printf("> destroying iterator...\n");
    tree_destroyiter(iter_two);

    TEST_PRINT("checking sentinel status...\n");
    check_sentinel_ok(tree);

    TEST_PRINT("testing tree_remove...\n");
    debug_remove(tree);

    printf("> destroying tree...\n");
    tree_destroy(tree);

    printf("\n\t :^) HOORAY NO SEGFAULT :^)\n");
}

int main() {
    init_tests();
    return 0;
}
