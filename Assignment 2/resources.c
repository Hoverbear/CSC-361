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
#include <sys/time.h>
#define MAX_PAYLOAD 1024
#define TIMEOUT     5
#include "resources.h"

///////////////////////
// Functions         //
///////////////////////
// Render a packet based on the struct given.
char* render_packet(packet* source) {
  char* result = calloc( MAX_PAYLOAD * 2.5, sizeof(char) );
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
  // Insert the data.
  strcat(&result[render_position], source->data);
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
  result->type = calloc(4, sizeof(char));
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
  if (time(&timer) > target->timeout) {
    fprintf(stderr, "Timed out, resending.\n");
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

transaction* queue_SYN(transaction* head) {
  transaction* last = head;
  while (last != NULL && last->tail != NULL) {
    last = last->tail;
  }
  // At the end, now add one more.
  transaction* new = create_transaction();
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

transaction* queue_FIN(transaction* head, int seqno) {
  transaction* last = head;
  while (last != NULL && last->tail != NULL) {
    last = last->tail;
  }
  // At the end, now add one more.
  transaction* new = create_transaction();
  set_timer(new);
  // Need send a SYN
  strcpy(new->packet->type, "FIN");
  new->packet->seqno = seqno;
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

transaction* find_match(transaction* head_transaction, packet* input) {
  transaction* result = head_transaction;
  while (result != NULL && result->packet->seqno != input->ackno) {
    result = result->tail;
  }
  return result; // Either NULL or the match.
}

transaction* queue_file_packets(transaction* head, FILE* file, int start_seqno) {
  // Seek to end of queue.
  transaction* last = head;
  transaction* first = head;
  while (last != NULL && last->tail != NULL) {
    last = last->tail;
  }
  // We'll add packets here.
  int last_seqno = start_seqno + 1; // First byte of data payload.
  int done = 0;
  while (!done) {
    transaction* new = create_transaction();
    strcpy(new->packet->type, "DAT");
    new->packet->seqno = last_seqno;
    new->packet->ackno = 0; // No ACKs on DATs
    new->packet->length = 0;
    new->packet->size = 0;
    new->string = render_packet(new->packet);
    // Need to populate the data until MAX_PAYLOAD.
    int payload_so_far = strlen(new->string);
    int position = 0;
    char insert_this;
    while ((insert_this = fgetc(file)) != EOF && payload_so_far <= MAX_PAYLOAD ) {
      new->packet->data[position] = insert_this;
      position++;
      payload_so_far++;
      // Attempt to detect Payload size changes.
      if (position % 10 == 0) {
        free(new->string);
        new->string = render_packet(new->packet);
        payload_so_far = strlen(new->string);
      }
    }
    // At MAX_PAYLOAD.
    free(new->string);
    new->string = render_packet(new->packet);
    set_timer(new);
    new->state = READY;
    if (head != NULL) {
      last->tail = new;
      last = new;
    } else {
      // This is the first! Wheeee
      first = new;
      last = new;
    }
    // Are we done?
    if (insert_this == EOF) {
      done = 1;
    }
    last_seqno += payload_so_far + 1;
  }
  // Ready to go.
  return first;
}

void log_packet(char event_type, struct sockaddr_in source, struct sockaddr_in destination, packet* the_packet) {
  char log_buffer[1024];
  char temp_log_buffer[1024];
  // Time
  struct timeval tv;
  time_t nowtime;
  struct tm *nowtm;

  gettimeofday(&tv, NULL);
  nowtime = tv.tv_sec;
  nowtm = localtime(&nowtime);
  strftime(temp_log_buffer, 1024, "%H:%M:%S", nowtm);
  snprintf(log_buffer, 1024, "%s.%06d", temp_log_buffer, (int) tv.tv_usec);
  // Event_type
  strcat(log_buffer, " ");
  log_buffer[strlen(log_buffer)+1] = event_type;
  // Source
  strcat(log_buffer, " ");
  strcat(log_buffer, inet_ntoa(source.sin_addr));
  strcat(log_buffer, ":");
  sprintf(&log_buffer[strlen(log_buffer)], "%d", source.sin_port);
  // Destination
  strcat(log_buffer, " ");
  strcat(log_buffer, inet_ntoa(destination.sin_addr));
  strcat(log_buffer, ":");
  sprintf(&log_buffer[strlen(log_buffer)], "%d", destination.sin_port);
  // Packet Type
  strcat(log_buffer, " ");
  strcat(log_buffer, the_packet->type);
  // Seq / ACK LEN / WIN
  strcat(log_buffer, " ");
  sprintf(&log_buffer[strlen(log_buffer)], "%d/%d %d/%d", the_packet->seqno, the_packet->ackno, the_packet->length, the_packet->size);
  // Print
  fprintf(stdout, "%s\n",  log_buffer);
}

