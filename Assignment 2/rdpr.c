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
char*         sender_ip;
int           sender_port;
char*         reciever_ip;
int           reciever_port;
char*         sender_file_name;

// The socket FD
int           socket_fd;
// Our all important socket struct.
struct sockaddr_in   socket_address;
socklen_t            socket_address_size;
// The file we send.
FILE*          sender_file;
// The overall system state.
enum system_state state;
// Threads
pthread_t reciever;  // Accepts packets from the reciever. (SYN, ACK, RST, FIN)
pthread_t sender;    // Sends packets to the reciever.     (SYN, DAT, RST, FIN)
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

void* reciever_thread(void* parameter) {
  fprintf(stderr, "Reciever thread spawned.\n");
  int done = 0;
  listen(socket_fd, 5);
  while (!done) {
    char* message = calloc( MAX_PAYLOAD, sizeof(char) );
    int bytes = recvfrom(socket_fd, message, MAX_PAYLOAD, 0, (struct sockaddr*) &socket_address, &socket_address_size);
    assert(bytes != -1); // This is an error.
    fprintf(stderr, "%s", message);
    done = 1;
  }
  // Recieve packets.
  return (void*) NULL;
}

void* sender_thread(void* parameter) {
  fprintf(stderr, "Sender thread spawned\n");
  int done = 0;
  while (!done) {
    char* message = calloc( MAX_PAYLOAD, sizeof(char) );
    strcat(message, "Bears!\0");
    sendto(socket_fd, message, MAX_PAYLOAD, 0, (struct sockaddr*) &socket_address, socket_address_size);
    done = 1;
  }
  return (void*) NULL;
}

// Main
int main(int argc, char* argv[]) {
  parse_render_test();
  // Check number of args.
  assert( !((argc > 6) || (argc < 6)) );
  // Parse Args.
  sender_ip         = argv[1];
  sender_port       = atoi(argv[2]);
  reciever_ip       = argv[3];
  reciever_port     = atoi(argv[4]);
  sender_file_name  = argv[5];
  // Set up Socket.
  socket_fd                     = socket(AF_INET, SOCK_DGRAM, 0);
  socket_address_size           = sizeof(socket_fd);
  if (socket_fd < 0) { fprintf(stderr, "Couldn't create a socket."); exit(-1); }
  socket_address.sin_family     = AF_INET;
  socket_address.sin_port       = htons(sender_port);
  socket_address.sin_addr.s_addr  = inet_addr(sender_ip);
  // Socket Opts
  int socket_ops = 1;
  assert(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&socket_ops, sizeof(socket_ops)) >= 0);
  // Bind.
  assert(bind(socket_fd, (struct sockaddr*) &socket_address, sizeof(socket_address)) >= 0);
  // Verify that the file exists.
  sender_file = fopen(sender_file_name, "r");
  assert(sender_file != NULL); // Couldn't open the file.
  // Spin up the threads.
  pthread_create(&reciever, 0, reciever_thread, NULL); // Should wait for a ACK, even though we haven't SYN'd.
  pthread_create(&sender, 0, sender_thread, NULL);     // Start sender after so we make sure we get the ACK.

  // Wait for the threads to join.
  pthread_join(sender, 0);
  pthread_join(reciever, 0);                           // Wait for the last packet.


  // Start the listen loop.
//  while (!done) {
//    transaction* the_transaction = calloc(1, sizeof(struct transaction));
//    the_transaction->string = calloc(MAX_PAYLOAD, sizeof(char));
//    the_transaction->state = INITIALIZED;
//    
//    // Switch based on State.
//    switch (state) {
//      case SYN:
//        break;
//      case DATorACK:
//        break;
//      case RST:
//        break;
//      case FIN:
//        break;
//      case default:
//        fprintf(stderr, "Invalid system state");
//        exit(-1);
//    }
//    sendto(socket_fd, the_transaction->string, MAX_PAYLOAD, 0, (struct sockaddr*) &socket_address, socket_address_size);
//    
//    fprintf(stderr, "Got some data\n");
//
//    done = 1;
//
//  };
//  // Return
  return 0;
}
