#include "parser.h"

int transferMoney(int accountNumber1, int accountNumber2, int amount)
{
    FILE* oldDbFile;
    char line[256];
    char *token, *rest;
    int dbAccountNumber;
    int foundNumber1, foundNumber2, foundBalance1, foundBalance2;
    foundNumber1 = foundNumber2 = foundBalance1 = foundBalance2 = 0;

    oldDbFile = fopen(DATABASEFILE, "r");
    if(!oldDbFile)
    {
        err_exit("fopen()");
    }

    if(flock(fileno(oldDbFile), LOCK_EX) == -1)
    {
        err_exit("flock()");
    }

    while(fgets(line, sizeof(line), oldDbFile))
    {
        rest = line;
        token = strtok_r(rest, " ", &rest);
        dbAccountNumber = (int)strtol(token, NULL, 10);
        if (dbAccountNumber == accountNumber1)
        {
            foundNumber1 = dbAccountNumber;
        }
        else if(dbAccountNumber == accountNumber2)
        {
            foundNumber2 = dbAccountNumber;
        }
    }
    if(flock(fileno(oldDbFile), LOCK_UN) == -1)
    {
        err_exit("flock()");
    }
    fclose(oldDbFile);

    if(foundNumber1 && foundNumber2)
    {
        depositOrWithdraw(foundNumber1, amount, 0);
        depositOrWithdraw(foundNumber2, amount, 1);
        return 0;
    }
    else
    {
        printf("Did not find both of the accounts from the database!\n");
    }

   return -1;
}


// Choice == 0 ----> Withdraw
// Choice == 1 ----> Deposit
int depositOrWithdraw(int accountNumber, int amount, int choice)
{
    FILE* oldDbFile, *newDbFile;
    char line[256];
    char *token, *rest;
    int dbAccountNumber, dbAccountBalance, newAccountBalance, foundAccount;

    foundAccount = 0;
    oldDbFile = fopen(DATABASEFILE, "r");
    if(!oldDbFile)
    {
        err_exit("fopen()");
    }

    if(flock(fileno(oldDbFile), LOCK_EX) == -1)
    {
        err_exit("flock()");
    }

    newDbFile = fopen("newDatabase.txt", "a");
    if(!newDbFile)
    {
        err_exit("fopen()");
    }
    if(flock(fileno(newDbFile), LOCK_EX) == -1)
    {
        err_exit("flock()");
    }
    while(fgets(line, sizeof(line), oldDbFile))
    {
        rest = line;
        token = strtok_r(rest, " ", &rest);
        dbAccountNumber = (int)strtol(token, NULL, 10);
        dbAccountBalance = (int)strtol(rest, NULL, 10);
        if(dbAccountNumber == accountNumber)
        {
            foundAccount = 1;
            if(choice == 0)
            {
                newAccountBalance = dbAccountBalance - amount;
                fprintf(newDbFile, "%d %d\n", dbAccountNumber, newAccountBalance);
            }
            else if(choice == 1)
            {
                newAccountBalance = dbAccountBalance + amount;
                fprintf(newDbFile, "%d %d\n", dbAccountNumber, newAccountBalance);
            }
        }
        else
        {
            fprintf(newDbFile, "%d %d\n", dbAccountNumber, dbAccountBalance);
        }
    }

    // TODO: Maybe add mutex here, uncertain what will happen here with concurreny.
    //       If this unlocks, then other thread gets the lock on this file and
    //       our thread removes the file.....
    if(flock(fileno(oldDbFile), LOCK_UN) == -1)
    {
        err_exit("flock()");
    }
    fclose(oldDbFile);
    if(unlink(DATABASEFILE) == -1 && errno != ENOENT)
    {
        err_exit("unlink()");
    }

    if(rename("newDatabase.txt", DATABASEFILE) == -1)
    {
        err_exit("rename");
    }

    if(flock(fileno(newDbFile), LOCK_UN) == -1)
    {
        err_exit("flock()");
    }

    fclose(newDbFile);
    return foundAccount;
}

int getAccountBalance(int accountNumber)
{
    FILE* dbFile;
    char line[256];
    char *token, *rest;
    int dbAccountNumber, dbAccountBalance;

    dbFile = fopen(DATABASEFILE, "r");
    if(!dbFile)
    {
        err_exit("fopen()");
    }

    if(flock(fileno(dbFile), LOCK_EX) == -1)
    {
        err_exit("flock()");
    }

    while(fgets(line, sizeof(line), dbFile))
    {
        rest = line;
        token = strtok_r(rest, " ", &rest);
        dbAccountNumber = (int)strtol(token, NULL, 10);
        dbAccountBalance = (int)strtol(rest, NULL, 10);
        if(dbAccountNumber == accountNumber)
        {
            if(flock(fileno(dbFile), LOCK_UN) == -1)
            {
                err_exit("flock()");
            }
            return dbAccountBalance;
        }
    }
    if(flock(fileno(dbFile), LOCK_UN) == -1)
    {
        err_exit("flock()");
    }
    printf("Did not find account with number %d\n", accountNumber);
    fclose(dbFile);
    return -1;
}

int executeCommand(char* command)
{
    char *commandSpecifier, *token;
    char* rest = command;
    int i, accountNumber1, accountNumber2, transferSum, balance1;
    i = accountNumber1 = accountNumber2 = transferSum = 0;

    // Lets use strtok_r to tokenize the string!
    while((token = strtok_r(rest, " ", &rest)))
    {
        // First lets save all the relevant data into variables.
        // Currently we do not care whether the user gives extra arguments.
        if(i == 0)
        {
            commandSpecifier = token;
        }
        else if(i == 1)
        {
            accountNumber1 = (int)strtol(token, NULL, 10);
        }
        else if(i == 2)
        {
            if(commandSpecifier[0] == 'w' || commandSpecifier[0] == 'd')
            {
                transferSum = (int)strtol(token, NULL, 10);
            }
            else if(commandSpecifier[0] == 't')
            {
                accountNumber2 = (int)strtol(token, NULL, 10);
            }
        }
        else if(i == 3)
        {
            if(commandSpecifier[0] == 't')
            {
                transferSum = (int)strtol(token, NULL, 10);
            }
        }
        i++;
    }


    // This part executes Database commands based on the first char of the command.
    if(commandSpecifier[0] == 'l')
    {
        if(accountNumber1 > 0)
        {
            balance1 = getAccountBalance(accountNumber1);
            if(balance1 != -1)
            {
                printf("Balance for account %d is: %d\n", accountNumber1, balance1);
            }
        }
        else
        {
            printf("Search failed: Accounts are numbered from 1-N\n");
            return -1;
        }
    }
    else if(commandSpecifier[0] == 'w')
    {
        if(transferSum <= 0)
        {
            printf("Transaction failed: Can't withdraw amount less or equal than 0\n");
            return -1;
        }
        else
        {
            if(depositOrWithdraw(accountNumber1, transferSum, 0))
            {
                printf("Succesfully withdrew %d dollars from account %d\n", transferSum, accountNumber1);
            }
            else
            {
                printf("Did not find account with number %d\n", accountNumber1);
                return -1;
            }
        }
    }
    else if(commandSpecifier[0] == 'd')
    {
       if(transferSum <= 0)
        {
            printf("Transaction failed: Can't deposit an amount less or equal than 0\n");
            return -1;
        }
        else
        {
            if(depositOrWithdraw(accountNumber1, transferSum, 1))
            {
                printf("Succesfully withdrew %d dollars from account %d\n", transferSum, accountNumber1);
            }
            else
            {
                printf("Did not find account with number %d\n", accountNumber1);
                return -1;
            }
        }
    }
    else if(commandSpecifier[0] == 't')
    {
        if(transferSum <= 0)
        {
            printf("Transaction failed: Can't transfer an amount less or equal than 0\n");
            return -1;
        }
        else
        {
            if(transferMoney(accountNumber1, accountNumber2, transferSum) == -1)
            {
                printf("Transaction failed!\n");
                return -2;
            }
            else
            {
                printf("Succesfully transfered %d dollars from account %d to account %d\n", transferSum, accountNumber1, accountNumber2);
            }
        }
    }
    else
    {
        printf("Command failed: Client gave wrong command identifier!\n");
        return -1;
    }

    return 0;
}
