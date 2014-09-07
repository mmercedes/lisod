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
#include "log.h"

#define MAX_FDS 1024
#define DEF_BUF_SIZE 10000
#define USAGE "./lisod <HTTP port> <HTTPS port> <log file> <lock file> <www folder> <CGI script path> <private key file> <certificate file>\n"


// struct to contain an IO buffer with size and current offset fields
typedef struct
{
    ssize_t size;
    ssize_t offset;
    char* data;
} Buffer;

/**************************  helper funcitons  *******************************/

// wrapper for close()
int close_socket(int sock);

// reads from a fd into it's own buffer in the bufs array
int read_to_buf(int fd, Buffer** bufs);

// writes from a fd's buffer in the bufs array to the fd
int write_from_buf(int fd, Buffer** bufs);

// creates a new buf in the bufs array for a fd
void new_buf(int fd, Buffer** bufs);

// free an individual buf in the bufs array
void free_buf(int fd, Buffer** bufs);

// frees the buf array and all bufs in it 
void free_bufs(Buffer** bufs);


/****************************  BEGIN main   **********************************/

int main(int argc, char* argv[])
{
    int sock, new_sock, i, port;
    ssize_t bytes;
    struct sockaddr_in addr;
    fd_set readfds, writefds, readyfds, wreadyfds;

    Buffer** bufs = malloc(MAX_FDS*sizeof(Buffer*));

    if(argc < 2){
        printf(USAGE);
        return 0;
    }
    log_start("log.txt");
    log_msg(L_ERR, "testing log %d %s", 120, "loglog");
    log_end();

    port = atoi(argv[1]);

    fprintf(stdout, "----- Echo Server on port %d-----\n", port);
    
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

    // clear the fd_sets
    FD_ZERO (&readyfds);
    FD_ZERO(&wreadyfds);
    FD_SET (sock, &readyfds);

    while(1)
    {
        readfds = readyfds;
        writefds = wreadyfds;
        if(select(FD_SETSIZE, &readfds, &writefds, NULL, NULL) < 0)
        {
            fprintf(stderr, "select() returned error.\n");
            return EXIT_FAILURE;
        }

        // loop thorugh the set of fds
        for(i = 0; i < FD_SETSIZE; i++)
        {
            if(FD_ISSET(i, &readfds))
            {
                // new connection request
                if(i == sock)
                {
                    // accept connection and add it to the fd_set
                    if((new_sock = accept(sock, NULL, NULL)) < 0)
                    {
                        fprintf(stderr, "accept() returned error.\n");
                    }
                    else
                    {
                        FD_SET(new_sock, &readyfds);
                        new_buf(new_sock, bufs);
                    }
                }
                // connection already established and ready to be read from
                else
                {
                    bytes = 0;
                    bytes = read_to_buf(i, bufs);

                    if(bytes <= 0)
                    {
                        // remove fd from fd_set when closing connection
                        FD_CLR(i, &readyfds);
                        close_socket(i);
                        free_buf(i, bufs);
                    }
                    else
                    {
                        FD_SET(i, &wreadyfds);
                    }
                }
            }
            else if(FD_ISSET(i, &writefds))
            {
                if(write_from_buf(i, bufs) < 0)
                {
                    FD_CLR(i, &wreadyfds);
                    FD_CLR(i, &readyfds);
                    close_socket(i);
                    free_buf(i, bufs);
                    fprintf(stderr, "Error writing to client\n");
                }
                FD_CLR(i, &wreadyfds);
            }
        }
    }
    close_socket(sock);
    free_bufs(bufs);
    return EXIT_SUCCESS;
}

/****************************  END main   ************************************/


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
    Buffer* buf = bufs[fd];
    int bytes = read(fd, buf->data + buf->offset, buf->size - buf->offset);

    if(bytes > 0)
    {
        if(bytes == buf->size - buf->offset)
        {
            buf->size = buf->size * 2;
            buf->data = realloc(buf->data, buf->size);
        }
        buf->offset += bytes;
    }
    return bytes;
}

int write_from_buf(int fd, Buffer** bufs)
{
    Buffer* buf = bufs[fd];

    int bytes = write(fd, buf->data, buf->offset);

    if(bytes != buf->offset)
    {
        fprintf(stderr, "Error sending to client\n");
    }

    memset(buf->data, 0, buf->size);
    buf->offset = 0;
    return bytes;
}

void new_buf(int fd, Buffer** bufs)
{
    Buffer* buf = malloc(sizeof(Buffer));
    
    buf->size = DEF_BUF_SIZE;
    buf->offset = 0;
    buf->data = malloc(DEF_BUF_SIZE*sizeof(char));

    bufs[fd] = buf;

    return;
}

void free_buf(int fd, Buffer** bufs)
{
    free(bufs[fd]->data);
    free(bufs[fd]);
    return;
}

void free_bufs(Buffer** bufs)
{
    int i;

    for(i = 0; i < MAX_FDS; i++)
    {
        if(bufs[i]->data != NULL) free(bufs[i]->data);
        free(bufs[i]);
    }
    free(bufs);
    return;
}