///////////////////////
// Andrew Hobden     //
// V00788452         //
///////////////////////
#include <stdlib.h>       // Builtin functions
#include <stdio.h>        // Standard IO.
#include <sys/types.h>    // Defines data types used in system calls.
#include <string.h>       // String functions.
#include <errno.h>
#include <assert.h>       // Needed for asserts.

#include <pthread.h>      // Threads! =D!

#include <sys/socket.h>   // Defines const/structs we need for sockets.
#include <netinet/in.h>   // Defines const/structs we need for internet domain addresses.
#include <arpa/inet.h>

// Internal Includes
#include "resources.h"

#define MAX_PAYLOAD 1024
// Potential header values, defined so we can change it later super easily.
#define MAGIC "CSc361"
#define DAT   "DAT"
#define ACK   "ACK"
#define SYN   "SYN"
#define FIN   "FIN"
#define RST   "RST"

///////////////////////
// Structures        //
///////////////////////

///////////////////////
// Global Variables. //
///////////////////////
// Arguments.
char*         reciever_ip;
int           reciever_port;
char*         reciever_file_name;

// The socket FD
int           socket_fd;
// Our all important socket struct.
struct sockaddr_in   socket_address;
// The file we recieve.
FILE          sender_file;
// Is the file done?
int           done;

///////////////////////
// Functions         //
///////////////////////
void parse_render_test() {
  // Test packet parsing and rendering.
  char* test_packet_string = "_magic_ CSc361\r\n_type_ SYN\r\n_seqno_ 1\r\n_ackno_ 10\r\n_length_ 100\r\n_size_ 1000\r\n\r\nSome Data.";
  packet* test_packet_struct = parse_packet(test_packet_string);
  char* test_packet_result = render_packet(test_packet_struct);
  assert(strcmp(test_packet_string, test_packet_result) == 0); // Consistency test. If this fails, we're boned.
  free(test_packet_result);
  free_packet(test_packet_struct);
  return;
}
// Main
int main(int argc, char* argv[]) {
  parse_render_test();
  // Check number of args.
  assert( !((argc > 4) || (argc < 4)) );
  // Parse Args.
  reciever_ip       = argv[1];
  reciever_port     = atoi(argv[2]);
  reciever_file_name  = argv[3];
  // Set up Socket.
  socket_fd                     = socket(AF_INET, SOCK_DGRAM, 0);
  socklen_t socket_address_size = sizeof(socket_address);
  if (socket_fd < 0) { fprintf(stderr, "Couldn't create a socket."); exit(-1); }
  socket_address.sin_family     = AF_INET;
  socket_address.sin_port       = htons(reciever_port);
  socket_address.sin_addr.s_addr  = inet_addr(reciever_ip);
  // Socket Opts
  int socket_ops = 1;
  assert(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&socket_ops, sizeof(socket_ops)) >= 0);
  // Bind
  assert(bind(socket_fd, (struct sockaddr*) &socket_address, sizeof(socket_address)) >= 0);
  listen(socket_fd, 5); // Listen to 5 connections at a time.
  // Start the listen loop.
  done = 0;
  while (!done) {
    transaction* the_transaction = calloc(1, sizeof(struct transaction));
    the_transaction->string = calloc(MAX_PAYLOAD, sizeof(char));
    the_transaction->state = INITIALIZED;
    int bytes = recvfrom(socket_fd, the_transaction->string, MAX_PAYLOAD, 0, (struct sockaddr*) &socket_address, &socket_address_size);
    assert(bytes != -1); // This is an error.
    
    fprintf(stderr, "Got some data");

    done = 1;
  };
  // Return
  return 0;
}
