#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>


#define CMDLINE_MAX 512
#define ARGS_COUNT_MAX 16
#define TOKEN_LENGTH_MAX 32

struct command {

    char *list[18]; // first element must be "command name", followed by max of 16 arguments, terminated with NULL

};

struct pipedCmds {
    char *list[3];
    int size;
};

struct pipedCmdsArgs {
    struct command list[3];
    int size;
};


void ParsePipedCommands(char *commandLine, struct pipedCmds *pipedCmds) {
    char *str = strdup(commandLine);

    char *token;

    for (int i = 0; i < 3; i++) {
        token = strsep(&str, "|");
        if (token == NULL) {
            break;
        } else {
            pipedCmds->list[i] = token;
            pipedCmds->size += 1;
        }
    }
}

void RedirectOutputTo(char *fileName) {
    int fd;

    fd = open(fileName, O_WRONLY | O_CREAT,
              S_IWUSR | S_IRUSR); // OPpen file and grants permission, create file first if it does not exist

    if (fd == -1) {

        fprintf(stderr, "Error: cannot open output file\n");
        exit(1);
    }

    dup2(fd, STDOUT_FILENO);

    close(fd);
}

void execute(char rawCmd[], struct command cmd) {
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) { // Child
        if (!strcmp("cd", cmd.list[0])) {
            exit(0);
        }

        execvp(cmd.list[0], cmd.list);
        perror("Error");

        exit(1);
    } else if (pid > 0) { // Parent
        waitpid(-1, &status, 0);
        switch (WEXITSTATUS(status)) {
            case 0:
                chdir(cmd.list[1]);
        }
        fprintf(stderr, "+ completed '%s' [%d]\n", rawCmd, WEXITSTATUS(status));     // collects child's exit status
    } else { // Forking Error
        perror("fork");
        exit(1);
    }
}

// Return boolean to tell shell if there is parsing error or not
bool parse(char *commandLine, struct command *cmd) {
    char *token;
    int index = 0;
    bool outputRedirectionEnabled = false; // do we need to redirect output?
    char *redirectionFileName; // file to redirect output to, if redirection enabled

    // get first token from commandLine
    token = strtok(commandLine, " "); // use space(s) as delimeter
    if (!strcmp(token, ">") || !strcmp(token, "|")) {
        fprintf(stderr, "Error: missing command\n");
        return true; // there is error
    }
    cmd->list[0] = token;

    // walk through remaining tokens
    while (token != NULL) {
        token = strtok(NULL, " ");
        index++;
        if (index > ARGS_COUNT_MAX) {
            fprintf(stderr, "Error: too many process arguments from PARSE\n");
            return true; // there is error
        }
        if (token == NULL && outputRedirectionEnabled) { // Didn't supply output file name

            fprintf(stderr, "Error: no output file\n");
            return true;
        }
        if (token == NULL) {
            break;
        }

        if (strcmp(token, ">") == 0) {
            outputRedirectionEnabled = true;
            continue; // don't store ">" because it is not necessary for exec function to work
        }
        if (outputRedirectionEnabled) { // if output redirection enabled
            redirectionFileName = token; // store name of redirection file
            index--;
            RedirectOutputTo(redirectionFileName);
            break;
        } else { // if output redirection not enabled
            cmd->list[index] = token; // this token is argument, stores it into command.list
        }

    }

    cmd->list[index] = NULL; // terminate with NULL so it can be used by exec family functions
    return false;
}

// Parse command without output redirection
struct command parseWithoutRedirection(char *commandLine, bool *errorFlag) {
    char *token;
    int index = 0;

    struct command cmd;

    // get first token from commandLine
    token = strtok(commandLine, " "); // use space(s) as delimeter
    cmd.list[0] = token;

    if (!strcmp(token, ">") || !strcmp(token, "|")) {
        fprintf(stderr, "Error: missing command\n");
        *errorFlag = true; // there is error
        return cmd;
    }

    // walk through remaining tokens
    while (token != NULL) {
        token = strtok(NULL, " ");
        index++;
        if (index > ARGS_COUNT_MAX) {
            fprintf(stderr, "Error: too many process arguments from parsewithoutredir\n");
            *errorFlag = true; // there is error
            return cmd;
        }
        if (token == NULL) {
            break;
        }
        cmd.list[index] = token; // this token is argument, stores it into command.list

    }

    cmd.list[index] = NULL; // terminate with NULL so it can be used by exec family functions
    return cmd;
}


// Return boolean to tell shell if there is parsing error or not
bool ParsePipedCommandsArgs(struct pipedCmds pipedCmds, struct pipedCmdsArgs *pipedCmdsArgs) {
    int numCommands = pipedCmds.size;
    struct command cmd;
    bool errorFlag = false;

    for (int i = 0; i < numCommands; i++) {

        cmd = parseWithoutRedirection(pipedCmds.list[i], &errorFlag);
        if (errorFlag) {
            return true;
        }
        pipedCmdsArgs->list[i] = cmd;
        pipedCmdsArgs->size += 1;
    }

    return false;
}


/*
 pipeline3 calls three piped commands
 first command | second command | third command
*/
void pipeline3(struct command p1, struct command p2, struct command p3) {
    int fd[2]; // child and grandchild
    int fd1[2]; //parent and child

    pipe(fd);

    if (fork() == 0) { // Parent (second command)

        pipe(fd1);

        if (fork() == 0) { // Child (first command)

            dup2(fd1[1], STDOUT_FILENO);
            close(fd1[0]);
            close(fd1[1]);
            execvp(p1.list[0], p1.list);
            perror("Error");
            exit(1);

        } else { // Parent (second command)

            dup2(fd1[0], STDIN_FILENO);
            dup2(fd[1], STDOUT_FILENO);
            close(fd1[0]);
            close(fd1[1]);
            execvp(p2.list[0], p2.list);
            perror("Error");
            exit(1);
        }

        exit(1);

    } else {  // Grandchild (third command)
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);
        close(fd[1]);
        execvp(p3.list[0], p3.list);
        perror("Error");
        exit(4);
    }
}


// p1(parent) | p2(child)
void pipeline2(struct command p1, struct command p2) {
    int fd[2]; //parent and child
    pid_t pid = fork();

    pipe(fd);

    if (pid == 0) { // Child (p2)

        close(fd[1]);  // Don't need write access to pipe
        dup2(fd[0], STDIN_FILENO); // And replace it with the pipe
        close(fd[0]); // Close fd[0]

        execvp(p2.list[0], p2.list);


    } else if (pid > 0) { // Parent (p1)

        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        execvp(p1.list[0], p1.list);

    } else {
        perror("Fork");
    }


}

int main(void) {
    char cmd[CMDLINE_MAX];
    struct command command;
    int savedStdOut = dup(1); // save current stdout for use later for stdout restoration
    char cmdCopy[CMDLINE_MAX]; // copy of original command inputted by user
    bool errorFound = false; // flag to check if there is parsing error

    while (1) {
        struct pipedCmds pipedCmds; // eg. list[0] = "Echo Hello", list[1] = "grep Hello", ..etc. tokenized by "|"
        struct pipedCmdsArgs pipedCmdsArgs; // parsed args for each piped command (1, 2, or 3)
        pipedCmds.size = 0;
        pipedCmdsArgs.size = 0;

        char *nl;

        /* Print prompt */
        printf("sshell$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        strcpy(cmdCopy, cmd); // save a copy of original command to print after command is completed

        ParsePipedCommands(cmd, &pipedCmds);

        if (pipedCmds.size == 1) {
            /* Builtin command */
            if (!strcmp(cmd, "exit")) {
                fprintf(stderr, "Bye...\n+ completed '%s' [0]\n", cmdCopy);
                break;
            }

            /* Parse command */
            errorFound = parse(cmd, &command);

            if (errorFound) {
                continue;
            }
            /* Execute command */
            execute(cmdCopy, command);


        } else {

            pid_t pid = fork();
            int status;
            errorFound = ParsePipedCommandsArgs(pipedCmds, &pipedCmdsArgs);
            if (errorFound) {
                errorFound = false; // reset error flag
                continue;
            }

            struct command process1 = pipedCmdsArgs.list[0];
            struct command process2 = pipedCmdsArgs.list[1];

            if (pid == 0) { /* Child */

                if (pipedCmds.size == 2) { // if there are two pipeline commands

                    pipeline2(process1, process2);

                } else if (pipedCmds.size == 3) { // if there are three pipeline commands

                    struct command process3 = pipedCmdsArgs.list[2];
                    pipeline3(process1, process2, process3);

                } else {
                    perror("Pipe");
                }


            } else if (pid > 0) { /* Parent */
                waitpid(-1, &status, 0);
            } else { /* Forking Error */
                perror("fork");
                exit(1);
            }

        }

        // Restore stdout so we print to terminal again
        dup2(savedStdOut, 1);

    }

    return EXIT_SUCCESS;
}

