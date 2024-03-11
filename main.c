#include "common.h"

int main(int argc, char* argv[])
{
    if (getuid() != 0)
    {
        fprintf(stderr,"must be as root!\n");
        exit(EXIT_FAILURE);
    }
    
    
    int listenfd = tcp_server(NULL,5188);

    return 0;
}