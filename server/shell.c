#include "shell.h"


static void replace_substring(char **str, int from, int to, char *new_substring) {
    int old_substring_len = to - from + 1;
    int new_substring_len = strlen(new_substring);
    
    int old_str_len = strlen(*str);
    int new_str_len = old_str_len - old_substring_len + new_substring_len;
    
    if (new_str_len > old_str_len) {
        *str = realloc(*str, (new_str_len + 1) * sizeof(char));
    } 

    // strcat(*str, *str + to + 1);
    strcpy(*str + from + new_substring_len, *str + to + 1);
    strncpy(*str + from, new_substring, new_substring_len);
}


static bool exec_and_return(char *cmd_name, char **argv) {
    pid_t pid = fork();
    if (pid == -1) {
        return false;
    }
    else if (pid != 0) {                        // Parent process
        waitpid(pid, NULL, 0);
        return true;
    }
    else {
        execvp(cmd_name, argv);
        return false;
    }
}


static bool execute_external_cmd(command **cmd, int i, int client_socket_fd) {
    int pipe_fd[2] = {-1, -1};
    if (cmd[i]->pipe_to != 0 && cmd[i]->redirect_out == NULL) {
        if (pipe(pipe_fd) == -1) {
            char *res = "pipe error\n";
            send(client_socket_fd, res, sizeof(res), 0);
            return false;
        }
        cmd[cmd[i]->pipe_to]->pipe_fd[0] = pipe_fd[0];
        cmd[i]->pipe_fd[1] = pipe_fd[1];
    }

    pid_t pid = fork();
    if (pid == -1) {
        char *res = "fork error\n";
        send(client_socket_fd, res, sizeof(res), 0);
        return false;
    }
    else if (pid != 0) {                        // Parent process
        close(pipe_fd[1]);
        close(cmd[i]->pipe_fd[0]);

        if (!cmd[i]->background) {
            waitpid(pid, NULL, 0);
        }

        while (true) {
            pid_t pid = waitpid(-1, NULL, WNOHANG);
            if (pid <= 0) {
                break;
            }
        }
    }
    else {
        if (cmd[i]->redirect_in != NULL) {
            int fd = open(cmd[i]->redirect_in, O_RDONLY);
            if (fd == -1) {
                char *res = "open error\n";
                send(client_socket_fd, res, sizeof(res) + 1, 0);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }
        else if (cmd[i]->pipe_fd[0] != -1) {
            dup2(cmd[i]->pipe_fd[0], STDIN_FILENO);
            close(cmd[i]->pipe_fd[0]);
        }
        
        if (cmd[i]->redirect_out != NULL) {
            int fd = open(cmd[i]->redirect_out, O_CREAT | O_WRONLY | O_TRUNC, 0664);          // O_TRUNC for clearing the previous file contents
            if (fd == -1) {
                char *res = "open error\n";
                send(client_socket_fd, res, sizeof(res), 0);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
        else if (cmd[i]->pipe_fd[1] != -1) {
            dup2(cmd[i]->pipe_fd[1], STDOUT_FILENO);
            close(cmd[i]->pipe_fd[1]);
        }
        else {
            dup2(client_socket_fd, STDOUT_FILENO);
        }

        if (cmd[i]->redirect_err != NULL) {
            int fd = open(cmd[i]->redirect_err, O_CREAT | O_WRONLY | O_TRUNC, 0664);          // O_TRUNC for clearing the previous file contents
            if (fd == -1) {
                char *res = "open error\n";
                send(client_socket_fd, res, sizeof(res), 0);
                exit(EXIT_FAILURE);
            }
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        else {
            dup2(client_socket_fd, STDERR_FILENO);
        }
        
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        close(cmd[i]->pipe_fd[0]);
        close(cmd[i]->pipe_fd[1]);

        execvp(cmd[i]->com_name, cmd[i]->argv);
        
        char *res = "execvp error\n";
        send(client_socket_fd, res, strlen(res), 0);
        // char res_end[1] = {PROTOCOL_RES_END};
        // send(client_socket_fd, res_end, sizeof(res_end), 0);
        exit(EXIT_FAILURE);
    }
}


static bool is_builtin_cmd(command *cmd) {
    return (
        (strcmp(cmd->com_name, "pwd") == 0) || 
        (strcmp(cmd->com_name, "cd") == 0) || 
        (strcmp(cmd->com_name, "history") == 0) 
    );
}


static void execute_builtin_cmd(command **cl, int i, char **history, int history_len, int history_start_idx, int client_socket_fd) {
    command *cmd = cl[i];
    int pipe_fd[2] = {-1, -1};
    if (cl[i]->pipe_to != 0 && cl[i]->redirect_out == NULL) {
        if (pipe(pipe_fd) == -1) {
            perror("pipe error\n");
            exit(EXIT_FAILURE);
        }
        cl[cl[i]->pipe_to]->pipe_fd[0] = pipe_fd[0];
        cl[i]->pipe_fd[1] = pipe_fd[1];
    }

    if (strcmp(cmd->com_name, "pwd") == 0) {
        if (cmd->argc == 1) {
            char *cwd = get_current_dir_name();
            if (cwd == NULL) {
                char *res = "getcwd failed to allocate memory\n";
                send(client_socket_fd, res, strlen(res), 0);
            }
            else if (cmd->redirect_out != NULL) {
                FILE* f = fopen(cmd->redirect_out, "w");
                fprintf(f, "%s\n", cwd);
                fclose(f);
            }
            else if (cmd->pipe_to != 0) {
                write(cmd->pipe_fd[1], cwd, strlen(cwd));
                write(cmd->pipe_fd[1], "\n", strlen("\n"));
            }
            else {
                send(client_socket_fd, cwd, strlen(cwd), 0);
                send(client_socket_fd, "\n", strlen("\n"), 0);
            }

            if (cwd != NULL) {
                free(cwd);
            }
        }
        else {
            char *res = "Invalid number of arguments\nUsage: pwd [> OUTPUT_FILE] [2> ERROR_FILE]\n";
            send(client_socket_fd, res, strlen(res), 0);
        }
    }
    else if (strcmp(cmd->com_name, "cd") == 0) {
        if (cmd->argc == 1) {
            char *home = getpwuid(getuid())->pw_dir;
            if (chdir(home) == -1) {
                char *res = "chdir to HOME failed\n";
                send(client_socket_fd, res, strlen(res), 0);
            }
            else {
                send(client_socket_fd, home, strlen(home), 0);
                send(client_socket_fd, "\n", strlen("\n"), 0);
            }
        }
        else if (cmd->argc == 2) {
            if (chdir(cmd->argv[1]) == -1) {
                char *res = "NOT valid directory\n";
                send(client_socket_fd, res, strlen(res), 0);
            }
            else {
                send(client_socket_fd, cmd->argv[1], strlen(cmd->argv[1]), 0);
                send(client_socket_fd, "\n", strlen("\n"), 0);
            }
        }
        else {
            char *res = "Invalid number of arguments\nUsage: cd [NEW_DIRECTORY] [2> ERROR_FILE]\n";
            send(client_socket_fd, res, strlen(res), 0);
        }
    }
    else if (strcmp(cmd->com_name, "history") == 0) {
        if (cmd->argc == 1) {
            int fd;
            if (cmd->redirect_out != NULL) {
                fd = open(cmd->redirect_out, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                
            }
            else if (cmd->pipe_to != 0) {
                fd = cmd->pipe_fd[1];
            }
            else {
                fd = client_socket_fd;
            }

            for (int i = 0; i < history_len; i++) {
                char line_num[15];
                sprintf(line_num, "%5d\t", i);
                write(fd, line_num, strlen(line_num));
                write(fd, history[(history_start_idx + i) % HISTORY_SIZE], strlen(history[(history_start_idx + i) % HISTORY_SIZE]));
                write(fd, "\n", strlen("\n"));
            }

            if (cmd->redirect_out != NULL) {
                close(fd);
            }
        }
        else {
            char *res = "Invalid number of arguments\nUsage: history [> OUTPUT_FILE] [2> ERROR_FILE]\n";
            send(client_socket_fd, res, strlen(res), 0);
        }
    }
    
    if (cmd->pipe_fd[0] != -1) {
        close(cmd->pipe_fd[0]);
    }
    if (cmd->pipe_fd[1] != -1) {
        close(cmd->pipe_fd[1]);
    }
}


bool resolve_history_pointers(char **cmd, char **history, int history_start_idx, int history_len) {
    int in_qoute = 0;
    
    for (int i = 0; (*cmd)[i]; i++) {
        if ((*cmd)[i] == '"' || (*cmd)[i] == '\'') {
            in_qoute = !in_qoute;
            continue;
        }
        else if (in_qoute || (*cmd)[i] != '!') {
            continue;
        }
        else if ((*cmd)[i] == '\\') {
            if ((*cmd)[i + 1]) {
                i++;
            }
            continue;
        }
        
        char *end = strchr(*cmd + i, ' ');
        if (end == NULL) {
            end = strchr(*cmd + i, '\0');
        }

        if (isdigit((*cmd)[i + 1])) {
            int idx = atoi(*cmd + i + 1);
            if (idx >= history_len) {
                // Error
                return false;
            }

            char *prev_cmd = history[(history_start_idx + idx) % HISTORY_SIZE];
            replace_substring(cmd, i, end - *cmd - 1, prev_cmd);
        }
        else {
            char *prev_cmd = NULL;
            for (int j = history_len - 1; j >= 0; j--) {
                if (strncmp(*cmd + i + 1, history[(history_start_idx + j) % HISTORY_SIZE], end - *cmd - i - 1) == 0) {
                    prev_cmd = history[(history_start_idx + j) % HISTORY_SIZE];
                    break;
                }
            }

            if (prev_cmd == NULL) {
                return false;
            }

            replace_substring(cmd, i, end - *cmd - 1, prev_cmd);
        }
    }

    return true;
}


static void execute_protocol_cmd(protocol_cmd_type cmd_type, char *cmd, char **history, int *history_start_idx, int *history_len, int client_socket_fd) {
    char res_end[1] = {PROTOCOL_RES_END};
    
    if (cmd_type == PROTOCOL_CMD_READ_FROM_HISTORY) {
        int history_idx = atoi(cmd);
        
        if (history_idx >= 0 && history_idx < *history_len) {
            char *res = history[(*history_start_idx + *history_len - 1 - history_idx) % HISTORY_SIZE];
            send(client_socket_fd, res, strlen(res), 0);
        }

        write(client_socket_fd, res_end, sizeof(res_end));
        return;
    }
    else if (cmd_type == PROTOCOL_CMD_EXECUTE_SHELL_CMD) {
        if (!resolve_history_pointers(&cmd, history, *history_start_idx, *history_len)) {
            char *res = "couldn't resolve history pointers\n";
            send(client_socket_fd, res, strlen(res), 0);
            write(client_socket_fd, res_end, sizeof(res_end));
            return;
        }

        while (isspace(*cmd)) {
            cmd++;
        }

        if (*cmd == '\0') {
            write(client_socket_fd, res_end, sizeof(res_end));
            return;
        }

        // Add to history buffer
        history[(*history_start_idx + *history_len) % HISTORY_SIZE] = strdup(cmd);
        if (*history_len < HISTORY_SIZE) {
            (*history_len)++;
        }
        else {
            *history_start_idx = (*history_start_idx + 1) % HISTORY_SIZE;
        }

        command **parsed_cmd = process_cmd_line(cmd, 1);
        if (parsed_cmd == NULL) {
            char *res = "couldn't parse your command\n";
            send(client_socket_fd, res, strlen(res), 0);
            write(client_socket_fd, res_end, sizeof(res_end));
            return;
        }

        for (int i = 0; parsed_cmd[i] != NULL; i++) {
            if (is_builtin_cmd(parsed_cmd[i])) {
                execute_builtin_cmd(parsed_cmd, i, history, *history_len, *history_start_idx, client_socket_fd);
            }
            else if (execute_external_cmd(parsed_cmd, i, client_socket_fd)) {
            }
            else {
                break;
            }
        }

        write(client_socket_fd, res_end, sizeof(res_end));
        clean_up(parsed_cmd);
    }
}


void run_server_remote_shell(int client_socket_fd) {
    command **cl;

    char *history[HISTORY_SIZE] = {NULL};
    int history_start_idx = 0;
    int history_len = 0;

    while (true) {
        protocol_cmd_type cmd_type;
        int read_size = read(client_socket_fd, &cmd_type, sizeof(protocol_cmd_type));
        if (read_size < 0) {
            perror("read error\n");
            break;
        }
        else if (read_size == 0 || cmd_type == PROTOCOL_CMD_EXIT) {
            break;
        }

        char *protocol_cmd_content = malloc(sizeof(char));
        int protocol_cmd_content_len = 0;
        int protocol_cmd_content_cap = 1;
        do {
            protocol_cmd_content_cap += 1024;
            protocol_cmd_content = realloc(protocol_cmd_content, protocol_cmd_content_cap * sizeof(char));
            read_size = read(client_socket_fd, protocol_cmd_content + protocol_cmd_content_len, 1024);
            protocol_cmd_content_len += read_size;
        } while (protocol_cmd_content[protocol_cmd_content_len - 1] != '\0');

        execute_protocol_cmd(cmd_type, protocol_cmd_content, history, &history_start_idx, &history_len, client_socket_fd);
        free(protocol_cmd_content);
    }

    close(client_socket_fd);
}
