#include "ftpproto.h"
#include "session.h"
#include "sysutil.h"
#include "str.h"
#include "common.h"
#include "tunable.h"
#include "privsock.h"


int list_common(session_t *sess, int detail);
static void ftp_reply(session_t *sess, int status,  const char *text);
int port_active(session_t *sess);
int pasv_active(session_t *sess);
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
    {"FEAT", do_feat},
    {"PWD",do_pwd},
    {"TYPE",do_type},
    {"PASV",do_pasv},
    {"PORT",do_port},
    {"LIST",do_list}
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
        str_trim_crlf(sess->cmdline);
        str_split(sess->cmdline, sess->cmd, sess->arg, ' ');
        int i;
        int size = sizeof(ctrl_cmds) / sizeof(ctrl_cmds[0]);
        for (i = 0; i < size; i++)
        {
            if (strcmp(ctrl_cmds[i].cmd, sess->cmd) == 0)
            {
                if (ctrl_cmds[i].cmd_handler != NULL)
                {
                    ctrl_cmds[i].cmd_handler(sess);
                }
                else
                {
                    ftp_reply(sess, 502, "Unimplement command.");
                }
                break;
            }
            
        }

        if (i == size)
        {
            ftp_reply(sess, 500, "unknown command.");
        }
        
    }
    
}

int list_common(session_t *sess, int detail)
{
    int off = 0;
    DIR *dir = opendir(".");
    if(dir == NULL)
    {
        return 0;
    }
    
    
    struct dirent *dt = NULL;
    struct stat sbuf = {0};
    char buf[1024] = {0};
    
    while((dt = readdir(dir)) != NULL)
    {
        if(lstat(dt->d_name, &sbuf) < 0 )
        {
            continue;
        }
        
        if (dt->d_name[0] == '.') {
            continue;
        }
        if (detail) {
            const char *perm = statbuf_get_perms(&sbuf);
            off = 0;
            off += sprintf(buf + off, "%s ", perm);
            off += sprintf(buf + off, "%3ld %-8d %-8d ",sbuf.st_nlink, sbuf.st_uid, sbuf.st_gid);
            off += sprintf(buf + off, "%8lu ", (unsigned long)sbuf.st_size);
        
            const char *datebuf= statbuf_get_date(&sbuf);
            off += sprintf(buf + off, "%s ", datebuf);
        
            if (S_ISLNK(sbuf.st_mode))
            {
                char tmp[124] = {0};
                readlink(dt->d_name, tmp, sizeof(tmp));
                sprintf(buf + off, "%s -> %s\r\n", dt->d_name, tmp);
            } else
            {
              sprintf(buf + off,"%s\r\n", dt->d_name);
            }
        } else {
          sprintf(buf, "%s\r\n", dt->d_name);
        }
        
        printf("%s",buf);
        writen(sess->data_fd,buf,strlen(buf));
    }
    closedir(dir);
    return 1;
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
  chdir(sess->arg);
  ftp_reply(sess, 250,"Directory successful changed.");
}
static void do_cdup(session_t *sess)
{
}
static void do_quit(session_t *sess)
{
}
static void do_port(session_t *sess)
{
    unsigned int v[6] = {0};
    sscanf(sess->arg,"%u,%u,%u,%u,%u,%u",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
    sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
    sess->port_addr->sin_family = AF_INET;
    unsigned char *p = (unsigned char*)&sess->port_addr->sin_port;
    p[0] = v[4];
    p[1] = v[5];
    
    p = (unsigned char *)&sess->port_addr->sin_addr;
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];  
    p[3] = v[3];
    ftp_reply(sess, 200,"PORT command successful.Consider using PASV");
}
static void do_pasv(session_t *sess)
{
  char ip[4] = {0};
  getlocalip(ip);
  
  priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PRIV_LISTEN);
  unsigned short port = priv_sock_get_int(sess->child_fd);
  
  unsigned int v[4];
  sscanf(ip,"%u.%u.%u.%u",&v[0],&v[1],&v[3],&v[4]);
  char text[1024] = {0};
  sprintf(text,"Entering pasv mode (%u,%u,%u,%u,%u,%u)",v[0],v[1],v[2],v[3],port>>8, port & 0xff);
  ftp_reply(sess,227,text);
}
void do_type(session_t *sess)
{
    if(strcmp(sess->arg,"A") == 0)
    {
        ftp_reply(sess, 200,"switching to ASCII mode");
        sess->is_ascii = 1;
    }
    else if(strcmp(sess->arg,"B") == 0 || strcmp(sess->arg,"I") == 0)
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

int port_active(session_t *sess)
{
  if (sess->port_addr != NULL) {
    if (pasv_active(sess))
      return 0;
    return 1;
  }
  return 0;
}

int pasv_active(session_t *sess)
{
  int active;
  priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PRIV_ACTIVE);
  active = priv_sock_get_int(sess->child_fd);
  if (active) {
    if (port_active(sess)) {
      fprintf(stderr,"botn port an pasv are active.");
      exit(EXIT_FAILURE);
    }
    return 1;
  }
  return 0;
}

int get_port_fd(session_t *sess)
{
  priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_GET_DATA_SOCK);
  unsigned short port = ntohs(sess->port_addr->sin_port);
  char *ip = inet_ntoa(sess->port_addr->sin_addr);
  priv_sock_send_int(sess->child_fd, (int)port);
  priv_sock_send_buf(sess->child_fd, ip, sizeof(ip));
  char res = priv_sock_get_result(sess->child_fd);
  if (res == PRIV_SOCK_RESULT_BAD) {
    return 0;
  } else if (res == PRIV_SOCK_RESULT_OK) {
    sess->data_fd = priv_sock_recv_fd(sess->child_fd);
  }
  return 1;
}

int get_pasv_fd(session_t *sess)
{
  priv_sock_send_cmd(sess->child_fd, PRIV_SOCK_PRIV_ACCEPT);
  int res = priv_sock_get_result(sess->child_fd);
  if (res == PRIV_SOCK_RESULT_BAD) {
    return 0;
  } else if (res == PRIV_SOCK_RESULT_OK) {
    sess->data_fd = priv_sock_recv_fd(sess->child_fd);
  }
  return 1;
}

int get_transfer_fd(session_t *sess)
{
  int fd;
  int ret=1;
  if (!port_active(sess) && !pasv_active(sess))
  {
    fprintf(stderr,"debug:No port or pasv mode.");
    return 0;
  }
  
  if (port_active(sess)) {
    if (get_port_fd(sess) == 0) {
      ret = 0;
    }
  }
  
  if(pasv_active(sess)) {
    if (get_pasv_fd() == 0) {
      ret = 0;
    }
  }
  
  if (sess->port_addr) {
    free(sess->port_addr);
    sess->port_addr = NULL;
  }
  
  return ret;
}


static void do_list(session_t *sess)
{
  if (get_transfer_fd(sess) == 0) {
    ftp_reply(sess,425,"Use port or pasv frist");
    return;
  }
  ftp_reply(sess,150,"Here comes the directory listing");
  
  list_common(sess, 1);
  close(sess->data_fd);
  ftp_reply(sess,226,"Directory send OK.");
}

static void do_nlst(session_t *sess)
{
  if (get_transfer_fd(sess) == 0) {
    ftp_reply(sess,425,"Use port or pasv frist");
    return;
  }
  ftp_reply(sess,150,"Here comes the directory listing");
  
  list_common(sess, 0);
  close(sess->data_fd);
  ftp_reply(sess,226,"Directory send OK.");
}

static void do_rest(session_t *sess)
{
}
static void do_abor(session_t *sess)
{
}
static void do_pwd(session_t *sess)
{
    char text[1024+1] = {0};
    char dir[1024+1] = {0};
    getcwd(dir, 1024);
    sprintf(text, "\"%s\"",dir);
    ftp_reply(sess, 257, text);
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
