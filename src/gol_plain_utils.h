#ifndef GOL_PLAIN_UTILS
#define GOL_PLAIN_UTILS

#include "gol_field.h"

static inline int countNeighbors(struct Field *currentField, int x, int y)
{
    int sum = 0;

    sum += currentField->field[calcIndex(currentField->width, x - 1, y - 1)];
    sum += currentField->field[calcIndex(currentField->width, x + 0, y - 1)];
    sum += currentField->field[calcIndex(currentField->width, x + 1, y - 1)];

    sum += currentField->field[calcIndex(currentField->width, x - 1, y + 0)];
    sum += currentField->field[calcIndex(currentField->width, x + 1, y + 0)];

    sum += currentField->field[calcIndex(currentField->width, x - 1, y + 1)];
    sum += currentField->field[calcIndex(currentField->width, x + 0, y + 1)];
    sum += currentField->field[calcIndex(currentField->width, x + 1, y + 1)];

    return sum;
}

static inline bool golKernel(struct Field *currentField, struct Field *newField, int x, int y)
{
    int n = countNeighbors(currentField, x, y);

    FieldType value = (n == 3 || (n == 2 && currentField->field[calcIndex(currentField->width, x, y)]));

    // Either 3 neighbors or 2 neighbors and alive
    newField->field[calcIndex(currentField->width, x, y)] = value;

    return value != currentField->field[calcIndex(currentField->width, x, y)];
}

#endif // GOL_PLAIN_UTILS
