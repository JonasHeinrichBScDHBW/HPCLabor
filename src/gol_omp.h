#ifndef GOL_OMP
#define GOL_OMP

#include "gol_field.h"
#include "gol_plain_utils.h"

static inline void simulateStepOMPPlain(struct Field *currentField, struct Field *newField, int timestep)
{
    VTK_INIT(timestep)

    #pragma omp parallel
    {
        int num = omp_get_thread_num();

        //   0   1   2   3
        // | 0 | 1 | 2 | 3 |   0 
        // | 4 | 5 | 6 | 7 |   1

        // 3 % 3 = 0
        int i = num % currentField->segmentsX;
        // 5 / 3 = 1
        int j = num / currentField->segmentsX;

        printf("(Thread: %d) i: %d\n", num, i);
        printf("(Thread: %d) j: %d\n", num, j);


        int startX = 1 + currentField->factorX * i + 0.5;
        int startY = 1 + currentField->factorY * j + 0.5;

        int endX = 1 + currentField->factorX * (i + 1) + 0.5;
        int endY = 1 + currentField->factorY * (j + 1) + 0.5;

        int x, y;
        for (y = startY; y < endY; y++)
        {
            for (x = startX; x < endX; x++)
            {
                golKernel(currentField, newField, x, y);
            }
        }

        VTK_OUTPUT_SEGMENT(currentField, 0, startX, endX, 0, startY, endY)
    }

    // Write global ghost layer
    // upper and lower (including corners)
    VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, 0, 1)
    VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, currentField->height - 1, currentField->height)

    // left and right (excluding corners)
    VTK_OUTPUT_SEGMENT(currentField, 0, 0, 1,                                         0, 1, currentField->height - 1)
    VTK_OUTPUT_SEGMENT(currentField, 0, currentField->width - 1, currentField->width, 0, 1, currentField->height - 1)

    VTK_OUTPUT_MASTER(currentField, timestep, currentField->width, currentField->segmentsX, currentField->height, currentField->segmentsY)
}

static inline void simulateStepOMPParallelFor(struct Field *currentField, struct Field *newField, int timestep)
{
    VTK_INIT(timestep)

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < currentField->segmentsX; i++)
    {
        for (int j = 0; j < currentField->segmentsY; j++)
        {
            int startX = 1 + currentField->factorX * i + 0.5;
            int startY = 1 + currentField->factorY * j + 0.5;

            int endX = 1 + currentField->factorX * (i + 1) + 0.5;
            int endY = 1 + currentField->factorY * (j + 1) + 0.5;

            int x, y;
            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    golKernel(currentField, newField, x, y);
                }
            }

            VTK_OUTPUT_SEGMENT(currentField, 0, startX, endX, 0, startY, endY)
        }
    }

    // Write global ghost layer
    // upper and lower (including corners)
    VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, 0, 1)
    VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, currentField->height - 1, currentField->height)

    // left and right (excluding corners)
    VTK_OUTPUT_SEGMENT(currentField, 0, 0, 1,                                         0, 1, currentField->height - 1)
    VTK_OUTPUT_SEGMENT(currentField, 0, currentField->width - 1, currentField->width, 0, 1, currentField->height - 1)

    VTK_OUTPUT_MASTER(currentField, timestep, currentField->width, currentField->segmentsX, currentField->height, currentField->segmentsY)
}

#endif // GOL_OMP