#include "shell.h"

#define  ESC   (27) 
#define  DEL   (127) 


static void display_clear(int num_chars) {
    printf("\033[0m");
    for (int i = 0; i < num_chars; i++) {
        printf("\b");
    }
    for (int i = 0; i < num_chars; i++) {
        printf(" ");
    }
    for (int i = 0; i < num_chars; i++) {
        printf("\b");
    }
}


static void display_cmd(char *cmd, int cursor, bool newline) {
    printf("\033[0m");
    int i;
    for (i = 0; cmd[i]; i++) {
        if (i == cursor && !newline) {
            printf("\033[1;47;30m");
        }
        printf("%c", cmd[i]);
        if (i == cursor) {
            printf("\033[0m");
        }
    }

    if (cursor == i && !newline) {
        printf("\033[1;47;30m");
    }
    printf(" \033[0m");
    if (newline) {
        printf("\r\n");
    }
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


void disable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}


void enable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}


static char *get_server_response(int server_socket_fd) {
    char *res = malloc(sizeof(char));
    int res_len = 0;
    int res_cap = 1;
    int read_size;
    *res = '\0';
    
    do {
        res_cap += 1024;
        res = realloc(res, res_cap * sizeof(char));
        read_size = read(server_socket_fd, res + res_len, 1024);
        res_len += read_size;
    } while (res[res_len - 1] != PROTOCOL_RES_END && read_size > 0);
    
    res[res_len] != '\0';
    return res;
}


char *get_user_cmd_str(char *prompt, int server_socket_fd) {
    char ch;
    int cmd_cap = 100;
    int cmd_len = 0;
    int displayed_cmd_len = 0;
    int cmd_cursor_idx = 0;
    int history_idx = -1;
    char *history_cmd = NULL;
    char *cmd = malloc((cmd_cap + 1) * sizeof(char));
    cmd[0] = '\0';

    struct termios org_termios;
    tcgetattr(STDIN_FILENO, &org_termios);
    
    struct termios raw_termios = org_termios;
    cfmakeraw(&raw_termios);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw_termios);
    printf("\e[?25l");          // hide terminal built-in cursor

    printf("%s  ", prompt);
    do {
        display_clear(displayed_cmd_len + 1);
        if (history_idx == -1) {
            displayed_cmd_len = cmd_len;
            display_cmd(cmd, cmd_cursor_idx, false);
        }
        else {
            displayed_cmd_len = strlen(history_cmd);
            display_cmd(history_cmd, displayed_cmd_len, false);
        }

        if (cmd_len >= cmd_cap) {
            cmd_cap *= 2;
            cmd = realloc(cmd, (cmd_cap + 1) * sizeof(char));
        }

        ch = getchar();
        if (ch == ESC) {
            ch = getchar();
            if (ch != '[') {
                continue;
            }

            ch = getchar();
            if ((ch < 'A' || ch > 'D') && ch != '3') {
                ungetc(ch, stdin);
                continue;
            }

            
            if (ch == 'C') {
                if (history_idx != -1) {
                    printf("\a");
                }
                else {
                    if (cmd_cursor_idx < cmd_len) {
                        cmd_cursor_idx++;
                    }
                    else {
                        printf("\a");
                    }
                }
            } 
            else if (ch == 'D') {
                if (history_idx != -1) {
                    free(cmd);
                    cmd = history_cmd;
                    cmd_len = strlen(cmd);
                    cmd_cap = cmd_len;
                    cmd_cursor_idx = cmd_len;
                    history_idx = -1;
                }

                if (cmd_cursor_idx > 0) {
                    cmd_cursor_idx--;
                }
                else {
                    printf("\a");
                }
            }
            
            // TODO: navigate history ('A' for up - 'B' for down)
            else if (ch == 'A') {
                history_idx++;
                char idx_str[15];
                sprintf(idx_str, "%d", history_idx);
                protocol_cmd_type cmd_type = PROTOCOL_CMD_READ_FROM_HISTORY;
                send(server_socket_fd, &cmd_type, sizeof(protocol_cmd_type), 0);
                send(server_socket_fd, idx_str, strlen(idx_str) + 1, 0);
                
                char *res = get_server_response(server_socket_fd);
                if (*res != '\0') {
                    free(history_cmd);
                    history_cmd = res;
                }
                else {
                    history_idx--;
                    printf("\a");
                }
            }
            else if (ch == 'B') {
                history_idx--;
                char idx_str[15];
                sprintf(idx_str, "%d", history_idx);
                protocol_cmd_type cmd_type = PROTOCOL_CMD_READ_FROM_HISTORY;
                send(server_socket_fd, &cmd_type, sizeof(protocol_cmd_type), 0);
                send(server_socket_fd, idx_str, strlen(idx_str) + 1, 0);
                
                char *res = get_server_response(server_socket_fd);
                if (*res != '\0') {
                    free(history_cmd);
                    history_cmd = res;
                }
                else {
                    history_idx = -1;
                    printf("\a");
                }
            }

            // TODO: handle delete key
            else if (ch == '3') {
                if (history_idx != -1) {
                    ungetc(ch, stdin);
                    printf("\a");
                }
                else {
                    ch = getchar();
                    if (ch == '~') {
                        if (cmd_cursor_idx < cmd_len) {
                            for (int i = cmd_cursor_idx; i < cmd_len; i++) {
                                cmd[i] = cmd[i + 1];
                            }
                            cmd_len--;
                        }
                        else {
                            printf("\a");
                        }
                    }
                    else {
                        ungetc('~', stdin);
                        ungetc('3', stdin);
                    }
                }
            }
        }
        else if (history_idx != -1) {
            free(cmd);
            cmd = strdup(history_cmd);
            cmd_len = strlen(cmd);
            cmd_cap = cmd_len;
            cmd_cursor_idx = cmd_len;
            history_idx = -1;
            if (ch != '\n' && ch != '\r') {
                ungetc(ch, stdin);
            }
        }
        else if (ch == DEL) {
            if (cmd_cursor_idx > 0) {
                for (int i = cmd_cursor_idx; i <= cmd_len; i++) {
                    cmd[i - 1] = cmd[i];
                }
                cmd_len--;
                cmd_cursor_idx--;
            }
            else {
                printf("\a");
            }
        }
        else if (!iscntrl(ch)) {
            for (int i = cmd_len; i >= cmd_cursor_idx; i--) {
                cmd[i + 1] = cmd[i];
            }
            cmd[cmd_cursor_idx] = ch;
            cmd_cursor_idx++;
            cmd_len++;
        }
        else {
            printf("\a");
        }

        cmd[cmd_len] = '\0';
    } while (ch != '\n' && ch != '\r');

    display_clear(displayed_cmd_len + 1);
    display_cmd(cmd, cmd_cursor_idx, true);
    tcsetattr(STDIN_FILENO, TCSANOW, &org_termios);
    printf("\e[?25h");          // re-show terminal built-in cursor
    return cmd;
}


void run_client_remote_shell(int server_socket_fd) {
    char buffer[1024] = { 0 };
    bool exit = false;
    char *res;
    
    char *prompt = malloc(2 * sizeof(char));
    strcpy(prompt, "%");

    signal(SIGQUIT, ignore_sig);
    signal(SIGINT, ignore_sig);
    signal(SIGTSTP, ignore_sig);
    disable_echo();
    setbuf(stdout, NULL);

    while (!exit) {
        char *cmd = get_user_cmd_str(prompt, server_socket_fd);
        if (cmd == NULL) {
            continue;
        }
        
        if (strcmp(cmd, "exit") == 0) {
            exit = true;
            protocol_cmd_type cmd_type = PROTOCOL_CMD_EXIT;
            send(server_socket_fd, &cmd_type, sizeof(protocol_cmd_type), 0);
        }
        else if (strncmp(cmd, "prompt", strlen("prompt")) == 0) {
            free(prompt);
            prompt = strdup(cmd + strlen("prompt") + 1);
        }
        else {
            protocol_cmd_type cmd_type = PROTOCOL_CMD_EXECUTE_SHELL_CMD;
            send(server_socket_fd, &cmd_type, sizeof(protocol_cmd_type), 0);
            send(server_socket_fd, cmd, strlen(cmd) + 1, 0);
            
            char *res = get_server_response(server_socket_fd);
            printf("%s", res);
            free(res);
        }
        
        free(cmd);
    }

    enable_echo();
}
