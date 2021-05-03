# File-Transfer-Client-Server

## server1: iterative
## server2: concurrent
Client can request one or more files to server. Have fun!

# Usage:
Build it in your source folder using: <br>

gcc -std=gnu99 -o server server1/*.c *.c -Iserver1 -lpthread -lm <br>
gcc -std=gnu99 -o client client1/*.c *.c -Iclient1 -lpthread -lm <br>
gcc -std=gnu99 -o server server2/*.c *.c -Iserver2 -lpthread -lm <br>

Execute server as: <br>
./server <dest_port> 

Execute client as: <br>
./client <dest_address> <dest_port> <filename> [filename ... ]