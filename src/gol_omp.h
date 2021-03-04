#ifndef GOL_OMP
#define GOL_OMP

#include "gol_field.h"
#include "gol_plain_utils.h"

static inline void simulateStepOMPPlain(struct Field *currentField, struct Field *newField, int timestep)
{
    VTK_INIT

    #pragma omp parallel for collapse(2)
    for (int i = 0; i < currentField->segmentsX; i++)
    {
        for (int j = 0; j < currentField->segmentsY; j++)
        {
            int startX = currentField->factorX * i + 0.5;
            int startY = currentField->factorY * j + 0.5;

            int endX = currentField->factorX * (i + 1) + 0.5;
            int endY = currentField->factorY * (j + 1) + 0.5;

            int x, y;
            for (y = startY; y < endY; y++)
            {
                for (x = startX; x < endX; x++)
                {
                    golKernel(currentField, newField, x, y);
                }
            }

            VTK_OUTPUT_SEGMENT(currentField, timestep, startX, endX, startY, endY)
        }
    }

    VTK_OUTPUT_MASTER(currentField, timestep)
}

#endif // GOL_OMP