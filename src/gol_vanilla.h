#ifndef GOL_VANILLA
#define GOL_VANILLA

#include <assert.h>
#include <stdint.h>

#include "gol_field.h"
#include "gol_plain_utils.h"
#include "simd_utils.h"

static inline void simulateStepVanillaPlain(struct Field *currentField, struct Field *newField, int timestep)
{
    VTK_INIT(timestep)

    int x, y;
    for (y = 1; y < currentField->height - 1; y++)
    {
        for (x = 1; x < currentField->width - 1; x++)
        {
            golKernel(currentField, newField, x, y);
        }
    }

    VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, 0, currentField->height)
    VTK_OUTPUT_MASTER(currentField, timestep, currentField->width, 0, currentField->height, 0)
}


static inline void simulateStepVanillaSIMD(struct Field *currentField, struct Field *newField, int timestep)
{
    // TODO: Increase efficiency of AVX512 version - it's currently slower than AVX2

    // Assumes that FieldType is a char
    assert(sizeof(FieldType) == 1);

    VTK_INIT(timestep)

    static bool constArraysInitialized = false;

    #ifdef AVX_512
    static const int32_t constInt8_0 = 0x00000000;
    static const int32_t constInt8_1 = 0x01010101;
    static ALIGN(int constInt8Array_0[REGISTER_SIZE_INT32]);
    static ALIGN(int constInt8Array_1[REGISTER_SIZE_INT32]);
    if(!constArraysInitialized)
    {
        int i;
        for(i = 0; i < REGISTER_SIZE_INT32; i++)
        {
            constInt8Array_0[i] = constInt8_0;
            constInt8Array_1[i] = constInt8_1;
        }
    }
    STATIC_REGISTER const register_int constRegister_0 = loadAlignedMemory(&constInt8Array_0);
    STATIC_REGISTER const register_int constRegister_1 = loadAlignedMemory(&constInt8Array_1);
    #endif

    static const int32_t constInt8_2 = 0x02020202;
    static const int32_t constInt8_3 = 0x03030303;
    static ALIGN(int constInt8Array_2[REGISTER_SIZE_INT32]);
    static ALIGN(int constInt8Array_3[REGISTER_SIZE_INT32]);
    if(!constArraysInitialized)
    {
        int i;
        for(i = 0; i < REGISTER_SIZE_INT32; i++)
        {
            constInt8Array_2[i] = constInt8_2;
            constInt8Array_3[i] = constInt8_3;
        }

        constArraysInitialized = true;
    }
    STATIC_REGISTER const register_int constRegister_2 = loadAlignedMemory(&constInt8Array_2);
    STATIC_REGISTER const register_int constRegister_3 = loadAlignedMemory(&constInt8Array_3);

    // Goal: Always load sequential memory
    //
    // Since this code is constructed to generalize to a shared memory scenario
    // (omp), it cannot be guaranteed that the outermost cells outer neighbors
    // have sequential memory addresses.
    //
    // We therefore have to compute these borders the old fashioned way.
    //
    // | width                                       |
    // | SISD Segment Left |  SIMD Segment           |
    // | rest              |  INT8_REGISTER_SIZE * n |

    int sisdSegmentLeftStart = 1;
    int sisdSegmentLeftEnd = 1 + ((currentField->width - 2) % REGISTER_SIZE_INT8);
    int simdSegmentStart = sisdSegmentLeftEnd;

#ifdef DEBUG
    printf("=======================================\n");
    printf("Width %d\n", currentField->width);
    printf("SISD Segment Left Start: %d\n", sisdSegmentLeftStart);
    printf("SISD Segment Left End: %d\n", sisdSegmentLeftEnd);
    printf("SIMD Segment Start: %d\n", simdSegmentStart);
#endif

    int x, y;

    // SISD Segment Left
    for (y = 1; y < currentField->height - 1; y++)
    {
        for (x = sisdSegmentLeftStart; x < sisdSegmentLeftEnd; x++)
        {
            golKernel(currentField, newField, x, y);
        }
    }

    // SIMD Calculations

    register_int neighborSum;

    // Neighbor Registers (Compass)
    // NW |  N | NE
    //  W |  a |  E
    // SW |  S | SE
    FieldType* neighborNWAdress;
    register_int neighborNW;
    register_int neighborN;
    register_int neighborNE;

    FieldType* neighborWAdress;
    register_int neighborW;
    register_int aliveRegister;
    register_int neighborE;

    FieldType* neighborSWAdress;
    register_int neighborSW;
    register_int neighborS;
    register_int neighborSE;

    #ifdef AVX_512
    register_mask cmpAliveMask;
    register_mask cmp2Mask;
    register_mask cmp3Mask;
    register_mask resultMask;
    #else
    register_int cmp2Register;
    register_int cmp3Register;
    #endif // AVX_512

    register_int resultRegister;
    for (y = 1; y < currentField->height - 1; y++)
    {
        for (x = simdSegmentStart; x < currentField->width - 1; x += REGISTER_SIZE_INT8)
        {
            neighborNWAdress = &currentField->field[calcIndex(currentField->width, x - 1, y - 1)];
            neighborNW = loadUnalignedMemory(neighborNWAdress);
            neighborN = loadUnalignedMemory(neighborNWAdress + sizeof(FieldType));
            neighborNE = loadUnalignedMemory(neighborNWAdress + sizeof(FieldType) * 2);
            neighborSum = addRegisterInt8(neighborNW, neighborN);
            neighborSum = addRegisterInt8(neighborSum, neighborNE);

            neighborWAdress = &currentField->field[calcIndex(currentField->width, x - 1, y)];
            neighborW = loadUnalignedMemory(neighborWAdress);
            aliveRegister = loadUnalignedMemory(neighborWAdress + sizeof(FieldType));
            neighborE = loadUnalignedMemory(neighborWAdress + sizeof(FieldType) * 2);
            neighborSum = addRegisterInt8(neighborSum, neighborW);
            neighborSum = addRegisterInt8(neighborSum, neighborE);

            neighborSWAdress = &currentField->field[calcIndex(currentField->width, x - 1, y + 1)];
            neighborSW = loadUnalignedMemory(neighborSWAdress);
            neighborS = loadUnalignedMemory(neighborSWAdress + sizeof(FieldType));
            neighborSE = loadUnalignedMemory(neighborSWAdress + sizeof(FieldType) * 2);
            neighborSum = addRegisterInt8(neighborSum, neighborSW);
            neighborSum = addRegisterInt8(neighborSum, neighborS);
            neighborSum = addRegisterInt8(neighborSum, neighborSE);

            #ifdef AVX_512
            cmp2Mask = cmpRegisterInt8(neighborSum, constRegister_2);
            cmpAliveMask = cmpRegisterInt8(aliveRegister, constRegister_1);
            cmp3Mask = cmpRegisterInt8(neighborSum, constRegister_3);

            resultMask = orMask(cmp3Mask, andMask(cmp2Mask, cmpAliveMask));

            resultRegister = blendMask(resultMask, constRegister_0, constRegister_1);
            #else
            cmp2Register = cmpRegisterInt8(neighborSum, constRegister_2);
            cmp3Register = cmpRegisterInt8(neighborSum, constRegister_3);
            resultRegister = andRegister(cmp2Register, aliveRegister);
            resultRegister = orRegister(cmp3Register, resultRegister);
            #endif // AVX_512

            storeRegister(&newField->field[calcIndex(currentField->width, x, y)], resultRegister);
        }
    }

    VTK_OUTPUT_SEGMENT(currentField, 0, 0, currentField->width, 0, 0, currentField->height)
    VTK_OUTPUT_MASTER(currentField, timestep, currentField->width, 0, currentField->height, 0)
}

#endif // GOL_VANILLA
