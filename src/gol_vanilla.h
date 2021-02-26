#ifndef GOL_VANILLA
#define GOL_VANILLA

#include "gol_field.h"
#include "gol_plain_utils.h"

static inline void simulateStepVanillaPlain(struct Field *currentField, struct Field *newField, int timestep)
{
#ifdef OUTPUT_VTK
    char pathPrefix[1024];
    snprintf(pathPrefix, sizeof(pathPrefix), "output/");
#endif

    int x, y;
    for (y = 0; y < currentField->height; y++)
    {
        for (x = 0; x < currentField->width; x++)
        {
            golKernel(currentField, newField, x, y);
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

#endif // GOL_VANILLA
