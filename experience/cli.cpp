// g++ --std=c++11 cli.cpp -o ./cli -Wno-deprecated-declarations

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int socket_fd;
    struct sockaddr_un server_address, client_address;
    int bytes_received, bytes_sent, integer_buffer;
    socklen_t address_length = sizeof(struct sockaddr_un);

    if((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("client: socket");
        return -1;
    }
         
    memset(&client_address, 0, sizeof(client_address));
    client_address.sun_family = AF_UNIX;
    strcpy(client_address.sun_path, "#UdsClient");
    client_address.sun_path[0] = 0;

    if(bind(socket_fd,
            (const struct sockaddr *) &client_address,
            address_length) < 0)
    {
        perror("client: bind");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, "#UdsServer");
    server_address.sun_path[0] = 0;

    integer_buffer = 1;

    for (;;)
    {
       bytes_sent = sendto(socket_fd,
                           &integer_buffer,
                           sizeof(integer_buffer),
                           0,
                           (struct sockaddr *) &server_address,
                           address_length);

       bytes_received = recvfrom(socket_fd,
                                 &integer_buffer,
                                 sizeof(integer_buffer),
                                 0,
                                 (struct sockaddr *) &server_address,
                                 &address_length);

       if (bytes_received != sizeof(integer_buffer))
       {
           printf("Error: recvfrom - %d.\n", bytes_received);
           return -1;
       }

       printf("received: %d\n", integer_buffer);

       integer_buffer += 1;

       sleep(10);
    }

    close(socket_fd);

    return 0;
}
