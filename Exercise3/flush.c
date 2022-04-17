#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

typedef struct bg_job // Struct for background jobs
{
    pid_t pid;               // PID of background job
    char *command;           // Command of background job
    struct bg_job *next_job; // Next job
    struct bg_job *prev_job; // Previous job
} bg_job;

int detect_input_redirect(char **args, char **input_file_name)
{
    int i;
    for (i = 0; args[i] != NULL; i++)
    {
        if (args[i][0] == '<')
        {
            if (args[i + 1] == NULL)
            {
                return -1;
            }
            *input_file_name = args[i + 1];

            while (args[i - 1] != NULL)
            {
                args[i] = args[i + 2];
                i++;
            }
            return 1;
        }
    }
    return 0;
}

int detect_output_redirect(char **args, char **output_file_name)
{
    int i;
    for (i = 0; args[i] != NULL; i++)
    {
        if (args[i][0] == '>')
        {
            if (args[i + 1] == NULL)
            {
                return -1;
            }
            *output_file_name = args[i + 1];

            while (args[i - 1] != NULL)
            {
                args[i] = args[i + 2];
                i++;
            }
            return 1;
        }
    }
    return 0;
}
char **parse_command(char *inp_command, int *inp_size) // Splits input into array
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

int print_work_dir() // Prints workdir
{
    char current_dir[100];
    if (getcwd(current_dir, sizeof(current_dir)) != NULL)
    {
        printf("%s: ", current_dir);
    }
    else
    {
        perror("Could not get working directory \n");
        return 1;
    }
    return 0;
}

void insert_bg_job(bg_job **head, pid_t pid, char *command) // Insert new job as head
{
    bg_job *new_job = malloc(sizeof(struct bg_job)); // Allocate memory for new node
    new_job->pid = pid;                              // Set PID of child
    new_job->command = command;                      // Set the command

    new_job->next_job = (*head); // Set next as NULL or head
    new_job->prev_job = NULL;    // Set prev to NULL

    if ((*head) != NULL) // If list is not empy, set the heads previous as new node
    {
        (*head)->prev_job = new_job;
    }
    (*head) = new_job;
}

void delete_bg_job(bg_job **head, bg_job *delete_job) // Deletes job from linked list
{
    if ((*head) == NULL || delete_job == NULL)
    {
        return; // Return if both are NULL
    }

    if ((*head) == delete_job) // if the head is the node to be deleted
    {
        (*head) = delete_job->next_job;
    }

    if (delete_job->prev_job != NULL) // if the node is in the middle
    {
        delete_job->prev_job->next_job = delete_job->next_job;
    }

    free(delete_job);
}

void display_current_jobs(bg_job *head) // Displays all jobs
{
    printf("Printing jobs... \n");
    while (head != NULL)
    {
        printf("[%d]: %s \n", head->pid, head->command);
        head = head->next_job;
    }

    if (head == NULL)
    {
        printf("NULL \n");
    }
}

int main(int argc, char *argv[])
{
    printf("Welcome to flush \n");

    size_t input_bytes;
    size_t input_size = 2048;
    char *input_str = (char *)malloc(input_size);
    int inp_size = 0;
    int is_bg_task = 0; // Bool if command ends with &

    // For redirection
    int redir_in = 0;
    int redir_out = 0;
    char *redir_in_file;
    char *redir_out_file;

    int child_status;
    bg_job *head = NULL;

    while (1)
    {
        bg_job *temp_next_job = head;
        while (temp_next_job != NULL)
        {
            int ret_status;
            int child_return = waitpid(temp_next_job->pid, &ret_status, WNOHANG);

            bg_job *temp = temp_next_job->next_job;
            if (WIFEXITED(ret_status))
            {
                printf("Exit status [%s] = %d\n", temp_next_job->command, ret_status);
                delete_bg_job(&head, temp_next_job);
            }
            temp_next_job = temp;
        }

        print_work_dir();
        input_bytes = getline(&input_str, &input_size, stdin);

        int inp_length = strlen(input_str);                      // Length of input
        is_bg_task = (input_str[inp_length - 2] == '&') ? 1 : 0; // Checks if command ends with &
        input_str[strcspn(input_str, "&\r\n")] = 0;              // Removed newline and &

        char **parsed_input = parse_command(input_str, &inp_size);
        if (parsed_input[0] == NULL)
        {
            continue;
        }

        if (strcmp(parsed_input[0], "cd") == 0) // Internal implementation of cd
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

        if (!strcmp(parsed_input[0], "jobs")) // List all bg jobs
        {
            display_current_jobs(head);
            continue;
        }

        if ((redir_in = detect_input_redirect(parsed_input, &redir_in_file)) == -1)
        {
            printf("Redirect: missing input file\n");
            continue;
        }

        if ((redir_out = detect_output_redirect(parsed_input, &redir_out_file)) == -1)
        {
            printf("Redirect: missing output file\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            if (redir_in)
            {
                freopen(redir_in_file, "r", stdin);
            }

            if (redir_out)
            {
                freopen(redir_out_file, "w+", stdout);
            }
            int ret_status = execvp(parsed_input[0], parsed_input);
            exit(ret_status);
        }
        else if (pid > 0)
        {
            if (is_bg_task)
            {
                insert_bg_job(&head, pid, parsed_input[0]);
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