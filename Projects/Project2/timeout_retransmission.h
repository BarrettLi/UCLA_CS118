void retransmitThreeWayInitatePacketUponTimeout() {
	if (ENABLE_TIMEOUT == 0) {
		pthread_exit(NULL);
	}
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = RTO;
	// size_t num_to_send = PACKET_HEADERSIZE;
	// the above is actully the right way but to simplify things just transmit MAXPACKETSIZE bytes
	size_t num_to_send = MAXPACKETSIZE;
	while (1) {
		if (nanosleep(&tim, NULL) < 0) {
			handle_error("nanosleep");
		}
		if (threeWayInitiatePacketAcked == 0) {
			ssize_t num_sent = sendto(sockfd_sendto, /* socket */
			                          // argv[3], /* buffer to send */
			                          (void*)&threeWayInitiatePacket, /* buffer to send */
			                          num_to_send, /* number of bytes to send */
			                          0, /* flags=0: bare-bones use case*/
			                          (const struct sockaddr*)&server_addr, /* the destination */
			                          sizeof(server_addr)); /* size of the destination struct */
			if (num_sent < 0) {
				handle_error("sendto");
			}
			if (DEBUG) {
				printf("I just sent %ld bytes!\n", num_sent);
				printf("Just sent packet:\n");
				printf("SYN:%d, ", threeWayInitiatePacket.SYN);
				printf("FIN:%d, ", threeWayInitiatePacket.FIN);
				printf("ACK:%d, ", threeWayInitiatePacket.ACK);
				printf("flag:%d, ", threeWayInitiatePacket.flag);
				printf("ACKnum:%d, ", threeWayInitiatePacket.ACKnum);
				printf("SEQ:%d", threeWayInitiatePacket.SEQ);
				printf("\n");
			}
			if (SPEC_STDOUT) {
				printf("Sending packet %d Retransmission ", threeWayInitiatePacket.SEQ);
				if (threeWayInitiatePacket.SYN == 1) {
					printf("SYN ");
				}
				if (threeWayInitiatePacket.FIN == 1) {
					printf("FIN ");
				}
				printf("\n");
			}
		}
		else {
			pthread_exit(NULL);
		}
	}
}
void retransmitThreeWayResponsePacketUponTimeout() {
	if (ENABLE_TIMEOUT == 0) {
		pthread_exit(NULL);
	}
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = RTO;
	// size_t num_to_send = PACKET_HEADERSIZE;
	// the above is actully the right way but to simplify things just transmit MAXPACKETSIZE bytes
	size_t num_to_send = MAXPACKETSIZE + filenameArgument_sizeof;
	while (1) {
		if (nanosleep(&tim, NULL) < 0) {
			handle_error("nanosleep");
		}
		if (threeWayResponsePacketAcked == 0) {
			ssize_t num_sent = sendto(sockfd_sendto, /* socket */
			                          // argv[3], /* buffer to send */
			                          (void*)&threeWayResponsePacket, /* buffer to send */
			                          num_to_send, /* number of bytes to send */
			                          0, /* flags=0: bare-bones use case*/
			                          (const struct sockaddr*)&server_addr, /* the destination */
			                          sizeof(server_addr)); /* size of the destination struct */
			if (num_sent < 0) {
				handle_error("sendto");
			}
			if (DEBUG) {
				printf("I just sent %ld bytes!\n", num_sent);
				printf("Just sent packet:\n");
				printf("SYN:%d, ", threeWayResponsePacket.SYN);
				printf("FIN:%d, ", threeWayResponsePacket.FIN);
				printf("ACK:%d, ", threeWayResponsePacket.ACK);
				printf("flag:%d, ", threeWayResponsePacket.flag);
				printf("ACKnum:%d, ", threeWayResponsePacket.ACKnum);
				printf("SEQ:%d", threeWayResponsePacket.SEQ);
				printf("\n");
			}
			if (SPEC_STDOUT) {
				printf("Sending packet %d Retransmission ", threeWayResponsePacket.SEQ);
				if (threeWayResponsePacket.SYN == 1) {
					printf("SYN ");
				}
				if (threeWayResponsePacket.FIN == 1) {
					printf("FIN ");
				}
				printf("\n");
			}
		}
		else {
			pthread_exit(NULL);
		}
	}
}
void retransmitFINPacketUponTimeout() {
	if (ENABLE_TIMEOUT == 0) {
		pthread_exit(NULL);
	}
	struct timespec tim;
	tim.tv_sec = 0;
	tim.tv_nsec = RTO;
	// size_t num_to_send = PACKET_HEADERSIZE;
	// the above is actully the right way but to simplify things just transmit MAXPACKETSIZE bytes
	size_t num_to_send = MAXPACKETSIZE;
	while (1) {
		if (nanosleep(&tim, NULL) < 0) {
			handle_error("nanosleep");
		}
		if (FINPacketAcked == 0) {
			ssize_t num_sent = sendto(sockfd_sendto, /* socket */
			                          // argv[3], /* buffer to send */
			                          (void*)&FINPacket, /* buffer to send */
			                          num_to_send, /* number of bytes to send */
			                          0, /* flags=0: bare-bones use case*/
			                          (const struct sockaddr*)&server_addr, /* the destination */
			                          sizeof(server_addr)); /* size of the destination struct */
			if (num_sent < 0) {
				handle_error("sendto");
			}
			if (DEBUG) {
				printf("I just sent %ld bytes!\n", num_sent);
				printf("Just sent packet:\n");
				printf("SYN:%d, ", FINPacket.SYN);
				printf("FIN:%d, ", FINPacket.FIN);
				printf("ACK:%d, ", FINPacket.ACK);
				printf("flag:%d, ", FINPacket.flag);
				printf("ACKnum:%d, ", FINPacket.ACKnum);
				printf("SEQ:%d", FINPacket.SEQ);
				printf("\n");
			}
			if (SPEC_STDOUT) {
				printf("Sending packet %d Retransmission ", FINPacket.SEQ);
				if (FINPacket.SYN == 1) {
					printf("SYN ");
				}
				if (FINPacket.FIN == 1) {
					printf("FIN ");
				}
				printf("\n");
			}
		}
		else {
			pthread_exit(NULL);
		}
	}
}