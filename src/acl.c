
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "acl.h"
#include "radix.h"

acl_tree_t* acl_new(void) {
    return radix_new();
}

static int dummy_free_leaf(acl_rule_t *rule) {
	free(rule);
    return 1;
}

void acl_free(acl_tree_t *tree) {
    radix_end(tree, dummy_free_leaf);
}

int acl_add_rule(acl_tree_t *tree, acl_rule_t *rule) {
    int error;

    radix_add(tree, rule, 0, 0, &error);

    if (error == -1) return 0;
    else return 1;
}

static uint32_t parse_mask(char *str)
{
    uint32_t mask;
    int shift;

    if (strchr(str, '.')) {
        inet_pton(AF_INET, str, &mask);
        return ntohl(mask);
    } else {
        shift = atoi(str);
        return (0xFFFFFFFF << (32 - shift));
    }
}

static ipv4_cidr_t parse_cidr(char *str)
{
    ipv4_cidr_t cidr;
	uint32_t ip, mask;
    char *p = NULL;

    if (strcmp(str, "any") == 0) {
        cidr.ip = 0;
        cidr.mask = 0;
        return cidr;
    }

    p = strchr(str, '/');
    if (!p) {
        mask = 0xFFFFFFFF;
    } else {
    	*p = '\0';
    	mask = parse_mask(p + 1);
	}

    inet_pton(AF_INET, str, &ip);
	cidr.ip = ntohl(ip) & mask;
	cidr.mask = mask;

    return cidr;
}

int acl_add_rule2(acl_tree_t *tree, int action, char *ipstr)
{
    acl_rule_t *rule = NULL;

	rule = (acl_rule_t *)malloc(sizeof(acl_rule_t));
	if (!rule) {
		return 0;
	}

    rule->cidr = parse_cidr(ipstr);
    rule->action = action;

    return acl_add_rule(tree, rule);
}

int acl_lookup(acl_tree_t *tree, uint32_t ip)
{
    acl_rule_t rule, *r;

	rule.cidr.ip = ip;
    rule.cidr.mask = 0xFFFFFFFF;

    r = radix_get(tree, &rule, 0);

    return (r && r->action);
}

int acl_del_rule(acl_tree_t *tree, acl_rule_t *rule) {
    radix_del(tree, rule, 0);

    return 0;
}

void acl_foreach(acl_tree_t *tree, int (*cb)(acl_rule_t)) {
    radix_fwd(tree, cb);
}


