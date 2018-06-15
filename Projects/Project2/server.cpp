#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <vector>

#include "CongestionControl.h"
#include "packet.h"

#define congestion_control 1

using namespace std;

int sendBase;
int nextSeqNum;

int cwnd = WINDOW_SIZE;
int cc_state = 0;// congestion control state: 
                 // 0 for nothing, 
                 // 1 for SlowStart
                 // 2 for congestionavoidance
                 // 3 for fastrecovery 
int threshold;

bool valid_ack(Packet* ack){
    if((ack->ACK_num > sendBase) || (ack->ACK_num + MAX_SEQ_NUM - sendBase < cwnd)){
        return true;
    }
    return false;
}

bool is_oversize(int end_byte, int base){
    int tempresult = end_byte - base;
    int diff= (uint16_t)((tempresult % (MAX_SEQ_NUM + 1) + (MAX_SEQ_NUM + 1)) % (MAX_SEQ_NUM + 1));
    return diff > cwnd;
}

bool is_timeout(struct timeval start, struct timeval curr){
    struct timeval duration;

    timersub(&curr, &start, &duration);

    return (duration.tv_sec != 0) || (duration.tv_usec > TIME_OUT * 1000);
}

bool is_FIN_timeout(struct timeval start, struct timeval curr){
    struct timeval duration;

    timersub(&curr, &start, &duration);

    return (duration.tv_sec != 0) || (duration.tv_usec > 2 * TIME_OUT * 1000);
}

int main(int argc, char** argv){

    // if there is congestion control
    // redefine parameters
    if(congestion_control){
        cwnd = CC_INIT_WINDOW_SIZE;
        cc_state = 1;
        threshold = 15360;
    }

    struct timeval start_time, curr_time;
    // get arguments
    if(argc < 3){
        printf("Error: wrong number of arguments\n");
        printf("Usage: ./server [hostname] [port]\n");
        exit(1);
    }

    char* hostname = argv[1];   // IP address of server
    int portno = atoi(argv[2]); // port number 

    // open a datagram socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd == -1){
        perror("Socket error:");
        exit(1);
    }

    struct sockaddr_in server, client, client_send;
    // clear out server struct
    memset((void*)&server, '\0', sizeof(struct sockaddr_in));

    // set family and port
    server.sin_family = AF_INET;
    server.sin_port = htons(portno);
    // set address manually
    // server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_addr.s_addr = INADDR_ANY;
    
    // bind server to socket
    if(bind(sockfd, (struct sockaddr*) &server, sizeof(struct sockaddr_in)) == -1){
        perror("Bind error:");
        exit(1);
    }

    // printf("Waiting for request at IP:%s port:%d\n", argv[1], portno);
    ///////////////////three way handshake////////////////////////////
    int nbytes_received = -1;
    Packet msg;         // the message received from client
    socklen_t client_len = sizeof(struct sockaddr_in);

    // listen to the socket
    while(true){
        nbytes_received = recvfrom(sockfd, &msg, MAX_PACKET_SIZE, 0, (struct sockaddr *) &client, &client_len);
        if(isSYN(msg)){
            break;
        }
    }
    client_send = client;
    client_send.sin_port = htons(12345);

	// memcpy((void*)&msg, (const void*)b, (size_t)nbytes_received);

    printf("Receiving packet %d\n", msg.ACK_num);

    // initialize a random seq number
    srand(123);
    sendBase = rand() % MAX_SEQ_NUM;
    nextSeqNum = sendBase;

    // printf("\ninitial sequence number is %d\n", sendBase);
    // create new packet
    int ack_num = (msg.SEQ_num + 1) % MAX_SEQ_NUM;
    Packet* ack = new_Packet(1, 0, ack_num, nextSeqNum);
    ack->SYN = 1;
    ack->FIN = 0;
    ack->reserved[0] = 0; //payload size is 0

    // send back the ack with serv_isn
    int send_size = HEADER_SIZE + ack->reserved[0];
    sendto(sockfd, ack, send_size, 0, (struct sockaddr*) &client_send, client_len);

    if(congestion_control){
        printf("Sending packet %d %d %d SYN\n", ack->SEQ_num, cwnd, threshold);
    }else{
        printf("Sending packet %d %d SYN\n", ack->SEQ_num, cwnd);
    }
    
    while(true){
        // wait for the response from client
        nbytes_received = recvfrom(sockfd, &msg, MAX_PACKET_SIZE, 0, (struct sockaddr *) &client, &client_len);
        printf("Receiving packet %d\n", msg.ACK_num);
        if(!isSYN(msg)){
            break;
        }
        sendto(sockfd, ack, send_size, 0, (struct sockaddr*) &client_send, client_len);
        if(congestion_control){
            printf("Sending packet %d %d %d Retransmission SYN\n", ack->SEQ_num, cwnd, threshold);
        }else{
            printf("Sending packet %d %d Retransmission SYN\n", ack->SEQ_num, cwnd);
        }
    }
    nextSeqNum++;
    client.sin_port = htons(12345);

    if(valid_ack(&msg)){
        // SYN message is ACKed by the client
        // move window forward
        sendBase = msg.ACK_num;
        ack_num += 1;
    }
///////////////////////////////////////three way hand shake finished/////////////////////////////////////////////////////////////////////////

    // printf("--------------------three way handshake finished-----------------------------\n");
    // open the requested file
    ifstream file(msg.DATA, ios::in | ios::binary);

    if(!file){
        // if the file is not found
        printf("Cannot find file\n");
        exit(1);
    }

    // if the file is found, packet data into segments
    vector<Packet> segments;
    char buff[MAX_DATA_SIZE];
    int count = 0;

    while(!file.eof()){
        char c = file.get();
        // printf("Read: %c\n", c);
        buff[count] = c;

        
        // if the file is larger than one packet
        if(count == MAX_DATA_SIZE - 1){
            // create a packet 
            Packet* pac = new_Packet(0, 1, ack_num, nextSeqNum);
            ack_num++;
            pac->SYN = 0;
            pac->FIN = 0;
            memcpy(pac->DATA, buff, MAX_DATA_SIZE);
            pac->reserved[0] = MAX_DATA_SIZE; // pay load size is MAX_DATA_SIZE

            // push the packet into a vector
            segments.push_back(*pac);

            count = -1;
            nextSeqNum = (nextSeqNum + MAX_DATA_SIZE) % MAX_SEQ_NUM;
        }
        count ++;
    }
    count--;
    // if there is no more data
    if(count == 0){
        segments[segments.size()-1].flag = 0;
    }else{
        
        // otherwise create the last segment of the file
        Packet* pac = new_Packet(0, 0, ack_num, nextSeqNum);
        pac->SYN = 0;
        pac->FIN = 0;
        pac->reserved[0] = count; // payload size is count
        memcpy(pac->DATA, buff, count);
        segments.push_back(*pac);

        nextSeqNum = (nextSeqNum + count) % MAX_SEQ_NUM;
    }

    // start sending packets
    send_size = HEADER_SIZE + segments[0].reserved[0];
    sendto(sockfd, &segments[0], send_size, 0, (struct sockaddr*) &client_send, client_len);
    if(congestion_control){
        printf("Sending packet %d %d %d\n", segments[0].SEQ_num, cwnd, threshold);
    }else{
        printf("Sending packet %d %d\n", segments[0].SEQ_num, cwnd);
    }
    // send the rest segments
    int packet_num = 1;
    for(; packet_num < segments.size(); packet_num++){
        int end_byte = (segments[packet_num].SEQ_num + segments[packet_num].reserved[0] + HEADER_SIZE) % MAX_SEQ_NUM;

        // if there isn't enough room for the segment to send
        if(is_oversize(end_byte, sendBase)){
            break;
        }

        send_size = HEADER_SIZE + segments[packet_num].reserved[0];
        sendto(sockfd, &segments[packet_num], send_size, 0, (struct sockaddr*) &client_send, client_len);
        if(congestion_control){
            printf("Sending packet %d %d %d\n", segments[packet_num].SEQ_num, cwnd, threshold);
        }else{
            printf("Sending packet %d %d\n", segments[packet_num].SEQ_num, cwnd);
        }
    }
    // start clock
    gettimeofday(&start_time, NULL);

    // process response
    int first_nonACK_seg = 0;
    while(true){
        gettimeofday(&curr_time, NULL);

        // if time out
        if(is_timeout(start_time, curr_time)){
            // retransmit all the packets in the window
            for(int i=first_nonACK_seg; i < packet_num; i++){
                sendto(sockfd, &segments[i], segments[i].reserved[0] + HEADER_SIZE, 0, (struct sockaddr*) &client_send, client_len);
                if(congestion_control){
                    printf("Sending packet %d %d %d Retransmission\n", segments[i].SEQ_num, cwnd, threshold);
                }else{
                    printf("Sending packet %d %d Retransmission\n", segments[i].SEQ_num, cwnd);
                }
            }

            // restart the clock
            gettimeofday(&start_time, NULL);
            continue;
        }

         // get ACK
        while(true){
            Packet ack;
            nbytes_received = recvfrom(sockfd, &ack, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr*) &client, &client_len);

            if(nbytes_received == 0 || nbytes_received == -1){
                // there is no message yet
                break;
            }

            printf("Receiving packet %d\n", ack.ACK_num);

            // slide window upon successful ACKs
            if(valid_ack(&ack)){
                int temp = ack.ACK_num - sendBase;
                int byte_diff = (temp % (MAX_SEQ_NUM + 1) + (MAX_SEQ_NUM + 1)) % (MAX_SEQ_NUM + 1);
                first_nonACK_seg += ceil((double) byte_diff / MAX_PACKET_SIZE);

                sendBase = ack.ACK_num;
                ack_num = ack.SEQ_num + 1;

                // if it is in slow start, increment cwnd by one packet size
                // upon each ACK
                if(in_SS(cc_state)){
                    if(cwnd + MAX_PACKET_SIZE <= threshold){
                        cwnd += MAX_PACKET_SIZE;
                    }else{
                        cc_state = CongestionAvoidance;
                    }
                }
                // if it is in congestion avoidance, 
                // increment cwnd by packet/cwnd upon each ACK
                else if(in_CA(cc_state)){
                    if(cwnd <= 1.5 * threshold ){
                        int max_pac_num = floor(cwnd/MAX_PACKET_SIZE);
                        cwnd += floor(MAX_PACKET_SIZE/max_pac_num);
                    }
                }
                gettimeofday(&start_time, NULL);
            }
        }
    
        // send packets in window
        for(packet_num; packet_num < segments.size(); packet_num++){
            int end_byte = (segments[packet_num].SEQ_num + segments[packet_num].reserved[0] + HEADER_SIZE) % MAX_SEQ_NUM;

            // segments[packet_num].ACK_num = ack_num;
            // if there isn't enough room for the segment to send
            if(is_oversize(end_byte, sendBase)){
                break;
            }

            send_size = HEADER_SIZE + segments[packet_num].reserved[0];
            sendto(sockfd, &segments[packet_num], HEADER_SIZE + segments[packet_num].reserved[0], 0, (struct sockaddr*) &client_send, client_len);
            if(congestion_control){
                printf("Sending packet %d %d %d\n", segments[packet_num].SEQ_num, cwnd, threshold);
            }else{
                printf("Sending packet %d %d\n", segments[packet_num].SEQ_num, cwnd);
            }
            gettimeofday(&start_time, NULL);
        }
        
        
        // if all ack received
        if(first_nonACK_seg == segments.size()){
            break;
        }
    }

    // three way handshake FIN
    Packet* fin = new_Packet(0, 0, ack_num, nextSeqNum);
    fin->SYN = 0;
    fin->FIN = 1;
    nextSeqNum++;

    sendto(sockfd, fin, HEADER_SIZE, 0, (struct sockaddr*) &client_send, client_len);
    if(congestion_control){
        printf("Sending packet %d %d %d FIN\n", fin->SEQ_num, cwnd, threshold);
    }else{
        printf("Sending packet %d %d FIN\n", fin->SEQ_num, cwnd);
    }
    gettimeofday(&start_time, NULL);

    while(true){
        gettimeofday(&curr_time, NULL);
        if(is_timeout(start_time, curr_time)){
            sendto(sockfd, fin, HEADER_SIZE, 0, (struct sockaddr*) &client_send, client_len);
            if(congestion_control){
                printf("Sending packet %d %d %d Retransmission FIN\n", fin->SEQ_num, cwnd, threshold);
            }else{
                printf("Sending packet %d %d Retransmission FIN\n", fin->SEQ_num, cwnd);
            }
            //restart the clock
            gettimeofday(&start_time, NULL);
        }
        nbytes_received = recvfrom(sockfd, &msg, sizeof(msg), MSG_DONTWAIT, (struct sockaddr*) &client, &client_len);
        if(nbytes_received != -1){
            printf("Receiving packet %d\n", msg.ACK_num);
        }
        if(valid_ack(&msg)){
            sendBase = msg.ACK_num;
            break;
        }
    }

    while(true){
        nbytes_received = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*) &client, &client_len);
        if(msg.FIN == 1){
            break;
        }
    }

    printf("Receving packet %d\n", msg.ACK_num);

    Packet* finack = new_Packet(1,0,msg.SEQ_num + 1, msg.ACK_num);
    finack->SYN = 0;
    finack->FIN = 1;
    sendto(sockfd, finack, HEADER_SIZE, 0, (struct sockaddr*) &client_send, client_len);
    if(congestion_control){
        printf("Sending packet %d %d %d FIN\n", finack->SEQ_num, cwnd, threshold);
    }else{
        printf("Sending packet %d %d FIN\n", finack->SEQ_num, cwnd);
    }
    gettimeofday(&start_time, NULL);

    while(true){
        gettimeofday(&curr_time, NULL);
        if(is_FIN_timeout(start_time, curr_time)){
            break;
        }
        nbytes_received = recvfrom(sockfd, &msg, MAX_PACKET_SIZE, MSG_DONTWAIT, (struct sockaddr*)&client, &client_len);
        if((nbytes_received !=-1) && (msg.FIN == 1)){
            sendto(sockfd, finack, HEADER_SIZE, 0, (struct sockaddr*) &client_send, client_len);
            if(congestion_control){
                printf("Sending packet %d %d %d Retransmission FIN\n", finack->SEQ_num, cwnd, threshold);
            }else{
                printf("Sending packet %d %d Retransmission FIN\n", finack->SEQ_num, cwnd);
            }
        }
    }
    return 0;
}