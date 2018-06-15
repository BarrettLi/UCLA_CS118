uint16_t safe_add(uint16_t num, uint16_t addhowmany) {
	return (num + addhowmany) % (MAX_SEQNUM + 1);
}
uint16_t safe_subtract(uint16_t num, uint16_t subtracthowmany) {
	// (-26 % 10 + 10) % 10
	int tempresult = (int)num - (int)subtracthowmany;
	return (uint16_t)((tempresult % (MAX_SEQNUM + 1) + (MAX_SEQNUM + 1)) % (MAX_SEQNUM + 1));
}