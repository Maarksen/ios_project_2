#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <ctype.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <fcntl.h>

#define NUM_ARG 5       //number of intended arguments
#define MAX_TIME 1000   //maximum number that time can be set to

//allocating shared memory
#define MMAP(pointer){(pointer) = mmap(NULL, sizeof(*(pointer)), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);}
#define UNMAP(pointer) {munmap((pointer), sizeof((pointer)));}

//declarations of shared variables
typedef struct shared {
    int line;           //line counter
    int num_oxygen;     //the overall number of oxygen
    int num_hydrogen;   //the overall number of hydrogen

    int curr_oxygen;    //current number of oxygen
    int curr_hydrogen;  //current number of hydrogen

    int id_m;           //the id of molecule to keep track

    int oxy_idx;        //the oxygen we are working with
    int hydro_idx;      //the hydrogen we are working with

    //declarations of semaphores
    sem_t oxy;
    sem_t hydro;
    sem_t mol;
    sem_t mutex;
    sem_t out;

}shared_t;

//prototypes of functions to verify arguments
bool enough_arguments(int argc);
bool check_arguments(char *argv);
bool check_time(int time);

//prototype of a function to put a process to sleep
void rand_sleep(unsigned time);

//prototypes of functions for initialization
void init(shared_t *shared);
void destruct(shared_t *shared);

//prototypes of functions creating molecules
void oxygen(int id_o, shared_t *shared, unsigned wait_time, unsigned create_time);
void hydrogen(int id_h, shared_t *shared, unsigned wait_time, unsigned create_time);
void create_molecule(shared_t *shared);



int main(int argc, char *argv[]){

    if(!(enough_arguments(argc))){
        return 1;
    }
    for(int i = 1; i < NUM_ARG; i++){
        if(!(check_arguments(argv[i]))) {
            return 1;
        }
    }

    unsigned number_oxygen = atoi(argv[1]);
    unsigned number_hydrogen = atoi(argv[2]);
    unsigned wait_time = atoi(argv[3]);
    unsigned create_time = atoi(argv[4]);

    if(!(check_time(wait_time))){
        return 1;
    }
    if(!(check_time(create_time))){
        return 1;
    }

    //creating and initializing shared memory
    shared_t *shared;
    MMAP(shared)
    init(shared);

    pid_t pid;

    for(unsigned i = 0; i < number_oxygen; i++){
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"error forking oxygen");
        }
        if(pid == 0){
            oxygen(i+1, shared, wait_time, create_time);
            exit(0);
        }
    }

    for(unsigned i = 0; i < number_hydrogen; i++){
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"error forking hydrogen");
        }
        if(pid == 0){
            hydrogen(i+1, shared, wait_time, create_time);
            exit(0);
        }
    }

    destruct(shared);
    return 0;
}

//function that checks whether enough arguments were input
bool enough_arguments(int argc){
    if(argc < NUM_ARG){
        fprintf(stderr,"not enough arguments\n");
        return false;
    }
    else if(argc > NUM_ARG){
        fprintf(stderr,"too many arguments\n");
        return false;
    }
    return true;
}

//function that checks whether arguments are numbers
bool check_arguments(char *argv) {
    for (int i = 0; argv[i]!= '\0'; i++){
        if (!(isdigit(argv[i]))) {
            fprintf(stderr, "invalid argument\n");
            return false;
        }
    }
    return true;
}

//function that checks whether the time was set correctly
bool check_time(int time){
    if(!(time >= 0 && time <= MAX_TIME)){
        fprintf(stderr, "invalid argument\n");
        return false;
    }
    return true;
}

//initializing shared memory and semaphores
void init(shared_t *shared){
    shared->num_oxygen = 0;
    shared->num_hydrogen = 0;
    shared->oxy_idx = 0;
    shared->hydro_idx = 0;

    sem_init(&(shared->oxy), 1, 0);
    sem_init(&(shared->hydro), 1, 0);
    sem_init(&(shared->mol), 1, 1);
    sem_init(&(shared->mol), 1, 1);
    sem_init(&(shared->mutex), 1, 1);
    sem_init(&(shared->out), 1, 1);
}

//destroying shared memory and semaphores
void destruct(shared_t *shared){
    sem_destroy(&(shared->oxy));
    sem_destroy(&(shared->hydro));
    sem_destroy(&(shared->mol));
    sem_destroy(&(shared->mutex));
    sem_destroy(&(shared->out));

}

//function creating oxygen
void oxygen(int id_o, shared_t *shared, unsigned wait_time, unsigned create_time){
    sem_wait(&(shared->out));
    printf("%d: O %d: started\n", ++shared->line,  id_o);
    sem_post(&(shared->out));
    rand_sleep(wait_time);
    sem_wait(&(shared->out));
    printf("%d: O %d: going to queue\n", ++shared->line, id_o);
    sem_post(&(shared->out));

    sem_wait(&(shared->mutex));
    ++shared->num_oxygen;

    if(shared->num_oxygen >= 1 && shared->num_hydrogen >= 2) {
        create_molecule(shared);
    }
    else{
        sem_post(&(shared->mutex));
    }

    sem_wait(&(shared->oxy));

    ++shared->id_m;
    int ID = shared->id_m;
    ++shared->hydro_idx;
    int IDX = shared->hydro_idx;
    sem_wait(&(shared->out));
    printf("%d: H %d: Creating molecule %d\n",++shared->line, IDX, ID);
    sem_post(&(shared->out));
    rand_sleep(create_time);
    sem_wait(&(shared->out));
    printf("%d: H %d: Molecule %d created\n",++shared->line, IDX, ID);
    sem_post(&(shared->out));


}

//function creating oxygen
void hydrogen(int id_h, shared_t *shared, unsigned wait_time, unsigned create_time){
    sem_wait(&(shared->out));
    printf("%d: H %d: started\n", ++shared->line, id_h);
    sem_post(&(shared->out));
    rand_sleep(wait_time);
    sem_wait(&(shared->out));
    printf("%d: H %d: going to queue\n", ++shared->line, id_h);
    sem_post(&(shared->out));

    sem_wait(&(shared->mutex));
    ++shared->num_hydrogen;

    if(shared->num_oxygen >= 1 && shared->num_hydrogen >= 2) {
        create_molecule(shared);
    }
    else{
        sem_post(&(shared->mutex));
    }
    sem_wait(&(shared->hydro));

    ++shared->id_m;
    int ID = shared->id_m;
    ++shared->oxy_idx;
    int IDX = shared->oxy_idx;
    sem_wait(&(shared->out));
    printf("%d: O %d: Creating molecule %d\n",++shared->line, IDX, ID);
    sem_post(&(shared->out));
    rand_sleep(create_time);
    sem_wait(&(shared->out));
    printf("%d: O %d: Molecule %d created\n",++shared->line, IDX, ID);
    sem_post(&(shared->out));
}

//function creating individual molecules
void create_molecule( shared_t *shared){

    sem_post(&(shared->hydro));
    sem_post(&(shared->hydro));
    shared->num_hydrogen -= 2;
    shared->curr_hydrogen -= 2;

    sem_post(&(shared->oxy));
    shared->num_oxygen -= 1;
    shared->curr_oxygen -= 1;
}

//function that puts a process to sleep
void rand_sleep(unsigned time){
    usleep((rand()%(time+1)));
}
