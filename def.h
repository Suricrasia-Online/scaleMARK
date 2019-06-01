
#ifndef DEF_H_
#define DEF_H_

#define __STR_HELPER(x) #x
#define STRINGIFY(x) __STR_HELPER(x)

#define   likely(x) __builtin_expect (!!(x), 1)
#define unlikely(x) __builtin_expect (!!(x), 0)

#ifdef MODE_DEV
#define k2lib_func __attribute__((__externally_visible__, __used__))
#else
#define k2lib_func
#endif

#define never_inline __attribute__((__noinline__))
#define force_inline __attribute__((__always_inline__))
#ifdef FORCE_LTO_OFF
#define   lto_inline
#else
#define   lto_inline __attribute__((__always_inline__))
#endif

#ifndef __cplusplus__
#define auto __auto_type
#endif

#ifdef __ARM_EABI__
#define SYSCALL_FUNC __attribute__((__target__("arm"))) force_inline static
#else
#define SYSCALL_FUNC force_inline static
#endif

#define MEMORY_BARRIER() asm volatile("":::"memory")

#define DONT_CARE(t) ({\
	t __r;\
	asm volatile("":"=r"(__r));\
	__r;\
})\

/*
 * This returns a constant expression while determining if an argument is
 * a constant expression, most importantly without evaluating the argument.
 * Glory to Martin Uecker <Martin.Uecker@med.uni-goettingen.de>
 */
#define __is_constexpr(x) \
        (sizeof(int) == sizeof(*(8 ? ((void *)((long)(x) * 0l)) : (int *)8)))\

// borked :(
/*#define __is_fixed_array(x) \
        (Generic((x), \
            typeof(*x)[sizeof(x)/sizeof(*(x))]: 1,\
            default: 0))\*/

// #define MIN(x,y) ({auto __a=(x);auto __b=(y);__a<__b?__a:__b;})
// #define MAX(x,y) ({auto __a=(x);auto __b=(y);__a>__b?__a:__b;})
// #define CLAMP(x,y,z) MIN(MAX(x,y), z)
// #define ABS(x) ({auto __y=(x);__y<0?-__y:__y;})

#ifndef offsetof
#define offsetof(type, member)  __builtin_offsetof (type, member)
#endif

#ifndef __cplusplus
#define __reinterpret_cast_base(Tto, x) ({\
	union { __typeof(x) f; Tto t; } u;\
	u.f = (x);\
	u.t;\
})\

extern long __fixsfdi (float a);
extern unsigned long __fixunssfdi (float a);
extern float __floatdisf (long i);
extern float __floatundisf (unsigned long i);
#define reinterpret_cast_(Tto, x) _Generic((x),\
	float: _Generic((signed long)(1),\
		Tto: __fixsfdi(x),\
		default: _Generic((unsigned long)(1),\
			Tto: __fixunssfdi(x),\
			default: __reinterpret_cast_base(Tto,x))),\
	signed long: _Generic((float)(1.0f),\
		Tto: __floatdisf(x),\
		default: __reinterpret_cast_base(Tto,x)),\
	unsigned long: _Generic((float)(1.0f),\
		Tto: __floatundisf(x),\
		default: __reinterpret_cast_base(Tto,x)),\
	default: __reinterpret_cast_base(Tto,x)\
	)\

#else
#define reinterpret_cast_(Tto, x) reinterpret_cast<Tto>(x)
#endif

#endif

