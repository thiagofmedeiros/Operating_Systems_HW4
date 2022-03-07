/*
*   Name: Thiago André Ferreira Medeiros
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/resource.h>
#include<time.h>

#define READERS_QTY 250000000
#define WRITER_QTY 25000

struct shared_dat {
    int value;     /* shared variable to store result*/
};

struct shared_dat *counter;
bool in_cs = false;
sem_t mutex;
sem_t writing;
int numOfActiveReaders = 0;
int numOfReaders = 0;
int numOfReadersStarted = 0;
int numOfReadersAllowedToStartBeforeWriter;
bool writerFinished = false;

struct rusage mytiming;

void *reader_thread(void *arg) {
    int reader_number = (int) arg;
    bool canRead = false;

    // printf("Reader %d started\n", reader_number);

    // wait for being able to read
    while (!canRead) {
        // Prevent other Readers to manipulate shared variables
        sem_wait(&mutex);

        // Writer has finished and Readers can run without problems
        if (writerFinished) {
            canRead = true;

            numOfActiveReaders++;
            numOfReadersStarted++;
        } else {
            // Enable Readers up to numOfReadersAllowedToStartBeforeWriter
            if (numOfReadersStarted < numOfReadersAllowedToStartBeforeWriter) {
                canRead = true;

                numOfActiveReaders++;
                numOfReadersStarted++;

                // Prevent writer to enter critical section
                // Only do this for the first reader to not increment number of semaphores
                if (numOfActiveReaders == 1) {
                    sem_wait(&writing);
                }
            }
                // Too many Readers have been enabled before Writer
            else {
                canRead = false;
            }
        }
        // Release Readers critical section
        sem_post(&mutex);
    }

    //printf("Reader %d started\n", reader_number);
    for (int i = 0; i < READERS_QTY; i++) {
        // Error flag check
        if (in_cs) {
            printf("\nERROR: in_cs is set\n");
        }
        // Reading
        counter->value = counter->value;
    }

    // Wait for manipulating Readers shared variables
    sem_wait(&mutex);

    numOfActiveReaders--;

    // If there are no readers, allow writer to write
    if (numOfActiveReaders == 0) {
        // Semaphore to free writer
        sem_post(&writing);
    }
    // Release Readers critical section
    sem_post(&mutex);

    printf("Reader %d finished\n", reader_number);

    return (NULL);
}

void *writer_thread(void *arg) {
    // Wait for Writer semaphore
    sem_wait(&writing);
    printf("Writer Started\n");

    for (int i = 0; i < WRITER_QTY; i++) {
        in_cs = true;
        counter->value += 1;
        in_cs = false;
    }

    writerFinished = true;

    sem_post(&writing);

    printf("Writer finished\n");

    return (NULL);
}

int main(int argc, char **argv) {

    if (argc == 2) {

        numOfReaders = atoi(argv[1]);

        if (numOfReaders < 19 && numOfReaders > 0) {

            // Ensure fairness with random number
            srand(time(0));
            numOfReadersAllowedToStartBeforeWriter = rand() % numOfReaders;
            printf("\nnumOfReadersAllowedToFinishBeforeWriter %d\n\n", numOfReadersAllowedToStartBeforeWriter);

            pthread_t readers[18];     /* process id for thread 1 */
            pthread_t writer[1];     /* process id for thread 1 */
            pthread_attr_t attr[1];     /* attribute pointer array */

            sem_init(&mutex, 0, 1);
            sem_init(&writing, 0, 1);

            counter = (struct shared_dat *) malloc(sizeof(struct shared_dat));

            /* initialize shared memory to 0 */
            counter->value = 0;

            int k = (int) (numOfReaders / 2);

            /* Create half of readers */
            for (int i = 0; i < k; i++) {
                pthread_create(&readers[i], &attr[0], reader_thread, (void *) i);
            }

            /* Create the writer thread */
            pthread_create(&writer[0], &attr[0], writer_thread, NULL);

            /* Create last half of readers */
            for (int i = k; i < numOfReaders; i++) {
                pthread_create(&readers[i], &attr[0], reader_thread, (void *) i);
            }

            pthread_join(writer[0], NULL);

            for (int i = 0; i < numOfReaders; i++) {
                pthread_join(readers[i], NULL);
            }

            getrusage(RUSAGE_SELF, &mytiming);
            printf("\nTime used is sec: %d, usec %d\n", mytiming.ru_utime.tv_sec,
                   mytiming.ru_utime.tv_usec);
            printf("System Time used is sec: %d, usec %d\n", mytiming.ru_stime.tv_sec,
                   mytiming.ru_stime.tv_usec);

            return 0;
        } else {
            printf("Expecting number of readers 1-18\n");

            return 1;
        }
    } else {
        printf("Expecting 1 parameter and received %d\n", argc);

        return 1;
    }
}
