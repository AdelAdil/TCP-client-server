# TCP-client-server
A client server model C implementation of the TCP protocol.
Networking course assignment at the University of Victoria

#Sender (Client)
Usage: rdps sender_ip sender_port receiver_ip receiver_port sender_file_name

After specifying the file name. The rdps program reads the file and starts sending 

#Reciever (Server)
Usage: rdpr receiver_ip receiver_port receiver_file_name

The Server should be running before the Client connects. It is preffered that it is run on IP: 10.0.0.1

The reciever will print the recieved file to the file specified in "reciever_file_name"

Both programs print logs of all the packets. This inculds time, packet type, source ip, destination ip, sequence number, acknowledment number, payload, window size.

#Protocol
The protocol transfers files of any size, by splitting the file to packets of size 990 byte. The header is 32 bytes. Making each packet 1022 bytes. The header contains:

1- Header name

2- Packet type

3- Sequence number

4- Acknowledgement number

5- Payload

6- Wiindow sizeh

The header fields are separated by semicolons. And are serialized and deserialized by special methods in both programs.

Flow Control: The sender sends packets three at a time. 

Error Control: It will only send the next three packets once it gets their Acknowledgment from the reciever. The three packets have to be in order. Mistakes can happen in the router. For example, 4,5,6 can arrive as 5,4,6. The reciever sends the Ack of 3 again to make the sender resend everything. This is to make sure no packets are reordered. The program will take a few more seconds because of that.



