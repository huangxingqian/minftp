#include "sysutil.h"

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

int accept_timeout(int fd, struct sockaddr_in *addr, socklen_t addrlen, unsigned int wait_seconds)
{
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
