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
  result->seqno = (int) strtoul(without_checksum, &without_checksum, 10);
  // Get the ackno.
  without_checksum++;
  result->ackno = (int) strtoul(without_checksum, &without_checksum, 10);
  // Get the payload.
  without_checksum++;
  result->payload = (int) strtoul(without_checksum, &without_checksum, 10);
  // Get the payload.
  without_checksum++;
  result->window = (int) strtoul(without_checksum, &without_checksum, 10);
  // Get the data!
  without_checksum++;
  result->data = calloc(MAX_PAYLOAD_LENGTH, sizeof(char));
  result->next = NULL;
  sprintf(result->data, "%s", without_checksum);
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
int send_SYN(int socket_fd, struct sockaddr_in* peer_address, socklen_t peer_address_size, struct sockaddr_in* host_address) {
  srand (time(NULL));
  // int seqno = (int) (rand() % 65530);
  int seqno = 0;
  // Build a SYN packet.
  packet_t syn_packet;
  syn_packet.type     = SYN;
  syn_packet.seqno    = seqno;
  syn_packet.ackno    = 0;
  syn_packet.payload  = 0;
  syn_packet.window   = 0;
  syn_packet.data     = calloc(1, sizeof(char));
  strcpy(syn_packet.data, "");
  char* syn_string    = render_packet(&syn_packet);
  // Send it.
  sendto(socket_fd, syn_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) peer_address, peer_address_size);
  log_packet('s', host_address, peer_address, &syn_packet);
  // Free the stuff.
  free(syn_packet.data);
  free(syn_string);
  // Return the initial seqno.
  return seqno;
}

// Finds (if applicable) a timed out packet from the queue.
packet_t* get_timedout_packet(packet_t* timeout_queue) {
  packet_t* head = timeout_queue;
  struct timeval now;
  gettimeofday(&now, NULL);
  // Get the first one we need to send.
  while (head != NULL) {
    if (head->timeout.tv_usec == 0 || timercmp(&now, &head->timeout, >)) {
      fprintf(stderr, "%d Detected a timeout on %d %d\n", head->timeout.tv_usec, head->seqno, head->type);
      return head;
    }
    head = head->next;
  }
  return NULL;
}
// Sends enough DAT packets to fill up the window give.
packet_t* send_enough_DAT_to_fill_window(int socket_fd, struct sockaddr_in* host_address, struct sockaddr_in* peer_address, socklen_t peer_address_size, FILE* file, int* current_seqno, packet_t* last_ack, packet_t* timeout_queue, enum system_states* system_state) {
  packet_t* head = timeout_queue;
  // Calculate the number of packets to send given the window size.
  int initial_seqno = *current_seqno;
  // (initial_seqno - last_ack->ackno) = Number of packets that we've sent that haven't been ACK'd.
  // This should give us the number of packets we want to send, because we know how many we've sent and we know how many the reciever has recieved.
  int packets_to_send = MAX_WINDOW_SIZE_IN_PACKETS - ((initial_seqno - last_ack->ackno)  / MAX_PAYLOAD_LENGTH);
  fprintf(stderr, "   I should send %d packets now.\n", packets_to_send);
  int sent_packets = 0;
  char* packet_string;
  while (sent_packets < packets_to_send) {
    // TODO: Verify this works!
    // Read in data from file.
    int seqno_increment = 0;
    packet_t* packet = calloc(1, sizeof(struct packet_t));
    packet->data = calloc(MAX_PAYLOAD_LENGTH+1, sizeof(char));
    if ((seqno_increment = (int) fread(packet->data, sizeof(char), MAX_PAYLOAD_LENGTH, file)) == 0) {
      // fprintf(stderr, "Should be EOF, ackno %d, cur %d\n", last_ack->ackno, *current_seqno);
        // Build.
        packet->type     = FIN;
        packet->seqno    = *current_seqno;
        packet->ackno    = 0;
        packet->payload  = 0;
        packet->window   = 0;
        strcpy(packet->data, "");
        // Send.
        //log_packet('s', host_address, peer_address, packet);
        head = add_to_timers(head, packet);
        //sendto(socket_fd, packet_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) peer_address, peer_address_size);
        fprintf(stderr, "Setting state to exit.\n");
        *system_state = EXIT;
      break;
    } else {
      fprintf(stderr, "Queueing a DAT\n");
      // Build.
      packet->type     = DAT;
      packet->seqno    = *current_seqno; // TODO: Might need +1
      packet->ackno    = 0;
      packet->payload  = seqno_increment;
      packet->window   = 0;
      // packet_string = render_packet(&packet);
      // Send.
      // log_packet('s', host_address, peer_address, &packet);
      // sendto(socket_fd, packet_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) peer_address, peer_address_size);
      head = add_to_timers(head, packet);
      // Increment the number of packets sent.
      // free(packet_string);
      if (seqno_increment != MAX_PAYLOAD_LENGTH) {
        sent_packets++; // Don't the seqno if this is the FIN.
      } else {
        sent_packets++;
        *current_seqno += seqno_increment;
      }
    }
  }
  // Return the head, in case it changed.
  return head;
}
// Send an ACK for the given seqno.
void send_ACK(int socket_fd, struct sockaddr_in* host_address, struct sockaddr_in* peer_address, socklen_t peer_address_size, int seqno, int window_size) {
  // Build an ACK packet.
  packet_t ack_packet;
  ack_packet.type     = ACK;
  ack_packet.seqno    = 0;
  ack_packet.ackno    = seqno;
  ack_packet.payload  = 0;
  ack_packet.window   = window_size;
  ack_packet.data     = calloc(1, sizeof(char));
  strcpy(ack_packet.data, "");
  char* ack_string    = render_packet(&ack_packet);
  log_packet('s', host_address, peer_address, &ack_packet);
  // Send it.
  sendto(socket_fd, ack_string, MAX_PACKET_LENGTH, 0, (struct sockaddr*) peer_address, peer_address_size);
  // Free the stuff.
  free(ack_packet.data);
  free(ack_string);
  // Return.
  return;
}
// (Re)send a DAT packet.
void resend_packet(int socket_fd, struct sockaddr_in* peer_address, socklen_t peer_address_size, packet_t* packet) {
  // Timer
  struct timeval now;
  struct timeval post_timeout;
  gettimeofday(&now, NULL);
  post_timeout.tv_usec = TIMEOUT * 1000;
  timeradd(&now, &post_timeout, &packet->timeout);
  // Resend packet.
  char* buffer = render_packet(packet);
  sendto(socket_fd, buffer, MAX_PACKET_LENGTH, 0, (struct sockaddr*) peer_address, peer_address_size);
  // Since we don't need to manipulate the queue it's cool, just return the head.
  return;
}

packet_t* add_to_timers(packet_t* timeout_queue, packet_t* packet) {
  packet_t* current_position = timeout_queue;
  if (current_position != NULL) {
    while (current_position->next != NULL) {
      current_position = current_position->next;
    }
    packet->next = current_position->next;
    current_position->next = packet;
    packet->timeout.tv_usec = 0;
  } else {
    fprintf(stderr, "Making new head\n");
    timeout_queue = packet;
    packet->timeout.tv_usec = 0;
  }
  return timeout_queue;
}

// Remove packets up to the given packet's ackno.
packet_t* remove_packet_from_timers_by_ackno(packet_t* packet, packet_t* timeout_queue) {
  // fprintf(stderr, "Removing a packet\n");
  packet_t* head = timeout_queue;
  packet_t* current = head;
  // Try to find the packet this corresponds to. Actually, we want the one before it. (We need to unlink it)
  if (current->seqno == packet->ackno) {
    fprintf(stderr, "Chopping the head off the timers. %d\n", current->seqno);
    head = current->next;
  } else {
    while (current != NULL && current->next != NULL) {
      if (current->next->seqno == packet->ackno) {
        fprintf(stderr, "Chopping off somewhere else in the timers %d\n", current->next->seqno);
        // Remove it from the queue.
        packet_t* target = current->next;
        current->next = target->next;
        free(target->data);
        free(target);
        break;
      } else {
        // Stop, this is obviously too far along in the queue.
        // break;
      }
      current = current->next;
    }
  }
  // Counter
  // int temp_count = 1;
  // packet_t* temp = head;
  // do { fprintf(stderr, "temp->seqno: %d\n", temp->seqno); temp_count++; } while (((temp = temp->next) != NULL));
  // fprintf(stderr, "Length of timers was was: %d\n", temp_count);
  //
  return head;
}

//////////////////
// Files        //
//////////////////
// Write the file to the buffer.
packet_t* write_packet_to_window(packet_t* packet, packet_t* head, FILE* file, int* window_size) { // Updates it only if the window flushed.
  packet_t* selected_window_packet = head;
  int travelled = 0;
  int is_contiguous = 1;
  int ttl = MAX_WINDOW_SIZE_IN_PACKETS;
  // Walk through the packet list and find where this should go. For this operation you could get the spot **before** where this one will go, since you need to insert it. IF you go past your determined WINDOW_SIZE, drop the packet and exit.
  if (selected_window_packet == NULL) {
    // fprintf(stderr, "Selected_window_packet == NULL\n");
    // We're at the HEAD of the window, it hasn't been populated, good for us!
    head = packet; // Return the address of the new head!
    selected_window_packet = head;
    travelled++;
  } else {
    // fprintf(stderr, "Traversing, since head is not null\n");
    while (selected_window_packet->next != NULL && (ttl--) != 0) {
      // Determine if there is a rollover.
      int next_possible_seqno = selected_window_packet->seqno + MAX_PAYLOAD_LENGTH;
      if (selected_window_packet->next->seqno != next_possible_seqno) {
        // If we're in this statement, it means we're not contiguous, so we can't flush the window.
        // fprintf(stderr, "No longer contig %d != %d\n", selected_window_packet->next->seqno, next_possible_seqno);
        is_contiguous = 0;
      }
      if (selected_window_packet->next->seqno == packet->seqno + MAX_PAYLOAD_LENGTH) { // It means we insert the packet here.
        // fprintf(stderr, "Found my spot\n");
        break;
      }
      selected_window_packet = selected_window_packet->next; // Move forward.
      travelled++;
    }
    // We're not at the HEAD, so we should be inserting after our current packet.
    packet->next = selected_window_packet->next;
    selected_window_packet->next = packet;
    selected_window_packet = selected_window_packet->next;
    travelled += 2;
  }
  // fprintf(stderr, "Is Contig:%d, has travelled %d\n", is_contiguous, travelled);
  if (is_contiguous && selected_window_packet != NULL && (travelled == MAX_WINDOW_SIZE_IN_PACKETS || selected_window_packet->payload < MAX_PAYLOAD_LENGTH)) {
    // fprintf(stderr, "Contig, window packet is not null\n");
    // Up until this point should be flushed to file. Do we need to do more?
    while (selected_window_packet->next != NULL && !(selected_window_packet->next->seqno >= selected_window_packet->seqno + MAX_PAYLOAD_LENGTH)) {
      // fprintf(stderr, "In the stupid while loop\n");
      // Walk through the rest of the nodes until we're going to hit a node that wouldn't be contiguous.
      selected_window_packet = selected_window_packet->next;
      travelled++;
    }
    while (head == selected_window_packet || head->seqno != selected_window_packet->seqno) {
      // fprintf(stderr, "Flushing out %d\n", head->seqno);
      // Flush up to this point into a file by looping through the packets.
      fwrite(head->data, sizeof(char), head->payload, file);
      head = head->next;  // Update the head of the window.
      travelled--;
      if (head == NULL) { break; }
    }
    // Used only for window size calculation.
    while (selected_window_packet->next != NULL) {
      travelled++;
      selected_window_packet = selected_window_packet->next;
    }
  }
  *window_size = MAX_WINDOW_SIZE_IN_PACKETS - travelled;
  // Return the head, again (it might have changed, that's ok.)
  return head;
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
  fprintf(stdout, "%s.%06li %c %s:%d %s:%d %s %d/%d %d/%d\n", time_string, (long int) tv.tv_usec,
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
