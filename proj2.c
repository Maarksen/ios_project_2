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

    int id_m;           //the id of molecule to keep track
    int max_mol;        //maximum number of molecules
    int max_atoms;      //all atoms that should go into queue
    int queue;          //number of atoms already in queue

    FILE* file;

    //declarations of semaphores
    sem_t oxy;
    sem_t hydro;
    sem_t mol;
    sem_t mutex;
    sem_t mutex2;
    sem_t mutex3;
    sem_t mutex4;
    sem_t hydro_mutex;
    sem_t sem_queue;
    sem_t out;

}shared_t;

//prototypes of functions to verify arguments
bool enough_arguments(int argc);
bool check_arguments(char *argv);
bool check_positive(int arg);
bool check_time(int time);

//prototype of a function to put a process to sleep
void rand_sleep(int time);

//prototype of function to print out a message
void print_mess(shared_t *shared, int mess_id, int id_a, char type);

//prototypes of functions for initialization
void init(shared_t *shared);
void destruct(shared_t *shared);

//prototypes of functions creating molecules
void oxygen(int id_o, shared_t *shared, int wait_time, int create_time);
void hydrogen(int id_h, shared_t *shared, int wait_time, int create_time);
int max_molecules(int num_oxygen, int num_hydrogen);



int main(int argc, char *argv[]){

    //checking whether there is enough arguments
    if(!(enough_arguments(argc))){
        return 1;
    }
    //checking if all the arguments are only numbers
    for(int i = 1; i < NUM_ARG; i++){
        if(!(check_arguments(argv[i]))) {
            return 1;
        }
    }

    int NO = atoi(argv[1]);     //number of oxygen
    int NH = atoi(argv[2]);     //number of hydrogen
    int TI = atoi(argv[3]);     //wait_time - max time needed to wait before an atom goes to queue
    int TB = atoi(argv[4]);     //create_time - max time needed to wait before molecule is created

    //checking hydrogen and oxygen
    if(!(check_positive(NO))){
        return 1;
    }
    if(!(check_positive(NH))){
        return 1;
    }
    //checking both times
    if(!(check_time(TI))){
        return 1;
    }
    if(!(check_time(TB))){
        return 1;
    }


    //creating and initializing shared memory
    shared_t *shared;
    MMAP(shared)
    init(shared);

    shared->max_mol = max_molecules(NO, NH);
    shared->max_atoms = NO + NH;

    pid_t pid;
    //the hydrogen forking process
    for(int i = 0; i < NH; i++){
        pid = fork();
        if(pid == -1){      //if the pid is negative the forking process failed
            fprintf(stderr,"error forking hydrogen\n");
        }
        if(pid == 0){       //pid == 0 means that we are on the child process
            hydrogen(i+1, shared, TI, TB);
            exit(0);
        }
    }
    //the oxygen forking process
    for(int i = 0; i < NO; i++){
        pid = fork();
        if(pid == -1){      //if the pid is negative the forking process failed
            fprintf(stderr,"error forking oxygen\n");
        }
        if(pid == 0){       //pid == 0 means that we are on the child process
            oxygen(i+1, shared, TI, TB);
            exit(0);
        }
    }

    //deactivating and destroying the memory
    destruct(shared);
    UNMAP(shared)
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

//checking whether all arguments are positive
bool check_positive(int arg){
    if(arg <= 0){
        fprintf(stderr, "invalid argument\n");
        return false;
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
    //initializing variables
    shared->num_oxygen = 0;
    shared->num_hydrogen = 0;
    shared->id_m = 1;
    shared->queue = 0;

    //specifying the file
    shared->file = fopen("proj2.out", "w");

    //initializing the semaphores
    sem_init(&(shared->oxy), 1, 0);
    sem_init(&(shared->hydro), 1, 0);
    sem_init(&(shared->mol), 1, 0);
    sem_init(&(shared->mutex), 1, 1);
    sem_init(&(shared->mutex2), 1, 0);
    sem_init(&(shared->mutex3), 1, 0);
    sem_init(&(shared->mutex4), 1, 0);
    sem_init(&(shared->hydro_mutex), 1, 2);
    sem_init(&(shared->sem_queue), 1, 0);
    sem_init(&(shared->out), 1, 1);
}

//destroying shared memory and semaphores
void destruct(shared_t *shared){
    //destroying the semaphores
    sem_destroy(&(shared->oxy));
    sem_destroy(&(shared->hydro));
    sem_destroy(&(shared->mol));
    sem_destroy(&(shared->mutex));
    sem_destroy(&(shared->mutex2));
    sem_destroy(&(shared->mutex3));
    sem_destroy(&(shared->mutex4));
    sem_destroy(&(shared->hydro_mutex));
    sem_destroy(&(shared->sem_queue));
    sem_destroy(&(shared->out));

    //closing the file
    fclose(shared->file);
}

//function to printf messages
void print_mess(shared_t *shared, int mess_id, int id_a, char type){
    switch (mess_id){
        case '0':
            fprintf(shared->file, "%d: %c %d: started\n", ++shared->line, type,  id_a);
            fflush(shared->file);
            break;

        case '1':
            fprintf(shared->file, "%d: %c %d: going to queue\n", ++shared->line, type, id_a);
            shared->queue++;
            if(shared->queue == shared->max_atoms){             //if all atoms are already in queue
                for(int i = 0; i < shared->max_atoms; i++){
                    sem_post(&(shared->sem_queue));         //posts semaphore for every atom so all could pass
                }
            }
            fflush(shared->file);
            break;

        case '2':
            fprintf(shared->file, "%d: %c %d: creating molecule %d\n",++shared->line, type, id_a, shared->id_m);
            fflush(shared->file);
            break;

        case '3':
            fprintf(shared->file, "%d: %c %d: molecule %d created\n",++shared->line, type, id_a, shared->id_m);
            fflush(shared->file);
            break;

        case '4':
            if(type == 'O') {
                fprintf(shared->file, "%d: %c %d: not enough H\n", ++shared->line, type, id_a);
                fflush(shared->file);
            }
            else if(type == 'H'){
                fprintf(shared->file, "%d: %c %d: not enough O or H\n", ++shared->line, type, id_a);
                fflush(shared->file);
            }
            break;

        default:
            fprintf(stderr, "Wrong message ID input");
            break;
    }
}

//function creating oxygen
void oxygen(int id_o, shared_t *shared, int wait_time, int create_time){
    sem_wait(&(shared->out));
    print_mess(shared, '0', id_o, 'O');     //prints out oxygen started
    sem_post(&(shared->out));

    //waits random time from 0 to TI
    rand_sleep(wait_time);

    sem_wait(&(shared->out));
    print_mess(shared, '1', id_o, 'O');     //prints out oxygen in queue
    sem_post(&(shared->out));

    //waits, only one oxygen process can print molecule at a time
    sem_wait(&(shared->mutex));

    //if all molecules are already made
    if(shared->max_mol < shared->id_m){
        if(shared->queue != shared->max_atoms){
            sem_wait(&(shared->sem_queue));    //waits until all atoms are in queue
        }
        print_mess(shared, '4', id_o, 'O');     //prints not enough H

        //lets another oxygen pass
        sem_post(&shared->mutex);
        //lets two hydrogens pass
        sem_post(&shared->mutex3);
        sem_post(&shared->mutex3);
        exit(EXIT_SUCCESS);
    }

    //waits for two hydrogens
    sem_wait(&(shared->oxy));       //semaphores make sure that
    sem_wait(&(shared->oxy));       //one atom can not print creating before all atoms are in queue
    //lets two hydrogens pass
    sem_post(&(shared->hydro));
    sem_post(&(shared->hydro));


    sem_wait(&(shared->out));
    print_mess(shared, '2', id_o, 'O');     //prints molecule creating
    sem_post(&(shared->out));

    //waits for two hydrogens
    sem_wait(&(shared->mutex2));    //semaphores make sure that
    sem_wait(&(shared->mutex2));    //one atom can not print created before all finish creating
    //lets two hydrogens pass
    sem_post(&(shared->mutex3));
    sem_post(&(shared->mutex3));

    //waits random time from 0 to TB
    rand_sleep(create_time);

    sem_wait(&(shared->out));
    print_mess(shared, '3', id_o, 'O');     //prints molecule created
    sem_post(&(shared->out));

    //semaphores make sure that all molecule processes are finished
    sem_wait(&(shared->mol));
    sem_wait(&(shared->mol));

    sem_post(&(shared->mutex4));
    sem_post(&(shared->mutex4));

    ++shared->id_m;     //increments number of molecules

    //lets another oxygen process go
    sem_post(&(shared->mutex));
    exit(EXIT_SUCCESS);
}

//function creating hydrogen
void hydrogen(int id_h, shared_t *shared,int wait_time, int create_time) {
    sem_wait(&(shared->out));
    print_mess(shared, '0', id_h, 'H');     //prints out hydrogen started
    sem_post(&(shared->out));

    //waits random time from 0 to TI
    rand_sleep(wait_time);

    sem_wait(&(shared->out));
    print_mess(shared, '1', id_h, 'H');     //prints out hydrogen in queue
    sem_post(&(shared->out));

    //saves the id of hydrogen into a variable
    //to ensure that the hydrogen id stays the same for the whole process
    int idx_h = id_h;

    sem_wait(&(shared->hydro_mutex));

    //if all the available molecules have been created we can no longer create any
    if (shared->max_mol < shared->id_m) {
        if(shared->queue != shared->max_atoms){
            sem_wait(&(shared->sem_queue));     //we wait for all the atoms to go to queue before printing
        }
        print_mess(shared, '4', id_h, 'H');     // //prints not enough O or H
        sem_post(&shared->mutex2);
        sem_post(&(shared->hydro_mutex));
        exit(EXIT_SUCCESS);
    }
    //lets oxygen pass
    sem_post(&(shared->oxy));
    //waits for oxygen to lets this process continue
    sem_wait(&(shared->hydro));

    sem_wait(&(shared->out));
    print_mess(shared, '2', idx_h, 'H');      //prints molecule creating
    sem_post(&(shared->out));

    //lets oxygen continue
    sem_post(&(shared->mutex2));
    //waits for oxygen to let this process go
    sem_wait(&(shared->mutex3));

    //waits random time from 0 to TB
    rand_sleep(create_time);

    sem_wait(&(shared->out));
    print_mess(shared, '3', idx_h, 'H');      //prints molecule created
    sem_post(&(shared->out));

    //semaphores make sure that all processes from one molecule
    //finish in the same time
    sem_post(&(shared->mol));
    sem_wait(&(shared->mutex4));

    //lets another hydrogen process go
    sem_post(&(shared->hydro_mutex));

    exit(EXIT_SUCCESS);
}

//calculates the maximum number of atoms
int max_molecules(int num_oxygen, int num_hydrogen){
    int max_mol;
    if(num_oxygen > num_hydrogen/2){
        max_mol = num_hydrogen/2;
    }
    else{
        max_mol = num_oxygen;
    }
    return max_mol;
}

//function that puts a process to sleep
void rand_sleep(int time){
    usleep((rand()%(time+1)));
}
