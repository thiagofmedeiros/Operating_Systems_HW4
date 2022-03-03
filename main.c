/*
*   Name: Thiago André Ferreira Medeiros
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>

struct shared_dat
{
    int value;     /* shared variable to store result*/
};

struct shared_dat  *counter;
bool in_cs = false;
sem_t mutex;
sem_t writing;
bool writerActive = false;
bool canRead = false;
int numReadsWithoutWriting = 0;
int numOfActiveReaders = 0;
int numOfReaders = 0;

void * reader_thread(void *arg) {
    int reader_number = (int) arg;
    int i = 0;
    bool ignoreWriter = false;

    printf("Reader %d started\n", reader_number);

    while (i < 2500000) {

        sem_wait (&writing);
        ignoreWriter = !writerActive;
        sem_post(&writing);

        if (ignoreWriter) {
            counter->value = counter->value;
            i++;
        } else {

            sem_wait(&mutex);

            numOfActiveReaders++;

            if (numOfActiveReaders == 1 &&
                numReadsWithoutWriting < numOfActiveReaders) {

                sem_wait(&writing);
            }

            if (numReadsWithoutWriting < numOfActiveReaders) {
                canRead = true;
                numReadsWithoutWriting++;
            } else {
                canRead = false;
            }

            sem_post(&mutex);

            if (canRead) {
                counter->value = counter->value;
                i++;

                sem_wait(&mutex);

                numOfActiveReaders--;

                sem_post(&mutex);

                if (numOfActiveReaders == 0) {
                    sem_post(&writing);
                }
            }
        }
    }
    printf("Reader %d finished\n", reader_number);

    return(NULL);
}

void * writer_thread(void *arg) {
    int i;

    printf("Writer Started\n");

    sem_wait(&writing);
    writerActive = true;
    sem_post (&writing);

    for(i = 0; i < 2500000 * 18; i++) {
        sem_wait (&writing);

        //printf("Writer acquired lock\n");
        in_cs = true;
        counter->value += 1;

        // printf("Writer %d\n", counter->value);

        //printf("Writer released lock\n");
        numReadsWithoutWriting = 0;
        in_cs = false;

        sem_post (&writing);
    }
    sem_wait(&writing);
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
                pthread_join(readers[0], NULL);
            }

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
