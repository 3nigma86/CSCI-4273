/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

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
#include <unistd.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */

void get(char buf[], int sock, struct sockaddr_in serveraddr);
void delete(char buf[]);
void put(char buf[], int sock, struct sockaddr_in serveraddr);
void ls(char buf []);
void error(char *);

int sockfd; /* socket */
int portno; /* port to listen on */
int clientlen; /* byte size of client's address */
struct sockaddr_in serveraddr; /* server's addr */
struct sockaddr_in clientaddr; /* client addr */
struct hostent *hostp; /* client host info */
char buf[BUFSIZE]; /* message buf */
char *hostaddrp; /* dotted decimal host addr string */
int optval; /* flag value for setsockopt */
int n; /* message byte size */ 


int main(int argc, char **argv) {
	int quit = 0;

  	/* 
   	* check command line arguments 
   	*/
	if (argc != 2) {
    		fprintf(stderr, "usage: %s <port>\n", argv[0]);
    		exit(1);
  	}
  	portno = atoi(argv[1]);

  	/* 
   	* socket: create the parent socket 
   	*/
  	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  	if (sockfd < 0){
    		error("ERROR opening socket");
	}

  	/* setsockopt: Handy debugging trick that lets 
   	* us rerun the server immediately after we kill it; 
   	* otherwise we have to wait about 20 secs. 
   	* Eliminates "ERROR on binding: Address already in use" error. 
   	*/
  	optval = 1;
  	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  	/*
   	* build the server's Internet address
   	*/
  	bzero((char *) &serveraddr, sizeof(serveraddr));
  	serveraddr.sin_family = AF_INET;
  	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  	serveraddr.sin_port = htons((unsigned short)portno);

  	/* 
   	* bind: associate the parent socket with a port 
   	*/
  	if (bind(sockfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr)) < 0){ 
    		error("ERROR on binding");
	}
  	/* 
   	* main loop: wait for a datagram, then echo it
   	*/
  	clientlen = sizeof(clientaddr);
  	while (quit == 0) {

    		/*
     		* recvfrom: receive a UDP datagram from a client
     		*/
    		bzero(buf, BUFSIZE);
    		n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);
    		if (n < 0){
      			error("ERROR in recvfrom");
		}

    		/* 
     		* gethostbyaddr: determine who sent the datagram
     		*/
    		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    		if (hostp == NULL){
      			error("ERROR on gethostbyaddr");
		}
    		hostaddrp = inet_ntoa(clientaddr.sin_addr);
    		if (hostaddrp == NULL){
      			error("ERROR on inet_ntoa\n");
		}
    		printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
    		printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    
    		//Same basic implementation as in client, uses strncmp to make sure the correct function to sync with 
		//client.  If the command is not right, both client and server can restart the loop together. 
		if (strncmp("get ", buf, 4) == 0){
			get(buf,sockfd,serveraddr);
		}
		else if (strncmp("put ", buf, 4) ==0){
			put(buf, sockfd,serveraddr);
		}
		else if (strncmp("delete ",buf,7)== 0){
			delete(buf);
		}
		else if (strncmp("ls", buf,2) == 0){
			ls(buf);
		}
		else if (strncmp("exit",buf,4) == 0){
			quit = 1;
		}
		else{
			perror("Invalid Format");
		}

	}close(sockfd);
	return EXIT_SUCCESS;
}
void get(char buf[], int sockfd, struct sockaddr_in serveraddr){
	int n;
	char filebuf[BUFSIZE];
	FILE *fp;
	//Use memcpy to use filebuf to open up the indicated file for transfer, open up file in binary. 
	memcpy(filebuf, buf+4, strlen(buf)+1);
	if (strcmp (buf, "") != 0){
		fp = fopen(filebuf, "r+b");
		if (fp == NULL){
			bzero (buf, BUFSIZE);
			strcpy( buf, "File does not exist");
			n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
			if (n < 0){
				error("Failure to send");
			}
			
		}
		else{   //bzero the buffer and read from correct file to input into buf for the socket transfer. 
			//when fread indicates the file is done we send end of file indicator over socket.  
			bzero (buf, BUFSIZE);
			while(fread(buf,sizeof(buf),1, fp) != 0){
				n = sendto(sockfd, buf, sizeof(buf)+1, 0, (struct sockaddr *) &clientaddr, clientlen);
				bzero(buf, BUFSIZE);
				if (n < 0){
					error("not sending to client line 152");
				}
			}
			strcpy( buf, "ZXCVBnmAS");
			//error(buf);
			n = sendto(sockfd, buf, BUFSIZE, 0, (struct sockaddr *) &clientaddr, clientlen);
		}
		
	}
return;
}
void put(char buf[], int sockfd, struct sockaddr_in serveraddr){
	char filebuf[BUFSIZE];
	int eof;
	FILE *fp;

	memcpy(filebuf, buf+4, strlen(buf)+1);
	if (strcmp (buf, "") != 0){
		fp = fopen(filebuf, "w+b");
		bzero(buf,BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);
		if(n<0){
			error("Error at line 192");
		}
		while (eof != 1){
			if (strcmp(buf, "QWERTyui") ==0){
				eof = 1;
				fclose(fp);
			}
			else{
				perror(buf);
				fwrite(buf,sizeof(buf),1,fp);
				bzero(buf, BUFSIZE);
				n = recvfrom(sockfd, buf, sizeof(buf)+1, 0,(struct sockaddr *) &clientaddr, &clientlen);
				if (n<0){
					error("Error receiving file");
				}
			}
		}
	}
return;			
}
void delete(char buf[]){
	char filebuf[BUFSIZE];
	FILE *fp;
	//Read in the buf and use filebuf again to find and use remove to delete the correct file.  If the 
	//remove function is successful then send confirmation to client, if it is not then send failure message.
	memcpy(filebuf, buf+7, strlen(buf)+1);
	if (strcmp (buf, "") != 0){
		if ( remove(filebuf)==0){
			strcpy(buf, "File Deleted Successfully");
			n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
			if (n<0){
				error("Error receiving file");
			}
		}
		else {
			strcpy(buf, "Unable to delete file");
			n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
			if (n<0){
				error("Error receiving file");
			}
		}
	}
return;
}
void ls(char buf[]){
	FILE *fp;
	//Use popen to input the "ls" linux command to get the ls into a temporary file (as far as I understand)
	//and then read out of that temp file to use for the communication to client.   
	fp = popen(buf, "r");
	bzero(buf, BUFSIZE);
	if (fp != NULL){
		while (fgets(buf, BUFSIZE, fp) != NULL){
			n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
			if (n<0){
					error("Error receiving file");
			}
			bzero(buf,BUFSIZE);
		}
		strcpy(buf, "QWERTyuiopASDFG");
		n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
			if (n<0){
				error("Error receiving file");
			}
		pclose(fp);
	}
	else{
		perror("Invalid input format");
		pclose(fp);
	}
return;
}
void error(char *msg){
	perror(msg);
	exit(1);
}
