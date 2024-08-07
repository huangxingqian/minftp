#include "sysutil.h"


int getlocalip(char *ip)
{
    char host[100] = {0};
    if (gethostname(host,sizeof(host)) < 0)
        return -1;
    struct hostent *hp;
    if ((hp = gethostbyname(host)) == NULL)  
        return -1;
    
    strcpy(ip, inet_ntoa(*(struct in_addr*)hp->h_addr));
    return 0;
}

int tcp_client(unsigned short port)
{
  int sockfd;
  if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
    ERR_EXIT("client socket");
  }
  if (port > 0) {
    int on = 1;
    if ((setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&on,sizeof(on))) < 0)
      ERR_EXIT("sersockopt");
      
      struct sockaddr_in localaddr;
      localaddr.sin_family = AF_INET;
      localaddr.sin_port = htons(port);
      localaddr.sin_addr.s_addr = inet_addr("10.58.14.133");
      
      if(bind(sockfd,(struct sockaddr*)&localaddr,sizeof(localaddr)) < 0)
        ERR_EXIT("bind");
  }
  return sockfd;
}
/**
 * 
*/
int tcp_server(const char *host, unsigned short port)
{
	int listenfd;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		ERR_EXIT("socket");

	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;

	if (host != NULL)
	{
		if (inet_aton(host, &servaddr.sin_addr) == 0)
		{

			struct hostent *hp;
			hp = gethostbyname(host);
			if (hp == NULL)
				ERR_EXIT("gethostbyname");

			servaddr.sin_addr = *(struct in_addr*)hp->h_addr;
		}
	}
	else
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	servaddr.sin_port = htons(port);

	int on = 1;
		ERR_EXIT("setsockopt");

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		ERR_EXIT("bind");
	
	if (listen(listenfd, SOMAXCONN) < 0)
		ERR_EXIT("listen");

	return listenfd;
}

ssize_t readn(int fd, void *buf, size_t count)
{
	ssize_t nleft = count;
	ssize_t nread;
	char *bufp =(char*)buf;

	while (nleft > 0)
	{
		if ((nread = read(fd, bufp, nleft)) < 0)
		{
			if (errno == EINTR) //信号中断
				continue;
			return -1;
		}
		else if (nread == 0) //对方关闭套接字
			return count - nleft; 

		bufp += nread;
		nleft -= nread;

	}
	return count;
}

ssize_t recv_peek(int sockfd, void *buf, ssize_t len)
{
	while(1)
	{
		int ret = recv(sockfd, buf, len, MSG_PEEK);
		if (ret == -1 && errno == EINTR)
			continue;
		return ret;
	}
}

ssize_t readline(int sockfd, void *buf, ssize_t maxline)
{
	int ret;
	int nread;
	char *bufp = buf;
	int nleft= maxline;
	while(1)
	{
		ret = recv_peek(sockfd, bufp, nleft);
		if (ret < 0)
			return ret;
		else if(ret == 0)
			return ret;
		
		nread = ret;
		int i;
		for(i = 0; i < nread; i++)
		{
			if(bufp[i]=='\n')
			{
				ret = readn(sockfd, bufp, i+1);
				if (ret != i+1) 
					exit(EXIT_FAILURE);
				return ret;
			}
		}

		if (nread > nleft) 
			exit(EXIT_FAILURE);

		nleft -= nread;
		ret = readn(sockfd, bufp, nread);
		if (ret != nread) 
			exit(EXIT_FAILURE);
		bufp += nread;
	}
}

ssize_t writen(int fd, const void *buf, size_t count)
{
	ssize_t nleft = count;
        ssize_t nwrite;
        char *bufp =(char*)buf;

        while (nleft > 0)
        {
                if ((nwrite = write(fd, bufp, nleft)) < 0)
                {
                        if (errno == EINTR) //信号中断
                                continue;
                        return -1;
                }
                else if (nwrite == 0)
                        continue;

                bufp += nwrite;
                nleft -= nwrite;

        }
        return count;
}

int read_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if (wait_seconds > 0) {
		fd_set read_fdset;
		FD_ZERO(&read_fdset);
		FD_SET(fd, &read_fdset);

		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
                timeout.tv_usec = 0;

		do
		{
			ret = select(fd+1, &read_fdset, NULL, NULL, &timeout);
		} while (ret < 0 && errno == EINTR);

		if (ret == 0) {
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret == 1)
			ret = 0;

	}
	return ret;
}

int write_timeout(int fd, unsigned int wait_seconds)
{
	int ret = 0;
	if (wait_seconds > 0) {
		fd_set write_fdset;
		FD_ZERO(&write_fdset);
		FD_SET(fd,&write_fdset);

		struct timeval timeout;
		timeout.tv_sec = wait_seconds;
		timeout.tv_usec = 0;
		
		do
		{
			ret = select(fd+1, NULL, &write_fdset, NULL, &timeout);
		} while(ret < 0 && errno == EINTR);
		
		if (ret == 0) {
			ret = -1;
			errno = ETIMEDOUT;
		} else if (ret == 1)
			ret = 0;

	}
	return ret;
}

int accept_timeout(int fd, struct sockaddr_in *addr, unsigned int wait_seconds)
{
	socklen_t addrlen = sizeof(struct sockaddr_in);
	int ret;
	if (wait_seconds > 0) {
		fd_set accept_fdset;
                FD_ZERO(&accept_fdset);
                FD_SET(fd,&accept_fdset);

		struct timeval timeout;
                timeout.tv_sec = wait_seconds;
                timeout.tv_usec = 0;
		do
                {
                        ret = select(fd+1, &accept_fdset, NULL, NULL, &timeout);
                } while(ret < 0 && errno == EINTR);
		
		if (ret == -1)
			return -1;
		else if (ret == 0) {
			errno = ETIMEDOUT;
			return -1;
		}
	}

	if (addr != NULL)
		ret = accept(fd, (struct sockaddr*)addr, &addrlen);
	else
		ret = accept(fd, NULL, NULL);
	
	if (ret == -1)
		ERR_EXIT("accept");
	
	return ret;

}

void activate_nonblock(int fd)
{
	int ret;
	int flags;
	flags = fcntl(fd, F_GETFL);
	
	if (flags == -1)
		ERR_EXIT("fcntl");
	flags |= O_NONBLOCK;
	ret = fcntl(fd, F_SETFL, flags);
	if (ret == -1)
		ERR_EXIT("fcntl");
}

void deactivate_nonblock(int fd)
{
        int ret;
        int flags;
        flags = fcntl(fd, F_GETFL);

        if (flags == -1)
                ERR_EXIT("fcntl");
        flags &= ~O_NONBLOCK;
        ret = fcntl(fd, F_SETFL, flags);
        if (ret == -1)
                ERR_EXIT("fcntl");
}


int connect_timeout(int fd, struct sockaddr_in *addr, socklen_t addrlen,unsigned int wait_seconds)
{
	int ret;
	if (wait_seconds > 0)
		activate_nonblock(fd);
	
	ret = connect(fd, (struct sockaddr*)addr, addrlen);
	if (ret < 0 && errno == EINPROGRESS) {
		fd_set connect_fdset;
                FD_ZERO(&connect_fdset);
                FD_SET(fd,&connect_fdset);

                struct timeval timeout;
                timeout.tv_sec = wait_seconds;
                timeout.tv_usec = 0;
                do
                {
                        ret = select(fd+1,  NULL, &connect_fdset,NULL, &timeout);
                } while(ret < 0 && errno == EINTR);

		if (ret == 0) {
			errno = ETIMEDOUT;
			return -1;
		} else if( ret < 0)
			return -1;
		else if (ret == 1) 
		{
			int err;
			socklen_t socklen = sizeof(err);
			int sockoptret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &socklen);
			if (sockoptret == -1)
				return -1;
			if (err == 0)
				ret = 0;
			else
			{
				errno = err;
				ret = -1;
			}
		}
	}
	if (wait_seconds > 0)
		deactivate_nonblock(fd);

	return ret;
}

void send_fd(int sockfd, int fd)
{
  int ret;
  struct msghdr msg;
  struct cmsghdr *p_cmsg;
  struct iovec vec;
  char cmsgbuf[CMSG_SPACE(sizeof(fd))];
  int *p_fds;
  char sendchar=0;
  msg.msg_control=cmsgbuf;
  msg.msg_controllen=sizeof(cmsgbuf);
  
  p_cmsg=CMSG_FIRSTHDR(&msg);
  p_cmsg->cmsg_level=SOL_SOCKET;
  p_cmsg->cmsg_type=SCM_RIGHTS;
  p_cmsg->cmsg_len=CMSG_LEN(sizeof(fd));
  p_fds=(int*)CMSG_DATA(p_cmsg);
  *p_fds=fd;
  
  msg.msg_name=NULL;
  msg.msg_namelen=0;
  msg.msg_iov=&vec;
  msg.msg_iovlen=1;
  msg.msg_flags=0;
  
  vec.iov_base=&sendchar;
  vec.iov_len=sizeof(sendchar);
  ret = sendmsg(sockfd, &msg, 0);
  if (ret!=1) {
    ERR_EXIT("sendmsg");
  }
}

int recv_fd(int sockfd)
{
  int ret;
  struct msghdr msg;
  char recvchar=0;
  struct iovec vec;
  int recvfd;
  char cmsgbuf[CMSG_SPACE(sizeof(recvfd))];
  int *p_fd;
  struct cmsghdr *p_cmsg;
  
  vec.iov_base=&recvchar;
  vec.iov_len=sizeof(recvchar);
  
  msg.msg_control=cmsgbuf;
  msg.msg_controllen=sizeof(cmsgbuf);
  msg.msg_name=NULL;
  msg.msg_namelen=0;
  msg.msg_iov=&vec;
  msg.msg_iovlen=1;
  msg.msg_flags=0;
  
  p_fd=(int*)CMSG_DATA(CMSG_FIRSTHDR(&msg));
  *p_fd=-1;
  ret = recvmsg(sockfd, &msg, 0);
  if (ret!=1) {
    ERR_EXIT("recvmsg");
  }
  
  p_cmsg=CMSG_FIRSTHDR(&msg);
  if (p_cmsg==NULL) {
    ERR_EXIT("no passed fd");
  }
  
  p_fd=(int*)CMSG_DATA(p_cmsg);
  recvfd=*p_fd;
  if (recvfd==-1) {
    ERR_EXIT("no passed fd");
  }
  return recvfd;
}