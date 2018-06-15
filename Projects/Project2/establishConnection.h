void establishConnection(int argc, char** argv) {
	srand(time(NULL));
	curr_SEQnum = (uint16_t)rand() % (MAX_SEQNUM + 1);
	printf("establishing connection...\n");
	printf("initial sequence number is %d\n", curr_SEQnum);
	return;
}