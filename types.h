/*
 * types.h
 *
 * Definitions of basic types
 */

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

typedef int32_t  b32; // Boolean

#define F32Max FLT_MAX
#define F32Min -FLT_MAX