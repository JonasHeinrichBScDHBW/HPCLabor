#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <omp.h>

#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>

#define calcIndex(width, x, y) ((y) * (width) + (x))
#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

#define DEBUG
#undef DEBUG

#define OUTPUT_VTK
#undef OUTPUT_VTK

typedef char FieldType;
struct Field
{
    int width;
    int height;

    int segmentsX;
    int segmentsY;
    double factorX;
    double factorY;

    FieldType *field;
};

typedef void (*kernel_t)(struct Field *, struct Field *, int, int);

void writeVTK2(struct Field *data, char pathPrefix[1024], char prefix[1024], int startX, int endX, int startY, int endY)
{
    char filename[2048];
    int x, y;

    float deltax = 1.0;
    long nxy = data->width * data->height * sizeof(float);

    snprintf(filename, sizeof(filename), "%s%s_%d_%d.vti", pathPrefix, prefix, startX, startY);
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"%d %d %d\" Spacing=\"%le %le %le\">\n", 
        startX, startX + (endX - startX), startY, startY + (endY - startY), 0, 0,
        startX, startY, 0,
        deltax, deltax, 0.0);
    fprintf(fp, "<CellData Scalars=\"%s\">\n", prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s\" format=\"appended\" offset=\"0\"/>\n", prefix);
    fprintf(fp, "</CellData>\n");
    fprintf(fp, "</ImageData>\n");
    fprintf(fp, "<AppendedData encoding=\"raw\">\n");
    fprintf(fp, "_");
    fwrite((unsigned char *)&nxy, sizeof(long), 1, fp);

    for (y = startY; y < endY; y++)
    {
        for (x = startX; x < endX; x++)
        {
            float value = (float)data->field[calcIndex(data->width, x, y)];
            fwrite((unsigned char *)&value, sizeof(float), 1, fp);
        }
    }

    fprintf(fp, "\n</AppendedData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void writeVTK2Master(struct Field *data, char pathPrefix[1024], char prefix[1024])
{
    char filename[2048];

    float deltax = 1.0;
    snprintf(filename, sizeof(filename), "%s%s_master.pvti", pathPrefix, prefix);
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"PImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    
    fprintf(fp, "<PImageData WholeExtent=\"%d %d %d %d %d %d\" GhostLevel=\"#\" Origin=\"%d %d %d\" Spacing=\"%le %le %le\">\n", 
        0, data->width, 0, data->height, 0, 0,
        0, 0, 0,
        deltax, deltax, 0.0);
    fprintf(fp, "<PCellData Scalars=\"%s%s\">\n", pathPrefix, prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s%s\" format=\"appended\" offset=\"0\"/>\n", pathPrefix, prefix);
    fprintf(fp, "</PCellData>\n");

    for(int i = 0; i < data->segmentsX; i++)
    {
        for(int j = 0; j < data->segmentsY; j++)
        {
            int startX = data->factorX * i + 0.5;
            int startY = data->factorY * j + 0.5;

            int endX = data->factorX * (i + 1) + 0.5;
            int endY = data->factorY * (j + 1) + 0.5;
            fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                startX, endX, startY, endY,
                prefix, startX, startY
            );
        }
    }

    fprintf(fp, "</PImageData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void show(struct Field *currentField)
{
    printf("\033[H");
    int x, y;
    for (y = 0; y < currentField->height; y++)
    {
        for (x = 0; x < currentField->width; x++)
            printf(currentField->field[calcIndex(currentField->width, x, y)] ? "\033[07m  \033[m" : "  ");
        //printf("\033[E");
        printf("\n");
    }
    fflush(stdout);
}

int countNeighbors(struct Field *currentField, int x, int y)
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

void simulateStep(struct Field *currentField, struct Field *newField, int timestep, kernel_t kernelFunction)
{
    // Original
    // for (y = 0; y < currentField->height; y++)
    // {
    //     for (x = 0; x < currentField->width; x++)
    //     {
    //         kernelFunction(currentField, newField, x, y);
    //     }
    // }

#ifdef OUTPUT_VTK
    char pathPrefix[1024];
    snprintf(pathPrefix, sizeof(pathPrefix), "output/");
#endif

    #pragma omp parallel for collapse(2)
    for(int i = 0; i < currentField->segmentsX; i++)
    {
        for(int j = 0; j < currentField->segmentsY; j++)
        {  
            int startX = currentField->factorX * i + 0.5;
            int startY = currentField->factorY * j + 0.5;

            int endX = currentField->factorX * (i + 1) + 0.5;
            int endY = currentField->factorY * (j + 1) + 0.5;

#ifdef DEBUG
            if(startX > currentField->width || endX > currentField->width || startY > currentField->height || endY > currentField->height)
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

                    // NOTE: This causes segfaults and data races
                    // kernelFunction(currentField, newField, x, y);
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

static inline void fill(struct Field *currentField)
{
    int i;
    for (i = 0; i < currentField->width * currentField->height; i++)
    {
        currentField->field[i] = (rand() < RAND_MAX / 10) ? 1 : 0;
    }
}

static inline void initializeField(struct Field *field, int width, int height, int segmentsX, int segmentsY)
{
    field->width = width;
    field->height = height;

    // Calculate optimal cuts if none are given
    if(!(segmentsX && segmentsY))
    {
        int numberThreads = omp_get_max_threads();
        int sqrtNumberThreads = sqrt(numberThreads);

        // numberThreadFactorSmall is the biggest factor of numberThreads <= sqrt(numberThreads)
        int numberThreadFactorSmall;
        for (numberThreadFactorSmall = sqrtNumberThreads; numberThreadFactorSmall > 0; numberThreadFactorSmall--) {
            if (numberThreads % numberThreadFactorSmall == 0) {
                break;
            }
        }
        // numberThreadFactorSmall * numberThreadFactorLarge == numberThreads && numberThreadFactorSmall <= numberThreadFactorLarge
        int numberThreadFactorLarge = numberThreads / numberThreadFactorSmall;

        if(height > width) {
            segmentsX = numberThreadFactorSmall;
            segmentsY = numberThreadFactorLarge;
        }
        else {
            segmentsX = numberThreadFactorLarge;
            segmentsY = numberThreadFactorSmall;
        }
    }

    field->segmentsX = segmentsX;
    field->segmentsY = segmentsY;
    field->factorX = field->width / (double)field->segmentsX;
    field->factorY = field->height / (double)field->segmentsY;

    field->field = calloc(width * height, sizeof(FieldType));
}

static inline void initializeFieldOther(struct Field *field, struct Field *other)
{
    field->width = other->width;
    field->height = other->height;
    field->segmentsX = other->segmentsX;
    field->segmentsY = other->segmentsY;
    field->factorX = other->factorX;
    field->factorY = other->factorY;

    field->field = calloc(other->width * other->height, sizeof(FieldType));
}

void simulateSteps(int timesteps, struct Field *currentField, struct Field *newField)
{
    long t;
    for (t = 0; t < timesteps; t++)
    {
        simulateStep(currentField, newField, t, &golKernel);

#ifdef DEBUG
        printf("Timestep: %ld\n", t);
        show(&currentField);
        usleep(200000);
#endif

        // SWAP
        struct Field *temp = currentField;
        currentField = newField;
        newField = temp;
    }
}


void simulateField(int timesteps, int width, int height, int segmentsX, int segmentsY)
{
    struct Field currentField;
    initializeField(&currentField, width, height, segmentsX, segmentsY);
    struct Field newField;
    initializeFieldOther(&newField, &currentField);

    fill(&currentField);
    simulateSteps(timesteps, &currentField, &newField);

#ifdef DEBUG
    printf("Done\n");
#endif

    free(currentField.field);
    free(newField.field);
}

int main(int c, char **argv)
{
    int timesteps = 0, width = 0, height = 0, segmentsX = 0, segmentsY = 0;

    // 1000 1000 1000 takes about 25s on one thread

    // Read width
    if (c > 1)
        timesteps = atoi(argv[1]);
    if (c > 2)
        width = atoi(argv[2]);
    if (c > 3)
        height = atoi(argv[2]);
    if (c > 4)
        segmentsX = atoi(argv[2]);
    if (c > 5)
        segmentsY = atoi(argv[2]);

    // Default values
    if (timesteps <= 0)
        timesteps = 100;
    if (width <= 0)
        width = 30;
    if (height <= 0)
        height = 30;

    simulateField(timesteps, width, height, segmentsX, segmentsY);

    return 0;
}
