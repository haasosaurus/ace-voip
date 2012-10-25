/*
 * ACE - Automated Corporate Enumerator
 * Copyright (C) 2008 Sipera VIPER Lab
 * Author:  Jason Ostrom
 * Chief Developer:  Arjun Sambamoorthy
 *
 * This file is part of ACE.
 *
 * ACE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * ACE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "includes.h"
#include "tftp_client.h"

#ifndef OS_WINDOWS

#include "client.h"
#include "pathnames.h"
#include "signals.h"
#include "errno.h"
#include "syslog.h"
#include "sys/ioctl.h"
#include "sys/stat.h"

#endif

#ifdef OS_WINDOWS

#include "winsock.h"
#include "windef.h"

#endif

// This function converts ":" Deliminated MAC address in string format.(Removes : delimiter)
void mac2string (char *mac, char *s)
{
	while(*mac){
                if ( *mac == ':'){
                        *mac++;
                }
                *s = *mac;
                *mac++;
                *s++;
        }
        *s = '\0';
}



int send_ack_packet(int sock, struct sockaddr_in *saddr, short block, short port){

	int send_err = 0;
	struct tftp_ack tftp_ack_packet;
	
	tftp_ack_packet.opcode = htons(ACK);
        tftp_ack_packet.block = block;
        saddr->sin_port = port;

        send_err = sendto(sock,&tftp_ack_packet,sizeof(struct tftp_ack),0,(struct sockaddr *)saddr,sizeof(struct sockaddr_in));
        if(send_err < 0){
        	printf("Error sending TFTP ACK, %s\n",strerror(errno));
                return -1;
        }
	return 0;

}
	

int  recv_request_reply(int sock, struct sockaddr_in *saddr, char *file_name) {

	int recv_err = 0, data_size = DATA_SIZE;
	struct tftp_recv tftp_recv_packet;
	struct sockaddr_in saddr1;
	FILE *fp;
	ssize_t size_of;
	size_of = sizeof(saddr1);
	tftp_recv_packet.data[512] = '\0';

	while(data_size == 512) {  // Data size less than 512 implies last data packet

		memset(tftp_recv_packet.data,'\0',sizeof(tftp_recv_packet.data));
        	recv_err = recvfrom(sock,&tftp_recv_packet,sizeof(tftp_recv_packet),0,(struct sockaddr*)&saddr1,&size_of);
        	if(recv_err < 0){
                	printf("Error while receiving TFTP data packets,%s\n",strerror(errno));
                	exit(1);
        	}

        	if(tftp_recv_packet.opcode == htons(ERROR_CODE)){

			printf("***********************************\n");
                	printf("Received Error message:  %s \n",tftp_recv_packet.data);
                	data_size = strlen(tftp_recv_packet.data);
			return -1;
        	}
		else if(tftp_recv_packet.opcode == htons(DATA_PACKET)){

			// Please only print these lines in verbose mode
			//printf("**********************************\n");
                	//printf("Received data packet\n");
               		//printf("Block Number:  %d\n",ntohs(tftp_recv_packet.block));
                	//printf("Block size:  %d\n",strlen(tftp_recv_packet.data));
                	data_size = strlen(tftp_recv_packet.data);

			/* Writes Data into the file */
                	if(ntohs(tftp_recv_packet.block) == 1){

                        	fp = fopen(file_name,"w+");
                        	if(fp == NULL){
                                	printf("Error opening file:%s,%s \n",file_name,strerror(errno));
                                	exit(1);
                        	}
                	}
                	fputs(tftp_recv_packet.data,fp);
                	memset(tftp_recv_packet.data,'\0',512);

			/* Send ACK packet */
			int send_err = send_ack_packet(sock,saddr,tftp_recv_packet.block,saddr1.sin_port);
			if(send_err < 0)
				return -1;
        	}

     	}
	fclose(fp);	
	return 0;
		

}

int send_tftp_request(struct tftp *tftp_packet, char *file_name, char *mode, int sock, struct sockaddr_in *saddr){

	int send_err = 0;
	memset(tftp_packet->file_name,'\0',sizeof(tftp_packet->file_name));

	tftp_packet->opcode = htons(READ_REQUEST);
        memcpy(tftp_packet->file_name,file_name,strlen(file_name));
        memcpy(tftp_packet->file_name + strlen(file_name) + 1, mode, strlen(mode));
        strcpy(tftp_packet->file_name + strlen(file_name) + 1 + strlen(mode),"\0");

        send_err = sendto(sock,tftp_packet,sizeof(struct tftp)-80+2+strlen(file_name)+strlen("octet"),0,(struct sockaddr*)saddr,sizeof(struct sockaddr_in));
        if(send_err < 0){
                printf("Error: sending TFTP request for file %s, %s \n",file_name,strerror(errno));
               	return -1;
        }
	else{
		printf("TFTP request for file %s sent\n",file_name);
	}
	return 0;
	
}
	

void intialize_server_socket(struct sockaddr_in *saddr,char *tftp_server_ip,int port) {

	saddr->sin_port = htons(port);
	saddr->sin_family = AF_INET;
	saddr->sin_addr.s_addr = inet_addr(tftp_server_ip);
}

int create_socket(){

	int sock = 0;
#ifdef OS_WINDOWS

	WORD wVersionRequested;
	WSADATA wsaData;
	
	wVersionRequested = MAKEWORD(2,2);
	int wsa_error = WSAStartup(wVersionRequested, &wsaData);
	if(wsa_error != 0){

		printf("WSAStartup failed with error: %d\n",wsa_error);
		exit(1);
	}

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( sock == INVALID_SOCKET){
		printf("Error: Windows socket creation, error %d\n", WSAGetLastError());
		exit(1);
	}
#else
	sock = socket(AF_INET,SOCK_DGRAM,0);
	if(sock < 0){
		perror("Socket Creation \n");
		exit(1);
	}
#endif
	else{
		return sock;
	}
}

void create_file_from_mac(char *file_name, char *real_filename){

	char temp_file[64];

	if(strlen(file_name) != 12 ){
		printf("MAC Address should be 6 Bytes\n");
		printf("Example:  ace -m 00:1E:F7:28:9C:8e\n");
		exit(1);
	}
	strcpy(temp_file,"SEP");
	strcat(temp_file,file_name);
	strcat(temp_file,".cnf.xml");
	
	strcpy(real_filename,temp_file);
}

int tftp_handler(char *mac, char *tftp_server_ip){

	//printf("Running TFTP Handler\n");
	int sock = 0,data_size = 512,send_err = 0,file_number = 1, i = 0;
	char mode[16], real_filename[64];
	char mac_string[18];
	char temp_buf[8];
	struct sockaddr_in saddr;
	struct tftp tftp_packet;
	struct tftp_recv tftp_recv_packet;
	struct tftp_ack tftp_ack_packet;
	
	memset(mac_string,'\0',sizeof(mac_string));
	mac2string(mac,mac_string);

	/* Create the SEP.cnf.xml file name */	
	create_file_from_mac(mac_string,real_filename);

	/*Creates tftp socket and initializes the server address */
	sock = create_socket();
	intialize_server_socket(&saddr,tftp_server_ip,TFTP_SERVER_PORT);

	/*file transfer mode requested - octet*/
	memset(mode,'\0',sizeof(mode));
	strncpy(mode,"octet",strlen("octet"));
	
	/* Sends TFTP request */	
	send_err = send_tftp_request(&tftp_packet,real_filename,mode,sock,&saddr);
	if(send_err < 0)
		return -1;	

	/*Receive reply for TFTP request and sends ACK for data packetS received */
	int recv_err;
	recv_err = recv_request_reply(sock,&saddr,real_filename);

	if(recv_err < 0){
		close(sock);
		printf("Phone configuration file %s: not found\n", real_filename);
		return -1;
	}
	else if(recv_err >= 0){
		close(sock);
	}
	
	/* Extracts directory, authentication, services, informational URL from config file */
	int return_url = xml_config_parser(real_filename);
	if(return_url < 0){
		printf("Directory url not found in xml configuration file. Probably no corporate directory has been setup. \n");
		return -1;
	}

	/* check for asp or jsp */
	int return_val = check_file_format(directory_buf);
	if(return_val < 0 ){
		printf("File format not handled or wrong file format\n");
		return -1;
	}
	
	return 0;	

} 			

	
