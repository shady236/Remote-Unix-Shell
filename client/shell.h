#ifndef  SHELL_H_
#define  SHELL_H_


#define _GNU_SOURCE

#include "../common/protocol.h"
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>


#define  HISTORY_SIZE   (1000) 


void run_client_remote_shell(int server_socket_fd);


#endif   // SHELL_H_
