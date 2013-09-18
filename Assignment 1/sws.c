
#include <sys/types.h>    /* Defines data types used in system calls. */
#include <stdio.h>        /* Standard IO. */
#include <sys/socket.h>   /* Defines const/structs we need for sockets. */
#include <netinet/in.h>   /* Defines const/structs we need for internet domain addresses. */
#include <arpa/inet.h>
#include <string.h>       /* String functions */
#include <stdlib.h>       /* Builtin functions */
#include <pthread.h>      /* Threads! =D! */
#include <termios.h>      /* Keypress testing. */
#include <unistd.h>       /* Keypress testing. */

/* Request Struct
 * --------------
 * Used for threading requests.
 */
struct request {
  int origin_socket;
  struct sockaddr_in address;
  int address_size;
  char buffer[256];
};

/* Quit Worker
 * -----------
 * Called via a pthread to watch for exit signals.
 */
void *quit_worker() {
  int key;

  /* --- BEGIN DERIVED CODE :: Found http://stackoverflow.com/questions/2984307/c-key-pressed-in-linux-console/2984565#2984565 */
  struct termios org_opts, new_opts;
  int res=0;
      /*-----  store old settings -----------*/
  res=tcgetattr(STDIN_FILENO, &org_opts);
      /*---- set new terminal parms --------*/
  memcpy(&new_opts, &org_opts, sizeof(new_opts));
  new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
  do {                  /* CHANGED */
    key=getchar();      /* CHANGED */
  } while(key != 113);  /* CHANGED */
      /*------  restore old settings ---------*/
  res=tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
  /* --- END DERIVED CODE */
  exit(0);
  return((void *) 0); 
}

/* Request Worker
 * --------------
 * Called via pthread to respond to a socket request.
 * Params:
 *   - struct request pointer
 */ 
 void *request_worker(void *pointer) {
  struct request *req = pointer;
  fprintf(stderr, "IP: '%s'\n", inet_ntoa(req->address.sin_addr));
  fprintf(stderr, "Buffer: '%s'\n", req->buffer);
  return((void *) 0); 

  free(pointer);
 }

/*
 * A simple web server.
 * Params:
 *  - port: The port to use.
 *  - dir:  The directory to get files from.
 */
int main(int argc, char *argv[]) {
  int socket_fd,         /* Socket File Descriptor. */
      port;              /* The Port we'll use. */

  /* Server Addresses. */
  struct sockaddr_in server_address;
  /* In sockaddr_in:
      short   sin_family
      u_short sin_port
      struct  in_addr sin_addr
     In in_addr:
      u_long  s_addr
  */

  /* The directory we're serving from. */
  char dir[256];

  /* Do we have PORT and DIR args? */
  if (argc < 3) {
    fprintf(stderr, "Usage: sws PORT DIR\n");
    fprintf(stderr, "   Ex: sws 8080 tmp\n");
    exit(-1);
  }

  /* Assign our Port */
  port = atoi(argv[1]);
  /* Assign our dir */
  strncpy(dir, argv[2], 256);

  /* Create our socket. */
  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    fprintf(stderr, "Couldn't get a socket. Perhaps your computer is bonkers.");
    exit(-1);
  }

  /* Clear the buffer. */  
  bzero((char *) &server_address, sizeof(server_address));

  /* Set up the Server Address Struct */
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);        /* htons() converts to host byte order! */
  server_address.sin_addr.s_addr = INADDR_ANY;  /* IP of us */

  /* Bind to the socket. */
  if (bind(socket_fd, (struct sockaddr*) &server_address, sizeof(server_address)) < 0) {
    close(socket_fd);
    fprintf(stderr, "Error on binding to port: %d\n", port);
    exit(-1);
  }

  /* Spin up a worker thread. */
  pthread_t quit_thread;
  pthread_create(&quit_thread, 0, quit_worker, 0);

  do {
    /* Buffer for reading/writing characters from the socket */
    struct request *request = calloc(1, sizeof *request);
    request->origin_socket = socket_fd;
    request->address_size = sizeof(request->address);

    /* Get! */
    int bytes = recvfrom(request->origin_socket, &request->buffer, 255, 0, (struct sockaddr *)&request->address, (socklen_t*) &request->address_size);
    if (bytes == -1) {
      fprintf(stderr, "Error reading from socket.\n");
      exit(-1);
    }

    /* terminate the string */
    request->buffer[bytes] = '\0';

    /* Spin up a client thread whenever we get one */
    pthread_t request_thread;
    pthread_create(&request_thread, 0, request_worker, request);
  } while (1);

  /* Cleanly exit */
  pthread_join(quit_thread, 0);
  exit(0);
}
