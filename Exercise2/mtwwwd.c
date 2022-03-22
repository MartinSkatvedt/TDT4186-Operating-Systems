#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

char *read_file(char *path)
{
    FILE *file;
}

int handle_request(char *wwwpath, char *req, char **res)
{
    strtok(req, " ");
    char *file_path = strtok(NULL, " ");
    char *full_path = strncat(wwwpath, file_path, 64);
    if (!access(full_path, F_OK))
    {
        perror("404 not found");
        exit(1);
    }
    *res = read_file(full_path);
    return 1;
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
    socklen_t socklen;
    struct sockaddr_in addr;

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
        size_t buffer_size = 1024;
        char *buffer = malloc(buffer_size);
        char *res = malloc(sizeof(char) * 10000);
        if (listen(sock, 10) < 0)
        {
            perror("server: listen");
            exit(1);
        }
        printf("Listening for connections...\n");

        if ((new_sock = accept(sock, (struct sockaddr *)&addr, &socklen)) < 0)
        {
            perror("server: accept");
            exit(1);
        }
        printf("Accepted connection\n");

        if (recv(new_sock, buffer, buffer_size, 0) < 0)
        {
            perror("server: recv");
            exit(1);
        }
        printf("Recieved request\n");

        if (handle_request(wwwpath, buffer, &res) < 0)
        {
            perror("server: handle");
            exit(1);
        }
        free(buffer);
        printf("Handled request\n");

        if (write(new_sock, res, strlen(res)) < 0)
        {
            perror("server: write");
            exit(1);
        }
        free(res);
        printf("Wrote response back\n");

        if (close(new_sock) < 0)
        {
            perror("server: close");
            exit(1);
        }
        printf("Closed connection\n");
    }
    close(sock);
    return 0;
}