#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <omp.h>

#define NUMITER 26

#define TRUE 1
#define FALSE 0

typedef struct parallelstack {
  omp_lock_t stacklock;  //lock for accessing the stack
  int cancel;            //flag that indicates if threads should stop working
  char *buffer;          //stack elements
  int size;              //size of the stack
  int count;             //current position in the stack
} ParallelStack;

static inline ParallelStack* newParallelStack() {
  return calloc(1, sizeof(ParallelStack));
}

static inline ParallelStack* ParallelStack_init(ParallelStack* pq, int size) {
	
	//TODO
	
  return pq;
}

static inline ParallelStack* ParallelStack_deinit(ParallelStack* pq) {

	//TODO
	
  return pq;
}

static inline ParallelStack* freeParallelStack(ParallelStack* pq) {
  free(pq);
  return pq;
}

static int ParallelStack_put(ParallelStack* pq, char item) {
  int writtenChars = FALSE; // TRUE if the stack was abel to put the data, FALSE if the stack is full, the data will be rejected

	//TODO

  return writtenChars;
}

int ParallelStack_get(ParallelStack* pq, char *c) {
	int numReadedChars = 0; // TRUE if the stack was abel to get the data, FALSE if the stack is empty
	
	
		//TODO

	
  return numReadedChars;
}

void ParallelStack_setCanceled(ParallelStack* pq) {

		//TODO

}


int ParallelStack_isCanceled(ParallelStack* pq) {
  int canceled = FALSE;
  
		//TODO

  return canceled;
}

/////////////////////////////////////////
// DO NOT EDIT BEYOND THIS LINE !!!!
/////////////////////////////////////////

void producer(int tid, ParallelStack* pq) {
  int i = 0;
  char item;
  while( i < NUMITER) {
    item = 'A' + (i % 26);
    
    if ( ParallelStack_put(pq, item) == 1) {
      i++;
      printf("->Thread %d is Producing %c ...\n",tid, item);
    }
    //sleep(1);
   }
   ParallelStack_setCanceled(pq);
}


void consumer(int tid, ParallelStack* pq)
{
  char item;
  while( ParallelStack_isCanceled(pq) == FALSE) {

    if (ParallelStack_get(pq, &item) == 1) {
      printf("<-Thread %d is Consuming %c\n",tid, item);
    }
    sleep(2);
  }
}

int main()
{
    int tid;
    ParallelStack* pq = ParallelStack_init(newParallelStack(), 5);

    #pragma omp parallel private(tid) num_threads(4) 
    {
       tid=omp_get_thread_num();

       if(tid==1) 
       {
         producer(tid, pq);
       } else 
       {
         consumer(tid, pq);
       }
    }
    
    freeParallelStack(ParallelStack_deinit(pq));
    
    return 0;
}
