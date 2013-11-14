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

#define MAX_PACKET_LENGTH 1024

// Example Packet:
// CHECKSUMHERE\nCSc361 DAT 3 2 1000 1001\n\nDATAHERE
// CHECKSUMHERE\nCSc361 DAT 3 2 100 1001\n\nDATAHERE

// Packet Specifications:
// <Checksum>
// <_Magic_> <_Type_> <_Seqno_> <_Ackno_> <_Length_> <_Window_>
// 
// <_Data_>

// Possible Packet Types
enum packet_type {
  DAT,
  ACK,
  SYN,
  FIN,
  RST
};

// The Packet Structure
typedef struct packet_t {
  // Checksum, then Magic.
  enum packet_type    type;           // Type of packet.
  unsigned short      seqno;          // Byte sequence number.
  unsigned short      ackno;          // Byte acknowledgement number.
  unsigned short      payload;        // Payload Length in bytes.
  unsigned short      window;         // Window Size in bytes.
                                      // Checksum goes here.
                                      // Empty line goes here.
  char*               data;           // The data of the payload.
} packet_t;

unsigned long hash(char *str);
packet_t* parse_packet(char* source);
char* render_packet(packet_t* source);
