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
//#define DEBUG

typedef struct {
    int pid;
    char name[MAX_BUFFER];
    char vmsize[MAX_BUFFER];
} proc_info;

typedef struct {
    int min_pid;
    int max_pid;
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
// Shared resource between processes
proc_info *proc_buffer;

void *load_info(void *arg);
void *print_info(void *arg);

/*
 * 
 */
int main(int argc, char** argv) {
    
/*
    argc = 3;
    argv[0] = "aaa";
    argv[1] = "1160";
    argv[2] = "1163";
*/

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
    int procs[buffer_size];
    //Create threads, semaphores and mutex
    if(sem_init(&empty, 0, buffer_size) != 0){
        printf("ERROR\n");
        exit(1);
    }
    if(sem_init(&full, 0, 0) != 0){
        printf("ERROR\n");
        exit(1);
    }
    pthread_t my_threads[(n_procs + 1)];
    //The last one will print info
    pthread_create(&my_threads[n_procs], NULL, &print_info, NULL);
    
    int i;
    for (i = 0; i < n_procs; i++) {
        procs[i] = atoi(argv[i + 1]);
        printf("pid enviado: %d \n", procs[i]);
        pthread_create(&my_threads[i], NULL, &load_info, &procs[i]);

    }
    printf("sale del for%s\n", "");
    
    int j;
    for (j = 0; j < n_procs + 1; j++) {
        pthread_join(my_threads[j], NULL);
    }
    //pthread_join(my_threads[n_procs], NULL);
    
    //free
    
    free(proc_buffer);
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);
    
    
    return (EXIT_SUCCESS);
}

void* load_info(void* arg) {
    
    int _pid = *((int*) arg);
    char *saveptr1;
    proc_info myinfo;
    FILE *fpstatus;
    char buffer[MAX_BUFFER];
    char path[MAX_BUFFER];
    char* token;

    sprintf(path, "/proc/%d/status", _pid);
    printf("%s\n", path);
    fpstatus = fopen(path, "r");
    assert(fpstatus != NULL);
    myinfo.pid = _pid;
    
    while (fgets(buffer, MAX_BUFFER, fpstatus)) {
        token = strtok_r(buffer, ":\t", &saveptr1);
        if (strstr(token, "Name")) {
            token = strtok_r(NULL, ":\t", &saveptr1);
            
            #ifdef  DEBUG
            printf("%s\n", token);
            #endif
            strcpy(myinfo.name, token);
        } else if (strstr(token, "VmSize")) {
            token = strtok_r(NULL, ":\t", &saveptr1);
            strcpy(myinfo.vmsize, token);
        }
        
        #ifdef  DEBUG
                printf("%s\n", token);
        #endif
    }
    //Protect critical region
    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    
    proc_buffer[idx_buffer] = myinfo;
    idx_buffer ++;
    
    pthread_mutex_unlock(&mutex);
    sem_post(&full);
    //Protect critical region
    //
    fclose(fpstatus);
}

void *print_info(void *arg){
    
    int i;
    for (i = 0; i < buffer_size; i++) {
        sem_wait(&full);
        printf("printing: %d\n", rem_to_print);

        //pthread_mutex_lock(&mutex);
       // proc_info info = proc_buffer[rem_to_print];
        printf("id: %d\n", proc_buffer[rem_to_print].pid);
        printf("name: %s", proc_buffer[rem_to_print].name);
        printf("mem: %s", proc_buffer[rem_to_print].vmsize);
        
        //calc min and max
        int i;
        proc_info proc;
        proc_summary sum;
        for (i = 0; i <= rem_to_print; i++) {
            proc = proc_buffer[i];
            if (i > 0) {
                if (atoi(proc.vmsize) < atoi(sum.min_vm)) {
                    sum.min_pid = proc.pid;
                    strcpy(sum.min_vm, proc.vmsize);                    
                } else if(atoi(proc.vmsize) > atoi(sum.max_vm)) {
                   sum.max_pid = proc.pid;
                   strcpy(sum.max_vm, proc.vmsize); 
                }
            }else{
                sum.min_pid = proc.pid;
                strcpy(sum.min_vm, proc.vmsize);
                sum.max_pid = proc.pid;
                strcpy(sum.max_vm, proc.vmsize);
            }                
        }

        printf("General Statistics:\n");
        printf("Max Mem: Id %d %s", sum.max_pid, sum.max_vm);
        printf("Min Men: Id %d %s", sum.min_pid, sum.min_vm);
        
        rem_to_print ++;
        //pthread_mutex_unlock(&mutex);
        
        //sem_post(&empty);
        
    }
 
}