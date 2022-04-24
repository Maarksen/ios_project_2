#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <semaphore.h>

#define NUM_ARG 5 //number of intended arguments
#define MAX_TIME 1000 //maximum number that time can be set to

//prototypes of functions to verify arguments
bool enough_arguments(int argc);
bool check_arguments(char *argv);
bool check_time(int time);

//prototype of a function to put a process to sleep
void rand_sleep(int time);


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

//function that puts a process to sleep
void rand_sleep(int time){
    usleep((rand()%(time+1)));
}
