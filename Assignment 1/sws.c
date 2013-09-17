
#include <sys/types.h>    /* Defines data types used in system calls. */
#include <stdio.h>        /* Standard IO. */
#include <sys/socket.h>   /* Defines const/structs we need for sockets. */
#include <netinet/in.h>   /* Defines const/structs we need for internet domain addresses. */
#include <string.h>       /* String functions */
#include <stdlib.h>       /* Builtin functions */

/*
 * A simple web server.
 * Args:
 *  - PORT: The port to use.
 *  - DIR:  The directory to get files from.
 */
int main(int argc, char *argv[]) {
  int socket_fd,      /* Socket File Descriptor. */
      new_socket_fd,  /* Socket File Descriptor. */
      port,           /* The Port we'll use. */
      client_length,  /* Size of the client's address. (Used for `accept`) */
      num_chars;      /* Number of characters written by `read()` and `write()` */

  /* Server and Client Addresses. */
  struct sockaddr_in server_address,
                     client_address; 
  /* In sockaddr_in:
      short   sin_family
      u_short sin_port
      struct  in_addr sin_addr
     In in_addr:
      u_long  s_addr
  */

  /* The directory we're serving from. */
  char dir[256];

  /* Buffer for reading/writing characters from the socket */
  char buffer[1024];

  /* TODO: Add files to read from! */

  /* Do we have PORT and DIR args? */
  if (argc < 3) {
    fprintf(stderr, "Usage: sws PORT DIR\n");
    fprintf(stderr, "   Ex: sws 8080 tmp\n");
    exit(1);
  }

  /* Assign our Port */
  port = atoi(argv[1]);
  /* Assign our dir */
  strncpy(dir, argv[2], 256);

  /* Just print some debugging garbage. */
  fprintf(stderr, "%d, %s\n", port, dir);
}