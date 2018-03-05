#include <iostream>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

//If this value is true, all debug statements are printed.
bool DEBUG = false, METRIC = true;

// Global Constants
// • PMAX: Total number of products to be generated by all producer threads
// • QMAX: Size of the queue to store products for both producer and consumer threads (0 for
// unlimited queue size)
// • QNTM: Value of quantum used for round-robin scheduling.
int PMAX, QMAX, QNTM;

// Global Variables
// • NPROD: Total Number of Products produced
// • NCONS: Total Number of Products cosumed
// • UNLIM: QUEUE has no limit on size
int NPROD = 0, NCONS = 0;
bool PDONE = false, CDONE = false, UNLIM = false, RDRB = false;

// Global Metrics
// • TIMET: Total Processing Time
// • MINTA: Minimum Turnaround Time
// • MAXTA: Maximum Turnaround Time
// • AVGTA: Average Turnaround Time
// • MINW: Minimum Wait
// • MAXW: Maximum Wait
// • AVGW: Average Wait
// • PRODT: Producer Throughput
// • CNSMRT: Consumer Throughput
clock_t MINTA=0, MAXTA=0,  PRODT=0, CNSMRT=0;
float AVGTA=0, AVGW=0, MINW=0, MAXW=0, TIMET=0;

// Global pthread variables
// • queue_mutex: Mutex for the QUEUE variable
// • condp: condition for producers
// • condc: condition for consumers
pthread_mutex_t queue_mutex;
pthread_cond_t condp = PTHREAD_COND_INITIALIZER, condc = PTHREAD_COND_INITIALIZER;

int compare_time(clock_t first, clock_t second){
    if(first > second) return 1;
    else if(second > first) return -1;
    else return 0;
}

// Product class that holds a product ID, timestamp, and life
// Consume methods executes fb N times depending on algo
class Product{
    private:
        int id;
        int life;
        clock_t timestamp;
        clock_t end;
        clock_t begin;
        clock_t turnaround;
        float wait;
        int fb(int n){
           if (n <= 1)
              return n;
           return fb(n-1) + fb(n-2);
        }

    public:
        Product (int id) : id(id), life(rand() % 1024), timestamp(clock()), end(timestamp), wait(0) {
            //if(DEBUG) std::cout << "+Product ID (produced): " << this->id << std::endl;
            //if(DEBUG) std::cout << "Initial End: " << end << std::endl;
        }
        int get_id(){
            return this->id;
        }
        void set_begin(clock_t begin){
            this->begin = begin;
        }
        void set_end(clock_t end){
            this->end = end;
            TIMET += (float)(this->end - this->begin);    // Adding to the total time of process
        }
        void turnaround_update(){
            this->turnaround = clock() - this->timestamp;
            if(MINTA == 0 || MINTA > this->turnaround) MINTA = this->turnaround;
            if(MAXTA == 0 || MAXTA < this->turnaround) MAXTA = this->turnaround;
            AVGTA += ((float)this->turnaround);
        }
        void wait_update(){
            //if(DEBUG) std::cout << "Begin: " << this->begin << std::endl;
            // if(DEBUG) std::cout << "End: " << this->end << std::endl;
            this->wait += (float)(this->begin - this->end);
            // if(DEBUG) std::cout << "Update Wait: " << this->wait << std::endl;
        }
        void wait_finish(){
            // if(DEBUG) std::cout << "Final Wait: " << this->wait << std::endl;
            if(MINW == 0 || MINW > this->wait) MINW = this->wait;
            if(MAXW == 0 || MAXW < this->wait) MAXW = this->wait;
            AVGW += this->wait;
        }
        // Runs fibo sequence [N=life] times
        void consume(){
            this->wait_update();
            for(int i = 0; i < this->life; i++) fb(10);
            // if(DEBUG) std::cout << "-ProductID (consumed): " << this->id << std::endl;
        }
        // Round robin consume. If ready to be removed from queue, function returns true
        bool consume(int quantum){
            if(this->life > quantum){
                this->wait_update();
                this->life -= quantum;
                // if(DEBUG) std::cout << "<3Life reduced: " << this->life << std::endl;
                for(int i = 0; i < quantum; i++) fb(10);
                // if(DEBUG) std::cout << "-ProductID (consumed): " << this->id << std::endl;
                return false;
            }else{
                consume();
                // if(DEBUG) std::cout << "<3Life Drained" << std::endl;
                // if(DEBUG) std::cout << "-ProductID (consumed): " << this->id << std::endl;
                return true;
            }
        }
};

// • QUEUE: Queue that holds all products
std::queue<Product> QUEUE;

void *producer(void *id){
    int int_id = *(int*)id; // The unique producer thread id
    // if(DEBUG) std::cout << "!Producer Thread ID: " << int_id << std::endl;

    // PDONE is true when PMAX has been reached
    while(!PDONE){
        // if(DEBUG) std::cout << "$PRODUCER LOCK REQUESTED ID: " << int_id << std::endl;
        pthread_mutex_lock(&queue_mutex); 

        // Checks if Queue limit is reached or skips if the queue size has no limit
        while(QUEUE.size() >= QMAX && !UNLIM) {
            // if(DEBUG) std::cout << "...Producer Thread Waiting ID: " << int_id << std::endl;
            pthread_cond_wait(&condp, &queue_mutex);    // Waits for consumer to consume
            // if(DEBUG) std::cout << "...Producer Thread Finished Waiting ID: " << int_id << std::endl;
        }
        // if(DEBUG) std::cout << "$PRODUCER LOCK RECEVIED ID: " << int_id << std::endl;

        // If enough products have been made, quit.
        if(NPROD == PMAX){
            PRODT = clock() - PRODT;
            PDONE = true;
            pthread_cond_broadcast(&condp); // Lets all waiting producers continue
            pthread_mutex_unlock(&queue_mutex);
            // if(DEBUG) std::cout << "$PRODUCER UNLOCKED ID: " << int_id << std::endl;
            break;
        // If no products have been consumed, start a clock for throughput
	    }else if(NPROD == 0)
            PRODT = clock();

        QUEUE.push(Product(NPROD));
        // if(DEBUG) std::cout << "^Queue Size (produced): " << QUEUE.size() << std::endl;
        std::cout << "Producer " << int_id << " has produced product " << NPROD << std::endl;
	    ++NPROD;
        // if(DEBUG) std::cout << "*Number of Products Produced: " << NPROD << std::endl;
        pthread_cond_signal(&condc);    // Lets a single waiting consumer continue
        pthread_mutex_unlock(&queue_mutex); 
        // if(DEBUG) std::cout << "$PRODUCER UNLOCKED ID: " << int_id << std::endl;
        usleep(100000); // Sleep 100 milliseconds (100000 microseconds)
    }
    // if(DEBUG) std::cout << "!Producer ID: " << int_id << " Exited" << std::endl;
    pthread_exit(NULL);
}

void *consumer(void *id){
    int int_id = *(int*)id; // The unique consumer thread ID
    if(DEBUG) std::cout << "!Consumer Thread ID: " << int_id << std::endl;

    // CDONE is true when PMAX has been reached
    while(!CDONE){
        if(DEBUG) std::cout << "$CONSUMER LOCK REQUEST ID: " << int_id << std::endl;
        pthread_mutex_lock(&queue_mutex);

        // Checks if there are any products or continue if all possible products have been consumed 
        while(QUEUE.size() < 1 && NCONS < PMAX) {
            if(DEBUG) std::cout << "...Consumer Thread Waiting ID: " << int_id << std::endl;
            pthread_cond_wait(&condc, &queue_mutex);
            if(DEBUG) std::cout << "...Consumer Thread Finished Waiting ID: " << int_id << std::endl;
        }
        if(DEBUG) std::cout << "$CONSUMER LOCK RECEVIED ID: " << int_id << std::endl; 

        // If all possible products have been consumed, exit.
        if(NCONS == PMAX){
            CNSMRT = clock() - CNSMRT;
            CDONE = true;
            pthread_cond_broadcast(&condc); // Lets all waiting consumers continue
            pthread_mutex_unlock(&queue_mutex);
            if(DEBUG) std::cout << "$CONSUMER UNLOCKED ID: " << int_id << std::endl;
            break;
        // If no products have been consumed, start a clock for throughput
	    }else if(NCONS == 0)
            CNSMRT = clock();
        
        // If Round Robin and the item hasn't reached the end of its life, push it back
        if(RDRB){
            Product prod = QUEUE.front();
            prod.set_begin(clock());
            QUEUE.pop();
            if(!prod.consume(QNTM)) QUEUE.push(prod);
            else {
                ++NCONS;
                prod.turnaround_update();   // Updates the turnaround value
                prod.wait_finish();         // Updates with the calculated waits
                std::cout << "Consumer " << int_id << " has consumed product " << prod.get_id() << std::endl;
            }
            prod.set_end(clock());          // Updates the end value
        } else {
            Product prod = QUEUE.front();
            prod.set_begin(clock());
	        ++NCONS;
            prod.consume();
            QUEUE.pop();
            prod.wait_finish();             // Updates with the calculated wait
            prod.turnaround_update();
            prod.set_end(clock());          // Updates the end value
            std::cout << "Consumer " << int_id << " has consumed product " << prod.get_id() << std::endl;
        }

        if(DEBUG) std::cout << "*Number of Products Consumed: " << NCONS << std::endl;
        if(DEBUG) std::cout << "vQueue Size (consumed): " << QUEUE.size() << std::endl;
        pthread_cond_signal(&condp);    // Lets a single waiting producer continue
        pthread_mutex_unlock(&queue_mutex); 
        if(DEBUG) std::cout << "$CONSUMER UNLOCKED ID: " << int_id << std::endl;
        usleep(100000); //Sleep 100 milliseconds (100000 microseconds)
    }
    if(DEBUG) std::cout << "!Consumer ID: " << int_id << " Exited" << std::endl;
    pthread_exit(NULL);
}

int main(int argc, char* argv[]){
    if(argc != 8) {
        std::cout << "Usage: ./assign1 P1 P2 P3 P4 P5 P6 P7\n"
                  << "P1: Number of producer threads\n"
                  << "P2: Number of consumer threads\n"
                  << "P3: Total number of products to be generated by all producer threads\n"
                  << "P4: Size of the queue to store products for both producer and consumer threads (0 for unlimited queue size)\n"
                  << "P5: 0 or 1 for type of scheduling algorithm: 0 for First-Come-First-Serve, and 1 for Round-Robin\n"
                  << "P6: Value of quantum used for round-robin scheduling\n"
                  << "P7: Seed for a random number generator"
                  << std::endl;
        return -1;
    }

    // Input Values

    int nump = atoi(argv[1]);   // Number of Producers
    if(nump < 1) std::cout << "P1 should be at least 1" << std::endl;
    int numc = atoi(argv[2]);   // Number of Consumers
    if(numc < 1) std::cout << "P2 should be at least 1" << std::endl;
    PMAX = atoi(argv[3]);       // Number of Products to be Produced
    if(PMAX < 1) std::cout << "P3 should be at least 1" << std::endl;
    QMAX = atoi(argv[4]);       // Queue Limit
    if(QMAX < 0) std::cout << "P4 should be at least 0" << std::endl;
    int algo = atoi(argv[5]);   // Algorithm: 0 for FIFS, 1 for Round-Robin
    if(algo != 0 && algo != 1) std::cout << "P5 should be 0 or 1" << std::endl;
    QNTM = atoi(argv[6]);       // Round Robin Quantum
    if(QNTM < 1 && QNTM > 1023) std::cout << "P6 should be at least 1 and no more than 1023" << std::endl;
    int seed = atoi(argv[7]);   // Seed for random value in Product life
    if(seed < 1) std::cout << "P7 should be larger than 1" << std::endl;

    // Set Seed/Initialize Mutexes/Declare threads & ids/Set UNLIM & RDRB

    srand(seed);
    pthread_mutex_init(&queue_mutex, NULL);
    pthread_t prod_thread[nump], consmr_thread[numc];
    int prodID[nump], consmrID[numc];
    if(QMAX == 0) UNLIM = true;
    if(algo == 1) RDRB = true;
    
    // Create prod threads

    for (int i=0;i<nump;i++){
        prodID[i] = i;
        pthread_create(&prod_thread[i], NULL, producer, &prodID[i]);
    }

    // Create consumer threads

    for (int i=0;i<numc;i++){
        consmrID[i] = i;
        pthread_create(&consmr_thread[i], NULL, consumer, &consmrID[i]);
    }

    // Join consumer/producer threads

    for (int i=0;i<nump;i++)
        pthread_join(prod_thread[i],NULL);

    for (int i=0;i<numc;i++)
        pthread_join(consmr_thread[i],NULL);

    // Destroy everything

    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&condp);
    pthread_cond_destroy(&condc);

    // Print Metrics
    if(METRIC){
        std::cout << "_______________________________\n" << "[METRICS]" << std::endl;
        std::cout << "Total Time: " << (TIMET*1000)/CLOCKS_PER_SEC << " miliseconds" << std::endl;
        std::cout << "Minimum Turnaround: " << ((float)MINTA*1000)/(CLOCKS_PER_SEC) << " miliseconds" << std::endl;
        std::cout << "Maximum Turnaround: " << ((float)MAXTA*1000)/(CLOCKS_PER_SEC) << " miliseconds" << std::endl;
        std::cout << "Average Turnaround: " << ((float)AVGTA*1000)/(PMAX*CLOCKS_PER_SEC) << " miliseconds" << std::endl;
        std::cout << "Minimum Wait: " << (MINW*1000)/(CLOCKS_PER_SEC) << " miliseconds" << std::endl;
        std::cout << "Maximum Wait: " << (MAXW*1000)/(CLOCKS_PER_SEC) << " miliseconds" << std::endl;
        std::cout << "Average Wait: " << (AVGW*1000)/(PMAX*CLOCKS_PER_SEC) << " miliseconds" << std::endl;
        std::cout << "Producer Throughput: " << (((float)PRODT)*1000/CLOCKS_PER_SEC)/PMAX << " milliseconds per product produced" << std::endl;
        std::cout << "Consumer Throughput: " << (((float)CNSMRT)*1000/CLOCKS_PER_SEC)/PMAX << " milliseconds per product consumed" << std::endl;
        std::cout << "_______________________________\n" << std::endl;
    }

    pthread_exit(0);
    return 0;
}
