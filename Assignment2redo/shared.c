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

#include "shared.h"
char* packet_type_array[] = { "DAT", "ACK", "SYN", "FIN", "RST" };

// http://www.cse.yorku.ca/~oz/hash.html djb2 hash.
unsigned long hash(char *str) {
  unsigned long hash = 5381;
  int c;
  while ((c = *str++)) {
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  }
  return hash;
}

packet_t* parse_packet(char* source) {
  packet_t* result = calloc(1, sizeof(struct packet_t));
  char* without_checksum;
  // Check the checksum.
  unsigned long checksum = strtoul(source, &without_checksum, 10);
  without_checksum++; // \n goes here.
  if (checksum != hash(without_checksum)) {
    return NULL;
  }
  // Check the magic.
  if (strncmp(without_checksum, "CSc361", 6) != 0) {
    return NULL;
  }
  without_checksum += 7; // Include the space.
  // Check the Type.
  int i;
  for (i=0; i<5; i++) {
    if (strncmp(without_checksum, packet_type_array[i], 3) == 0) {
      result->type = i;
      break;
    }
  }
  without_checksum += 3; // Don't include space, do later.
  // Get the seqno.
  without_checksum++;
  result->seqno = (unsigned short) strtoul(without_checksum, &without_checksum, 10);
  // Get the ackno.
  without_checksum++;
  result->ackno = (unsigned short) strtoul(without_checksum, &without_checksum, 10);
  // Get the payload.
  without_checksum++;
  result->payload = (unsigned short) strtoul(without_checksum, &without_checksum, 10);
  // Get the payload.
  without_checksum++;
  result->window = (unsigned short) strtoul(without_checksum, &without_checksum, 10);
  // Get the data!
  without_checksum++;
  result->data = calloc(MAX_PACKET_LENGTH, sizeof(char));
  strncpy(result->data, without_checksum, MAX_PACKET_LENGTH);
  return result;
}

char* render_packet(packet_t* source) {
  char* pre_checksum = calloc(MAX_PACKET_LENGTH+1, sizeof(char));
  // I love sprintf!
  sprintf(pre_checksum, "CSc361 %s %d %d %d %d\n%s", packet_type_array[source->type],
          source->seqno, source->ackno, source->payload, source->window, source->data);
  char* result = calloc(MAX_PACKET_LENGTH+1, sizeof(char));
  sprintf(result, "%lu\n%s", hash(pre_checksum), pre_checksum);
  return result;
}

// A self test
// int main() {
//   packet_t test_packet;
//   test_packet.type = SYN;
//   test_packet.seqno = 1;
//   test_packet.ackno = 10;
//   test_packet.payload = 101;
//   test_packet.window = 1000;
//   test_packet.data = calloc(MAX_PACKET_LENGTH, sizeof(char));
//   strcpy(test_packet.data, "This is a string.");
//   
//   char* test_packet_as_string = render_packet(&test_packet);
//   packet_t* test_packet_as_packet = parse_packet(test_packet_as_string);
//   char* test_packet_as_string_again = render_packet(test_packet_as_packet);
//   fprintf(stderr, "%d", strcmp(test_packet_as_string, test_packet_as_string_again));
//   fprintf(stderr, "\n%s\n\n\n%s", test_packet_as_string, test_packet_as_string_again);
// }
