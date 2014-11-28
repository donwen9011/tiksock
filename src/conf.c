
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "log.h"
#include "acl.h"

extern acl_tree_t *acl_db;
extern char *log_file;
extern char *log_level;
extern char *host;
extern int   port;
extern int   is_daemon;

struct dispatch_table {
    const char *name;
    void (*fun)(int, char**);
};
 
#define ADD_TO_DISPATCH_TABLE(name, fun) \
    static const struct dispatch_table entry_##fun \
        __attribute__((__section__("cmd_dispatch_table"))) = \
        { name, fun }

int process_cmd(int argc, char **argv)
{
    extern struct dispatch_table __start_cmd_dispatch_table;
    extern struct dispatch_table __stop_cmd_dispatch_table;
    struct dispatch_table *dt;
 
    char *cmd = argv[0];

    for (dt = &__start_cmd_dispatch_table; dt != &__stop_cmd_dispatch_table; dt++) {
        if (strcmp(cmd, dt->name) == 0) {
            printf("name: %s\n", dt->name);
            (*dt->fun)(argc - 1, argv + 1);
        }
    }
    return 0;
}

int load_conf(char *file)
{
    char line[1024];

    FILE *fd;
    char *argv[64], *pch;
    int i = 0;

    fd = fopen(file, "r");
    if (!fd) {
        printf("Couldn't load configuration file: %s\n", file);
        return 0;
    }

    while (fgets(line, 1024, fd)) {

        if (line[0] == '#')
            continue;

        i = 0;
        pch = strtok(line," \r\n");

        while (pch != NULL) {
            argv[i++] = pch;
            pch = strtok (NULL, " \r\n");
        }

        if (i > 0) {
            process_cmd(i, argv);
        }
    }

    return 1;
}

void cmd_host(int argc, char **argv)
{
    host = strdup(argv[0]);
}

ADD_TO_DISPATCH_TABLE("host", cmd_host);

void cmd_port(int argc, char **argv)
{
    port = atoi(argv[0]);
}

ADD_TO_DISPATCH_TABLE("port", cmd_port);

void cmd_log(int argc, char **argv)
{
    if (strcmp(argv[0], "file") == 0) 
        log_file = strdup(argv[1]);
    else if (strcmp(argv[0], "level") == 0)
        log_level = strdup(argv[1]);
}

ADD_TO_DISPATCH_TABLE("log", cmd_log);

void cmd_daemon(int argc, char **argv)
{
    if (strcmp(argv[0], "yes") == 0) 
        is_daemon = 1;
    else 
        is_daemon = 0;
}

ADD_TO_DISPATCH_TABLE("daemon", cmd_daemon);

void cmd_rule(int argc, char **argv)
{
    if (strcmp(argv[0], "pass") == 0)
        acl_add_rule2(acl_db, 1, argv[1]);
    else
        acl_add_rule2(acl_db, 0, argv[1]);
}

ADD_TO_DISPATCH_TABLE("rule", cmd_rule);
 



