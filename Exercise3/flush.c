#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

struct Command
{
    pid_t pid;
    char command;
};

int execute_command(struct Command *command)
{
    return 0;
}

int main(int argc, char *argv[])
{
    printf("Welcome to flush \n");

    char input_str[100];
    while (1)
    {
        scanf("%100s", input_str);

    }

    return 0;   
}

