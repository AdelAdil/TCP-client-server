#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/timeb.h>

#define SERVER_PORT     8080
#define BUFFER_LENGTH    1024
#define MAX_HOST_NAME_LENGTH 256
#define FALSE              0
#define SERVER_NAME     "ServerHostName"

//client
//Usage: rdps sender_ip sender_port receiver_ip receiver_port sender_file_name
//test ./rdps 192.168.1.100 8080 10.10.1.100 8080 /home/aadil/CSC370/out.txt

//state 0 = dead.
//state 1 = First SYN sent. changed after recieving SYN back
//state 2 = connection established.
//state 3 = recvd FIN.
//state 4 = RST recieved
//state 5 = Send ack for SYN. changed after sending 3rd packet

int state;
int total_data_bytes_sent;
int total_data_packets_sent;
int lost_packets;
int lost_data;
int slow_start = 1;
int expected_ack;
int file_size = 0;
int file_sent = 0;
int first_seq;
int first_recieved_seq;
int first_ack;
int expected_ack;
int num_packets = 0;
int l = 0;//number of packet to send

//RDP header struct, it is 20 bytes long without the header
typedef struct hdr{ //THE HEADER IS 32 Bytes long
	char Magic[10]; //magic is 12 bytes
	int type; //type is 4 bytes Type: 1:SYN 2:DAT 3:ACK 4:RST 5:FIN
	int seqno; // 4 bytes
	int ackno; // 4 bytes
	int length;
	int size;
	char payload[990];
}hdr;

hdr next_packet;
hdr recieved_packet;
char buffer[BUFFER_LENGTH];


//This function us run on every recieved data packet
//it extracts the information from a buffer into a header struct
//changing the state of the program in the process
hdr deserialize(char buffer[1024], struct sockaddr_in cli_addr, int event){
	hdr first;//ANY recieved packet is stored here
	int seqno, ackno, length, size, type;
	char Magic[10];
	char payload[990];

	char* token;

	token = strtok(buffer, ":");
	strncpy(first.Magic, token, 10);

	token = strtok(NULL, ":");

	first.type = atoi(token);
	token = strtok(NULL, ":");

	first.seqno = atoi(token);
	token = strtok(NULL, ":");

	first.ackno = atoi(token);
	token = strtok(NULL, ":");

	first.length = atoi(token);
	token = strtok(NULL, ":");

	first.size = atoi(token);
	token = strtok(NULL, ":");

	strcpy(first.payload, token);

	if(state == 2){
		return first;
	}
	
	if(first.type == 1 && state == 1){
		//if SYN. then we recieved packet 2 (SYN+ACK to our SYN)
		//Send ACK back. change state to send ack (state 5)
		if(first.ackno == first_seq +1){//We have recieved the right ack
			state = 5;
			first_ack = first.ackno;
			first_recieved_seq = first.seqno;
		}
		else{
			state = 4;//ABORT
		}
	}	
	if(first.type == 3 && state == 1){//if we get ack and we just sent syn.
		//we have date
		state = 2;
	}

	if(first.type == 5 && state == 2){//recieved FIN
		state = 3;//state finished
	}

	if(first.type == 4){//RST 
		state = 4;
	}

	//printf("packet de magic:%s type:%d seqno:%d ack:%d length:%d \n", first.Magic,first.type,first.seqno,first.ackno,first.length);
	int i = print_packet(first, cli_addr, event);
	return first;
}


int print_packet(hdr packet, struct sockaddr_in cli_addr, int event){
	char* type;	
	//printf("recieved packet type %d\n", packet.type);
	if (packet.type == 1){//SYN
		type = "SYN";
	}
	if (packet.type == 2){//DAT
		type = "DAT";

	}
	if (packet.type == 3){//ACK
		type = "ACK";
	}
	if (packet.type == 4){//RST
		type = "RST";
	}
	if (packet.type == 5){//FIN
		type = "FIN";
	}

	struct timeval  tv;
	gettimeofday(&tv, NULL);
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	char *now = asctime(timeinfo);
	int time_in_mill = (int) (tv.tv_usec);
	
	char time[11];
	strncpy(time, now + 11 , 8);
	time[8] = '\0';
	//printf("TIME IS %s.%d\n", time, time_in_mill);
	
	char* event_type;
	if(event == 1){//send 's'
		event_type = "s";
	}
	else if(event == 2){
		event_type = "S";
	}
	else if(event == 3){
		event_type = "r";
	}
	else if(event == 4){
		event_type = "R";
	}
	
	if(event == 3 || event == 4){

	printf("%s.%d %s %s:%d %s:%s %s %d/%d %d/%d \n", time, time_in_mill, event_type, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), "192.168.1.100" , "8080" , type , packet.seqno, packet.ackno, packet.length, packet.size);
	}

	else if(event == 1 || event == 2){
	printf("%s.%d %s %s:%s %s:%d %s %d/%d %d/%d \n", time, time_in_mill, event_type,"192.168.1.100" , "8080" , inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), type, packet.seqno, packet.ackno, packet.length, packet.size);
	}
return 1;
}

int print_packets(struct hdr a, struct hdr b, struct hdr c){
return 0;

}


int main(int argc, char *argv[]){

	int    sd=-1, rc, bytesReceived;
	int numbytes;
	char   server[MAX_HOST_NAME_LENGTH];
	struct sockaddr_in reciever_addr, sender_addr;
	int size;
	int not_all_ackd = 1;
	FILE *in_file;
	fd_set read_fd;
   	struct timeval timeout;
	
	if (argc < 6 || argc > 6) {
        	printf( "Usage: %s <sender_ip> <sender_port> <recv_ip> <recv_port> <send_file_name>\n", argv[0] );
        	fprintf(stderr,"ERROR, no address provided\n");
        	return -1;
	}
	int count = 0;
	in_file = fopen(argv[5], "r");
	if (in_file == NULL) {
		printf("Cannot open %s\n", argv[5]);
		return(-1);
	}
	

	//START READING FILE
	char ch;
	while (1) {
		ch = fgetc(in_file);
			if (ch == EOF)
            			break;
        		++count;
   		}
	fclose(in_file);
	char File_buffer[count];
	file_size = count;
	in_file = fopen(argv[5], "r");
	count = 0;
	if (in_file == NULL) {
		printf("Cannot open %s\n", argv[5]);
		return(-1);
	}

	while (1) {
		ch = fgetc(in_file);
		if (ch == EOF)
            		break;
        	File_buffer[count] = ch;
		count++;
   	}
	File_buffer[count - 1] = '\0';
	fclose(in_file);

	//END READING FILE


	if(count % 990 == 0){
		num_packets = count/990;
	}
	else{
		num_packets = (count/990) + 1;
	}
	hdr sender_window[num_packets];// array struct of data packets to send
	int ACKd[num_packets]; // 1 is ack'd. 0 is not ack'd. more means musltiple ACK's
	int leftover = num_packets % 3; // leftover packets that does not get sent as 3
	int packet_num3 = num_packets - leftover; //how many packets that are multiple of 3
	int last_expected_ack = (num_packets * 990) + 1;

	hdr packet1;
	//printf("header size is %zu \n", sizeof(packet1));
	//printf("numpackets is %d \n", num_packets);

	do{
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sd < 0){
        		perror("socket() failed");
        		break;
      		}

		
		memset(&sender_addr, 0, sizeof(sender_addr));
		//printf("sender boundnd to add:%s port:%s \n", argv[1], argv[2]);
		sender_addr.sin_family      = AF_INET;
		sender_addr.sin_port        = htons(atoi(argv[2]));
      		sender_addr.sin_addr.s_addr = inet_addr(argv[1]);
		//sender_addr.sin_addr.s_addr = inet_addr(argv[1]);
		//printf("%s:%d sender\n ", inet_ntoa(sender_addr.sin_addr), ntohs(sender_addr.sin_port));

		rc = bind(sd, (struct sockaddr *)&sender_addr, sizeof(sender_addr));
		if (rc < 0){
			perror("bind() failed");
			break;
		}

		//strcpy(server, argv[1]);
		//printf("send to ip:%s port:%s \n", argv[3], argv[4]);
		memset(&reciever_addr, 0, sizeof(reciever_addr));
      		reciever_addr.sin_family      = AF_INET;
      		reciever_addr.sin_port        = htons(atoi(argv[4]));
      		reciever_addr.sin_addr.s_addr = inet_addr(argv[3]);//10.10.1.100

	
		socklen_t reciever_length = sizeof(reciever_addr);
		socklen_t sender_length = sizeof(sender_addr);


		//SEND FIRST PACKET HERE
		hdr first;
		strncpy(first.Magic, "UVicCSc361", 11);
		first.type = 1;//First packet is type SYN
		first.seqno = 0;
		first.ackno = 0;
		first.length = 32;
		first.size = 0;
		//strncpy(first.payload, File_buffer, strlen(File_buffer));
		
		first_seq = first.seqno;
		sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n",first.Magic, first.type, first.seqno,first.ackno,first.length, first.size, first.payload);

		//printf("SENDING BUFFER : %s \n ", buffer);
		state = 1;
		print_packet(first, reciever_addr, 1);
		numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
		if (numbytes < 0){
			perror("sendto() failed");
			break;
		}
		timeout.tv_sec  = 2;
     		timeout.tv_usec = 0;

      		FD_ZERO(&read_fd);
      		FD_SET(sd + 1, &read_fd);

      		rc = select(sd, &read_fd, NULL, NULL, &timeout);
      		if (rc < 0)
      		{
         		perror("select() failed");
        		 break;
       		}

     		if (rc == 0){
       		  	print_packet(first, reciever_addr, 2);
			numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
			if (rc < 0){
				perror("sendto() failed");
				break;
			}
        			
     		}

		while(1){
			if ((numbytes = recvfrom(sd, buffer, strlen(buffer) , 0, (struct sockaddr *) &reciever_addr, &reciever_length)) == -1) {
            			perror("sws: error on recvfrom()!");
            			return -1;
       			}
			hdr recieved_packet = deserialize(buffer, reciever_addr, 3);
			//printf("state is %d after ser\n", state);
			//printf("after serialize");

			
			if(state == 5){
				//We recieved a SYN+ACK (second packet)
				//We now send an ACK (third packet)
				FD_ZERO(&read_fd);
				hdr ack_packet;
				strncpy(ack_packet.Magic, "UVicCSc361", 11);
				ack_packet.type = 3;//send ack 
				ack_packet.seqno = first_ack;//seq is the packet 2's ack
				ack_packet.ackno = first_recieved_seq + 1;//ack their seq
				ack_packet.length = 32;//does not matter for no payload packets
				ack_packet.size = num_packets;//This size is the number of packets i am sending (only in this packet)
				sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n",ack_packet.Magic, ack_packet.type, ack_packet.seqno, 						ack_packet.ackno, ack_packet.length, ack_packet.size, ack_packet.payload);
				numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
				if (rc < 0){
					perror("sendto() failed");
					break;
				}
				print_packet(ack_packet, reciever_addr, 1);
				//Now that we have sent the last ACK, the 3 way handshake is over
					timeout.tv_sec  = 2;
     					timeout.tv_usec = 0;

      					FD_ZERO(&read_fd);
      					FD_SET(sd, &read_fd);

      					rc = select(sd + 1, &read_fd, NULL, NULL, &timeout);
      					if (rc < 0){
         					perror("select() failed");
        				 	break;
       					}
					if (rc == 0){
       		  				print_packet(ack_packet, reciever_addr, 2);
						numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
						if (numbytes < 0){
							perror("sendto() failed");
							break;
						}
        			
     					}

				FD_ZERO(&read_fd);
				state = 2; //connection established

				//We now create our sender_window, Which is an array of hdr packets.
				//it records all the packets that we will send
				int i = 0;
				int this_file_size = strlen(File_buffer);
				int this_payload;
				int incr = 1;
				int read_bytes = 0;
				for (i = 0; i < num_packets ; i++){
					strncpy(sender_window[i].Magic, "UVicCSc361", 11);
					sender_window[i].type = 2;//All these are data packets
					sender_window[i].seqno = incr;//first seq is 1, 991, 1981... etc
					printf("packet %d seqno is %d \n", i, incr);
					sender_window[i].ackno = 1;//does not matter for sender data packets
					if((this_file_size - read_bytes) > 990){
						this_payload = 990;
						incr += 990;
						sender_window[i].length = 990;
						strncpy(sender_window[i].payload, File_buffer + read_bytes, 990);
						read_bytes += 990;
						continue;
					}
					else{
						this_payload = this_file_size - read_bytes;
						sender_window[i].length = this_payload;
						strncpy(sender_window[i].payload, File_buffer + read_bytes , this_payload);
						break;
					}
				}
			}//end state 5
			while(state == 2 && not_all_ackd){
				//start sending 3 packets from sender window
				int re_type = 1;
				while(l != packet_num3){
					//SEND PACKET 1
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", sender_window[l].Magic, sender_window[l].type, sender_window[l].seqno, sender_window[l].ackno, sender_window[l].length, sender_window[l].size, sender_window[l].payload);

					numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
					if (numbytes < 0){
						perror("sendto() failed");
						break;
					}
					print_packet(sender_window[l], reciever_addr, re_type);


					//SEND PACKET 2
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", sender_window[l+1].Magic, sender_window[l+1].type, 	sender_window[l+1].seqno, sender_window[l+1].ackno, sender_window[l+1].length, sender_window[l+1].size, sender_window[l+1].payload);

					numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
					if (numbytes < 0){
						perror("sendto() failed");
						break;
					}
					print_packet(sender_window[l+1], reciever_addr, re_type);

					//SEND PACKET 3

					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", sender_window[l+2].Magic, sender_window[l+2].type, 	sender_window[l+2].seqno, sender_window[l+2].ackno, sender_window[l+2].length, sender_window[l+2].size, sender_window[l+2].payload);

					numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
					if (numbytes < 0){
						perror("sendto() failed");
						break;
					}
					print_packet(sender_window[l+2], reciever_addr, re_type);



					//Try to recieve ACK
					//RECIEVE PACKET 1
					if ((numbytes = recvfrom(sd, buffer, strlen(buffer) , 0, (struct sockaddr *) &reciever_addr, &reciever_length)) == -1) 		{
            					perror("sws: error on recvfrom()!");
            					return -1;
       					}
					
					hdr packet1 = deserialize(buffer, reciever_addr, 3);

					if(packet1.type == 5 ){//Discard any non ack unless it is Fin
						if ((rc = recvfrom(sd, buffer, strlen(buffer) , 0, (struct sockaddr *) &reciever_addr, &reciever_length)) == -1) 			{
            					perror("sws: error on recvfrom()!");
            					return -1;
       						}
					packet1 = deserialize(buffer, reciever_addr, 4);
					}


					//Check if finished
					if(packet1.ackno == last_expected_ack){
							not_all_ackd = 0;
							state = 3;
							l+=3;
							break;
					}
					//check if correct ack, if so keep sending 3 packets
					//
					else if(packet1.ackno == sender_window[l+2].seqno + 990){
						FD_ZERO(&read_fd);
						ACKd[l] = 1;
						ACKd[l+1] = 1;
						ACKd[l+2] = 1;
	
						l += 3;
						re_type = 1;
						//Keep sending
						continue;
					}
					else{
						re_type = 2;//we didnt get an ack for all 3 packets. send them all again
						state = 2;//we restart sending
						continue;
					}
				}//done sending 3 packets
				int c = leftover;
				while(c != 0){
					c--;					
					int i = 0;
					
						//SEND leftover PACKET 
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", sender_window[l+i].Magic, sender_window[l+i].type, sender_window[l+i].seqno, sender_window[l+i].ackno, sender_window[l+i].length, sender_window[l+i].size, sender_window[l+i].payload);

					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
					if (rc < 0){
						perror("sendto() failed");
						break;
					}
					print_packet(sender_window[l+i], reciever_addr, 1);
					ACKd[l+i] = 1;
					i++;
				}//end leftover

				if(leftover != 0){
					if ((rc = recvfrom(sd, buffer, strlen(buffer) , 0, (struct sockaddr *) &reciever_addr, &reciever_length)) == -1) 				{
            					perror("sws: error on recvfrom()!");
            					return -1;
       					}
					hdr ack = deserialize(buffer, reciever_addr, 3);


					if(ack.ackno == last_expected_ack){
						state = 3;
						break;
					}
					else{
						break;//failed to send leftover packets
					}

				}//end leftover checking
				
				
					
			}//state 2


			if(state == 3){
				//send last message and print information
				hdr fin_packet;
				strncpy(fin_packet.Magic, "UVicCSc361", 11);
				fin_packet.type = 5;//send FIN 
				fin_packet.seqno = last_expected_ack;//seq is the packet 2's ack
				fin_packet.ackno = 1;//ack their seq
				fin_packet.length = 32;//does not matter for no payload packets
				fin_packet.size = 0;
				sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n",fin_packet.Magic, fin_packet.type, fin_packet.seqno, 						fin_packet.ackno, fin_packet.length, fin_packet.size, fin_packet.payload);
				numbytes = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&reciever_addr, reciever_length);
				if (rc < 0){
					perror("sendto() failed");
					break;
				}
				print_packet(fin_packet, reciever_addr, 1);

				while(1){
					if ((numbytes = recvfrom(sd, buffer, strlen(buffer) , 0, (struct sockaddr *) &reciever_addr, &reciever_length)) == -1) 		{
            					perror("sws: error on recvfrom()!");
            					return -1;
       					}
					hdr last_ack = deserialize(buffer, reciever_addr, 3);
					if (last_ack.ackno == last_expected_ack + 1){
						state = 3;
						break;
					}	
				}
			}

			if(state == 4){
				//RST
				
				break;
			}
			


		}//end while
	
		if(state == 4){
			printf("SENDER RESTARTING \n");
			continue;
		}
		if(state == 3){//fin
			printf("DONE program \n");
			break;
		}

	}while(FALSE);
	close(sd);

}
