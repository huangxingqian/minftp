#ifndef _SESSION_H_
#define _SESSION_H_
#include "common.h"

typedef struct session
{
    int ctrl_fd;
    // 控制连接
    char cmdline[MAX_COMMAND_LINE];
    char cmd[MAX_COMMAND];
    char arg[MAX_ARG];

    // 父子进程通信
    int parent_fd;
    int child_fd;

    struct sockaddr_in *port_addr;
    int data_fd;

    uid_t uid;
    int is_ascii;
} session_t;

void begin_session(session_t *sess);

#endif