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

void list(char *);
void get(char *);
void put(char *);
int verify(char *);
void parse(char *);
void inthandler(int);

int socket_desc, client_sock, c;
char userdir[25], user[25], password[25];
struct sockaddr_in server, client;
struct stat st = {0};
pid_t pid;



int main(int argc, char *argv[]){
	char client_message[500];
	int verified, read_size, enable = 1, port;
	signal(SIGINT, inthandler);
	struct dirent *dir;
	DIR *d;

	if(argc < 1){
		printf("\nMust specify a port number\n");
		exit(1);
	}
	port = atoi(argv[1]);
	socket_desc = socket(AF_INET , SOCK_STREAM, 0);
	if (socket_desc == -1){
        printf("Could not create socket");
    }
    puts("Socket created");
    if (setsockopt(socket_desc, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
    	perror("setsockopt(SO_REUSEADDR) failed");
	}
	server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( port );

    if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0){
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    pid = getpid();
    puts("bind done");
    listen(socket_desc , 3);

    while(1){

    	c = sizeof(struct sockaddr_in);

    	LABEL:
    	if (pid != 0){
    		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
    	}
    	if (client_sock < 0){
        	perror("accept failed");
    	}
    	if(pid != 0){
    		if(pid = fork() != 0){
    			goto LABEL;
    		}
    	}
    	LABEL3:
    	if((read_size = recv(client_sock , client_message , 50 , 0)) > 0 ){
    		printf("%s\n",client_message);
    		verified = verify(client_message);
    		if(verified == 1){
				strcat(user, "/");
				if (stat(user, &st) == -1){
					mkdir(user, 0700);
				}
    			//d = opendir(user);
    			//if(d == NULL){
    				//printf("Unable to open directory\n");
    			//}
    			LABEL2:
    			printf("waiting for command...\n\n");
    			bzero(client_message, 500);
    			read_size = 0;
    			if((read_size = recv(client_sock , client_message , 100 , 0)) > 0 ){
    				printf("Client command = %s\n",client_message);
    				if(strncmp(client_message, "get", 3) == 0){
    					get(client_message);
    				}
    				else if(strncmp(client_message, "put", 3) == 0){
    					put(client_message);
    				}
    				else if(strncmp(client_message, "list", 4) == 0){
    					printf("calling list\n");
    					list(client_message);
					}
					else if(strncmp(client_message, "shutdown", 8) == 0){
						printf("called shutdown\n");
						close(client_sock);
						kill(0, SIGKILL);
						exit(1);
					}
					else{
						printf("Command not found\n");
						goto LABEL2;
					}
					printf("go to label2\n");
					goto LABEL2;
				}
				else{
					close(client_sock);
					close(socket_desc);
					return 0;
				}
    		}
    		else{
    			goto LABEL3;
    		}
    	}
    }
}



void get(char client_message[]){
	int fd = 0, read_size, filesize, status, i, bytesread;
	char buffer[50000], fileslist[500], fileslist2[500], firstfile[500], *tok, *filecheck, *files1, *file1, *files2, *filesizec, filebuf[500],filebuf2[500], userstr[25];
	struct dirent *dir;
	struct stat fileStat;
	DIR *d;


	
	strcpy(filebuf, client_message);
	strcpy(userstr, user);
	printf("%s\n",filebuf);
	filecheck = strtok(filebuf, "\n");
	filecheck = strrchr(filecheck, ' ');
	filecheck = strpbrk(filecheck, filecheck + 1);
	printf("Opening %s directory\n",user);
	d = opendir(user);
	if(d  == NULL){
    	perror("Cannot Open /cache/\n");
    }
	while((dir = readdir(d)) != NULL){
		strcat(fileslist, dir->d_name);
		strcat(fileslist, "|");
	}
	strcpy(filebuf2, filecheck);
	strcpy(fileslist2, fileslist);
	tok = strtok(fileslist2, "|");
	while(tok != NULL){
		tok = strtok(NULL, "|");
		for(i = 1; i < 4; i++){
			bzero(filebuf, 500);
			sprintf(filebuf, ".%s.%d", filebuf2,i);
			if(strcmp(tok, filebuf)==0){
				tok = NULL;
				strcpy(firstfile, filebuf);
				printf("File Found!\n");
				break;
			}
		}
	}
	
	strcat(userstr, filebuf);
	printf("opening %s file\n", userstr);
	fd = open(userstr, O_RDONLY);
	if(fd > 0){
		status = stat(userstr, &fileStat);
		filesize = fileStat.st_size;
		sprintf(filebuf, "%s|%d|",userstr, filesize);
		printf("Sending to client = %s\n",filebuf);
		send(client_sock, filebuf, 50, 0);
		bytesread = 0;
		while(1){
			read_size = read(fd, buffer, 1);
			send(client_sock, buffer, read_size,0);
			bytesread += read_size;
			bzero(buffer, 50000);
			if (bytesread == filesize){
				break;
			}
		}
		printf("Bytes sent = %d\n", bytesread);
		close(fd);
	}
	else{
		printf("Unable to locate %s\n",userstr);
		sprintf(buffer, "Unable to locate file");
		send(client_sock, buffer, 50,0);
	}
	strcpy(fileslist2, fileslist);
	tok = strtok(fileslist2, "|");
	while(tok != NULL){
		tok = strtok(NULL, "|");
		for(i = 1; i < 5; i++){
			bzero(filebuf, 500);
			sprintf(filebuf, ".%s.%d", filebuf2,i);
			if(strcmp(filebuf, tok)==0){
				printf("Comparing the last file opened = %s and next file %s\n",firstfile, tok);
				if(strcmp(firstfile, tok)!= 0){
					tok = NULL;
					printf("Non-Duplicate File Found!\n");
					break;
				}
				else{
					continue;
				}
			}
		}
	}
	strcpy(userstr, user);
	strcat(userstr, filebuf);
	printf("Opening %s\n",userstr);
	fd = open(userstr, O_RDONLY);
	if(fd > 0){
		status = stat(userstr, &fileStat);
		filesize = fileStat.st_size;
		sprintf(filebuf, "%s|%d|",userstr,filesize);
		printf("sending to client = %s\n", filebuf);
		send(client_sock, filebuf, 50,0);
		bytesread = 0;
		while(1){
			read_size = read(fd, buffer, 1);
			send(client_sock, buffer, read_size,0);
			bytesread += read_size;
			bzero(buffer, 50000);
			if(bytesread == filesize){
				break;
			}
		}
		bzero(buffer, 50000);
		close(fd);
	}
	bzero(fileslist, 500);
	bzero(userstr, 25);
	bzero(filebuf, 500);
	bzero(filebuf2, 500);

	return;
}


void put(char client_message[]){
	int fd = 0, read_size, verified = 0, filesize, bytesread;
	char buffer[50000], filebuf[500], *filesizec, userstr[25], *file1, *file2, *filesend;



	bzero(client_message, 500);
	strcpy(userstr, user);
	if((read_size = recv(client_sock , client_message , 30 , 0)) > 0){
		strcpy(filebuf, client_message);
		file1 = strtok(filebuf, "|");
		filesizec = strtok(NULL, "|" );
		filesize = atoi(filesizec);
		strcat(userstr, file1);
		printf("File 1 = %s\n", userstr);
		printf("Filesize = %d\n",filesize);
		if((fd = open(userstr, O_CREAT | O_RDWR, 0600)) < 0){
			printf("Unable to create file\n");
		}
		file1 = strrchr(client_message, '|');
		bytesread = 0;
		while(1){
			read_size = read(client_sock, buffer, 1);
			write(fd, buffer, read_size);
			bytesread += read_size;
			if(bytesread == filesize){
				break;
			}
		}
		printf("bytes read = %d\n",bytesread);
		
		close(fd);
		bzero(buffer, 50000);
		bzero(client_message,500);
		if((read_size = recv(client_sock, client_message , 30, 0)) > 0){
			strcpy(filebuf, client_message);
			file2 = strtok(filebuf, "|");
			filesizec = strtok(NULL, "|");
			filesize = atoi(filesizec);
			filesend = strtok(userstr, ".");
			strcat(filesend, file2);
			printf("File 2 = %s\n", filesend);
			printf("File 2 size = %d\n",filesize);
			if((fd = open(filesend, O_CREAT | O_RDWR, 0600)) < 0){
				printf("Unable to create file\n");
			}
			file2 = strrchr(client_message, '|');
			bytesread = 0;
			while(1){
				//read_size = recv(client_sock, buffer, 1, 0);
				read_size = read(client_sock, buffer, 1);
				write(fd, buffer, read_size);
				bytesread += read_size;
				if(read_size == 0){
					break;
				}
				if(bytesread == filesize){
					break;
				}
				}
				printf("Bytes read = %d\n",bytesread);
			//}
		}
		printf("Leaving Put\n");
		close(fd);
		bzero(buffer, 50000);
		bzero(client_message, 500);
	}
return;
}




void list(char client_message[]){
	DIR *d;
	struct dirent *dir;
	char filebuf[2000];


	printf("called list\n");
	
	d = opendir(user);
	if(d == NULL){
		printf("Cannot open directory\n");
	}
	while((dir = readdir(d)) != NULL){
		strcat(filebuf, dir->d_name);
		strcat(filebuf, "|");
	}
	printf("Sending %s\n",filebuf);
	send(client_sock, filebuf, 2000,0);
	closedir(d);
	return;
}




int verify(char client_message[]){
	FILE* fd = 0;
	char *password, filecontent[100], *userptr, *passwordptr;

	if((fd = fopen("dfs.conf","r")) == NULL){
		printf("config file did not open\n");
	}
	fread(filecontent, 100, 1, fd);
	fclose(fd);
	userptr = strstr(client_message, "Username: ");
	userptr = strtok(userptr, "\n");
	passwordptr = strstr(client_message, "Password: ");
	passwordptr = strtok(passwordptr, "\n");
	memcpy(userptr, userptr + 10, strlen(userptr));
	strcpy(user, userptr);
	strcpy(password, passwordptr);
	memcpy(passwordptr, passwordptr + 10, strlen(passwordptr));
	if ((strstr(filecontent, userptr) != NULL)){
		if((strstr(filecontent, passwordptr)!=NULL)){
			printf("User: %s logged in successfully\n",userptr);
			sprintf(filecontent, "User: %s logged in successfully\n",userptr);
			send(client_sock, filecontent, 500,0);
			return 1;
		}
		else{
			printf("Incorrect Password\n");
			sprintf(client_message, "Incorrect Password\n");
			send(client_sock, client_message, 500,0);
			return 0;
		}
	}
	else{
		printf("User was not unable to be authenticated\n");
		sprintf(client_message, "User was not unable to be authenticated\n");
		send(client_sock, client_message, 500,0);
		return 0;
	}

}

void parse(char client_message[]){
	char filebuf[100], *file1, *file2;

	memcpy(filebuf, client_message+4, strlen(client_message));
	file2 = strtok(filebuf, "\n");
	file2 = strrchr(file2, ' ');
	file1 = strtok(filebuf, " ");
	
	return;
}

void inthandler(int sig){
	int i;

	printf("\n\nKILLING EVERYTHIN\n");
	close(client_sock);
	kill(0, SIGKILL);
}