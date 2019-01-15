#ifndef PARSER_H
#define PARSER_H
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <sys/file.h>
#include "errorHandler.h"


#define DATABASEFILE "bankDatabase.txt"

typedef struct {
    int accountNumber;
    int accountBalance;
} Account;

int depositOrWithdraw(int accountNumber, int amount, int choice);

int executeCommand(char* command);
void listAccountBalance(int accountNumber, int balance);

int getAccountBalance();

#endif
