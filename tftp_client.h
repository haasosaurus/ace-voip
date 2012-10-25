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

#define READ_REQUEST 1
#define DATA_PACKET 3
#define ACK 4
#define ERROR_CODE 5
#define ERROR_CODE_FILE_NOT_FOUND 1

#define TFTP_SERVER_PORT 69
#define HTTP_PORT 80
#define DATA_SIZE 512

#define CORPORATE_FILE "DATA.TXT"

int	server_iis;
int	server_apache;
int 	server_localdirectory;
int	verbosity;
int	http_port;
char  cookie[64];	

void parse_url_tag(char *);
int xml_config_parser(char *);
void parse_url(char *);
void request_http_corporate_url(char *,int);
void intialize_server_socket(struct sockaddr_in *,char *,int);
int parse_prompt(); 
void print_corporate_data();
int parse_first_recv(char *);
int parse_other_recv(char *);
int tftp_handler(char *,char *);
void http_handler();
int check_prompt_apache(char *);
void parse_http_response(char *);
int parse_first_two_messages(char *);

char directory_buf[256],auth_buf[256],info_buf[256],services_buf[256],directory_buf1[256];
char directory_ip[16],directory_url[256];
char prompt_string[256];
int directory_start, directory_end, directory_total,structure_size;
char ucm_ip[16];

struct corporate_directory{

	char name[64];
	char telephone_number[64];
	struct corporate_directory *next;
};

struct corporate_directory *directory, *directory_first, *directory_hl;

struct tftp{

	unsigned short opcode;
	char file_name[80];
}__attribute__((packed));

struct tftp_recv{

	unsigned short opcode;
	unsigned short block;
	char data[513];
}__attribute__((packed));

struct tftp_ack{

	unsigned short opcode;
	unsigned short block;
}__attribute__((packed));

