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

    //限速
    unsigned int bw_upload_rate_max;
    unsigned int bw_download_rate_max;
    long bw_transfer_star_sec;
    long bw_transfer_star_usec;
    
    // 父子进程通信
    int parent_fd;
    int child_fd;
    
    //数据通道连接标志位
    int data_process;

    struct sockaddr_in* port_addr;
    int pasv_listen_fd;
    int data_fd;

    uid_t uid;
    int is_ascii;
    long long restart_pos;
    char *rnfr_name;
} session_t;

void begin_session(session_t *sess);

#endif