default: server client

# compile server
server: packet.h server.cpp CongestionControl.h
	g++ -o server server.cpp -I.

# compile client
client: establishConnection.h safe_add.h setup_bind.h client.c handle_packet.h timeout_retransmission.h
	gcc -o client client.c -lpthread

# start running server
runserver:
	./server 127.0.0.1 7890

# test large file 20MB
testlarge:
	dd if=/dev/urandom of=test.bin bs=1MB count=20
	./client 127.0.0.1 7890 test.bin

# test big file 1MB
testbig:
	dd if=/dev/urandom of=test.bin bs=1MB count=1
	./client 127.0.0.1 7890 test.bin

# check data transformation comsistency
diff:
	diff test.bin received.data

# add 20% loss rate
addloss:
	sudo tc qdisc add dev lo root netem loss 100%

# add 20% loss rate and 100ms delay
addlossdelay:
	sudo tc qdisc add dev lo root netem loss 5% delay 100ms

# clean intermediate files
clean:
	rm server client received.data *.bin *.tar

# stop tc
stoptc:
	sudo tc qdisc del dev lo root

submission:
	tar -cvzf project2_104844760_204848946.tar client.c CongestionControl.h establishConnection.h handle_packet.h Makefile packet.h README safe_add.h server.cpp setup_bind.h timeout_retransmission.h Report.pdf