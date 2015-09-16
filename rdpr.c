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


#define BUFFER_LENGTH	1024 
#define FALSE              0
#define MAXBUFLEN 256

//RDP header struct, it is 20 bytes long without the header
typedef struct hdr{ //THE HEADER IS 32 Bytes long
	char Magic[10]; //magic is 12 bytes
	int type; //type is 4 bytes
	int seqno; // 4 bytes
	int ackno; // 4 bytes
	int length;// 4 bytes length of payload
	int size; // size of sender_window
	char payload[990];
}hdr;

int state = 0;
//state 0 = dead.
//state 1 = recieved sync. awaiting ack
//state 2 = connection established.
//state 3 = recvd FIN.
//state 4 = RST recieved
int state;
int first_seq;

int num_packets = 0;
int l = 0;
int expected_seqno = 1;


//This function executes while the server is revieving.
//If 'q' is pressed on the keyboard during, then we quit rdps
int checkReadyForRead(int sockfd)
{
    while (1){
        char read_buffer[BUFFER_LENGTH];
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);
        int retval = select(sockfd+1, &readfds, NULL, NULL, NULL);
        if(retval <=0) //error or timeout
            return retval;
        else
        {
            if(FD_ISSET(STDIN_FILENO, &readfds) &&
               (fgets(read_buffer, MAXBUFLEN, stdin) != NULL) &&
               strchr(read_buffer, 'q') != NULL)  // 'q' pressed
                return 1;
            else if(FD_ISSET(sockfd, &readfds))   // recv buffer ready
                return 2;
        }
    }
    return -1;
}


hdr deserialize(char buffer[1024], struct sockaddr_in cli_addr, int event){
	hdr first;
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
	
	if(first.type == 1 && state == 0){//if SYN. first part of handshake
		state = 1;
		first_seq = first.seqno;
	}	

	if(first.type == 3 && state == 1){//if we get ack and we just sent syn.
		num_packets = first.size;
		state = 2;//connection established. handshake finished
	}

	if(first.type == 5 && state == 2){
		state = 3;//state finished
	}

	if(first.type == 4 && state == 2){
		state = 4;
	}

	//printf("packet de magic:%s type:%d seqno:%d ack:%d length:%d \n", first.Magic,first.type,first.seqno,first.ackno,first.length);
	int i = print_packet(first, cli_addr, event);
	return first;
}

//THIS is a dummy function that doesn't write everything to a file
// it is just here to show where and how I would print the packets from reciever_window
int print_packets(struct hdr a, struct hdr b, struct hdr c, char *file_name){
return 0;

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
	
	char* sip;
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

	printf("%s.%d %s %s:%d %s:%s %s %d/%d %d/%d \n", time, time_in_mill, event_type, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), "10.10.1.100" , "8080" , type , packet.seqno, packet.ackno, packet.length, packet.size);
	}

	else if(event == 1 || event == 2){
	printf("%s.%d %s %s:%s %s:%d %s %d/%d %d/%d \n", time, time_in_mill, event_type,"10.10.1.100" , "8080" , inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), type, packet.seqno, packet.ackno, packet.length, packet.size);
	}
return 1;
}

int main(int argc, char *argv[]){

	
	int    sd=-1, sd2=-1;
	int    rc, length_buf, cli_len, on=1;
	char   buffer[BUFFER_LENGTH];
	fd_set read_fd;
	struct timeval timeout;
	struct sockaddr_in serveraddr, cli_addr;
	
	if (argc < 4 || argc > 4) {
        printf( "Usage: %s <reciever_ip> <reciever_port> <file>\n", argv[0] );
        fprintf(stderr,"ERROR, no address provided\n");
        return -1;
	}
	
	do{
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sd < 0){
        		perror("socket() failed");
        		break;
      		}

		rc = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
		if (rc < 0){
			perror("setsockopt(SO_REUSEADDR) failed");
			break;
		}



		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sin_family      = AF_INET;
		serveraddr.sin_port        = htons(atoi(argv[2]));
		serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
		//printf("%s:%d serverIP\n ", inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port));

		//printf(" reciever bound to ip:%s port:%s \n ", argv[1], argv[2]);

		rc = bind(sd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if (rc < 0){
			perror("bind() failed");
			break;
		}

		int numbytes;

		//main program loop
		while(1){

			bzero((char *) &cli_addr, sizeof(cli_addr));
			cli_len = sizeof(cli_addr);
			//printf("recieving\n");

			int ret = checkReadyForRead(sd);//checks if q is pressed
        		if(ret == 1){
            		exit(1);
			}

			if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            			perror("sws: error on recvfrom()!");
            			return -1;
       			}

			hdr recieved_packet;

			//check the recieved packet for type
			recieved_packet = deserialize(buffer, cli_addr, 3);

			if(state == 1){
				//we recieved 1 SYN packet. We send a SYN packet back(intiate handshake 2)
				hdr second_syn;
				strncpy(second_syn.Magic, "UVicCSc361", 11);
				second_syn.type = 1;//we are sending SYN + ACK. rdps understands SYN as SYN + ACK
				second_syn.seqno = 0;//This can be any random number.
				second_syn.ackno = first_seq + 1;
				second_syn.length = 32;
				second_syn.size = 0;
				socklen_t cli_length = sizeof(cli_addr);
				sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n",second_syn.Magic, second_syn.type, second_syn.seqno, 						second_syn.ackno, second_syn.length, second_syn.size, second_syn.payload);
				print_packet(second_syn, cli_addr, 1);
				rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, cli_length);
				if (rc < 0){
					perror("sendto() failed");
					break;
				}
			}

			hdr reciever_window[3];// array struct of data packets recieved, maximum is 3 packets
			int ACKd[num_packets]; // 1 is ack'd. 0 is not ack'd. more means musltiple ACK's
			int leftover = num_packets % 3; // leftover packets that does not get sent as 3
			int packet_num3 = num_packets / 3; //how many packets of 3 are there

			while(state == 2){
				//RECIEVE DAT PACKET 1
				//If the recieved packet is not the expected sequence number or the 2 packets after, we drop it
				while(packet_num3 != 0){//Loop only if there is 3 packets or more				
				expected_seqno = (l * 990) + 1;
				
				if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       				}
				hdr packet1 = deserialize(buffer, cli_addr, 3);

				if (packet1.type == 3){//IF we recieve an ACK just discard. only recieve DAT packets
					if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       					}	
					packet1 = deserialize(buffer, cli_addr, 3);
				}
				
				if (packet1.type == 1){//IF we recieve a SYN just discard. only recieve DAT packets
					if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       					}	
					packet1 = deserialize(buffer, cli_addr, 3);
				}
				

				//Now that we know its a data packet
				if (packet1.seqno == expected_seqno){//first packet can go in 0
					reciever_window[0] = packet1;
				}
				else if (packet1.seqno == expected_seqno + 990){//first packet can go in 1
					reciever_window[1] = packet1;
				}
				else if (packet1.seqno == expected_seqno + 1980){//first packet can go in 2
					reciever_window[2] = packet1;
				}


				//RECIEVE DAT PACKET 2 and place in reciever buffer
				if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       				}
				hdr packet2 = deserialize(buffer, cli_addr, 3);

				if (packet1.type == 3){//IF we recieve an ACK just discard. only recieve DAT packets
					if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       					}	
					packet1 = deserialize(buffer, cli_addr, 3);
				}
				
				if (packet1.type == 1){//IF we recieve a SYN just discard. only recieve DAT packets
					if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       					}	
					packet1 = deserialize(buffer, cli_addr, 3);
				}
				
				if (packet2.seqno == expected_seqno){//first packet can go in 0
					reciever_window[0] = packet2;
				}
				else if (packet2.seqno == expected_seqno + 990){//first packet can go in 1
					reciever_window[1] = packet2;
				}
				else if (packet2.seqno == expected_seqno + 1980){//first packet can go in 2
					reciever_window[2] = packet2;
				}

				//RECIEVE DAT PACKET 3
				if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       				}
				hdr packet3 = deserialize(buffer, cli_addr, 3);

				if (packet1.type == 3){//IF we recieve an ACK just discard. only recieve DAT packets
					if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       					}	
					packet1 = deserialize(buffer, cli_addr, 3);
				}
				
				if (packet1.type == 1){//IF we recieve a SYN just discard. only recieve DAT packets
					if ((numbytes = recvfrom(sd, buffer, BUFFER_LENGTH , 0, (struct sockaddr *) &cli_addr, &cli_len)) == -1) {
            				perror("sws: error on recvfrom()!");
            				return -1;
       					}	
					packet1 = deserialize(buffer, cli_addr, 3);
				}

				if (packet1.seqno == expected_seqno){//first packet can go in 0
					reciever_window[0] = packet3;
				}
				else if (packet3.seqno == expected_seqno + 990){//first packet can go in 1
					reciever_window[1] = packet3;
				}
				else if (packet3.seqno == expected_seqno + 1980){//first packet can go in 2
					reciever_window[2] = packet3;
				}

				//DATA recieved, check if correct data
				//if any of the packets has a sequence number of 0, Discard currupt packets
				if(reciever_window[0].seqno == 0 || reciever_window[1].seqno == 0 || reciever_window[2].seqno == 0 ){
					hdr ack_goback;
					strncpy(ack_goback.Magic, "UVicCSc361", 11);
					ack_goback.type = 3;//ACK
					ack_goback.seqno = 0;//This can be any random number.
					expected_seqno = (l * 990) + 1;
					ack_goback.ackno = expected_seqno;// ack last acknowledged packet
					ack_goback.length = 32;//doesnt matter
					ack_goback.size = 0;
					socklen_t cli_length = sizeof(cli_addr);
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", ack_goback.Magic, ack_goback.type, ack_goback.seqno, 						ack_goback.ackno, ack_goback.length, ack_goback.size, ack_goback.payload);
					print_packet(ack_goback, cli_addr, 2);
					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, cli_length);
					if (rc < 0){
						perror("sendto() failed");
						break;
					}
					
					continue;
				}//end if


					//Susscessful packets have to be the right sequence number and ordered
				if(reciever_window[0].seqno == expected_seqno && reciever_window[1].seqno == (expected_seqno + 990) && reciever_window[2].seqno == expected_seqno + 1980){
					//PACKETS ARRIVED SUCCESSFULY
					//SEND collective ack the 3 packets (last packets seq + 990)
					hdr ack_succ;
					strncpy(ack_succ.Magic, "UVicCSc361", 11);
					ack_succ.type = 3;//ACK
					ack_succ.seqno = 0;//This can be any random number.
					ack_succ.ackno = expected_seqno + 990 + 990 + 990;
					// ack all packets. (ack is seqno of next byte)
					ack_succ.length = 32;//doesnt matter
					ack_succ.size = 0;
					socklen_t cli_length = sizeof(cli_addr);
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", ack_succ.Magic, ack_succ.type, ack_succ.seqno, 						ack_succ.ackno, ack_succ.length, ack_succ.size, ack_succ.payload);
					print_packet(ack_succ, cli_addr, 1);
					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, cli_length);
					if (rc < 0){
						perror("sendto() failed");
						break;
					}
					//print ackd packets here 
					print_packets(reciever_window[l], reciever_window[l+1], reciever_window[l+2], argv[4]);
					l += 3;
					continue;
				
				}
				else{
					//at this point, packets are reordered or duplicated or, not what we are expecting
				//Discard all 3 and send an ACK of the last acknowledged packet
				//GO-BACK-AND-N where n is 3
					hdr ack_goback;
					//printf("problem, resending else\n");
					strncpy(ack_goback.Magic, "UVicCSc361", 11);
					ack_goback.type = 3;//ACK
					ack_goback.seqno = 0;//This can be any random number.
					ack_goback.ackno = expected_seqno;// ack last acknowledged packet
					ack_goback.length = 32;//doesnt matter
					ack_goback.size = 0;
					socklen_t cli_length = sizeof(cli_addr);
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", ack_goback.Magic, ack_goback.type, ack_goback.seqno, 						ack_goback.ackno, ack_goback.length, ack_goback.size, ack_goback.payload);
					print_packet(ack_goback, cli_addr, 2);
					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, cli_length);
					if (rc < 0){
						perror("sendto() failed");
						break;
					}
					continue;
				}

				
				}//end 3packets
			
				//start handling leftover packets
				//leftover packets are not
				//just send an ack 
				if(leftover != 0){

					hdr ack_succ;
					strncpy(ack_succ.Magic, "UVicCSc361", 11);
					ack_succ.type = 3;//ACK
					ack_succ.seqno = 0;//This can be any random number.
					ack_succ.ackno = (num_packets*990) + 1;
					// ack all packets. (ack is seqno of next byte)
					ack_succ.length = 32;//doesnt matter
					ack_succ.size = 0;
					socklen_t cli_length = sizeof(cli_addr);
					sprintf(buffer, "%s:%d:%d:%d:%d:%d:%s\n", ack_succ.Magic, ack_succ.type, ack_succ.seqno, 						ack_succ.ackno, ack_succ.length, ack_succ.size, ack_succ.payload);
					print_packet(ack_succ, cli_addr, 1);
					rc = sendto(sd, buffer, strlen(buffer), 0, (struct sockaddr *)&cli_addr, cli_length);
					if (rc < 0){
						perror("sendto() failed");
						break;
					}
					l += 3;
					state = 3;
				}
				
				


			}//end while state 2
			if(state == 4){
				break;
			}
	
		}

		if(state == 4){
			printf("RESTARTING RECIEVER \n");
			continue;
		}

		if(state == 3){
			//print finsih stuff
			break;
		}

	}while (FALSE);

	close(sd);
}
