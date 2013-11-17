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

// The statistics for the session.
statistics_t         statistics;

// The socket FD
int                  socket_fd;
// Our all important socket struct.
struct sockaddr_in   host_address;
socklen_t            host_address_size;
// Peer address
struct sockaddr_in   peer_address;
socklen_t            peer_address_size;
// The file we send.
FILE*                file;
// The overall system state.

int main(int argc, char* argv[]) {
  if (argc != 4) {
    fprintf(stderr,"  rdpr receiver_ip receiver_port receiver_file_name");
    exit(-1);
  }
  // It's a reciever!
  host_ip       = argv[1];
  host_port     = atoi(argv[2]);
  file_name     = argv[3];
  // Verify we can open the file for writing.
  file = fopen(file_name, "w");
  
  // Set up Host.
  socket_fd                    = socket(AF_INET, SOCK_DGRAM, 0);
  host_address_size            = sizeof(struct sockaddr_in);
  if (socket_fd < 0) { fprintf(stderr, "Couldn't create a socket."); exit(-1); }
  host_address.sin_family      = AF_INET;
  host_address.sin_port        = host_port;
  host_address.sin_addr.s_addr = inet_addr(host_ip);
  peer_address_size            = sizeof(struct sockaddr_in);
  // Socket Opts
  int socket_ops = 1;
  if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &socket_ops, sizeof(socket_ops)) < 0) {
    perror("Couldn't set socket.");
  }
  // Bind.
  if (bind(socket_fd, (struct sockaddr*) &host_address, sizeof(host_address)) < 0) {
    perror("Couldn't bind to socket");
  }
  //////////////////
  // Reciever     //
  //////////////////
  unsigned short initial_seqno;
  unsigned short system_seqno;
  packet_t* file_head = NULL;
  packet_t* temp_head;
  int window_size = MAX_WINDOW_SIZE_IN_PACKETS;
  char* buffer = calloc(MAX_PACKET_LENGTH+1, sizeof(char));
  enum system_states system_state = HANDSHAKE;
  for (;;) {
    memset(buffer, '\0', MAX_PACKET_LENGTH);
    // First we need something to work on!
    packet_t* packet;
    int bytes = recvfrom(socket_fd, buffer, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, &peer_address_size); // This socket is blocking.
    if (bytes == -1) {
      fprintf(stderr, "Error in receiving.\n");
    }
    packet = parse_packet(buffer);
    if (packet == NULL) {
      fprintf(stderr, "== Packet corrupt, dropped == \n%s\n== END ==\n", buffer);
      continue;
    }
    log_packet('r', &host_address, &peer_address, packet);
    // By now, packet is something. But what type is it?
    switch (packet->type) {
      case SYN:
        // Got a SYN request, awesome. Need to ACK it and get ready for DATA.
        system_state = TRANSFER;
        initial_seqno = packet->seqno;
        system_seqno = initial_seqno;
        send_ACK(socket_fd, &host_address, &peer_address, peer_address_size, packet->seqno, (unsigned short) (MAX_PAYLOAD_LENGTH * window_size));
        break;
      case ACK:
        // Wait, why is the reciever getting an ACK?
        fprintf(stderr, "You probably want to send an ACK on the reciever. ;)");
        exit(-1);
        break;
      case DAT:
        // Write the data into the window, that function will flush it to file and update the seqno if it has all the packets in a contiguous order.
        temp_head = file_head;
        file_head = write_packet_to_window(packet, file_head, file, &window_size); // THIS UPDATED WINDOW_SIZE
        send_ACK(socket_fd, &host_address, &peer_address, peer_address_size, file_head->seqno, (unsigned short) (MAX_PAYLOAD_LENGTH * window_size));
        break;
      case RST:
        system_state = RESET;
        // TODO: Rewind file pointer.
        // TODO: Empty the window.
        // TODO: Reset the connection, by sending an ACK.
        send_ACK(socket_fd, &host_address, &peer_address, peer_address_size, packet->seqno, (unsigned short) (MAX_PAYLOAD_LENGTH * window_size));
        system_state = HANDSHAKE;
        break;
      case FIN:
        system_state = EXIT;
        // Finished the file. We can send an ACK and close up shop.
        send_ACK(socket_fd, &host_address, &peer_address, peer_address_size, packet->seqno, (unsigned short) (MAX_PAYLOAD_LENGTH * window_size));
        log_statistics(statistics);
        exit(0);
        break;
    }
  }
  // 
  return 0;
}
