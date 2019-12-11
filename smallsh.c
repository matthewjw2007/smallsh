/****
 * Author: Matthew Esqueda
 * Date: 20 November 2019
 * Assignment: Program 3 - Smallsh
 * Sources: https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
 *          https://oregonstate.instructure.com/courses/1738958/pages/block-3
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

typedef enum {false, true} bool;

// Maximum length of characters and maximum arguments allowed
#define MAX_CHAR_LENGTH 2048
#define MAX_ARG 512

int main(int argc, char* argv[]) {
    int exitShell = 0;                 // Value to know to exit the shell or not
    int exitStatus;
    int signalStatus=0;
    char userInput[MAX_CHAR_LENGTH];    // Value to hold the input the user enters
    char *arguments[MAX_ARG];            // Value to hold the arguments the user enters
    char *token;
    int i=0;
    int j;
    int totalArgs=0;
    pid_t openProcesses[500];
    int totalOpenProcesses=0;
    bool backgroundProcess = false;
    int status;

    // Signal structures - used lecture 3.3 examples as source
//    struct sigaction SIGINT_action;
//    struct sigaction SIGTSTP_action;
//
//    SIGINT_action.sa_handler = SIG_IGN;
//    SIGINT_action.sa_flags = 0;
//    sigaction(SIGINT, &SIGINT_action, NULL);

    // Get the process ID of the shell
    int shellPID = getpid();

    // Allocate memory
    for(i=0; i<=MAX_ARG; i++){
        arguments[i] = (char *)malloc(2048 * sizeof(char));
    }

    do{
        totalArgs = 0;
        // Prompt user for input
        printf(": ");
        fflush(stdout);
        fflush(stdin);

        // Get the user's input
        fgets(userInput, MAX_CHAR_LENGTH, stdin);

        // Parse out the input the user entered
        // https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/
        token = strtok(userInput, " \n");

        while(token != NULL){
            // Assign values to the argument array
            arguments[totalArgs] = token;
            token = strtok(NULL, " \n");
            totalArgs++;
        }

        arguments[totalArgs] = NULL;

        // Check if we are calling a background process
        if(strcmp(arguments[totalArgs-1], "&") == 0){
            backgroundProcess = true;
            arguments[totalArgs-1] = NULL;
        } else {
            backgroundProcess = false;
        }

        // Check if any argument contains $$ and replace with the shell pid

        // Check if # is the first character or if arguments is empty - means it is a comment or nothing was entered by the user
        if(arguments[0] == NULL || strncmp(arguments[0], "#", 1) == 0){
            // Do nothing
            exitStatus = 0;
        }
        // Check if the exit command is received
        else if (strcmp(arguments[0], "exit") == 0) {
            printf("Exiting program. Killing background processes.\n");
            fflush(stdout);

            // Kill background processes
            // https://www.geeksforgeeks.org/signals-c-language/
            for(i=0; i<totalOpenProcesses; i++){
                kill(openProcesses[i], SIGKILL);
            }

            // Change value of exitStatus
            exitShell = 1;
        }
        // Check if the cd command is received
        // https://www.geeksforgeeks.org/chdir-in-c-language-with-examples/
        else if (strcmp(arguments[0], "cd") == 0) {
            // Check if there is a second argument - if so, change to that directory
            if(totalArgs > 1){
                if(chdir(arguments[1]) != 0){
                    printf("chdir() error\n");
                    fflush(stdout);
                }
            } else {
                // If no second argument - change to HOME directory
                chdir(getenv("HOME"));
            }
        }
        // Check if the status command is received
        else if (strcmp(arguments[0], "status") == 0) {
            int childExitMethod;
            // Print exit status or term signal of last fg process
            if(WIFEXITED(status) != 0){
                exitStatus = WEXITSTATUS(status);
                printf("exit value %d\n", exitStatus);
                fflush(stdout);
            } else {
                printf("terminated by signal %d\n", signalStatus);
                fflush(stdout);
            }
        }
        // If none of the above - forks and executes for non-built in commands
        else {
            // Variables for forking
            pid_t spawnPid;

            spawnPid = fork();

            // if fork worked
            if(spawnPid == 0){
                // Variables for redirection and files
                char outputFile[20];
                char inputFile[20];
                bool inputRedirection = false;
                bool outputRedirection = false;

                // Check if we are redirecting anything
                if(totalArgs>4 && strcmp(arguments[totalArgs-4], "<") == 0){
                    inputRedirection = true;

                    // Grab the file name after operator
                    strcpy(inputFile, arguments[totalArgs-3]);

                    // Check for output redirection
                    if(strcmp(arguments[totalArgs-2], ">") == 0){
                        // Grab output file name
                        strcpy(outputFile, arguments[totalArgs-1]);
                        outputRedirection=true;
                    }

                    // Remove redirection operators and file names
                    for(i=1; i<=4; i++){
                        arguments[totalArgs-1] = NULL;
                        totalArgs--;
                    }
                }
                else if(totalArgs>2 && strcmp(arguments[totalArgs-2], "<") == 0){
                    //printf("passed\n");
                    inputRedirection = true;

                    // Grab the file name after operator
                    strcpy(inputFile, arguments[totalArgs-1]);
                    //printf("%s\n", inputFile);

                    // Remove redirection operator
                    for(i=1; i<=2; i++){
                        arguments[totalArgs-1] = NULL;
                        totalArgs--;
                    }
                    //printf("%s\n", inputFile);
                }

                if(totalArgs>4 && strcmp(arguments[totalArgs-4], ">") == 0){
                    outputRedirection = true;

                    // Grab the file name after operator
                    strcpy(outputFile, arguments[totalArgs-3]);

                    // Check for input redirection
                    if(strcmp(arguments[totalArgs-2], "<") == 0){
                        // Grab output file name
                        strcpy(inputFile, arguments[totalArgs-1]);
                        inputRedirection=true;
                    }

                    // Remove redirection operators and file names
                    for(i=1; i<=4; i++){
                        arguments[totalArgs-1] = NULL;
                        totalArgs--;
                    }
                }
                else if(totalArgs>2 && strcmp(arguments[totalArgs-2], ">") == 0){
                    outputRedirection = true;

                    // Grab the file name after operator
                    strcpy(outputFile, arguments[totalArgs-1]);

                    // Remove redirection operator
                    for(i=1; i<=2; i++){
                        arguments[totalArgs-1] = NULL;
                        totalArgs--;
                    }
                }

                // Set up dup2
                // https://www.geeksforgeeks.org/input-output-system-calls-c-create-open-close-read-write/
                if(backgroundProcess == true && inputRedirection == false){
                    int f0;

                    f0 = open("/dev/null", O_RDONLY);

                    if(f0 == -1){
                        printf("Input file error\n");
                        fflush(stdout);
                        exit(1);
                    }

                    if(dup2(f0, 0) == -1){
                        printf("Input file error\n");
                        fflush(stdout);
                        exit(1);
                    }
                }

                if(inputRedirection == true){
                    int f1;

                    f1 =open(inputFile, O_RDONLY);

                    if(f1 == -1){
                        printf("Input file error\n");
                        fflush(stdout);
                        exit(1);
                    }

                    if(dup2(f1, 0) == -1){
                        printf("Input file error\n");
                        fflush(stdout);
                        exit(1);
                    }
                }

                if(outputRedirection == true){
                    int f2;

                    f2 = open(outputFile, O_WRONLY|O_CREAT|O_TRUNC, 0644);

                    if(f2 == -1){
                        printf("Output file error\n");
                        fflush(stdout);
                        exit(1);
                    }

                    if(dup2(f2, 1) == -1){
                        printf("Output file error\n");
                        fflush(stdout);
                        exit(1);
                    }
                }

                execvp(arguments[0], arguments);

                printf("Exec failed\n");
                fflush(stdout);
                exit(1);
            }

            // If running a background process, don't wait for it to complete
            if(backgroundProcess == true){
                // Print background process ID
                printf("background pid is %d\n", spawnPid);
                fflush(stdout);

                // Add backgound pid to array
                openProcesses[totalOpenProcesses] = spawnPid;
                totalOpenProcesses++;
                backgroundProcess = false;
            } else {
                waitpid(spawnPid, &status, 0);
            }
        }

        int backgroundStatus;

        // Check if any background process have ended
        for(i=0; i<totalOpenProcesses; i++){
            // Check if the process has completed
            if(waitpid(openProcesses[i], &status, WNOHANG) != 0){
                // Check if the process was exited
                if(WIFEXITED(status) != 0){
                    backgroundStatus = WEXITSTATUS(status);
                    printf("Background PID %d is done: exit value %d\n", openProcesses[i], backgroundStatus);
                    fflush(stdout);
                }
                // Check if the process was terminated
                else if(WIFSIGNALED(status) != 0){
                    backgroundStatus = WTERMSIG(status);
                    printf("Background PID %d is done: terminated by signal %d\n", openProcesses[i], backgroundStatus);
                    fflush(stdout);
                }

                // Remove the process from the array and push other values forward
                // https://www.geeksforgeeks.org/delete-an-element-from-array-using-two-traversals-and-one-traversal/
                totalOpenProcesses--;
                for(j=i; j<totalOpenProcesses; j++){
                    openProcesses[j] = openProcesses[j+1];
                }

                i--;
            }
        }

    } while (exitShell != 1);

    return 0;
}