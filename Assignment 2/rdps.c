///////////////////////
// Andrew Hobden     //
// V00788452         //
///////////////////////
#include <stdlib.h>       // Builtin functions
#include <stdio.h>        // Standard IO.
#include <sys/types.h>    // Defines data types used in system calls.
#include <string.h>       // String functions.
#include <errno.h>

#include <pthread.h>      // Threads! =D!

#include <sys/socket.h>   // Defines const/structs we need for sockets.
#include <netinet/in.h>   // Defines const/structs we need for internet domain addresses.
#include <arpa/inet.h>

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
// A parsed (Or not yet rendered) packet.
typedef struct packet {
  char*       magic;    // CSc361 RDP protocol.
  char*       type;     // Byte sequence number.
  int         seqno;    // Byte acknowledgement number.
  int         payload;  // Payload Length in bytes.
  int         size;     // Window Size in bytes.
                      // Empty line goes here.
  char*       data;     // The data of the payload.
} packet;

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
// The file we send.
FILE          sender_file;

///////////////////////
// Functions         //
///////////////////////
// Render a packet based on the struct given.
char* render_packet(packet source) {
  char* result = calloc( 1024, sizeof(char) );
  if (result == NULL) { fprintf(stderr, "Couldn't render a packet."); exit(-1); }
  // TODO
  return result;
}

// Parse out a packet into a struct given the data.
packet* parse_packet(char* source) {
  packet* result = calloc( 1, sizeof(struct packet) );
  if (result == NULL) { fprintf(stderr, "Couldn't parse a packet."); exit(-1); }
  // TODO
  return result;
}

// Main
int main(int argc, char* argv[]) {
  // Check number of args.
  if ( (argc > 6) || (argc < 6) ) {
    fprintf(stderr, "Bad args.\n"); // TODO
    exit(-1);
  }
  // Parse Args.
  sender_ip         = argv[1];
  sender_port       = atoi(argv[2]);
  reciever_ip       = argv[3];
  reciever_port     = atoi(argv[4]);
  sender_file_name  = argv[5];
  // Set up Socket.
  socket_fd                     = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) { fprintf(stderr, "Couldn't create a socket."); exit(-1); }
  socket_address.sin_family     = AF_INET;
  socket_address.sin_port       = htons(sender_port);
  socket_address.sin_addr.s_addr  = inet_addr(sender_ip);
  // Connect.
  char* test_packet_string = "_magic_ CSc361\r\n_type_ SYN\r\n_seqno_ 1\r\n_length_ 1\r\n_size_ 1\r\n\r\nSome Data.";
  packet* test_packet_struct = parse_packet(test_packet_string);
  char* test_packet_result = render_packet(*test_packet_struct);
  // Spawn workers on new connections, dispatch on existing.
  
  // Return
  return 0;
}
