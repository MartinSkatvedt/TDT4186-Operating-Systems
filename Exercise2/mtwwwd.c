#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FILE_404 "/404.html"

int read_file(char *path, char **buffer, size_t *buffer_size)
{
    FILE *file;
    struct stat file_stat;

    if ((file = fopen(path, "r")) == NULL)
    {
        perror("Unable to open file");
        return -1;
    }

    if (fstat(fileno(file), &file_stat) != 0)
    {
        perror("Unable to get info of file");
        fclose(file);
        return -1;
    }

    *buffer_size = file_stat.st_size;
    *buffer = malloc(sizeof(char) * (file_stat.st_size));

    if (fread(*buffer, sizeof(char), file_stat.st_size, file) == 0)
    {
        perror("Unable to read file");
        fclose(file);
        return -1;
    }

    fclose(file);
    return 0;
}

int prepare_response(char *wwwpath, char *req, char **res, size_t *res_size)
{
    strtok(req, " ");
    char *file_path = strtok(NULL, " ");
    char *full_path = malloc(strlen(wwwpath) + strlen(file_path) + 1);
    sprintf(full_path, "%s%s", wwwpath, file_path);
    if ((read_file(full_path, res, res_size)) < 0)
    {
        sprintf(full_path, "%s%s", wwwpath, FILE_404);
        printf("Full path: %s\n", full_path);
        if ((read_file(full_path, res, res_size)) < 0)
        {
            perror("Cannot read 404 file");
            return -1;
        }
    }
    free(full_path);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Incorrect argument amount.\n Usage: mtwwwd www-path port #threads #bufferslots\n");
        return 1;
    }
    char *wwwpath = argv[1];
    int port = atoi(argv[2]);
    int threads = atoi(argv[3]);
    int bufferslots = atoi(argv[4]);
    // printf("wwwpath: %s\nport: %d\n#threads: %d\n#bufferslots: %d\n\n", wwwpath, port, threads, bufferslots);

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
        int req_buffer_size = 1024;
        char *req_buffer;

        size_t res_buffer_size;
        char *res_buffer;

        if (listen(sock, 10) < 0)
        {
            perror("Unable to listen for connections");
            exit(1);
        }
        printf("Listening for connections...\n");

        if ((new_sock = accept(sock, (struct sockaddr *)&addr, &socklen)) < 0)
        {
            perror("Unable to accept connection");
            continue;
        }
        printf("Accepted connection\n");

        req_buffer = malloc(req_buffer_size);

        if (recv(new_sock, req_buffer, req_buffer_size, 0) < 0)
        {
            perror("Unable to recieve request");
            close(new_sock);
            continue;
        }
        printf("Recieved request\n");

        if (prepare_response(wwwpath, req_buffer, &res_buffer, &res_buffer_size) < 0)
        {
            perror("Unable to prepare response");
            close(new_sock);
            continue;
        }
        free(req_buffer);
        printf("Prepared response\n");

        if (write(new_sock, res_buffer, strlen(res_buffer)) < 0)
        {
            perror("Unable to write response back");
            close(new_sock);
            continue;
        }
        free(res_buffer);
        printf("Wrote response back\n");

        if (close(new_sock) < 0)
        {
            perror("Unable to close connection");
            exit(1);
        }
        printf("Closed connection\n");
    }
    close(sock);
    return 0;
}