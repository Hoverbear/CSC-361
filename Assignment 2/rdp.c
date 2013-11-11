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
#include <unistd.h>       // Sleep
#include <pthread.h>      // Threads! =D!

#include <sys/socket.h>   // Defines const/structs we need for sockets.
#include <netinet/in.h>   // Defines const/structs we need for internet domain addresses.
#include <arpa/inet.h>

// Internal Includes
#include "resources.h"

#define MAX_PAYLOAD 1024
#define TIMEOUT     5
// Potential header values, defined so we can change it later super easily.
#define MAGIC "CSc361"

///////////////////////
// Structures        //
///////////////////////

///////////////////////
// Global Variables. //
///////////////////////
// Arguments.
char*         host_ip;
int           host_port;
char*         peer_ip;
int           peer_port;
char*         file_name;

// The socket FD
int           socket_fd;
// Our all important socket struct.
struct sockaddr_in   host_address;
socklen_t            host_address_size;
// Peer address
struct sockaddr_in   peer_address;
socklen_t            peer_address_size;
// The file we send.
FILE*          file;
// The overall system state.
enum system_role role;
enum system_state state;
// Threads
pthread_t reciever;  // Accepts packets from the reciever. (SYN, ACK, RST, FIN)
pthread_t sender;    // Sends packets to the reciever.     (SYN, DAT, RST, FIN)
int all_done;
// Transactions
transaction* head_transaction;

///////////////////////
// Functions         //
///////////////////////
void parse_render_test() {
  // Test packet parsing and rendering.
  char* test_packet_string = "_magic_ CSc361\r\n_type_ SYN\r\n_seqno_ 1\r\n_ackno_ 10\r\n_length_ 0\r\n_size_ 1000\r\n\r\nSome Data.";
  packet* test_packet_struct = parse_packet(test_packet_string);
  char* test_packet_result = render_packet(test_packet_struct);
  assert(strcmp(test_packet_string, test_packet_result) == 0); // Consistency test. If this fails, we're boned.
  free(test_packet_result);
  free_packet(test_packet_struct);
  return;
}

void* reciever_thread() {
  fprintf(stderr, "Reciever thread spawned.\n");
  while (!all_done) {
    // Prep, read, and start the timer on the packet.i
    // Always build a transaction, even though SYN, WAIT, RST, and FIN don't really handle them.
    int bytes;
    char* buffer = calloc(MAX_PAYLOAD, sizeof(char));
    bytes = recvfrom(socket_fd, buffer, MAX_PAYLOAD, 0, (struct sockaddr*) &peer_address, &peer_address_size); // This populates our peer.
    if (bytes == -1) { // This is an error.
      perror("Bad recieve");
    };
    packet* incoming = parse_packet(buffer);
    fprintf(stderr, "Got a packet:\n   Type: %s\n   Port: %d\n   Address: %s\n", incoming->type, peer_address.sin_port, inet_ntoa(peer_address.sin_addr));
    if (strcmp(incoming->type, "DAT") == 0) {
      // TODO
    } else if (strcmp(incoming->type, "ACK") == 0) {
      // TODO
    } else if (strcmp(incoming->type, "SYN") == 0) {
      // Start packet.
      head_transaction = queue_ACK(head_transaction, incoming->seqno + 1, incoming->seqno, MAX_PAYLOAD, MAX_PAYLOAD);
    } else if (strcmp(incoming->type, "FIN") == 0) {
      // TODO
    } else if (strcmp(incoming->type, "RST") == 0) {
      // TODO
    }
    free(buffer);
  }
  // Recieve packets.
  return (void*) NULL;
}

void* sender_thread() {
  fprintf(stderr, "Sender thread spawned\n");
  while (!all_done) {
    // Prep, read, and start the timer on the packet.i
    // Always build a transaction, even though SYN, WAIT, RST, and FIN don't really handle them.
    int bytes;
    transaction* current_transaction = head_transaction;
    transaction* last_transaction = NULL;
    while (current_transaction != NULL) {
      switch (current_transaction->state) {
        case READY:
        case TIMEDOUT:
          // Needs to be (re)sent.
          bytes = sendto(socket_fd, current_transaction->string, MAX_PAYLOAD, 0, (struct sockaddr*) &peer_address, peer_address_size);
          if (bytes == -1) {
            perror("Wasn't able to transmit");
          }
          current_transaction->state = WAITING;
          fprintf(stderr, "I'm done sending:\n   Type: %s\n   Port: %d\n   Address: %s\n", current_transaction->packet->type, peer_address.sin_port, inet_ntoa(peer_address.sin_addr));
          break;
        case WAITING:
          // Did it time out yet?
          check_timer(current_transaction);
          break;
        case ACKNOWLEDGED:
        case DONE:
          if (last_transaction != NULL) {
            // Can set unlink the transaction.
            last_transaction->tail = current_transaction->tail;
          }
          free_transaction(current_transaction);
          break;
        default:
          perror("Bad state");
      }
      // Need to set the last transaction and step forward.
      last_transaction = current_transaction;
      current_transaction = current_transaction->tail;
    }
    sleep(2);
  }
  return (void*) NULL;
}

// Main
int main(int argc, char* argv[]) {
  parse_render_test();
  // Check number of args.
  switch (argc) {
    case 6:
      // It's a sender!
      host_ip         = argv[1];
      host_port       = atoi(argv[2]);
      peer_ip         = argv[3];
      peer_port       = atoi(argv[4]);
      file_name       = argv[5];
      state           = SYN;
      role            = SENDER;
      // Verify that the file exists.
      file            = fopen(file_name, "r");
      if (file == NULL) { // Couldn't open the file.
        perror("Couldn't open file for reading.");
      }
      // Queue up all of the transactions.
      head_transaction = queue_SYN(head_transaction, 1);
      break;
    case 4:
      // It's a reciever!
      host_ip       = argv[1];
      host_port     = atoi(argv[2]);
      file_name     = argv[3];
      state         = WAIT;
      role          = RECIEVER;
      // Verify we can open the file for writing.
      file = fopen(file_name, "w");
      if (file == NULL) { // Couldn't open the file.
        perror("Couldn't open file for writing.");
      }
      // Transactions will be built as they arrive.
      break;
    default:
      // It's a derp.
      perror("Bad args.");
  }
    // Set up Socket.
  socket_fd                    = socket(AF_INET, SOCK_DGRAM, 0);
  host_address_size            = sizeof(struct sockaddr_in);
  if (socket_fd < 0) { fprintf(stderr, "Couldn't create a socket."); exit(-1); }
  host_address.sin_family      = AF_INET;
  host_address.sin_port        = host_port;
  host_address.sin_addr.s_addr = inet_addr(host_ip);
  // If the Sender, set up the peer network as well.
  if (role == SENDER) {
    peer_address.sin_family      = AF_INET;
    peer_address.sin_port        = peer_port;
    peer_address.sin_addr.s_addr = inet_addr(peer_ip);
  }
  peer_address_size            = sizeof(struct sockaddr_in);

  // Socket Opts
  int socket_ops = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&socket_ops, sizeof(socket_ops)) < 0) {
    perror("Couldn't set socket.");
  }
  // Bind.
  if (bind(socket_fd, (struct sockaddr*) &host_address, sizeof(host_address)) < 0) {
    perror("Couldn't bind to socket");
  }
  
  // Spin up the threads.
  pthread_create(&reciever, 0, reciever_thread, NULL); // Should wait for a ACK, even though we haven't SYN'd.
  pthread_create(&sender,   0, sender_thread,   NULL);     // Start sender after so we make sure we get the ACK.

  // Wait for the threads to join.
  pthread_join(sender, 0);
  pthread_join(reciever, 0);                           // Wait for the last packet.

  // Return
  return 0;
}
