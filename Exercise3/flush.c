#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

typedef struct bg_job
{
    pid_t pid;
    char *command;
    struct bg_job *next_job;
    struct bg_job *prev_job;
} bg_job;

char **parse_command(char *inp_command, int *inp_size)
{
    char **parsed_input = malloc(strlen(inp_command)); // array of parsed input
    int input_index = 0;                               // array index

    char *pch = strtok(inp_command, " ");
    while (pch != NULL)
    {
        parsed_input[input_index] = pch;
        input_index++;
        pch = strtok(NULL, " ");
    }

    parsed_input[input_index] = NULL; // Last index is NULL
    *inp_size = input_index;
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

int get_IO_index(char **inp_arr, int *inp_size)
{
    for (int i = 0; i < *inp_size; i++)
    {
        if (strcmp(inp_arr[i], "<") == 0 || strcmp(inp_arr[i], ">") == 0)
        {
            return i;
        }
    }
    return -1;
}

void insert_bg_job(bg_job *current_job, pid_t pid, char *command)
{
    bg_job *new_job = malloc(sizeof(struct bg_job));

    new_job->pid = pid;
    new_job->command = command;
    new_job->next_job = NULL;

    if (current_job == NULL)
    {
        new_job->prev_job = NULL;
    }
    else
    {
        current_job->next_job = new_job;
        new_job->prev_job = current_job;
    }

    current_job = malloc(sizeof(struct bg_job));
    memcpy(current_job, new_job, sizeof(struct bg_job));
    free(new_job);
}

void delete_bg_job(bg_job *current_job, bg_job *delete_job)
{
    if (current_job == NULL || delete_job == NULL)
    {
        return;
    }

    if (delete_job == current_job) // if head == del, set head to previous
    {
        current_job = delete_job->prev_job;
    }

    if (delete_job->next_job != NULL) // if it is not first job
    {
        delete_job->next_job->prev_job = delete_job->prev_job;
    }

    if (delete_job->prev_job != NULL) // if it is not last job
    {
        delete_job->prev_job->next_job = delete_job->next_job;
    }

    free(delete_job);
}

int main(int argc, char *argv[])
{
    printf("Welcome to flush \n");

    size_t input_bytes;
    size_t input_size = 2048;
    char *input_str = (char *)malloc(input_size);
    int inp_size = 0;

    int child_status;
    bg_job *current_job = malloc(sizeof(struct bg_job));

    while (1)
    {
        bg_job *temp_prev_job = current_job;
        while (temp_prev_job->command != NULL)
        {
            int ret_status;
            int child_return = waitpid(temp_prev_job->pid, &ret_status, WNOHANG);
            bg_job *temp = malloc(sizeof(struct bg_job));
            temp = temp_prev_job->prev_job;

            if (WIFEXITED(ret_status))
            {
                printf("Exit status [%s] = %d\n", temp_prev_job->command, ret_status);

                delete_bg_job(current_job, temp_prev_job);
            }
            temp_prev_job = temp;
            free(temp);
        }

        print_work_dir();
        input_bytes = getline(&input_str, &input_size, stdin);
        input_str[strcspn(input_str, "\r\n")] = 0;

        char **parsed_input = parse_command(input_str, &inp_size);
        if (parsed_input[0] == NULL)
        {
            continue;
        }

        if (strcmp(parsed_input[0], "cd") == 0)
        {
            char *path;
            if (parsed_input[1] == NULL)
            {
                path = "/";
            }
            else
            {
                path = parsed_input[1];
            }

            if ((chdir(path)) < 0)
            {
                perror("Could not change directory");
            }
            continue;
        }

        if (!strcmp(parsed_input[0], "jobs"))
        {
            bg_job *temp = current_job;
            while (temp->command != NULL)
            {
                printf("[%d]: %s", temp->pid, temp->command);
                temp = temp_prev_job->prev_job;
            }
            continue;
        }

        int IO_index = get_IO_index(parsed_input, &inp_size);
        if (IO_index >= 0)
        {
            printf("Found redirect\n");
        }

        pid_t pid = fork();

        if (pid == 0)
        {
            int ret_status = execvp(parsed_input[0], parsed_input);
            exit(ret_status);
        }
        else if (pid > 0)
        {
            if (strchr(parsed_input[inp_size - 1], '&') != NULL)
            {
                insert_bg_job(current_job, pid, parsed_input[-1]);
            }
            else
            {
                int child_return = waitpid(pid, &child_status, 0);
                printf("Exit status [%s] = %d\n", input_str, child_status);
            }
            free(parsed_input);
        }
        else
        {
            printf(" %s", "Fork failed...");
        }
    }

    return 0;
}