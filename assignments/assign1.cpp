#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

//Global Variables (These probably don't need to be global)
// • NUMP: Number of producer threads
// • NUMC: Number of consumer threads
// • NUMPT: Total number of products to be generated by all producer threads
// • TOTP: Size of the queue to store products for both producer and consumer threads (0 for
// unlimited queue size)
// • ALGO: 0 or 1 for type of scheduling algorithm: 0 for First-Come-First-Serve, and 1 for
// Round-Robin
// • QUANTUM: Value of quantum used for round-robin scheduling.
// • SEED: Seed for random number generator 

int NUMP, NUMC, TOTP, QSIZE, ALGO, QUANTUM, SEED;
// Number of Products currently in queue
int NPROD;
pthread_mutex_t prod_mutex;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER, condc = PTHREAD_COND_INITIALIZER;

class Product{
    private:
        int id;
        clock_t timestamp;
        int life;

    public:
        Product(int id) : id(id), timestamp(clock()), life(random() % 1024){  
            pthread_mutex_lock(&prod_mutex);
            while(NPROD >= QSIZE) pthread_cond_wait(&condp, &prod_mutex);
            NPROD++;
            pthread_cond_signal(&condc);
            pthread_mutex_unlock(&prod_mutex);
        }
};

void *producer(void *id){
    int int_id = *(int*)id;
    pthread_exit(NULL);
    //Insert Product
    Product prod (int_id);
    //Sleep 100 milliseconds
    usleep(100000);
}

int fb(int n)
{
   if (n <= 1)
      return n;
   return fib(n-1) + fib(n-2);
}

void *consumer(void *id){
    int v = *(int*)id;
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    // for (int i = 0; i < argc; ++i)
    //     std::cout << argv[i] << "\n";
    NUMP = atoi(argv[1]);
    NUMC = atoi(argv[2]);
    TOTP = atoi(argv[3]);
    QSIZE = atoi(argv[4]);
    ALGO = atoi(argv[5]);
    QUANTUM = atoi(argv[6]);
    SEED = atoi(argv[7]);
    
    // Initialize Seed
    
    srandom(SEED);

    // Initialize Mutexes
    pthread_mutex_init(&prod_mutex, NULL);
    
    // Create prod threads

    int i;

    pthread_t prod_thread[NUMP];
    int prodID[NUMP];

    for (i=0;i<NUMP;i++){
        prodID[i] = i;
        pthread_create(&prod_thread[i], NULL, producer, &prodID[i]);
    }

    // create consumer threads

    pthread_t consmr_thread[NUMC];
    int consmrID[NUMP];

    for (i=0;i<NUMC;i++){
        consmrID[i] = i;
        pthread_create(&consmr_thread[i], NULL, consumer, &consmrID[i]);
    }

    // join consumer/producer threads

    for (i=0;i<NUMP;i++)
        pthread_join(prod_thread[i],NULL);

    for (i=0;i<NUMC;i++)
        pthread_join(consmr_thread[i],NULL);

    // delete everything

    pthread_mutex_destroy(&prod_mutex);
    pthread_exit(0);

    return 0;
}
