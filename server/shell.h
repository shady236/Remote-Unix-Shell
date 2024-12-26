#ifndef  SHELL_H_
#define  SHELL_H_


#define _GNU_SOURCE
#include <errno.h>
#include "parser.h"
#include "../common/protocol.h"
#include <glob.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/socket.h>


#define  HISTORY_SIZE   (1000) 


void run_server_remote_shell(int client_socket_fd);


#endif   // SHELL_H_