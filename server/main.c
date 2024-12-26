#include "parser.h"
#include "shell.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>


#define PORT                    (8080)
#define MAX_CONCURRENT_CLIENTS  (100) 

char *users[] = {
    "User1", 
    "User2", 
    "User3", 
    "test"
};

char *passwords[] = {
    "User1-password", 
    "User2-password", 
    "User3-password", 
    "test"
};


/*
 * main.c
 * A simple Command Line Parser.
 * Author : Michael Roberts <mroberts@it.net.au>
 * Last Modification : 11/09/01
 * Modified 11/9/01 by Nick Nelissen, who speparated the main into a separate source
 *	file
 */


int init_server(struct sockaddr_in *address) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CONCURRENT_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}


bool login(int client_socket_fd) {
    char username[100];
    char password[100];
    int num_users = sizeof(users) / sizeof(users[0]);

    read(client_socket_fd, username, sizeof(username));
    read(client_socket_fd, password, sizeof(password));
    
    for (int i = 0; i < num_users; i++) {
        if (strcmp(username, users[i]) == 0 && strcmp(password, passwords[i]) == 0) {
            char res[] = "login successful\n";
            write(client_socket_fd, res, sizeof(res));
            return true;
        }
    }

    char res[] = "login failed, try again\n";
    write(client_socket_fd, res, sizeof(res));
    return false;
}


void* client_handler(void* argv) {
    int client_socket = *((int*) argv);
    while (login(client_socket) == false);
    run_server_remote_shell(client_socket);
    close(client_socket);
    return NULL;
}


void ignore_sig(int sig) {
    if (sig == SIGQUIT) {               // CTRL-\ signal

    }
    else if (sig == SIGINT) {           // CTRL-C signal
#ifdef DEBUG
        exit(0);
#endif // DEBUG
    }
    else if (sig == SIGTSTP) {          // CTRL-Z signal

    }
}


int main(void) {
    signal(SIGQUIT, ignore_sig);
    signal(SIGINT, ignore_sig);
    signal(SIGTSTP, ignore_sig);
    
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int server_fd = init_server(&address);

    while (1) {
        int client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_socket < 0) {
            perror("accept error\n");
            continue;
        }

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, &client_socket);
    }

    close(server_fd);
    return 0;
}                       /*End of main() */
