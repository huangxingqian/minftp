#include "ftpproto.h"
#include "session.h"
#include "sysutil.h"

static void ftp_reply(session_t *sess, int status,  const char *text);
static void do_user(session_t *sess);
static void do_pass(session_t *sess);
static void do_feat(session_t *sess);
static void do_syst(session_t *sess);
static void do_cwd(session_t *sess);
static void do_cdup(session_t *sess);
static void do_quit(session_t *sess);
static void do_port(session_t *sess);
static void do_pasv(session_t *sess);
static void do_type(session_t *sess);
static void do_stru(session_t *sess);
static void do_node(session_t *sess);
static void do_retr(session_t *sess);
static void do_stor(session_t *sess);
static void do_appe(session_t *sess);
static void do_list(session_t *sess);
static void do_nlst(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_nkd(session_t *sess);
static void do_rnd(session_t *sess);
static void do_dele(session_t *sess);
static void do_rnfr(session_t *sess);
static void do_rnto(session_t *sess);
static void do_site(session_t *sess);
static void do_size(session_t *sess);
static void do_stat(session_t *sess);
static void do_noop(session_t *sess);
static void do_help(session_t *sess);

typedef struct ftpcmd
{
    const char *cmd;
    void (*cmd_handler)(session_t *sess);
}ftpcmd_t;

ftpcmd_t ctrl_cmds[] = 
{
    {"USER", do_user},
    {"PASS", do_pass},
    {"FEAT", do_feat}
};


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

        int i;
        int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
        for (i = 0; i < size; i++)
        {
            if (strcmp(ctrl_cmds[i].cmd, sess->arg) == 0)
            {
                if (ctrl_cmds[i].cmd_handler != NULL)
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else
                {
                    ftp_reply(sess, 502, "Unimplement command.");
                }
            }
            
        }

        if (i == size)
        {
            ftp_reply(sess, 500, "unknown command.");
        }
        
    }
    
}

static void ftp_reply(session_t *sess, int status, const char *text)
{
    char buf[1024] = {0};
    sprintf(buf,"%d %s\r\n", status, text);
    writen(sess->ctrl_fd, buf, strlen(buf));
}

  static void ftp_lreply(session_t *sess, int status, const char *text)
 {
     char buf[1024] = {0};
     sprintf(buf, "%d-%s\r\n",status, text);
     writen(sess->ctrl_fd, buf, strlen(buf));
 }

static void do_user(session_t *sess)
{
    struct passwd *pw = getpwnam(sess->arg);
    if (pw == NULL)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    sess->uid = pw->pw_uid;
    ftp_reply(sess, 331, "Please specify password");  
}

static void do_pass(session_t *sess)
{
    struct passwd *pw = getpwuid(sess->uid);
    if (pw == NULL)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    
    struct spwd *sp = getspnam(pw->pw_name);
    if (sp == NULL)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    
    char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
    if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
    {
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    ftp_reply(sess, 230, "Login successful");
}

static void do_feat(session_t *sess)
{
    ftp_lreply(sess, 211, "Features:");
    writen(sess->ctrl_fd,"EPRT\r\n", strlen("EPRT\r\n"));
    writen(sess->ctrl_fd,"EPSU\r\n", strlen("EPSU"));
    writen(sess->ctrl_fd,"MDTM\r\n", strlen("MDTM\r\n")); 
    writen(sess->ctrl_fd,"PASU\r\n", strlen("PASU\r\n"));
    writen(sess->ctrl_fd,"REST STREAM\r\n", strlen("REST STREAM\r\n"));
    writen(sess->ctrl_fd,"SIZE\r\n",strlen("SIZE\r\n")); 
    writen(sess->ctrl_fd,"TUFS\r\n",strlen("TUFS\r\n")); 
    writen(sess->ctrl_fd,"UTF8\r\n",strlen("UTF8\r\n")); 
    ftp_reply(sess, 211, "End");
}

static void do_syst(session_t *sess)
{
   ftp_reply(sess, 215, "SYS TYPE");
}

static void do_cwd(session_t *sess)
{
}
static void do_cdup(session_t *sess)
{
}
static void do_quit(session_t *sess)
{
}
static void do_port(session_t *sess)
{
}
static void do_pasv(session_t *sess)
{
}
void do_type(session_t *sess)
{
    if(strcmp(sess->arg,"A") == 0)
    {
        ftp_reply(sess, 200,"switching to ASCII mode");
        sess->is_ascii = 1;
    }
    else if(strcmp(sess->arg,"B") == 0)
    {
        sess->is_ascii = 0;
        ftp_reply(sess, 200,"switching to Binary mode");
    }
    else
    {
        ftp_reply(sess, 500,"Unrecognized  TYPE command");
    }
}
static void do_stru(session_t *sess)
{
}
static void do_node(session_t *sess)
{
}
static void do_retr(session_t *sess)
{
}
static void do_stor(session_t *sess)
{
}
static void do_appe(session_t *sess)
{
}
static void do_list(session_t *sess)
{
}
static void do_nlst(session_t *sess)
{
}
static void do_rest(session_t *sess)
{
}
static void do_abor(session_t *sess)
{
}
static void do_pwd(session_t *sess)
{
}
static void do_nkd(session_t *sess)
{
}
static void do_rnd(session_t *sess)
{
}
static void do_dele(session_t *sess)
{
}
static void do_rnfr(session_t *sess)
{
}
static void do_rnto(session_t *sess)
{
}
static void do_site(session_t *sess)
{
}
static void do_size(session_t *sess)
{
}
static void do_stat(session_t *sess)
{
}
static void do_noop(session_t *sess)
{
}
static void do_help(session_t *sess)
{
}
