/*Alaa Zuhd - 1180865
  Rawan Yassin - 1182224
  */

#include <time.h>
#include "local.h"


char map(int );

char temp[30];
int passenger_pid; 
int expired_date;
int time_limit;
int passenger_type; 
int officer_number;
int start = 0;
int end = 0;
int flag  = 1;
int main(int argc, char *argv[ ])
{
  time_t rawtime;
  struct tm * timeinfo1;
  if(argc != 4) {
    perror("Number of arguments must be 3!");
    exit(-1);
  } 
  passenger_pid = getpid();
  passenger_type = atoi(argv[1]);
  officer_number = atoi(argv[2]);
  time_limit = atoi(argv[3]);

	message.mesg_type = 1;
  srand((unsigned int)passenger_pid);
  if(rand() % 100 <= 90 ){ //90% of passengers would have their passport right (not expired)
    expired_date = 0;
    message.is_expired = 0;
  }else{
    expired_date = 1;
    message.is_expired = 1;
  }
  if(rand() % 100 <= 90 ){ //90% of passengers would have their passport with them
    message.forget_passport = 0;
  }else{
    message.forget_passport = 1;
  } 
  message.from_pid = passenger_pid; 
  sprintf(message.mesg_text,"abc Passenger with type %d + id %d ", passenger_type,officer_number);
  //Setting the start time
  time ( &rawtime );
  timeinfo1 = localtime ( &rawtime );
  message.time_limit = time_limit;
  message.start_sec = timeinfo1->tm_sec;
  message.start_min = timeinfo1->tm_min;
	// msgsnd to send message
	msgsnd(officer_number, &message, sizeof(message), 0);
  printf("\033[0;36m"); // set the color to cyan
  // display the message
  printf("Passenger: I'm Passenger with Type: %c and with Passport Status- Is_Expired: %d, Is-Forgotten: %d --> Entering Queue %d\n",
         map(passenger_type), expired_date, message.forget_passport, officer_number);
  printf("\033[0m");// reset the color to the default
  while(1);
  
	return 0;
}

char map(int type){
  if(type == 0)
    return 'P';
  else if (type == 1)
    return 'J';
  else if (type == 2)
    return 'F'; 
}

