#ifndef GOL_MPI
#define GOL_MPI

#include "mpi.h"

#include "gol_field.h"
#include "gol_plain_utils.h"

#define ROOT_RANK 0

static inline bool coordinatedStop(int rank, int size, MPI_Comm comm, bool rawStop)
{
    static bool initialized = false;
    static char *stopReceiveBuffer;

    if(!initialized)
    {
        if(rank == ROOT_RANK)
        {
            stopReceiveBuffer = (char *)malloc(size * sizeof(char));
        }

        initialized = true;
    }

    char stop = (char)rawStop;
    #ifdef DEBUG
        printf("(Rank: %d) stop: %d\n", rank, stop);
    #endif

    MPI_Gather(
        &stop, 1, MPI_CHAR,
        stopReceiveBuffer, 1, MPI_CHAR,
        ROOT_RANK, comm
    );

    bool stopCommand;
    if(rank == ROOT_RANK)
    {
        stopCommand = (char)true;
        int i;
        for(i = 0; i < size && stopCommand; i++)
        {
            stopCommand = stopCommand && stopReceiveBuffer[i];
        }
    }
    MPI_Bcast(&stopCommand, 1, MPI_CHAR, ROOT_RANK, comm);

    #ifdef DEBUG
        printf("(Rank: %d) stopCommand: %d\n", rank, stopCommand);
    #endif

    if(stopCommand && rank == ROOT_RANK)
    {
        free(stopReceiveBuffer);
        initialized = false;
    }

    return stopCommand;
}

static inline void simulateStepMPI1DPlain(struct Field *currentField, struct Field *newField, int timestep)
{
    // NOTE: We ignore the segments and factors here, however they could be used to use OpenMP as a
    //       second parallelisation layer on a cluster.

    enum DIRECTIONS {LEFT, RIGHT};
    static bool initialised = false;

    static int rank;
    static int size;
    static MPI_Comm comm;
    static int neighborRanks[2] = {0, 0};

    static MPI_Request receiveRequests[2];
    static MPI_Status receiveStatus[2];
    static MPI_Request sendRequests[2];
    static MPI_Status sendStatus[2];

    static char *leftSendBuffer;
    static char *leftReceiveBuffer;
    static char *rightSendBuffer;
    static char *rightReceiveBuffer;

    if(!initialised)
    {
        MPI_Comm_size(MPI_COMM_WORLD, &size);

        int dims[1] = {0};
        MPI_Dims_create(size, 1, dims);
        int periods[1] = {true};
        int reorder = (int)true;
        // Create 1D virtual topology
        MPI_Cart_create(
            MPI_COMM_WORLD,
            1,
            dims,
            periods,
            reorder,
            &comm
        );
        MPI_Cart_shift(
            comm,
            0, // dimension
            1, // displacement
            &neighborRanks[LEFT],
            &neighborRanks[RIGHT]
        );
        
        MPI_Comm_rank(comm, &rank);

        leftSendBuffer = (char*)malloc(currentField->height * sizeof(char));
        leftReceiveBuffer = (char*)malloc(currentField->height * sizeof(char));
        rightSendBuffer = (char*)malloc(currentField->height * sizeof(char));
        rightReceiveBuffer = (char*)malloc(currentField->height * sizeof(char));

        #ifdef DEBUG
            printf("(Rank %d): size: %d\n", rank, size);
            printf("(Rank %d): rightNeighbor: %d\n", rank, neighborRanks[RIGHT]);
            printf("(Rank %d): leftNeighbor: %d\n", rank, neighborRanks[LEFT]);
        #endif

        initialised = true;
    }

    VTK_INIT(timestep)

    // Initialize borders
    if(timestep == 0)
    {
        MPI_Irecv(leftReceiveBuffer, currentField->height, MPI_CHAR, neighborRanks[LEFT], 2, comm, &receiveRequests[LEFT]);
        MPI_Irecv(rightReceiveBuffer, currentField->height, MPI_CHAR, neighborRanks[RIGHT], 1, comm, &receiveRequests[RIGHT]);

        for(int y = 0; y < currentField->height; y++)
        {
            leftSendBuffer[y] = currentField->field[calcIndex(currentField->width, 1, y)];
            rightSendBuffer[y] = currentField->field[calcIndex(currentField->width, currentField->width - 2, y)];
        }

        MPI_Isend(leftSendBuffer, currentField->height, MPI_CHAR, neighborRanks[LEFT], 1, comm, &sendRequests[LEFT]);
        MPI_Isend(rightSendBuffer, currentField->height, MPI_CHAR, neighborRanks[RIGHT], 2, comm, &sendRequests[RIGHT]);

        int index;
        for(int i = 0; i < 2; i++)
        {
            MPI_Waitany(2, receiveRequests, &index, receiveStatus);

            for(int y = 0; y < currentField->height; y++)
            {
                if(index == LEFT)
                {
                    currentField->field[calcIndex(currentField->width, 0, y)] = leftReceiveBuffer[y];
                }
                else
                {
                    currentField->field[calcIndex(currentField->width, currentField->width - 1, y)] = rightReceiveBuffer[y];
                }
            }
        }

        MPI_Waitall(2, sendRequests, sendStatus);
    }

    MPI_Irecv(leftReceiveBuffer, currentField->height, MPI_CHAR, neighborRanks[LEFT], 2, comm, &receiveRequests[LEFT]);
    MPI_Irecv(rightReceiveBuffer, currentField->height, MPI_CHAR, neighborRanks[RIGHT], 1, comm, &receiveRequests[RIGHT]);

    int x, y;
    bool delta = false;
    bool temp;
    for (y = 1; y < currentField->height - 1; y++)
    {
        for (x = 1; x < currentField->width - 1; x++)
        {
            temp = golKernel(currentField, newField, x, y);
            delta = delta || temp;
        }
    }

    for(int y = 0; y < currentField->height; y++)
    {
        leftSendBuffer[y] = newField->field[calcIndex(currentField->width, 1, y)];
        rightSendBuffer[y] = newField->field[calcIndex(currentField->width, currentField->width - 2, y)];
    }

    MPI_Isend(leftSendBuffer, currentField->height, MPI_CHAR, neighborRanks[LEFT], 1, comm, &sendRequests[LEFT]);
    MPI_Isend(rightSendBuffer, currentField->height, MPI_CHAR, neighborRanks[RIGHT], 2, comm, &sendRequests[RIGHT]);

    // Copy ghost layer between oneself
    // destination, source, count
    memcpy(&newField->field[calcIndex(currentField->width, 0, 0)], &newField->field[calcIndex(currentField->width, 0, currentField->height - 2)], newField->width);
    memcpy(&newField->field[calcIndex(currentField->width, 0, currentField->height - 1)], &newField->field[calcIndex(currentField->width, 0, 1)], newField->width);

    // Write back ghost layer changes
    int index;
    for(int i = 0; i < 2; i++)
    {
        MPI_Waitany(2, receiveRequests, &index, receiveStatus);

        for(int y = 0; y < currentField->height; y++)
        {
            if(index == LEFT)
            {
                newField->field[calcIndex(currentField->width, 0, y)] = leftReceiveBuffer[y];
            }
            else
            {
                newField->field[calcIndex(currentField->width, currentField->width - 1, y)] = rightReceiveBuffer[y];
            }
        }
    }

    // Wait for send requests to arrive
    MPI_Waitall(2, sendRequests, sendStatus);

    // NOTE: For testing purposes: Stops at a timestep, where all ranks are a divisor of (timestep + 1)
    // bool stop = false;
    // if(rank == 0 || rank == 1)
    //     stop = true;
    // else if((timestep + 1) % rank == 0)
    //     stop = true;

    // Stop if no delta
    bool stop = !delta;

    // Exit gracefully
    if(coordinatedStop(rank, size, comm, stop))
    {
        #ifdef DEBUG
            printf("(Rank: %d) Stopping.\n", rank);
        #endif
        free(leftSendBuffer);
        free(leftReceiveBuffer);
        free(rightSendBuffer);
        free(rightReceiveBuffer);

        MPI_Finalize();
        exit(0);
    }

    #ifdef DEBUG
        // Output
        if(size == 1)
        {
            VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, 0, currentField->height)
        }
        else
        {
            // First segment
            if(rank == 0)
            {
                VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width - 1, 0, 0, currentField->height)
            }
            // Last Segment
            else if(rank == size - 1)
            {
                int offset = (currentField->width - 2) * rank;
                VTK_OUTPUT_SEGMENT(currentField, offset, 1, currentField->width, 0, 0, currentField->height)
            }
            // Other
            else
            {
                int offset = (currentField->width - 2) * rank;
                VTK_OUTPUT_SEGMENT(currentField, offset, 1, currentField->width - 1, 0, 0, currentField->height)
            }
        }
        if(rank == (timestep % size))
        {
            VTK_OUTPUT_MASTER(currentField, timestep, 2 + (currentField->width - 2) * size, 0, currentField->height, 0)
        }
    #endif
}

#endif // GOL_MPI
