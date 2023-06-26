#include "proj2.h"


// function to convert string to integer
int str_to_int(char *str, unsigned int *num) {
    char *endptr;
    *num = strtol(str, &endptr, BASE_TEN);

    if (endptr == str) {
        return 1;
    }

    return 0;
}

void argument_parsing(int argc, char *argv[], args_t *args) {
    if (argc != 6) {
        fprintf(stderr, "Invalid argument count.\n");
        exit(EXIT_FAILURE);
    }

    int error = 0;
    error += str_to_int(argv[1], &args->NZ);
    error += str_to_int(argv[2], &args->NU);
    error += str_to_int(argv[3], &args->TZ);
    error += str_to_int(argv[4], &args->TU);
    error += str_to_int(argv[5], &args->F);

    if (error) {
        perror("Argument conversion to integer failed");
        exit(EXIT_FAILURE);
    }

    if (args->NU == 0) {
        fprintf(stderr, "Number of postal workers must be greater than 0.\n");
        exit(EXIT_FAILURE);
    }

    if (args->TZ < 0 || args->TZ > 10000) {
        fprintf(stderr,
                "The maximum time, that client waits before entering the post has to be from the interval <0,10000>.\n");
        exit(EXIT_FAILURE);
    }
    if (args->TU < 0 || args->TU > 100) {
        fprintf(stderr, "The maximum time of postal worker's break has to be from the interval <0,100>.\n");
        exit(EXIT_FAILURE);
    }
    if (args->F < 0 || args->F > 10000) {
        fprintf(stderr,
                "The maximum time, that post is opened for new clients has to be from the interval <0,10000>.\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Function maps shared memory.
 *
 * @param f file that must be closed in case error occurs
 * @param args struct where parametres of the program are stored (NZ, NU, TZ, TU, F)
 * @return void
 */
void init_shared_memory(FILE *f) {
    if ((sh_mem = mmap(NULL, sizeof(shared_memory_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0)) ==
        MAP_FAILED) {
        fprintf(stderr, "Error while mapping shared memory\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }

    // Setting values to shared variables
    sh_mem->msg_count = 1;
    sh_mem->post = true;
    // Set all elements in the queue array to 0
    for (int i = 0; i < 3; i++) {
        sh_mem->queue[i] = 0;
    }
}

/**
 * @brief Function maps and opens semaphores.
 *
 * @param f file that must be closed in case error occurs
 * @return void
 */
void init_semaphores() {
    // unlink semaphores
    sem_unlink(POSTAL_WORKER_SEM);
    sem_unlink(CLIENT_SEM);
    sem_unlink(FIRST_QUEUE_SEM);
    sem_unlink(SECOND_QUEUE_SEM);
    sem_unlink(THIRD_QUEUE_SEM);
    sem_unlink(POST_OPEN_SEM);
    sem_unlink(POST_CLOSED_SEM);
    sem_unlink(WRITE_SEM);
    sem_unlink(MUTEX);

    // open semaphores
    if ((s_postal_worker = sem_open(POSTAL_WORKER_SEM, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED ||
        (s_client = sem_open(CLIENT_SEM, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED ||
        (s_queue1 = sem_open(FIRST_QUEUE_SEM, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED ||
        (s_queue2 = sem_open(SECOND_QUEUE_SEM, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED ||
        (s_queue3 = sem_open(THIRD_QUEUE_SEM, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED ||
        (s_post_open = sem_open(POST_OPEN_SEM, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED ||
        (s_post_closed = sem_open(POST_CLOSED_SEM, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED ||
        (s_write = sem_open(WRITE_SEM, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED ||
        (s_mutex = sem_open(MUTEX, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED) {
        fprintf(stderr, "Error while opening semaphore\n");
        exit(EXIT_FAILURE);
    }
}

void clean_shared_memory() {
    if (munmap(sh_mem, sizeof(shared_memory_t))) {
        fprintf(stderr, "Error while unmapping shared memory\n");
        exit(EXIT_FAILURE);
    }
}

void clean_semaphores(FILE *f) {
    // close semaphores
    if (sem_close(s_postal_worker) == -1 || sem_close(s_client) == -1 ||
        sem_close(s_queue1) == -1 || sem_close(s_queue2) == -1 ||
        sem_close(s_queue3) == -1 || sem_close(s_write) == -1 ||
        sem_close(s_post_open) || sem_close(s_post_closed) == -1 ||
        sem_close(s_mutex) == -1) {
        fprintf(stderr, "Error while closing semaphore\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    // unlink semaphores
    if (sem_unlink(POSTAL_WORKER_SEM) == -1 || sem_unlink(CLIENT_SEM) == -1 ||
        sem_unlink(FIRST_QUEUE_SEM) == -1 || sem_unlink(SECOND_QUEUE_SEM) == -1 ||
        sem_unlink(THIRD_QUEUE_SEM) == -1 || sem_unlink(WRITE_SEM) == -1 ||
        sem_unlink(POST_OPEN_SEM) == -1 || sem_unlink(POST_CLOSED_SEM) == -1 ||
        sem_unlink(MUTEX) == -1) {
        fprintf(stderr, "Error while unlinking semaphore\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }
}

void postal_worker_process(FILE *f, unsigned postal_worker_id, args_t args) {
    // write message the postal worker has started
    write_msg(f, postal_worker_id, "started\n", 'U');

    while (1) {
        // get the needed states with mutex
        sem_wait(s_mutex);
        bool queue_full;
        queue_full = (sh_mem->queue[0] || sh_mem->queue[1] || sh_mem->queue[2]);
        unsigned post_status = 0;
        sem_post(s_mutex);

        // if there is a customer waiting, serve him
        if (queue_full) {
            // choose randomly from queue which is not empty
            srand(time(NULL) * getpid() + postal_worker_id);
            unsigned queue = rand() % 3 + 1;
            while (!sh_mem->queue[queue - 1]) {
                queue = rand() % 3 + 1;
            }

            sem_wait(s_mutex);
            sh_mem->queue[queue - 1]--;
            sem_post(s_mutex);

            // wait for the customer to come to the postal worker
            if (queue == 1) sem_post(s_queue1);
            else if (queue == 2) sem_post(s_queue2);
            else if (queue == 3) sem_post(s_queue3);

            // write message that the postal worker is serving a client
            sprintf(MESSAGE, "serving a service of type %d\n", queue);
            write_msg(f, postal_worker_id, MESSAGE, 'U');

            // serve the customer for random time
            random_time(postal_worker_id, 10)

            // write message that the postal worker has finished serving a client
            write_msg(f, postal_worker_id, "service finished\n", 'U');
        } else if (sh_mem->post) {
            // write message that the postal worker is taking a break
            write_msg(f, postal_worker_id, "taking break\n", 'U');

            // if office is closed while employee is taking a break, post office semaphore so the office doesn't wait forever
            if (!sh_mem->post) {
                sem_post(s_post_open);
                post_status = 1;
            }

            // take a break for random time
            random_time(postal_worker_id, args.TU)

            // write message that the postal worker has finished taking a break
            write_msg(f, postal_worker_id, "break finished\n", 'U');
        } else if (!sh_mem->post) {
            // check again if there are customers waiting after the shop closed in case one got in the line after the queue_full check
            if (sh_mem->queue[0] || sh_mem->queue[1] || sh_mem->queue[2]) {
                continue;
            }

            // if office semaphore wasn't posted before, post it so the office doesn't wait forever
            if (!post_status) {
                sem_post(s_post_open);
            }

            // if there are no clients waiting, the postal worker can go home
            unsigned client_count = 0; // total number of clients waiting
            for (int i = 0; i < 3; i++) {
                client_count += sh_mem->queue[i];
                // if there were clients waiting in the queue, unblock them
                if (sh_mem->queue[i] > 0) {
                    for (int j = 0; j < sh_mem->queue[i]; j++) {
                        if (i == 0) sem_post(s_queue1);
                        else if (i == 1) sem_post(s_queue2);
                        else if (i == 2) sem_post(s_queue3);
                    }
                }
            }
            // wait until post prints out closing message
            sem_wait(s_post_closed);

            // write message that the postal worker is going home
            write_msg(f, postal_worker_id, "going home\n", 'U');
            exit(EXIT_SUCCESS);
        }
    }
}

void client_process(FILE *f, unsigned client_id, args_t args) {
    // send message that customer has started
    write_msg(f, client_id, "started\n", 'Z');

    // wait before enter the service for random time
    random_time(client_id, args.TZ)

    // client chooses service
    srand(time(NULL) * getpid() + client_id);
    int service = rand() % 3 + 1;

    // write message that customer is entering the service
    if (sh_mem->post) {
        // add customer to the queue
        sem_wait(s_mutex);
        sh_mem->queue[service - 1]++;
        sem_post(s_mutex);

        sprintf(MESSAGE, "entering office for a service %d\n", service);
        write_msg(f, client_id, MESSAGE, 'Z');
    } else {
        // if the office is closed, go home
        write_msg(f, client_id, "going home\n", 'Z');
        exit(EXIT_SUCCESS);
    }

    // wait until the customer is called by the post worker
    if (service == 1) sem_wait(s_queue1);
    else if (service == 2) sem_wait(s_queue2);
    else if (service == 3) sem_wait(s_queue3);

    // write message that customer is being served
    write_msg(f, client_id, "called by office worker\n", 'Z');
    // wait for the service to finish
    random_time(client_id, 10)
    // write message that customer is leaving the service
    write_msg(f, client_id, "going home\n", 'Z');

    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // open output file for writing
    FILE *f = fopen("proj2.out", "w");
    if (!f) {
        fprintf(stderr, "Error while opening file\n");
        exit(EXIT_FAILURE);
    }

    args_t args;
    argument_parsing(argc, argv, &args);

    init_shared_memory(f);
    init_semaphores();

    // Creating processes
    for (int i = 0; i < args.NU + args.NZ; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Error while forking\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (i < args.NU) {
                postal_worker_process(f, i, args);
            } else if (i <= args.NU + args.NZ) {
                client_process(f, i - args.NU, args);
            }
            exit(EXIT_SUCCESS);
        }
    }

    // sleep for a half F time and close the shop
    srand(time(NULL) * getpid());
    usleep(rand() % (args.F * 1000 + 1) + args.F / 2 * 1000);

    sh_mem->post = false;

    // postal worker can't start the break after the shop is closed
    for (int i = 0; i < args.NU; i++) {
        sem_wait(s_post_open);
    }

    // print that the shop is closing
    sem_wait(s_write);
    fprintf(f, "%d: closing\n", sh_mem->msg_count++);
    fflush(f);
    sem_post(s_write);

    // let all the employees go home
    for (int i = 0; i < args.NU; i++) {
        sem_post(s_post_closed);
    }

    // main process waits for its children to exit
    while (wait(NULL) != -1 || errno == EINTR);

    // clean up
    clean_semaphores(f);
    clean_shared_memory();
    return 0;
}
