/*Alaa Zuhd - 1180865
  Rawan Yassin - 1182224
  */
//#include "local.h"

#include <sys/stat.h>
#include <time.h>
#include "local.h" 


char map(int ); //mapping the type with a character for displaying
int calculate_time_difference(int, int, int, int); //for time_limit purpose

int officer_type; // 0 for palestinian, 1 for jordanina , 2 for foreign
int processing_passenger_rate; //as each officer process passengers in different rate
int flag = 0; 

int main(int argc, char *argv[ ])
{
  flag = 0; 
  time_t rawtime;
  struct tm * timeinfo2;
  int max_hall_threshold;
  int min_hall_threshold;
  int passed_max_impatient_passenger_number; 
  int passed_max_deniend_passengers_number;
  int passed_max_granted_passengers_number;
  int time_difference;

  int            semid, shmid;
  pid_t          ppid = getppid();
  char          *shmptr;
  struct MEMORY *memptr;
  int queue_id;

	if(argc != 8) {
        perror("Number of arguments must be 3!");
        exit(-1);
  }
  officer_type = atoi(argv[1]);
  queue_id = atoi(argv[2]);
  srand((unsigned int)getpid());
  processing_passenger_rate = rand() % 3 + 1;
  min_hall_threshold = atoi(argv[3]);
  max_hall_threshold = atoi(argv[4]);
  passed_max_impatient_passenger_number = atoi(argv[5]);
  passed_max_deniend_passengers_number = atoi(argv[6]);
  passed_max_granted_passengers_number = atoi(argv[7]); 

  printf("\033[0;34m"); // set the color to cyan
  printf("Officer: I'm Officer with Type: %c and Queue ID: %d, Will Start Handling\n", map(officer_type), queue_id);
  printf("\033[0m");// reset the color to the default

  //Access and attach to shared memory
  if ( (shmid = shmget((int) ppid, 0, 0)) != -1 ) {
    if ( (shmptr = (char *) shmat(shmid, (char *)0, 0)) 
	 == (char *) -1 ) {
      perror("shmat -- producer -- attach");
      exit(1);
    }
    memptr = (struct MEMORY *) shmptr;
  }
  else {
    perror("shmget -- producer -- access");
    exit(2);
  }
  //Access the semaphore set
  if ( (semid = semget((int) ppid, 5, 0)) == -1 ) {
    perror("semget -- producer -- access");
    exit(3);
  }
  
  while(1){
    //recieve the message from the passenger to be handled
    msgrcv(queue_id, &message, sizeof(message), 1, 0);
    //1- The first case to consider is that the time-limit for this specific passenger has not exceeded yet
    time ( &rawtime );
    timeinfo2 = localtime ( &rawtime );
    time_difference = calculate_time_difference(message.start_min, message.start_sec, timeinfo2->tm_min, timeinfo2->tm_sec);
    if(time_difference > message.time_limit){
      //Updating the variable in shared memory
      acquire.sem_num = TIME_LIMIT_EXCEEDED;
      if ( semop(semid, &acquire, 1) == -1 ) {
        perror("semop -- producer -- acquire1");
        exit(4);
      }
      memptr->termination_variables[2]++; 
      if(memptr->termination_variables[2]>passed_max_impatient_passenger_number){
        printf("\033[5;31m"); // set the color to red
        printf("Terminating Due to Exceeding MAX_IMPATIENT_PASSENGERS_NUMBER\n");
        printf("\033[0m");// reset the color to the default
        kill(getppid(),SIGQUIT);
      }    
      kill(message.from_pid,SIGTERM);
      printf("\033[0;31m"); // set the color to red
      printf("Passenger: Passenger (%d) Is Impatient & Getting Out\n", message.from_pid);
      printf("\033[0m");// reset the color to the default
      printf("\033[1;33m"); // set the color to yellow
      printf("\n------------------------------UPDATE: TIME_LIMIT_EXCEEDED: %d-----------------\n", memptr->termination_variables[2]);
      printf("\033[0m");// reset the color to the default

      release.sem_num = TIME_LIMIT_EXCEEDED;
      if ( semop(semid, &release, 1) == -1 ) {
        perror("semop -- producer -- release");
        exit(5);
      }
      continue;
    }
    sleep(processing_passenger_rate);
    
    acquire.sem_num = CURRENT_HALL_CAPACITY;
    if ( semop(semid, &acquire, 1) == -1 ) {
      perror("semop -- producer -- acquire2");
      exit(4);
    }
   
    if( (memptr->currently_in_hall < max_hall_threshold && flag == 0) || (flag == 1 && memptr->currently_in_hall < min_hall_threshold) ) { //If the current number of passenger is less than max_threshold, or that it is below the minimum_threshold after having reached the max, it can be handled
      flag = 0; 
      printf("\033[0;36m"); // set the color to cyan
      printf("Officer: Handeling Passenger {%d} in Officer {%d} with a Passport Status- Is-Expired: %d, Is-Forgotten: %d\n", 
            message.from_pid, queue_id, message.is_expired, message.forget_passport);
      printf("\033[0m");// reset the color to the default
      //2- If the passport is expired or forgotten, the passenger is denied access and variables updated accordingly
      if(message.is_expired == 1 || message.forget_passport == 1) {
        acquire.sem_num = DENIED_ACCESS;
        if ( semop(semid, &acquire, 1) == -1 ) {
          perror("semop -- producer -- acquire3");
          exit(4);
        }
        memptr->termination_variables[1]++; 
        if(memptr->termination_variables[1]>passed_max_deniend_passengers_number){
          printf("\033[5;31m"); // set the color to red
          printf("Terminating Due to Exceeding MAX_DENIED_PASSENGERS_NUMBER\n");
          printf("\033[0m");// reset the color to the default
          kill(getppid(),SIGQUIT);
        } 
        kill(message.from_pid, SIGTERM);     
        printf("\033[1;31m"); // set the color to red
        printf("\n------------------------------UPDATE: DENIED ACCESS: %d-----------------\n", memptr->termination_variables[1]);
        printf("\033[0m");// reset the color to the default
        
        release.sem_num = DENIED_ACCESS;
        
        if ( semop(semid, &release, 1) == -1 ) {
          perror("semop -- producer -- release");
          exit(5);
        } 
      } else {
        //3- If passenger can pass, variables are updated accordingly
        acquire.sem_num = GRANTED_ACCESS;
        if ( semop(semid, &acquire, 1) == -1 ) {
          perror("semop -- producer -- acquire4");
          exit(4);
        }
        memptr->currently_in_hall++; 
        memptr->leaving_passenger_ids[memptr->tail]= message.from_pid;
        memptr->tail = (memptr->tail + 1) % MAX;  
        memptr->termination_variables[0]++;  
        if(memptr->termination_variables[0]>passed_max_granted_passengers_number){
          printf("\033[5;31m"); // set the color to red
          printf("Terminating Due to Exceeding MAX_GRANTED_PASSENGERS_NUMBER\n");
          printf("\033[0m");// reset the color to the default
          kill(getppid(),SIGQUIT);
        } 
        printf("\033[1;32m"); // set the color to green
        printf("\n------------------------------UPDATE: GRANTED ACCESS: %d-----------------\n", memptr->termination_variables[0]);
        printf("\033[0m");// reset the color to the default
        printf("Passenger [%d] Waiting in the Hall...\n", message.from_pid);
      
        release.sem_num = GRANTED_ACCESS;
        
        if ( semop(semid, &release, 1) == -1 ) {
          perror("semop -- producer -- release");
          exit(5);
        }  
        
      } 
    } else if (memptr->currently_in_hall >= max_hall_threshold) {
      flag = 1; 
      msgsnd(queue_id, &message, sizeof(message), 0);
    } 
    printf("\033[1;37m"); // set the color to purpel
    printf("\n------------------------------UPDATE: CURRENT_HALL_CAPACITY: %d-----------------\n", memptr->currently_in_hall);
    printf("\033[0m");// reset the color to the default
    release.sem_num = CURRENT_HALL_CAPACITY;
    if ( semop(semid, &release, 1) == -1 ) {
      perror("semop -- producer -- release");
      exit(5);
    }
             
  }
	return 0;
}

int calculate_time_difference(int start_min, int start_sec, int end_min, int end_sec){
  return (end_min - start_min)*60 + (end_sec-start_sec);

}

char map(int type){
  if(type == 0)
    return 'P';
  else if (type == 1)
    return 'J';
  else if (type == 2)
    return 'F'; 
}


