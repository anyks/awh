// g++ --std=c++11 srv.cpp -o ./srv -Wno-deprecated-declarations

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

int main(void)
{
    int socket_fd;
    struct sockaddr_un server_address, client_address;
    int bytes_received, bytes_sent, integer_buffer;
    socklen_t address_length = sizeof(struct sockaddr_un);

    if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
    {
        perror("server: socket");
        return -1;
    }
   
    memset(&server_address, 0, sizeof(server_address));
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, "#UdsServer");
    server_address.sun_path[0] = 0;
   
    if (bind(socket_fd,
             (const struct sockaddr *) &server_address,
             address_length) < 0)
    {
        close(socket_fd);
        perror("server: bind");
        return -1;
    }

    for (;;)
    {
        bytes_received = recvfrom(socket_fd,
                                  &integer_buffer,
                                  sizeof(integer_buffer),
                                  0,
                                  (struct sockaddr *) &client_address,
                                  &address_length);

        if(bytes_received != sizeof(integer_buffer))
        {
            printf("Error: recvfrom - %d.\n", bytes_received);
        } else {
            printf("received: %d.\n", integer_buffer);

            integer_buffer += 10;

            bytes_sent = sendto(socket_fd,
                                &integer_buffer,
                                sizeof(integer_buffer),
                                0,
                                (struct sockaddr *) &client_address,
                                address_length);
        }
    }

    close(socket_fd);

    return 0;
}
