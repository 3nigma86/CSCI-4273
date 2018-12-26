#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <memory.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>  
#include <pthread.h> 
#include <time.h>
#include <openssl/md5.h>
#include "openssl/ssl.h"
#include <dirent.h>

void get(char *);
void put(char *);
void list(char *);
void connect(char *);
void setup(char *);
void parse(char *);
void inthandler(int);
int md5file(char *);

char *password, *user, config[500],cmd[500], lsoutput[10000];
struct sockaddr_in socketstruct[4];
int socketints[4];


int main(int argc, char **argv){
	int fd;
	signal(SIGINT, inthandler);
	signal(SIGPIPE, SIG_IGN);

	if (argc != 2){
		printf("Must specify config file\n");
		exit(1);
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0){
		printf("Config file not found\n");
		exit(1);
	}
	//Getting the configuration setup by reading in config file specified and passing it to functions
	read(fd, config, 500);
	close(fd);
	setup(config);
	printf("returned from setup\n");
	//Infinite loop for reading user input. Must use exit call or ctrl+c to exit.
	while(1){
		bzero(cmd, 500);
		printf("\n\nPlease choose an operation to perform in the following format:\nget [filename]\nput [filename]\nlist\nexit\nEnter command:");
		fgets(cmd, 500, stdin);
		parse(cmd);
	}
return 1;
}


void setup(char config[]){
	char message[4][500], buf[500];
	int i = 0;

	//Setup parses out the username and password for the config and sends to servers
	strcpy(buf, config);
	user = strstr(buf, "Username:");
	user = strtok(user, "\n");
	password = strstr(buf, "Password:");
	password = strtok(password, "\n");
	//Must use config file to connect to servers before logging in can happen
	connect(config);
	sprintf(message[0], "%s\n%s\n",user, password);
	//Send message to servers for login.
	for(i = 0; i < 4; i++){
		send(socketints[i], message[0], 50,0);
		printf("Sent to server %d on socket %d\n",i + 1, socketints[i]);
	}
	bzero(message[0], 500);
	//Wait for reply to see if user is properly logged in or not.  
	for(i = 0; i < 4; i++){
		recv(socketints[i], message[i], 500, 0);
		printf("Server %d responds: %s\n",i + 1,message[i]);
	}
	if(strcmp(message[0], "Incorrect Password\n") == 0){
		kill(0, SIGKILL);
	}
	for(i = 0; i < 4; i++){
		bzero(message[i], 500);
	}
	printf("return from setup\n");
	return;
}



void connect(char config[]){
	char *sockets[] = {"DFS1", "DFS2", "DFS3", "DFS4"}, connectinfo[500];
	char *ip, *port;
	int porti, status, i, tried;


	//for loop processes the specified connection criteria out of config file
	for(i = 0; i < 4; i++){
		strcpy(connectinfo, config);
		if((socketints[i]=socket(AF_INET, SOCK_STREAM, 0)) < 0){
			printf("Socket Failed\n");
		}
		ip = strstr(connectinfo, sockets[i]);
		port = strtok(ip, "\n");
		port = strchr(port, ':');
		port = strpbrk(port, port+1);
		porti = atoi(port);
		ip = strtok(ip, ":");
		ip = strchr(ip, ' ');
		ip = strpbrk(ip, ip + 1);
		socketstruct[i].sin_family = AF_INET;
		socketstruct[i].sin_addr.s_addr = INADDR_ANY;
		socketstruct[i].sin_port = htons (porti);
		socketstruct[i].sin_addr.s_addr = inet_addr(ip);
		printf("CONFIGURED with %s for ip and %d port\n",ip, porti);
		tried = 0;
		LABEL4:
		if((status = connect(socketints[i], (struct sockaddr *)&socketstruct[i], sizeof(socketstruct[i]))) == -1){
			perror(sockets[i]);
			if(tried == 1){
				continue;
			}
			sleep(1);
			tried = 1;
			goto LABEL4;
		}
		else{
			printf("Connected to server %s\n", sockets[i]);

		}
	}
	printf("return from connect\n");
	return;
}



void list(char cmd[]){
	int found, i, str;
	char buffer[4][2000], bigbuffer[10000], *copybuffer, *tok, *tok2, filecheck[500], file[500], file2[500];


	//Send the list command for the servers and then read in the available files for each server into one buffer
	sprintf(buffer[0], "list");
	for(i = 0; i < 4; i++){
			send(socketints[i], buffer[0], 100,0);
	}
	bzero(buffer[0],2000);
	for(i = 0; i < 4; i++){
		recv(socketints[i], buffer[i], 2000,0);
		strcat(bigbuffer, buffer[i]);
	}

	copybuffer = bigbuffer;
	strcpy(lsoutput, "\n");
	//While loop takes out each file name and looks for all sections of the file
	//if all 4 parts of the file are found in the list then add to the list. 
	//if file is not found then concatenate the <incomplete> message.
	while((tok = strsep(&copybuffer, "|")) != NULL){
		if(strcmp(tok, ".") == 0 || strcmp(tok, "..")== 0){
			continue;
		}
		if(strlen(copybuffer)==0){
			break;
		}
		tok2 = tok;
		strcpy(file, tok2);
		str = strlen(file);
		file[str - 2] = '\0';	
		strcpy(file,file + 1);
		if(strstr(lsoutput, file)!=NULL){
			continue;
		}
		found = 0;
		for(i = 1; i < 5; i++){
			sprintf(filecheck, "%s.%d",file, i);
			if(strstr(copybuffer, filecheck) != NULL){
				found++;
			}
			bzero(filecheck, 500);
		}
		if(found == 4){
			strcat(lsoutput, file);
			strcat(lsoutput, "\n");
		}
		else{
			strcat(lsoutput, file);
			strcat(lsoutput, " <INCOMPLETE>\n");
		}
		

	}
	
return;
}

void get(char cmd[]){
	int fd, i, offset, readsize, j, toread = 1, filesize, bytesread;
	char buffer[50000], message[4][500], *file, *offsetc, filebuf[500], *check;

	
	//Parse out the file name and send to the servers for finding. 
	strcpy(filebuf, cmd);
	file = strtok(filebuf, "\n");
	strcpy(file, file + 4);
	//file == strpbrk(file, file + 1);
	list(cmd);
	if((check = strstr(lsoutput, file))!= NULL){
		check = strtok(check, "\n");
		printf("checking = %s\n",check);
		if(strlen(check) > strlen(file) + 1){
			printf("Cannot get incomplete file\n");
			bzero(lsoutput, 10000);
			return;
		}
	}
	else{
		printf("File not found in database\n");
		bzero(lsoutput, 10000);
		return;
	}
	bzero(lsoutput, 10000);

	for(i = 0; i < 4; i++){
		send(socketints[i], cmd, 100,0);
		printf("Sending... %s\n",cmd);
	}
	//Takes in the filenames and filesize and uses to set an offset value depending on 
	//which piece it is.  Open the file, seek in by offset and write data, then close file. 
	for(i = 0; i < 4; i++){
		for(j = 0; j < 2; j++){
			fd = open(file, O_CREAT | O_RDWR, 0600);
			if (fd < 0){
				printf("Cannot open file in get\n");
				return;
			}
			printf("Waiting for filename...\n");
			printf("socket = %d\n", socketints[i]);
			recv(socketints[i], message[i], 50, 0);
			printf("Server %d responds: %s\n",i + 1,message[i]);
			if(strlen(message[i]) > 0){
				offsetc = strstr(message[i], "|");
				offsetc = strpbrk(offsetc, offsetc + 1);
				offsetc = strtok(offsetc, "|");
				filesize = atoi(offsetc);
				printf("filesize = %d\n",filesize);
				toread = 1;
				if((strstr(message[i], ".1")) != NULL){
					offset = filesize;
					printf(".1 offset = %d\n",offset);
				}
				else if((strstr(message[i], ".2")) != NULL){
					offset = filesize;
					lseek(fd, offset, SEEK_SET);
					printf(".2 offset = %d\n",offset);
				}
				else if((strstr(message[i], ".3")) != NULL){
					offset = filesize;
					lseek(fd, offset * 2,SEEK_SET);
					printf(".3 offset = %d\n",offset);
				}
				else if((strstr(message[i], ".4")) != NULL){
					lseek(fd, offset * 3, SEEK_SET);
					printf(".4 offset = %d\n",offset);
				}
				else if((strcmp(message[i], "Unable to locate file")) == 0){
					bzero(message[i], 500);
					break;
				}
				bytesread = 0;
				while(1){
					readsize = recv(socketints[i], buffer, 1 , 0);
					write(fd, buffer, readsize);
					bytesread += readsize;
					if(bytesread == filesize){
						break;
					}
				}
				bzero(message[i], 500);
			}
			close(fd);
		}
	}
	for(i = 0; i < 4; i++){
		bzero(message[i], 500);
	}
	bzero(cmd, 500);
	return;
}

void put(char cmd[]){
	int fd, status, filesize, remainder, mod, putsockets[4], temp, read_size, bytesread, i, distr;
	char buffer [50000], message[4][500], putfile[500], splitfile[500], *filestr, cmd2[500];
	struct stat fileStat;


	//Take out the filename, open file and get size of file.  
	strcpy(cmd2, cmd);
	filestr = cmd2;
	for (i = 0; i < 4; i++){
		send(socketints[i], cmd, 100,0);
		printf("Put sending...%s\n", cmd);
	}
	bzero(putfile, 500);
	filestr = strtok(filestr, "\n");
	filestr = strrchr(filestr, ' ');
	filestr = strpbrk(filestr, filestr + 1);
	strcpy(putfile, filestr);
	printf("filestr = %s and putfile = %s\n",filestr, putfile);
	status = stat(filestr, &fileStat);
	filesize = fileStat.st_size;
	remainder = filesize % 4;
	//Make sure filesize is properly divided by 4 with no remainder, do mod to grab remainder
	//on file to add to the 4th piece of the file everytime. 
	while(filesize%4 != 0){
		filesize--;
	}
	filesize = filesize/4;
	//Call md5 sum to get file distribution. 
	distr = md5file(filestr);
	printf("Remainder = %d and filesize = %d\n",distr, filesize);
	for(i=0; i<4; i++){
		putsockets[i] = socketints[i];
	}
	//Use the md5sum value to rotate the sockets to the correct spot in the array so 
	//file is distributed to the correct spots.  
	for(i = 0; i < distr; i++){
			temp = putsockets[0];
			putsockets[0] = putsockets[1];
			putsockets[1] = putsockets[2];
			putsockets[2] = putsockets[3];
			putsockets[3] = temp;
	}
	fd = open(filestr, O_RDONLY);
	printf("fd = %d\n",fd);
	if (fd < 0){
		printf("Unable to open %s\n",filestr);
		return;
	}
	//String of operations that resplice the filename to match which part of the file it is.
	//use the filesize operations to determine how much of the file to read and send to server.
	bytesread = 0;
	strcat(splitfile,putfile);
	sprintf(putfile, ".%s.1|%d|",splitfile, filesize);
	printf("Splitfile = %s\n",putfile);
	send(putsockets[0],putfile,30,0);
	send(putsockets[3],putfile,30,0);
	while(bytesread < filesize){
		read_size = read(fd, buffer, 1);
		bytesread += read_size;
		send(putsockets[0],buffer, read_size, 0);
		send(putsockets[3],buffer, read_size, 0);
		bzero(buffer,50000);
	}
	bytesread = 0;


	sprintf(putfile, ".%s.2|%d|",splitfile, filesize);
	printf("Splitfile = %s\n",putfile);
	send(putsockets[0],putfile,30,0);
	send(putsockets[1],putfile,30,0);
	while(bytesread < filesize){
		read_size = read(fd, buffer, 1);
		bytesread += read_size;
		send(putsockets[0],buffer, read_size, 0);
		send(putsockets[1],buffer, read_size, 0);
		bzero(buffer, 50000);
	}
	bytesread = 0;
	sprintf(putfile, ".%s.3|%d|",splitfile, filesize);
	printf("Splitfile = %s\n",putfile);
	send(putsockets[1],putfile,30,0);
	send(putsockets[2],putfile,30,0);
	while(bytesread < filesize){
		read_size = read(fd, buffer, 1);
		bytesread += read_size;
		send(putsockets[1],buffer, read_size, 0);
		send(putsockets[2],buffer, read_size, 0);
		bzero(buffer, 50000);
	}
	sprintf(putfile, ".%s.4|%d|",splitfile, filesize + remainder);
	printf("remainder = %d\n",remainder);
	printf("Splitfile = %s\n", putfile);
	send(putsockets[2],putfile,30,0);
	send(putsockets[3],putfile,30,0);
	bytesread = 0;
	while(1){
		read_size = read(fd, buffer, 1);
		if(bytesread == filesize + remainder){
			break;
		}
		bytesread += read_size;
		send(putsockets[2],buffer, read_size, 0);
		send(putsockets[3],buffer, read_size, 0);
		bzero(buffer, 50000);
	}
	printf("Sent %d\n",bytesread);

bzero(putfile, 500);
bzero(buffer, 50000);
bzero(splitfile, 500);
return;
}

void parse(char cmd[]){
	char shutdown[500];
	int i;

	if (strncmp("get ",cmd, 4) == 0){
		get(cmd);
	}
	else if (strncmp("put ",cmd, 4) == 0){
		put(cmd);
	}
	else if (strncmp("list", cmd, 4) == 0){
		list(cmd);
		printf("%s\n",lsoutput);
		bzero(lsoutput, 10000);
	}
	else if (strncmp("exit", cmd, 4) == 0){
		sprintf(shutdown, "shutdown");
		for(i = 0; i < 4; i++){
			send(socketints[i], shutdown, 100,0);
		}
		for(i = 0; i < 4; i++){
			close(socketints[i]);
		}
		exit(1);
	}
	else{
		printf("Incorrect Command format...\n");
		bzero(cmd, 500);
		return;
	}
	return;
}
int md5file(char filename[]){
	int fd, read_size, bytesread, i, v1, v2, v3, v4, hash;
	char hashdata[50000], mdString[33];
	unsigned char digest[16];


	//Opens the file and reads it into a buffer to use libraries to calculate the md5 of the file contents
	//Then we assign the values to variables and xor them together to get the int, and mod it for the number desired. 
	fd = open(filename, O_RDONLY);
	while((read_size = read(fd, hashdata, 1000)) != 0){
		bytesread += read_size;
	}
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, hashdata, strlen(hashdata));
	MD5_Final(digest, &ctx);
	for(i = 0; i < 16; i++){
		sprintf(&mdString[i * 2], "%02x", (unsigned int)digest[i]);
	}
	/*sscanf(&mdString[0], "%x", &v1);
	sscanf(&mdString[8], "%x", &v2);
	sscanf(&mdString[16], "%x", &v3);
	sscanf(&mdString[24], "%x", &v4);
	hash = v1 ^ v2 ^ v3 ^ v4;
	//hash = hash%4;*/
	unsigned long long a = *(unsigned long long*)mdString;
	unsigned long long b = *(unsigned long long*)(mdString + 8);
	printf("MD5 = %d\n",(a ^ b)%4);
	close(fd);


return (a ^ b)%4;
}
void inthandler(int sig){
	int i;

	printf("\n\nKILLING EVERYTHIN\n");
	for(i = 0; i < 4; i++){
		close(socketints[i]);
	}
	kill(0, SIGKILL);
}