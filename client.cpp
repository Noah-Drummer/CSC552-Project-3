//Author:         Noah Cregger
//Creation Date:  4/18/21
//Due Date:       5/6/21
//Course:         CSC552
//Professor Name: Dr. Spiegel
//Assignment:     Assignment 3
//Filename:       client.cpp
//Purpose:        This program is the client side of the message queue; it sets up a socket, then connects to the server that is listening.
//                Once a client connects, it allocates shared memory and sets up semaphores for updating shared memory and the log file.
//                Client can then ask requests of the server in a loop until they decide to quit, decrementing the number of clients on the 
//                machine and ending. 

/**
 * @file client.cpp
 * @author Noah Cregger
 * @brief  This program is the client side of the message queue; it sets up a socket, then connects to the server that is listening.
 Once a client connects, it allocates shared memory and sets up semaphores for updating shared memory and the log file. 
 Client can then ask requests of the server in a loop until they decide to quit, decrementing the number of clients on the
 machine and ending. 
 */

#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message.h"

using namespace std;

/* acad's Internet address, defined in /etc/hosts */
#define SERV_HOST_ADDR "156.12.127.18"

/**
 * A cli_info struct for the shared memory.
 */
typedef struct {
  int pid;
  int start;
  int numCom;
  int last;
} cli_info;


/**
* @brief Function to issue a wait() with semaphore
* @param key_t the semaphore ID
* @param int the semaphore's number
*/
void P(key_t, int);

/**
* @brief Function to issue a signal() with semaphore
* @param key_t the semaphore ID
* @param int the semaphore's number
*/
void V(key_t, int);

/**
* @brief Function to display the menu of available commands/options
*/	
void displayMenu();

/**
* @brief Function to get the command from the user
* @param char the character they input 
* @param MES & the message queue holding proper information
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
* @return char the char containing their command
*/
char getOpt(char, MES &, int, int, fstream&);

/**
* @brief Function to loop through client and get commands to send to server
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
*/
void sendRecieve(int, int, fstream&);

/**
* @brief Function to handle new record command
* @param MES & message struct for sending messages
* @param MES & message struct for incoming messages
* @param MES & message struct for recieving messages
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
*/	
void newRecord(MES &, MES &, MES &, int, int, fstream&);

/**
* @brief Function to handle display record command
* @param MES & message struct for sending messages
* @param MES & message struct for incoming messages
* @param MES & message struct for recieving messages
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
*/	
void displayRec(MES &, MES &, MES &, int, int, fstream&);

/**
* @brief Function to handle change record command
* @param MES & message struct for sending messages
* @param MES & message struct for incoming messages
* @param MES & message struct for recieving messages
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
*/	
void changeRec(MES &, MES &, MES &, int, int, fstream&);

/**
* @brief Function to handle show logs command
* @param MES & message struct for sending messages
* @param MES & message struct for incoming messages
* @param MES & message struct for recieving messages
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
*/	
void showLogs(MES &, MES &, MES &, int, int, fstream&);

/**
* @brief Function to handle list pid command
* @param MES & message struct for sending messages
* @param MES & message struct for incoming messages
* @param MES & message struct for recieving messages
* @param int sock file descriptor
* @param int the semaphore set ID
* @param fstream& the client log file
*/	
void listPID(MES &, MES &, MES &, int, int, fstream&);

/**
* @brief Update the shared memory space
* @param int semID the semaphore set ID
*/
void updateShm(int);

/**
* @brief Function to get the size of the log file
* @param fstream& the open log file
* @return int the size of the log file 
*/
int getLogSize(ifstream&);

int semID;
int shmid;
void *shmptr;
cli_info *clientInfo;
int maxCli = 10; 
char *numCli;



int main(int argc, char* argv[])
{
  int sockfd, i;
  struct sockaddr_in serv_addr;
  int SERV_TCP_PORT=15007;
  char *pname;
  key_t semKey=getuid();
  fstream log;
  int val;
  string conv;
  char c;


  pname = argv[0];

    /* create a socket which is one end of a communication channel */
  if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  { perror("client: cannot open stream socket"); exit(1);  }

    /* specify server address */
  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;  /* address family: Internet */
  serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	//serv_addr.sin_addr.s_addr = inet_aton(SERV_HOST_ADDR);
  serv_addr.sin_port = htons(SERV_TCP_PORT);

    /* connect the socket, sockfd, to the ser program.  */
  if(connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    { perror("client: cannot connect to server"); exit(2);  }

  //4 semaphores, 0 for read log, 1 for write log, 2 for read shared, 3 for write shared
  if ((semID = semget(semKey, 1, 0))<0) {
    semID = semget(semKey, 4, 0666|IPC_CREAT);
	log.open("log.cli", ios::out | ios::in | ios::trunc); //open the log file (r/w) and truncate it if it exists
	if (!log){
	  cout << "Error opening log file" << endl;
	  exit(0);
    }
	shmid = shmget(getuid(), (sizeof(cli_info)*maxCli)+1, IPC_CREAT|0600);
	shmptr = shmat(shmid, NULL, 0);
	numCli = (char *) shmptr;
	
	*numCli = '1';
	clientInfo = (cli_info *) (1 + shmptr);
	clientInfo->pid = getpid();
	clientInfo->start = 0;
	clientInfo->numCom = 0;
	clientInfo->last = 0;
	V(semID, 3);
  }
  else{
	P(semID, 3);
	shmid = shmget(getuid(), sizeof(cli_info)*maxCli+1, 0600);
	shmptr = shmat(shmid, 0, 0);
	numCli = (char *) shmptr;
	val = atoi(numCli);
	val = val + 1;
	conv = to_string(val);
	c = conv[0];
	*numCli = c;
	cout << "Num Cli: " << *numCli << endl;
	clientInfo = (cli_info *) ((1 + (sizeof(cli_info) * val))+numCli);
	clientInfo->pid = getpid();
	clientInfo->start = 0;
	clientInfo->numCom = 0;
	clientInfo->last = 0;
	V(semID, 3);
  }
  V(semID, 0);
  V(semID, 1);
  V(semID, 2);

  displayMenu();
  sendRecieve(sockfd, semID, log);

}

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
  // semCmd.sem_flg=0;
  semop(semID,&semCmd,1);
}

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
* @brief Function to display the menu of available commands/options
*/	
void displayMenu(){
  cout << "------------------" << endl;
  cout << "MENU:" << endl;
  cout << "------------------" << endl;
  cout << "N)ew Record" << endl;
  cout << "D)isplay Record" << endl;
  cout << "C)hange Record" << endl;
  cout << "S)how Log" << endl;
  cout << "L)ist PIDs" << endl;
  cout << "Q)uit" << endl;
  cout << "------------------" << endl;
  return; 
}

/**
* @brief Update the shared memory space
* @param int semID the semaphore set ID
*/
void updateShm(int semID){
  P(semID, 3);
  clientInfo->numCom += 1;
  clientInfo->last += 1;
  V(semID, 3);
}


/**
* @brief Function to get the command from the user
* @param char opt the character they input 
* @param MES & mes_out the message queue holding proper information
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
* @return char opt the char containing their command
*/
char getOpt(char opt, MES &mes_out, int sockfd, int semID, fstream& log){
  string in;
  int val;
  string conv;
  char c;
  cout << "CMD >";		
  cout.flush();
  memset(mes_out.buffer, 0x0, BUFSIZ);	//Clear buffer
  cin >> in;
  strcpy(mes_out.buffer, in.c_str());
  opt = mes_out.buffer[0];
  cout << opt << endl;
  //store appropriate request in message buffer for client request
  while (opt != 'N' && opt != 'n' && opt != 'D' && opt != 'd' && opt != 'C' && opt != 'c' && opt != 'S' && opt != 's' && opt != 'L' && opt != 'l' && opt != 'Q' && opt != 'q'){
	cout << "Not a command, try again\nCMD >";
	cout.flush();
	memset(mes_out.buffer, 0x0, BUFSIZ);	//Clear buffer
	cin >> in;
    strcpy(mes_out.buffer, in.c_str());
    opt = mes_out.buffer[0];
	cout << opt << endl;
  }	
  if (opt == 'N' || opt == 'n'){
	mes_out.buffer[0] = '1';
  }
  else if (opt == 'D' || opt == 'd'){
	mes_out.buffer[0] = '2';
  }
  else if (opt == 'C' || opt == 'c'){
	mes_out.buffer[0] = '3';
  }
  else if (opt == 'S' || opt == 's'){
	mes_out.buffer[0] = '4';
  }
  else if (opt == 'L' || opt == 'l'){
	mes_out.buffer[0] = '5';
  }
  else if (opt == 'Q' || opt == 'q'){
	mes_out.buffer[0] = '6';
	if(write(sockfd, &mes_out, sizeof(MES)) == -1){
	  perror("client: write"); exit(3);
	}
	
	P(semID, 3);
	val = atoi(numCli);
	val = val - 1;
	cout << val << endl;
	conv = to_string(val);
	c = conv[0];
	*numCli = c;
	if(val == 0){
	  log.close();
	  shmdt(shmptr);
	  shmctl(shmid, IPC_RMID, 0);	
	  semctl(semID, 4, IPC_RMID);
	}
	V(semID, 3);
	
	exit(0);
  }
  return opt;
}

/**
* @brief Function to handle new record command
* @param MES &mes_out message struct for sending messages
* @param MES &mes_in message struct for incoming messages
* @param MES &tmp message struct for recieving messages
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
*/	
void newRecord(MES &mes_out, MES &mes_in, MES& tmp, int sockfd, int semID, fstream& log){
  P(semID, 2);
  P(semID, 1);
  log<<"Client PID: "<<clientInfo->pid<<", Client Start Time: "<<clientInfo->start<<", Client Num Commands: "<<clientInfo->numCom<<", Client Last Msg Time: "<<clientInfo->last<<endl; 
  V(semID, 1);
  V(semID, 2);
  string in; 
  cout << "-------------------------------------------------------------------------------" << endl;
  cout << "Enter the full record to add (2 digits each, if single digit, type space before or after, otherwise no spaces):\n1st Rec(n must be first input before 8 digits)> ";
  cout.flush();
  cin >> in;
  strcpy(mes_out.buffer, in.c_str());
  if(write(sockfd, &mes_out, sizeof(MES)) == -1){
    perror("client: write"); exit(3);
  }
  memset(tmp.buffer, 0x0, BUFSIZ);
  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	perror("server: read"); exit(4);
  }
  mes_in.mtype = ntohl(tmp.mtype);
  printf("%s\n", tmp.buffer);
  updateShm(semID);
  
}

/**
* @brief Function to handle display record command
* @param MES &mes_out message struct for sending messages
* @param MES &mes_in message struct for incoming messages
* @param MES &tmp message struct for recieving messages
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
*/	
void displayRec(MES &mes_out, MES &mes_in, MES& tmp, int sockfd, int semID, fstream& log){
  P(semID, 2);
  P(semID, 1);
  log<<"Client PID: "<<clientInfo->pid<<", Client Start Time: "<<clientInfo->start<<", Client Num Commands: "<<clientInfo->numCom<<", Client Last Msg Time: "<<clientInfo->last<<endl; 
  V(semID, 1);
  V(semID, 2);
  int numRec, recNum;
  cout << endl << "-------------------------------------------------------------------------------" << endl;
  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	perror("server: read"); exit(4);
  }
  mes_in.mtype = ntohl(tmp.mtype);
  printf("Number of Records: %s\n", tmp.buffer);
  cout << "-------------------------------------------------------------------------------" << endl;
  numRec = atoi(tmp.buffer);
  cout << "Enter the number of the record to view (-999 for all)\nRecNum >";		
  cout.flush();
  cin >> recNum;
  
  //Check user input and make sure it is in bounds
  while((recNum < 1 || recNum > numRec) && recNum != -999){
	cout << "Out of range: Enter the number of the record to view (-999 for all)\nRecNum >";
	cout.flush();
	cin >> recNum;
  }
  //store response in the msg buffer
  mes_out.number = htonl(recNum);
  mes_out.buffer[0] = 'd';
  if(write(sockfd, &mes_out, sizeof(MES)) == -1){
    perror("client: write"); exit(3);
  }
  memset(tmp.buffer, 0x0, BUFSIZ);
  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	perror("server: read"); exit(4);
  }
  if(recNum == -999){
	cout << endl << "-------------------------------------------------------------------------------" << endl;
	cout << "Streaming Service Usage Per year (in millions):" << endl;
	cout << "2017 | 2018 | 2019 | 2020|" << endl;
	cout << "--------------------------" << endl << " ";
	for (int i = 0, j = 0; i < numRec; i++, j+=8){
	  cout << tmp.buffer[j] << tmp.buffer[j+1] << " | ";
	  cout << tmp.buffer[j+2] << tmp.buffer[j+3] << " | ";
	  cout << tmp.buffer[j+4] << tmp.buffer[j+5] << " | ";
	  cout << tmp.buffer[j+6] << tmp.buffer[j+7] << " | " << endl;
	}
	cout << endl;
  }
  //otherwise, print single full record
  else{
	cout << endl << "-------------------------------------------------------------------------------" << endl;
	cout << "Streaming Service Usage Per year (in millions):" << endl;
	cout << "2017 | 2018 | 2019 | 2020|" << endl;
	cout << "--------------------------" << endl << " ";
	for(int i = 0; i < 8; i+=2){
	  cout << tmp.buffer[i] << tmp.buffer[i+1] << "  |  ";
	}
	cout << endl << endl;;
  }
  memset(tmp.buffer, 0x0, BUFSIZ);
  updateShm(semID); 
}

/**
* @brief Function to handle change record command
* @param MES &mes_out message struct for sending messages
* @param MES &mes_in message struct for incoming messages
* @param MES &tmp message struct for recieving messages
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
*/
void changeRec(MES &mes_out, MES &mes_in, MES& tmp, int sockfd, int semID, fstream& log){
  P(semID, 2);
  P(semID, 1);
  log<<"Client PID: "<<clientInfo->pid<<", Client Start Time: "<<clientInfo->start<<", Client Num Commands: "<<clientInfo->numCom<<", Client Last Msg Time: "<<clientInfo->last<<endl; 
  V(semID, 1);
  V(semID, 2);
  int numRec, recNum, recIdx, rec;
  int recIn;
  string in;
  cout << endl << "-------------------------------------------------------------------------------" << endl;
  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	perror("server: read"); exit(4);
  }
  mes_in.mtype = ntohl(tmp.mtype);
  printf("Number of Records: %s\n", tmp.buffer);
  cout << "-------------------------------------------------------------------------------" << endl;
  numRec = atoi(tmp.buffer);
  cout << "Enter the number of the record to change: \nRecNum >";		
  cout.flush();
  cin >> recIn;
  recNum = recIn;
  
  while(recNum < 1 || recNum > numRec){
	cout << "Out of range: Enter the number of the record to change:\nRecNum >";
	cout.flush();
	cin >> recIn;
	recNum = recIn;
  }
  memset(mes_out.buffer, 0x0, BUFSIZ);
  mes_out.number = htonl(recNum);

  //tag the client message to send with 'c' so server knows what to process
  mes_out.buffer[0] = 'c';  
  
  cout << "-------------------------------------------------------------------------------" << endl;
  if(write(sockfd, &mes_out, sizeof(MES)) == -1){
    perror("client: write"); exit(3);
  } 
  memset(tmp.buffer, 0x0, BUFSIZ);
  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	perror("server: read"); exit(4);
  }
  cout << "This is the record you would like to change:\n";
  cout << "2017  |  2018  |  2019  |  2020  |\n";
  cout.flush();
  for (int i = 0, j = 1; i<8, j<=4; i+=2, j++){
    cout << j << ".) " << tmp.buffer[i] << tmp.buffer[i+1] << " | ";
  }
  cout << endl;
  cout << "-------------------------------------------------------------------------------" << endl;
  cout.flush();
  memset(mes_out.buffer, 0x0, BUFSIZ);
  cout << "Enter the record to change (1-4):\nRec >";
  cout.flush();
  cin >> recIdx;
  cout << recIdx << endl;
  cout << "-------------------------------------------------------------------------------" << endl;
  while(recIdx < 1 || recIdx > 4){
	cout << "Out of range: Enter the record to change (1-4)\nRec >";
	cout.flush();
	cin >> recIdx;
  }
  //store response in the msg buffer
  mes_out.buffer[0] = 'r';
  mes_out.number = htonl(recIdx);
  
  if(write(sockfd, &mes_out, sizeof(MES)) == -1){
    perror("client: write"); exit(3);
  } 
  memset(mes_out.buffer, 0x0, BUFSIZ);
  
  cout << "What would you like to change the record to (integer between 1-99, if single digit, type a space before it):\nCmd >";
  cout.flush();
  cin >> in;
  strcpy(mes_out.buffer, in.c_str());
  rec = atoi(mes_out.buffer);
  
  //check client input to make sure new record is in bounds
  while(rec < 0 || rec > 99){
	cout << "Out of range: Enter the number of the new Record (0-99)\nRecNum >";
	cout.flush();
	memset(mes_out.buffer, 0x0, BUFSIZ);
	cin >> in;
	strcpy(mes_out.buffer, in.c_str());
    rec = atoi(mes_out.buffer);
	//cin >> rec;
  }
  cout << "-------------------------------------------------------------------------------" << endl;
  mes_out.buffer[2] = mes_out.buffer[1];
  mes_out.buffer[1] = mes_out.buffer[0];
  mes_out.buffer[0] = 'R';
  //tag the client message to send with 'R' so server knows what to process
  if(write(sockfd, &mes_out, sizeof(MES)) == -1){
    perror("client: write"); exit(3);
  } 
  memset(tmp.buffer, 0x0, BUFSIZ);
  //recieve confirmation of request from  server
  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	perror("server: read"); exit(4);
  }
  printf("%s\n", tmp.buffer);
  memset(tmp.buffer, 0x0, BUFSIZ);
  updateShm(semID);
}

/**
* @brief Function to handle show logs command
* @param MES &mes_out message struct for sending messages
* @param MES &mes_in message struct for incoming messages
* @param MES &tmp message struct for recieving messages
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
*/
void showLogs(MES &mes_out, MES &mes_in, MES& tmp, int sockfd, int semID, fstream& log){
  int numLog;
  int i = 1;
  string in;
  memset(mes_out.buffer, 0x0, BUFSIZ);
  //recieve size of the log file for printing it out
  cout << "Display Server Logs (S) " << endl;
  cout << "Display Client Logs (L)\nCmd >";
  cout.flush();
  cin >> in;
  strcpy(mes_out.buffer, in.c_str());
  if(write(sockfd, &mes_out, sizeof(MES)) == -1){
	perror("client: write"); exit(3);
  }
  
  if(mes_out.buffer[0] == 'S' || mes_out.buffer[0] == 's'){
    if(read(sockfd, &tmp, sizeof(MES)) == -1){
	  perror("server: read"); exit(4);
    }
    printf("Number of logs in the file: %s\n", tmp.buffer);
    numLog = atoi(tmp.buffer);
    cout << "\n";
  
    cout << "DISPLAYING LOGS: "<< endl;
    //Print out the log file line by line
    while(numLog > 0){
	  memset(tmp.buffer, 0x0, BUFSIZ);
	  cout << "-------------------------------------------------------"<<endl;
	  //recieve a single log and print it
	  if(read(sockfd, &tmp, sizeof(MES)) == -1){
	    perror("server: read"); exit(4);
      }
      printf("%d.%s\n", i, tmp.buffer);
	  numLog = numLog - 1;
	  i = i+1;
    }
    cout << "-------------------------------------------------------"<<endl<<endl;
  }
  else{
	P(semID, 0);
	ifstream infile;
    string line;
	infile.open("log.cli");
	while(getline(infile, line)){
	  strcpy(tmp.buffer, line.c_str());
	  cout<<"-------------------------------------------------------"<<endl;
	  printf("%d.%s\n", i, tmp.buffer);
	  i=i+1;
	  memset(tmp.buffer, 0x0, BUFSIZ);
	}
	cout<<"-------------------------------------------------------"<<endl;
	V(semID, 0);
  }
  updateShm(semID);
}

/**
* @brief Function to handle list pid command
* @param MES &mes_out message struct for sending messages
* @param MES &mes_in message struct for incoming messages
* @param MES &tmp message struct for recieving messages
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
*/
void listPID(MES &mes_out, MES &mes_in, MES& tmp, int sockfd, int semID, fstream& log){
  int val; 
  cout << "DISPLAYING CLIENT PIDs ON LOCAL MACHINE" << endl;
  cout << "---------------------------------------" << endl;
  P(semID, 3);
  val = atoi(numCli);
  //cout << val<< endl;
  for (int i = 1; i <= val; i++){
	clientInfo = (cli_info *) ((1 + (sizeof(cli_info) * i))+numCli);
	cout << clientInfo->pid << endl;
  }
  cout << "---------------------------------------" << endl;
  V(semID, 3);
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
* @brief Function to loop through client and get commands to send to server
* @param int sockfd sock file descriptor
* @param int semID the semaphore set ID
* @param fstream& log the client log file
*/
void sendRecieve(int sockfd, int semID, fstream& log){
  MES mes_out, mes_in, tmp;
  string in;
  char opt;
  
  mes_out.mtype = htonl(getpid());

  opt = getOpt(opt, mes_out, sockfd, semID, log);
  while (opt != 'Q' && opt != 'q'){
	if(write(sockfd, &mes_out, sizeof(MES)) == -1){
	  perror("client: write"); exit(3);
    }
	
	if (opt == 'N' || opt == 'n'){
	  newRecord(mes_out, mes_in, tmp, sockfd, semID, log);
	}
	if (opt == 'D' || opt == 'd'){
	  displayRec(mes_out, mes_in, tmp, sockfd, semID, log);
	}
	if (opt == 'C' || opt == 'c'){
	  changeRec(mes_out, mes_in, tmp, sockfd, semID, log);
	}
	if (opt == 'S' || opt == 's'){
	  showLogs(mes_out, mes_in, tmp, sockfd, semID, log);
	}
	if (opt == 'L' || opt == 'l'){
	  listPID(mes_out, mes_in, tmp, sockfd, semID, log);
	}
	displayMenu();
	opt = getOpt(opt, mes_out, sockfd, semID, log);
  }
    
  close(sockfd);
  exit(0);
}
