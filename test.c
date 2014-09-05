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
#include <fcntl.h>

#define ECHO_PORT 9999
#define MAX_FDS 1024
#define DEF_BUF_SIZE 10000


typedef struct
{
    ssize_t size;
    ssize_t offset;
    char* data;
} Buffer;

int close_socket(int sock)
{
    if (close(sock))
    {
        fprintf(stderr, "Failed closing socket.\n");
        return 1;
    }
    return 0;
}

int read_to_buf(int fd, Buffer** bufs)
{
    errno = 0;

    Buffer* buf = bufs[fd];
    int bytes = read(fd, buf->data + buf->offset, buf->size - buf->offset);

    if(bytes > 0){
        buf->offset += bytes;
    }
    if(bytes == buf->size - buf->offset){
        buf->size = buf->size * 2;
        buf->data = realloc(buf->data, buf->size);
    }
    if(errno == EAGAIN || errno == EWOULDBLOCK) bytes = 0;

    //printf("returned %d bytes\n", (int)bytes);
    return bytes;
}

int write_from_buf(int fd, Buffer** bufs)
{
    Buffer* buf = bufs[fd];

    if(buf->offset == 0) return 0;

    int bytes = write(fd, buf->data, buf->offset);

    if(bytes != buf->offset){
        fprintf(stderr, "Error sending to client\n");
    }

    //printf("wrote %d bytes\n", bytes);

    memset(buf->data, 0, buf->size);
    buf->offset = 0;
    return bytes;
}

void new_buf(int fd, Buffer** bufs)
{
    Buffer* buf = malloc(sizeof(Buffer));
    
    buf->size = DEF_BUF_SIZE;
    buf->offset = 0;
    buf->data = calloc(DEF_BUF_SIZE, sizeof(char));

    bufs[fd] = buf;

    printf("NEW BUF %d\n", fd);
    return;
}

int main(int argc, char* argv[])
{
    int sock, new_sock, i, port;
    ssize_t bytes;
    struct sockaddr_in addr;
    fd_set readfds, writefds, readyfds;

    Buffer** bufs = malloc(MAX_FDS*sizeof(Buffer*));

    for(i = 0; i < MAX_FDS; i++)
    {
        new_buf(i, bufs);
    }

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
                    fcntl(new_sock, F_SETFL, fcntl(new_sock, F_GETFL) | O_NONBLOCK);
                    FD_SET(new_sock, &readyfds);
                    //new_buf(new_sock, bufs);
                }
                // connection already established and ready to be written to
                else //if(FD_ISSET(i, &writefds))
                {
                    bytes = 0;

                    bytes = read_to_buf(i, bufs);

                    // if(bytes < 0){
                    //     FD_CLR(i, &readyfds);
                    //     close_socket(i);
                    //     fprintf(stderr, "Error reading from client socket.\n");
                    // }
                    // while((bytes = read_to_buf(i, bufs)) > 0){
                    //     printf("read %d bytes\n", (int)bytes);
                    // }

                    // if(bytes == 0){
                    //     write_from_buf(i, bufs);
                    //     FD_CLR(i, &readyfds);
                    //     close_socket(i);
                    // }
                    if(bytes < 0)
                    {
                        // remove fd from fd_set when closing connection
                        FD_CLR(i, &readyfds);
                        close_socket(i);
                        fprintf(stderr, "Error reading from client socket.\n");
                    }
                }
            }
        }
        for(i = 0; i < FD_SETSIZE; i++)
        {
            if(!FD_ISSET(i, &readfds) && FD_ISSET(i, &writefds))
            {
                if(write_from_buf(i, bufs) < 0)
                {
                    FD_CLR(i ,&readyfds);
                    close_socket(i);
                    fprintf(stderr, "Error writing to client\n");
                }
            }
        } 
    }
    close_socket(sock);
    //free(bufs);
    return EXIT_SUCCESS;
}
