#include "common.h"
#include "session.h"
#include "sysutil.h"
#include "parseconf.h"
#include "tunable.h"
#include "ftpproto.h"

int main(int argc, char* argv[])
{
    signal(SIGCHLD,SIG_IGN);
    int listenfd = –1;
    int conn;
    pid_t pid;
    //初始化会话变量
    session_t sess = 
    {
        -1, "", "", "",

        -1, -1,
        
        NULL,-1,-1,

        0,0,0,NULL
    };
    
    //需要root用户来运行
    if (getuid() != 0) {
        fprintf(stderr,"must be as root!\n");
        exit(EXIT_FAILURE);
    }
    
    //加载配置文件
    parseconf_load_file("minftpd.conf");
    
    //创建侦听套接字
    listenfd = tcp_server(NULL, 5188);
    if (listenfd < 0) {
        fprintf(stderr,"create listenfd failed.\n");
        exit(EXIT_FAILURE);
    }
    
    while (1) {
        //等待客户端连接
        conn = accept_timeout(listenfd, NULL, 0);
        if (conn == 1)
            ERR_EXIT("accept_timeout");
        
        pid = fork();
        if (pid == -1)
            ERR_EXIT("fork");

        if (pid == 0)
        {
            sess.ctrl_fd = conn;
            begin_session(&sess);
        }
        else
            close(conn);
    }

    return 0;
}