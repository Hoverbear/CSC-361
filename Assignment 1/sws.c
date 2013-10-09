#include <sys/types.h>    /* Defines data types used in system calls. */
#include <stdio.h>        /* Standard IO. */
#include <sys/socket.h>   /* Defines const/structs we need for sockets. */
#include <netinet/in.h>   /* Defines const/structs we need for internet domain addresses. */
#include <sys/stat.h>
#include <arpa/inet.h>
#include <string.h>       /* String functions */
#include <stdlib.h>       /* Builtin functions */
#include <pthread.h>      /* Threads! =D! */
#include <termios.h>      /* Keypress testing. */
#include <unistd.h>       /* Keypress testing. */
#include <errno.h>

#define IN_PACKETSIZE   1024
#define PATHSIZE        256

/* Request Struct
 * --------------
 * Used for threading requests.
 */
 struct request {
  int origin_socket;
  struct sockaddr_in address;
  int address_size;
  char buffer[PATHSIZE];
  char dir[PATHSIZE];
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
  new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ICRNL);
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
  /* Setup */
  struct request *req = pointer;
  char* status;
  char path[PATHSIZE]; /* The file that is requested. */
  char* response;

  /* Find the start of the path */ 
  int path_start = 0;
  while (req->buffer[path_start] != ' ') { /* GET_/ HTTP/1.0 */
    path_start += 1;
  }
  path_start += 1;

  /* Find the end of the path */
  int path_end = path_start + 1;
  while (req->buffer[path_end] != ' ') { /* GET /_HTTP/1.0 */
    path_end += 1;
  }

  if (path_end < path_start) {
    fprintf(stderr, "Malformed URL\n");
    exit(-1);
  }
  /* Copy the path */
  strncpy(path, &req->buffer[path_start], path_end - path_start);

  /* Is it an index page? If so, make it look at the index. */
  if (path[strlen(path)-1] == '/') {
      strncat(path, "index.html", 12);
  }

  /* Get the file, dump it to file_read, set the status. */
  FILE* target;
  char* file_read; /* The file contents requested. */
  long file_size = 0;
  char* full_path = strncat(req->dir,path, 256);
  /* See if we can read. */
  struct stat fileStat;
  if ((strncmp(path, "/", 1) != 0) || (strncmp(path, "/../", 4) == 0)) {
    status = "400 Bad Request";
  } else if (stat(full_path,&fileStat) < 0) {
    /* There is no file */
    status = "404 Not Found";
  }
  else if (!(fileStat.st_mode & S_IRUSR)) {
    /* We can't read the file */
    status = "400 Bad Request";
  }
  else {
    /* We can read the file */
    target = fopen(full_path, "r");
    /* We have a file, we need to read it. */
    fseek (target , 0 , SEEK_END);
    file_size = ftell (target);
    rewind (target);
    file_read = calloc(file_size, sizeof(char));

    if (fread(file_read, file_size, file_size, target) == 0) {
      /* We can't read from the file for some reason. */
      fprintf(stderr, "Reached end of file unexpectedly.\n");
      exit(-1);
    }
    else {
      /* We successfully read from the file, respond the the request. */
      status = "200 OK";
    }
  }

  /* Build the header. */
  /* In this Assignment we just need the HTTP and the file (if applicable) */
  int head_size = 9 + strlen(status) + 4;
  response = calloc(file_size + head_size, sizeof(char)); /* The UDP Packet. */
  strncat(response, "HTTP/1.0 ", 9);
  strncat(response, status, 25);
  strncat(response, "\r\n\r\n", 4); /* Emit two blank lines. */
  if (file_read != NULL) {
    strncat(&response[head_size], file_read, file_size);
  }

  int loc = 0;
  while (loc < file_size + head_size) {
    int limit;
    if (strlen(&response[loc]) <= 576) {
      limit = strlen(&response[loc]);
    }
    else {
      limit = 576;
    }
    if (sendto(req->origin_socket, &response[loc], limit, 0, (struct sockaddr *)&req->address, req->address_size) == -1) {
      fprintf(stderr, "Failed to respond to client: %s\n", inet_ntoa(req->address.sin_addr));
    }
    loc += limit;
  }

  /* Finally, log the message. */
  time_t the_time;
  time (&the_time);
  struct tm* timeinfo =  localtime(&the_time); /* Time struct, contains localtime information. */
  char time_buffer[16];
  strftime (time_buffer,15,"%b %d %H:%M:%S", timeinfo); /* Formats the time as needed. */
  int req_length =  14 + (path_end - path_start) -1; /* Figure out how long our request was. */
  fprintf(stdout, "%s %s:%d %.*s; %.*s; %s\n", time_buffer, inet_ntoa(req->address.sin_addr), req->address.sin_port, req_length, req->buffer, head_size -4, response, path);

  /* Cleanup */
  /*free(pointer);
  free(file_read);
  free(response);*/
  return((void *) 0); 
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
  char dir[PATHSIZE];

  /* Do we have PORT and DIR args? */
  if (argc < 3) {
    fprintf(stderr, "Usage: sws <port> <dir>\n");
    fprintf(stderr, "   Ex: sws 8080 tmp\n");
    exit(-1);
  }

  /* Assign our Port */
  port = atoi(argv[1]);
  /* Assign our dir */
  strncpy(dir, argv[2], PATHSIZE);

  fprintf(stdout, "sws is running on UDP port %d and serving %s\npress 'q' to quit ...\n", port, dir);

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
    strncpy(request->dir, dir, PATHSIZE);

    /* Get! */
    int bytes = recvfrom(request->origin_socket, &request->buffer, IN_PACKETSIZE, 0, (struct sockaddr *)&request->address, (socklen_t*) &request->address_size);
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
  close(socket_fd);
  pthread_join(quit_thread, 0);
  exit(0);
}
