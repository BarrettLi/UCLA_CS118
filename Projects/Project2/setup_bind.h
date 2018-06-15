int setup_bind(unsigned short port, struct sockaddr_in* my_addr) {
	int sockfd_recvfrom = -1;
	if ((sockfd_recvfrom = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(my_addr, 0, sizeof(*my_addr));
	(*my_addr).sin_family = AF_INET;
	(*my_addr).sin_addr.s_addr = INADDR_ANY;
	(*my_addr).sin_port = htons(port);

	if (bind(sockfd_recvfrom, (struct sockaddr*)my_addr, sizeof(struct sockaddr_in)) < 0) {
		perror("bind");
		exit(1);
	}
	return sockfd_recvfrom;
}