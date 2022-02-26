
#include "local.h" 

//Needed functions
void read_configuration(char *file_name);
void bus_start_transfer(int );
void bus_ready_for_new_trip(int );
void officers_forking();
void passenger_forking();
void buses_forking();
void kill_all();
void sig_quit_catcher(int);

//Needed variables
int number_crossing_points_p; //p_officers
int number_crossing_points_j; //j_officers
int number_crossing_points_f; //f_officers
int number_officers;
int number_buses;
int max_impatient_passengers_number;
int max_denied_passengers_number;
int max_granted_passengers_number;
int max_hall_threshold;
int min_hall_threshold; 
int bus_capacity;
int bus_min_transfer_time;
int bus_max_transfer_time;
int termination_flag = 1;
int passengers_counter = 0; 
//Resources pids, stored dynamiclly to be freed later.
int *p_officers_queue_ids;
int *j_officers_queue_ids;
int *f_officers_queue_ids;
int *buses_ids;
int *officers_pids; 
int *passengers_pids; 

int main(int argc, char *argv[])
{
    printf("\033[0m");// reset the color to the default
    printf("\t \t Welcome to Our Smart Crossing Border Managment System\n\n");
    
    static struct  MEMORY memory;
    struct MEMORY *memptr;
    static ushort  start_val[5] = {1, 1, 1, 1, 1};
    int            semid, shmid, croaker;
    char          *shmptr;
    pid_t          p_id, c_id, pid = getpid();
    union semun    arg;
    memory.head = memory.tail = 0;
    termination_flag = 1;
    passengers_counter =0; 

    if (sigset(SIGQUIT, sig_quit_catcher) == SIG_ERR) {
      printf("\033[0;31m"); // set the color to red 
      perror("Sigset can not set SIGQUIT");
      printf("\033[0m");// reset the color to the default
      exit(-2);
    }

    if(argc != 2) {
        perror("Number of arguments must be 2!");
        exit(-1);
    }
    //Creating and attaching the shared memory
    if ( (shmid = shmget((int) pid, sizeof(memory),
                IPC_CREAT | 0600)) != -1 ) {
        
        if ( (shmptr = (char *) shmat(shmid, 0, 0)) == (char *) -1 ) {
        perror("shmptr -- parent -- attach");
        exit(1);
        }
        memcpy(shmptr, (char *) &memory, sizeof(memory));
        memptr = (struct MEMORY *) shmptr;
    }
    else {
        perror("shmid -- parent -- creation");
        exit(2);
    }
    //Creating and initiallizing the semaphores
    if ( (semid = semget((int) pid, 5,
                IPC_CREAT | 0666)) != -1 ) {
        arg.array = start_val;
        
        if ( semctl(semid, 0, SETALL, arg) == -1 ) {
        perror("semctl -- parent -- initialization");
        exit(3);
        }
    }
    else {
        perror("semget -- parent -- creation");
        exit(4);
    }
    passengers_pids = malloc(MAX * sizeof(int));
    srand(time(NULL));
    //Reading the configuration to get the user defined variables.
    read_configuration(argv[1]);  
    //forking officers to start the management
    officers_forking();
    //forking buses 
    buses_forking();
    //Keep forking passengers until one of the termination conditions is reached
    while(termination_flag == 1){ 
        passenger_forking(); 
        passengers_counter ++; 
        sleep(rand() % 3 + 1);
    }
    //Deacttach the memory and remove both memory and semaphores
    shmdt(shmptr);
    shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0); /* remove */
    semctl(semid, 0, IPC_RMID, 0);

 
  
    return(0);
}
void sig_quit_catcher(int sig_num){
    kill_all(); 
    printf("\033[5;0m");// reset the color to the default
    printf("\t \t \t Terminating... \n");
    termination_flag = 0;
}
//This fucntion used for reading and parsing the user-defined variables
void read_configuration(char *file_name) {
    char read[50];
    FILE *fptr;
    fptr = fopen(file_name,"r"); 
    if (fptr == NULL)
    {
        printf("\033[0;31m"); // set the color to red 
        perror("Error while opening the file.\n");
        printf("\033[0m");// reset the color to the default
        exit(-1);
    }
    while (fgets(read, sizeof(read), fptr)) {
        //printf("line: %s\n", read);
        char *token = strtok(read," ");
        //printf("token: %s\n", token);
        if(strcmp(token, "NUMBER_CROSSING_POINTS_P") == 0){
            number_crossing_points_p = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "NUMBER_CROSSING_POINTS_J") == 0){
            number_crossing_points_j = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "NUMBER_CROSSING_POINTS_F") == 0){
            number_crossing_points_f = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "NUMBER_OFFICERS") == 0){
            number_officers = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "NUMBER_BUSES") == 0){
            number_buses = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MAX_IMPATIENT_PASSENGERS_NUMBER" ) == 0){
            max_impatient_passengers_number = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MAX_DENIED_PASSENGERS_NUMBER" ) == 0){
            max_denied_passengers_number = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "MAX_GRANTED_PASSENGERS_NUMBER" ) == 0){
            max_granted_passengers_number = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "MAX_HALL_THRESHOLD" ) == 0){
            max_hall_threshold = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "MIN_HALL_THRESHOLD" ) == 0){
            min_hall_threshold = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "BUS_CAPACITY" ) == 0){
            bus_capacity = atoi(strtok(NULL,"\n"));
        }else if (strcmp(token, "BUS_MIN_TRANSFER_TIME" ) == 0){
            bus_min_transfer_time = atoi(strtok(NULL,"\n"));
        } else if (strcmp(token, "BUS_MAX_TRANSFER_TIME" ) == 0){
            bus_max_transfer_time = atoi(strtok(NULL,"\n"));
        } 
    }
    if(fclose(fptr)){
        exit(-1);
    }
}
//This function us used for forking the specified number of buses, with random transfer time for each bounded by the transfer min and max limits 
void buses_forking() {
    char pass_bus_capacity[5], pass_bus_transfer_time[5], pass_max_hall_threshold[10]; 
    int bus_transfer_time; 
    sprintf(pass_bus_capacity,"%d",bus_capacity);
    buses_ids = malloc(number_buses * sizeof(int ));
    int pid; 
    for(int i=0; i< number_buses; i++) {
        if ((pid = fork()) == -1) {
            printf("Error in Forking\n");
            exit (-1);
        }
        if ( pid != 0 ) {             //parent 
            *(buses_ids + i) = pid; 
        }
        else {                         //forked child
            bus_transfer_time = (rand() % (bus_max_transfer_time + 1)) + bus_min_transfer_time ; 
            sprintf(pass_bus_transfer_time,"%d",bus_transfer_time);
            sprintf(pass_max_hall_threshold,"%d",max_hall_threshold);
            execlp("./bus", "./bus",  pass_bus_capacity, pass_bus_transfer_time,pass_max_hall_threshold, (char *) NULL);
        }
    }

}
//This function is used to fork the needed number of officers, with each one, having a random type
void officers_forking(){
    int queue_id = 0;
    int type = 0;   //0 -> palestinian, 1 -> Jordanian, 2 -> forgein
    char pass_queue_id[30];  
    char pass_type[2];
    char pass_max_threshold[10];
    char pass_min_threshold[10];
    char pass_max_impatient_passenger_number[10]; 
    char pass_max_deniend_passengers_number[10];
    char pass_max_granted_passengers_number[10];
    int pid; 
    p_officers_queue_ids = malloc(number_crossing_points_p* sizeof(int));
    j_officers_queue_ids = malloc(number_crossing_points_j* sizeof(int));
    f_officers_queue_ids = malloc(number_crossing_points_f* sizeof(int));
    officers_pids = malloc((number_officers+1) * sizeof(int));
    int i = 0;
    //palestinian officers
    for (int p = 0; p < number_crossing_points_p; p++ ){
        queue_id = msgget(i, 0666 | IPC_CREAT); 
        *(p_officers_queue_ids+p) = queue_id;
        i++;
        type = 0;
        if ((pid = fork()) == -1) {
            printf("Error in Forking\n");
            exit (-1);
        }
        if ( pid != 0 ) {             //parent 
            *(officers_pids+i) = pid; 
        }
        else {                         //forked child
            sprintf(pass_queue_id,"%d",queue_id);
            sprintf(pass_type,"%d",type);
            sprintf(pass_max_threshold,"%d",max_hall_threshold);
            sprintf(pass_min_threshold,"%d",min_hall_threshold);
            sprintf(pass_max_impatient_passenger_number,"%d",max_impatient_passengers_number);
            sprintf(pass_max_deniend_passengers_number,"%d",max_denied_passengers_number);
            sprintf(pass_max_granted_passengers_number,"%d",max_granted_passengers_number);
            execlp("./crossing_border_officer", "./crossing_border_officer",  pass_type,pass_queue_id,pass_min_threshold, pass_max_threshold,pass_max_impatient_passenger_number,pass_max_deniend_passengers_number,pass_max_granted_passengers_number,(char *) NULL);
        }
    }
    //jordanian officers
    for (int j = 0; j < number_crossing_points_j; j++ ){
        queue_id = msgget(i, 0666 | IPC_CREAT); 
        *(j_officers_queue_ids+j) = queue_id;
        i++;
        type = 1;
        if ((pid = fork()) == -1) {
            printf("Error in Forking\n");
            exit (-1);
        }
        if ( pid != 0 ) {             //parent 
            *(officers_pids+i) = pid; 
        }
        else {                         //forked child
            sprintf(pass_queue_id,"%d",queue_id);
            sprintf(pass_type,"%d",type);
            sprintf(pass_max_threshold,"%d",max_hall_threshold);
            sprintf(pass_min_threshold,"%d",min_hall_threshold);
            sprintf(pass_max_impatient_passenger_number,"%d",max_impatient_passengers_number);
            sprintf(pass_max_deniend_passengers_number,"%d",max_denied_passengers_number);
            sprintf(pass_max_granted_passengers_number,"%d",max_granted_passengers_number);
            execlp("./crossing_border_officer", "./crossing_border_officer",  pass_type,pass_queue_id, pass_min_threshold, pass_max_threshold,pass_max_impatient_passenger_number,pass_max_deniend_passengers_number,pass_max_granted_passengers_number,(char *) NULL);
        }
    }  
    //forgein officers
    for (int f = 0; f < number_crossing_points_f; f++ ){
        queue_id = msgget(i, 0666 | IPC_CREAT); 
        *(f_officers_queue_ids+f) = queue_id;
        i++;
        type = 2;
        if ((pid = fork()) == -1) {
            printf("Error in Forking\n");
            exit (-1);
        }
        if ( pid != 0 ) {             //parent 
            *(officers_pids+i) = pid; 
        }
        else {                         //forked child
            sprintf(pass_queue_id,"%d",queue_id);
            sprintf(pass_type,"%d",type);
            sprintf(pass_max_threshold,"%d",max_hall_threshold);
            sprintf(pass_min_threshold,"%d",min_hall_threshold);
            sprintf(pass_max_impatient_passenger_number,"%d",max_impatient_passengers_number);
            sprintf(pass_max_deniend_passengers_number,"%d",max_denied_passengers_number);
            sprintf(pass_max_granted_passengers_number,"%d",max_granted_passengers_number);
            execlp("./crossing_border_officer", "./crossing_border_officer",  pass_type,pass_queue_id,pass_min_threshold, pass_max_threshold,pass_max_impatient_passenger_number,pass_max_deniend_passengers_number,pass_max_granted_passengers_number,(char *) NULL);
        }
    }  
}
//This function is used for forking differed types of passengers randomly.
void passenger_forking(){
    int random_number = 0;
    int pid;
    int type;
    int random_officer_number;
    char pass_officer_number[10];
    char pass_time_limit[20];
    char pass_type[2];
    int random_time_limit;
    if ((pid = fork()) == -1) {
        printf("fork failure ... getting out\n");
        exit (-1);
    }
    if ( pid != 0 ) {             //parent 
        *(passengers_pids+passengers_counter) = pid; 
    }
    else {                         //forked child
        //if the random number between 1-80, the passenger will be considered as Palestinian_Jordanian, else Forgein
        srand((unsigned int) getpid());
        random_number = rand() % 100 + 1;
        random_time_limit = rand()%3 + 3; //minimum time_limit is 2 mathalan
        if (random_number >= 1 && random_number <= 40 ){
            //setting type of passenger as PJ
            type = 0;
            sprintf(pass_type,"%d",type);
            sprintf(pass_time_limit,"%d",random_time_limit);
            random_officer_number = rand() % number_crossing_points_p;
            sprintf(pass_officer_number,"%d",*(p_officers_queue_ids+random_officer_number));
            execlp("./passenger", "./passenger", pass_type, pass_officer_number,pass_time_limit,(char *) NULL);
        } 
        else if (random_number > 40 && random_number <= 80){
            type = 1;
            sprintf(pass_type,"%d",type);
            sprintf(pass_time_limit,"%d",random_time_limit);
            random_officer_number = rand() % number_crossing_points_j;
            sprintf(pass_officer_number,"%d",*(j_officers_queue_ids+random_officer_number));
            execlp("./passenger", "./passenger", pass_type, pass_officer_number,pass_time_limit,(char *) NULL);
        }
        else {
            type = 2;
            sprintf(pass_type,"%d",type);
            sprintf(pass_time_limit,"%d",random_time_limit);
            random_officer_number = rand() % number_crossing_points_f;
            sprintf(pass_officer_number,"%d",*(f_officers_queue_ids+random_officer_number));
            execlp("./passenger", "./passenger", pass_type,pass_officer_number,pass_time_limit,(char *) NULL);
        }
    }
}
//This funtion is used in terminating, to free and kill
void kill_all() {
    //killing officers
    for(int i=1; i<=number_officers; i++) {
        kill(*(officers_pids+i), SIGTERM);
    }
    char *status;
    //killing the still alive passengers
    for(int i=0; i<passengers_counter; i++) {
        if(waitpid(*(passengers_pids+i), &status, WNOHANG) != 0){
        }
        else{
             kill(*(passengers_pids+i), SIGTERM);
        }  
    }
    // killing the buses
    for(int i=0; i<number_buses; i++) {
        //printf("id before killing is %d\n", *(officers_pids+i));
        kill(*(buses_ids+i), SIGTERM);
    }
    int queue_id;
    //Removing all message queues
    for(int p = 0; p < number_crossing_points_p; p ++){
	   // to destroy the message queue
       queue_id = *(p_officers_queue_ids+p);
	    msgctl(queue_id, IPC_RMID, NULL);
    }
    for(int j = 0; j < number_crossing_points_j; j ++){
	   // to destroy the message queue
       queue_id = *(j_officers_queue_ids+j);
	    msgctl(queue_id, IPC_RMID, NULL);
    }
    for(int f = 0; f < number_crossing_points_f; f ++){
	   // to destroy the message queue
       queue_id = *(f_officers_queue_ids+f);
	    msgctl(queue_id, IPC_RMID, NULL);
    }
    
    free(officers_pids);
    free(passengers_pids);
    free(buses_ids);
    free(p_officers_queue_ids);
    free(j_officers_queue_ids);
    free(f_officers_queue_ids);
}