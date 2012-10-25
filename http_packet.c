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
#include "http_packet.h"
#include "tftp_client.h"

void build_http_request(char *packet,char *directory_buf){

        strncpy(packet, "GET ",strlen("GET "));
        strcat(packet,directory_buf);
        strcat(packet," HTTP/1.1\r\n");
        strcat(packet,"Host: ");
        strcat(packet,directory_ip);
	strcat(packet,":");
	char httpport[9] = {'\0'};
	sprintf(httpport,"%d",http_port);
	strcat(packet,httpport);
        strcat(packet,"\r\n");
	strcat(packet,"Connection: close\r\n");
	strcat(packet,"Cookie: ");
	strcat(packet,cookie);
	strcat(packet,"\r\n");
	strcat(packet,"User-Agent: Allegro-Software-WebClient/4.34\r\n");
	strcat(packet,"Accept: x-CiscoIPPhone/*, text/*,image/png,*/*\r\n");
	strcat(packet,"Accept-Language: en_US\r\n");
	strcat(packet,"Accept-Charset: iso-8859-1,utf-8;q=0.8\r\n");
	strcat(packet,"x-CiscoIPPhoneModelName: CP-7942G\r\n");
	strcat(packet,"x-CiscoIPPhoneSDKVersion: 6.0.2\r\n");
	strcat(packet,"x-CiscoIPPhoneDisplay: 298,144,3,G\r\n");
        //strcat(packet,"User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.7.8) Gecko/20050524 Fedora/1.0.4-4  Firefox/1.0.4\r\n"); 
        //strcat(packet,"Accept: text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5\r\n");
        //strcat(packet,"Accept-Language: en-us,en;q=0.5\r\n");
        //strcat(packet,"Accept-Encoding: gzip,deflate\r\n");
        //strcat(packet,"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n");
        //strcat(packet,"Keep-Alive: 300\r\n");
        //strcat(packet,"Connection: keep-alive\r\n");
        strcat(packet,"\r\n");
}

void send_http_packet(int sock, struct sockaddr_in* http_addr, char *msg){

	int con_err = 0, send_err = 0;
	
	con_err = connect(sock,(struct sockaddr*)http_addr, sizeof(struct sockaddr_in));
	if(con_err < 0){
		perror("Connection error to HTTP server \n");
		exit(-1);
	}

	send_err = sendto(sock,msg,strlen(msg),0,(struct sockaddr*)http_addr,sizeof(struct sockaddr_in));
	if(send_err < 0){
		perror("Error: Sending HTTP GET request message \n");
		exit(-1);
	}
} 

void parse_http_response(char *msg){

	char *result = NULL;
	
	result = strstr(msg,"HTTP/1.1 200 OK");
	if(result == NULL){
		fprintf(stderr,"Error: Receiving HTTP 200 OK \n");
		fprintf(stderr,"HTTP response received is %s \n",msg);
		exit(-1);
	}
	else{
		if(verbosity)
			printf("HTTP 200 OK Received \n");
	}
}	
