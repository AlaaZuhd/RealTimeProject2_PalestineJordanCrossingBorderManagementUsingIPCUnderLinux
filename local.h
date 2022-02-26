#ifndef __LOCAL_H_
#define __LOCAL_H_

/*
 * Common header file: parent, producer and consumer
 */




#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <wait.h>
#include <signal.h>





#define MAX 1000


// structure for message queue
struct mesg_buffer {
	long mesg_type;
  int is_expired; /* 0 or 1 to indicate weather the passport of the passenger is expired or not */ 
  int forget_passport; 
  int from_pid; /* sender pid */
  int time_limit;
  int start_sec;
  int start_min;
	char mesg_text[100];
} message;

union semun {
  int              val;
  struct semid_ds *buf;
  ushort          *array; 
};

struct MEMORY {
  int  termination_variables[3];
  int leaving_passenger_ids[MAX]; //passengers leaving the crossing-borders and entering the Jordanian side successfully. 
  int  head, tail;
  int currently_in_hall;
};

struct sembuf acquire = {0, -1, SEM_UNDO}, 
              release = {0,  1, SEM_UNDO};

enum {GRANTED_ACCESS, DENIED_ACCESS, TIME_LIMIT_EXCEEDED,CURRENT_HALL_CAPACITY, BUS_TURN};

#endif

