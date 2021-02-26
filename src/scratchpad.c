#include <stdio.h>
#include <omp.h>

void deadlock() {
    printf("Before the barrier (%d)\n", omp_get_thread_num());
    fflush(stdout);
    #pragma omp barrier
    printf("After the barrier (%d)\n", omp_get_thread_num());
    fflush(stdout);
}

int main() {
    int n = omp_get_max_threads() + 1;

    printf("Start\n");
    fflush(stdout);

    #pragma omp parallel for num_threads(n - 1) shared(n)
    for(int i = 0; i < n; i++)
    {
        deadlock();
    }

    printf("Never reached\n");
    fflush(stdout);
    
    return 0;
}
