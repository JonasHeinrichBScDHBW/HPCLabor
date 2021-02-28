#ifndef GOL_VANILLA
#define GOL_VANILLA

#include "gol_field.h"
#include "gol_plain_utils.h"

static inline void simulateStepVanillaPlain(struct Field *currentField, struct Field *newField, int timestep)
{
    VTK_INIT

    int x, y;
    for (y = 0; y < currentField->height; y++)
    {
        for (x = 0; x < currentField->width; x++)
        {
            golKernel(currentField, newField, x, y);
            VTK_OUTPUT_SEGMENT(timestep, 0, currentField->width, 0, currentField->height)
        }
    }

    VTK_OUTPUT_MASTER(timestep)
}

#endif // GOL_VANILLA
