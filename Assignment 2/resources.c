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
#define TIMEOUT     5
#include "resources.h"

///////////////////////
// Functions         //
///////////////////////
// Render a packet based on the struct given.
char* render_packet(packet* source) {
  char* result = calloc( 1024, sizeof(char) );
  if (result == NULL) { fprintf(stderr, "Couldn't render a packet.\n"); exit(-1); }
  int render_position = 0;
  // Insert the _magic_.
  strcat(&result[render_position], "_magic_ CSc361\r\n");
  render_position = strlen(result);
  // Insert the _type_.
  sprintf(&result[render_position], "_type_ %s\r\n", source->type);
  render_position = strlen(result);
  // Insert the _seqno_.
  sprintf(&result[render_position], "_seqno_ %d\r\n", source->seqno);
  render_position = strlen(result);
  // Insert the _ackno_.
  sprintf(&result[render_position], "_ackno_ %d\r\n", source->ackno);
  render_position = strlen(result);
  // Insert the _length_.
  sprintf(&result[render_position], "_length_ %d\r\n", source->length);
  render_position = strlen(result);
  // Insert the _size_.
  sprintf(&result[render_position], "_size_ %d\r\n", source->size);
  render_position = strlen(result);
  // Insert the newlines.
  sprintf(&result[render_position], "\r\n");
  render_position = strlen(result);
  // Insert the data.
  sprintf(&result[render_position], "%s", source->data);
  // Return.
  return result;
}

// Parse out a packet into a struct given the data.
packet* parse_packet(char* source) {
  packet* result = calloc( 1, sizeof(struct packet) );
  assert(result != NULL);
  // Check for the _magic_, move along if satisfied.
  if (strncmp(source, "_magic_ CSc361\r\n", 16) != 0) { fprintf(stderr, "Bad packet magic... Exiting.\n"); exit(-1); }
  source = &source[16]; //16 to cut off the \r\n 
  // Check the _type_.
  if (strncmp(source, "_type_ ", 7) != 0) { fprintf(stderr, "Bad packet type... Exiting.\n"); exit(-1); }
  source = &source[7]; // Get the space too. 
  result->type = calloc( 4, sizeof(char) );
  assert(result->type != NULL);
  strncpy(result->type, source, 3);
  source = &source[3+2]; // Jump to next line;
  // Check _seqno__.
  if (strncmp(source, "_seqno_ ", 8) != 0) { fprintf(stderr, "Bad packet seqno... Exiting.\n"); exit(-1); }
  source = &source[8];
  int end_of_seq = 0;
  while (source[end_of_seq] != '\r') { end_of_seq++; }
  char* temp_string = calloc( end_of_seq + 1, sizeof(char) );
  assert(temp_string != NULL);
  strncpy(temp_string, source, end_of_seq);
  result->seqno = atoi(temp_string);
  free(temp_string);
  source = &source[end_of_seq + 2]; // Jump lines.
  end_of_seq = 0;
  // Check _ackno_.
  if (strncmp(source, "_ackno_ ", 8) != 0) { fprintf(stderr, "Bad packet ackno... Exiting.\n"); exit(-1); }
  source = &source[8];
  while (source[end_of_seq] != '\r') { end_of_seq++; }
  temp_string = calloc( end_of_seq + 1, sizeof(char) );
  assert(temp_string != NULL);
  strncpy(temp_string, source, end_of_seq);
  result->ackno = atoi(temp_string);
  free(temp_string);
  source = &source[end_of_seq + 2]; // Jump lines.
  end_of_seq = 0;
  // Check _length_.
  if (strncmp(source, "_length_ ", 9) != 0) { fprintf(stderr, "Bad packet length... Exiting.\n"); exit(-1); }
  source = &source[9];
  while (source[end_of_seq] != '\r') { end_of_seq++; }
  temp_string = calloc( end_of_seq + 1, sizeof(char) );
  assert(temp_string != NULL);
  strncpy(temp_string, source, end_of_seq);
  result->length = atoi(temp_string);
  free(temp_string);
  source = &source[end_of_seq + 2]; // Jump lines.
  end_of_seq = 0;
  // Check _size_.
  if (strncmp(source, "_size_ ", 7) != 0) { fprintf(stderr, "Bad packet size... Exiting.\n"); exit(-1); }
  source = &source[7];
  while (source[end_of_seq] != '\r') { end_of_seq++; }
  temp_string = calloc( end_of_seq + 1, sizeof(char) );
  assert(temp_string != NULL);
  strncpy(temp_string, source, end_of_seq);
  result->size = atoi(temp_string);
  free(temp_string);
  source = &source[end_of_seq + 2]; // Jump lines.
  end_of_seq = 0;
  // Verify there is an empty line.
  if (strncmp(source, "\r\n", 2) != 0) { fprintf(stderr, "Bad empty line... Exiting.\n"); exit(-1); }
  // Check data.
  source = &source[2];
  result->data = calloc( strlen(source) + 1, sizeof(char) );
  assert(result->data != NULL);
  strcpy(result->data, source);
  // Return the data.
  return result;
}

packet* create_packet(void) {
  packet* result = calloc(1, sizeof(struct packet));
  result->type = calloc(3, sizeof(char));
  result->data = calloc(MAX_PAYLOAD, sizeof(char));
  return result;
}
// Frees a packet properly.
void free_packet(packet* target) {
 free(target->type);
 free(target->data);
 free(target);
}

transaction* create_transaction(void) {
  transaction* result = calloc(1, sizeof(struct transaction));
  result->packet = create_packet();
  // result->string = calloc(MAX_PAYLOAD, sizeof(char));
  return result;
}

void set_timer(transaction* target) {
  time_t timer;
  target->fire_time = time(&timer);
  target->timeout = target->fire_time + TIMEOUT;
  return;
}

void check_timer(transaction* target) {
  time_t timer;
  fprintf(stderr, "Checking timer %d %d\n", time(&timer), target->timeout);
  if (time(&timer) > target->timeout) {
    fprintf(stderr, "Timed out\n");
    target->state = TIMEDOUT;
    target->fire_time = time(&timer);
    target->timeout = target->fire_time + TIMEOUT;
  }
  return;
}

void free_transaction(transaction* target) {
  free(target->string);
  free_packet(target->packet);
  free(target);
  return;
}

transaction* queue_SYN(transaction* head, int window_size) {
  transaction* last = head;
  while (last != NULL && last->tail != NULL) {
    last = last->tail;
  }
  // At the end, now add one more.
  transaction* new = create_transaction();
  new->string = calloc(1, sizeof(char));
  set_timer(new);
  // Need send a SYN
  strcpy(new->packet->type, "SYN");
  new->packet->seqno = rand();
  new->packet->ackno = 0;
  new->packet->length = 0;
  new->packet->size = 0;
  strcpy(new->packet->data,"");
  new->string = render_packet(new->packet);

  // Ready to go.
  new->state = READY;
  if (head != NULL) {
    last->tail = new;
    return head;
  } else {
    // Only on first index.
    return new;
  }
}

transaction* queue_ACK(transaction* head, int seqno, int ackno, int length, int size) {
  transaction* last = head;
  while (last != NULL && last->tail != NULL) {
    last = last->tail;
  }
  // At the end, now add one more.
  transaction* new = create_transaction();
  new->string = calloc(1, sizeof(char));
  set_timer(new);
  strcpy(new->packet->type, "ACK");
  new->packet->seqno = seqno;
  new->packet->ackno = ackno;
  new->packet->length = length;
  new->packet->size = size;
  strcpy(new->packet->data, "");
  new->string = render_packet(new->packet);
  
  // Ready to go.
  new->state = READY;
  if (head != NULL) {
    last->tail = new;
    return head;
  } else {
    // Only on the first index.
    return new;
  }
}
