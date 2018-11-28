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
#include <dirent.h>

void get(char *);
void error();
void inthandler(int);
void post(char *);
void grab(char *);

int socket_desc , client_sock , c , read_size, cachetime;
struct sockaddr_in server , client;
char client_message[4000];
pid_t pid;


int main(int argc , char *argv[]){
	struct stat fileStat;
	struct tm* time1;
	struct timeval timeout;
	struct dirent *dir;
	signal(SIGINT, inthandler);
	int enable = 1, fileagei, nowi, fp, port, cnt;
	char *fileagec, *nowc, *token, something[200], something1[200], hostname[4000], tokenbuf[200];
	timeout.tv_sec = 7;
	timeout.tv_usec = 0;
	FILE* fd;
	DIR *d;
	time_t today;
	

	if(argc < 2){
		printf("\nMust specify a port number\n");
		error();
	}
	//Use command line arguments for port number and cachetimeout
	port = atoi(argv[1]);
	cachetime = atoi(argv[2]);
     
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
    server.sin_port = htons( port ); //port is now specified by the client on the command line.
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
    		//This is where I'm trying to do the cache time out but it doesn't work yet. 
    		d = opendir("cache/");
    		if(d  == NULL){
    			perror("Cannot Open /cache/\n");
    		}
    		//The main process handles checking the ctime stamp of all cached files
    		//and deleting files that are over the time alloted in the command line.
    		while((dir = readdir(d)) != NULL){
    		 	strcpy(something, "cache/");
    		 	strcat(something, dir->d_name);
    		 	//printf("checking file = %s\n",something);
    			if(lstat(something, &fileStat) < 0){
    				perror("lstat");
    			}
    			time(&today);
    			time1 = localtime (&today);
    			nowc = asctime(time1);
    			strncpy(something1, nowc + 14, 2);
    			nowi = atoi(something1) * 60;
    			strncpy(something1, nowc + 17, 2);
    			nowi = nowi + atoi(something1);
    			fileagec = ctime(&fileStat.st_ctime);
    			strncpy(something1, fileagec + 14, 2);
    			fileagei = atoi(something1) * 60;
    			strncpy(something1, fileagec + 17, 2);
    			fileagei = fileagei + atoi(something1);
    			if((nowi - fileagei) >= (cachetime)){
    				unlink(something);
    				//printf("File age is %d seconds\n", nowi-fileagei);
    			}
    		}
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
    	//Checks blacklist first and sends 403 if found, then makes sure http header is 
    	//a GET request, sends 400 bad request if not.  
    	bzero(client_message, 4000);
    	if((read_size = recv(client_sock , client_message , 1000 , 0)) > 0 ){
    			strncpy(hostname, client_message, strlen(client_message));
    			printf("\n%s\n\n", client_message);
    			token = strstr(hostname, "Host:");
    			memcpy(hostname, token + 6, strlen(client_message));
    			token = strtok(hostname, "\r");
    			strcpy(tokenbuf, token);
    			fd = fopen("blacklist.txt", "r");
    			bzero(hostname,4000);
    			fread(hostname, 1, 4000, fd);
    			fclose(fd);
    			if(strstr(hostname, tokenbuf) == NULL){
    				if(strstr(client_message, "GET") == NULL){
    					bzero(hostname, 4000);
    					sprintf(hostname,"HTTP/1.1 400 Bad Request\r\n");
    					printf("INCORRECT HTTP FORMAT\n");
    					send(client_sock, hostname, strlen(hostname),0);
    					//close(client_sock);
    				}
    				else{
    					printf("Valid HTTP Header\n");
    					get(client_message);
    				}
				}
				else{
					bzero(hostname, 4000);
					time1 = localtime(&today);
    				sprintf(hostname, "HTTP/1.1 403 Forbidden\r\nDate: %sConnection: close\r\n\r\n<html>\n<head>\n<title>ERROR 403 - FORBIDDEN</title>\n<head>\n<body>\n<h1>ERROR 403 -FORBIDDEN</h1>\n<p>\nThe website you are accessing is forbidden</p>\n</body>\n</html>", asctime(time1));
    				printf("FORBIDDEN HOST\n Error message = %s\n", hostname);
    				send(client_sock, hostname, strlen(hostname),0);
    				bzero(hostname, 4000);
    				//close(client_sock);
    				close(fp);
    			}
    			bzero(hostname, 4000);
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
	char *token,*filetype,filebuf[4000],messageout[4000],lookup[200],cache[200];
	int fd = 0,n, size, cnts, filehash = 0;
	


    memcpy(filebuf, client_message + 4, strlen(client_message));
    token = strtok(filebuf, " ");
    
    //hash the function out to an integer
    for (int i = 0; token[i] != '\0';i++){
    	filehash = 31*filehash + token[i];
    }


    //pass hashed number to lookup and concat the cache/ file path onto it.
    sprintf(lookup, "%d",filehash);
    strcpy(cache, "cache/");
    strcat(cache, lookup);
    fd = open(cache, O_RDWR);
    printf("LOOKING FOR %s IN CACHE\n",cache);
   	if (fd > 0){
		printf("FILE FOUND\n");
   		while (1){
   			cnts=read(fd, messageout, 4000);
   			if(cnts <= 0){
   				break;
   			}
    		write(client_sock, messageout , cnts);
    	}
    	close(fd);
    	//this is left over from the last assignment and I probably don't need it anymore
    	if((strstr(client_message, "Connection: close") != NULL)){
    		printf("Killing Process oops\n");
    		bzero(filebuf, 4000);
    		bzero(client_message, 4000);
    		bzero(messageout, 4000);
    		pid = getpid();
    		kill(pid, SIGKILL);
		}
    }
    //If the file didn't exist we'll just go to the server through
    //the grab function
    else{
    	printf("%s\n", strerror(errno));
    	grab(client_message);
    }
    return;
} 
void grab(char client_message[]){
	char *filetype, *header, *token, *token2, cache[100],filebuf[50000],file[500],filename[2000],lookup[200],ip_char[256];
	FILE* fd = 0;
	int result, fp, filehash = 0, enable=1,reads, socket_desc1 , c1 , read_size1, client_sock1;
	struct sockaddr_in client1;
	struct addrinfo hints;
	struct addrinfo *infoptr, *p;
	struct tm* time1;
	time_t today;
	
	//First memcpy I'm pretty sure is useless.  
	memcpy(filename, client_message + 5, strlen(client_message));
	//find the Host: part of header
    token = strstr(filename, "Host:");
    //takes out the "Host: " part.
    memcpy(filename, filename + 6, strlen(client_message));
    //cuts string off at the next \n
    token = strtok(token, "\n");
    //Maybe this gets rid of a character return? It's needed for 
    //dns lookup to work. 
    //Check for cached IP's for host name and use IP if found to create connection.
    memcpy(lookup, token, strlen(token)-1);
    bzero(filename, 2000);
    fd = fopen("lookup.txt", "r");
    reads = fread(filename, 1, 2000, fd);
    fclose(fd);
    if((token = strstr(filename, lookup)) != NULL){
    	token = strtok(token, "\n");
    	token = strrchr(token, ',');
    	token = strpbrk(token, token+1);
    	printf("USING CACHED IP = %s\n",token);
    	socket_desc1 = socket(AF_INET, SOCK_STREAM, 0);
    	setsockopt(socket_desc1, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    	printf("%d\n",socket_desc1);
    	if(socket_desc1 == -1){
    		printf("Could not create socket\n");
    	}
    	puts("Socket created");
    	client1.sin_family = AF_INET;
    	client1.sin_addr.s_addr = INADDR_ANY;
    	client1.sin_port = htons( 80 );
    	client1.sin_addr.s_addr = inet_addr(token);
    	
    	
    	if((reads = connect(socket_desc1, (struct sockaddr *)&client1, sizeof(client1))) == -1){
    		printf("Connect failed\n");
    	}
    	bzero(filename, 2000);
    	printf("CONNECTED!!!!! reads = %d\n", reads);
    }
    


    else{
		printf("NEITHER IP OR FILE CACHED\n");
    	memset(&hints, 0, sizeof(struct addrinfo));
    	hints.ai_family = AF_INET;
    	hints.ai_socktype = SOCK_STREAM;
    	result = getaddrinfo(lookup, "80", &hints, &infoptr);
    	

    	if (result != 0){
    		printf("%s\n",gai_strerror(result));
    	}
    	

    	//iterate through possible IP's and see if one will connect
    	for (p = infoptr; p != NULL; p = p->ai_next){
    		getnameinfo(p->ai_addr, p->ai_addrlen, ip_char, sizeof(ip_char), NULL,0,NI_NUMERICHOST);
    		socket_desc1 = socket(AF_INET , SOCK_STREAM, 0);
    		if (socket_desc1 == -1){
        		printf("Could not create socket");
    		}
    		puts("Socket created");
    	

    		
    		if (connect(socket_desc1, p->ai_addr, p->ai_addrlen) !=-1){
    			//if the connect works then cache the host and IP, also check
    			//the blacklist for the IP address and send 403 if it's blacklisted.
    			fd = fopen("blacklist.txt", "r");
    			reads = fread(filename, 1, 2000, fd);
    			fclose(fd);
    			if((strstr(filename, ip_char))!= NULL){
    				time1 = localtime(&today);
    				sprintf(filename, "HTTP/1.1 403 Forbidden\r\nDate: %sConnection: close\r\n\r\n<html>\n<head>\n<title>ERROR 403 - FORBIDDEN</title>\n<head>\n<body>\n<h1>ERROR 403 -FORBIDDEN</h1>\n<p>\nThe website you are accessing is forbidden</p>\n</body>\n</html>", asctime(time1));
    				send(client_sock, filename, strlen(filename),0);
    			}
    			bzero(filename, 2000);
    			fd = fopen("lookup.txt", "r");
    			bzero(filename, 2000);
    			reads = fread(filename, 1, 2000, fd);
    			fclose(fd);
    			if((token = strstr(filename, lookup)) == NULL){
    				fd = fopen("lookup.txt", "a");
					fprintf(fd,"%s,%s\n",lookup, ip_char);
				}
    				fclose(fd);
    				bzero(filename, 2000);
    			break;
    		}
    	

    		else{
    			puts("Connect failed\n");
    		}
    	}
    }
	
	//printf("Client message = %s\n",client_message);

    //Send the http header request we got from bowser.  
    if((reads = send(socket_desc1, client_message, strlen(client_message),0)) < 0){
    	puts("Send failed\n");
    }
    //This is parsing out the url. 
    memcpy(file, client_message + 4, strlen(client_message));
    token2 = strtok(file," ");
    //This is hashing it with the same function as above.
    for (int i = 0; token2[i] != '\0';i++){
    	filehash = 31*filehash + token2[i];
    }
  
    //Transferring it into lookup buffer and concat /cache/ filepath.
    sprintf(lookup, "%d",filehash);
    strcpy(cache, "cache/");
    strcat(cache, lookup);
    printf("USING SOCKET = %d\nTO SEND CONTENTS FOR URL = %s\nCACHING TO = %s\n",socket_desc1, token2,cache);
    //open the file with create flag and read and write priveledges
    if((fp = open(cache, O_CREAT | O_RDWR , 0600)) > 0){
    	
    	printf("Reading and Writing...\n");
    	bzero(filebuf, 50000);
    	while((read_size1 = recv(socket_desc1 , filebuf , 50000-read_size1 , 0)) > 0){
    		c1 += read_size1;
    		//printf("%s\n",filebuf);
    		send(client_sock, filebuf, read_size1,0);
			reads = write(fp ,filebuf, read_size1);
			//write(client_sock, filebuf, read_size1);
			bzero(filebuf, 50000);
			
		}
		close(client_sock1);
		close(socket_desc1);
		printf("Closing File Having read and wrote %d bytes\n",c1);
		close(fp);
	}
	

	else{
		printf("File didn't Open for Caching\n");
	}

return;
}
//I don't even use this function.  I could pretty much just delete it. 
void error(){
	char messageout[2000];

	sprintf(messageout,"HTTP/1.1 500 Internal Server Error\n");
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

    
   
