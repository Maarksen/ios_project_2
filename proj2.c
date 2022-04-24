#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <ctype.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>

#define NUM_ARG 5 //number of intended arguments
#define MAX_TIME 1000 //maximum number that time can be set to

//prototypes of functions to verify arguments
bool enough_arguments(int argc);
bool check_arguments(char *argv);
bool check_time(int time);

//prototype of a function to put a process to sleep
void rand_sleep(unsigned time);

//prototypes of functions creating molecules
int oxygen(int id_o, unsigned wait_time);
int hydrogen(int id_h, unsigned wait_time);
void create_molecule(sem_t oxygen, sem_t hydrogen, unsigned num_hydrogen, unsigned num_oxygen);


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

    sem_t oxy;
    sem_t hydro;

    unsigned num_hydrogen;
    unsigned num_oxygen;

    if(!(check_time(wait_time))){
        return 1;
    }
    if(!(check_time(create_time))){
        return 1;
    }

    pid_t pid;

    for(int i = 0; i < number_oxygen; i++){
        pid = fork();
        if(pid == -1){
            printf(stderr,"error forking oxygen");
        }
        if(pid == 0){
            oxygen(i+1, wait_time);
            exit(0);
        }
    }

    for(int i = 0; i < number_hydrogen; i++){
        pid = fork();
        if(pid == -1){
            printf(stderr,"error forking hydrogen");
        }
        if(pid == 0){
            hydrogen(i+1, wait_time);
            exit(0);
        }
    }

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

//function creating oxygen
int oxygen(int id_o, unsigned wait_time){
    printf("O %d: started\n", id_o);
    rand_sleep(wait_time);
    printf("O %d: going to queue\n", id_o);

    return 0;
}

//function creating oxygen
int hydrogen(int id_h, unsigned wait_time){
    printf("H %d: started\n", id_h);
    rand_sleep(wait_time);
    printf("H %d: going to queue\n", id_h);

    return 0;
}

//function creating individual molecules
void create_molecule(sem_t oxy, sem_t hydro, unsigned num_hydrogen, unsigned num_oxygen){
    sem_post(&(hydro));
    sem_post(&(hydro));
    num_hydrogen -= 2;
    sem_post(&(oxy));
    num_oxygen -= 1;
}

//function that puts a process to sleep
void rand_sleep(unsigned time){
    usleep((rand()%(time+1)));
}
