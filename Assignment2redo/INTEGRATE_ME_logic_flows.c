//////////////////
// Sender       //
//////////////////
// TODO: Make socket non-blocking.
unsigned short initial_seqno = send_SYN(socket_fd, &peer_address, &peer_address_size); // Sets the initial random sequence number.
unsigned short system_seqno = initial_seqno;
char* buffer = calloc(MAX_PAYLOAD+1, sizeof(char));
packet_t* timeout_queue; // Used for timeouts. Whenever you send DATs assign the return to this.
// Used for logging exclusively.
char log_type; // S, s, R, or r.
for (;;) {
  // First we need something to work on!
  packet_t* packet;
  enum system_states system_state = HANDSHAKE;
  while (packet == NULL) {
    // Read from the socket if there is anything.
    int bytes = recvfrom(socket_fd, buffer, MAX_PAYLOAD+1, 0, (struct sockaddr*) &peer_address, &peer_address_size); // This socket is non-blocking.
    if (bytes == -1) {
      // Didn't read anything.
      // Find a packet that has timed out.
      packet = get_timedout_packet(timeout_queue); // Returns NULL if no packet has timeout.
      if (packet != NULL) { log_type = 'S'; }
    } else {
      // Got a packet, need to parse it.
      packet = parse_packet(buffer);
       // TODO: This might be broken..
      if (packet->ackno < system_seqno) { log_type = 'R'; }
      else { log_type = 'r'; }
    }
  }
  if (log_type != NULL) {
    log_packet(host_address, peer_address, packet, log_type);
  }
  // By now, packet is something. But what type is it?
  switch (packet->type) {
    case SYN:
      // Wait, why is the reciever sending a SYN?
      fprintf(stderr, "The sender shouldn't recieve SYNs. ;)");
      exit(-1);
      break;
    case ACK:
      // Act depending on what's required.
      switch (system_state) {
        case FINISH:
          // Done, sent a FIN and got an ACK.
          system_state = EXIT;
          log_statistics(statistics);
          exit(0);
          break;
        case HANDSHAKE:
          system_state = TRANSFER;
          // We're handshaked, start sending files.
          // Don't update the seqno until we get ACKs.
          timeout_queue = send_enough_DAT_to_fill_window(socket_fd, &peer_address, 
                            peer_address_size, file, &system_seqno, 
                            packet->window, timeout_queue);
          break;
        case TRANSFER:
          system_seqno = packet->seqno; // Update the seqno now!
          // Drop the packet from timers.
          timeout_queue = remove_packet_from_timers_by_ackno(packet);
          // Send some new data packets to fill that window.
          timeout_queue = send_enough_DAT_to_fill_window(socket_fd, &peer_address, 
                            peer_address_size, file, &system_seqno, 
                            packet->window, timeout_queue);
          break;
      }
      break;
    case DAT:
      // This is a packet we need to resend. (Or the reciever sent us a DAT, in that case, wtf mate?)
      // We know there is room here since it timed out. :)
      resend_DAT(socket_fd, &peer_address, peer_address_size, packet, timeout_queue);
      reset_timer(packet);
      break;
    case RST:
      // Need to restart the connection.
      system_state = RESET;
      // TODO: Rewind file pointer.
      // TODO: Empty packet queue.
      // TODO: Reset the connection, by sending a SYN.
      system_state = HANDSHAKE;
      break;
    case FIN:
      // Wait, why is the reciever sending a FIN?
      fprintf(stderr, "You probably want to send a FIN on the sender. ;)");
      exit(-1);
      break;
}

//////////////////
// Reciever     //
//////////////////
unsigned short initial_seqno;
unsigned short system_seqno;
unsigned short temp_seqno_compare; // Used for handling the sliding window.
char* window[WINDOW_SIZE_IN_PACKETS];
for (;;) {
  // First we need something to work on!
  packet_t* packet;
  enum system_states system_state = HANDSHAKE;

  int bytes = recvfrom(socket_fd, buffer, MAX_PAYLOAD+1, 0, (struct sockaddr*) &peer_address, &peer_address_size); // This socket is blocking.

  // By now, packet is something. But what type is it?
  switch (packet->type) {
    case SYN:
      // Got a SYN request, awesome. Need to ACK it and get ready for DATA.
      system_state = TRANSFER;
      intial_seqno = packet->seqno;
      system_seqno = initial_seqno;
      send_ACK(socket_fd, &peer_address, peer_address_size, packet->seqno);
      break;
    case ACK:
      // Wait, why is the reciever getting an ACK?
      fprintf(stderr, "You probably want to send an ACK on the reciever. ;)");
      exit(-1);
      break;
    case DAT:
      // Write the data into the window, that function will flush it to file and update the seqno if it has all the packets in a contiguous order.
      temp_seqno_compare = system_seqno;
      system_seqno = write_packet_to_window(&peer_address, &peer_address_size, window, initial_seqno); // Updates it only if the window flushed.
        if (system_seqno != temp_seqno_compare) { // If it gets pushed forward.
          send_ACK(&peer_address, &peer_address_size, packet->seqno);
        }
      break;
    case RST:
      system_state = RESET;
      // TODO: Rewind file pointer.
      // TODO: Empty the window.
      // TODO: Reset the connection, by sending an ACK.
      send_ACK(socket_fd, &peer_address, &peer_address_size, packet->seqno);
      system_state = HANDSHAKE;
      break;
    case FIN:
      system_state = EXIT;
      // Finished the file. We can send an ACK and close up shop.
      send_ACK(&peer_address, &peer_address_size, packet->seqno);
      log_statistics(statistics);
      exit(0);
      break;
}

