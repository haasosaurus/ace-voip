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

#ifdef OS_WINDOWS
#include "winsock.h"
#include "winsock2.h"
#include "windef.h"
#include "windows.h"
#endif

void parse_cookie(char *);
int create_http_socket();
void copy_buffer_directory();

int create_http_socket(){

	int sock = 0;
#ifdef OS_WINDOWS
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == INVALID_SOCKET){
		printf("HTTP socket creation failed:%d\n",WSAGetLastError());
		exit(1);
	}
#else
	sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock<0){
		perror("Socket creation error \n");
		exit(1);
	}
#endif
	return sock;
}

void request_http_corporate_url(char *directory_buf,int message_id){

	struct sockaddr_in http_addr;
	int sock = 0,err = 0;
	char send_msg[2048] = {'\0'},recv_msg[2048] = {'\0'};
	int num_of_msg = 0,recv_err = 0;

	/* HTTP Socket creation and address and port intialization*/
	sock = create_http_socket();
	intialize_server_socket(&http_addr,directory_ip,http_port);
		
	/* Building HTTP GET Request message */ 
	memset(send_msg,'\0',sizeof(send_msg));	
	build_http_request(send_msg,directory_buf);

	/* Connects and sends HTTP Request message */
	send_http_packet(sock,&http_addr,send_msg);
	if(verbosity){
		printf("HTTP GET Request sent for %s to web server at %s \n",directory_buf,directory_ip);
	}	

	memset(recv_msg,'\0',sizeof(recv_msg));
	memset(send_msg,'\0',sizeof(send_msg));
	do {

		if(verbosity){
			printf("Waiting to receive HTTP message \n");
		}
		recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);

		if(recv_err < 0){
			perror("Error: Receiving HTTP message  \n");
			exit(1);
		}
	
		//Check whether we received 200 OK
		if(num_of_msg == 0){
			parse_http_response(recv_msg);
		}
	
		//printf("HTTP Message received is %s \n",recv_msg);
	
		if(recv_err > 0){

			if(message_id == 1 || message_id == 2){
				parse_url_tag(recv_msg);
			}
			else if(message_id == 3 && num_of_msg == 1){
				parse_first_recv(recv_msg);
			}
			else if(message_id == 3 && num_of_msg > 1){
				parse_other_recv(recv_msg);
			}
		}
		
	++num_of_msg;
	memset(recv_msg,'\0',sizeof(recv_msg));
	}while(recv_err > 0);

	close(sock);
}

void http_handler(){

	int num_of_msg = 0, return_prompt = 0;

	if(server_iis == 1){

		printf("Sending HTTP request for corporate directory, server uses asp format \n");
		directory_first = NULL;

		/* Send HTTP Request for the first msg --> argument 1 = first msg */
	        request_http_corporate_url(directory_url,1);
        	/*Parse the received URL for IP address and port number, this URL has the hyper link for xmldirectory.asp */
		if(directory_buf1 == NULL){
			fprintf(stderr,"XML format not supported, no url tag found\n");
			exit(-1);
		}	
        	parse_url(directory_buf1);

	        /* Sends second HTTP Request */
        	request_http_corporate_url(directory_url,2);
		if(directory_buf1 == NULL){
			fprintf(stderr,"XML format not supported, no url tag found\n");
			exit(-1);
		}
        	parse_url(directory_buf1);

	        /* Sends third HTTP Request */
        	request_http_corporate_url(directory_url,3);
        	/* Parse for prompt tags and get values */
        	int return_parse = parse_prompt();
		if(return_parse < 0){
			fprintf(stderr,"Error: parsing xml prompt tag, tag received is %s\n",prompt_string);
			exit(-1);
		}

		/* Allocates memory for the directory structure based on the total number of users list in the prompt tag */
                directory = calloc(sizeof(struct corporate_directory),directory_total);

	        while(directory_end < directory_total){
	
        	        char temp[8];
                	char directory_url_temp[256];
               	 	memset(temp,'\0',sizeof(temp));
               	 	memset(directory_url_temp,'\0',sizeof(directory_url_temp));
                	//strncpy(directory_url_temp,directory_url,sizeof(directory_url_temp) - strlen("?l=&f=&start=")-1);
			strcpy(directory_url_temp,directory_url);
                	strcat(directory_url_temp,"?l=&f=&start=");
                	sprintf(temp,"%d",directory_end + 1);
                	strcat(directory_url_temp,temp);

                	request_http_corporate_url(directory_url_temp,3);
                	int return_parse = parse_prompt();
			if(return_parse < 0){
				fprintf(stderr,"Error: parsing xml prompt tag, tag received is %s\n",prompt_string);
			}
        	}
		/* This copies the directory name and telephone number stored in the linked list buffer to the corporate structure */
		copy_buffer_directory();
	}
	else if(server_localdirectory == 1){

		fprintf(stdout, "Sending HTTP request for local corporate directory \n");
		
		/* HTTP Socket creation and address and port intialization*/
		struct sockaddr_in http_addr;
            	int sock = 0,err = 0;
        	char send_msg[4096] = {'\0'};
		char recv_msg[4096] = {'\0'};;
        	int recv_err = 0;
                directory_first = NULL;
                char *firstBuffer = NULL;

        	sock = create_http_socket();
        	intialize_server_socket(&http_addr,directory_ip,http_port);

		/* Building HTTP GET Request message */
        	memset(send_msg,'\0',sizeof(send_msg));
		/* Appends the search string to the url tobe requested */
		strcat(directory_url,"/search");
        	build_http_request(send_msg,directory_url);
		/* Connects to the web server and sends HTTP Request message */
        	send_http_packet(sock,&http_addr,send_msg);
		
		if(verbosity){
                fprintf(stdout,"HTTP GET Request sent for %s to web server at %s \n",directory_url,directory_ip);
                }

                memset(recv_msg,'\0',sizeof(recv_msg));
	        memset(send_msg,'\0',sizeof(send_msg));

                if(verbosity){
                        fprintf(stdout,"Waiting to receive HTTP message \n");
                }
		
                int previousMsgLength = 0;
                do{
                	recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);
                         if(recv_err < 0){
                                 printf("Error: Receiving HTTP response\n");
                                 close(sock);
                                 exit(-1);
                         }
                         if(firstBuffer == NULL){

                                 firstBuffer = (char *)calloc(recv_err+1,sizeof(char));
                                 if(firstBuffer == NULL){
                                         printf("Error: Allocating memory\n");
                                         close(sock);
                                         exit(-1);
                                 }
                                 memcpy(firstBuffer,recv_msg,recv_err);
                                 previousMsgLength = recv_err;
                         }
                         else{
                                char *tempRealloc = firstBuffer;
                                firstBuffer = (char *)calloc((previousMsgLength+recv_err+1),sizeof(char));
                                if(firstBuffer == NULL){
                                        printf("Error: Allocating memory\n");
                                        if(tempRealloc != NULL){
                                                free(tempRealloc);
                                        }
                                        close(sock);
                                        exit(-1);
                                }
                                memcpy(firstBuffer,tempRealloc,previousMsgLength);
                                free(tempRealloc);
                                memcpy(firstBuffer + previousMsgLength,recv_msg,recv_err);
                                previousMsgLength += recv_err;
                        }
                        memset(recv_msg,'\0',sizeof(recv_msg));
                }while(recv_err > 0);	
		
		parse_http_response(firstBuffer);
		return_prompt = parse_localdirectory(firstBuffer);
                if(return_prompt < 0){
                        printf("Error: Parsing XML directory data\n");
                        free(firstBuffer);
                        close(sock);
                        exit(-1);
                }

                free(firstBuffer);
                firstBuffer = NULL;
                close(sock);

                int return_parse = parse_prompt();
                if(return_parse < 0){
                        fprintf(stderr,"Error: parsing xml prompt tag, tag received is %s\n",prompt_string);
            		exit(-1);
        	}

                /* Allocates memory for the directory structure based on the total number of users list in the prompt tag */
                directory = calloc(sizeof(struct corporate_directory),directory_total);

                return_prompt = 0;

		while(directory_end < directory_total){

                		num_of_msg = 0;
                                char temp[8];
                                char directory_url_temp[256];
                                memset(temp,'\0',sizeof(temp));
                                memset(directory_url_temp,'\0',sizeof(directory_url_temp));
                                //strncpy(directory_url_temp,directory_url,sizeof(directory_url_temp)-strlen("&l=&f=&n=&start=")-1);
                                strcpy(directory_url_temp,directory_url);
                                strcat(directory_url_temp,"/search?l=&f=&n=&start=");
                                sprintf(temp,"%d",directory_end + 1);
                                strcat(directory_url_temp,temp);

                                sock = create_http_socket();
                                intialize_server_socket(&http_addr,directory_ip,http_port);

                               memset(send_msg,'\0',sizeof(send_msg));
                                build_http_request(send_msg,directory_url_temp);

                                /* Connects and sends HTTP Request message */
                                send_http_packet(sock,&http_addr,send_msg);
				
				do{
                                        memset(send_msg,'\0',sizeof(send_msg));
                                        memset(recv_msg,'\0',sizeof(recv_msg));
                                        recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);

                                        if(recv_err < 0){
                                        	printf("Error: Receiving HTTP response\n");
                                                close(sock);
                                                exit(-1);
                                        }

                                        if(firstBuffer == NULL){

                                        	firstBuffer = (char *)calloc(recv_err+1,sizeof(char));
                                                if(firstBuffer == NULL){
                                                	printf("Error: Allocating memory\n");
                                                        close(sock);
                                                        exit(-1);
                                                }
                                                memcpy(firstBuffer,recv_msg,recv_err);
                                                previousMsgLength = recv_err;
                                         }
                                         else{
                                         	char *tempRealloc = firstBuffer;
                                                firstBuffer = (char *)calloc((previousMsgLength+recv_err+1),sizeof(char));
                                                if(firstBuffer == NULL){
                                                	printf("Error: Allocating memory\n");
                                                        if(tempRealloc != NULL){
                                                        	free(tempRealloc);
                                                        }
                                                        close(sock);
                                                        exit(-1);
                                                }
                                                memcpy(firstBuffer,tempRealloc,previousMsgLength);
                                                free(tempRealloc);
                                                memcpy(firstBuffer + previousMsgLength,recv_msg,recv_err);
                                                previousMsgLength += recv_err;
                                        }
					
					memset(recv_msg,'\0',sizeof(recv_msg));
                                } while(recv_err > 0);

                                return_prompt = parse_localdirectory(firstBuffer);
                                if(return_prompt < 0){
                                	printf("Error: Parsing XML directory data\n");
                                        free(firstBuffer);
                                        close(sock);
                                        exit(-1);
                                }

                                free(firstBuffer);
                                firstBuffer = NULL;
                                close(sock);
	                        int return_parse = parse_prompt();
                                return_prompt = 0;
                                if(return_parse < 0){
                                        fprintf(stderr,"Error: parsing xml prompt tag, tag received is %s\n",prompt_string);
                                        exit(-1);
                                }
        	}
		copy_buffer_directory();
	}					

	else if(server_apache == 1){

		fprintf(stdout,"Sending HTTP request for corporate directory ~ likely UCM 5.x or 6.x server\n");
				
		struct sockaddr_in http_addr;
	    int sock = 0,err = 0;
        char send_msg[4096] = {'\0'};
		char recv_msg[4096] = {'\0'};;
        int recv_err = 0;
		directory_first = NULL;
		char *firstBuffer = NULL;

	    /* HTTP Socket creation and address and port intialization*/
        sock = create_http_socket();
        intialize_server_socket(&http_addr,directory_ip,http_port);

	    /* Building HTTP GET Request message */
        memset(send_msg,'\0',sizeof(send_msg));
        build_http_request(send_msg,directory_url);
		
	/* Connects to the web server and sends HTTP Request message */
        send_http_packet(sock,&http_addr,send_msg);
		
	if(verbosity){
        	fprintf(stdout,"HTTP GET Request sent for %s to web server at %s \n",directory_url,directory_ip);					
		}
	
		memset(recv_msg,'\0',sizeof(recv_msg));
        memset(send_msg,'\0',sizeof(send_msg));

		if(verbosity){
			fprintf(stdout,"Waiting to receive HTTP message \n");
		}

		int previousMsgLength = 0;
		do{
        	 recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);
			 if(recv_err < 0){
				 printf("Error: Receiving HTTP response\n");
				 close(sock);
				 exit(-1);
			 }
			 if(firstBuffer == NULL){

				 firstBuffer = (char *)calloc(recv_err+1,sizeof(char));
				 if(firstBuffer == NULL){
					 printf("Error: Allocating memory\n");
					 close(sock);
					 exit(-1);
				 }
				 memcpy(firstBuffer,recv_msg,recv_err);
				 previousMsgLength = recv_err;
			 }
			 else{
				char *tempRealloc = firstBuffer;
				firstBuffer = (char *)calloc((previousMsgLength+recv_err+1),sizeof(char));
				if(firstBuffer == NULL){
					printf("Error: Allocating memory\n");
					if(tempRealloc != NULL){
						free(tempRealloc);
					}
					close(sock);
					exit(-1);
				}
				memcpy(firstBuffer,tempRealloc,previousMsgLength);
				free(tempRealloc);
				memcpy(firstBuffer + previousMsgLength,recv_msg,recv_err);
				previousMsgLength += recv_err;
			}
			memset(recv_msg,'\0',sizeof(recv_msg));
		}while(recv_err > 0);
	
		parse_http_response(firstBuffer);
		parse_cookie(firstBuffer);
		int return_val = parse_first_two_messages(firstBuffer);
		if(return_val < 0){
				fprintf(stderr,"XML format not supported \n");
				free(firstBuffer);
				close(sock);
				exit(-1);
		}
		parse_url(directory_buf1);	
		close(sock);	
		free(firstBuffer);
		firstBuffer = NULL;

		sock = create_http_socket();
        intialize_server_socket(&http_addr,directory_ip,http_port);

		memset(send_msg,'\0',sizeof(send_msg));
        build_http_request(send_msg,directory_url);

		/* Connects and sends HTTP Request message */
                send_http_packet(sock,&http_addr,send_msg);

		if(verbosity){
			printf("HTTP GET Request sent for %s to web server at %s \n",directory_url,directory_ip);
			printf("Waiting to receive HTTP message \n");
		}
		
		memset(recv_msg,'\0',sizeof(recv_msg));
		do{
			recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);
			if(recv_err < 0){
				printf("Error: Receiving HTTP response\n");
				close(sock);
				exit(-1);
			}

			if(firstBuffer == NULL){

				 firstBuffer = (char *)calloc(recv_err+1,sizeof(char));
				 if(firstBuffer == NULL){
					 printf("Error: Allocating memory\n");
					 close(sock);
					 exit(-1);
				 }
				 memcpy(firstBuffer,recv_msg,recv_err);
				 previousMsgLength = recv_err;
			 }
			 else{
				char *tempRealloc = firstBuffer;
				firstBuffer = (char *)calloc((previousMsgLength+recv_err+1),sizeof(char));
				if(firstBuffer == NULL){
					printf("Error: Allocating memory\n");
					if(tempRealloc != NULL){
						free(tempRealloc);
					}
					close(sock);
					exit(-1);
				}
				memcpy(firstBuffer,tempRealloc,previousMsgLength);
				free(tempRealloc);
				memcpy(firstBuffer + previousMsgLength,recv_msg,recv_err);
				previousMsgLength += recv_err;
			}

			memset(recv_msg,'\0',sizeof(recv_msg));
		}while(recv_err > 0);

		memset(directory_buf1,'\0',sizeof(directory_buf1));
		parse_http_response(firstBuffer);
		parse_url_tag(firstBuffer);
		if(!strlen(directory_buf1)){
			fprintf(stderr,"XML format not supported, no url found \n");
			free(firstBuffer);
			close(sock);
			exit(-1);
		}
		parse_url(directory_buf1);
		close(sock);
		free(firstBuffer);
		firstBuffer = NULL;
		
		sock = create_http_socket();
        intialize_server_socket(&http_addr,directory_ip,http_port);			

		memset(send_msg,'\0',sizeof(send_msg));
	    build_http_request(send_msg,directory_url);

		/* Connects and sends HTTP Request message */
        send_http_packet(sock,&http_addr,send_msg);

		if(verbosity){
			printf("HTTP GET Request sent for %s to web server at %s \n",directory_url,directory_ip);
		}

		do{
			memset(send_msg,'\0',sizeof(send_msg));
			memset(recv_msg,'\0',sizeof(recv_msg));
	        recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);
	
			if(recv_err < 0){
				printf("Error: Receiving HTTP response\n");
				close(sock);
				exit(-1);
			}

			if(firstBuffer == NULL){

				 firstBuffer = (char *)calloc(recv_err+1,sizeof(char));
				 if(firstBuffer == NULL){
					 printf("Error: Allocating memory\n");
					 close(sock);
					 exit(-1);
				 }
				 memcpy(firstBuffer,recv_msg,recv_err);
				 previousMsgLength = recv_err;
			 }
			 else{
				char *tempRealloc = firstBuffer;
				firstBuffer = (char *)calloc((previousMsgLength+recv_err+1),sizeof(char));
				if(firstBuffer == NULL){
					printf("Error: Allocating memory\n");
					if(tempRealloc != NULL){
						free(tempRealloc);
					}
					close(sock);
					exit(-1);
				}
				memcpy(firstBuffer,tempRealloc,previousMsgLength);
				free(tempRealloc);
				memcpy(firstBuffer + previousMsgLength,recv_msg,recv_err);
				previousMsgLength += recv_err;
			}

			memset(recv_msg,'\0',sizeof(recv_msg));

		} while(recv_err > 0);
		
		return_prompt = parse_first_recv(firstBuffer);
		if(return_prompt < 0){
			printf("Error: Parsing XML directory data\n");
			free(firstBuffer);
			close(sock);
			exit(-1);
		}

		free(firstBuffer);
		firstBuffer = NULL;
		close(sock);

		int return_parse = parse_prompt();
		if(return_parse < 0){
			fprintf(stderr,"Error: parsing xml prompt tag, tag received is %s\n",prompt_string);
            exit(-1);
        }

			/* Allocates memory for the directory structure based on the total number of users list in the prompt tag */
			directory = calloc(sizeof(struct corporate_directory),directory_total);

			return_prompt = 0;
		
			while(directory_end < directory_total){

	       			num_of_msg = 0;	
		                char temp[8];
        	                char directory_url_temp[256];
                	        memset(temp,'\0',sizeof(temp));
          	                memset(directory_url_temp,'\0',sizeof(directory_url_temp));
                	        //strncpy(directory_url_temp,directory_url,sizeof(directory_url_temp)-strlen("&l=&f=&n=&start=")-1);
				strcpy(directory_url_temp,directory_url);
         	                strcat(directory_url_temp,"?l=&f=&n=&start=");
                	        sprintf(temp,"%d",directory_end + 1);
                     	   	strcat(directory_url_temp,temp);

				sock = create_http_socket();
		                intialize_server_socket(&http_addr,directory_ip,http_port);

		               memset(send_msg,'\0',sizeof(send_msg));
                		build_http_request(send_msg,directory_url_temp);

		                /* Connects and sends HTTP Request message */
                		send_http_packet(sock,&http_addr,send_msg);

				do{
        		                memset(send_msg,'\0',sizeof(send_msg));
                		        memset(recv_msg,'\0',sizeof(recv_msg));
                        		recv_err = recv(sock,recv_msg,sizeof(recv_msg),0);
								
								if(recv_err < 0){
									printf("Error: Receiving HTTP response\n");
									close(sock);
									exit(-1);
								}

								if(firstBuffer == NULL){

									firstBuffer = (char *)calloc(recv_err+1,sizeof(char));
									if(firstBuffer == NULL){
										printf("Error: Allocating memory\n");
										close(sock);
										exit(-1);
									}
									memcpy(firstBuffer,recv_msg,recv_err);
									previousMsgLength = recv_err;
								}
								else{
									char *tempRealloc = firstBuffer;
									firstBuffer = (char *)calloc((previousMsgLength+recv_err+1),sizeof(char));
									if(firstBuffer == NULL){
										printf("Error: Allocating memory\n");
										if(tempRealloc != NULL){
											free(tempRealloc);
										}
										close(sock);
										exit(-1);
									}
									memcpy(firstBuffer,tempRealloc,previousMsgLength);
									free(tempRealloc);
									memcpy(firstBuffer + previousMsgLength,recv_msg,recv_err);
									previousMsgLength += recv_err;
								}	
                	        	memset(recv_msg,'\0',sizeof(recv_msg));
                		} while(recv_err > 0);
			
						return_prompt = parse_first_recv(firstBuffer);
						if(return_prompt < 0){
							printf("Error: Parsing XML directory data\n");
							free(firstBuffer);
							close(sock);
							exit(-1);
						}

						free(firstBuffer);
						firstBuffer = NULL;
						close(sock);				
                        int return_parse = parse_prompt();
						return_prompt = 0;
						if(return_parse < 0){
              			        fprintf(stderr,"Error: parsing xml prompt tag, tag received is %s\n",prompt_string);
                        		exit(-1);
                		}
                	}		
	}
	copy_buffer_directory();
}						

void copy_buffer_directory(){

	struct corporate_directory *var,*tmp;
	int i=0;	

	var = directory_hl;	
	while(var != NULL){

		strncpy(directory[i].name,var->name,sizeof(directory[i].name)-1);
		strncpy(directory[i].telephone_number,var->telephone_number,sizeof(directory[i].telephone_number)-1);
		tmp = var;
		var = var -> next;
		free(tmp);
		i = i + 1;
	}
	directory_first = NULL;
} 
	
void parse_cookie(char *msg){


	char *temp;
	char *temp1;
	memset(cookie,'\0',sizeof(cookie));
	if((temp = strstr(msg,"Set-Cookie:")) != NULL){

		if((temp1 = strstr(temp,";")) != NULL){

			temp = temp + strlen("Set-Cookie: ");
			int length = temp1 - temp;				 
			
			strncpy(cookie,temp,sizeof(cookie)-1);
		}
	}
}
