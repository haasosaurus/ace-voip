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

void parse_url_tag(char *data){

	int read_char = 0, i = 0, j = 0, k = 0;
	char tag_buf[256];

	 while(data[i] != '\0'){

                memset(tag_buf,'\0',sizeof(tag_buf));
                read_char = data[i]; ++i;
                if(read_char == '<'){
                        j = 0;
                        while(read_char != '>'){
                                if(data[i] == '\0')
                                        break;
                                read_char = data[i];++i;
                                if(read_char != '>'){
                                        tag_buf[j] = read_char;
                                        j++;
                                }
                        }
                }
		if(strcmp(tag_buf,"URL") == 0){
                	k = 0;
                	memset(directory_buf1,'\0',sizeof(directory_buf1));
                	while(read_char != '<'){
                        	read_char = data[i];++i;
                        	if(read_char == '\0')
                                	break;
                        	if(read_char != '<'){
                                	directory_buf1[k] = read_char;
                                	k++;
                        	}
                	}
        		break;
        	}
	}
}

int xml_config_parser(char *file_name){

	FILE *fp;
	int file_char = 0, i = 0;
	char tag_buf[256];
	int  directory_url_yes = 0;

	printf("Successfully received file via TFTP, beginning to parse file name:  %s\n",file_name);
	
	fp = fopen(file_name,"r");
	if(fp == NULL){
		perror("File Open Error \n");
		exit(0);
	}

	while(feof(fp) == 0){

		file_char = fgetc(fp);
		if(file_char == '<'){
				i = 0;
				while(file_char != '>'){	
					file_char = fgetc(fp);
					if(file_char != '>'){		
						tag_buf[i] = file_char;
						i++;
					}
				}
		}
		if(strcmp(tag_buf,"directoryURL") == 0){
			file_char = 0,i = 0;
			while(file_char != '<'){ 
				file_char = fgetc(fp);
				if(file_char != '<'){
					directory_buf[i] = file_char;
					i++;
				}
			}
			directory_url_yes = 1;
			if(verbosity){
				printf("Directory URL:  %s\n",directory_buf);
			}
		}
		/*
		else if(strcmp(tag_buf,"authenticationURL") == 0){
                        file_char = 0,i = 0;
                        while(file_char != '<'){
                                file_char = fgetc(fp);
                                if(file_char != '<'){
                                        auth_buf[i] = file_char;
                                        i++;
                                }
                        }
                        //printf("Authentication URL:  %s\n",auth_buf);
                }
		else if(strcmp(tag_buf,"informationURL") == 0){
                        file_char = 0,i = 0;
                        while(file_char != '<'){
                                file_char = fgetc(fp);
                                if(file_char != '<'){
                                        info_buf[i] = file_char;
                                        i++;
                                }
                        }
                        //printf("Information URL:  %s\n",info_buf);
                }
		else if(strcmp(tag_buf,"servicesURL") == 0){
                        file_char = 0,i = 0;
                        while(file_char != '<'){
                                file_char = fgetc(fp);
                                if(file_char != '<'){
                                        services_buf[i] = file_char;
                                        i++;
                                }
                        }
                        //printf("Services URL:  %s\n",services_buf);
                }
		*/
		else if(strcmp(tag_buf,"processNodeName") == 0){

			file_char = 0, i = 0;
			while(file_char != '<'){
				file_char = fgetc(fp);
				if(file_char != '<'){
					ucm_ip[i] = file_char;
					i++;
				}
			}
		}
			
		memset(tag_buf,'\0',sizeof(tag_buf));
	}
	fclose(fp);
	
	if(directory_url_yes == 1){
		return 1;
	}
	return -1;

}
				
void parse_url(char *url){

	char temp_buf[256] = {'\0'}, temp_buf1[256] = {'\0'}, temp_buf2[256] = {'\0'}, port[16] = {'\0'};
	char *result = NULL;
	if((result = strstr(url,"http://")) == NULL){

		fprintf(stderr,"Invalid url found: %s\n",url);
		exit(-1);
	} 
	strcpy(temp_buf,url+strlen("http://"));
	memset(directory_ip, '\0', sizeof(directory_ip));      
	memset(port,'\0',sizeof(port));
	
	if(strrchr(temp_buf,':')){
		char delims[] = ":";
		result = strtok(temp_buf,delims);
		strncpy(directory_ip,result,sizeof(directory_ip)-1);
		char delims1[] = "/";
		strncpy(temp_buf1,url+strlen("http://")+strlen(directory_ip)+1,sizeof(temp_buf1)-1);
		result = strtok(temp_buf1,delims1);
		strncpy(port,result,sizeof(port)-1);
		http_port = atol(port);
		memset(directory_url,'\0',sizeof(directory_url));
		strncpy(directory_url,url+strlen("http://") + strlen(directory_ip) + 1 + strlen(port),sizeof(directory_url)-1);
	}
	else{
		char delims[] = "/";
		result = strtok(temp_buf,delims);
		memset(directory_ip,'\0',sizeof(directory_ip));
		strncpy(directory_ip,result,sizeof(directory_ip)-1);
		char delims1[] = "<";
		strncpy(temp_buf1,url+strlen("http://")+strlen(directory_ip),sizeof(temp_buf1)-1);
		result = strtok(temp_buf1,delims1);
		memset(directory_url,'\0',sizeof(directory_url));
		strncpy(directory_url,result,sizeof(directory_url)-1);
	}

	/* Resolve the host name of the server using dns queries */
	struct hostent *he;
	he = gethostbyname(directory_ip);
	if(he == NULL){
		perror("Error: gethostbyname");
		exit(1);
	}
	
	memset(directory_ip, '\0', sizeof(directory_ip));
	strncpy(directory_ip, inet_ntoa(*(struct in_addr*)he->h_addr),sizeof(directory_ip)-1);
	
}

int check_file_format(char *url){

	char *str1 = ".asp";
	char *str2 = ".jsp";
	char *str3 = "localdirectory";

	char *result = strstr(url,str1);
	if(result != NULL){
		
		server_iis = 1;
		return 0;
	}
	else{
		char *result1 = strstr(url,str2);
		if(result1 != NULL){
			
			server_apache = 1;
			return 0;
		}
		else{
			char *result2 = strstr(url,str3);
			if(result2 != NULL){
				//fprintf(stdout,"Dont know the web server format, there could be an error parsing the xml tags \n");
				server_localdirectory = 1;
				return -1;
			}
			fprintf(stdout,"The web server uses different format, ace does not handle the format. \n");
			return -1;
		}
	}
	return 0;
}	
	
int parse_prompt(){
	
	char *result = NULL, *result1 = NULL; 
	char value[8] = { '\0' },value1[8] = { '\0' };
	char prompt_buf[256] =  { '\0' };
	strcpy(prompt_buf,prompt_string + strlen("Records "));
	char delims[] = "to", delims1[] = "of";

	result = strtok(prompt_buf,delims);
	if(result != NULL){
		directory_start = atoi(result);	
		strcpy(value,result);
	
		memset(prompt_buf,'\0',sizeof(prompt_buf));
		strcpy(prompt_buf,prompt_string + strlen("Records ")+strlen(value)+strlen("to "));

		result1 = strtok(prompt_buf,delims1);
		if(result1 != NULL){
			directory_end = atoi(result1);
			strcpy(value1,result1);
	
			memset(prompt_buf,'\0',sizeof(prompt_buf));
			strcpy(prompt_buf,prompt_string + strlen("Records ") + strlen(value) + strlen("to ") + strlen(value1) + strlen("of "));
			directory_total = atoi(prompt_buf);
			return 0;
		}
	}
	return -1;
}
