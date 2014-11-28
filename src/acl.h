

#ifndef _ACL_H_
#define _ACL_H_


typedef struct ipv4_cidr {
    uint32_t ip;
    uint32_t mask;
}ipv4_cidr_t;

typedef struct acl_rule {
    ipv4_cidr_t cidr;
    int action;
} acl_rule_t;

#define LEAFTYPE   acl_rule_t *
#define ROOTSTRUCT acl_tree
#define NO_LEAF    NULL
#define COMPARE    acl_rule_compare
#define DECIDE(n, b, dummy) (((n->cidr.ip) >> (31 - (b))) & 1)
#define EXTRA_ARG  uint32_t

/*
 *  * src : leaf node in the tree
 *   * dst : input
 *    */
static inline int32_t acl_rule_compare(acl_rule_t *src, acl_rule_t *dst) {
    uint32_t xor, mask, result;

printf("src ip = %u, dst ip = %u\n", src->cidr.ip, dst->cidr.ip);

    xor = src->cidr.ip ^ (dst->cidr.ip & src->cidr.mask);
printf(":::: 0.1\n");
    for (mask = 0x80000000, result = 0;
            result < 32 && ! (xor & mask);
            mask >>= 1, result ++);

printf(":::: 0.2 : %d\n", result);
    if (result == 32) return -1;
    else return result;
}


typedef struct ROOTSTRUCT acl_tree_t;

acl_tree_t* acl_tree_new(void);


#include "radix.h"

#endif


