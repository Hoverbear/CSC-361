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

// System states. Should be self descriptive.
enum system_states {
  HANDSHAKE,
  TRANSFER,
  RESET,
  EXIT
};

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
  // Not in the actual packet.
  time_t              timeout;        // The time the packet times out.
} packet_t;

// Runtime statistics.
typedef struct statistics_t {            // (S/R Label) (Description)
  int    total_data;                     // (Sent/Recieved) Total of all bytes.
  int    unique_data;                    // (Sent/Recieved) Total of only unique bytes.
  int    total_packets;                  // (Sent/Recieved) Total number of data packets.
  int    unique_packets;                 // (Sent/Recieved) Total of only unique packets.
  int    SYN;                            // (Sent/Recieved) Number of SYN packets.
  int    FIN;                            // (Sent/Recieved) Number of FIN packets.
  int    RST;                            // (Sent/Recieved) Number of RST packets.
  int    ACK;                            // (Recieved/Sent) Number of ACK packets.
  int    RST_2;                          // (Recieved/Sent) Number of RST packets (inverse!).
  time_t start_time;                     // The start time, use it to calculate the total time.
} statistics_t;

//////////////////
// Packets      //
//////////////////
// Returns a hash for the packet.
unsigned long hash(char *str);
// Parses the string to a packet_t struct.
packet_t* parse_packet(char* source);
// Builds a string from the packet_t struct.
char* render_packet(packet_t* source);

// Sets the initial sequence number.
unsigned short send_SYN(sockaddr_in* peer_address, socklen_t* peer_address_size);
// Finds (if applicable) a timed out packet from the queue.
packet_t* get_timedout_packet(packet_t* timeout_queue);
// Sends enough DAT packets to fill up the window give.
packet_t* send_enough_DAT_to_fill_window(sockaddr_in* peer_address, socklen_t* peer_address_size,
                       FILE* file, short position, short window_size, packet* timeout_queue);
// Send an ACK for the given seqno.
void send_ACK(sockaddr_in* peer_address, socklen_t peer_address_size, short seqno);
// (Re)send a DAT packet.
packet_t* send_DAT(sockaddr_in* peer_address, socklen_t* peer_address_size, packet_t* packet, packet_t* timeout_queue);
// Remove packets up to the given packet's ackno.
packet_t* remove_packet_from_timers_by_ackno(packet_t* packet);

//////////////////
// Logging      //
//////////////////
// Logs the given packet.
void log_packet(char event_type, sockaddr_in* source, sockaddr_in* destination, packet_t* the_packet);
// Outputs the log file.
void log_statistics(statistics_t statistics);
