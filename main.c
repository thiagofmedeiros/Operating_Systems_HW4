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

#define READERS_QTY 250000000
#define WRITER_QTY 25000

struct shared_dat
{
    int value;     /* shared variable to store result*/
};

struct shared_dat  *counter;
bool in_cs = false;
sem_t mutex;
sem_t writing;
bool writerActive = false;
int numReadsWithoutWriting = 0;
int numOfActiveReaders = 0;
int numOfReaders = 0;

struct rusage mytiming;

void * reader_thread(void *arg) {
    int reader_number = (int) arg;
    int i = 0;
    bool ignoreWriter = false;
    bool canRead = false;

    int readBlocked = 0;

    printf("Reader %d started\n", reader_number);

    while (i < READERS_QTY) {

        sem_wait (&writing);
        ignoreWriter = !writerActive;
        sem_post (&writing);

        if(ignoreWriter) {
            canRead = true;
            counter->value = counter->value;
            i++;
        } else {
            //printf("Reader %d  waiting for sem\n", reader_number);
            sem_wait(&mutex);
            //printf("Reader %d  got sem\n", reader_number);

            numOfActiveReaders++;

            if (numOfActiveReaders == 1) {
                sem_wait(&writing);
            }

            if (numReadsWithoutWriting < numOfReaders) {
                canRead = true;
            } else {
                numOfActiveReaders--;

                canRead = false;

                if (numOfActiveReaders == 0) {
                    sem_post(&writing);
                }
            }
/*
            //printf("numOfActiveReaders %d\n", numOfActiveReaders);
            if (numOfActiveReaders == 1) {
                //printf("Semaphore to stop writer\n");
                if (numReadsWithoutWriting < numOfReaders) {
                    sem_wait(&writing);
                    canRead = true;
                } else {
                    numOfActiveReaders--;
                    canRead = false;
                }
            }
            */
            //printf("Reader %d  released sem\n", reader_number);
            sem_post(&mutex);

            if (canRead) {
                if (in_cs) {
                    printf("\nERROR: in_cs is set\n");
                }

                counter->value = counter->value;
                numReadsWithoutWriting++;
                i++;

                //printf("Reader %d  waiting for sem\n", reader_number);
                sem_wait(&mutex);
                //printf("Reader %d  got sem\n", reader_number);

                numOfActiveReaders--;

                //printf("numOfActiveReaders %d\n", numOfActiveReaders);
                if (numOfActiveReaders == 0) {
                    //printf("Semaphore to free writer\n");
                    sem_post(&writing);
                }
                //  printf("Reader %d  released sem\n", reader_number);
                sem_post(&mutex);
            }
        }

        if (canRead && i > 0 && i % (READERS_QTY/100) == 0) {
            float percentage = i / (READERS_QTY/1.0);
            printf("Reader %d read %.0f%\n", reader_number, percentage * 100);
        }
    }
    printf("Reader %d finished\n", reader_number);

    return(NULL);
}

void * writer_thread(void *arg) {
    int i;

    sem_wait (&writing);
    printf("Writer Started\n");
    writerActive = true;
    sem_post (&writing);

    for(i = 0; i < WRITER_QTY; i++) {
        //printf("Writer waiting for semaphore\n");
        sem_wait (&writing);
        //printf("Writer semaphore free\n");

        in_cs = true;
        counter->value += 1;
        //printf("%d\n", counter->value);
        if (counter->value % (WRITER_QTY/100) == 0) {
            float percentage = counter->value / (WRITER_QTY/1.0);
            printf("Writer %.0f%\n", percentage * 100);
        }

        if (numReadsWithoutWriting > 0) {
            numReadsWithoutWriting = 0;
        }
        in_cs = false;

        //printf("Writer semaphore released\n");
        sem_post (&writing);
    }

    sem_wait (&writing);
    writerActive = false;
    sem_post (&writing);

    printf("Writer finished\n");

    return(NULL);
}

int main(int argc, char** argv) {

    if (argc == 2) {

        int k = 0;
        int i = 0;

        numOfReaders = atoi(argv[1]);

        if (numOfReaders < 19 && numOfReaders > 0) {

            pthread_t readers[18];     /* process id for thread 1 */
            pthread_t writer[1];     /* process id for thread 1 */
            pthread_attr_t attr[1];     /* attribute pointer array */

            sem_init(&mutex, 0, 1);
            sem_init(&writing, 0, 1);

            counter = (struct shared_dat *) malloc(sizeof(struct shared_dat));

            in_cs = true;

            /* initialize shared memory to 0 */
            counter->value = 0;

            k = (int) (numOfReaders / 2);

            /* Create half of readers */
            for(i = 0; i < k; i++) {
                pthread_create(&readers[i], &attr[0], reader_thread, (void *) i);
            }

            /* Create the writer thread */
            pthread_create(&writer[0], &attr[0], writer_thread, NULL);

            /* Create last half of readers */
            for(i = k ; i < numOfReaders ; i++) {
                pthread_create(&readers[i], &attr[0], reader_thread, (void *) i);
            }

            pthread_join(writer[0], NULL);

            for (i = 0; i < numOfReaders; i++) {
                pthread_join(readers[i], NULL);
            }

            getrusage(RUSAGE_SELF, &mytiming);
            printf("Time used is sec: %d, usec %d\n",mytiming.ru_utime.tv_sec,
                   mytiming.ru_utime.tv_usec);
            printf("System Time used is sec: %d, usec %d\n",mytiming.ru_stime.tv_sec,
                   mytiming.ru_stime.tv_usec);

            return 0;
        }
        else {
            printf("Expecting number of readers 1-18\n");

            return 1;
        }
    }
    else {
        printf("Expecting 1 parameter and received %d\n", argc);

        return 1;
    }
}
