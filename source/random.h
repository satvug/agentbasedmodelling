#ifndef RANDOM_H

/*
// linear congruential generator
// modulus _has_ to be a power of two,
// otherwise it won't work as intended
#define ANSI_C_LCG_MODULUS      0x80000000
#define ANSI_C_LCG_MULTIPLIER   0x41c64e6d
#define ANSI_C_LCG_INCREMENT    0x00003039
#define RANDOM_MAX              (ANSI_C_LCG_MODULUS - 1)
*/

#define RANDOM_MAX U32_MAX

struct RNG
{
    u32 state;
};

inline void random_seed(RNG *rng, u32 seed)
{
    rng->state = seed;
}

/*
inline u32 random_uint(RNG *rng)
{
    u32 result = (ANSI_C_LCG_MULTIPLIER * rng->state + ANSI_C_LCG_INCREMENT) & RANDOM_MAX;
	rng->state = result;

    return result;
}
*/

inline u32 random_uint(RNG *rng)
{
    u32 result = rng->state;

	result ^= result << 13;
	result ^= result >> 17;
	result ^= result << 5;

	rng->state = result;

    return result;
}

inline f32 random_01(RNG *rng)
{
    f32 result = random_uint(rng) / (f32)RANDOM_MAX;

    return result;
}

inline bool random_bool(RNG *rng, f32 threshold = 0.5)
{
    bool result = random_01(rng) > threshold;

    return result;
}

inline s32 random_between_i(RNG *rng, s32 a, s32 b)
{
    s32 result = a + (s32)(random_01(rng) * (b - a));

    return result;
}

inline f32 random_between_f(RNG *rng, f32 a, f32 b)
{
    f32 result = a + random_01(rng) * (b - a);

    return result;
}

#define RANDOM_H
#endif
