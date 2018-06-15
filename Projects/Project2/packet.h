#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

using namespace std;

#define MAX_PACKET_SIZE 1024
#define HEADER_SIZE 24
#define MAX_DATA_SIZE 1000

#define MAX_SEQ_NUM 30720
#define WINDOW_SIZE 5120
#define TIME_OUT 500

#define CC_INIT_WINDOW_SIZE 1024

struct Packet{
        // header
        char SYN;   // 1 for SYN
        char FIN;   // 1 for FIN
        char ACK;   // 1 for ACK; 0 for message
        char flag;  // 0 for last file segment; otherwise, 1
        uint16_t ACK_num;   // ack number
        uint16_t SEQ_num;   // seq number
        int reserved[4];    // reserved for other use, just in case

        // payload
        char DATA[MAX_DATA_SIZE];
};

Packet* new_Packet(char ack, char flag, uint16_t ack_num, uint16_t seq_num){
    Packet* temp = (Packet*) malloc(sizeof(Packet));

    temp->ACK = ack;
    temp->flag = flag;
    temp->ACK_num = ack_num;
    temp->SEQ_num = seq_num;

    return temp;
}

bool isSYN(Packet msg){
    return (msg.SYN == 1);
}

