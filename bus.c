#include "local.h"

void bus_start_working(int );

int capacity; 
int transfer_time;
int max_hall_threshold;
int flag = 0; 


int main(int argc, char *argv[ ])
{

    if(argc != 4) {
    perror("Number of arguments must be 3!");
    exit(-1);
    } 
    flag =0; 

    capacity = atoi(argv[1]);
    int temp_capacity = capacity; 
    transfer_time = atoi(argv[2]);
    max_hall_threshold = atoi(argv[3]);

    //get the memory and semaphore. 
    int            semid, shmid;
    pid_t          ppid = getppid();
    char          *shmptr;
    struct MEMORY *memptr;
    //Accessing the memory
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
    //Accessing the semaphore
    if ( (semid = semget((int) ppid, 5, 0)) == -1 ) {
        perror("semget -- producer -- access 434343");
        exit(3);
    }
    while(1){
        //a BUS_TURN semaphore is used to synchronize buses work and to allow only one bus to be working until its full capacity
        acquire.sem_num = BUS_TURN;
        if ( semop(semid, &acquire, 1) == -1 ) {
        perror("semop -- bus -- acquire");
        exit(4);
        }
        while(temp_capacity > 0 ) {
            //AS long as there is capacity in the bus, passengers will be getting out of hall and entering the bus
            acquire.sem_num = CURRENT_HALL_CAPACITY;
            if ( semop(semid, &acquire, 1) == -1 ) {
            perror("semop -- producer -- acquire");
            exit(4);
            }
            if(memptr->currently_in_hall > 0){
                memptr->currently_in_hall --; 
                kill(memptr->leaving_passenger_ids[memptr->head], SIGTERM);
                printf("\033[0;35m"); // set the color to purple
                printf("BUS: Adding Passenger [%d] to Bus [%d]\n", memptr->leaving_passenger_ids[memptr->head], getpid()); 
                printf("\033[0m");// reset the color to the default
                printf("\033[1;37m"); // set the color to purpel
                printf("\n------------------------------UPDATE: CURRENT_HALL_CAPACITY: %d-----------------\n", memptr->currently_in_hall);
                printf("\033[0m");// reset the color to the default
                memptr->head = (memptr->head + 1) % MAX;
                temp_capacity --; 
            }
            release.sem_num = CURRENT_HALL_CAPACITY;
            if ( semop(semid, &release, 1) == -1 ) {
                perror("semop -- OFFICER_HALL at bus -- release");
                exit(5);
            } 
        }
        temp_capacity = capacity; 
        release.sem_num = BUS_TURN; 
        if ( semop(semid, &release, 1) == -1 ) {
        perror("semop -- bus -- release");
        exit(5);
        }
        printf("\033[0;35m"); // set the color to purple
        printf("Bus [%d] Will Start a New Trip _Full Capacity_\n", getpid());
        printf("\033[0m");// reset the color to the default
        sleep(transfer_time); 
        printf("\033[0;35m"); // set the color to purple
        printf("Bus [%d] Done This Trip.\n", getpid());
        printf("\033[0m");// reset the color to the default
    }

    return 0;
}

