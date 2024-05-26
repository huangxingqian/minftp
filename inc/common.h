#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>         
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <netdb.h>
#include <pwd.h>
#include <ctype.h>
#include <shadow.h>
#include <crypt.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>


#define MAX_COMMAND_LINE 1024
#define MAX_COMMAND 32
#define MAX_ARG 1024

#define ERR_EXIT(m)\
	do\
	{	\
		perror(m);\
		exit(EXIT_FAILURE);\
	} while(0)

#endif
