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
#include "shared.h"

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

int main(int argc, char* argv[]) {
  if (argc != 6) {
    fprintf(stderr,"  rdps sender_ip sender_port receiver_ip receiver_port sender_file_name");
    exit(-1);
  }
  // It's a sender!
  host_ip         = argv[1];
  host_port       = atoi(argv[2]);
  peer_ip         = argv[3];
  peer_port       = atoi(argv[4]);
  file_name       = argv[5];
  // Verify that the file exists.
  file            = fopen(file_name, "r");
  // Set up Host.
  socket_fd                    = socket(AF_INET, SOCK_DGRAM, 0);
  host_address_size            = sizeof(struct sockaddr_in);
  if (socket_fd < 0) { fprintf(stderr, "Couldn't create a socket."); exit(-1); }
  host_address.sin_family      = AF_INET;
  host_address.sin_port        = host_port;
  host_address.sin_addr.s_addr = inet_addr(host_ip);
  // Set up peer.
  peer_address.sin_family      = AF_INET;
  peer_address.sin_port        = peer_port;
  peer_address.sin_addr.s_addr = inet_addr(peer_ip);
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
  // BEGIN DIFFERENT CODE
  // Make a SYN
  packet_t syn_packet;
  syn_packet.type = SYN;
  syn_packet.seqno = (unsigned short) rand();
  syn_packet.ackno = 0;
  syn_packet.payload = 0;
  syn_packet.window = 0;
  syn_packet.data = calloc(1, sizeof(char));
  strcpy(syn_packet.data, "");
  char* syn_string = render_packet(&syn_packet);
  sendto(socket_fd, syn_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, peer_address_size);
  // Prep, read, and start the timer on the packet.
  int bytes;
  char* buffer = calloc(MAX_PACKET_LENGTH, sizeof(char));
  bytes = recvfrom(socket_fd, buffer, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, &peer_address_size); // This populates our peer.
  if (bytes == -1) { // This is an error.
    perror("Bad recieve");
  };
  packet_t* incoming = parse_packet(buffer);
  fprintf(stderr, "%s", buffer);
  // END DIFFERENT CODE
  //
  return 0;
}
