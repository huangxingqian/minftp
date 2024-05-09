#include "common.h"
#include "session.h"
#include "sysutil.h"

int main(int argc, char* argv[])
{
    if (getuid() != 0)
    {
        fprintf(stderr,"must be as root!\n");
        exit(EXIT_FAILURE);
    }
    
    session_t sess = 
    {
        -1, "", "", "",
        -1, -1
    };

    int listenfd = tcp_server(NULL, 5188);
    int conn;
    pid_t pid;

    while (1)
    {
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