/*
 * Parser.c
 * A simple Command Line Parser.
 * Author : Michael Roberts <mroberts@it.net.au>
 * Last Modification : 14/08/01
 */

#include "parser.h"


char* next_arg(char *cmd, int *end_idx) {
    int i = 0;
    int arg_len = 0;
    int arg_cap = 1;
    char *arg;
    
    if (cmd == NULL) {
        return NULL;
    }

    while (cmd[i] == ' ') {
        i++;
    }

    if (cmd[i] == '\0') {
        *end_idx = i;
        return NULL;
    }

    arg = malloc(2 * sizeof(char));
    arg[0] = '\0';

    int in_quote = 0;
    while (cmd[i]) {
        if (cmd[i] == '"') {
            in_quote = !in_quote;
        }
        else if (cmd[i] == ' ' && !in_quote) {
            break;
        }
        else {
            if (cmd[i] == '\\') {
                if (cmd[i + 1] == '\0') {
                    continue;
                }
                else {
                    i++;
                }
            }
            
            if (arg_len >= arg_cap) {
                arg_cap *= 2;
                arg = realloc(arg, arg_cap * sizeof(char));
            }

            arg[arg_len] = cmd[i];
            arg_len++;
            arg[arg_len] = '\0';
        }

        i++;
    }
    
    if (end_idx != NULL) {
        *end_idx = i;
    }
    return arg;
}


/*
 * This function breakes the simple command token isolated in other functions
 * into a sequence of arguments. Each argument is bounded by white-spaces, and
 * there is no special character intepretation. The results are stored in the
 * argv array of the result command structure.
 *
 * Arguments :
 *      cmd - the string to be processed.
 *      result - the comand struct to store the results in.
 *
 * Returns :
 *      None.
 *
 */
void process_simple_cmd(char *cmd, command *result)
{
    char *arg;
    result->argc = 1;
    int i;

#ifdef DEBUG
    fprintf(stderr, "process_simple_cmd\n");
#endif

    result->com_name = next_arg(cmd, &i);
    cmd += i;
    result->argv = realloc(result->argv, sizeof(char *));
    result->argv[0] = strdup(result->com_name);
    
    while ((arg = next_arg(cmd, &i)) != NULL && arg[0] != '\0') {
        cmd += i;

        if (index(arg, '?') != NULL || index(arg, '*') != NULL) {
            glob_t pglob;
            int stat = glob(arg, GLOB_NOSORT, NULL, &pglob);
            
            if (stat != 0) {
                if (stat == GLOB_NOSPACE) {
                    fprintf(stderr, "glob running out of memory when expanding \"%s\"\n", arg);
                }
                else if (stat == GLOB_ABORTED) {
                    fprintf(stderr, "glob read error \"%s\"\n", arg);
                }
                else if (stat == GLOB_NOMATCH) {
                    fprintf(stderr, "glob no found matches for \"%s\"\n", arg);
                }
                
                clean_up_cmd(result);
                return;
            }

            result->argv = realloc(result->argv, (result->argc + pglob.gl_pathc) * sizeof(char *));
            for (int i = 0; i < pglob.gl_pathc; i++) {
                result->argv[result->argc] = strdup(pglob.gl_pathv[i]);
                result->argc++;
            }
            globfree(&pglob);
        }
        else {
            result->argv = realloc(result->argv, (result->argc + 1) * sizeof(char *));
            result->argv[result->argc] = arg;
            result->argc++;
        }
    }
    
    /*Set the final array element NULL. */
    result->argv = realloc(result->argv, (result->argc + 1) * sizeof(char *));
    result->argv[result->argc] = NULL;
} /*End of process_simple_cmd() */

/*
 * This function parses the commands isolated from the command line string in
 * other functions. It searches the string looking for input and output
 * redirection characters. The simple commands found are sent to
 * process_simple_comd(). The redirection information is stored in the result
 * command structure.
 *
 * Arguments :
 *      cmd - the command string to be processed.
 *      result - the command structure to store the results in.
 *
 * Returns :
 *      None.
 *
 */
void process_cmd(char *cmd, command *result) {
    int in_quote = 0;

    for (int i = 0; cmd[i]; i++) {
        if (cmd[i] == '"') {
            in_quote = !in_quote;
        }
        else if (in_quote) {
            continue;
        }
        else if (cmd[i] == '\\') {
            if (cmd[i + 1]) {
                i++;
            }
            continue;
        }
        else if (cmd[i] == '<') {
            result->redirect_in = next_arg(cmd + i + 1, NULL);
            cmd[i] = '\0';
        } 
        else if (cmd[i] == '>') {
            result->redirect_out = next_arg(cmd + i + 1, NULL);
            cmd[i] = '\0';
        }
        else if (cmd[i] == '2' && cmd[i + 1] == '>') {
            result->redirect_err = next_arg(cmd + i + 2, NULL);
            cmd[i] = '\0';
        }
    }

    process_simple_cmd(cmd, result);
} /*End of process_cmd() */

/*
 * This function processes the command line. It isolates tokens seperated by
 * the '&' or the '|' character. The tokens are then passed on to be processed
 * by other functions. Once the first token has been isolated this function is
 * called recursivly to process the rest of the command line. Once the entire
 * command line has been processed an array of command structures is created
 * and returned.
 *
 * Arguments :
 *      cmd - the command line to be processed.
 *
 * Returns :
 *      An array of pointers to command structures.
 *
 */
command** process_cmd_line(char *cmd, int new) {
    static command **cmd_line;
    static int lc;
    int in_quote = 0;

    if (new) {
        lc = 0;
        cmd_line = NULL;
        char *rc = cmd;
        while ((rc = index(rc, '\'')) != NULL) {
            *rc = '"';
        }
    }

    while (*cmd == ' ') {
        cmd++;
    }

    for (int i = 0; cmd[i]; i++) {
        if (cmd[i] == '"') {
            in_quote = !in_quote;
        }

        if (in_quote) {
            continue;
        }
        else if (cmd[i] == '\\') {
            if (cmd[i + 1]) {
                i++;
                continue;
            }
        }

        if (cmd[i] == '&' || cmd[i] == ';' || cmd[i] == '|' || cmd[i + 1] == '\0') {
            cmd_line = realloc(cmd_line, (lc + 1) * sizeof(command *));
            if (cmd_line == NULL) {
                clean_up(cmd_line);
                return NULL;
            }

            cmd_line[lc] = calloc(1, sizeof(command));
            if (cmd_line[lc] == NULL) {
                clean_up(cmd_line);
                return NULL;
            }
            
            cmd_line[lc]->pipe_fd[0] = -1;
            cmd_line[lc]->pipe_fd[1] = -1;
            cmd_line[lc]->background = (cmd[i] == '&');
            cmd_line[lc]->pipe_to = (cmd[i] == '|') * (lc + 1);

            if (cmd[i] == '&' || cmd[i] == ';' || cmd[i] == '|') {
                cmd[i] = '\0';
            }
            process_cmd(cmd, cmd_line[lc]);
            if (cmd_line[lc]->com_name == NULL) {
                cmd_line[lc] = NULL;
                return cmd_line;
            }
            lc++;

            process_cmd_line(cmd + i + 1, 0);
            return cmd_line;
        }
    }

    cmd_line = realloc(cmd_line, (lc + 1) * sizeof(command *));
    cmd_line[lc] = NULL;
    return cmd_line;
} /*End of Process Cmd Line */

void clean_up_cmd(command *cmd) {
    if (cmd->com_name != NULL) {
        free(cmd->com_name); /*Free Com_Name */
        cmd->com_name = NULL;
    }

    if (cmd->argv != NULL) {
        for (int i = 0; cmd->argv[i] != NULL; i++) {
            free(cmd->argv[i]); /*Free each pointer in Argv */
        }
        free(cmd->argv); /*Free Argv Itself */
        cmd->argv = NULL;
    }

    if (cmd->redirect_in != NULL) {
        free(cmd->redirect_in); /*Free Redirect - In */
        cmd->redirect_in = NULL;
    }

    if (cmd->redirect_out != NULL) {
        free(cmd->redirect_out); /*Free Redirect - Out */
        cmd->redirect_out = NULL;
    }
    
    if (cmd->redirect_err != NULL) {
        free(cmd->redirect_err); /*Free Redirect - Err */
        cmd->redirect_err = NULL;
    }
}

/*
 * This function cleans up some of the dynamicly allocated memory. Each array
 * element is visited, and the contained data is free'd before the entire
 * structure is free'd.
 *
 * Arguments :
 *      cmd - the array of pointers to command structures to be cleaned.
 *
 * Returns :
 *      None.
 *
 */
void clean_up(command **cmd)
{
    for (int i = 0; cmd[i] != NULL; i++) {
        clean_up_cmd(cmd[i]);
        cmd[i] = NULL;
    }
    free(cmd); /*Free the Array */
    cmd = NULL;
    return;
} /*End of clean_up() */

/*
 * This function dumps the contents of the structure to stdout.
 *
 * Arguments :
 *      c - the structure to be displayed.
 *      count - the array position of the structure.
 *
 * Returns :
 *      None.
 *
 */
void dump_structure(command *c, int count)
{
    int lc = 0;

    printf("---- Command(%d) ----\n", count);
    printf("%s\n", c->com_name);
    if (c->argv != NULL) {
        while (c->argv[lc] != NULL) {
            printf("+-> argv[%d] = %s\n", lc, c->argv[lc]);
            lc++;
        }
    }
    printf("Background = %d\n", c->background);
    printf("Redirect Input = %s\n", c->redirect_in);
    printf("Redirect Output = %s\n", c->redirect_out);
    printf("Redirect Error = %s\n", c->redirect_err);
    printf("Pipe to Command = %d\n\n", c->pipe_to);
    printf("Pipe Read FD = %d\n\n", c->pipe_fd[0]);
    printf("Pipe Write FD = %d\n\n", c->pipe_fd[1]);

    return;
} /*End of dump_structure() */

/*
 * This function dumps the contents of the structure to stdout in a human
 * readable format..
 *
 * Arguments :
 *      c - the structure to be displayed.
 *      count - the array position of the structure.
 *
 * Returns :
 *      None.
 *
 */
void print_human_readable(command *c, int count)
{
    int lc = 1;

    printf("Program : %s\n", c->com_name);
    if (c->argv != NULL) {
        printf("Parameters : ");
        while (c->argv[lc] != NULL) {
            printf("%s ", c->argv[lc]);
            lc++;
        }
        printf("\n");
    }
    if (c->background == 1)
        printf("Execution in Background.\n");
    if (c->redirect_in != NULL)
        printf("Redirect Input from %s.\n", c->redirect_in);
    if (c->redirect_out != NULL)
        printf("Redirect Output to %s.\n", c->redirect_out);
    if (c->redirect_err != NULL)
        printf("Redirect Error to %s.\n", c->redirect_err);
    if (c->pipe_to != 0) {
        printf("Pipe Output to Command# %d\n", c->pipe_to);
        printf("Pipe Read FD %d\n", c->pipe_fd[0]);
        printf("Pipe Write FD %d\n", c->pipe_fd[1]);
    }
    printf("\n\n");

    return;
} /*End of print_human_readable() */
