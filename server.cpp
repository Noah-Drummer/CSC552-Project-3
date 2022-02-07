//Author:         Noah Cregger
//Creation Date:  4/18/21
//Due Date:       5/6/21
//Course:         CSC552
//Professor Name: Dr. Spiegel
//Assignment:     Assignment 3
//Filename:       server.cpp
//Purpose:        This program is the server side of the message queue; it sets up a socket, then waits in a loop for clients to connect.
//                Once a client connects, it forks off a child server and handles the client's requests via its own child server process. 
//                It handles a log file which keeps track of the client actions, and also opens a binary file to do requests on. Semaphores
//                are used in this to ensure data doesn't get messed up in the log file or data file. 

/**
\mainpage  Documentation to Appear on Site Home Page
\n
The following documentation describes the implementation of a client/server system that 
uses a binary file to read/write records. This client/server system is implemented using sockets over the internet. It makes use of
semaphores to solve reader/writer issues, and also makes use of signals and shared memory. 
\n
---
\n
The server sets up a socket and waits for a connection, then it forks off and to a child server which handles client requests.
The server creates semaphores for reading and writing to data/log files. The server also has a signal handler for sigint which
will shut down the program and destroy all IPC.
\n
\n
---

The client sends tags a different key to the front of every message it sends for every seperate request so
the server knows how to handle the requests. The client also sets up semaphores, as well as shared data in order to do reading/writing
to log files and shared memory. The shared memory is in tact for client's to see what other clients on their local machine are doing. 

**This application was created for csc552 assignment 3	**
*/

/**
 * @file server.cpp
 * @author Noah Cregger
 * @brief  This program is the server side of the message queue; it sets up a socket, then waits in a loop for clients to connect.
 Once a client connects, it forks off a child server and handles the client's requests via its own child server process. 
 It handles a log file which keeps track of the client actions, and also opens a binary file to do requests on. Semaphores
 are used in this to ensure data doesn't get messed up in the log file or data file. 
 */

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>

#include "message.h"

using namespace std;

/* acad Internet address, defined in /etc/hosts */
// This is the new acad's IP address
#define SERV_HOST_ADDR "156.12.127.18"

// Issues a wait() with SEMUNDO
/**
* @brief Function to issue a wait() with semaphore
* @param key_t semID the semaphore ID
* @param int semNum the semaphore's number
*/
void P(key_t semID,int semNum);

// Issue a signal()
/**
* @brief Function to issue a signal() with semaphore
* @param key_t semID the semaphore ID
* @param int semNum the semaphore's number
*/
void V(key_t semID,int semNum);

/**
* @brief Function to receive request from client and process it to respond
* @param int the server's sock file descriptor
*/	
void recieveSend(int);

/**
* @brief Function to get the number of records in the file
* @param fstream& the open binary file
* @return int the number of records in the file
*/
int getNumRec(fstream&);

/**
* @brief Function to read all of the records in the binary file
* @param fstream& the open binary file
* @param vector<char>& vector to store all of the records' info in
* @param int the number of records in the file
*/	
void readAll(fstream&, vector<char> &, int);

/**
* @brief Function to read a record from the binary file
* @param int the record to read
* @param fstream& the open binary file
* @param vector<char>& vector to store the record info in
*/	
void readRecord(int, fstream&, vector<char> &);

/**
* @brief Function to write a new record to the binary file
* @param fstream& the open binary file
* @param vector<string>& vector that contains the new record info
*/	
void newRecord(fstream&, vector<string>&);

/**
* @brief Function to give concurrent access to readers and make writers wait
*/
void getConcurrentAccess();

/**
* @brief Function to free concurrent access of readers and signal writers if no readers left
*/
void freeConcurrentAccess();

/**
* @brief Function to give concurrent access to readers and make writers wait for log
*/
void getLConcurrentAccess();

/**
* @brief Function to free concurrent access of readers and signal writers if no readers left for log
*/
void freeLConcurrentAccess();

/**
* @brief Function to get the size of the log file
* @param fstream& the open log file
* @return int the size of the log file 
*/	
int getLogSize(ifstream&);

/**
* @brief Function to create semaphores if they don't exists
* @param key_t semaphore key for creation
* @return int the semaphore ID
*/
int createSem(key_t);

/**
* @brief Function to add a new record to binary file
* @param MES& the message struct with needed information
* @param fstream& the open binary file
* @param int sock file descriptor for client
*/	
void addNewRec(MES&, fstream&, int);

/**
* @brief Function for handling the starting of display request
* @param MES& the message struct with needed information
* @param MES& the message struct with needed information
* @param fstream& the open binary file
* @param int sock file descriptor for client
*/
void startDisplay(MES&, MES&, fstream&, int);

/**
* @brief Function to display and send records to client
* @param MES& the message struct with needed information
* @param fstream& the open binary file
* @param int sock file descriptor for client
*/
void display(MES&, fstream&, int);

/**
* @brief Function for handling the starting of change record request
* @param MES& the message struct with needed information
* @param fstream& the open binary file
* @param int sock file descriptor for client
*/
void startChange(MES&, fstream&, int);

/**
* @brief Function to send the record to be changed to client
* @param MES& the message struct with needed information
* @param fstream& the open binary file
* @param int sock file descriptor for client
* @param vector<char>& vector to store full record in
* @return int the number of the record to change
*/
int sendRecord(MES&, fstream&, int, vector<char>&);

/**
* @brief Function change the record and send confirmation to client
* @param MES& the message struct with needed information
* @param fstream& the open binary file
* @param vector<char>& the vector holding the full record 
* @param int the number of the record in the table
* @param int the index of the record in the row to change
* @param int sock file descriptor for client
*/
void endChange(MES&, fstream&, vector<char>&, int, int, int);

/**
* @brief Function to send the server log file to the client
* @param MES& the message struct with needed information
* @param int sock file descriptor for client
*/
void displayLogs(MES&, int);

/**
* @brief signal handler for sigchld
* @param int the signal to catch
*/
void handler(int);

/**
* @brief signal handler for sigint
* @param int the signal to catch
*/
void inHandler(int sig);

int semID;
int readCount = 0;
int readLcount = 0;
int writeCount = 0;
int recSize = 128;
int numChildSer = 0;

int main(int argc,char *argv[])
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int SERV_TCP_PORT = 15007;
    

    /* create a socket which is one end of a communication channel */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    { perror("server: cannot open stream socket"); exit(1); }

    /* initialize server address */
	memset(&serv_addr, 0x0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; /* address family: Internet */
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
         /* accept a connection on any Internet interface */
    serv_addr.sin_port = htons(SERV_TCP_PORT); /* specify port number */

    /* associate server address with the socket */
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
       < 0)
    { perror("server: cannot bind local address"); exit(2); }

    /* specify the queue size of the socket to be 5 */
    listen(sockfd, 5);
	recieveSend(sockfd);
}

// Issues a wait() with SEMUNDO
/**
* @brief Function to issue a wait() with semaphore
* @param key_t semID the semaphore ID
* @param int semNum the semaphore's number
*/
void P(key_t semID,int semNum)
{ struct sembuf semCmd;
  semCmd.sem_num=semNum;
  semCmd.sem_op=-1;
  semCmd.sem_flg=SEM_UNDO;
  semop(semID,&semCmd,1);
}

// Issue a signal()
/**
* @brief Function to issue a signal() with semaphore
* @param key_t semID the semaphore ID
* @param int semNum the semaphore's number
*/
void V(key_t semID,int semNum)
{ struct sembuf semCmd;
  semCmd.sem_num=semNum;
  semCmd.sem_op=1;
  semCmd.sem_flg=0;
  semop(semID,&semCmd,1);
}

/**
* @brief Function to get the number of records in the file
* @param fstream& file the open binary file
* @return int numRec the number of records in the file
*/
int getNumRec(fstream& file){
	
  int begin, end;
  int numRec;

  getConcurrentAccess();
  file.seekg(0, ios::beg); //seek beginning of file and store it in begin
  begin = file.tellg();
  file.seekg(0, ios::end); //seek end of file and store it in end
  end = file.tellg();
	
  numRec = (end-begin)/recSize; //Process number of records (recSize is 128 because thats how many bytes are in each record)
  freeConcurrentAccess();
  return numRec;
}

/**
* @brief Function to read all of the records in the binary file
* @param fstream& file the open binary file
* @param vector<char>& tempAll vector to store all of the records' info in
* @param int numRec the number of records in the file
*/	
void readAll(fstream& file, vector<char> &tempAll, int numRec){
  getConcurrentAccess();
  file.seekg(0, ios::beg); //seek to beginning of file
  file.read(&tempAll[0], (numRec*recSize)); //read the entire file into tempAll 
  freeConcurrentAccess();
  return;	
}

/**
* @brief Function to read a record from the binary file
* @param int record the record to read
* @param fstream& file the open binary file
* @param vector<char> &temp vector to store the record info in
*/
void readRecord(int record, fstream& file, vector<char> &temp){
  getConcurrentAccess();
  int loc = (record-1)*recSize;
  file.seekg(loc, ios::beg); //seek to the location of the record in the file
  file.read(&temp[0], recSize); //read the record (128 bytes) and store it in temp vector
  freeConcurrentAccess();
  return;
}

/**
* @brief Function to write a new record to the binary file
* @param fstream& file the open binary file
* @param vector<string> &newRec vector that contains the new record info
*/
void newRecord(fstream& file, vector<string> &newRec){
  P(semID, 3);
  file.seekg(0, ios::end); //seek to the end of the file
  file.write(newRec[0].c_str(), newRec.size()*sizeof(string)); //write the full new record to the end of the file
  V(semID, 3);
  return;
}

/**
* @brief Function to change a record in the binary file
* @param fstream& file the open binary file
* @param vector<char> &toChange vector that holds the new element to put into a record
* @param vector<char> &tempChStore vector that holds the original record from the file
* @param int record the number of the record to change
* @param int recordIdx the index of the element to change in the record
*/	
void changeRecord(fstream& file, vector<char> &toChange, vector<char> &tempChStore, int record, int recordIdx){
  P(semID, 3);
  int loc = (record-1)*recSize;
  vector<string> writeRow;
  
  //find which index of the record to change the new record to
  if (recordIdx == 1){
	tempChStore[0] = toChange[0];
	tempChStore[1] = toChange[1];
  }
  else if (recordIdx == 2){
	tempChStore[2] = toChange[0];
	tempChStore[3] = toChange[1];
  }
  else if (recordIdx == 3){
	tempChStore[4] = toChange[0];
	tempChStore[5] = toChange[1];
  }
  else if (recordIdx == 4){
	tempChStore[6] = toChange[0];
	tempChStore[7] = toChange[1];
  }  
  //store the changed full record into string array for writing to file
  for (int i = 0; i < tempChStore.size(); i+=2){
	string a(1, tempChStore.at(i));
	string b(1, tempChStore.at(i+1));
	string c = a + b;
	writeRow.push_back(c);
  }
  file.seekg(loc, ios::beg); //seek the location of the record
  file.write(writeRow[0].c_str(), writeRow.size()*sizeof(string)); //write new full record to that location 
  V(semID, 3);
  return;
}

/**
* @brief Function to get the size of the log file
* @param fstream& infile the open log file
* @return int numLines the size of the log file 
*/
int getLogSize(ifstream& infile){
  int numLines=0;
  string line;
  //increment numLines while not eof 
  while(getline(infile, line)){
	++numLines;
  }
  return numLines;  
}

/**
* @brief Function to give concurrent access to readers and make writers wait
*/
void getConcurrentAccess(){
  P(semID, 2);
  readCount += 1;
  if(readCount == 1){
	P(semID, 3);
  }
  V(semID, 2);
}

/**
* @brief Function to free concurrent access of readers and signal writers if no readers left
*/
void freeConcurrentAccess(){
  P(semID, 2);
  readCount -= 1;
  if(readCount == 0){
	V(semID, 3);
  }
  V(semID, 2);
}

/**
* @brief Function to give concurrent access to readers and make writers wait for log
*/
void getLConcurrentAccess(){
  P(semID, 0);
  readLcount += 1;
  if(readLcount == 1){
    P(semID, 1);
  }
  V(semID, 0);
}

/**
* @brief Function to give concurrent access to readers and make writers wait for log
*/
void freeLConcurrentAccess(){
  P(semID, 0);
  readLcount -= 1;
  if(readLcount == 0){
	V(semID, 1);
  }
  V(semID, 0);
}

/**
* @brief Function to create semaphores if they don't exists
* @param key_t semKey semaphore key for creation
* @return int semID the semaphore ID
*/
int createSem(key_t semKey){
  if ((semID = semget(semKey, 1, 0))<0) {
    semID = semget(semKey, 4, 0666|IPC_CREAT);
  }
  return semID;
}

/**
* @brief Function to add a new record to binary file
* @param MES& tmp the message struct with needed information
* @param fstream& file the open binary file
* @param int newsockfd sock file descriptor for client
*/	
void addNewRec(MES &tmp, fstream &file, int newsockfd){
  string newDone = "Record successfully added!";
  int numRec;
  vector<char> newCharRec(8);
  vector<string> newRec(4);
  for (int i = 0; i < 8; i++){
    newCharRec[i] = tmp.buffer[i+1];
  }		
  for (int j = 0, i = 0; j < newCharRec.size(); j+=2, i++){
    string a(1, newCharRec.at(j));
    string b(1, newCharRec.at(j+1));
    string c = a + b;
    newRec[i] = c;
  }
  numRec = getNumRec(file);
  newRecord(file, newRec);
  memset(tmp.buffer, 0x0, BUFSIZ);
  strcpy(tmp.buffer, newDone.c_str());
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write"); 
	exit(6);
  }
}

/**
* @brief Function for handling the starting of display request
* @param MES& tmp the message struct with needed information
* @param MES& message the message struct with needed information
* @param fstream& file the open binary file
* @param int newsockfd sock file descriptor for client
*/
void startDisplay(MES& tmp, MES& message, fstream &file, int newsockfd){
  int numRec; 
  numRec = getNumRec(file);
  memset(tmp.buffer, 0x0, BUFSIZ);
  cout << message.mtype << ": DISPLAYING RECORD" << endl;
  string str = to_string(numRec);
  strcpy(tmp.buffer, str.c_str());
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write");
	exit(6);
  }	
}

/**
* @brief Function for handling the starting of change record request
* @param MES& tmp the message struct with needed information
* @param fstream& file the open binary file
* @param int newsockfd sock file descriptor for client
*/
void startChange(MES& tmp, fstream& file, int newsockfd){
  int numRec;
  memset(tmp.buffer, 0x0, BUFSIZ);	//Clear buffer
  numRec = getNumRec(file);
  string str = to_string(numRec);
  strcpy(tmp.buffer, str.c_str());
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write");
	exit(6);
  }
}

/**
* @brief Function to send the record to be changed to client
* @param MES& tmp the message struct with needed information
* @param fstream& file the open binary file
* @param int newsockfd sock file descriptor for client
* @param vector<char>& tempChStore vector to store full record in
* @return int recordC the number of the record to change
*/
int sendRecord(MES& tmp, fstream& file, int newsockfd, vector<char>& tempChStore){
  int recordC;
  vector<char> tempCh(recSize);
  recordC = ntohl(tmp.number);
  memset(tmp.buffer, 0x0, BUFSIZ);	//Clear buffer
  readRecord(recordC, file, tempCh);
  //fill message buffer and tempChStore with the full record to change
  for(int i = 0, j = 0; i < tempCh.size(), j<8; i+=32, j+=2){
    tmp.buffer[j] = tempCh[i];
    tempChStore[j] = tempCh[i];
    tmp.buffer[j+1] = tempCh[i+1];
    tempChStore[j+1] = tempCh[i+1];
  }
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write");
	exit(6);
  }
  return recordC;  
}

/**
* @brief Function change the record and send confirmation to client
* @param MES& tmp the message struct with needed information
* @param fstream& file the open binary file
* @param vector<char>& tempChStore the vector holding the full record 
* @param int recordC the number of the record in the table
* @param int recordIdx the index of the record in the row to change
* @param int newsockfd sock file descriptor for client
*/
void endChange(MES& tmp, fstream& file, vector<char>& tempChStore, int recordC, int recordIdx, int newsockfd){
  vector<char> toChange(2);
  string done = "Record successfully changed!";
  toChange[0] = tmp.buffer[1];
  toChange[1] = tmp.buffer[2];
  
  changeRecord(file, toChange, tempChStore, recordC, recordIdx);
  memset(tmp.buffer, 0x0, BUFSIZ);
  strcpy(tmp.buffer, done.c_str());	
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write");
	exit(6);
  }
}

/**
* @brief Function to send the server log file to the client
* @param MES& tmp the message struct with needed information
* @param int newsockfd sock file descriptor for client
*/
void displayLogs(MES& tmp, int newsockfd){
  getLConcurrentAccess();
  ifstream infile;
  string line;
  infile.open("log.ser");
  int numLines = getLogSize(infile);
  memset(tmp.buffer, 0x0, BUFSIZ);
  string str = to_string(numLines);
  strcpy(tmp.buffer, str.c_str());
  //send the number of lines to client
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write");
	exit(6);
  }
  memset(tmp.buffer, 0x0, BUFSIZ);
  infile.close();
  infile.open("log.ser");
  //send the log line by line to the client 
  while(getline(infile, line)){
    strcpy(tmp.buffer, line.c_str());
    if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	  perror("server: write");
	  exit(6);
    }
  }
  infile.close();  
  freeLConcurrentAccess();
}

/**
* @brief Function to display and send records to client
* @param MES& tmp the message struct with needed information
* @param fstream& file the open binary file
* @param int newsockfd sock file descriptor for client
*/
void display(MES& tmp, fstream& file, int newsockfd){
  int numRec, recordD;
  
  numRec = getNumRec(file);
		  
  recordD = ntohl(tmp.number);
		  
  if(recordD == -999){
    memset(tmp.buffer, 0x0, BUFSIZ);	//Clear buffer
	cout << "DISPLAY ALL" << endl;
    vector<char> tempAll(recSize*numRec);
    readAll(file, tempAll, numRec);
    for(int i = 0, j = 0; i < tempAll.size(); i+=32, j+=2){
      tmp.buffer[j] = tempAll[i];
      tmp.buffer[j+1] = tempAll[i+1];
    }
  }
  //otherwise, process a single record and store it in the message buffer to send
  else{
	cout << "DISPLAY ONE" << endl;
    memset(tmp.buffer, 0x0, BUFSIZ);	//Clear buffer
    vector<char> temp(recSize);
    readRecord(recordD, file, temp);
    for(int i = 0, j = 0; i < temp.size(), j<8; i+=32, j+=2){
      tmp.buffer[j] = temp[i];
      tmp.buffer[j+1] = temp[i+1];
    }
  }
  if(write(newsockfd, &tmp, sizeof(MES)) == -1){
	perror("server: write");
	exit(6);
  }
}

/**
* @brief Function to receive request from client and process it to respond
* @param int sockfd the server's sock file descriptor
*/	
void recieveSend(int sockfd){
  fstream log, file;
  int newsockfd, clilen, pid, err, numRec, recordD, recordC, recordIdx;
  struct sockaddr_in cli_addr;
  vector<char> tempChStore(8);
  vector<char> toChange(2); 
  MES message, tmp;
  key_t semKey=15007;
  struct sigaction action, action2;
  action.sa_handler = (void (*)(int))handler;
  action2.sa_handler = (void(*)(int))inHandler;
  int signalnum = 17;
  int sigInt = 2;
  int status;
	
  file.open("x", ios::in | ios::out | ios::binary); //open the binary file for read/write 
  if (!file){
    cout << "Error opening file" << endl;
    exit(0);
  } 
  log.open("log.ser", ios::out | ios::in | ios::trunc); //open the log file (r/w) and truncate it if it exists
  if (!log){
	cout << "Error opening log file" << endl;
	exit(0);
  }
  //CREATE SEMAPHORES, 4 SETS, 1 SET FOR READ, 1 SET FOR MUTUAL EXCLUSION AKA WRITE on both Log and binary
  //0 READ LOG and 1 WRITE LOG, 2 READ DATA and 3 WRITE DATA
  int semID;
  semID = createSem(semKey);
  sigaction(sigInt, &action2, NULL);

  while(1){
    clilen = sizeof(cli_addr);
	
	/* wait for connection from the cli program.  Once connected, */
	/* newsockfd is the fd to be used with read() and write()  */
	if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
			   &clilen)) < 0)
	  { perror("server: accept error"); exit(3);  }
	V(semID,0);
	V(semID,1);
	V(semID,2);
	V(semID,3);
	if((pid = fork()) < 0)
	  { perror("server: fork"); exit(4); }
	numChildSer= numChildSer+1; 
	if(pid > 0){
	  //sigaction(sigInt, &action2,NULL);
	  //sigaction(sigInt, &action2,NULL);
	  if(signal(signalnum, handler) == SIG_ERR){
		if (sigaction(signalnum, &action, NULL)<0){
		}
	  }
	  if (numChildSer == 0){
		cout << "SEND KILL" << endl;
		kill(getpid(), 2);  
	  }
	  //waitpid(pid, &status, 0); 
	  //pid = wait(NULL);
	  //sigaction(sigInt, &action2,NULL);
	}
	else if(pid==0){ /* child process */
	  fprintf(stderr, "NUM CHILD SERV %d\n", numChildSer);
	  P(semID, 1);
	  log<<"Client Address: "<<ntohl(cli_addr.sin_addr.s_addr)<<", Client Port: "<<ntohl(cli_addr.sin_port)<< ", Connected."<<endl;
	  V(semID, 1);
	  while(1){
		if((err = read(newsockfd, &tmp, sizeof(MES))) == -1)
		  { perror("server: read"); exit(4); }
		else if (err == 0) exit(5);
		
		message.mtype = ntohl(tmp.mtype);
		message.buffer[0] = tmp.buffer[0];
		if(message.buffer[0] == '1'){
		  P(semID, 1);
		  log<<"Client Address: "<<ntohl(cli_addr.sin_addr.s_addr)<<", Client Port: "<<ntohl(cli_addr.sin_port)<< ", Command: New Record."<<endl;
		  V(semID, 1);
		  cout << message.mtype << ": CREATING NEW RECORD" << endl;
		}
		else if(message.buffer[0] == 'n'){
		  addNewRec(tmp, file, newsockfd);
		}
		else if(message.buffer[0] == '2'){
		  P(semID, 1);
		  log<<"Client Address: "<<ntohl(cli_addr.sin_addr.s_addr)<<", Client Port: "<<ntohl(cli_addr.sin_port)<< ", Command: Display Record."<<endl;
		  V(semID, 1);
		  startDisplay(tmp, message, file, newsockfd);
		}
		else if(message.buffer[0] == 'd'){
		  display(tmp, file, newsockfd);
		}
		
		else if(message.buffer[0] == '3'){
		  P(semID, 1);
		  log<<"Client Address: "<<ntohl(cli_addr.sin_addr.s_addr)<<", Client Port: "<<ntohl(cli_addr.sin_port)<< ", Command: Change Record."<<endl;
		  V(semID, 1);		
		  startChange(tmp, file, newsockfd);
		}
		else if(message.buffer[0] == 'c'){
		  recordC = sendRecord(tmp, file, newsockfd, tempChStore);    	
		}
		else if(message.buffer[0] == 'r'){
		  recordIdx = ntohl(tmp.number);
		}
		else if (message.buffer[0] == 'R'){
		  endChange(tmp, file, tempChStore, recordC, recordIdx, newsockfd);
		}
		
		else if (message.buffer[0] == '4'){
		  
		}
		
		else if (message.buffer[0] == 'S' || message.buffer[0] == 's'){
		  displayLogs(tmp, newsockfd);
	    }
		else if(message.buffer[0] == '6'){
		  //numChildSer = numChildSer - 1;
		  exit(17);
		}
		
		memset(message.buffer, 0x0, BUFSIZ);
		memset(tmp.buffer, 0x0, BUFSIZ);
	  }
	  
	}
	close(newsockfd);
        /* do nothing else for the parent process */
  }
}

/**
* @brief signal handler for sigchld
* @param int sig the signal to catch
*/
void handler(int sig)
{
	fprintf(stderr,"Caught Signal %d\n",sig);
	numChildSer = numChildSer - 1;
	fprintf(stderr,"ChildServers: %d\n", numChildSer);
}

/**
* @brief signal handler for sigint
* @param int sig the signal to catch
*/
void inHandler(int sig){
	string in;
	sigset_t sigset,oldset,pendingset;
	fprintf(stderr,"Caught Signal %d\n",sig);
	cout << "Would you like to shut down(s) or wait(w) for more clients?\nCMD>";
	cin >> in;
	if(in == "S" || in == "s"){
		semctl(semID, 4, IPC_RMID);
		//close(newsockfd);
		kill(getpid(), 2);
	}
	else{
		semctl(semID, 4, IPC_RMID);
		//close(newsockfd);
		kill(getpid(), 2);
	  //sigaddset(&sigset,2);
	  //sigprocmask(SIG_BLOCK,&sigset,&oldset);
	  //sigpending(&pendingset);
	}
	
}