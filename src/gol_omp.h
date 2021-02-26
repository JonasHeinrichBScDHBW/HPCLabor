#ifndef GOL_OMP
#define GOL_OMP

#include "gol_field.h"
#include "gol_plain_utils.h"

static inline void simulateStepOMPPlain(struct Field *currentField, struct Field *newField, int timestep)
{
#ifdef OUTPUT_VTK
    char pathPrefix[1024];
    snprintf(pathPrefix, sizeof(pathPrefix), "output/");
#endif

#pragma omp parallel for collapse(2)
    for (int i = 0; i < currentField->segmentsX; i++)
    {
        for (int j = 0; j < currentField->segmentsY; j++)
        {
            int startX = currentField->factorX * i + 0.5;
            int startY = currentField->factorY * j + 0.5;

            int endX = currentField->factorX * (i + 1) + 0.5;
            int endY = currentField->factorY * (j + 1) + 0.5;

#ifdef DEBUG
            if (startX > currentField->width || endX > currentField->width || startY > currentField->height || endY > currentField->height)
            {
#pragma omp critical
                {
                    printf("==========\n");
                    printf("Out of bounds detected\n");
                    printf("Segments X: %d\n", currentField->segmentsX);
                    printf("Segments Y: %d\n", currentField->segmentsY);
                    printf("Factor   X: %lf\n", currentField->factorX);
                    printf("Factor   Y: %lf\n", currentField->factorY);
                    printf("Start    X: %d\n", startX);
                    printf("End      X: %d\n", endX);
                    printf("Start    Y: %d\n", startY);
                    printf("End      Y: %d\n", endY);
                }
            }
#endif

            int x, y;
            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    golKernel(currentField, newField, x, y);
                }
            }

#ifdef OUTPUT_VTK
            char prefix[1024];
            snprintf(prefix, sizeof(prefix), "gol_mtp_%05d", timestep);
            writeVTK2(currentField, pathPrefix, prefix, startX, endX, startY, endY);
#endif
        }
    }

#ifdef OUTPUT_VTK
    char masterPrefix[1024];
    snprintf(masterPrefix, sizeof(masterPrefix), "gol_mtp_%05d", timestep);
    writeVTK2Master(currentField, pathPrefix, masterPrefix);
#endif
}

#endif // GOL_OMP