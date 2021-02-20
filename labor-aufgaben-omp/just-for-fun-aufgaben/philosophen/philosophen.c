#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

// number of philosophers
#define N 5
// left neighbour
#define LEFT  (id)
// right neighbour
#define RIGHT ((id + 1) % num_threads)

#define TRUE  1
#define FALSE 0

// Global variables
int num_threads;
omp_lock_t forks[N];

void think(int philosopher) {
  printf("%d is thinking.\n", philosopher);
}
void eat(int philosopher) {
  printf("%d is eating.\n", philosopher);
}

void philosopher(int id) {
  while(TRUE) {
    think(id);
    // get forks
    
    //TODO let each  philosopher eat with tow forks, the left and right one!  
    //omp_set_lock(&forks[LEFT]);
    //omp_set_lock(&forks[RIGHT]);
    eat(id);
    // put forks
    //omp_unset_lock(&forks[LEFT]);
    //omp_unset_lock(&forks[RIGHT]);
  }
}

int main (int argc, char *argv[]) {
  int i;
  int id;

  for (i = 0; i < N; i++){
    omp_init_lock(&forks[i]);
  }

  omp_set_num_threads(N);
  #pragma omp parallel private(id) shared(num_threads, forks)
  {
    id = omp_get_thread_num();
    num_threads = omp_get_num_threads();

    philosopher(id);
  }

  for (i = 0; i < N; i++){
    omp_destroy_lock(&forks[i]);
  }
  return 0;
}

