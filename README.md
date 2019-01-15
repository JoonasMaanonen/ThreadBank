# ThreadBank
This course project implements a simple bank using C.
The goal of this project was to practice threading and different client server infrastructures.
Therefore the bank itself is not very advanced but the focus was on the backend Linux system programming.

## Usage
- The bank server is in the bank dir

- Run it ./bankServer <amountOfThreads>, max threads is 10 defined in the code.

- The client app is on client dir run it ./bankClient
    - Then it connects to the UNIX domain socket defined by bankServer.
    - There are 4 types of commands you can give:
        - l accountNumber
        - w accountNumber amountToWithdraw
        - d accountNumber amountToDeposit
        - t accountNumber1 accountNumber2 amountToTransfer
