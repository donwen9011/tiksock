
#ifndef _RADIX_H_
#define _RADIX_H_

/*
typedef struct ipv4_cidr {
    uint32_t ip;
    uint32_t mask;
}ipv4_cidr_t;

typedef struct ipv4_radix ipv4_radix_t;

ipv4_radix_t* ipv4_radix_new(void);

void ipv4_radix_free(ipv4_radix_t *tree);

int ipv4_radix_add(ipv4_radix_t *tree, ipv4_cidr_t cidr);

int ipv4_radix_lookup(ipv4_radix_t *tree, ipv4_cidr_t cidr);

int ipv4_radix_del(ipv4_radix_t *tree, ipv4_cidr_t cidr);

void ipv4_radix_each(ipv4_radix_t *tree, int (*cb)(ipv4_cidr_t));
*/

LEAFTYPE radix_get(struct ROOTSTRUCT * tree, LEAFTYPE leaf, EXTRA_ARG aux);
 
struct ROOTSTRUCT*  radix_new(void);



#endif
