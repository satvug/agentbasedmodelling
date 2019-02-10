#ifndef TYPES_H

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float  f32;
typedef double f64;

#define U8_MAX  0x00000000000000ff
#define U16_MAX 0x000000000000ffff
#define U32_MAX 0x00000000ffffffff
#define U64_MAX 0xffffffffffffffff
#define S8_MAX  0x000000000000007f
#define S16_MAX 0x0000000000007fff
#define S32_MAX 0x000000007fffffff
#define S64_MAX 0x7fffffffffffffff
#define S8_MIN  0x0000000000000080
#define S16_MIN 0x0000000000008000
#define S32_MIN 0x0000000080000000
#define S64_MIN 0x8000000000000000

#define TYPES_H
#endif
