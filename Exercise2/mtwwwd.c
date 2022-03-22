#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    int buffer_size = 1024;
    char *buffer = malloc(buffer_size);
    struct sockaddr_in addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) > 0)
    {
        printf("Socket has been created\n");
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0)
    {
        printf("Socket has been bound\n");
    }

    while (1)
    {
        if (listen(sock, 10) < 0)
        {
            perror("server: listen");
            exit(1);
        }

        if ((new_sock = accept(sock, (struct sockaddr *)&addr, &socklen)) < 0)
        {
            perror("server: accept");
            exit(1);
        }

        if (recv(new_sock, buffer, buffer_size, 0) < 0)
        {
            perror("server: recv");
            exit(1);
        }

        if (write(new_sock, "hello world\n", 12) < 0)
        {
            perror("server: write");
            exit(1);
        }

        if (close(new_sock) < 0)
        {
            perror("server: close");
            exit(1);
        }
    }
    close(sock);
    return 0;
}