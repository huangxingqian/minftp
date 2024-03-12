#include "session.h"
#include "common.h"
#include "privparent.h"
#include "ftpproto.h"

void begin_session(session_t *sess)
{
    struct passwd *pw = getpwnam("nobody");
    if (pw == NULL)
    {
        return;
    }
    
    if (setegid(pw->pw_gid) < 0)
        ERR_EXIT("setegid");
    if (seteuid(pw->pw_uid) < 0)
        ERR_EXIT("seteuid");
    

    int sockfd[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockfd) < 0)
        ERR_EXIT("socketpair");
    
    pid_t pid;
    pid = fork();

    if (pid < 0)
        ERR_EXIT("fork");

    if (pid == 0)
    {
        // ftp
        close(sockfd[0]);
        handle_child(sess);
    }
    else
    {
        // nobody
        close(sockfd[1]);
        handle_parent(sess);
    }

}