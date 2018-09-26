Casey Durham
CSCI 4273

How to Run the program:
Open 2 terminals, one in the the client folder, one in the server folder. 
Run make to compile each file. Or run dual make file if both .c files are in the same directory.  

Below are the commmand line formats to run each program
./server portnumber
./client serverIP serverPortnumber

From there follow the prompts the client prints out.
The client allows you to do five things. 
1. get: Get a copy of a file residing in the same directory as the server's binary
2. put: transfer a file residing in the same directory as the client binary to the directory containing the server's 
binary
3. delete: delete a file in the same directory as the server binary
4. ls: list the files in the same directory as the server binary
5. exit: terminate both the server and client programs

