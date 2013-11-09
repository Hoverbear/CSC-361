///////////////////////
// Enums             //
///////////////////////
// Transaction States
enum transaction_state {
  // Sender cares about:
  READY,
  WAITING,
  TIMEDOUT,
  // Reciever cares about:
  RECIEVED,
  ACKNOWLEDGED,
  // Universal:
  INITIALIZED,
  DONE
};

enum system_state {
  SYN,
  DATorACK,
  RST,
  FIN
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
} transaction;
///////////////////////
// Functions         //
///////////////////////
char* render_packet(packet* source);
packet* parse_packet(char* source);
void free_packet(packet* target);
void free_transaction(transaction* target);
