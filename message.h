//Author:         Noah Cregger
//Creation Date:  4/18/21
//Due Date:       5/6/21
//Course:         CSC552
//Professor Name: Dr. Spiegel
//Assignment:     Assignment 3
//Filename:       message.h
//Purpose:        This is the header file for the message struct 

#include    <stdlib.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>
#include	<sys/stat.h>
#include	<ctype.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<signal.h>
#include	<unistd.h>
#include	<stdlib.h>
#include 	<string.h>
#include	<limits.h>
#include    <fstream>
#include    <string>
#include    <cstring>
#include    <vector>
#include    <sys/wait.h>
#include    <stdio.h>
#include    <sstream>
#include    <unistd.h>
#include    <stdlib.h>
#include    <iostream>
#include    <ctype.h>
#include    <math.h>
#include    <sys/shm.h>
#include    <sys/sem.h>
#include    <algorithm>
#include    <time.h>

#define BUFSIZ 256 

/**
 * A MES struct for the socket communication.
 */
typedef struct {
long mtype;
long number;
char buffer[BUFSIZ];
} MES;

