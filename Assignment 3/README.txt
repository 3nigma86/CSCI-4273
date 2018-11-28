README For webproxy.cpp

1) In order to run my program download webproxy.cpp and all associated files.  Check to make sure that
MAKEFILE, lookup.txt, and blacklist.txt and the cache folder are all present within the directory.  

2) Run makefile to compile webproxy.cpp.  

3) Command arguments are as follows, ./webproxy <portnumber> <caching timeout(in seconds)> where the portnumber is 
the port you wish to connect your browser to in the webproxy, and caching timeout is how long your cached web
pages will be saved for before being deleted.  

4) After running program proceed to search the web with your browser that has been configured to run on your proxy. 

DESCRPTION: 
	My program utilizes the arguments to set the port number for the socket to the client and the caching
timeout in seconds.  The Main process of my code has the task of running the listen socket and accepting all 
incoming connections, then forking a child process to handle data transfers. The main process also uses a filestat struct
to loop through all files within the "cache" directory and checks ctime to determine if it's time to delete the file.

	 Each process when receiving a GET command will first check the host name and determine if the host is blacklisted.
If the host name is contained in the blacklist folder 403 FORBIDDEN error will be sent.  If not I verify that the
message from the client is a GET HTTP command.  If not 400 Bad Request is sent. If all conditions are met the process
will hash the url in the GET command and check if the hashed number exists in the cached directory, if not then it will
set up a new socket to the web using the hostname to either lookup an ip address and save it in "lookup.txt" or if it
already exists in "lookup.txt" then I will use the cached IP to create the socket instead. The URL is hashed and header
forwarded on through the new TCP socket.  Data is read from webserver and simultaneously cached in the file and forwarded
back to the client.   