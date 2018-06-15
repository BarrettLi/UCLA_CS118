static void handleThreeWay(ssize_t num_received, struct UDP_PACKET packet);
//static void handleShutdown(ssize_t num_received, struct UDP_PACKET packet);
void handle_packet(ssize_t num_received, struct UDP_PACKET packet) {
	if (threeWayCompleted == 0) {
		handleThreeWay(num_received, packet);
		threeWayCompleted = 1;
	}
	else {
		if (packet.SYN == 0) {
			// regular data
			static int lastDataPacketReceived = 0;

			their_SEQnum = packet.SEQ;
			if (their_SEQnum == expected_SEQnum) {	// arriving packet is in order
				if (packet.FIN == 1) {
					receivedFIN = 1;
				}
				if (threeWayResponsePacketAcked == 0) {
					threeWayResponsePacketAcked = 1;
				}
				size_t dataSize = (size_t)num_received - PACKET_HEADERSIZE;
				struct UDP_PACKET response;
				response.SYN = 0;
				response.FIN = 0;
				response.ACK = 1;
				response.flag = 1;
				response.SEQ = curr_SEQnum;
				response.ACKnum = dataSize == 0 ? safe_add(their_SEQnum, 1) : safe_add(their_SEQnum, dataSize);
				expected_SEQnum = dataSize == 0 ? safe_add(their_SEQnum, 1) : safe_add(their_SEQnum, dataSize);
				if (dataSize != 0 && lastDataPacketReceived != 1) {
					memcpy((void*) & (fileBuffer[fileIndex]), (const void*)packet.DATA, dataSize);
					if (DEBUG) {
						printf("Writing %ld bytes to file...\n", dataSize);
					}
					fileIndex += dataSize;
				}
				if (packet.flag == 0) {
					lastDataPacketReceived = 1;
				}
				if (packet.ACK == 1 && receivedFIN == 1 && sentFIN == 1) {
					// prepare to shutdown
					FINPacketAcked = 1;
					if (DEBUG) {
						printf("Received last ACK. Send nothing and wait for 2*RTO for shutdown.\n");
					}
					struct timespec tim;
					/* In our project, 2*RTO is just 1sec */ // tim.tv_sec = 0; tim.tv_nsec = 2 * RTO;
					tim.tv_sec = 1;
					tim.tv_nsec = 0;
					if (nanosleep(&tim, NULL) < 0) {
						handle_error("nanosleep");
					}
					if (DEBUG) {
						printf("OK. Waited for 2*RTO. Start to shutdown\n");
					}

					beginShutdown = 1;



					size_t fileSize = fileIndex;
					if (DEBUG)
						printf("Exit\n");
					int writeFileFD = open(SAVEFILENAME, O_WRONLY | O_CREAT, 0644);
					if (writeFileFD == -1) {
						handle_error("write");
					}
					ssize_t writeCount = write(writeFileFD, (const void*)fileBuffer, fileSize);
					if (writeCount != (ssize_t)fileSize) {
						printf("write error\n");
						exit(1);
					}
					close(writeFileFD);
					exit(0);
				}
				// size_t num_to_send = sizeof(struct UDP_PACKET);
				size_t num_to_send = PACKET_HEADERSIZE;
				ssize_t num_sent = sendto(sockfd_sendto, /* socket */
				                          // argv[3], /* buffer to send */
				                          (void*)&response, /* buffer to send */
				                          num_to_send, /* number of bytes to send */
				                          0, /* flags=0: bare-bones use case*/
				                          (const struct sockaddr*)&server_addr, /* the destination */
				                          sizeof(server_addr)); /* size of the destination struct */
				curr_SEQnum = safe_add(curr_SEQnum, 1);
				if (num_sent < 0) {
					handle_error("sendto");
				}
				if (DEBUG) {
					printf("I just sent %ld bytes!\n", num_sent);
					printf("Just sent packet:\n");
					printf("SYN:%d, ", response.SYN);
					printf("FIN:%d, ", response.FIN);
					printf("ACK:%d, ", response.ACK);
					printf("flag:%d, ", response.flag);
					printf("ACKnum:%d, ", response.ACKnum);
					printf("SEQ:%d", response.SEQ);
					printf("\n");
				}
				if (SPEC_STDOUT) {
					printf("Sending packet %d ", response.SEQ);
					if (response.SYN == 1) {
						printf("SYN ");
					}
					if (response.FIN == 1) {
						printf("FIN ");
					}
					printf("\n");
				}
				if (receivedFIN == 1 && sentFIN == 0) {	// send FIN
					struct UDP_PACKET FIN_packet;
					FIN_packet.SYN = 0;
					FIN_packet.FIN = 1;
					FIN_packet.ACK = 0;
					FIN_packet.flag = 1;
					FIN_packet.SEQ = curr_SEQnum;
					FIN_packet.ACKnum = expected_SEQnum;
					/* no need to modify exptectd_SEQnum */ // expected_SEQnum = dataSize == 0 ? safe_add(their_SEQnum, 1) : safe_add(their_SEQnum, dataSize);





					// size_t num_to_send = sizeof(struct UDP_PACKET);
					size_t num_to_send = PACKET_HEADERSIZE;
					ssize_t num_sent = sendto(sockfd_sendto, /* socket */
					                          // argv[3], /* buffer to send */
					                          (void*)&FIN_packet, /* buffer to send */
					                          num_to_send, /* number of bytes to send */
					                          0, /* flags=0: bare-bones use case*/
					                          (const struct sockaddr*)&server_addr, /* the destination */
					                          sizeof(server_addr)); /* size of the destination struct */
					curr_SEQnum = safe_add(curr_SEQnum, 1);
					if (num_sent < 0) {
						handle_error("sendto");
					}
					if (DEBUG) {
						printf("I just sent %ld bytes!\n", num_sent);
						printf("Just sent packet:\n");
						printf("SYN:%d, ", FIN_packet.SYN);
						printf("FIN:%d, ", FIN_packet.FIN);
						printf("ACK:%d, ", FIN_packet.ACK);
						printf("flag:%d, ", FIN_packet.flag);
						printf("ACKnum:%d, ", FIN_packet.ACKnum);
						printf("SEQ:%d", FIN_packet.SEQ);
						printf("\n");
					}
					if (SPEC_STDOUT) {
						printf("Sending packet %d ", FIN_packet.SEQ);
						if (FIN_packet.SYN == 1) {
							printf("SYN ");
						}
						if (FIN_packet.FIN == 1) {
							printf("FIN ");
						}
						printf("\n");
					}
					sentFIN = 1;
					FINPacket = FIN_packet;

					pthread_create(&FIN_packet_thread, NULL, (void *) &retransmitFINPacketUponTimeout, NULL);
				}


			}
			else {	// arriving packet out of order

				/* because out of order. no need for this line*/  // size_t dataSize = (size_t)num_received - PACKET_HEADERSIZE;
				struct UDP_PACKET response;
				response.SYN = 0;
				response.FIN = 0;
				response.ACK = 1;
				response.flag = 1;
				response.SEQ = safe_subtract(curr_SEQnum, 1);
				response.ACKnum = expected_SEQnum;

				// size_t num_to_send = sizeof(struct UDP_PACKET);
				size_t num_to_send = PACKET_HEADERSIZE;
				ssize_t num_sent = sendto(sockfd_sendto, /* socket */
				                          // argv[3], /* buffer to send */
				                          (void*)&response, /* buffer to send */
				                          num_to_send, /* number of bytes to send */
				                          0, /* flags=0: bare-bones use case*/
				                          (const struct sockaddr*)&server_addr, /* the destination */
				                          sizeof(server_addr)); /* size of the destination struct */
				if (num_sent < 0) {
					handle_error("sendto");
				}
				if (DEBUG) {
					printf("I just REsent %ld bytes!\n", num_sent);
					printf("Just sent packet:\n");
					printf("SYN:%d, ", response.SYN);
					printf("FIN:%d, ", response.FIN);
					printf("ACK:%d, ", response.ACK);
					printf("flag:%d, ", response.flag);
					printf("ACKnum:%d, ", response.ACKnum);
					printf("SEQ:%d", response.SEQ);
					printf("\n");
				}
				if (SPEC_STDOUT) {
					printf("Sending packet %d Retransmission ", response.SEQ);
					if (response.SYN == 1) {
						printf("SYN ");
					}
					if (response.FIN == 1) {
						printf("FIN ");
					}
					printf("\n");
				}
			}

		}
	}
	return;
}

static void handleThreeWay(ssize_t num_received, struct UDP_PACKET packet) {
	if (packet.SYN == 1 && packet.ACK == 1) {
		threeWayInitiatePacketAcked = 1;
		their_SEQnum = packet.SEQ;
		struct UDP_PACKET threeWayResponse;
		threeWayResponse.SYN = 0;
		threeWayResponse.FIN = 0;
		threeWayResponse.ACK = 1;
		threeWayResponse.flag = 0;
		threeWayResponse.SEQ = curr_SEQnum;
		threeWayResponse.ACKnum = safe_add(their_SEQnum, 1);
		memcpy((void*)threeWayResponse.DATA, (const void*)filenameArgument, strlen(filenameArgument) + 1);
		filenameArgument_sizeof = strlen(filenameArgument) + 1;
		expected_SEQnum = safe_add(their_SEQnum, 1);
		// send the threeWayResponse packet
		size_t num_to_send = sizeof(struct UDP_PACKET);

		// The next line is the "Right way", but to make things easier, we fixed client's response packet to be MAXPACKETSIZE
		// size_t num_to_send = PACKET_HEADERSIZE + (strlen(filenameArgument) + 1);
		ssize_t num_sent = sendto(sockfd_sendto, /* socket */
		                          // argv[3], /* buffer to send */
		                          (void*)&threeWayResponse, /* buffer to send */
		                          num_to_send, /* number of bytes to send */
		                          0, /* flags=0: bare-bones use case*/
		                          (const struct sockaddr*)&server_addr, /* the destination */
		                          sizeof(server_addr)); /* size of the destination struct */
		curr_SEQnum = safe_add(curr_SEQnum, 1);
		if (num_sent < 0) {
			handle_error("sendto");
		}
		threeWayResponsePacket = threeWayResponse;
		pthread_create(&threeWayResponsePacket_thread, NULL, (void *) &retransmitThreeWayResponsePacketUponTimeout, NULL);
		if (DEBUG) {
			printf("I just sent %ld bytes!\n", num_sent);
			printf("Just sent packet:\n");
			printf("SYN:%d, ", threeWayResponse.SYN);
			printf("FIN:%d, ", threeWayResponse.FIN);
			printf("ACK:%d, ", threeWayResponse.ACK);
			printf("flag:%d, ", threeWayResponse.flag);
			printf("ACKnum:%d, ", threeWayResponse.ACKnum);
			printf("SEQ:%d", threeWayResponse.SEQ);
			printf("\n");
		}
		if (SPEC_STDOUT) {
			printf("Sending packet %d ", threeWayResponse.SEQ);
			if (threeWayResponse.SYN == 1) {
				printf("SYN ");
			}
			if (threeWayResponse.FIN == 1) {
				printf("FIN ");
			}
			printf("\n");
		}
	}
}