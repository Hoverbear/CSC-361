
#include <sys/types.h>    /* Defines data types used in system calls. */
#include <stdio.h>        /* Standard IO. */
#include <sys/socket.h>   /* Defines const/structs we need for sockets. */
#include <netinet/in.h>   /* Defines const/structs we need for internet domain addresses. */
#include <string.h>       /* String functions */
#include <stdlib.h>       /* Builtin functions */
#include <termios.h>      /* Keypress testing. */
#include <unistd.h>       /* Keypress testing.

/*
 * A simple web server.
 * Args:
 *  - PORT: The port to use.
 *  - DIR:  The directory to get files from.
 */
int main(int argc, char *argv[]) {
  int socket_fd,         /* Socket File Descriptor. */
      client_socket_fd,  /* Socket File Descriptor. */
      port,              /* The Port we'll use. */
      client_length,     /* Size of the client's address. (Used for `accept`) */
      num_chars;         /* Number of characters written by `read()` and `write()` */

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

  /* Create our socket. */
  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "Couldn't get a socket. Perhaps your computer is bonkers.");
    exit(1);
  }

  /* Clear the buffer. */  
  bzero((char *) &server_address, sizeof(server_address));

  /* Set up the Server Address Struct */
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);        /* htons() converts to host byte order! */
  server_address.sin_addr.s_addr = INADDR_ANY;  /* IP of us */

  /* Bind to the socket. */
  if (bind(socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
    fprintf(stderr, "Error on binding to port: %d\n", port);
    exit(1);
  }
  
  /* Start listening, only allow 5 waiting people */
  listen(socket_fd, 5);

  /* Trying keypresses
   * Thanks to: http://stackoverflow.com/posts/7411735/revisions
   */
  struct termios old,new;
  tcgetattr(fileno(stdin),&old);
  tcgetattr(fileno(stdin),&new);
  cfmakeraw(&new);
  tcsetattr(fileno(stdin),TCSANOW,&new);
  fputs("Press any key to continue.",stdout);
  fflush(NULL);
  int key = fgetc(stdin);
  tcsetattr(fileno(stdin),TCSANOW,&old);

  if (key == 113) {
    exit(0);
  }

  /* Start up an `accept()` call, which will block the process until we get a connection. */
  client_length = sizeof(client_address);
  client_socket_fd = accept(socket_fd, (struct sockaddr*) &client_address, &client_length);
  if (client_socket_fd < 0) {
    fprintf(stderr, "Error accepting a connection on port: %d\n", port);
  }


  /* Clear our read buffer, then read into it. */
  bzero(buffer, 1024);
  while (1) {
    num_chars = read(client_socket_fd, buffer, 1024);
    if (num_chars < 0)
    {
      fprintf(stderr, "Error reading from the socket on port: %d\n", port);

    }

    /* Print the message we got. */
    fprintf(stderr, "%s", buffer);
  }

  /* Cleanly exit */
  exit(0);
}