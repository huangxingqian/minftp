#ifndef _SYS_UTIL_H_
#define _SYS_UTIL_H_

#include "common.h"

int tcp_server(const char *host, unsigned short port);
int tcp_client(unsigned short port);
ssize_t readn(int fd, void *buf, size_t count);
ssize_t writen(int fd, const void *buf, size_t count);
int read_timeout(int fd, unsigned int wait_seconds);
int write_timeout(int fd, unsigned int wait_seconds);
int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds);
void activate_nonblock(int fd);
void deactivate_nonblock(int fd);
int connect_timeout(int fd, struct sockaddr_in *addr, socklen_t addrlen,unsigned int wait_seconds);
ssize_t readline(int sockfd, void *buf, ssize_t maxline);
int getlocalip(char *ip);

void send_fd(int sockfd, int fd);
int recv_fd(int sockfd);

const char *statbuf_get_perms(struct stat *sbuf);
const char *statbuf_get_date(struct stat *sbuf);
int lock_file_read(int fd);
int lock_file_write(int fd);
int unlock_file(int fd);

long get_time_sec(void);
long get_time_usec(void);
void nano_sleep(double seconds);
#endif