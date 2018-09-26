/*
 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */

#include <string.h>
#include <sys/types.h>
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
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>

#define BUFSIZE 1024
void get(char buf[], int sock, struct sockaddr_in serveraddr);
void delete(char []);
void put(char buf [], int sock, struct sockaddr_in);
void ls(char buffer []);
void error(char *);

int sockfd, portno, n;
int serverlen;
struct sockaddr_in serveraddr;
struct hostent *server;
struct stat fileStat;
char *hostname;
char buf[BUFSIZE];
char server_in[BUFSIZE];
char * bufwords;


int main(int argc, char **argv) {
	int quit = 0;

	/* check command line arguments */
	if (argc != 3) {
		fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	hostname = argv[1];
        portno = atoi(argv[2]);

        /* socket: create the socket */
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) 
        	error("ERROR opening socket");

        /* gethostbyname: get the server's DNS entry */
        server = gethostbyname(hostname);
        if (server == NULL) {
        	fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        	exit(0);
    	}

    	/* build the server's Internet address */
    	bzero((char *) &serveraddr, sizeof(serveraddr));
    	serveraddr.sin_family = AF_INET;
    	bcopy((char *)server->h_addr, 
	(char *)&serveraddr.sin_addr.s_addr, server->h_length);
    	serveraddr.sin_port = htons(portno);
    
    	while(quit == 0 ){
		/* get a message from the user */
    		bzero(buf, BUFSIZE);
    		printf("\nEnter one of the following for server access:\nget [FILENAME]\nput [FILENAME]\ndelete [FILENAME]\nls\nexit\nPlease enter msg: ");
    		fgets(buf, BUFSIZE, stdin);
		//Uses strtok to eliminate any newline characters from further processing of buf.
		strtok(buf, "\n");
		/* send the message to the server */
		serverlen = sizeof(serveraddr);
		n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
        	if (n < 0){
			error("Warning: problem in sendto");
		}
		//Takes argument to launch appropriate function.
		
		if (strncmp( "get ", buf, 4 ) == 0){
			get(buf, sockfd, serveraddr);
		}
		else if (strncmp( "put ",buf, 4 ) == 0){
			put(buf, sockfd, serveraddr);
		}
		else if (strncmp( "delete ", buf, 7 ) == 0){
			delete(buf);
		}
		else if (strncmp( "ls", buf, 2 ) == 0){
			ls(buf);
		}
		else if (strncmp( "exit", buf, 4) == 0){
			quit = 1;
		}
		//If input is not in proper format default to error message and reprompt for correct usage.
		else{
			printf("Invalid format, please make sure to use lower case letters");
		}
	}
	close(sockfd);
	return EXIT_SUCCESS;
}
void get(char buf[], int sockfd, struct sockaddr_in serveraddr){
	int eof;
	char filebuf[BUFSIZE];
	FILE *fp;
	
	//Use a second buffer to file opening. 
	memcpy( filebuf, buf + 4, strlen(buf)+1);
	if(strcmp( filebuf, "")!=0){
		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
		if(n < 0){
			error("Receipt from server failed");
		}
		//File is opened in binary mode to account for jpg files. If file is not found on server side
		//then it passes this if statment to a perror message for the user.    
		if(strcmp(buf, "File does not exist") != 0){
			fp = fopen( filebuf, "w+b");
			if (fp == NULL ){
				error("file was null line 124");
			}
			eof = 0;
			//While loop continually checks for server input end of file indicator, else it 
			//continues the writing to file.   
			while (eof != 1){
				if ( strcmp(buf, "ZXCVBnmAS") == 0){
					eof = 1;
					fclose (fp);
				}
				else { 
					//fputs(buf, fp);//ORIGINAL
					fwrite(buf,sizeof(buf),1,fp);
					//error(buf);
					bzero(buf, BUFSIZE);
					//error("after bzero");
					n = recvfrom(sockfd,buf,sizeof(buf)+1, 0, &serveraddr, &serverlen);
					if (n < 0){
						error("Error receiving from server2");
					}
				}
			}
		}
		else{
			perror("File does not exist");
		}
	}
return;
}
void put(char buf[], int sockfd, struct sockaddr_in serveraddr){
	char filebuf[BUFSIZE];
	char filetype;
	FILE *fp;
	//Using the same method for opening of a file in binary read mode.  
	memcpy( filebuf, buf + 4, strlen( buf ) +1);
	if ( strcmp ( filebuf, "") != 0 ) {
		fp = fopen( filebuf, "r+b");
		if (fp == NULL){
			error("File not found");
			fclose(fp);
		}
		else{	
			//zeros the buffer to get reading for inputing file contents into buffer. While loop
			//checks for fread to give output of zero indicating end of file.  
			bzero(buf, BUFSIZE);
			while(fread(buf, sizeof(buf), 1, fp) != 0){
				perror(buf);
				n = sendto(sockfd,buf, sizeof(buf)+1, 0, &serveraddr, serverlen);
				if ( n < 0 ){
					printf("Error sending file");
				}
				bzero(buf,BUFSIZE);
			}
			//When file is done we send end of file indicator to be matched on server side.   
			strcpy(buf, "QWERTyui");
			n = sendto(sockfd,buf, strlen(buf), 0, &serveraddr, serverlen);
			if (n < 0){
				printf("Error sending file 2");
			}
		}fclose(fp);
	}
}
void delete(char buf[]){
	
	//All we have to do is zero and receive if the delete was executed or not.  
	bzero(buf, sizeof(buf));
	n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
	if ( n < 0 ){
		printf("Error recieving from server");
	}
	perror(buf);
	return;
}
void ls(char buf[]){
	//Most of the work is done on the server side, we just have to read back the output to the user.  While
	//loop uses another end of file indicator. 
	n = recvfrom(sockfd, buf, BUFSIZE, 0, &serveraddr, &serverlen);
	if ( n < 0 ){
		printf("Error recieving from server");
	}
	int eof=0;
	while (eof != 1){
		if ( strcmp(buf, "QWERTyuiopASDFG") == 0){
			eof = 1;
		}
		else { 
			fputs(buf, stdout);
			bzero(buf, BUFSIZE);
			n = recvfrom(sockfd, buf, BUFSIZE-1, 0, &serveraddr, &serverlen);
					
			if (n < 0){
				error("Error receiving from server2");
			}
		}
	}
return;
}

void error(char *msg) {
	perror(msg);
	exit(1);
}
			
    
    		/* print the server's reply */
    		/*n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    		if (n < 0) 
      			error("ERROR in recvfrom");
    			printf("Echo from server: %s", buf);
    			return 0;*/

