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

// Sets the initial sequence number.
unsigned short send_SYN(int socket_fd, struct sockaddr_in* peer_address, socklen_t peer_address_size) {
  unsigned short seqno = (short) rand();
  // Build a SYN packet.
  packet_t syn_packet;
  syn_packet.type     = SYN;
  syn_packet.seqno    = (unsigned short) rand();
  syn_packet.ackno    = 0;
  syn_packet.payload  = 0;
  syn_packet.window   = 0;
  syn_packet.data     = calloc(1, sizeof(char));
  strcpy(syn_packet.data, "");
  char* syn_string    = render_packet(&syn_packet);
  // Send it.
  sendto(socket_fd, syn_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, peer_address_size);
  // Free the stuff.
  free(syn_packet.data);
  free(syn_string);
  // Return the initial seqno.
  return seqno;
}

// Finds (if applicable) a timed out packet from the queue.
packet_t* get_timedout_packet(packet_t* timeout_queue) {
  packet_t* head = timeout_queue;
  // TODO
  return head;
}
// Sends enough DAT packets to fill up the window give.
packet_t* send_enough_DAT_to_fill_window(int socket_fd, struct sockaddr_in* peer_address, socklen_t peer_address_size, FILE* file, unsigned short starting_seqno, short window_size, packet_t* timeout_queue) {
  packet_t* head = timeout_queue;
  // Calculate the number of packets to send given the window size.
  int packets_to_send = window_size / MAX_PAYLOAD_LENGTH;
  int sent_packets = 0;
  packet_t packet;
  packet.data = calloc(MAX_PAYLOAD_LENGTH, sizeof(char));
  char* packet_string;
  unsigned short current_seqno = starting_seqno;
  while (sent_packets < packets_to_send) {
    // TODO: Verify this works!
    // Read in data from file.
    if (fgets(packet.data, MAX_PAYLOAD_LENGTH, file) == NULL) {
      // If it's NULL, it's time to send a FIN packet and break out.
      // Build.
      packet.type     = FIN;
      packet.seqno    = current_seqno; // TODO: Might not need this (Could be 0)
      packet.ackno    = 0;
      packet.payload  = 0;
      packet.window   = 0;
      strcpy(packet.data, "");
      packet_string = render_packet(&packet);
      // Send.
      sendto(socket_fd, packet_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, peer_address_size);
      break;
    } else {
      // Build.
      packet.type     = DAT;
      current_seqno += strlen(packet.data);
      packet.seqno    = current_seqno; // TODO: Might need +1
      packet.ackno    = 0;
      packet.payload  = 0;
      packet.window   = 0;
      packet_string = render_packet(&packet);
      // Send.
      sendto(socket_fd, packet_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, peer_address_size);
      // Increment the number of packets sent.
      sent_packets++;
    }
  }
  // Free
  if (packet_string != NULL) {
    free(packet_string);
  }
  // Return the head, in case it changed.
  return head;
}
// Send an ACK for the given seqno.
void send_ACK(int socket_fd, struct sockaddr_in* peer_address, socklen_t peer_address_size, short seqno) {
  // Build an ACK packet.
  packet_t ack_packet;
  ack_packet.type     = ACK;
  ack_packet.seqno    = 0;
  ack_packet.ackno    = seqno;
  ack_packet.payload  = 0;
  ack_packet.window   = 0;
  ack_packet.data     = calloc(1, sizeof(char));
  strcpy(ack_packet.data, "");
  char* ack_string    = render_packet(&ack_packet);
  // Send it.
  sendto(socket_fd, ack_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, peer_address_size);
  // Free the stuff.
  free(ack_packet.data);
  free(ack_string);
  // Return.
  return;
}
// (Re)send a DAT packet.
void resend_DAT(int socket_fd, struct sockaddr_in* peer_address, socklen_t peer_address_size, packet_t* packet) {
  // Timer
  time_t timer;
  packet->timeout = time(&timer) + TIMEOUT;
  // Resend packet.
  sendto(socket_fd, packet, MAX_PACKET_LENGTH, 0, (struct sockaddr*) &peer_address, peer_address_size);
  // Since we don't need to manipulate the queue it's cool, just return the head.
  return;
}
// Remove packets up to the given packet's ackno.
packet_t* remove_packet_from_timers_by_ackno(packet_t* packet, packet_t* timeout_queue) {
  packet_t* head = timeout_queue;
  packet_t* current = head;
  // Try to find the packet this corresponds to. Actually, we want the one before it. (We need to unlink it)
  while (current->next != NULL) {
    if (current->next->seqno < packet->ackno) {
      // Remove it from the queue.
      packet_t* target = current->next;
      current->next = target->next;
      free(target->data);
      free(target);
    } else {
      // Stop, this is obviously too far along in the queue.
      break;
    }
    current = current->next;
  }
  return head;
}

//////////////////
// Files        //
//////////////////
// Write the file to the buffer.
unsigned short write_packet_to_window(struct sockaddr_in* peer_address, socklen_t peer_address_size, unsigned short window, unsigned short initial_seqno) { // Updates it only if the window flushed.
  // TODO
  return 0;
}

//////////////////
// Logging      //
//////////////////
// Logs the given packet.
void log_packet(char event_type, struct sockaddr_in* source, struct sockaddr_in* destination, packet_t* the_packet) {
  // Build a time string.
  char time_string[100];
  struct timeval tv;
  time_t nowtime;
  struct tm *nowtm;
  gettimeofday(&tv, NULL);
  nowtime = tv.tv_sec;
  nowtm = localtime(&nowtime);
  strftime(time_string, 100, "%H:%M:%S", nowtm);
  // Format the log.
  fprintf(stdout, "%s.%06d %c %s:%d %s:%d %s %d/%d %d/%d", time_string, (int) tv.tv_usec,
          event_type, inet_ntoa(source->sin_addr), source->sin_port, inet_ntoa(destination->sin_addr),
          destination->sin_port, packet_type_array[the_packet->type], the_packet->seqno,
          the_packet->ackno, the_packet->payload, the_packet->window);
  // Return.
  return;
}
// Outputs the log file.
void log_statistics(statistics_t statistics) {
  fprintf(stdout, "When I'm done programming this, magic will be performed.");
  // TODO
  return;
}
