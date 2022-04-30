#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <ctype.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>


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
    int max_mol;        //maximum number of molecules

    int oxy_idx;        //the oxygen we are working with
    int hydro_idx;      //the hydrogen we are working with
    int hydro2_idx;     //the hydrogen we are working with
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
void oxygen(int id_o, shared_t *shared, unsigned wait_time, unsigned create_time);
void hydrogen(int id_h, shared_t *shared, unsigned wait_time, unsigned create_time);
int max_molecules(int num_oxygen, int num_hydrogen);



int main(int argc, char *argv[]){

    if(!(enough_arguments(argc))){
        return 1;
    }
    for(int i = 1; i < NUM_ARG; i++){
        if(!(check_arguments(argv[i]))) {
            return 1;
        }
    }

    int number_oxygen = atoi(argv[1]);
    int number_hydrogen = atoi(argv[2]);
    int wait_time = atoi(argv[3]);
    int create_time = atoi(argv[4]);

    //checking hydrogen and oxygen
    if(!(check_positive(number_oxygen))){
        return 1;
    }
    if(!(check_positive(number_hydrogen))){
        return 1;
    }
    //checking both times
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

    shared->max_mol = max_molecules(number_oxygen, number_hydrogen);
    shared->max_atoms = number_oxygen + number_hydrogen;

    pid_t pid;


    for(int i = 0; i < number_hydrogen; i++){
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"error forking hydrogen");
        }
        if(pid == 0){
            hydrogen(i+1, shared, wait_time, create_time);
            exit(0);
        }
    }

    for(int i = 0; i < number_oxygen; i++){
        pid = fork();
        if(pid == -1){
            fprintf(stderr,"error forking oxygen");
        }
        if(pid == 0){
            oxygen(i+1, shared, wait_time, create_time);
            exit(0);
        }
    }

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
    shared->num_oxygen = 0;
    shared->num_hydrogen = 0;
    shared->oxy_idx = 0;
    shared->hydro_idx = 0;
    shared->id_m = 1;
    shared->queue = 0;

    shared->file = fopen("proj2.out", "w");

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
            if(shared->queue == shared->max_atoms){
                for(int i = 0; i < shared->max_atoms; i++){
                    sem_post(&(shared->sem_queue));
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
void oxygen(int id_o, shared_t *shared, unsigned wait_time, unsigned create_time){
    sem_wait(&(shared->out));
    print_mess(shared, '0', id_o, 'O');
    sem_post(&(shared->out));

    rand_sleep(wait_time);

    sem_wait(&(shared->out));
    print_mess(shared, '1', id_o, 'O');
    sem_post(&(shared->out));

    sem_wait(&(shared->mutex));

    if(shared->max_mol < shared->id_m){
        if(shared->queue != shared->max_atoms){
            sem_wait(&(shared->sem_queue));
        }
        print_mess(shared, '4', id_o, 'O');

        sem_post(&shared->mutex);
        sem_post(&shared->mutex3);
        sem_post(&shared->mutex3);
        exit(EXIT_SUCCESS);
    }

    sem_wait(&(shared->oxy));
    sem_wait(&(shared->oxy));

    sem_post(&(shared->hydro));
    sem_post(&(shared->hydro));


    sem_wait(&(shared->out));
    print_mess(shared, '2', id_o, 'O');
    sem_post(&(shared->out));

    sem_wait(&(shared->mutex2));
    sem_wait(&(shared->mutex2));

    sem_post(&(shared->mutex3));
    sem_post(&(shared->mutex3));


    rand_sleep(create_time);

    sem_wait(&(shared->out));
    print_mess(shared, '3', id_o, 'O');
    sem_post(&(shared->out));

    sem_wait(&(shared->mol));
    sem_wait(&(shared->mol));

    sem_post(&(shared->mutex4));
    sem_post(&(shared->mutex4));

    ++shared->id_m;

    sem_post(&(shared->mutex));
    exit(EXIT_SUCCESS);
}

//function creating oxygen
void hydrogen(int id_h, shared_t *shared, unsigned wait_time, unsigned create_time) {
    sem_wait(&(shared->out));
    print_mess(shared, '0', id_h, 'H');
    sem_post(&(shared->out));

    rand_sleep(wait_time);

    sem_wait(&(shared->out));
    print_mess(shared, '1', id_h, 'H');
    sem_post(&(shared->out));

    int idx_h = id_h;

    sem_wait(&(shared->hydro_mutex));

    //if all the available molecules have been created we can no longer create any
    if (shared->max_mol < shared->id_m) {
        if(shared->queue != shared->max_atoms){
            sem_wait(&(shared->sem_queue));
        }
        print_mess(shared, '4', id_h, 'H');
        sem_post(&shared->mutex2);
        sem_post(&(shared->hydro_mutex));
        exit(EXIT_SUCCESS);
    }

    sem_post(&(shared->oxy));

    sem_wait(&(shared->hydro));

    sem_wait(&(shared->out));
    print_mess(shared, '2', idx_h, 'H');
    sem_post(&(shared->out));

    sem_post(&(shared->mutex2));

    sem_wait(&(shared->mutex3));

    rand_sleep(create_time);

    sem_wait(&(shared->out));
    print_mess(shared, '3', idx_h, 'H');
    sem_post(&(shared->out));

    sem_post(&(shared->mol));
    sem_wait(&(shared->mutex4));

    sem_post(&(shared->hydro_mutex));

    exit(EXIT_SUCCESS);
}

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