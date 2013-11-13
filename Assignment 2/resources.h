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
#include <time.h>
#define MAX_PAYLOAD 1024
///////////////////////
// Enums             //
///////////////////////
// Transaction States
enum transaction_state {
  // Sender cares about:
  READY,          // Packet ready to send.
  TIMEDOUT,       // Packet timed out.
  // Reciever cares about:
  RECIEVED,       // Packet recieved, not acknowledged.
  ACKNOWLEDGED,   // Packet acknowledged, transaction ok to disgard.
  // Universal:
  WAITING,        // Packet waiting for an ACK or SYN.
  DONE            // Packet done, transaction ok to disgard.
};

enum system_state {
  SYN,        // Sender needs to SYN.
              //   Send SYN --> On ACK --> State = DAT.
  WAIT,       // Reciver is waiting. Will need to build an address on SYN, then ACK.
              //   On SYN --> Send ACK -> State = ACK.
  ACK,        // Data is free flowing. DAT from Sender, ACK from Reciever.
  DAT,        //   On RST --> Reset.
              //   On last DAT --> Wait for last ACK.
              //   On last ACK --> FIN.
  RST,        // Reset the channel whenever.
  FIN         // Finish up. FIN from server, FIN from Reciever. Then quit.
};

enum system_role {
  SENDER,
  RECIEVER
};
///////////////////////
// Structures        //
///////////////////////
// A parsed (Or not yet rendered) packet.
typedef struct packet {
  // char*       magic;    // CSc361 RDP protocol. Not needed, if they don't send this, exit.
  char*       type;     // Type of packet.
  int         ackno;    // Byte sequence number.
  int         seqno;    // Byte acknowledgement number.
  int         length;  // Payload Length in bytes.
  int         size;     // Window Size in bytes.
                        // Empty line goes here.
  char*       data;     // The data of the payload.
} packet;
// A transaction.
typedef struct transaction {
  enum transaction_state state;
  packet* packet;
  char* string;
  time_t fire_time;
  int timeout;
  struct transaction* tail;
} transaction;
///////////////////////
// Functions         //
///////////////////////
char* render_packet(packet* source);
packet* create_packet(void);
packet* parse_packet(char* source);
void free_packet(packet* target);
transaction* create_transaction(void);
void free_transaction(transaction* target);
void set_timer(transaction* target);
void check_timer(transaction* target);
transaction* queue_SYN(transaction* head, int size);
transaction* queue_FIN(transaction* head, int seqno);
transaction* queue_ACK(transaction* head, int seqno, int ackno, int length, int size);
transaction* find_match(transaction* head_transaction, packet* input);
transaction* queue_file_packets(transaction* head_transaction, FILE* file, int start_seqno, int window_size);
void log_packet(char event_type, struct sockaddr_in source, struct sockaddr_in destination, packet* the_packet);
void write_file(transaction* head, FILE* file);
