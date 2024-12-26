#ifndef _PARSER_H
#define _PARSER_H

/*
 * Parser.h
 * Data structures and various defines for parser.c
 * Author : Michael Roberts <mroberts@it.net.au>
 * Last Update : 15/07/01
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glob.h>


// #define DEBUG 


/*The Structure we create for the commands.*/
typedef struct Command_struct
{
   char *com_name;
   char **argv;
   int argc;
   int background;
   char *redirect_in;
   char *redirect_out;
   char *redirect_err;
   int pipe_to;
   int pipe_fd[2];
}
command;


/* Function prototypes added by Nick Nelissen 11/9/2001 */
command ** process_cmd_line(char *cmd,int);
void process_cmd(char *cmd, command * result);
void process_simple_cmd(char *cmd, command * result);
void clean_up_cmd(command *cmd);
void clean_up(command ** cmd);

#endif
