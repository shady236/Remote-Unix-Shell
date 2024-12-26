#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "shell.h"


#define PORT                    (8080)
#define SERVER_ADDRESS          ("127.0.0.1") 


/*
 * main.c
 * A simple Command Line Parser.
 * Author : Michael Roberts <mroberts@it.net.au>
 * Last Modification : 11/09/01
 * Modified 11/9/01 by Nick Nelissen, who speparated the main into a separate source
 *	file
 */


int connect_to_server() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0); 
    if (client_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr) < 0) {
        fprintf(stderr, "server address %s not valid", SERVER_ADDRESS);
        exit(EXIT_FAILURE);
    }

    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    return client_fd;
}


bool login(int server_socket_fd) {
    char username[100];
    char password[100];
    int len;
    
    printf("Username: ");
    fgets(username, sizeof(username) - 1, stdin);
    len = strlen(username);
    username[len - 1] = 0;      // delete '\n'
    write(server_socket_fd, username, len);
    
    printf("Password: ");
    fgets(password, sizeof(password) - 1, stdin);
    len = strlen(password);
    password[len - 1] = 0;      // delete '\n'
    write(server_socket_fd, password, len);
    
    char res[100];
    read(server_socket_fd, res, sizeof(res));
    printf("%s", res);
    
    return strcmp(res, "login successful\n") == 0;
}


int main(void) {
    int client_fd = connect_to_server();
    while (login(client_fd) == false);
    run_client_remote_shell(client_fd);
    close(client_fd);
    return 0;
}                       /*End of main() */
