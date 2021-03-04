#ifndef SIMD_UTILS
#define SIMD_UTILS

#include <immintrin.h>

#ifdef __GNUC__
  #define ALIGN(x) x __attribute__((aligned(32)))
#elif defined(_MSC_VER)
  #define ALIGN(x) __declspec(align(32))
#endif

// This is defined because in C, the SIMD loads do not recognize a static address as a compile-time constant
#ifdef __cplusplus
#define STATIC_REGISTER static
#else
#define STATIC_REGISTER
#endif // __cplusplus

#ifdef AVX_2

typedef __m256i register_int;

// 256 / 8  = 32
#define REGISTER_SIZE_INT8 32
// 256 / 32 = 8
#define REGISTER_SIZE_INT32 8

#define loadAlignedMemory(address) _mm256_load_si256((register_int*) address)
#define loadUnalignedMemory(address) _mm256_loadu_si256((register_int*) address)

#define addRegisterInt8 _mm256_add_epi8
#define cmpRegisterInt8 _mm256_cmpeq_epi8
#define andRegister _mm256_and_si256
#define orRegister _mm256_or_si256
#define storeRegister(destAddress, source) _mm256_storeu_si256((register_int*) destAddress, source)

#endif // AVX_2

#ifdef AVX_512

typedef __m512i register_int;
typedef __mmask64 register_mask;

// 512 / 8  = 64
#define REGISTER_SIZE_INT8 64
// 512 / 32 = 16
#define REGISTER_SIZE_INT32 16

#define loadAlignedMemory(address) _mm512_load_si512((register_int*) address)
#define loadUnalignedMemory(address) _mm512_loadu_si512((register_int*) address)

#define addRegisterInt8 _mm512_add_epi8
#define cmpRegisterInt8 _mm512_cmpeq_epi8_mask
#define andRegister _mm512_and_si512
#define orRegister _mm512_or_si512
#define storeRegister(destAddress, source) _mm512_storeu_si512((register_int*) destAddress, source)
#define blendMask _mm512_mask_blend_epi8

#define andMask _kand_mask64
#define orMask _kor_mask64

#endif // AVX_512

#endif // SIMD_UTILS