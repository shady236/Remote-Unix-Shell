# A Simple Unix Shell 

## Overview 
A simple remote UNIX shell is designed and implemented with C programming language. The shell runs on a server that can handle multiple clients simultaneously. Communication protocol between client and server is stated in the document "Protocol Documentation".

The shell supports the following built-in functionalities: 

### 1. Reconfiguring shell prompt command ```prompt``` (default %)
The shell must have a shell built-in command prompt for changing the current prompt. 

Example, type the following command
```% prompt Shady$```
should change the shell prompt to Shady$.

### 2. ```pwd```
This command prints the current directory (also known as working directory) of the shell process.

### 3. Change Directory ```cd```
This command is similar to that provided by the Bash built-in command cd. In particular, typing the command without a path should set the current directory of the shell to the home directory of the user.

### 4. Wildcard characters
If a token contains wildcard characters * or ?, the wildcard characters in such a token indicate to the shell that the filename must be expanded. 

Example:
```% ls *.c```
may be expanded to ls ex1.c ex2.c ex3.c if there are three matching files ex1.c ex2.c ex3.c in the current directory.

### 5. Redirection of the standard input, standard output and standard error <, > and 2>

### 6. ```history``` command
The shell remembers the commands entered into the shell and the user can select one of the previous commands by any of the following: 
1. The user can display the past commands using shell builtin command ```history```.
2. The Up Arrow key and Down Arrow key. 
3. Repeat a previous command using the special character ! followed by the command number in the history (eg, !12 would repeat the 12th command). 
4. Repeat a previous command using the special character ! followed by the command prefix string (e.g. !xyz would repeat the last command that starts with the string xyz).

### 7. Shell pipeline |

### 8. Sequential job execution &&

### 9. Background job execution &

### 10. Signal ignore
The shell isn't terminated by CTRL-C, CTRL-\, or CTRL-Z signals.

### 11. Built-in command ```exit```
Use the built-in command ```exit``` to terminate the shell program.

### 12. Execution of external commands
The shell supports execution of any external commands.

## How to run
Open two or more terminals, one for the server and others are for clients. 

For the server, change directiory to ./server and use ```make``` command to build to server using provided Makefile, then run it using ```./server```. 

Do the same for client, change directiory to ./client and use ```make``` command to build to client using provided Makefile (you can build it only one, no need to build it in all client terminals), then run it using ```./client```. 

