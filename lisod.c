/**********************************************************
* echo_server.c                                           *
*                                                         *
* Author: Matthew Mercedes <mmercede@andrew.cmu.edu>      *
*                                                         *
**********************************************************/

#include <netinet/in.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#define ECHO_PORT 9999

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int main(int argc, char* argv[])
{
    int sock, new_sock, i, port;
    ssize_t bytes, buf_size;
    struct sockaddr_in addr;
    char* buf;
    fd_set readfds, writefds, readyfds;

    buf_size = 1000000;
    buf = malloc(buf_size*sizeof(char));

    if(argc < 2){
        port = ECHO_PORT;
    }
    else port = atoi(argv[1]);

    fprintf(stdout, "----- Echo Server -----\n");

    
    if((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        fprintf(stderr, "Failed creating socket.\n");
        return EXIT_FAILURE;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sock, (struct sockaddr *) &addr, sizeof(addr)))
    {
        close_socket(sock);
        fprintf(stderr, "Failed binding socket.\n");
        return EXIT_FAILURE;
    }

    if(listen(sock, 5))
    {
        close_socket(sock);
        fprintf(stderr, "Error listening on socket.\n");
        return EXIT_FAILURE;
    }

    // clear the set of ready fds
    FD_ZERO (&readyfds);
    FD_SET (sock, &readyfds);

    while(1)
    {
        readfds = readyfds;
        writefds = readyfds;
        if(select(FD_SETSIZE, &readfds, &writefds, NULL, NULL) < 0)
        {
            fprintf(stderr, "select() returned error.\n");
            return EXIT_FAILURE;
        }

        // loop thorugh the set of fds
        for(i = 0; i < FD_SETSIZE; i++)
        {
            // the ith fd is ready to be read from
            if(FD_ISSET(i, &readfds))
            {
                //printf("reading fd %d\n", i);
                // new connection request
                if(i == sock)
                {
                    // accept the new connection and add it to the fd_set
                    if((new_sock = accept(sock, NULL, NULL)) < 0)
                    {
                        fprintf(stderr, "accept() returned error.\n");
                        return EXIT_FAILURE;
                    }
                    FD_SET(new_sock, &readyfds);
                }
                // connection already established and ready to be written to
                else if(FD_ISSET(i, &writefds))
                {
                    bytes = 0;
                    bytes = read(i, buf, buf_size);

                    printf("read %d bytes\n", (int)bytes);
                    if(bytes <= 0)
                    {
                        // remove fd from fd_set when closing connection
                        FD_CLR(i, &readyfds);
                        close_socket(i);
                        if(bytes == -1) fprintf(stderr, "Error reading from client socket.\n");
                    }
                    else if(write(i, buf, bytes) != bytes)
                    {
                        FD_CLR(i, &readyfds);
                        close_socket(i);
                        fprintf(stderr, "Error sending to client.\n");
                    }
                    //printf("wrote %d bytes\n", (int)bytes);

                    if(bytes == buf_size){
                        free(buf);
                        buf_size = buf_size*2;
                        buf = malloc(buf_size*sizeof(char));
                        //printf("BUF_SIZE DOUBLED\n");
                    }
                    else memset(buf, 0, buf_size);
                }
            }
        } 
    }
    close_socket(sock);
    free(buf);
    return EXIT_SUCCESS;
}
