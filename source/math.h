#ifndef MATH_H

#define half_pi                1.570796326794896f
#define pi                     3.141592653589793f
#define tau                    6.283185307179586f
#define epsilon                0.000001000000000f
#define power_of_two(a)    (((a) & ((a) - 1)) == 0)

//#define abs(a)      (((a) >= 0) ? (a) : -(a))
#define sign(a)     (((a) < 0) ? -1 : ((a) > 0) ? 1 : 0)
#define min(a, b)   (((a) < (b)) ? (a) : (b))
#define max(a, b)   (((a) > (b)) ? (a) : (b))
#define squared(a)  (a) * (a)

inline f32 infinity()
{
	u32 u = 0x7f800000;
	f32 result = *(f32 *)(&u);
	return result;
}

inline f32 fabs(f32 arg)
{
	u32 a = *(u32 *)&arg;
	a &= 0x7fffffff;
	f32 result = *(f32 *)&a;
	return result;
}

inline f64 fabs64(f64 arg)
{
	u64 a = *(u64 *)&arg;
	a &= 0x7fffffffffffffff;
	f64 result = *(f64 *)&a;
	return result;
}

inline bool almost_equal(f32 a, f32 b, f32 tolerance = epsilon)
{
	bool result = fabs(a - b) <= tolerance;
	return result;
}

inline f32 clamp(f32 min, f32 v, f32 max)
{
	f32 result = _mm_cvtss_f32(_mm_max_ss(_mm_min_ss(_mm_set_ss(max), _mm_set_ss(v)), _mm_set_ss(min)));
	return result;
}

inline f32 lerp(f32 from, f32 t, f32 to)
{
    f32 result = from + t * (to - from);
    return result;
}

inline f32 sqrt(f32 arg)
{
    f32 result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(arg)));
	return result;
}

// 4th degree polynomial approximation
f32 sin(f32 x)
{
    f32 sign;
    *(s32 *)&sign = (*(s32 *)&x & 0x80000000) | 0x3f800000;
    *(s32 *)&x &= 0x7fffffff; // absolute value

    if(x > tau)
    {
        x = x - tau;
    }
    if(x > pi)
    {
        x = tau - x;
        sign = -sign;
    }
    if(x > half_pi)
    {
        x = pi - x;
    }

    f32 x2 = x * x;
    f32 result = (0.02879711f * x2 * x2 - 0.20434070f * x2 * x +
                  0.02137301f * x2 + 0.99562618f * x) * sign;
	
    return result;
}

// 4th degree polynomial approximation
f32 cos(f32 x)
{
    f32 sign = 1;
    *(s32 *)&x &= 0x7fffffff; // absolute value

    if(x > tau)
    {
        x = x - tau;
    }
    if(x > pi)
    {
        x = tau - x;
    }
    if(x > half_pi)
    {
        x = pi - x;
        sign *= -1;
    }

    f32 x2 = x * x;
    f32 result = (0.02879711f * x2 * x2 + 0.02340310f * x2 * x -
                  0.51523567f * x2 + 0.00335429f * x + 1) * sign;
	
    return result;
}

// slow-but-kinda-precise Taylor series expansion
f32 exp(f32 x)
{
	f32 result = 1 + x;
	f64 increment = x;
	f64 iter_counter = 1;
	while(fabs64(increment) > epsilon)
	{
		increment *= x / (++iter_counter);
		result += (f32)increment;
	}
	return result;
}

inline bool is_odd(u32 a)
{
    bool result = a & 1;
    return result;
}

//
// Vector 2D
//

union V2
{
    struct 
    {
        f32 x;
        f32 y;
    };
    
    struct 
    {
        f32 u;
        f32 v;
    };

    f32 e[2];
};

inline V2 v2(f32 x = 0, f32 y = 0)
{
    V2 result = {x, y};
    return result;
}

inline V2 operator + (V2 a, V2 b)
{
    V2 result = {a.x + b.x, a.y + b.y};
    return result;
}

inline V2& operator += (V2& a, V2 b)
{
    a = a + b;
    return a;
}

inline V2 operator - (V2 v)
{
    V2 result = {-v.x, -v.y};
    return result;
}

inline V2 operator - (V2 a, V2 b)
{
    V2 result = {a.x - b.x, a.y - b.y};
    return result;
}

inline V2& operator -= (V2& a, V2 b)
{
    a = a - b;
    return a;
}

inline V2 operator * (V2 a, f32 b)
{
    V2 result = {a.x * b, a.y * b};
    return result;
}

inline V2 operator * (f32 b, V2 a)
{
    V2 result = {a.x * b, a.y * b};
    return result;
}

// Element-wise
inline V2 operator * (V2 a, V2 b)
{
    V2 result = {a.x * b.x, a.y * b.y};
    return result;
}

inline V2& operator *= (V2& a, f32 b)
{
    a = a * b;
    return a;
}

inline V2& operator *= (V2& a, V2 b)
{
    a = a * b;
    return a;
}

inline V2 operator / (V2 a, f32 b)
{
    V2 result = {a.x / b, a.y / b};
    return result;
}

inline V2& operator /= (V2& a, f32 b)
{
    a = a / b;
    return a;
}

inline f32 dot(V2 a, V2 b)
{
    f32 result = a.x * b.x + a.y * b.y;
    return result;
}

inline f32 length_squared(V2 v)
{
    f32 result = dot(v, v);
    return result;
}

inline f32 length(V2 v)
{
    f32 result = sqrt(length_squared(v));
    return result;
}

inline V2 normalize(V2 v)
{
    V2 result = {};
    f32 len_sq = length_squared(v);
    if(len_sq > 0)
    {
        result = v / sqrt(len_sq);
    }
    return result;
}

inline f32 distance_squared(V2 a, V2 b)
{
    f32 result = length_squared(a - b);
    return result;
}

inline f32 distance(V2 a, V2 b)
{
    f32 result = sqrt(distance_squared(a, b));
    return result;
}

inline V2 lerp(V2 from, f32 t, V2 to)
{
    V2 result = from + t * (to - from);
    return result;
}

inline V2 perp(V2 v)
{
    V2 result = {-v.y, v.x};
    return result;
}

// perpendicular to v pointing in the direction of dir
inline V2 perp_in_direction(V2 v, V2 dir)
{
    f32 perp_z = v.x * dir.y - v.y * dir.x;
    V2 result = perp_z * v2(-v.y, v.x);
    return result;
}

//
//

union V4
{
    struct
    {
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };

    struct
    {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    
    __m128 wide;
    f32 e[4];
};

inline V4 v4(f32 x = 0, f32 y = 0, f32 z = 0, f32 w = 0)
{
    V4 result = {x, y, z, w};
    return result;
}

inline V4 operator + (V4 a, V4 b)
{
    V4 result;
    result.wide = _mm_add_ps(a.wide, b.wide);
    return result;
}

inline V4& operator += (V4& a, V4 b)
{
    a = a + b;
    return a;
}

inline V4 operator - (V4 v)
{
    V4 result = {-v.x, -v.y, -v.z, -v.w};
    return result;
}

inline V4 operator - (V4 a, V4 b)
{
    V4 result;
    result.wide = _mm_sub_ps(a.wide, b.wide);
    return result;
}

inline V4& operator -= (V4& a, V4 b)
{
    a = a - b;
    return a;
}

inline V4 operator * (V4 a, f32 b)
{
    V4 result;
    result.wide = _mm_mul_ps(a.wide, _mm_set_ps1(b));
    return result;
}

inline V4 operator * (f32 b, V4 a)
{
    V4 result;
    result.wide = _mm_mul_ps(a.wide, _mm_set_ps1(b));
    return result;
}

inline V4& operator *= (V4& a, f32 b)
{
    a = a * b;
    return a;
}

inline V4 operator * (V4 a, V4 b)
{
    V4 result;
	result.wide = _mm_mul_ps(a.wide, b.wide);
    return result;
}

inline V4 operator / (V4 a, f32 b)
{
    V4 result = {a.x / b, a.y / b, a.z / b, a.w / b};
    return result;
}

inline V4& operator /= (V4& a, f32 b)
{
    a = a / b;
    return a;
}

inline f32 dot(V4 a, V4 b)
{
    f32 result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
    return result;
}

inline V4 normalize(V4 v)
{
    V4 result = {};
	f32 len_sq = dot(v, v);
	if(len_sq > 0)
	{
		result = v / sqrt(len_sq);
	}
    return result;
}

inline V4 lerp(V4 from, f32 t, V4 to)
{
    V4 result = from + t * (to - from);   
    return result;
}


//
// Rectangle2D
//


union Rect2
{
	struct
	{
		V2 min;
		V2 max;
	};
	f32 e[4];	
};

inline f32 get_width(Rect2 rect)
{
    f32 result = rect.max.x - rect.min.x;
    return result;
}

inline f32 get_height(Rect2 rect)
{
    f32 result = rect.max.y - rect.min.y;
    return result;
}

inline V2 get_dim(Rect2 rect)
{
    V2 result = {rect.max.x - rect.min.x,
                 rect.max.y - rect.min.y};
    return result;
}

inline V2 get_center(Rect2 rect)
{
    V2 result = 0.5 * (rect.max + rect.min);
    return result;
}

inline Rect2 rect(V2 min, V2 max)
{
    Rect2 result = {min, max};
    return result;
}

inline Rect2 rect(f32 min_x, f32 min_y, f32 max_x, f32 max_y)
{
    Rect2 result = {min_x, min_y, max_x, max_y};
    return result;
}

inline Rect2 center_halfdim_rect(V2 center, V2 halfdim)
{
    Rect2 result = {center.x - halfdim.x, center.y - halfdim.y,
					center.x + halfdim.x, center.y + halfdim.y};
    return result;
}

inline Rect2 center_dim_rect(V2 center, V2 dim)
{
    Rect2 result = center_halfdim_rect(center, 0.5 * dim);	
    return result;
}

inline Rect2 center_dim_rect(V2 center, f32 dim)
{
    Rect2 result = center_halfdim_rect(center, 0.5 * v2(dim, dim));	
    return result;
}

inline Rect2 min_dim_rect(V2 min, V2 dim)
{
    Rect2 result = {min, min + dim};
    return result;
}

inline Rect2 min_dim_rect(f32 minx, f32 miny, f32 dimx, f32 dimy)
{
    Rect2 result = rect(minx, miny, minx + dimx, miny + dimy);    
    return result;
}

inline Rect2 corners_rect(V2 a, V2 b)
{
	Rect2 result = rect(min(a.x, b.x), min(a.y, b.y),
						max(a.x, b.x), max(a.y, b.y));
	return result;
}

inline Rect2 offset_rect(Rect2 rect, V2 v)
{
    Rect2 result = {rect.min + v, rect.max + v};
    return result;
}

inline Rect2 expand_to_include(Rect2 rect, V2 v)
{
    Rect2 result = rect;

	result.min.x = min(result.min.x, v.x);
	result.max.x = max(result.max.x, v.x);
	result.min.y = min(result.min.y, v.y);
	result.max.y = max(result.max.y, v.y);

    return result;
}

inline Rect2 get_intersection(Rect2 a, Rect2 b)
{
    Rect2 result = {max(a.min.x, b.min.x), max(a.min.y, b.min.y),
					min(a.max.x, b.max.x), min(a.max.y, b.max.y)};
    return result;
}

inline bool intersect(Rect2 rect, V2 v)
{
    bool result = (v.x >= rect.min.x) && (v.x <= rect.max.x) &&
                  (v.y >= rect.min.y) && (v.y <= rect.max.y);
    return result;
}

inline bool intersect(Rect2 a, Rect2 b)
{
	V2 intersection_dim = get_dim(get_intersection(a, b));
    bool result = intersection_dim.x > 0 && intersection_dim.y > 0;
    return result;
}

inline Rect2 negative_infinity_rect()
{
    Rect2 result = {infinity(), infinity(), -infinity(), -infinity()};
    return result;
}

inline V2 get_rect_relative_p(Rect2 rect, V2 p)
{
	V2 result = {lerp(rect.min.x, p.x, rect.max.x), lerp(rect.min.y, p.y, rect.max.y)};
	return result;
}

//
//

bool intersect(Rect2 rect, V2 p, f32 r)
{
	bool result = false;

	if(p.x > rect.max.x)
	{
		if(p.y > rect.max.y)
		{
			// upper right corner
			result = distance_squared(p, rect.max) <= squared(r);
		}
		else if(p.y < rect.min.y)
		{
			// lower right corner
			result = distance_squared(p, v2(rect.max.x, rect.min.y)) <= squared(r);
		}
		else
		{
			// strictly to the right
			result = p.x - r <= rect.max.x;
		}
	}
	else if(p.x < rect.min.x)
	{
		if(p.y > rect.max.y)
		{
			// upper left corner
			result = distance_squared(p, v2(rect.min.x, rect.max.y)) <= squared(r);
		}
		else if(p.y < rect.min.y)
		{
			// lower left corner
			result = distance_squared(p, rect.min) <= squared(r);
		}
		else
		{
			// strictly to the left
			result = p.x + r >= rect.min.x;
		}
	}
	else if(p.y > rect.max.y)
	{
		// strictly above
		result = p.y - r <= rect.max.y;
	}
	else if(p.y < rect.min.y)
	{
		// strictly below
		result = p.y + r >= rect.min.y;
	}
	else
	{
		// inside
		result = true;
	}
	
	return result;
}

inline bool intersect(V2 p0, f32 r0, V2 p1)
{
    bool result = distance(p0, p1) <= r0;    
    return result;
}

inline bool intersect(V2 p0, f32 r0, V2 p1, f32 r1)
{
    bool result = distance(p0, p1) <= r0 + r1;    
    return result;
}

inline bool intersect(V2 p00, V2 p01, V2 p10, V2 p11)
{
	bool result = false;

	f32 l0_norm_x = p00.y - p01.y;
	f32 l0_norm_y = p01.x - p00.x;
	
	f32 v0_x = p10.x - p00.x;
	f32 v0_y = p10.y - p00.y;
	f32 v1_x = p11.x - p00.x;
	f32 v1_y = p11.y - p00.y;

	f32 dot00 = v0_x * l0_norm_x + v0_y * l0_norm_y;
	f32 dot01 = v1_x * l0_norm_x + v1_y * l0_norm_y;

	if(dot00 * dot01 <= 0)
	{
		f32 l1_norm_x = p10.y - p11.y;
		f32 l1_norm_y = p11.x - p10.x;

		f32 v2_x = p10.x - p01.x;
		f32 v2_y = p10.y - p01.y;

		f32 dot10 = v0_x * l1_norm_x + v0_y * l1_norm_y;
		f32 dot11 = v2_x * l1_norm_x + v2_y * l1_norm_y;
		
		result = dot10 * dot11 <= 0;
	}
	
	return result;
}

// NOTE: macro because we always want to inline it no matter what
// the compiler's opinion about it
#define intersect_inline(name, p00, p01, p10, p11)	\
	bool name = false;								\
	{												\
	    f32 l0_norm_x = (p00).y - (p01).y;			\
		f32 l0_norm_y = (p01).x - (p00).x;			\
													\
		f32 v0_x = (p10).x - (p00).x;						\
		f32 v0_y = (p10).y - (p00).y;						\
		f32 v1_x = (p11).x - (p00).x;						\
		f32 v1_y = (p11).y - (p00).y;						\
															\
		f32 dot00 = v0_x * l0_norm_x + v0_y * l0_norm_y;	\
		f32 dot01 = v1_x * l0_norm_x + v1_y * l0_norm_y;	\
															\
		if(dot00 * dot01 <= 0)								\
		{													\
			f32 l1_norm_x = (p10).y - (p11).y;				\
			f32 l1_norm_y = (p11).x - (p10).x;				\
															\
			f32 v2_x = (p10).x - (p01).x;						\
			f32 v2_y = (p10).y - (p01).y;						\
																\
			f32 dot10 = v0_x * l1_norm_x + v0_y * l1_norm_y;	\
			f32 dot11 = v2_x * l1_norm_x + v2_y * l1_norm_y;	\
																\
			name = dot10 * dot11 <= 0;							\
		}														\
	}															\

inline bool get_intersection(V2 p00, V2 p01, V2 p10, V2 p11, V2 *result = 0)
{
	V2 dif0 = p01 - p00;
	V2 dif1 = p11 - p10;
	f32 denom = dif0.x * dif1.y - dif0.y * dif1.x;
	V2 v = p10 - p00;
	f32 t0 = (dif1.y * v.x - dif1.x * v.y) / denom;
	f32 t1 = (dif0.y * v.x - dif0.x * v.y) / denom;
	bool intersect = t0 >= 0 && t0 <= 1 && t1 >= 0 && t1 <= 1;
	if(result && intersect)
	{
		*result = p00 + t0 * dif0;
	}
	return intersect;
}

#define MATH_H
#endif
