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

//prototypes of functions to verify arguments
bool enough_arguments(int argc);
bool check_arguments(char *argv);
bool check_time(int time);

//prototype of a function to put a process to sleep
void rand_sleep(unsigned time);

//prototypes of functions for initialization
void init();
void destruct();

//prototypes of functions creating molecules
void oxygen(int id_o, unsigned wait_time, unsigned create_time);
void hydrogen(int id_h, unsigned wait_time, unsigned create_time);
void create_molecule(unsigned create_time);

//declarations of shared variables
int *line = NULL;
int *num_hydrogen = NULL;
int *num_oxygen = NULL;
int *id_m = NULL;

//declarations of semaphores
sem_t oxy;
sem_t hydro;

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

    init();

    (*num_oxygen) = 0;
    (*num_hydrogen) = 0;

    pid_t pid;

    for(int i = 0; i < number_oxygen; i++){
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"error forking oxygen");
        }
        if(pid == 0){
            oxygen(i+1, wait_time, create_time);
            exit(0);
        }
    }

    for(int i = 0; i < number_hydrogen; i++){
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"error forking hydrogen");
        }
        if(pid == 0){
            hydrogen(i+1, wait_time, create_time);
            exit(0);
        }
    }

    destruct();
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
void init(){
    MMAP(line);
    MMAP(num_oxygen);
    MMAP(num_hydrogen);
    MMAP(id_m)
}

//destroying shared memory and semaphores
void destruct(){
    UNMAP(line);
    UNMAP(num_oxygen);
    UNMAP(num_hydrogen);
    MMAP(id_m);
}

//function creating oxygen
void oxygen(int id_o, unsigned wait_time, unsigned create_time){
    printf("%d: O %d: started\n", ++(*line),  id_o);
    ++(*num_oxygen);
    rand_sleep(wait_time);
    printf("%d: O %d: going to queue\n", ++(*line), id_o);

    create_molecule(create_time);
}

//function creating oxygen
void hydrogen(int id_h, unsigned wait_time, unsigned create_time){
    printf("%d: H %d: started\n", ++(*line), id_h);
    ++(*num_hydrogen);
    rand_sleep(wait_time);
    printf("%d: H %d: going to queue\n", ++(*line), id_h);

    create_molecule(create_time);
}

//function creating individual molecules
void create_molecule(unsigned create_time){
    if((*num_oxygen) >= 1 && (*num_hydrogen) >= 2){
        ++(*id_m);
        int index = (*id_m);
        printf("%d: Creating molecule %d\n",++(*line), index);
        (*num_oxygen) -= 1;
        (*num_hydrogen) -= 2;

        rand_sleep(create_time);
        printf("%d: Molecule %d created\n",++(*line), index);
    }

    /*sem_post(&(hydro));
    sem_post(&(hydro));
    num_hydrogen -= 2;
    sem_post(&(oxy));
    num_oxygen -= 1;*/
}

//function that puts a process to sleep
void rand_sleep(unsigned time){
    usleep((rand()%(time+1)));
}
