#ifdef GOL_VERSION_MPI
#include "mpi.h"
#include "gol_mpi.h"
#define simulateStepFunction simulateStepMPI1DPlain
#else
#define simulateStepFunction simulateStepOMPPlain
#endif // GOL_VERSION_MPI

#include "gol_field.h"
#include "gol_vanilla.h"
#include "gol_omp.h"

void simulateSteps(int timesteps, struct Field *currentField, struct Field *newField, simulate_func simulateFunction)
{
    long t;
    for (t = 0; t < timesteps; t++)
    {
        #ifdef DEBUG
            printf("Timestep: %ld\n", t);
            // printField(currentField);
            usleep(200000);
        #endif

        simulateFunction(currentField, newField, t);

        // SWAP
        struct Field *temp = currentField;
        currentField = newField;
        newField = temp;
    }
}

void runSimulation(int timesteps, int width, int height, int segmentsX, int segmentsY)
{
    struct Field currentField;
    struct Field newField;
    initializeFields(&currentField, &newField, width, height, segmentsX, segmentsY);

    fillRandom(&currentField);
    simulateSteps(timesteps, &currentField, &newField, &simulateStepFunction);

    free(currentField.field);
    free(newField.field);
}

int main(int argc, char **argv)
{
    #ifdef GOL_VERSION_MPI
        MPI_Init(&argc, &argv);
    #endif
    int timesteps = 0, width = 0, height = 0, segmentsX = 0, segmentsY = 0;

    // 500 1024 1024 takes about 25s on one thread

    // Read width
    if (argc > 1)
        timesteps = atoi(argv[1]);
    if (argc > 2)
        width = atoi(argv[2]);
    if (argc > 3)
        height = atoi(argv[3]);
    if (argc > 4)
        segmentsX = atoi(argv[4]);
    if (argc > 5)
        segmentsY = atoi(argv[5]);

    #ifdef DEBUG
        printf("===============================\n");
        printf("Main arguments:\n");
        printf("Timesteps: %d\n", timesteps);
        printf("Width: %d\n", width);
        printf("Height: %d\n", height);
        printf("Segments X: %d\n", segmentsX);
        printf("Segments Y: %d\n", segmentsY);
    #endif

    // Default values
    if (timesteps <= 0)
        timesteps = 100;
    if (width <= 0)
        width = 32;
    if (height <= 0)
        height = 32;

    #ifdef DEBUG
        printf("===============================\n");
        printf("Actual Program Parameters:\n");
        printf("Timesteps: %d\n", timesteps);
        printf("Width: %d\n", width);
        printf("Height: %d\n", height);
        printf("Segments X: %d\n", segmentsX);
        printf("Segments Y: %d\n", segmentsY);
        printf("===============================\n");
    #endif

    runSimulation(timesteps, width, height, segmentsX, segmentsY);

    #ifdef GOL_VERSION_MPI
        MPI_Finalize();
    #endif

    #ifdef DEBUG
        printf("Done\n");
    #endif

    return 0;
}
