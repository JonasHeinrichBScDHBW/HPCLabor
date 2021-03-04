#include "gol_field.h"
#include "gol_vanilla.h"
#include "gol_omp.h"
#include "gol_mpi.h"

void simulateSteps(int timesteps, struct Field *currentField, struct Field *newField, simulate_func simulateFunction)
{
    long t;
    for (t = 0; t < timesteps; t++)
    {
        simulateFunction(currentField, newField, t);

#ifdef DEBUG
        printf("Timestep: %ld\n", t);
        printField(&currentField);
        usleep(200000);
#endif

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
    simulateSteps(timesteps, &currentField, &newField, &simulateStepOMPPlain);

#ifdef DEBUG
    printf("Done\n");
#endif

    free(currentField.field);
    free(newField.field);
}

int main(int c, char **argv)
{
    int timesteps = 0, width = 0, height = 0, segmentsX = 0, segmentsY = 0;

    // 500 1024 1024 takes about 25s on one thread

    // Read width
    if (c > 1)
        timesteps = atoi(argv[1]);
    if (c > 2)
        width = atoi(argv[2]);
    if (c > 3)
        height = atoi(argv[3]);
    if (c > 4)
        segmentsX = atoi(argv[4]);
    if (c > 5)
        segmentsY = atoi(argv[5]);

    // Default values
    if (timesteps <= 0)
        timesteps = 100;
    if (width <= 0)
        width = 1026;
    if (height <= 0)
        height = 1026;

    runSimulation(timesteps, width, height, segmentsX, segmentsY);

    return 0;
}
