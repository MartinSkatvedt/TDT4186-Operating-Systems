#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

char **parse_command(char *inp_command)
{
    char **parsed_input = malloc(strlen(inp_command)); // array of parsed input
    int input_index = 0;                               // array index

    char *pch = strtok(inp_command, " ");
    while (pch != NULL)
    {
        parsed_input[input_index] = pch;
        input_index++;
        pch = strtok(NULL, " ,.-");
    }

    parsed_input[input_index] = NULL; // Last index is NULL
    return parsed_input;
}

int print_work_dir()
{
    char current_dir[100];
    if (getcwd(current_dir, sizeof(current_dir)) != NULL)
    {
        printf("%s: ", current_dir);
    }
    else
    {
        perror("Could not get working directory");
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    printf("Welcome to flush \n");

    size_t input_bytes;
    size_t input_size = 2048;
    char *input_str = (char *)malloc(input_size);

    int child_status;
    while (1)
    {
        print_work_dir();
        input_bytes = getline(&input_str, &input_size, stdin);
        char **parsed_input = parse_command(input_str);

        pid_t pid = fork();

        if (pid == 0)
        {
            int ret_status = execvp(parsed_input[0], parsed_input);
            _exit(ret_status);
        }
        else if (pid > 0)
        {
            int child_return = waitpid(pid, &child_status, 0);
            printf("Exit status [%s] = %d\n", input_str, child_return);
        }
        else
        {
            printf(" %s", "Fork failed...");
        }
    }

    return 0;
}
