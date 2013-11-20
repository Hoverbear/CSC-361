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
#include <fcntl.h>
#include <sys/time.h>

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
  // Set to non-blocking.
  fcntl(socket_fd, F_SETFL, O_NONBLOCK);
  // Set up statistics
  gettimeofday(&statistics.start_time, NULL);
  //////////////////
  // Sender       //
  //////////////////
  enum system_states system_state = HANDSHAKE;
  // Connect
  int initial_seqno = 0;
  char* buffer = calloc(MAX_PACKET_LENGTH+1, sizeof(char));
  initial_seqno = send_SYN(socket_fd, &peer_address, peer_address_size, &host_address); // Sets the initial random sequence number.
  int system_seqno = initial_seqno;
  packet_t* timeout_queue = NULL; // Used for timeouts. Whenever you send DATs assign the return to this.
  // Used for logging exclusively.
  char log_type; // S, s, R, or r.
  packet_t* packet = NULL;
  for (;;) {
    // First we need something to work on!
    while (packet == NULL) {
      // Read from the socket if there is anything.
      memset(buffer, '\0', MAX_PACKET_LENGTH+1);
      int bytes = recvfrom(socket_fd, buffer, MAX_PACKET_LENGTH+1, 0, (struct sockaddr*) &peer_address, &peer_address_size); // This socket is non-blocking.
      if (bytes == -1) {
        // Didn't read anything.
        // Find a packet that has timed out.
        packet = get_timedout_packet(timeout_queue); // Returns NULL if no packet has timeout.
        if (packet != NULL && packet->timeout.tv_usec == 0) { 
          // fprintf(stderr, "Packet is %d\n", packet);
          log_type = 's'; 
        } else {
          log_type = 'S'; 
        }
      } else {
        // Got a packet, need to parse it.
        packet = parse_packet(buffer);
        // if (packet->ackno < system_seqno) { log_type = 'R'; }
        // else { 
          log_type = 'r';
         // }
      }
    }
    if (log_type) {
      log_packet(log_type, &host_address, &peer_address, packet);
    }
    // By now, packet is something. But what type is it?
    switch (packet->type) {
      case SYN:
        // Wait, why is the reciever sending a SYN?
        fprintf(stderr, "The sender shouldn't receive SYNs. ;)");
        exit(-1);
        break;
      case ACK:
        // Act depending on what's required.
        switch (system_state) {
          case HANDSHAKE:
            system_state = TRANSFER;
            // We're handshaked, start sending files.
            // Don't update the seqno until we get ACKs.
            timeout_queue = send_enough_DAT_to_fill_window(socket_fd, &host_address, &peer_address, 
                              peer_address_size, file, &system_seqno, 
                              packet, timeout_queue, &system_state);
            break;
          case TRANSFER:
            // Drop the packet from timers.
            timeout_queue = remove_packet_from_timers_by_ackno(packet, timeout_queue);
            // if (packet->window == MAX_WINDOW_SIZE_IN_PACKETS * MAX_PAYLOAD_LENGTH) {
              // Send some new data packets to fill that window.
              timeout_queue = send_enough_DAT_to_fill_window(socket_fd, &host_address, &peer_address, 
                              peer_address_size, file, &system_seqno, 
                              packet, timeout_queue, &system_state);
            // }
            break;
          default:
            break;
        }
        break;
      case DAT:
        // This is a packet we need to resend. (Or the reciever sent us a DAT, in that case, wtf mate?)
        // We know there is room here since it timed out. :)
        resend_packet(socket_fd, &peer_address, peer_address_size, packet, &statistics);
        break;
      case RST:
        // Need to restart the connection.
        system_state = RESET;
        // TODO: Rewind file pointer.
        // TODO: Empty packet queue.
        // TODO: Reset the connection, by sending a SYN.
        system_state = HANDSHAKE;
        break;
      case FIN:
        if (log_type == 'r' || log_type == 'R') {// Got a FIN response.
          log_statistics(&statistics, 1);
          exit(0);
        } else {
          resend_packet(socket_fd, &peer_address, peer_address_size, packet, &statistics);
        }
        break;
      default:
        fprintf(stderr, "Packet was invalid type\n");
        break;
    }
    packet = NULL;
  }
  //
  return 0;
}
