#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

typedef struct bg_job // Struct for background jobs
{
    pid_t pid; // PID of background job
    char *command; // Command of background job
    struct bg_job *next_job; // Next job
    struct bg_job *prev_job; // Previous job
} bg_job;

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

void insert_bg_job(bg_job **head, pid_t pid, char *command) // Insert new job as head
{
    bg_job *new_job = malloc(sizeof(struct bg_job)); // Allocate memory for new node
    new_job->pid = pid; // Set PID of child
    new_job->command = command; // Set the command
   
    new_job->next_job = (*head); // Set next as NULL or head
    new_job->prev_job = NULL; // Set prev to NULL

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

  if (delete_job->prev_job != NULL) //if the node is in the middle
  {
      delete_job->prev_job->next_job = delete_job->next_job;
  }

  free(delete_job);
}

void display_current_jobs(bg_job *head) // Displays all jobs
{
    bg_job *temp_job = head;

    printf("Printing jobs... \n");
    while (temp_job != NULL)
    {
        printf("[%d]: %s \n", temp_job->pid, temp_job->command);
        temp_job = temp_job->next_job;
    }
    printf("\n");
}

int main(int argc, char *argv[])
{
    printf("Welcome to flush \n");

    size_t input_bytes;
    size_t input_size = 2048;
    char *input_str = (char *)malloc(input_size);
    int inp_size = 0;

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
        input_str[strcspn(input_str, "\r\n")] = 0;

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

        int IO_index = get_IO_index(parsed_input, &inp_size); // check for IO redirect
        if (IO_index >= 0)
        {
            printf("Found redirect\n");
        }

        pid_t pid = fork();


        if (pid == 0)
        {
            if (strchr(parsed_input[inp_size - 1], '&') != NULL)
            {
                parsed_input[inp_size - 1] = NULL;
            }

            int ret_status = execvp(parsed_input[0], parsed_input);
            exit(ret_status);
        }
        else if (pid > 0)
        {
            if (strchr(parsed_input[inp_size - 1], '&') != NULL)
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