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
int get_transfer_fd(session_t *sess);
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
static void do_nlist(session_t *sess);
static void do_rest(session_t *sess);
static void do_abor(session_t *sess);
static void do_pwd(session_t *sess);
static void do_mkd(session_t *sess);
static void do_rmd(session_t *sess);
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
    {"LIST",do_list},
    {"SYST",do_syst},
    {"CDUP",do_cdup},
    {"QUIT",do_quit},
    {"STRU",do_stru},
    {"NODE",do_node},
    {"RETR",do_retr},
    {"APPE",do_appe},
    {"REST",do_rest},
    {"NLIST",do_nlist},
    {"ABOR",do_abor},
    {"MKD",do_mkd},
    {"RMD",do_rmd},
    {"DELE",do_dele},
    {"RNFR",do_rnfr},
    {"RNTO",do_rnto},
    {"SITE",do_site},
    {"SIZE",do_size},
    {"STAT",do_stat},
    {"NOOP",do_noop},
    {"HELP",do_help},
    {"STOR",do_stor},
    {"CWD",do_cwd}

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
        printf("DEBUG: cmd = %s, arg = %s.\n", sess->cmd, sess->arg);
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
                    printf("DEBUG: Unimplement command. cmd = %s.\n", sess->cmd);
                    ftp_reply(sess, 502, "Unimplement command.");
                }
                break;
            }
            
        }

        if (i == size)
        {
            printf("DEBUG: Unknown command. cmd = %s.\n", sess->cmd);
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
        printf("DEBUG: Directory listing is NULL.\n");
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
    if (pw == NULL) {
        printf("DEBUG: Unknown User. User name = %s.\n", sess->arg);
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
        printf("DEBUG: Get spwd failed. pw_name = %s.\n", pw->pw_name);
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    
    char *encrypted_pass = crypt(sess->arg, sp->sp_pwdp);
    if (strcmp(encrypted_pass, sp->sp_pwdp) != 0)
    {
        printf("DEBUG: Password camper failed.\n");
        ftp_reply(sess, 530, "Login incorrect");
        return;
    }
    
    umask(tunable_local_umask);
    //部分用户权限过低，暂时屏蔽
    //setgid(pw->pw_gid);
    //setuid(pw->pw_uid);
    //chdir(pw->pw_dir);
    
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
    //依据arg改变目录
    if(chdir(sess->arg) < 0) {
        ftp_reply(sess, 550, "Filled to change directory.");
        return;
    }
    ftp_reply(sess, 250, "Directory successful changed.");
}
static void do_cdup(session_t *sess)
{
    //返回上一层目录
    if(chdir("..") < 0) {
        ftp_reply(sess, 550, "Filled to change directory.");
        return;
    }
    ftp_reply(sess, 250, "Directory successful changed.");
}
static void do_quit(session_t *sess)
{
}
static void do_port(session_t *sess)
{
    //获取主动模式发送IP和PORT
    unsigned int v[6] = {0};
    sscanf(sess->arg,"%u,%u,%u,%u,%u,%u",&v[0],&v[1],&v[2],&v[3],&v[4],&v[5]);
    sess->port_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    memset(sess->port_addr, 0, sizeof(struct sockaddr_in));
    sess->port_addr->sin_family = AF_INET;
    //网络字节序使用大端字节序
    //数据MSB存在低地址内存处
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
  
  //getlocalip(ip);
  
  priv_sock_send_cmd(sess->child_fd,PRIV_SOCK_PRIV_LISTEN);
  unsigned short port = priv_sock_get_int(sess->child_fd);
  
  unsigned int v[4];
  sscanf(ip,"%u.%u.%u.%u",&v[0],&v[1],&v[2],&v[3]);
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
    if (get_transfer_fd(sess) == 0) {
        ftp_reply(sess,425,"Use port or pasv frist");
        return;
    }
    long long offset = sess->restart_pos;
    sess->restart_pos = 0;
    //打开文件
    int fd = open(sess->arg, O_RDONLY);
    if (fd < 0) {
        ftp_reply(sess, 550,"Failed to open file.");
        return;
    }
    
    //加读的锁
    int ret;
    ret = local_file_read(fd);
    if (ret == -1) {
        ftp_reply(sess, 550,"Failed to open file.");
        return;
    }
    
    //判断是否为普通文件
    struct stat sbuf;
    ret = fstat(fd, &sbuf);
    if (!S_ISREG(sbuf.st_mode)) {
        ftp_reply(sess, 550,"Failed to open file.");
        return;
    }
    
    if (offset != 0) {
        ret = lseek(fd, offset, SEEK_SET);
        if (ret ==  -1) {
            ftp_reply(sess, 550,"Failed to open file.");
            return;
        }
    }
    
    char text[2048] = {0};
    if (sess->is_ascii) {
        sprintf(text, "Opening ASCII mode data connection for %s (%lld bytes)",
            sess->arg, (long long)sbuf.st_size);
    }
    else {
        sprintf(text, "Opening BINARY mode data connection for %s (%lld bytes)",
            sess->arg, (long long)sbuf.st_size);
    }
    
    ftp_reply(sess, 150, text);
    
    int flag = 0;
    /*
    //下载文件
    char buf[4096] = {0};
    while (1) {
        ret = readn(fd, buf, sizeof(buf));
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                flag = 1;
                break;
            }
        } else if (ret == 0) {
            flag = 0;
            break;
        }
        
        if (writen(sess->data_fd, buf, ret) != ret) {
            break;
            flag = 2;
        }
    }
    */
    
    long long bytes_to_send = sbuf.st_size;
    if (offset > bytes_to_send) {
        bytes_to_send = 0;
    } else {
        bytes_to_send -= offset;
    }
    
    while (bytes_to_send) {
        int num_this_time = bytes_to_send > 4096 ? 4096 : bytes_to_send;
        ret = sendfile(sess->data_fd, fd, NULL, num_this_time);
        if (ret == -1) {
            flag = 2;
            break;
        }
        bytes_to_send -= ret;
    }
    
    if (bytes_to_send == 0) {
        flag = 0;
    }
    
    close(sess->data_fd);
    sess->data_fd = -1;
    if (flag == 0 ) {
        ftp_reply(sess, 226,"Transfer complete.");
    } else if (flag == 1) {
        ftp_reply(sess, 451,"Failure reading form local file.");
    } else if (flag == 2) {
        ftp_reply(sess, 426,"Failure writing to network stream.");
    }
}

void upload_common(session_t *sess, int mode)
{
    if (get_transfer_fd(sess) == 0) {
        ftp_reply(sess,425,"Use port or pasv frist");
        return;
    }
    long long offset = sess->restart_pos;
    sess->restart_pos = 0;
    //打开文件
    int fd = open(sess->arg, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        ftp_reply(sess, 553,"Could not create file.");
        return;
    }
    
    //加写的锁
    int ret;
    ret = local_file_write(fd);
    if (ret == -1) {
        ftp_reply(sess, 550,"Failed to open file.");
        return;
    }
    
    //判断是否为普通文件
    struct stat sbuf;
    ret = fstat(fd, &sbuf);
    if (!S_ISREG(sbuf.st_mode)) {
        ftp_reply(sess, 550,"Failed to open file.");
        return;
    }
    
    if (offset != 0) {
        ret = lseek(fd, offset, SEEK_SET);
        if (ret ==  -1) {
            ftp_reply(sess, 550,"Failed to open file.");
            return;
        }
    }
    
    char text[2048] = {0};
    if (sess->is_ascii) {
        sprintf(text, "Opening ASCII mode data connection for %s (%lld bytes)",
            sess->arg, (long long)sbuf.st_size);
    }
    else {
        sprintf(text, "Opening BINARY mode data connection for %s (%lld bytes)",
            sess->arg, (long long)sbuf.st_size);
    }
    
    ftp_reply(sess, 150, text);
    
    int flag = 0;
    /*
    //下载文件
    char buf[4096] = {0};
    while (1) {
        ret = readn(fd, buf, sizeof(buf));
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                flag = 1;
                break;
            }
        } else if (ret == 0) {
            flag = 0;
            break;
        }
        
        if (writen(sess->data_fd, buf, ret) != ret) {
            break;
            flag = 2;
        }
    }
    */
    
    long long bytes_to_send = sbuf.st_size;
    if (offset > bytes_to_send) {
        bytes_to_send = 0;
    } else {
        bytes_to_send -= offset;
    }
    
    while (bytes_to_send) {
        int num_this_time = bytes_to_send > 4096 ? 4096 : bytes_to_send;
        ret = sendfile(sess->data_fd, fd, NULL, num_this_time);
        if (ret == -1) {
            flag = 2;
            break;
        }
        bytes_to_send -= ret;
    }
    
    if (bytes_to_send == 0) {
        flag = 0;
    }
    
    close(sess->data_fd);
    sess->data_fd = -1;
    if (flag == 0 ) {
        ftp_reply(sess, 226,"Transfer complete.");
    } else if (flag == 1) {
        ftp_reply(sess, 451,"Failure reading form local file.");
    } else if (flag == 2) {
        ftp_reply(sess, 426,"Failure writing to network stream.");
    }
}


static void do_stor(session_t *sess)
{
    upload_common(sess, 0);
}
static void do_appe(session_t *sess)
{
    upload_common(sess, 1);
}

int port_active(session_t *sess)
{
  if (sess->port_addr != NULL) {
    if (pasv_active(sess))
      return 0;
      
    printf("DEBUG: Enter port mode.\n");
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
  priv_sock_send_buf(sess->child_fd, ip, strlen(ip));
  char res = priv_sock_get_result(sess->child_fd);
  if (res == PRIV_SOCK_RESULT_BAD) {
    printf("ERROR: command PRIV_SOCK_GET_DATA_SOCK execute failed.");
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
    printf("ERROR: command PRIV_SOCK_PRIV_ACCEPT execute failed.");
    return 0;
  } else if (res == PRIV_SOCK_RESULT_OK) {
    sess->data_fd = priv_sock_recv_fd(sess->child_fd);
  }
  return 1;
}

int get_transfer_fd(session_t *sess)
{
  int ret=1;
  if (!port_active(sess) && !pasv_active(sess))
  {
    fprintf(stderr,"debug:No port or pasv mode.\n");
    return 0;
  }
  
  if (port_active(sess)) {
    if (get_port_fd(sess) == 0) {
      ret = 0;
    }
  }
  
  if(pasv_active(sess)) {
    if (get_pasv_fd(sess) == 0) {
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

static void do_nlist(session_t *sess)
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
    sess->restart_pos = str_to_longlong(sess->arg);
    char text[1024] = {0};
    sprintf(text, "Restart position accepted (%lld).", sess->restart_pos);
    ftp_reply(sess, 350, "Directory send OK.");
}
static void do_abor(session_t *sess)
{
}
static void do_pwd(session_t *sess)
{
    char text[2048] = {0};
    char dir[1024+1] = {0};
    getcwd(dir, 1024);
    sprintf(text, "\"%s\"",dir);
    ftp_reply(sess, 257, text);
}
static void do_mkd(session_t *sess)
{
  if(mkdir(sess->arg, 0777) < 0) {
      ftp_reply(sess, 550, "Create directory operation failed.");
      return;
  }
  
  char text[4096] = {0};
  if (sess->arg[0] == '/') {
      sprintf(text, "%s created.", sess->arg);
  } else {
     char dir[1024+1] = {0};
     getcwd(dir, 4096);
     if (dir[strlen(dir) - 1] == '/') {
         sprintf(text, "%s%s created.", dir,sess->arg);
     } else {
         sprintf(text, "%s/%s created.", dir,sess->arg);

     }
  }
  
  ftp_reply(sess, 257, text);
} 

static void do_rmd(session_t *sess)
{
  if(rmdir(sess->arg) < 0) {
      ftp_reply(sess, 550, "Remove directory operation failed.");
      return;
  }
  
  ftp_reply(sess, 250, "Remove directory operation successful.");
}
static void do_dele(session_t *sess)
{
  if(unlink(sess->arg) < 0) {
      ftp_reply(sess, 550, "Delete operation failed.");
      return;
  }
  
  ftp_reply(sess, 250, "Delete operation successful.");
}
static void do_rnfr(session_t *sess)
{
    sess->rnfr_name = (char *)malloc(strlen(sess->arg) + 1);
    memset(sess->rnfr_name, 0, strlen(sess->arg) + 1);
    strcpy(sess->rnfr_name,sess->arg);
    ftp_reply(sess, 350, "Ready for RNTO");
}
static void do_rnto(session_t *sess)
{
    if (sess->rnfr_name == NULL) {
        ftp_reply(sess, 503, "RNFR required frist.");
    }
    rename(sess->rnfr_name, sess->arg);
    ftp_reply(sess, 250, "Rename successful.");
    free(sess->rnfr_name);
    sess->rnfr_name = NULL;
}
static void do_site(session_t *sess)
{
}
static void do_size(session_t *sess)
{
    struct stat buf;
    if (stat(sess->arg, &buf) < 0) {
       ftp_reply(sess, 550, "Could not get file size.");
       return;
    }
    
    if (!S_ISREG(buf.st_mode)) {
       ftp_reply(sess, 550, "Could not get file size.");
       return;
    }
    char text[1024] = {0};
    sprintf(text, "%lld", (long long)buf.st_size);
    ftp_reply(sess, 213, text);
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
