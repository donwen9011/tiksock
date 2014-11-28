/* The authors of this work have released all rights to it and placed it
in the public domain under the Creative Commons CC0 1.0 waiver
(http://creativecommons.org/publicdomain/zero/1.0/).

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Retrieved from: http://en.literateprograms.org/Radix_tree_(C)?oldid=18892
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "acl.h"
#include "radix.h"

#define IS_LEAF(node, dir) ((node)->is_leaf &   (1 << (dir)))
#define SET_LEAF(node, dir) ((node)->is_leaf |=  (1 << (dir)))
#define SET_NODE(node, dir) ((node)->is_leaf &= ~(1 << (dir)))
#ifndef ALLOC
# include <stdlib.h>
# define ALLOC malloc
#endif
#ifndef DEALLOC
# include <stdlib.h>
# define DEALLOC free
#endif

struct ROOTSTRUCT {
    uint32_t leafcount;
    union {
        struct _internal_node * node;
        LEAFTYPE                leaf;
    } root;
};

struct _internal_node {
    uint32_t is_leaf :  2;
    uint32_t critbit : 30;
    union {
        struct _internal_node * node;
        LEAFTYPE                leaf;
    } child[2];
};

static int _dfs(struct _internal_node * node,
     int (*leafcb)(LEAFTYPE),
     uint32_t (*inodecb)(struct _internal_node *),
     uint32_t direction,
     int32_t pre_post) {
    uint32_t cur = direction;

    if ((pre_post == -1) && (inodecb != NULL) && (inodecb(node) == 0)) return 0;
    if (IS_LEAF(node, cur)) {
        if (leafcb(node->child[cur].leaf) == 0)
            return 0;
    } else {
        if (_dfs(node->child[cur].node, leafcb, inodecb, direction, pre_post) == 0)
            return 0;
    }

    if ((pre_post ==  0) && (inodecb != NULL) && (inodecb(node) == 0)) return 0;
    cur = 1 - cur; /* now the other child */
    if (IS_LEAF(node, cur)) {
        if (leafcb(node->child[cur].leaf) == 0)
            return 0;
    } else {
        if (_dfs(node->child[cur].node, leafcb, inodecb, direction, pre_post) == 0)
            return 0;
    }
    if ((pre_post ==  1) && (inodecb != NULL) && (inodecb(node) == 0)) return 0;

    return 1;
}

static uint32_t _dealloc_internal_node(struct _internal_node * node) {
    DEALLOC(node);
    return 1;
}

LEAFTYPE
radix_get(struct ROOTSTRUCT * tree, LEAFTYPE leaf, EXTRA_ARG aux) {
    LEAFTYPE result;
    struct _internal_node * node;
    uint32_t dir;

    if (tree->leafcount == 0) return NO_LEAF;
    if (tree->leafcount == 1) {
        result = tree->root.leaf;
        if (COMPARE(result, leaf) == -1) return result;
        else return NO_LEAF;
    } /* root points to a node */

    node = tree->root.node;
    while (1) {
        dir = DECIDE(leaf, node->critbit, aux);
        if (IS_LEAF(node, dir)) {
            result = node->child[dir].leaf;
            break;
        } else {
            node = node->child[dir].node;
        }
    }

    if (COMPARE(result, leaf) == -1) return result;
    else return NO_LEAF;
}

LEAFTYPE radix_add(struct ROOTSTRUCT * tree, LEAFTYPE leaf, EXTRA_ARG aux, int should_replace, int* error) 
{
    LEAFTYPE result;
    struct _internal_node * node, * parent, * child;
    uint32_t dir, dir2;
    int32_t critbit;

    *error = 0;
    if (tree->leafcount == 0) {
        tree->root.leaf = leaf;
        tree->leafcount ++;

        return NO_LEAF;
    } else if (tree->leafcount == 1) {
        result = tree->root.leaf;
        critbit = COMPARE(leaf, result);
        if (critbit == -1) {
            if (should_replace) {
                node->child[dir].leaf = leaf;
                return result;
            } else return leaf;
        } else {
            node = (struct _internal_node *) ALLOC(sizeof (struct _internal_node));
            if (node == NULL) {
                *error = -1;
                return NO_LEAF;
            } else {
                node->critbit = critbit;
                dir = DECIDE(leaf, critbit, aux);
                node->child[dir].leaf = leaf;
                SET_LEAF(node, dir);
            }
            result = tree->root.leaf;
            node->child[1 - dir].leaf = tree->root.leaf;
            SET_LEAF(node, 1 - dir);
            tree->root.node = node;
            tree->leafcount ++;
            return NO_LEAF;
        }
    } /* else */
    node = tree->root.node;
    while (1) {
        dir = DECIDE(leaf, node->critbit, aux);
        if (IS_LEAF(node, dir)) {
            result = node->child[dir].leaf;
            break;
        } else {
            node = node->child[dir].node;
        }
    }
    critbit = COMPARE(leaf, result);
    if (critbit == -1) {
        if (should_replace) {
            node->child[dir].leaf = leaf;
            return result;
        } else return leaf;
    } else {
        node = (struct _internal_node *) ALLOC(sizeof (struct _internal_node));
        if (node == NULL) {
            *error = -1;
            return NO_LEAF;
        } else {
            node->critbit = critbit;
            dir = DECIDE(leaf, critbit, aux);
            node->child[dir].leaf = leaf;
            SET_LEAF(node, dir);
        }
        child = tree->root.node;
        if (node->critbit < child->critbit) {
            node->child[1 - dir].node = child;
            SET_NODE(node, 1 - dir);
            tree->root.node = node;
        } else while (1) {
                parent = child;
                dir2 = DECIDE(leaf, parent->critbit, aux);
                if (IS_LEAF(parent, dir2)) {
                    result = parent->child[dir2].leaf;
                    //node->child[1 - dir].leaf = tree->root.leaf;
                    node->child[1 - dir].leaf = parent->child[dir2].leaf;
                    SET_LEAF(node, 1 - dir);
                    parent->child[dir2].node = node;
                    SET_NODE(parent, dir2);
					break;
                } else {
                    child = parent->child[dir2].node;
                    if (node->critbit < child->critbit) {
                        node->child[1 - dir].node = child;
                        SET_NODE(node, 1 - dir);
                        parent->child[dir2].node = node;
                        SET_NODE(parent, dir2);

                        break;
                    }
                }
            }
        tree->leafcount ++;
        return NO_LEAF;
    }
}

LEAFTYPE
radix_del(struct ROOTSTRUCT * tree, LEAFTYPE leaf, EXTRA_ARG aux) {
    LEAFTYPE result;
    struct _internal_node * node, * parent;
    uint32_t dir, dir2;

    if (tree->leafcount == 0) return NO_LEAF;
    if (tree->leafcount == 1) {
        result = tree->root.leaf;
        if (COMPARE(result, leaf) == -1) {
            tree->root.leaf = NO_LEAF;
            tree->leafcount --;
            return result;
        } else return NO_LEAF;
    } /* else */
    node = tree->root.node;
    while (1) {
        dir = DECIDE(leaf, node->critbit, aux);
        if (IS_LEAF(node, dir)) {
            result = node->child[dir].leaf;
            break;
        } else {
            node = node->child[dir].node;
        }
    }
    if (COMPARE(result, leaf) == -1) {
        parent = tree->root.node;
        while (1) {
            dir2 = DECIDE(leaf, parent->critbit, aux);
            if (parent->child[dir2].node == node) break;
            else parent = parent->child[dir2].node;
        }
        if (IS_LEAF(node, 1 - dir)) {
            parent->child[dir2].leaf = node->child[1 - dir].leaf;
            SET_LEAF(parent, dir2);
        } else {
            parent->child[dir2].node = node->child[1 - dir].node;
        }
        tree->leafcount --;
        DEALLOC(node);
        return result;
    } else return NO_LEAF;
}

void
radix_fwd(struct ROOTSTRUCT * tree, int (*cb)(LEAFTYPE)) {
    if (tree->leafcount == 0) return;
    if (tree->leafcount == 1) {
        cb(tree->root.leaf);
        return;
    } /* else */
    _dfs(tree->root.node, cb, NULL, 0, 0);
}

void
radix_rev(struct ROOTSTRUCT * tree, int (*cb)(LEAFTYPE)) {
    if (tree->leafcount == 0) return;
    if (tree->leafcount == 1) {
        cb(tree->root.leaf);
        return;
    } /* else */
    _dfs(tree->root.node, cb, NULL, 1, 0);
}

struct ROOTSTRUCT *
radix_new(void) {
    struct ROOTSTRUCT * result;

    result = (struct ROOTSTRUCT *) ALLOC(sizeof (struct ROOTSTRUCT));
    if (result != NULL) result->leafcount = 0;

    return result;
}

void
radix_end(struct ROOTSTRUCT * tree, int (*cb)(LEAFTYPE)) {
    if (! tree->leafcount == 0) {
        if (tree->leafcount == 1) {
            cb(tree->root.leaf);
        } else {
            _dfs(tree->root.node, cb, _dealloc_internal_node, 0, 1);
        }
    }
    DEALLOC(tree);
}

