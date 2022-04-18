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

void pwd() // Prints working directory
{
    int pwd_buf_sz = 256;
    char *current_dir;

    current_dir = malloc(pwd_buf_sz * sizeof(char));

    if (!current_dir)
    {
        perror("pwd");
        exit(EXIT_FAILURE);
    }

    if (getcwd(current_dir, pwd_buf_sz * sizeof(char)) == NULL)
    {
        perror("Could not get working directory\n");
        free(current_dir);
        exit(EXIT_FAILURE);
    }
    printf("%s: ", current_dir);
    free(current_dir);
}

char **parse_input(char *input) // Splits input into array
{
    int tkns_buf_sz = 64;
    char *tkn;
    char **tkns = malloc(tkns_buf_sz * sizeof(char *)); // array of strtok tokens
    int i = 0;

    if (!tkns)
    {
        perror("Parse command allocation failed\n");
        exit(EXIT_FAILURE);
    }

    tkn = strtok(input, " \t");
    while (tkn != NULL)
    {
        tkns[i] = tkn;
        i++;

        // Check if we have space for more tokens
        if (i >= tkns_buf_sz)
        {
            tkns_buf_sz += tkns_buf_sz;
            tkns = realloc(tkns, tkns_buf_sz * sizeof(char *)); // Allocate more memory
            if (!tkns)
            {
                // Allocation error
                printf("Parse command reallocation failed\n");
                exit(EXIT_FAILURE);
            }
        }

        tkn = strtok(NULL, " \t");
    }

    tkns[i] = NULL; // Last index is NULL
    return tkns;
}

int detect_input_redirect(char **args, char **input_file_name)
{
    int i;
    for (i = 0; args[i] != NULL; i++)
    {
        // Find the '<' symbol
        if (args[i][0] == '<')
        {
            // '<' found...
            if (args[i + 1] == NULL)
            {
                // but no file following
                return -1;
            }
            *input_file_name = args[i + 1];

            // Move the rest of the args appropriately
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
        // Find the '>' symbol
        if (args[i][0] == '>')
        {
            // '>' found...
            if (args[i + 1] == NULL)
            {
                // but no file following
                return -1;
            }
            *output_file_name = args[i + 1];

            // Move the rest of the args appropriately
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

int detect_bg_job(char *input) // Checks if command ends with &
{
    return (input[strlen(input) - 2] == '&') ? 1 : 0;
}

void insert_bg_job(bg_job **head, pid_t pid, char *command) // Insert new job as head
{
    bg_job *new_job = malloc(sizeof(struct bg_job)); // Allocate memory for new node
    new_job->pid = pid;                              // Set PID of child

    new_job->command = malloc(sizeof(command));
    strcpy(new_job->command, command); // Set the command

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
    free(delete_job->command);
    free(delete_job);
}

void kill_bg_job(bg_job **head, bg_job *delete_job) // Kill process on exit program
{
    kill(delete_job->pid, SIGKILL);
    delete_bg_job(head, delete_job);
}

void display_current_jobs(bg_job *head) // Displays all jobs
{
    while (head != NULL)
    {
        printf("[%d]: %s \n", head->pid, head->command);
        head = head->next_job;
    }
}

char *read_input(bg_job **head)
{
    char *input;
    size_t buf_sz = 0;

    if (getline(&input, &buf_sz, stdin) == -1)
    {
        if (feof(stdin))
        {
            while (*head != NULL) // Deletes all bg jobs if ctrl + d is pressed
            {
                kill_bg_job(head, *head);
            }
            exit(EXIT_SUCCESS);
        }
        else
        {
            perror("read input");
            exit(EXIT_FAILURE);
        }
    }
    return input;
}

int main(int argc, char *argv[])
{
    printf("Welcome to flush \n");

    char *input;
    char **parsed_input;

    int is_bg_task; // Bool if command ends with &

    // For redirection
    int redir_in, redir_out;
    char *redir_in_file, *redir_out_file;

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
            if (child_return > 0)
            {
                printf("Exit status [%s] = %d\n", temp_next_job->command, ret_status);
                delete_bg_job(&head, temp_next_job);
            }
            temp_next_job = temp;
        }

        pwd();

        input = read_input(&head); // Get user input

        is_bg_task = detect_bg_job(input); // Check if command should be sent to background

        input[strcspn(input, "&\r\n")] = 0; // Remove trailing characters

        parsed_input = parse_input(input); // Parse input to args

        if (parsed_input[0] == NULL)
        {
            // Empty command
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

        // Detect input redirection
        if ((redir_in = detect_input_redirect(parsed_input, &redir_in_file)) == -1)
        {
            printf("Redirect: missing input file\n");
            continue;
        }

        // Detect output redirection
        if ((redir_out = detect_output_redirect(parsed_input, &redir_out_file)) == -1)
        {
            printf("Redirect: missing output file\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            // Child
            if (redir_in)
            {
                freopen(redir_in_file, "r", stdin); // Redirect input to file instead of stdin
            }

            if (redir_out)
            {
                freopen(redir_out_file, "w+", stdout); // Redirect output to file instead of stdout
            }
            execvp(parsed_input[0], parsed_input); // Execute command
            exit(EXIT_FAILURE);                    // Exec calls should never return, something went wrong
        }
        else if (pid > 0)
        {
            // Parent
            if (is_bg_task)
            {
                insert_bg_job(&head, pid, parsed_input[0]); // Send to background
            }
            else
            {
                int child_return = waitpid(pid, &child_status, 0); // Wait for child to finish
                printf("Exit status [%s] = %d\n", input, child_status);
            }
            free(input);
            free(parsed_input);
        }
        else
        {
            printf(" %s", "Fork failed...");
        }
    }

    return 0;
}