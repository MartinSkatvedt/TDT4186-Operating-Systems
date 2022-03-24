#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bbuffer.h"
#include <pthread.h>

#define FILE_404 "/404.html"

struct BNDBUF *bbuffer;
int req_buffer_size = 1000;
char *wwwpath;

// Reads a file into a buffer
int read_file(char *path, char **buffer)
{
    FILE *file;
    struct stat file_stat;

    // Open the file for reading
    if ((file = fopen(path, "r")) == NULL)
    {
        perror("Unable to open file");
        return -1;
    }

    // Get size of file and allocate buffer memory
    if (fstat(fileno(file), &file_stat) != 0)
    {
        perror("Unable to get info of file");
        fclose(file);
        return -1;
    }
    *buffer = malloc(sizeof(char) * (file_stat.st_size));

    // Read entire file into buffer
    if (fread(*buffer, sizeof(char), file_stat.st_size, file) == 0)
    {
        perror("Unable to read file");
        fclose(file);
        return -1;
    }

    // Close the file
    fclose(file);
    return 0;
}

// Fills the response buffer with the appropriate file
int prepare_response(char *wwwpath, char *req, char **res)
{
    // Get file path
    strtok(req, " ");
    char *file_path = strtok(NULL, " ");
    char *full_path = malloc(strlen(wwwpath) + strlen(file_path) + 1);
    sprintf(full_path, "%s%s", wwwpath, file_path);

    // Read file or 404 file
    if ((read_file(full_path, res)) < 0)
    {
        sprintf(full_path, "%s%s", wwwpath, FILE_404);
        if ((read_file(full_path, res)) < 0)
        {
            perror("Cannot read 404 file");
            return -1;
        }
    }
    free(full_path);
    return 0;
}

void *request_handler()
{
    printf("New thread\n");

    char *req_buffer = malloc(req_buffer_size);
    char *res_buffer;

    while (1)
    {
        int thread_socket = bb_get(bbuffer);

        if (recv(thread_socket, req_buffer, req_buffer_size, 0) < 0)
        {
            perror("Unable to recieve request");
            close(thread_socket);
            continue;
        }
        printf("Recieved request\n");

        if (prepare_response(wwwpath, req_buffer, &res_buffer) < 0)
        {
            perror("Unable to prepare response");
            close(thread_socket);
            continue;
        }
        free(req_buffer);
        printf("Prepared response\n");

        if (write(thread_socket, res_buffer, strlen(res_buffer)) < 0)
        {
            perror("Unable to write response back");
            close(thread_socket);
            continue;
        }
        free(res_buffer);

        printf("Wrote response back\n");

        if (close(thread_socket) < 0)
        {
            perror("Unable to close connection");
            exit(1);
        }
        printf("Closed connection\n");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Incorrect argument amount.\n Usage: mtwwwd www-path port #threads #bufferslots\n");
        return 1;
    }
    wwwpath = argv[1];
    int port = atoi(argv[2]);
    int n_threads = atoi(argv[3]);
    int bufferslots = atoi(argv[4]);
    // printf("wwwpath: %s\nport: %d\n#threads: %d\n#bufferslots: %d\n\n", wwwpath, port, threads, bufferslots);

    bbuffer = bb_init(bufferslots);
    if (bbuffer == NULL)
    {
        printf("Could not create buffer");
    }

    pthread_t threads[n_threads];
    int thread_args[n_threads];

    for (int i = 0; i < n_threads; i++)
    {
        thread_args[i] = i;
        int res = pthread_create(&threads[i], NULL, &request_handler, &thread_args[i]);
    }

    // sprintf(" %d", bbuffer->size);
    int sock, new_sock;
    struct sockaddr_in addr;
    socklen_t socklen;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) <= 0)
    {
        perror("server: socket");
        exit(1);
    }
    printf("Socket has been created\n");

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        perror("server: bind");
        exit(1);
    }
    printf("Socket has been bound\n");

    while (1)
    {

        if (listen(sock, 10) < 0)
        {
            perror("Unable to listen for connections");
            close(sock);
            exit(1);
        }
        printf("Listening for connections...\n");

        if ((new_sock = accept(sock, (struct sockaddr *)&addr, &socklen)) < 0)
        {
            perror("Unable to accept connection");
            continue;
        }
        printf("Accepted connection\n");
        bb_add(bbuffer, new_sock);
    }
    close(sock);
    return 0;
}
