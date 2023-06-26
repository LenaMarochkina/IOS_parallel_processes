#ifndef UNTITLED5_PROJ2_H
#define UNTITLED5_PROJ2_H

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

#define BASE_TEN 10

char MESSAGE[255];

/**
 * Define macros for printing messages
 */
#define write_msg(f, id, msg, role) do{\
    sem_wait(s_write);              \
    fprintf(f, "%d: %c %d: %s", sh_mem->msg_count++, role, id + 1, msg); \
    fflush(f); \
    sem_post(s_write);}                \
    while(0)

/**
 * Define macros for choosing random time
 */
#define random_time(id, max_time)  \
srand(time(NULL) * getpid() + id);\
usleep(rand() % (max_time * 1000 + 1));\

/**
 * Define semaphores names
 */
#define POSTAL_WORKER_SEM "/xmaroc00_postal_worker_sem"
#define CLIENT_SEM "/xmaroc00_client_sem"
#define MUTEX "/xmaroc00_mutex"
#define WRITE_SEM "/xmaroc00_write_SEM"
#define FIRST_QUEUE_SEM "/xmaroc00_queue1"
#define SECOND_QUEUE_SEM "/xmaroc00_queue2"
#define THIRD_QUEUE_SEM "/xmaroc00_queue3"
#define POST_OPEN_SEM "/xmaroc00_post_open"
#define POST_CLOSED_SEM "/xmaroc00_post_closed"

/**
 * Structure for storing arguments
 */
typedef struct args {
    unsigned NZ;    // the number of clients
    unsigned NU;    // the number of postal workers
    unsigned TZ;    // the maximum time for that a client is waiting before entering the post office
    unsigned TU;    // the maximum time of postal worker's break
    unsigned F;     // the maximum time postal office is working
} args_t;

/**
 * Structure for shared memory
 */
typedef struct shared_memory {
    unsigned msg_count; // message counter
    unsigned queue[3]; // number of clients in each queue
    bool post; // true if post office is open, false if closed
} shared_memory_t;
shared_memory_t *sh_mem = NULL;

//semaphores
sem_t *s_postal_worker;
sem_t *s_client;
sem_t *s_mutex;
sem_t *s_write;
sem_t *s_queue1;
sem_t *s_queue2;
sem_t *s_queue3;
sem_t *s_post_open;
sem_t *s_post_closed;

/**
 * @brief function to parse arguments
 * @param argc - number of arguments
 * @param argv - array of arguments
 * @param args - structure for storing arguments
 **/
void argument_parsing(int argc, char **argv, args_t *args);

/**
 * @brief function to initialize semaphores
 */
void init_semaphores();

/**
 * @brief function to initialize shared memory
 * @param f - file
 */
void init_shared_memory(FILE *f);

/**
 * @brief function to clean semaphores
 * @param f - file
 */
void clean_semaphores(FILE *f);

/**
 * @brief function to clean shared memory
 */
void clean_shared_memory();

/**
 * @brief function for the postal worker process
 * @param id - id of postal worker
 * @param args - structure for storing arguments
 * @param f - file
 */
void postal_worker_process(FILE *f, unsigned postal_worker_id, args_t args);

/**
 * @brief function for the client process
 * @param id - id of client
 * @param args - structure for storing arguments
 * @param f - file
 */
void client_process(FILE *f, unsigned client_worker_id, args_t args);

#endif //UNTITLED5_PROJ2_H
