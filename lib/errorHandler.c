#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

typedef enum {FALSE, TRUE} Boolean;


/* These are our basic error handling functions. The formatting could be made
 * a lot fancier but simple is sufficient for our case. Inspiration was took from
 * the Kerrisk Linux Programming Interface book. Also Note that these functions
 * don't really handle errors that do not  use errno for their error handling.*/

static void terminate(Boolean useNormalExit)
{
    if (useNormalExit)
    {
        exit(EXIT_FAILURE);
    }
    else
    {
        _exit(EXIT_FAILURE);
    }
}

static void printError(Boolean flushStdout, const char *msg)
{
    if (flushStdout)
    {
        fflush(stdout);
    }
    perror(msg);
    fflush(stderr);
}

void errMsg(const char *msg)
{
    printError(TRUE, msg);
}

void errExit(const char *msg)
{
    printError(TRUE, msg);
    terminate(TRUE);
}

void err_exit(const char *msg)
{
    printError(FALSE, msg);
    terminate(FALSE);
}
