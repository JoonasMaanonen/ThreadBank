#include <sys/un.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include "errorHandler.h"

#define BUFSIZE 100
#define SOCKETPATH "/tmp/threadBank"

int main(int argc, char *argv[])
{

    int pid, cliFd, numRead, numWrote, readSize;
    char buf[BUFSIZE];
    char serverReply[BUFSIZE];
    struct sockaddr_un addr;

    cliFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(cliFd == -1)
    {
        errExit("socket()");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKETPATH, sizeof(addr.sun_path) - 1);

    if(connect(cliFd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
    {
        errExit("connect()");
    }

    switch(pid = fork())
    {
        case -1:
            errExit("fork()");
        case 0:
            while((numRead = read(STDIN_FILENO, buf, BUFSIZE)) > 0)
            {
                numWrote = write(cliFd, buf, numRead);
                if(numWrote != numRead)
                {
                    errExit("partial/failed write");
                }
                if(numRead == -1)
                {
                    errExit("read()");
                }
            }
        default:
            while(1)
            {
                if((readSize = read(cliFd, serverReply, sizeof(serverReply))) == -1)
                {
                    errExit("read()");
                }

                printf("%s", serverReply);
                memset(&serverReply, 0, sizeof(serverReply));
            }
    }

    close(cliFd);
    exit(EXIT_SUCCESS);
}

