#ifndef GOL_FIELD
#define GOL_FIELD

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
#define MAX(a, b) ((a) > (b) ? a : b)
#define MIN(a, b) ((a) < (b) ? a : b)

#ifdef VTK_OUTPUT

#define VTK_INIT(TIMESTEP) \
    char pathPrefix[1024]; \
    char segmentPrefix[1024]; \
    snprintf(segmentPrefix, sizeof(segmentPrefix), "gol_mtp_%05d", TIMESTEP); \
    snprintf(pathPrefix, sizeof(pathPrefix), "output/"); 

#define VTK_OUTPUT_SEGMENT(CURRENT_FIELD, OFFSET_X, START_X, END_X, OFFSET_Y, START_Y, END_Y) \
    writeVTK2(CURRENT_FIELD, pathPrefix, segmentPrefix, OFFSET_X, START_X, END_X, OFFSET_Y, START_Y, END_Y); \

#define VTK_OUTPUT_MASTER(CURRENT_FIELD, TIMESTEP, TOTAL_WIDTH, SEGMENTS_X, TOTAL_HEIGHT, SEGMENTS_Y) \
    char masterPrefix[1024]; \
    snprintf(masterPrefix, sizeof(masterPrefix), "gol_mtp_%05d", TIMESTEP); \
    writeVTK2Master(CURRENT_FIELD, pathPrefix, masterPrefix, TOTAL_WIDTH, SEGMENTS_X, TOTAL_HEIGHT, SEGMENTS_Y);

#else

#define VTK_INIT(TIMESTEP)
#define VTK_OUTPUT_SEGMENT(CURRENT_FIELD, OFFSET_X, START_X, END_X, OFFSET_Y, START_Y, END_Y)
#define VTK_OUTPUT_MASTER(CURRENT_FIELD, TIMESTEP, TOTAL_WIDTH, SEGMENTS_X, TOTAL_HEIGHT, SEGMENTS_Y)

#endif // VTK_OUTPUT

//
// Field & Initialization
//

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

// current_field, new_field, x, y
typedef void (*kernel_func)(struct Field *, struct Field *, int, int);

// current_field, new_field, timestep
typedef void (*simulate_func)(struct Field *, struct Field *, int);

static inline void initializeField(struct Field *field, int width, int height, int segmentsX, int segmentsY)
{
    field->width = width;
    field->height = height;

    // Calculate optimal cuts if none are given
    if (!(segmentsX && segmentsY))
    {
        int numberThreads = omp_get_max_threads();
        int sqrtNumberThreads = sqrt(numberThreads);

        // numberThreadFactorSmall is the biggest factor of numberThreads <= sqrt(numberThreads)
        int numberThreadFactorSmall;
        for (numberThreadFactorSmall = sqrtNumberThreads; numberThreadFactorSmall > 0; numberThreadFactorSmall--)
        {
            if (numberThreads % numberThreadFactorSmall == 0)
            {
                break;
            }
        }
        // numberThreadFactorSmall * numberThreadFactorLarge == numberThreads && numberThreadFactorSmall <= numberThreadFactorLarge
        int numberThreadFactorLarge = numberThreads / numberThreadFactorSmall;

        if (height > width)
        {
            segmentsX = numberThreadFactorSmall;
            segmentsY = numberThreadFactorLarge;
        }
        else
        {
            segmentsX = numberThreadFactorLarge;
            segmentsY = numberThreadFactorSmall;
        }

#ifdef DEBUG
        printf("Number Threads: %d\n", numberThreads);
        printf("Segments X: %d\n", segmentsX);
        printf("Segments Y: %d\n", segmentsY);
#endif
    }

    field->segmentsX = segmentsX;
    field->segmentsY = segmentsY;
    field->factorX = field->width / (double)field->segmentsX;
    field->factorY = field->height / (double)field->segmentsY;

    // Ghost Layer
    field->width += 2;
    field->height += 2;

#ifdef DEBUG
        printf("Factor X: %lf\n", field->factorX);
        printf("Factor Y: %lf\n", field->factorY);
#endif

    field->field = (FieldType *)calloc(field->width * field->height, sizeof(FieldType));
}

static inline void initializeFieldOther(struct Field *field, struct Field *other)
{
    field->width = other->width;
    field->height = other->height;
    field->segmentsX = other->segmentsX;
    field->segmentsY = other->segmentsY;
    field->factorX = other->factorX;
    field->factorY = other->factorY;

    field->field = (FieldType *)calloc((other->width) * (other->height), sizeof(FieldType));
}

void initializeFields(struct Field *currentField, struct Field *newField, int width, int height, int segmentsX, int segmentsY)
{
    initializeField(currentField, width, height, segmentsX, segmentsY);
    initializeFieldOther(newField, currentField);
}

static inline void fillRandom(struct Field *currentField)
{
    int x, y;
    for (y = 0; y < currentField->height; y++)
    {
        for (x = 0; x < currentField->width; x++)
        {
            if(y == 0)
            {
                currentField->field[calcIndex(currentField->width, x, y)] = 0;
            }
            else if(x == 0)
            {
                currentField->field[calcIndex(currentField->width, x, y)] = 0;
            }
            else if(x == currentField->width - 1)
            {
                currentField->field[calcIndex(currentField->width, x, y)] = 0;
            }
            else if(y == currentField->height - 1)
            {
                currentField->field[calcIndex(currentField->width, x, y)] = 0;
            }
            else
            {
                currentField->field[calcIndex(currentField->width, x, y)] = (rand() < RAND_MAX / 10) ? 1 : 0;
            }
        }
    }
}

//
// (VTK) Helper
//

void writeVTK2(struct Field *data, char pathPrefix[1024], char prefix[1024], int offsetX, int startX, int endX, int offsetY, int startY, int endY)
{
    char filename[2048];
    int x, y;

    float deltax = 1.0;
    long nxy = data->width * data->height * sizeof(float);

    int ret = snprintf(filename, sizeof(filename), "%s%s_%d_%d.vti", pathPrefix, prefix, offsetX + startX, offsetY + startY);
    if (ret < 0) {
         abort();
    }
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"ImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");
    fprintf(fp, "<ImageData WholeExtent=\"%d %d %d %d %d %d\" Origin=\"%d %d %d\" Spacing=\"%le %le %le\">\n",
            offsetX + startX, offsetX + startX + (endX - startX), offsetY + startY, offsetY + startY + (endY - startY), 0, 0,
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

void writeVTK2Master(struct Field *data, char pathPrefix[1024], char prefix[1024], int totalWidth, int segmentsX, int totalHeight, int segmentsY)
{
    if(totalWidth == 0 && totalHeight == 0)
    {
        totalWidth = data->width;
        totalHeight = data->height;
    }

    char filename[2048];

    float deltax = 1.0;
    snprintf(filename, sizeof(filename), "%s%s_master.pvti", pathPrefix, prefix);
    FILE *fp = fopen(filename, "w");

    fprintf(fp, "<?xml version=\"1.0\"?>\n");
    fprintf(fp, "<VTKFile type=\"PImageData\" version=\"0.1\" byte_order=\"LittleEndian\" header_type=\"UInt64\">\n");

    fprintf(fp, "<PImageData WholeExtent=\"%d %d %d %d %d %d\" GhostLevel=\"#\" Origin=\"%d %d %d\" Spacing=\"%le %le %le\">\n",
            0, totalWidth, 0, totalHeight, 0, 0,
            0, 0, 0,
            deltax, deltax, 0.0);
    fprintf(fp, "<PCellData Scalars=\"%s%s\">\n", pathPrefix, prefix);
    fprintf(fp, "<DataArray type=\"Float32\" Name=\"%s%s\" format=\"appended\" offset=\"0\"/>\n", pathPrefix, prefix);
    fprintf(fp, "</PCellData>\n");

    if(segmentsX && segmentsY)
    {
        for (int i = 0; i < data->segmentsX; i++)
        {
            for (int j = 0; j < data->segmentsY; j++)
            {
                int startX = 1 + data->factorX * i + 0.5;
                int startY = 1 + data->factorY * j + 0.5;

                int endX = 1 + data->factorX * (i + 1) + 0.5;
                int endY = 1 + data->factorY * (j + 1) + 0.5;
                fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                        startX, endX, startY, endY,
                        prefix, startX, startY);
            }
        }

        // Write global ghost layer
        // upper and lower (including corners)
        fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                0, data->width, 0, 1,
                prefix, 0, 0);
        fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                0, data->width, data->height - 1, data->height,
                prefix, 0, data->height - 1);

        // left and right (excluding corners)
        fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                0, 1, 1, data->height - 1,
                prefix, 0, 1);
        fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                data->width - 1, data->width, 1, data->height - 1,
                prefix, data->width - 1, 1);
    }
    else
    {
        int endX;
        int endY;
        for (int x = 0; x < totalWidth;)
        {
            for (int y = 0; y < totalHeight;)
            {
                endX = x + (data->width - 2);
                endY = y + (data->height - 2);

                if(x == 0)
                    endX++;
                if(y == 0)
                    endY++;

                if(endX == totalWidth - 1)
                    endX++;
                if(endY == totalHeight - 1)
                    endY++;

                fprintf(fp, "<Piece Extent=\"%d %d %d %d 0 0\" Source=\"%s_%d_%d.vti\"/>\n",
                        x, endX, y, endY,
                        prefix, x, y);
                
                if(x == 0)
                {
                    x++;
                }
                if(y == 0)
                {
                    y++;
                }

                x = endX;
                y = endY;
            }
        }
    }

    fprintf(fp, "</PImageData>\n");
    fprintf(fp, "</VTKFile>\n");
    fclose(fp);
}

void printField(struct Field *currentField)
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

#endif // GOL_FIELD
