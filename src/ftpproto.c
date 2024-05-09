#include "ftpproto.h"
#include "session.h"
#include "sysutil.h"

void handle_child(session_t *sess)
{
    int ret;
    writen(sess->ctrl_fd, "220 HelloWord\r\n", strlen("220 HelloWord\r\n"));
    while (1)
    {
        memset(sess->cmdline, 0, sizeof(sess->cmdline));
        memset(sess->cmd, 0,sizeof(sess->cmd));
        memset(sess->arg, 0,sizeof(sess->arg));

        //解析FTP命令和参数
        //处理FTP命令
        ret = readline(sess->ctrl_fd, sess->cmdline, MAX_COMMAND_LINE);
        if (ret == -1)
            ERR_EXIT("readline");
        else if (ret == 0)
            exit(EXIT_SUCCESS);
        
    }
    
}
