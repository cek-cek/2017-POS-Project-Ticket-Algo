#include <stdlib.h> // Contains even thread safe functions like rand_r and drand48_x
#include <stdio.h>

// Threading includes
#include <pthread.h>


// intptr_t
#include <stdint.h>

#include <time.h>

#define RAND_EXCLUSIVE_RANGE 501
#define DEBUG 1
#define DEBUG_YELLOW "\x1B[33m"
#define DEBUG_DEFAULT "\x1B[0m"

#define DECADIC_BASE 10

// TODO: INTRODUCE PROPER CONSTANTS

// Critical section entry mutex initialization 
int getticket (void);
void await (int aenter);
void advance (void);
void debugMessage(char* message);
void processParameters(char* argv[]);
int getticket();
void *ThreadWork(void *threadId);
unsigned int sleepThread(unsigned int *seed);
void showHelp();

struct Parameters
{
    long threadCount;
    long passes;
};

struct Parameters P;

pthread_mutex_t entryMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wakeCond = PTHREAD_COND_INITIALIZER;

int currentTicket = 1;



void await (int aenter)
{
    // First thing to do is to prevent other threads from entering
    if (pthread_mutex_lock(&entryMutex) != 0)
    {
        fprintf (stderr, "Error while using pthread_mutex_lock.\n");
    }

    while (currentTicket != aenter)
    {
        if (pthread_cond_wait(&wakeCond, &entryMutex) != 0)
            fprintf(stderr, "Error while using pthread_cond_wait().\n");
    }

    //   Maybe any thread that calls this function will be freezed in case critical section is entered by other thread alreadye
    //  printf ("Thread %d entered critical section.\n", aenter);
}

void advance ()
{
    if (pthread_mutex_unlock(&entryMutex) != 0)
    {
        fprintf (stderr, "Error while unlocking mutex.\n");
    }
    else
    {
        currentTicket++;
        if (pthread_cond_broadcast(&wakeCond))
        {
            fprintf(stderr, "Waking broadcast error!\n");
        }
    }
}





void *ThreadWork(void *threadId)
{
    int ticket;

    long tid;
    tid = (intptr_t) threadId;

    //printf ("Thread identifier #%ld was created and is now working!\n", tid);

    // Prepare seed for random generator
    unsigned int seed = (unsigned int) (time(NULL) + tid);


    unsigned int debugSleepTimer1;
    unsigned int debugSleepTimer2;


    while ((ticket = getticket()) < P.passes)
    {
        // Initial sleep
        debugSleepTimer1 = sleepThread(&seed);
        //printf ("Thread #%ld was put to sleep for %ums\n", tid, debugSleepTimer1);

        // await to enter critical section
        await(ticket);
        //printf ("Thread #%ld passed through await() function.\n", tid);


        // Print who is in critical section
        printf("%d\t(%d)\n", ticket, (int) tid);

        // advance to leave critical section
        advance();
        //printf ("Tread #%ld passed through advance() function.\n", tid);

        // Ending sleep
        debugSleepTimer2 = sleepThread(&seed);
        //printf ("Thread #%ld was put to sleep for %ums\n", tid, debugSleepTimer2);
    }


    pthread_exit(NULL);
}


int lastticket = 0;
int getticket()
{
    lastticket += 1;
    return lastticket;
}

int main (int argc, char* argv[])
{
    if (argc < 3)
    {
        showHelp();
        exit(1);
    }

    processParameters(argv);

    if (DEBUG)
    {
        debugMessage("Parameter parsing was successfull, program will now run.");   // God, I wish it was easier to work with integers, so I could print something more meaningful
        printf ("%s", DEBUG_YELLOW);
        printf("Program will be run with N=%ld, M=%ld\n", P.threadCount, P.passes);
        printf ("%s", DEBUG_DEFAULT);
    }

    // Spawn threads
    pthread_t threads[P.threadCount];
    long threadCreationCode;
    int i;

    for (i = 0; i < P.threadCount; i++)
    {
        threadCreationCode = pthread_create (&threads[i], NULL, ThreadWork, (void *)(intptr_t)i);
        if (threadCreationCode)
        {
            printf ("ERROR; return code from pthread_create() is %ld\n", threadCreationCode);
            exit (-1);
        }
    }

    printf ("Some additional output from main thread...\n");

    // Clean up threads
    for (i = 0; i < P.threadCount; i++)
    {
        pthread_join (threads[i], NULL);
        //printf ("Thread %i was collected.\n", i);
    }

    printf ("Some additional output from main thread...\n");
    
    pthread_exit (NULL);

    return 0;
}


void debugMessage(char* message)
{
    printf("%s", DEBUG_YELLOW);
    printf ("%s\n", message);
    printf("%s", DEBUG_DEFAULT);
}

/**
  * processParameters()
  * Takes char* argv from main, converts its contents to integer and fills the Parameters P global variable
  */
void processParameters(char* argv[])
{

    char *endptr;

    // Here should come some sort of a check whether this works.
    // TODO: Sanitize the input and buffer underflow, inspiration here: http://stackoverflow.com/questions/26080829/detecting-strtol-failure

    if (argv[1])
        P.threadCount = strtol(argv[1], &endptr, DECADIC_BASE);

    if (argv[2])
        P.passes = strtol(argv[2], &endptr, DECADIC_BASE);
}

unsigned int getRndThreadSleepTime(unsigned int *seed)
{
    return (rand_r(seed) % RAND_EXCLUSIVE_RANGE);
}

unsigned int sleepThread(unsigned int *seed)
{

    unsigned int sleepTime = getRndThreadSleepTime(seed); 

    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = (sleepTime * 1000000);

    nanosleep(&time, NULL);

    return sleepTime;
}

void showHelp ()
{
    printf ("Ticket algorithm synchronization demo.\n\n");
    printf ("Usage: ./XXX N M\n\n");
    printf ("Creates N threads and simulates M number of total passes through critical section protected by ticket algorithm.\n\n");
    printf ("No other available options. Wrong parameters to the program shows this help.\n");
}