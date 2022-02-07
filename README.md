# CSC552-Project-3
Client/Server project that I created for my advanced unix scripting course in grad school. 

The server loops infinitely until the client exits, in which case the child server exits, 
but the system is constantly waiting for new clients until a signal is given to kill it. I set up semaphores after the socket is made, and use those
for handling the data. When reading files, it is concurrent access. When writing to files, it is a mutex. This ensures that data isn't being messed up
from other processes getting in when they shouldn't. On the client side, I set it up to connect to the socket, and once they are connected, they will
create semaphores as long as there are none, and then allocate shared memory. 
	
For the shared memory, I used the first byte of it to count the number of clients on the machine and used it as an indexer. I stored a struct
that contained the information for each client in each slot of shared memory, using the indexer to read and write to the right one. I was unable
to find a way to ensure that all slots were used, so if there are multiple clients on a machine and one client leaves, it might cause overlap in
the shared memory. I set it up so that only 10 clients can be on a machine at a time, after that, there will be overflow and the program will probably
crash. 

The client side allows users to select from a menu of commands and the client will be able to interface with the server through these commands.
There are six overarching commands which are:
		Option          | Command
		-----------------------------
		N)ew Record     - 'N' or 'n'
		D)isplay Record - 'D' or 'd'
		C)hange Record  - 'C' or 'c'
		S)how Log       - 'S' or 's'
		L)ist PIDS      - 'L' or 'l'
		Q)uit           - 'Q' or 'q'
		-----------------------------
	For each command, it will break into an if statement. Each main command has several subcommands which I will go over here:
	
		N)ew Record:
		-----------------------------------------------------------------------------------------------------------------------------
			Command: n<8 digits> - this will create a new record out of those 8 digits. The full record must be prefaced with 'n'
			                       so the server knows what request to process; 'n' is the tag for the server. If one of the elements
								   you want to store in the record has only 1 digit, preface it with a space.
			Examples:
				I want to create a new record "82 88 79 80". The command would look like this:
					n82887980
				If I want to create a new record "8 9 10 12", the command would look like this:
					n 8 91012     OR      n8 9 1012
					
		D)isplay Record:
		------------------------------------------------------------------------------------------------------------------------------
			Record to view Command: <# of record to view> - It is simply 1 to 2 digits telling the server which record to view. If you want 
															to display all of the records, type -999 for this command. If your input is not 
															-999 or in bounds of the file, you will have to input this command until your input 
															is in bounds or -999.
			
				Examples of the Record to view Command:
					I want to view the 12th record. The command would look like this:
						12
					If I want to view all of the records, the command would look like this:
						-999
					
		C)hange Record:
		------------------------------------------------------------------------------------------------------------------------------
			Record to change Command: <# of record to change> - It is simply 1 to 2 digits telling the server which record you want to change. 
																If your input is not in bounds, you will have to input this command until you 
																input an appropriate record.
												   
				Example of the Record to Change Command:
					I want to change the 12th record. The command would look like this:
						12
						
			Record Index to Change Command: <# of record index> -  It is simply 1 digit telling the server the index of the record you would
																   like to change. Once an appropriate record index is input, it tags it with 'r,
																   so the server knows how to process this request.
																  
				Example of the Record Index to Change Command: 
					I want to change the 1st index. The command would look like this:
						1
						
			New Record Command: <# of new Record>            - This is simply 1 to 2 digits telling the server the new record to store into the
															   index of the previous command. If it is 1 digit, put a space before or after the 
															   digit. It can only be 1 to 2 digits and it must be positive. If your input is
															   out of bounds, you will have to input this command again until it is not out of
															   bounds. 
															   
				Examples of the New Record Command:
					I want the new record to be 32, the command would look like this:
						32
					If I want the new record to be 3, the command would look like this:
						3 (with a space before or after)
					
		S)how Log:
		------------------------------------------------------------------------------------------------------------------------------------------
			List Server Logs: S                             - This will contact the server and have it display all of the logs of the server
															  log in a well formattet way.
			
			List Client Logs: L                             - This will have the client display the client logs in a well
															  formatted way. 
															  
		L)ist PIDS:
		------------------------------------------------------------------------------------------------------------------------------------------													
			List Client PIDS: L								- This will cause the client to loop through shared memory, and print out all of the client
															  PIDS on the machine. 
		
		Q)uit:
		------------------------------------------------------------------------------------------------------------------------------------------
			Quit the client: Q | q                          - This will decrement the number of clients on the local machine and send a signal
								
                
How to run:
	-First, to compile the programs, I have given you a make file. You can type "make" to compile both programs, or you can specify one or the other.
	-After compiling the programs, you run ./server on one terminal, and then ./client on other terminals, and this runs the program.
	-I have also given you three test scripts: cliA.sh, cliB.sh and cliC.sh. These test scripts are to be run on separate machines to test 
	the server program.
	-To run the test scripts type:
		-sleep 2 && sh cliC.sh (on one machine), 
		-sleep 1 && sh cliB.sh (on another machine), 
		-sh cliA.sh (on the last machine)
	-As long as you time these well, it should appear to simulate clients running the server and giving commands on 3 separate machines. 
	-I tried to set the server to give you options for when you want to close it based off the number of clients on the server, but I had issues
	getting this to work. In the end, I settled with setting up a sigHandler for SIGINT, but the only way you can cause it is by inputting CTRL-C
	by hand on the server. So when you want to close the server, input CTRL C and it will prompt you to shutdown or wait (both result in shutdown because
	I was unable to get the server to wait properly). 
 
