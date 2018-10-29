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

void get(char *);
void error();
void inthandler(int);
void post(char *);

int socket_desc , client_sock , c , read_size;
struct sockaddr_in server , client;
struct timeval timeout;
char client_message[2000];
pid_t pid;


int main(int argc , char *argv[]){
	signal(SIGINT, inthandler);
	int enable = 1;
	timeout.tv_sec = 12;
	timeout.tv_usec = 0;
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    //Set options to be able to reuse address to stop errors when running program often
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
    	perror("setsockopt(SO_REUSEADDR) failed");
	}
	//Option to use a timeout of 12 seconds for both accept and recv functions
    if (setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) <0){
    	perror("setsockopt(SO_RCVTIMEO) failed");
    }
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    //PID is going to be important for future execution
    pid = getpid();
    puts("bind done");
    listen(socket_desc , 3);
    while(1){
    
    	//Accept and incoming connection
       	c = sizeof(struct sockaddr_in);
    	//Only the parent process is allowed to enter the accept function
    	LABEL:
    	if (pid != 0){
    		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
			puts("Connection accepted\n");
    	}
    	if (client_sock < 0){
        	perror("accept failed");
    	}
    	//Only the parent function is allowed to spawn a child after which it hits the LABEL and looks to accept again
    	if(pid != 0){
    		if(pid = fork() != 0){
    			goto LABEL;
    		}
    	}
    	//Child process continues through recv function and executes the proper function
    	if( (read_size = recv(client_sock , client_message , 2000 , 0)) > 0 ){
    		if(strncmp(client_message, "GET",3)==0){
    			printf("%s\n",client_message);
    			get(client_message);
    		}
    		if(strncmp(client_message, "POST",4)==0){
    			printf("%s\n",client_message);
    			post(client_message);
			}
    	}		
  		
    	//After the recv times out at 12 seconds if the read was -1 then there was nothing so we'll close out everything
    	if(read_size <= 0){
    		printf("Timing out connection\n");
    		close(client_sock);
    		close(socket_desc);
    		pid = getpid();
    		puts("Client disconnected\n");
    		kill(pid, SIGKILL);
    		
    	}

    	else if(read_size == -1){
        	perror("recv failed");
    	}

    }
//Just in case we ever pop out of the while, but we never should. 
close(client_sock);
close(socket_desc);

    	
return 0;
}
void get(char client_message[]){
	struct stat fileStat;
	char *token;
	char filebuf[2000];
	char messageout[2000];
	char *filetype;
    char *fileext[] = {".gif", ".html", ".txt", ".jpg",".png", ".js", ".css",".ico"};
	char *keep[] = {"keep-alive", "close"};
	int fd = 0;
	int n;
	int size;
	int cnts;
	//Memcpy and strtok to find the correct file path, if there is nothing then open default "index.html" else open filepath
	memcpy(filebuf, client_message + 5, strlen(client_message));
    token = strtok(filebuf,"HTTP");

 	if (strcmp(token, " ")==0){
    	fd = open("index.html", O_RDONLY);
    	filetype = ".html";
    }
    //if the filepath is used we must also look for the file extension
    else{
    	memcpy(filebuf, client_message + 5, strlen(client_message));
    	token = strtok(filebuf, " ");
    	fd = open(token, O_RDONLY);
    	filetype = strrchr(token, '.');
    	
    }
   	fstat(fd, &fileStat);
   	//if the file exists we should match file extensions for correct data type in header
   	if (fd > 0){



   		if (strcmp(filetype, fileext[1]) == 0){
   			bzero(filebuf,2000);
   			strcpy(filebuf, "text/html");
   		}
   		else if (strcmp(filetype, fileext[2])==0){
   			bzero(filebuf, 2000);
   			strcpy(filebuf, "text/plain");
   		}
   		else if (strcmp(filetype, fileext[3])==0){
   			bzero(filebuf, 2000);
   			strcpy(filebuf, "image/jpeg");
   		}
   		else if (strcmp(filetype, fileext[4])==0){
   			bzero(filebuf,2000);
   			strcpy(filebuf, "image/apng");
		}
		else if (strcmp(filetype, fileext[5])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "application/javascript");
		}
		else if (strcmp(filetype, fileext[6])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "text/css");
		}
		else if (strcmp(filetype, fileext[0])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "image/apng");
		}
		else if (strcmp(filetype, fileext[7])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "image/x-icon");
		}
   	
    	//Write out header with inputs on file size and and file type. Send header first, then start reading file and sending it as well
    	sprintf(messageout, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: %s\r\nConnection: keep-alive\r\n\r\n",fileStat.st_size,filebuf);
    	write(client_sock , messageout , strlen(messageout));
    	printf("%s\n",messageout);
    	while ((cnts=read(fd, messageout, 2000)) != 0){
    		write(client_sock, messageout , cnts);
    	}
    	close(fd);
    	//if the HTTP header specifies to close the connection, close the connection.
    	if((strstr(client_message, "Connection: close") != NULL)){
    		printf("Killing Process oops\n");
    		bzero(filebuf, 2000);
    		bzero(client_message, 2000);
    		bzero(messageout, 2000);
    		pid = getpid();
    		kill(pid, SIGKILL);
		}
    }
    //Handle non existing files with 500 server error.
    else{
    	printf("File didn't exist\n");
    	sprintf(messageout,"HTTP/1.1 500 Internal Server Error");
		write(client_sock, messageout, strlen(messageout));
    }
    return;
}
//Post does not work for some reason so I'm not going to add comments, it's pretty much exactly like get.  
void post(char client_message[]){
	struct stat fileStat;
	char *token;
	char filebuf[2000];
	char messageout[2000];
	char *filetype;
    char *fileext[] = {".gif", ".html", ".txt", ".jpg",".png", ".js", ".css",".ico"};
	char *keep[] = {"keep-alive", "close"};
	int fd = 0;
	int n;
	int size;
	int cnts;

	memcpy(filebuf, client_message + 6, strlen(client_message));
    token = strtok(filebuf,"HTTP");

 	if (strcmp(token, " ")==0){
    	fd = open("index.html", O_RDONLY);
    	filetype = ".html";
    }
    else{
    	memcpy(filebuf, client_message + 6, strlen(client_message));
    	token = strtok(filebuf, " ");
    	fd = open(token, O_RDONLY);
    	filetype = strrchr(token, '.');
    	
    }
   	fstat(fd, &fileStat);
   	if (fd > 0){



   		if (strcmp(filetype, fileext[1]) == 0){
   			bzero(filebuf,2000);
   			strcpy(filebuf, "text/html");
   		}
   		else if (strcmp(filetype, fileext[2])==0){
   			bzero(filebuf, 2000);
   			strcpy(filebuf, "text/plain");
   		}
   		else if (strcmp(filetype, fileext[3])==0){
   			bzero(filebuf, 2000);
   			strcpy(filebuf, "image/jpeg");
   		}
   		else if (strcmp(filetype, fileext[4])==0){
   			bzero(filebuf,2000);
   			strcpy(filebuf, "image/apng");
		}
		else if (strcmp(filetype, fileext[5])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "application/javascript");
		}
		else if (strcmp(filetype, fileext[6])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "text/css");
		}
		else if (strcmp(filetype, fileext[0])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "image/apng");
		}
		else if (strcmp(filetype, fileext[7])==0){
			bzero(filebuf, 2000);
			strcpy(filebuf, "image/x-icon");
		}
		sprintf(messageout, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-size: %d\r\n\r\n<html><body><pre><h1>POSTDATA</h1></pre>",filebuf,fileStat.st_size);
    	read(fd, filebuf, 2000 - strlen(messageout));
    	strcat(messageout,filebuf);
    	write(client_sock, messageout, strlen(messageout));
    	printf("%s\n",messageout);

    	while ((cnts=read(fd, messageout, 2000)) != 0){
    		printf("%s\n", messageout);
    		write(client_sock, messageout , cnts);
    	}
    	close(fd);
    	if((strstr(client_message, "Connection: close") != NULL)){
    		printf("Killing Process oops\n");
    		bzero(filebuf, 2000);
    		bzero(client_message, 2000);
    		bzero(messageout, 2000);
    		pid = getpid();
    		kill(pid, SIGKILL);
		}
    }
    else{
    	printf("File didn't exist\n");
    	sprintf(messageout,"HTTP/1.1 500 Internal Server Error");
		write(client_sock, messageout, strlen(messageout));
    }
return;
}
//Function to be able to send errors to for proper closing of sockets and terminating of process.  
void error(){
	char messageout[2000];

	sprintf(messageout,"HTTP/1.1 500 Internal Server Error");
	write(client_sock, messageout, strlen(messageout));
	bzero(messageout, 2000);
	printf("Killing process\n");
	pid = getpid();
	close(client_sock);
	close(socket_desc);
	kill(pid, SIGKILL);
}
//SigInt handler to make sure all processes get closed and sockets closed as well.   
void inthandler(int sig){
	printf("\n\nKILLING EVERYTHIN\n");
	close(client_sock);
	close(socket_desc);
	kill(0, SIGKILL);
}
    
   
