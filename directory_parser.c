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

char 		temp_buf[64];
char		directory_name[64];
char 		directory_number[64];
int 		directory_start_part = 0;
int	 	number_start_tag_part = 0;
int 		number_value_part = 0;
int		number_end_tag_part = 0;
int		name_start_tag_part = 0;
int 		name_end_tag_part = 0;
int		name_value_part = 0;
int		directory_end_part = 0;
int		start_tag = 0;
int		directory_count = 0;
int		parse_tag_part = 0;
int 		prompt_value_part = 0;
int 		directory_end_start_tag = 0;
//extern int 	structure_size; 
//extern char 	prompt_string[256];
//extern char	directory_buf1[256];

int alter_name_format(char *, int, char *);			/* Name is format as <First name> <Last name> */
int check_number_value(char *,int,char *);
int parse_tag(char *,int, char *); 			/* Parses the message for start of <DirectoryEntry> or <Prompt> and returns the index */
int parse_part_tag(char *, int, char *);		/* Parses the remaining part of the tag value from the next message */ 
int parse_prompt_value(char *, int );			/* Parses the prompt tag values and copies it to (prompt_string) buffer */
int parse_prompt_value_part(char *,int);		/* Parses the remanining part of the tag value from the next message */

int alter_name_format(char *name,int name_size,char *new_name){

	char *ptr;
	char *first_name;
	char *last_name;
	char delims[] = ",";

	if((ptr=strrchr(name,',')) == NULL){
		return -1;
	}
	else{
		int length = strlen(name);
		//int fname_len = ptr-name+1;
		int fname_len = length - (ptr-name);
		last_name = (char *) calloc((ptr-name+1),sizeof(char));
		first_name = (char *) calloc((fname_len + 1),sizeof(char));
		//memset(last_name,'\0',sizeof(last_name));
		//memset(first_name,'\0',sizeof(first_name));		
		last_name = strtok(name,delims);
		strcpy(first_name,ptr+2);
		strcpy(new_name,first_name);
		strcat(new_name, " ");
		strcat(new_name,last_name);
		memset(name,'\0',name_size);
		strncpy(name,new_name,name_size-1);
		return 0;
	}
}
				
void print_corporate_data(){

    int i = 0;
	FILE *fp;

	char *file_name = "directory-users.txt";
	char new_name[64];
	
	fp = fopen(file_name,"w");

	if(fp == NULL){
		perror("Error opening file\n");
		exit(1);
	}
    

	for(i = 0; i < structure_size ; i++){
		
		memset(new_name,'\0',sizeof(new_name));
		int return_value = alter_name_format(directory[i].name,sizeof(directory[i].name),new_name);
		if(return_value < 0){
			fprintf(stderr,"First name and last name cannot be differentiated, skipping the name %s \n",directory[i].name);
		}
		else{
                	//printf("Retrieved Name: %s\tNumber: %s\n",directory[i].name,directory[i].telephone_number);
			fputs(directory[i].name,fp);
			fputs(",",fp);
			fputs(directory[i].telephone_number,fp);
			fputs("\n",fp);
		}

		directory_count++;
	}
	fprintf(stdout,"%d directory users written to file: %s\n",i,file_name);
	fclose(fp);
}

int parse_prompt_value_part(char *data,int new_pointer){

	int read_char , i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = { '\0' };

	read_char = data[i];
        if(read_char == '<'){
                memset(prompt_string,'\0',sizeof(prompt_string));
                strncpy(prompt_string,temp_buf,sizeof(prompt_string)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));
                return ++i;
        }

         while(read_char != '<'){
                read_char = data[i];++i;
                if(read_char != '<' && k < (sizeof(buffer)-1)){
                        buffer[k] = read_char;
                        k++;
                }
        }
        strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
        memset(prompt_string,'\0',sizeof(prompt_string));
        strncpy(prompt_string,temp_buf,sizeof(prompt_string)-1);
        memset(temp_buf,'\0',sizeof(temp_buf));

	return i;
}
	
int parse_prompt_value(char *data,int new_pointer){

	int read_char , i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = { '\0' };

	memset(buffer,'\0',sizeof(buffer));
        read_char = data[i];

        if(read_char == '\0'){
		prompt_value_part = 1;
                return -1;
        }

        if(read_char == '<'){  // NULL value <Name></Name>
                return ++i;
        }

        while(read_char != '<'){
                read_char = data[i];++i;
                if(read_char == '\0'){
                        prompt_value_part = 1;
                        strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
                        return -1;
                }
                if(read_char != '<' && k < sizeof(buffer)-1){
                        buffer[k] = read_char;
                        k++;
                }
        }

        strncpy(prompt_string,buffer,sizeof(prompt_string)-1);

        return i;
}

int parse_part_tag(char *data, int new_pointer, char *string){

	int read_char , i = 0, k = 0;
	i = new_pointer;
	char buffer[64] = {'\0'};

	read_char = data[i];

	if(read_char == '>'){
                strncpy(string,temp_buf,63);
                memset(temp_buf,'\0',sizeof(temp_buf));
                return ++i;
        }

        if(read_char == '<'){
                ++i;
        }

        if(start_tag == 1){
                while(read_char != '<'){
                        read_char = data[i];++i;
                }
                start_tag = 0;
        }

	memset(buffer,'\0',sizeof(buffer));
        while(read_char != '>'){
                read_char = data[i];++i;
                if(read_char != '>' && k < sizeof(buffer) - 1){
                        buffer[k] = read_char;
                        k++;
                }
        }
        strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
        strncpy(string,temp_buf,63);
        memset(temp_buf,'\0',sizeof(temp_buf));

	return i;
}

int parse_tag(char *data,int new_pointer,char *string){

	int read_char = 0, i = 0, k = 0;
	i = new_pointer;
	char buffer[64] = {'\0'};

	read_char = data[i];
	if(read_char == '\0'){
	
		start_tag = 1;	
		parse_tag_part = 1;
		return -1;
	}

	while(read_char != '<'){

		read_char = data[i];++i;
		if(read_char == '\0'){
			start_tag = 1;
			parse_tag_part = 1;
			return -1;
		}
	}

	memset(buffer,'\0',sizeof(buffer));
	
	while(read_char != '>'){

		read_char = data[i];++i;
		if(read_char == '\0'){
			
			parse_tag_part = 1;	
			strncpy(temp_buf,buffer,sizeof(temp_buf));
			return -1;
		}
		if(read_char != '>' && k < sizeof(buffer)-1){

			buffer[k] = read_char;
                        k++;
                }
        }

	strncpy(string,buffer,63);
	return i;
}
			

int parse_starting_tag(char *data,int new_pointer,char *string, int *flag){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'};

        read_char = data[i];
	if(read_char == '\0'){
		*flag = 1;
		start_tag = 1;
		return -1;
	}

        while(read_char != '<'){
			
			read_char = data[i];++i;
			if(read_char == '\0'){
				*flag = 1;
				start_tag = 1;
				return -1;
			}
        }

        memset(buffer,'\0',sizeof(buffer));

        while(read_char != '>'){
                read_char = data[i];++i;
                if( read_char == '\0'){
                        *flag = 1;
                        strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
                        return -1;
                }
                if(read_char != '>' && k < sizeof(buffer)-1){
                        buffer[k] = read_char;
                        k++;
                }
        }

        if(strcmp(buffer,string) == 0){
                return i;
        }
	else{
                return -1;
        }

}	

int parse_directory_end_tag_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'};
	char directory_end_tag[24] = {'\0'};	
	
	read_char = data[i];
        if(read_char == '>'){
                memset(directory_end_tag,'\0',sizeof(directory_end_tag));
                strncpy(directory_end_tag,temp_buf,sizeof(directory_end_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));

		directory_hl = calloc(sizeof(struct corporate_directory),1);
        	directory_hl->next = directory_first;
        	directory_first = directory_hl;
        	strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name)-1);
        	strncpy(directory_hl->telephone_number,directory_number,sizeof(directory_hl->telephone_number)-1);
        	structure_size = structure_size + 1;

                return ++i;
        }

	if(read_char == '<'){
                ++i;
        }

        if(directory_end_start_tag == 1){

                while(read_char != '<'){
                        read_char = data[i];++i;
                }
                directory_end_start_tag = 0;
        }

	read_char = data[i];

        if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
                memset(directory_end_tag,'\0',sizeof(directory_end_tag));
                strncpy(directory_end_tag,buffer,sizeof(directory_end_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));
        }
	 else{
                 while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
                memset(directory_end_tag,'\0',sizeof(directory_end_tag));
                strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf) - strlen(temp_buf) - 1);
                strncpy(directory_end_tag,buffer,sizeof(directory_end_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));
        }

	directory_hl = calloc(sizeof(struct corporate_directory),1);
	directory_hl->next = directory_first;
	directory_first = directory_hl;
	strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name));
	strncpy(directory_hl->telephone_number,directory_number,sizeof(directory_hl->telephone_number));
        structure_size = structure_size + 1;

        return i;
}

int parse_number_end_tag_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'},number_end_tag[16] = {'\0'};	

        read_char = data[i];
        if(read_char == '>'){
                memset(number_end_tag,'\0',sizeof(number_end_tag));
                strncpy(number_end_tag,temp_buf,sizeof(number_end_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));

		new_pointer = check_directory_end(data,++i);
		if(new_pointer < 0)
			return -1;

		directory_hl = calloc(sizeof(struct corporate_directory),1);
	        directory_hl->next = directory_first;
        	directory_first = directory_hl;
  	      	strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name));
     	   	strncpy(directory_hl->telephone_number,directory_number,sizeof(directory_hl->telephone_number));
        	structure_size = structure_size + 1;		

                return new_pointer;
        }
        else if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
                memset(number_end_tag,'\0',sizeof(number_end_tag));
                strncpy(number_end_tag,buffer,sizeof(number_end_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));
        }
	else{
                 while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
                memset(number_end_tag,'\0',sizeof(number_end_tag));
                strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
                strncpy(number_end_tag,buffer,sizeof(number_end_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));
        }

	new_pointer = check_directory_end(data,i);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error parsing directory end\n");
		}
                return -1;
        }

	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,directory_number,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,directory_name);
	//strcpy(directory[structure_size].telephone_number,directory_number);
	structure_size = structure_size + 1;

        return new_pointer;

}	


int parse_number_value_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'},number_value[64] = {'\0'};

        read_char = data[i];
        if(read_char == '<'){

                memset(number_value,'\0',sizeof(number_value));
                strncpy(number_value,temp_buf,sizeof(number_value));
                memset(temp_buf,'\0',sizeof(temp_buf));

		new_pointer = check_number_tag_end(data,++i);
		if(new_pointer < 0)
			return -1;

		new_pointer = check_directory_end(data,new_pointer);
		if(new_pointer < 0)
			return -1;	
		
		directory_hl = calloc(sizeof(struct corporate_directory),1);
                directory_hl->next = directory_first;
                directory_first = directory_hl;
                strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name));
                strncpy(directory_hl->telephone_number,number_value,sizeof(directory_hl->telephone_number));
                structure_size = structure_size + 1;
		                	
		return new_pointer;
        }

         while(read_char != '<'){
                read_char = data[i];++i;
                if(read_char != '<' && k < sizeof(buffer)-1){
                        buffer[k] = read_char;
                        k++;
                }
        }
        strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf) - strlen(temp_buf)-1);
        memset(number_value,'\0',sizeof(number_value));
        strncpy(number_value,temp_buf,sizeof(number_value));
        memset(temp_buf,'\0',sizeof(temp_buf));

	new_pointer = check_number_tag_end(data,i);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing number end tag\n");
		}
                return -1;
        }
        new_pointer = check_directory_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing directory end\n");
		}
                return -1;
        }

	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,number_value,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,directory_name);
	//strcpy(directory[structure_size].telephone_number,number_value);
	structure_size = structure_size + 1;

        return new_pointer;

}

int parse_number_start_tag_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'},number_start_tag[16] = {'\0'};

	char number_buffer[64] = {'\0'};
	memset(number_buffer,'0',sizeof(number_buffer));

        read_char = data[i];
	
        if(read_char == '>'){
                memset(number_start_tag,'\0',sizeof(number_start_tag));
                strncpy(number_start_tag,temp_buf,sizeof(number_start_tag)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));

		new_pointer = check_number_value(data,++i,number_buffer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_number_tag_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_directory_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		directory_hl = calloc(sizeof(struct corporate_directory),1);
	        directory_hl->next = directory_first;
        	directory_first = directory_hl;
        	strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name));
        	strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number));
	        structure_size = structure_size + 1;			

                return new_pointer;
        }

	if(read_char == '<'){
		++i;
	}

	if(start_tag == 1){
                while(read_char != '<'){
                        read_char = data[i];++i;
                }
                start_tag = 0;
        }

        while(read_char != '>'){
                read_char = data[i];++i;
                if(read_char != '>' && k < sizeof(buffer)-1){
                        buffer[k] = read_char;
                        k++;
                }
        }
        strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
        memset(number_start_tag,'\0',sizeof(number_start_tag));
        strncpy(number_start_tag,temp_buf,sizeof(number_start_tag)-1);
        memset(temp_buf,'\0',sizeof(temp_buf));

	new_pointer = check_number_value(data,i,number_buffer);
        if(new_pointer < 0){
		if(verbosity){
               		printf("Error at parsing number value\n");
		}
                return -1;
        }
        new_pointer = check_number_tag_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing number end tag\n");
		}
                return -1;
        }
        new_pointer = check_directory_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing directory end\n");
		}
                return -1;
        }

	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,directory_name);
	//strcpy(directory[structure_size].telephone_number,number_buffer);
	structure_size = structure_size + 1;

        return new_pointer;

}

int parse_name_end_tag_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'},name_end_tag[8] = {'\0'};

	char number_buffer[64] = {'\0'};
	memset(number_buffer,'\0',sizeof(number_buffer));

	read_char = data[i];
        if(read_char == '>'){
                memset(name_end_tag,'\0',sizeof(name_end_tag));
                strncpy(name_end_tag,temp_buf,sizeof(name_end_tag));
                memset(temp_buf,'\0',sizeof(temp_buf));

		memset(string,'\0',sizeof(string));
        	strncpy(string,"Telephone",sizeof(string)-1);
        	new_pointer = parse_starting_tag(data,++i,string,&number_start_tag_part);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_number_value(data,new_pointer,number_buffer);
        	if(new_pointer < 0)				
			return -1;

		new_pointer = check_number_tag_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_directory_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		directory_hl = calloc(sizeof(struct corporate_directory),1);
	        directory_hl->next = directory_first;
        	directory_first = directory_hl;
        	strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name));
        	strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number));
        	structure_size = structure_size + 1;

	        return new_pointer;
        }
	else if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
		memset(name_end_tag,'\0',sizeof(name_end_tag));
		strncpy(name_end_tag,buffer,sizeof(name_end_tag)-1);
		memset(temp_buf,'\0',sizeof(temp_buf));
        }
	else{
		 while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
		memset(name_end_tag,'\0',sizeof(name_end_tag));
		strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
		strncpy(name_end_tag,buffer,sizeof(name_end_tag)-1);
		memset(temp_buf,'\0',sizeof(temp_buf));
	}

	memset(string,'\0',sizeof(string));
        strncpy(string,"Telephone",sizeof(string)-1);
        new_pointer = parse_starting_tag(data,i,string,&number_start_tag_part);
        if(new_pointer < 0){
		if(verbosity){
               	 	printf("Error at parsing number start tag\n");
		}
                return -1;
        }
        new_pointer = check_number_value(data,new_pointer,number_buffer);
        if(new_pointer < 0){
		if(verbosity){
               	 	printf("Error at parsing number value\n");
		}
                return -1;
        }
        new_pointer = check_number_tag_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing number end tag\n");
		}
                return -1;
        }
        new_pointer = check_directory_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing directory end\n");
		}
                return -1;
        }

	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,directory_name,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,directory_name);
	//strcpy(directory[structure_size].telephone_number,number_buffer);
	structure_size = structure_size + 1;

        return new_pointer;
	
}
		

int parse_name_value_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'},name_value[64] = {'\0'};

	char number_buffer[64] = {'\0'};
	memset(number_buffer,'\0',sizeof(number_buffer));

        read_char = data[i];
        if(read_char == '<'){
                memset(name_value,'\0',sizeof(name_value));
                strncpy(name_value,temp_buf,sizeof(name_value)-1);
                memset(temp_buf,'\0',sizeof(temp_buf));
                
		new_pointer = check_name_tag_end(data,++i);
        	if(new_pointer < 0)
			return -1;

		memset(string,'\0',sizeof(string));
        	strncpy(string,"Telephone",sizeof(string)-1);
       	 	new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_number_value(data,new_pointer,number_buffer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_number_tag_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_directory_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		directory_hl = calloc(sizeof(struct corporate_directory),1);
	        directory_hl->next = directory_first;
        	directory_first = directory_hl;
	        strncpy(directory_hl->name,name_value,sizeof(directory_hl->name));
	        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number));
        	structure_size = structure_size + 1;

	        return new_pointer;
        }

	 while(read_char != '<'){
                read_char = data[i];++i;
                if(read_char != '<' && k < sizeof(buffer)-1){
                        buffer[k] = read_char;
                        k++;
                }
        }
        strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
        memset(name_value,'\0',sizeof(name_value));
        strncpy(name_value,temp_buf,sizeof(name_value)-1);
        memset(temp_buf,'\0',sizeof(temp_buf));

	new_pointer = check_name_tag_end(data,i);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing name end tag\n");
		}
                return -1;
        }
	
	memset(string,'\0',sizeof(string));
        strncpy(string,"Telephone",sizeof(string)-1);
        new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing number start tag\n");
		}	
                return -1;
        }
        new_pointer = check_number_value(data,new_pointer,number_buffer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing number value\n");
		}
                return -1;
        }
        new_pointer = check_number_tag_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing number end tag\n");
		}
                return -1;
        }
        new_pointer = check_directory_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity){
                	printf("Error at parsing directory end\n");
		}
                return -1;
        }

	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,name_value,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,name_value);
	//strcpy(directory[structure_size].telephone_number,number_buffer);
	structure_size = structure_size + 1;
	
        return new_pointer;

}
	

int parse_name_start_tag_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'},string[64] = {'\0'},name_start_tag[8] = {'\0'};

	char name_buffer[64] = {'\0'},number_buffer[64] = {'\0'};

	memset(buffer,'\0',sizeof(buffer));
	memset(string,'\0',sizeof(string));
	memset(name_start_tag,'\0',sizeof(name_start_tag));	
	memset(name_buffer,'\0',sizeof(name_buffer));
	memset(number_buffer,'\0',sizeof(number_buffer));

	read_char = data[i];
        if(read_char == '>'){

                memset(name_start_tag,'\0',sizeof(name_start_tag));
                strncpy(name_start_tag,temp_buf,sizeof(name_start_tag)-1);
		memset(temp_buf,'\0',sizeof(temp_buf));
		
		new_pointer = check_name_value(data,++i,name_buffer);
	        if(new_pointer < 0)
	                return -1;

		new_pointer = check_name_tag_end(data,new_pointer);
	        if(new_pointer < 0){
              	 	 return -1;
        	}
	        memset(string,'\0',sizeof(string));
        	strncpy(string,"Telephone",sizeof(string)-1);
	        new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);
        	if(new_pointer < 0){
                	return -1;
        	}
 	       new_pointer = check_number_value(data,new_pointer,number_buffer);
	        if(new_pointer < 0){
	                return -1;
        	}

	        new_pointer = check_number_tag_end(data,new_pointer);
        	if(new_pointer < 0){
        	        return -1;
        	}
        	new_pointer = check_directory_end(data,new_pointer);
        	if(new_pointer < 0){
                	return -1;
        	}

	        directory_hl = calloc(sizeof(struct corporate_directory),1);
        	directory_hl->next = directory_first;
        	directory_first = directory_hl;
        	strncpy(directory_hl->name,name_buffer,sizeof(directory_hl->name));
        	strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number));
        	structure_size = structure_size + 1;

		return new_pointer;
	
       	}

	if(read_char == '<'){
		++i;
	}
	
	if(start_tag == 1){

                while(read_char != '<'){
                        read_char = data[i];++i;
                }
                start_tag = 0;
        }
	

	while(read_char != '>'){
                read_char = data[i];++i;
                if(read_char != '>' && k < sizeof(buffer)-1){
                        buffer[k] = read_char;
                        k++;
                }
        }
        memset(name_start_tag,'\0',sizeof(name_start_tag));
	memset(temp_buf,'\0',sizeof(temp_buf));

	new_pointer = check_name_value(data,i,name_buffer);
	if(new_pointer < 0){
		if(verbosity){
			printf("Error at parsing name value\n");
		}
		return -1;
	}

	new_pointer = check_name_tag_end(data,new_pointer);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error at parsing name end tag\n");
		return -1;
	}
	memset(string,'\0',sizeof(string));
	strncpy(string,"Telephone",sizeof(string)-1);
	new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error at parsing number start tag\n");
		return -1;
	}
	new_pointer = check_number_value(data,new_pointer,number_buffer);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error at parsing number value\n");
		return -1;
	}

	new_pointer = check_number_tag_end(data,new_pointer);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error at parsing number end tag\n");
		return -1;
	}
	new_pointer = check_directory_end(data,new_pointer);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error at parsing directory end\n");
		return -1;
	}

	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,name_buffer,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,name_buffer);
	//strcpy(directory[structure_size].telephone_number,number_buffer);
	structure_size = structure_size + 1;

        return new_pointer;

}

int parse_directory_part(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
	i = new_pointer;
	char buffer[64] = {'\0'},string[64] = {'\0'},directory_entry[24] = {'\0'};

	char name_buffer[64] = {'\0'},number_buffer[64] = {'\0'};

	memset(name_buffer,'\0',sizeof(name_buffer));
	memset(number_buffer,'\0',sizeof(number_buffer));

	read_char = data[i];
	if(read_char == '>'){
		memset(directory_entry,'\0',sizeof(directory_entry));
		strncpy(directory_entry,temp_buf,sizeof(directory_entry)-1);
		memset(temp_buf,'\0',sizeof(temp_buf));

		memset(string,'\0',sizeof(string));
        	strncpy(string,"Name",sizeof(string)-1);
 	        new_pointer = parse_starting_tag(data,++i,string,&name_start_tag_part);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_name_value(data,new_pointer,name_buffer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_name_tag_end(data,new_pointer);
        	if(new_pointer < 0)		
			return -1;

		memset(string,'\0',sizeof(string));
        	strncpy(string,"Telephone",sizeof(string)-1);
        	new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);
        	if(new_pointer < 0)		
			return -1;

		new_pointer = check_number_value(data,new_pointer,number_buffer);
        	if(new_pointer < 0)			
			return -1;

		new_pointer = check_number_tag_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;

		new_pointer = check_directory_end(data,new_pointer);
        	if(new_pointer < 0)
			return -1;
	
		directory_hl = calloc(sizeof(struct corporate_directory),1);
	        directory_hl->next = directory_first;
        	directory_first = directory_hl;
	        strncpy(directory_hl->name,name_buffer,sizeof(directory_hl->name));
	        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number));
       	 	structure_size = structure_size + 1;

	        return new_pointer;
		
	}
	
	if(read_char == '<'){
		++i;
	}


	if(start_tag == 1){

                while(read_char != '<'){
                        read_char = data[i];++i;
                }
                start_tag = 0;
        }
	
	while(read_char != '>'){
		read_char = data[i];++i;
		if(read_char != '>' && k < sizeof(buffer)-1){
			buffer[k] = read_char;
			k++;
		}
	}
	strncpy(temp_buf + strlen(temp_buf),buffer,sizeof(temp_buf)-strlen(temp_buf)-1);
	memset(directory_entry,'\0',sizeof(directory_entry));
	strncpy(directory_entry,temp_buf,sizeof(directory_entry)-1);
	memset(temp_buf,'\0',sizeof(temp_buf));

	memset(string,'\0',sizeof(string));
	strncpy(string,"Name",sizeof(string)-1);
	new_pointer = parse_starting_tag(data,i,string,&name_start_tag_part);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error parsing name tag\n");
		return -1;
	}
	new_pointer = check_name_value(data,new_pointer,name_buffer);
	if(new_pointer < 0){
		if(verbosity)
                	printf("Error at parsing name value\n");
                return -1;
        }
        new_pointer = check_name_tag_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error at parsing name end tag\n");
                return -1;
        }
        memset(string,'\0',sizeof(string));
        strncpy(string,"Telephone",sizeof(string)-1);
        new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error at parsing number start tag\n");
                return -1;
        }
        new_pointer = check_number_value(data,new_pointer,number_buffer);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error at parsing number value\n");
                return -1;
        }
	new_pointer = check_number_tag_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error at parsing number end tag\n");
                return -1;
        }
        new_pointer = check_directory_end(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error at parsing directory end\n");
                return -1;
        }	
	
	directory_hl = calloc(sizeof(struct corporate_directory),1);
        directory_hl->next = directory_first;
        directory_first = directory_hl;
        strncpy(directory_hl->name,name_buffer,sizeof(directory_hl->name)-1);
        strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number)-1);
	//strcpy(directory[structure_size].name,name_buffer);
	//strcpy(directory[structure_size].telephone_number,number_buffer);
	structure_size = structure_size + 1;	
	
	return new_pointer;	

}	

int check_number_value(char *data,int new_pointer,char *target){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'};

        memset(buffer,'\0',sizeof(buffer));
	memset(temp_buf,'\0',sizeof(temp_buf));
        read_char = data[i];
	
	if(read_char == '\0'){
		number_value_part = 1;
		return -1;
	}	
	
	if(read_char == '<'){  // NULL value <Number></Number>
		return ++i;
	}

        while(read_char != '<'){
        	read_char = data[i];++i;
		if(read_char == '\0'){
			number_value_part = 1;
			strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
			return -1;
		}
        	if(read_char != '<' && k < sizeof(buffer)-1){
        		buffer[k] = read_char;
        		k++;
        	}
        } 
	strncpy(target,buffer,63);

        return i;

}

int check_number_tag_end(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64] = {'\0'};

        memset(buffer,'\0',sizeof(buffer));
        read_char = data[i];++i;
	if(read_char == '\0'){
		number_end_tag_part = 1;
		return -1;
	}

        if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
				if(read_char == '\0'){
					number_end_tag_part = 1;
					strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
					return -1;
				}
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
        }
        if(strcmp(buffer,"Telephone") == 0){
                return i;
        }
        else{
                return -1;
        }
}		

int check_name_tag_end(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64];

	memset(buffer,'\0',sizeof(buffer));
        read_char = data[i];++i;

	if(read_char == '\0'){
		name_end_tag_part = 1;
		return -1;
	}

        if(read_char == '/'){
        	while(read_char != '>'){
                	read_char = data[i];++i;
				if(read_char == '\0'){
					name_end_tag_part = 1;
					strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
					return -1;
				}
                        	if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
        }
        if(strcmp(buffer,"Name") == 0){
                return i;
        }
	else{
		return -1;
	}
}

int check_url_end_tag(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64];

        memset(buffer,'\0',sizeof(buffer));
        read_char = data[i];++i;

        if(read_char == '\0'){
                name_end_tag_part = 1;
                return -1;
        }

        if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char == '\0'){
                                        name_end_tag_part = 1;
                                        strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
                                        return -1;
                                }
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
        }
	if(strcmp(buffer,"URL") == 0){
		return i;
	}
	else{
		return -1;
	}
}

int check_menu_item_end_tag(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64];

        memset(buffer,'\0',sizeof(buffer));
        read_char = data[i];++i;

        if(read_char == '\0'){
                name_end_tag_part = 1;
                return -1;
        }
	while(read_char != '<'){
		read_char = data[i];++i;
	}
	read_char = data[i];++i;

        if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
                                if(read_char == '\0'){
                                        name_end_tag_part = 1;
                                        strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
                                        return -1;
                                }
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
        }
        if(strcmp(buffer,"MenuItem") == 0){
                return i;
        }
        else{
                return -1;
        }
}
			


int check_name_value(char *data,int new_pointer,char *target){

	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64];

	memset(buffer,'\0',sizeof(buffer));
	memset(temp_buf,'\0',sizeof(temp_buf));
	memset(target,'\0',sizeof(target));
	read_char = data[i];

	if(read_char == '\0'){
		name_value_part = 1;
		return -1;
	}

	//Handling Null Value <Name></Name>
	if(read_char == '<'){
		return ++i;
	}

        	while(read_char != '<'){
                	read_char = data[i];++i;
				if(read_char == '\0'){
					name_value_part = 1;
					strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
					return -1;
				}
                        	if(read_char != '<' && k < sizeof(buffer)-1){
                                	buffer[k] = read_char;
                                        k++;
                                }
                }

	strncpy(target,buffer,63);

	return i;

}


int check_directory_end(char *data,int new_pointer){
	
	int read_char = 0, i = 0, k = 0;
        i = new_pointer;
        char buffer[64];

        read_char = data[i];

	if(read_char == '\0'){
                directory_end_start_tag = 1;
		directory_end_part = 1;
                return -1;
        }

        while(read_char != '<'){
                read_char = data[i];++i;

		if(read_char == '\0'){
			directory_end_part = 1;
                        directory_end_start_tag = 1;
                        return -1;
                }
        }

        memset(buffer,'\0',sizeof(buffer));

	read_char = data[i];++i;
	if(read_char == '/'){
                while(read_char != '>'){
                        read_char = data[i];++i;
				if(read_char == '\0'){
					directory_end_part = 1;
					strncpy(temp_buf,buffer,sizeof(temp_buf)-1);
					return -1;
				} 
                                if(read_char != '>' && k < sizeof(buffer)-1){
                                        buffer[k] = read_char;
                                        k++;
                                }
                }
        }
        if(strcmp(buffer,"DirectoryEntry") == 0){
                return i;
        }
        else{
                return -1;
        }
}		
		

int check_prompt(char *data,int new_pointer){

	int read_char = 0, i = 0, k = 0;
	i = new_pointer;
	char buffer[64];

	read_char = data[i];++i;
	while(read_char != '<'){
		read_char = data[i];++i;
	}
	
	memset(buffer,'\0',sizeof(buffer));
	while(read_char != '>'){
		read_char = data[i];++i;
		if(read_char != '>' && k < sizeof(buffer)-1){
			buffer[k] = read_char;
			k++;
		}
	}
	if(strcmp(buffer,"Prompt") == 0){
	
		memset(buffer,'\0',sizeof(buffer));
		k = 0;
		if(read_char != '<'){
			while(read_char != '<'){
				read_char = data[i];++i;
				if(read_char != '<' && k < sizeof(buffer)-1){
					buffer[k] = read_char;
					k++;
				}
			}
		}

		memset(prompt_string,'\0',sizeof(prompt_string));
		strncpy(prompt_string,buffer,sizeof(prompt_string)-1);

		memset(buffer,'\0',sizeof(buffer));
		k = 0;
		read_char = data[i];++i;
		if(read_char == '/'){
			while(read_char != '>'){
				read_char = data[i];++i;
				if(read_char != '>' && k < sizeof(buffer)-1){
					buffer[k] = read_char;
					k++;
				}
			}
		}
		
		if(strcmp(buffer,"Prompt") == 0){
			return i;
		}
		else{
			return -1;
		}
	}
	else{
		return -1;
	}
}
	
	
int check_cisco_directory(char *data,int new_pointer){

	int read_char= 0, i = 0,k = 0;
	i = new_pointer;
	char buffer[64];

	memset(buffer,'\0',sizeof(buffer));
	
	while(read_char != '<'){
		read_char = data[i];++i;
	}

	if(read_char == '<'){
		while(read_char != '>'){
			read_char = data[i];++i;
			if(read_char != '>' && k < sizeof(buffer)-1){
				buffer[k] = read_char;
				k++;
			}
		}
	}
	if(strcmp(buffer,"CiscoIPPhoneDirectory") == 0){
		return i;
	}
	else{
		return -1;
	}
}	
		
int parse_directory_data(char *data, int new_pointer){

	int end_of_flag = 1;
	char string[64];

	char name_buffer[64],number_buffer[64];

	while(end_of_flag){

		memset(name_buffer,'\0',sizeof(name_buffer));
		memset(number_buffer,'\0',sizeof(number_buffer));
		memset(string,'\0',sizeof(string));

		if(parse_tag_part == 1){
	
			new_pointer = parse_part_tag(data,new_pointer,string);
			if(new_pointer < 0){
			
				end_of_flag = 0;
				break;
			}
			parse_tag_part = 0;
		}
		else {	
			new_pointer = parse_tag(data,new_pointer,string);
			if(new_pointer < 0){

				end_of_flag = 0;
				break;
			}
		}
	
		if(strcmp(string,"DirectoryEntry") == 0){
	
			memset(string,'\0',sizeof(string));
			strncpy(string,"Name",sizeof(string)-1);
			new_pointer = parse_starting_tag(data,new_pointer,string,&name_start_tag_part);
		
	                if(new_pointer < 0){
				if(name_start_tag_part == 1){
					if(verbosity)
						printf("Part of name start tag is parsed\n");
				}
				else{
					if(verbosity)
                        			printf("Error parsing name start tag\n");
				}
	                        end_of_flag = 0;
        	                break;
               	 	}

	                new_pointer = check_name_value(data,new_pointer,name_buffer);
        	        if(new_pointer < 0){
				if(name_value_part == 1){
					if(verbosity)
						printf("Part of name value is parsed\n");
				}

				else{
					if(verbosity)
                	        		printf("Error parsing name value\n");
				}
        	                end_of_flag = 0;
                	        break;
                	}

			new_pointer = check_name_tag_end(data,new_pointer);
  	                if(new_pointer < 0){
				if(name_end_tag_part == 1){
				if(verbosity)
					printf("Part of name tag end part is parsed\n");
				}

				else{
					if(verbosity)
                	        		printf("Error parsing named end tag\n");
				}
				memset(directory_name,'\0',sizeof(directory_name));
        	                strncpy(directory_name,name_buffer,sizeof(directory_name));
                	        end_of_flag = 0;
             	           break;
                	}

			memset(string,'\0',sizeof(string));
        	        strncpy(string,"Telephone",sizeof(string)-1);
                	new_pointer = parse_starting_tag(data,new_pointer,string,&number_start_tag_part);

	                if(new_pointer < 0){
				if(number_start_tag_part == 1){
					if(verbosity)
						printf("Part of number tag start part is parsed\n");
				}
				else{
					if(verbosity)
        	                		printf("Error parsing number start tag\n");
				}
				memset(directory_name,'\0',sizeof(directory_name));
        	                strncpy(directory_name,name_buffer,sizeof(directory_name)-1);
    	                	end_of_flag = 0;
        	                break;
        	        }
                	new_pointer = check_number_value(data,new_pointer,number_buffer);
	                if(new_pointer < 0){
				if(number_value_part == 1){
					if(verbosity)
						printf("Part of number value tag is read \n");
				}
				else{
					if(verbosity)
                	        		printf("Error: parsing number value \n");
				}
				memset(directory_name,'\0',sizeof(directory_name));
				strncpy(directory_name,name_buffer,sizeof(directory_name)-1);
	                        end_of_flag = 0;
        	                break;
	                }
	                new_pointer = check_number_tag_end(data,new_pointer);
	                if(new_pointer < 0){
				if(number_end_tag_part == 1){
					if(verbosity)
						printf("Part of number end tag is parsed \n");
				}	
				else{
					if(verbosity)
                	        		printf("Error: Parsing number end tag \n");
				}
				memset(directory_name,'\0',sizeof(directory_name));
				memset(directory_number,'\0',sizeof(directory_number));
	     	                strncpy(directory_name,name_buffer,sizeof(directory_name)-1);
				strncpy(directory_number,number_buffer,sizeof(directory_number)-1);
                	        end_of_flag = 0;
                        	break;
     		      }
		 	new_pointer = check_directory_end(data,new_pointer);
        	        if(new_pointer < 0){
				if(directory_end_part == 1){
					if(verbosity)
						printf("Part of directory end tag is parsed \n");
				}
				else{
					if(verbosity)
                        			printf("Error:Parsing directory end tag \n");
				}
				memset(directory_name,'\0',sizeof(directory_name));
	                        memset(directory_number,'\0',sizeof(directory_number));
        	                strncpy(directory_name,name_buffer,sizeof(directory_name)-1);
               		        strncpy(directory_number,number_buffer,sizeof(directory_number)-1);
                        	end_of_flag = 0;
                        	break;
                	}
	
			directory_hl = calloc(sizeof(struct corporate_directory),1);
        		directory_hl->next = directory_first;
        		directory_first = directory_hl;
        		strncpy(directory_hl->name,name_buffer,sizeof(directory_hl->name)-1);
        		strncpy(directory_hl->telephone_number,number_buffer,sizeof(directory_hl->telephone_number)-1);	
			//strncpy(directory[structure_size].name,name_buffer,sizeof(directory[structure_size].name));
			//strncpy(directory[structure_size].telephone_number,number_buffer,sizeof(directory[structure_size].telephone_number));
			structure_size = structure_size + 1;
        	}
		else if(strcmp(string,"Prompt") == 0){

			new_pointer = parse_prompt_value(data,new_pointer);
			if(new_pointer < 0){
				end_of_flag = 0;
				break;
			}
		
			return 2;
		}
	}
	return 0;
}

int parse_localdirectory(char *data){

	int new_pointer = 0,temp;
        int new_pointer_check = 0;
        char string[64];

	new_pointer = check_cisco_directory(data,new_pointer);
        if(new_pointer < 0){
                if(verbosity)
                        printf("Error: Parsing CiscoIPPhoneDirectory \n");
        }
        new_pointer_check = new_pointer;
        new_pointer = new_pointer + 1;

        new_pointer = check_prompt(data,new_pointer);
        if(new_pointer < 0){
                if(verbosity)
                        printf("Error: Parsing Prompt or No prompt tag found \n");
		return -1;
                //int retval = parse_directory_data(data,new_pointer_check);
                //return retval;
        }
	else{
                ++new_pointer;
                int retval = parse_directory_data(data,new_pointer);
                return retval;
        }
}
		

int parse_first_recv(char *data){

	int new_pointer = 0,temp;
	int new_pointer_check = 0;
	char string[64];
	
	memset(string,'\0',sizeof(string));
        strncpy(string,"?xml version=\"1.0\"?",sizeof(string)-1);
        new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error: Parsing xml version \n");
		new_pointer = 0;
        }

	new_pointer = check_cisco_directory(data,new_pointer);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error: Parsing CiscoIPPhoneDirectory \n");
	}
	new_pointer_check = new_pointer;
	new_pointer = new_pointer + 1;

	new_pointer = check_prompt(data,new_pointer); 
	if(new_pointer < 0){
		if(verbosity)
			printf("Error: Parsing Prompt or No prompt tag found \n");
		int retval = parse_directory_data(data,new_pointer_check);
		return retval;
	}
	else{
		++new_pointer;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
}

int parse_other_recv(char *data){

	int new_pointer = 0;

	if(prompt_value_part == 1){

		new_pointer = parse_prompt_value_part(data,new_pointer);
		if(new_pointer < 0){
			return -1;
		}
		prompt_value_part = 0;
		return 2;
	}

	else if(directory_start_part == 1){
		
		new_pointer = parse_directory_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing directory part \n");
			return -1;
		}
		directory_start_part = 0;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
	else if(name_start_tag_part == 1){
		new_pointer = parse_name_start_tag_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing name part \n");
			return -1;
		}
		name_start_tag_part = 0;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}

	else if(name_value_part == 1){
		new_pointer = parse_name_value_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing name value part \n");
			return -1;
		}
		name_value_part = 0;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
		
	else if(name_end_tag_part == 1){
		new_pointer = parse_name_end_tag_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing name end part \n");
			return -1;
		}
		name_end_tag_part = 0;
		int retval =  parse_directory_data(data,new_pointer);
		return retval;
	}
	
	else if(number_start_tag_part == 1){
		new_pointer = parse_number_start_tag_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing number start tag part \n");
			return -1;
		}
		number_start_tag_part = 0;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}

	else if(number_value_part == 1){

		new_pointer = parse_number_value_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing number value part \n");
			return -1;
		}
		number_value_part = 0;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
	else if(number_end_tag_part == 1){
		new_pointer = parse_number_end_tag_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing number end tag part \n");
			return -1;
		}
		number_end_tag_part = 0;
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
	else if(directory_end_part == 1){
		new_pointer = parse_directory_end_tag_part(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing directory end tag part \n");
			return -1;
		}
		directory_end_part = 0;	
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
	else {
		int retval = parse_directory_data(data,new_pointer);
		return retval;
	}
}

int parse_first_two_messages(char *data){

	
	char string[64], name_buffer[64];
	int new_pointer = 0;
	int temp;
	
	memset(string,'\0',sizeof(string));
	strncpy(string,"?xml version=\"1.0\"?",sizeof(string)-1); 
	new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error: Parsing xml version \n");
		return 2;
	}

	memset(string,'\0',sizeof(string));
	strncpy(string,"CiscoIPPhoneMenu",sizeof(string)-1);
	new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error: Parsing CiscoIPPhoneMenu \n");
		return -1;
	}

	new_pointer = new_pointer + 1;
	int tempNewPointer = new_pointer;	

	int noPrompt = 0;	
        new_pointer = check_prompt(data,new_pointer);
        if(new_pointer < 0){
		if(verbosity)
                	printf("Error Parsing Prompt \n");
		//return -1;
		noPrompt = 1;
        }
	if(!noPrompt){
        	++new_pointer;
	}
	else{
		new_pointer = tempNewPointer;
	}
	
	memset(string,'\0',sizeof(string));
	strncpy(string,"MenuItem",sizeof(string)-1);
	
	new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error Parsing MenuItem starting tag \n");
		return -1;
	}
	
	memset(string,'\0',sizeof(string));
	strncpy(string,"Name",sizeof(string)-1);

	new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error Parsing Name tag \n");
		return -1;
	}

	new_pointer = check_name_value(data,new_pointer,name_buffer);
	if(new_pointer < 0){
		if(verbosity)
			printf("Error Parsing name value \n");
		return -1;
	}

	if(strcmp(name_buffer,"Personal Directory") == 0){
		
		new_pointer = check_name_tag_end(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error parsing name tag end \n");
			return -1;
		}
		memset(string,'\0',sizeof(string));
		strncpy(string,"URL",sizeof(string)-1);
		new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error parsing url start tag \n");
			return -1;
		}
		
		memset(name_buffer,'\0',sizeof(name_buffer));
		new_pointer = check_name_value(data,new_pointer,name_buffer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error parsing url value \n");
			return -1;
		}

		if(verbosity)
			printf("Personal Directory URL:  %s\n", name_buffer);

		new_pointer = check_url_end_tag(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing URL end tag \n");
			return -1;
		}

		new_pointer = check_menu_item_end_tag(data,new_pointer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing MenuItem end tag\n");
			return -1;
		}			
			
		memset(string,'\0',sizeof(string));
		strncpy(string,"MenuItem",sizeof(string)-1);

		new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error:Parsing MenuItem start tag \n");
			return -1;
		}

		memset(string,'\0',sizeof(string));
		strncpy(string,"Name",sizeof(string)-1);
		new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error:Parsing Name start tag \n");
			return -1;
		}

        	new_pointer = check_name_value(data,new_pointer,name_buffer);
		if(new_pointer < 0){
			if(verbosity)
				printf("Error: Parsing name value \n");
			return -1;
		}

		if(strcmp(name_buffer,"Corporate Directory") == 0){

			new_pointer = check_name_tag_end(data,new_pointer);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error: Parsing name end tag \n");
				return -1;
			}
			
			memset(string,'\0',sizeof(string));
			strncpy(string,"URL",sizeof(string)-1);
			new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error: Parsing URL start tag \n");
				return -1;
			}
		
			memset(name_buffer,'\0',sizeof(name_buffer));
			new_pointer = check_name_value(data,new_pointer,name_buffer);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error parsing url value \n");
				return -1;
			}
			if(verbosity)
				printf("Corporate Directory URL:  %s\n",name_buffer);
			memset(directory_buf1,'\0',sizeof(directory_buf1));
			strncpy(directory_buf1,name_buffer,sizeof(directory_buf1));
	
			new_pointer = check_url_end_tag(data,new_pointer);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error parsing url end tag \n");
				return -1;
			}
			new_pointer = check_menu_item_end_tag(data,new_pointer);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error parsing menu item end tag \n");
				return -1;
			}
		}
	}
	else if(strcmp(name_buffer,"Corporate Directory") == 0){

			new_pointer = check_name_tag_end(data,new_pointer);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error parsing name end tag \n");
				return -1;
			}

                        memset(string,'\0',sizeof(string));
                        strncpy(string,"URL",sizeof(string)-1);
                        new_pointer = parse_starting_tag(data,new_pointer,string,&temp);
			if(new_pointer < 0){
				if(verbosity)
					printf("Error parsing url start tag \n");
				return -1;
			}

                        memset(name_buffer,'\0',sizeof(name_buffer));
                        new_pointer = check_name_value(data,new_pointer,name_buffer);
			if(new_pointer < 0){
				if(verbosity)
                                	printf("Error parsing url value \n");
                                return -1;
                        }

			memset(directory_buf1,'\0',sizeof(directory_buf1));
                        strncpy(directory_buf1,name_buffer,sizeof(directory_buf1)-1);

                        new_pointer = check_url_end_tag(data,new_pointer);
			if(new_pointer < 0){
				if(verbosity)
                                printf("Error parsing url end tag \n");
                                return -1;
                        }

                        new_pointer = check_menu_item_end_tag(data,new_pointer);
			if(new_pointer < 0){
				if(verbosity)
                                	printf("Error parsing menu item end tag \n");
                                return -1;
                        }

	}	
	return 0;
}	

int check_prompt_apache(char *data){

	int i = 0,read_char = 0,j = 0,k = 0;
	char tag_buf[256];

	while(data[i] != '\0'){

		read_char = data[i]; ++i;
		if(read_char == '<'){

			j = 0;
                        while(read_char != '>'){
                                if(data[i] == '\0')
                                        break;
                                read_char = data[i];++i;
                                if(read_char != '>' && j < sizeof(tag_buf)-1){
                                        tag_buf[j] = read_char;
                                        j++;
                                }
                        }
                }			
			
		if(strcmp(tag_buf,"Prompt") == 0){
                        k = 0;
                        memset(prompt_string,'\0',sizeof(directory_buf1));
                        while(read_char != '<'){
                                read_char = data[i];++i;
                                if(read_char == '\0')
                                        break;
                                if(read_char != '<' && k < sizeof(prompt_string)-1){
                                        prompt_string[k] = read_char;
                                        k++;
                                }
                        }
			return 1;
                }
	}
	return -1;		
}	
