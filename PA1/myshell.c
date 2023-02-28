/*
    COMP3511 Fall 2022 
    PA1: Simplified Linux Shell (MyShell)

    Your name: CHEN,Jiawei
    Your ITSC email: jchenfq@connect.ust.hk 

    Declaration:

    I declare that I am not involved in plagiarism
    I understand that both parties (i.e., students providing the codes and students copying the codes) will receive 0 marks. 

*/

// Note: Necessary header files are included
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h> // For constants that are required in open/read/write/close syscalls
//#include <sys/wait.h> // For wait() - suppress warning messages
#include <fcntl.h> // For open/read/write/close syscalls

// Assume that each command line has at most 256 characters (including NULL)
#define MAX_CMDLINE_LEN 256

// Assume that we have at most 8 arguments
#define MAX_ARGUMENTS 8

// Assume that we only need to support 2 types of space characters: 
// " " (space) and "\t" (tab)
#define SPACE_CHARS " \t"

// The pipe character
#define PIPE_CHAR "|"

// Assume that we only have at most 8 pipe segements, 
// and each segment has at most 256 characters
#define MAX_PIPE_SEGMENTS 8

// Assume that we have at most 8 arguments for each segment
//
// We also need to add an extra NULL item to be used in execvp
//
// Thus: 8 + 1 = 9
//
// Example: 
//   echo a1 a2 a3 a4 a5 a6 a7 
//
// execvp system call needs to store an extra NULL to represent the end of the parameter list
//
//   char *arguments[MAX_ARGUMENTS_PER_SEGMENT]; 
//
//   strings stored in the array: echo a1 a2 a3 a4 a5 a6 a7 NULL
//
#define MAX_ARGUMENTS_PER_SEGMENT 9

// Define the  Standard file descriptors here
#define STDIN_FILENO    0       // Standard input
#define STDOUT_FILENO   1       // Standard output 


 
// This function will be invoked by main()
// TODO: Implement the multi-level pipes below
void process_cmd(char *cmdline);

// read_tokens function is given
// This function helps you parse the command line
// Note: Before calling execvp, please remember to add NULL as the last item 
void read_tokens(char **argv, char *line, int *numTokens, char *token);

// Here is an example code that illustrates how to use the read_tokens function
// int main() {
//     char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
//     int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
//     char cmdline[MAX_CMDLINE_LEN]; // the input argument of the process_cmd function
//     int i, j;
//     char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL}; 
//     int num_arguments;
//     strcpy(cmdline, "ls | sort -r | sort | sort -r | sort | sort -r | sort | sort -r");
//     read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);
//     for (i=0; i< num_pipe_segments; i++) {
//         printf("%d : %s\n", i, pipe_segments[i] );    
//         read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);
//         for (j=0; j<num_arguments; j++) {
//             printf("\t%d : %s\n", j, arguments[j]);
//         }
//     }
//     return 0;
// }


/* The main function implementation */
int main()
{
    char cmdline[MAX_CMDLINE_LEN];
    fgets(cmdline, MAX_CMDLINE_LEN, stdin);
    process_cmd(cmdline);
    return 0;
}

// TODO: implementation of process_cmd
void process_cmd(char *cmdline)
{
  // You can try to write: printf("%s\n", cmdline); to check the content of cmdline
    int i, j, k; 
    char *pipe_segments[MAX_PIPE_SEGMENTS]; // character array buffer to store the pipe segements
    int num_pipe_segments; // an output integer to store the number of pipe segment parsed by this function
    char *arguments[MAX_ARGUMENTS_PER_SEGMENT] = {NULL}; 
    int num_arguments;
    read_tokens(pipe_segments, cmdline, &num_pipe_segments, PIPE_CHAR);
    char *pipe_arg[num_pipe_segments][MAX_ARGUMENTS_PER_SEGMENT];  
    for (i = 0; i < num_pipe_segments; i++) {
        for(k = 0; k < MAX_ARGUMENTS_PER_SEGMENT; k++){
            arguments[k] = NULL;
        }
        read_tokens(arguments, pipe_segments[i], &num_arguments, SPACE_CHARS);
        for(j = 0; j < MAX_ARGUMENTS_PER_SEGMENT; j++){
            pipe_arg[i][j] = arguments[j];
        }
    }

    if(num_pipe_segments == 1){
        int in = 0;
        int out = 0;
        char input_f[MAX_CMDLINE_LEN] = {'\0'};
        char output_f[MAX_CMDLINE_LEN] = {'\0'};
        i = 0;
        while(pipe_arg[0][i] != NULL){
            if(strcmp(pipe_arg[0][i], "<") == 0){
                in = 1;
                k = i;
                strcpy(input_f,pipe_arg[0][i + 1]);
                while (pipe_arg[0][i] != NULL)
                {
                    pipe_arg[0][i] = pipe_arg[0][i + 2];
                    i++;
                }
            }

            else if(strcmp(pipe_arg[0][i], ">") == 0){
                out = 1;
                k = i;
                strcpy(output_f,pipe_arg[0][i + 1]);
                while (pipe_arg[0][i] != NULL)
                {
                    pipe_arg[0][i] = pipe_arg[0][i + 2];
                    i++;
                }
            }
            
            if(in == 1 || out == 1){
                i = k;
                continue;
            }

            i++;
        }
        
        if(in == 1){
            int fd_in;
            fd_in = open(input_f, O_RDONLY, S_IRUSR | S_IWUSR );
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }    
        
        if(out == 1){
            int fd_out;
            fd_out = open(output_f, O_CREAT | O_WRONLY , S_IRUSR | S_IWUSR );
            dup2(fd_out,STDOUT_FILENO); 
            close(fd_out);
        }
        
        execvp(pipe_arg[0][0], pipe_arg[0]);
    }

    else{
        int pfds[2]; 
        for(i = 0; i < num_pipe_segments-1; i++){
            pipe(pfds); 
            pid_t pid = fork(); 
            if ( pid == 0 ) { 
                close(1);
                dup2(pfds[1],1);
                close(pfds[0]);
                execvp(pipe_arg[i][0], pipe_arg[i]);
            } 
	        else {
                close(0);
                dup2(pfds[0],0); 
                close(pfds[1]);
                wait(NULL); 
            }
        } 

       	execvp(pipe_arg[i][0],pipe_arg[i]);

    }
}

// Implementation of read_tokens function
void read_tokens(char **argv, char *line, int *numTokens, char *delimiter)
{
    int argc = 0;
    char *token = strtok(line, delimiter);
    while (token != NULL)
    {
        argv[argc++] = token;
        token = strtok(NULL, delimiter);
    }
    *numTokens = argc;
}
