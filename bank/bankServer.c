#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "errorHandler.h"
#include "parser.h"

typedef struct {
    pthread_t threadId;
    int deskNumber;
} Thread;
Thread *tPtr; //  Array of Thread structures dynamically allocated.

#define MAXNDESKS 10
#define BUFSIZE 100
/* NOTE: tmp directory can be a security concern in real-world apps.*/
#define SOCKETPATH "/tmp/threadBank"

// Stuff shared between threads. Protect access to all of this with a mutex!
int cliFds[MAXNDESKS]; // List that stores the client socket fds.
int iget, iput; // Tells the thread which index has the new connection FD.
int occupiedDesks = 0; // This is used to check if all of our threads are already executing.
pthread_mutex_t clifdMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clifdCond = PTHREAD_COND_INITIALIZER;

static int serviceTheClient(int socketFd)
{
    printf("Service for client %d\n", socketFd);
    char buf[BUFSIZE];
    char copyBuff[BUFSIZE];
    ssize_t numRead, numWrote;
    while(1)
    {
        if((numRead = read(socketFd, buf, BUFSIZE)) == 0)
        {
            // Connection was closed
            printf("Connection was closed by the client\n");
            if(close(socketFd))
            {
                printf("Failed to close socket: %d\n", socketFd);
                err_exit("close()");
            }
            printf("Closed the socket %d\n", socketFd);
            break;
        }
        else
        {
            if(numRead == -1)
            {
                err_exit("read()");
            }

            // Take this copy since execute command touches the memory of buf.
            strncpy(copyBuff, buf, sizeof(buf));
            if(executeCommand(buf) < 0)
            {
                strcpy(copyBuff, "Executing the command failed for some reason. Check server logs!\n");
            }
            else
            {
                strcpy(copyBuff, "The command was succesfull!\n");
            }

            numWrote = write(socketFd, copyBuff, sizeof(copyBuff));
            if(numWrote == -1)
            {
                err_exit("write()");
            }
            memset(&buf, 0, sizeof(buf));
            memset(&copyBuff, 0, sizeof(copyBuff));
        }
    }
    return 0;
}

static void* threadExecute(void* arg)
{
    int connFd;
    int deskNumber = *((int*) arg);
    pthread_t threadId = pthread_self();
    pthread_detach(threadId);
    printf("Thread %d started executing with ID: %ld\n", deskNumber, threadId);
    while(1)
    {
        pthread_mutex_lock(&clifdMutex);
        while(iget == iput)
        {
            printf("Thread %d went to sleep!\n", deskNumber);
            pthread_cond_wait(&clifdCond, &clifdMutex);
        }
        printf("Thread %d woke up!\n", deskNumber);
        connFd = cliFds[iget];
        if(++iget == MAXNDESKS)
        {
            iget = 0;
        }
        pthread_mutex_unlock(&clifdMutex);
        serviceTheClient(connFd);
        pthread_mutex_lock(&clifdMutex);
        occupiedDesks--;
        pthread_mutex_unlock(&clifdMutex);
    }
    return NULL;
}

int main(int argc, char *argv[])
{

    int masterFd, newClientFd, nThreads;
    struct sockaddr_un addr;

    // Argument handling
    if (argc < 2 || strcmp(argv[1], "--help") == 0)
    {
        fprintf(stderr, "Usage: %s amountOfServiceDesks\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    else if (argc > 2)
    {
        fprintf(stderr, "You gave too many arguments! Only give one!\n");
        exit(EXIT_FAILURE);
    }

    /* Note: We assume that the user does not give value bigger than MAXINT,
     * which is like 2 billion something. This assumption makes the cast
     * work as intended and not chop of the excess bits. */
    nThreads = (int) strtol(argv[1], NULL, 10);
    if (nThreads > MAXNDESKS)
    {
        fprintf(stderr, "Our bank has maximum number of %d service desks.", MAXNDESKS);
        exit(EXIT_FAILURE);
    }

    // Create a new socket
    masterFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(masterFd == -1)
    {
        errExit("socket()");
    }

    // Let's remove the existing socket if we haven't cleaned up properly.
    if(unlink(SOCKETPATH) == -1 && errno != ENOENT)
    {
        errExit("unlink()");
    }

    // Setup the address struct
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKETPATH, sizeof(addr.sun_path) - 1);

    // Bind the socket to use that address
    if(bind(masterFd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
    {
        errExit("bind()");
    }

    // The backlog argument should not matter in our case, since we're not
    // expecting a lot of traffic simultaneosly and we can process the new
    // connection really fast.
    if(listen(masterFd, 5) == -1)
    {
        errExit("listen()");
    }
    tPtr = calloc(nThreads, sizeof(Thread));

    iget = iput = 0;
    printf("Creating %d threads for the service desks.\n", nThreads);
    for(int i = 0; i < nThreads; i++)
    {
        tPtr[i].deskNumber = i;
        if (pthread_create(&tPtr[i].threadId, NULL, &threadExecute, (void *)&tPtr[i].deskNumber) != 0)
        {
            err_exit("pthread_create()");
        }
    }

    /*Main thread continues executing here*/
    printf("Waiting for new customers..\n");
    for(;;)
    {
        newClientFd = accept(masterFd, NULL, NULL);
        if(newClientFd == -1)
        {
            errExit("accept()");
        }
        printf("New connection established with socketfd: %d\n", newClientFd);
        // LOCK HERE!
        pthread_mutex_lock(&clifdMutex);
        printf("Main thread got the mutex!\n");
        if(occupiedDesks < nThreads)
        {
            cliFds[iput] = newClientFd;
            if(++iput == MAXNDESKS)
            {
                iput = 0;
            }
            if(iput == iget)
            {
                printf("iput == iget == %d", iput);
                exit(EXIT_FAILURE);
            }

            printf("iput = %d, iget = %d\n", iput, iget);
            occupiedDesks++;
        }
        else
        {
            int wrote;
            printf("Queue not implemented yet, so drop this connection!\n");
            if((wrote = write(newClientFd, "The bank is full now\n", sizeof("The bank is full now\n"))) ==  -1)
            {
                err_exit("write()");
            }
            close(newClientFd);
        }

        // Wake up one of the threads
        pthread_cond_signal(&clifdCond);
        pthread_mutex_unlock(&clifdMutex);
        // UNLOCK HERE!
    }

    /* Cleanup all the resources!*/
    if(close(masterFd))
    {
        errExit("close() master socket");
    }

    if(unlink(SOCKETPATH) == -1 && errno != ENOENT)
    {
        errExit("unlink()");
    }

}
