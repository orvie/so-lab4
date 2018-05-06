/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: orvie
 *
 * Created on May 2, 2018, 9:10 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <semaphore.h>

/*
 * Shared variables
 */
#define MAX_BUFFER 100
#define DEBUG

typedef struct {
    int pid;
    char name[MAX_BUFFER];
    char state[MAX_BUFFER];
    char vmsize[MAX_BUFFER];
    char vmdata[MAX_BUFFER];
    char vmexe[MAX_BUFFER];
    char vmstk[MAX_BUFFER];
    int voluntary_ctxt_switches;
    int nonvoluntary_ctxt_switches;
} proc_info;

typedef struct {
    int pid;
    char min_vm[MAX_BUFFER];
    char max_vm[MAX_BUFFER];


} proc_summary;

// initialize with number of process
sem_t empty;
// start with cero
sem_t full;
// start with cero too
sem_t stats;

// Control race condition for multiple producers
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// which index buffer to fill with process info
int idx_buffer = 0;
// Control which process info to print 
int rem_to_print = 0;
// buffer size is equal than amount of process
unsigned int buffer_size = 0;
// Control when number of elements is equal to buffer size
int total_elements = 0;
// Shared resource between processes
proc_info *proc_buffer;

void *load_info(void *arg);
void *calc_stats();
void *print_info(void *arg);

/*
 * 
 */
int main(int argc, char** argv) {

    //Check arguments
    if (argc < 2) {
        printf("Error, wrong arguments\n");
        exit(1);
    }
    //Find number of possible processes
    int n_procs = argc - 1;
    //Assign process amount 
    //Memory allocation 
    proc_buffer = (proc_info *) malloc(sizeof (proc_info) * n_procs);

    buffer_size = n_procs;

    //Create threads, semaphores and mutex
    if(sem_init(&empty, 0, buffer_size) != 0){
        printf("ERROR\n");
        exit(1);
    }
    if(sem_init(&full, 0, 0) != 0){
        printf("ERROR\n");
        exit(1);
    }
    pthread_t my_threads[n_procs + 1];

    int i;
    for (i = 0; i < n_procs; i++) {
        int pid = atoi(argv[i + 1]);
        pthread_create(&my_threads[i], NULL, &load_info, &pid);

    }

    pthread_create(&my_threads[n_procs], NULL, &print_info, NULL);

    //Only wait for the last thread which prints values
    pthread_join(my_threads[n_procs], NULL);
    
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    
    
    return (EXIT_SUCCESS);
}

void* load_info(void* arg) {
    
    int _pid = *((int*) arg); 
    char *saveptr1, *saveptr2, *saveptr3;
    proc_info myinfo;
    FILE *fpstatus;
    char buffer[MAX_BUFFER];
    char path[MAX_BUFFER];
    char* token;

    sprintf(path, "/proc/%d/status", _pid);
    fpstatus = fopen(path, "r");
    assert(fpstatus != NULL);
    #ifdef DEBUG
    printf("%s\n", path);
    #endif // DEBUG
    myinfo.pid = _pid;
    while (fgets(buffer, MAX_BUFFER, fpstatus)) {
        token = strtok_r(buffer, ":\t", &saveptr1);
        if (strstr(token, "Name")) {
            token = strtok_r(NULL, ":\t", &saveptr2);
            #ifdef  DEBUG
            printf("%s\n", token);
            #endif // DEBUG
            strcpy(myinfo.name, token);
        } else if (strstr(token, "VmSize")) {
            token = strtok_r(NULL, ":\t", &saveptr3);
            strcpy(myinfo.vmsize, token);
        } 
#ifdef  DEBUG
        printf("%s\n", token);
#endif
    }

    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    if (total_elements == buffer_size) {
        exit(0);
    }
    proc_buffer[idx_buffer] = myinfo;
    idx_buffer = (idx_buffer + 1) % buffer_size;
    total_elements ++;
    pthread_mutex_unlock(&mutex);
    sem_post(&full);
    
  
    fclose(fpstatus);
}

void *print_info(void *arg){
    
    while (1) {
        sem_wait(&full);
        
    }

    
    
    
}