#ifndef GOL_PLAIN_UTILS
#define GOL_PLAIN_UTILS

#include "gol_field.h"

static inline int countNeighbors(struct Field *currentField, int x, int y)
{
    int sum = 0;

    for (int y1 = y - 1; y1 <= y + 1; y1++)
    {
        for (int x1 = x - 1; x1 <= x + 1; x1++)
        {
            sum += currentField->field[calcIndex(currentField->width,
                                                 (x1 + currentField->width) % currentField->width,
                                                 (y1 + currentField->height) % currentField->height)];
        }
    }
    sum -= currentField->field[calcIndex(currentField->width, x, y)];
    return sum;
}

void golKernel(struct Field *currentField, struct Field *newField, int x, int y)
{
    int n = countNeighbors(currentField, x, y);

    // Either 3 neighbors or 2 neighbors and alive
    newField->field[calcIndex(currentField->width, x, y)] = (n == 3 || (n == 2 && currentField->field[calcIndex(currentField->width, x, y)]));
}

#endif // GOL_PLAIN_UTILS
