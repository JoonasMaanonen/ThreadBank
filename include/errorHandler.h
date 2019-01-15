#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H


// Prints the error with perror, flushes stdout, but does not terminate program.
void errMsg(const char *msg);

// Prints the error with perror, flushes stdout and exits using exit().
void errExit(const char *msg);

// Prints the error, doesn't flush stdout and exits with _exit().
// This is useful for terminating child processes.
void err_exit(const char *msg);

#endif
