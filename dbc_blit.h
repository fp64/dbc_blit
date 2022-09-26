/*
    dbc_blit.h - single-file public domain software blitter.
    Version 2.0.7.

COMPILATION
    When included normally, this file acts as a header.
    To create an implementation
#define DBC_BLIT_IMPLEMENTATION
    in ONE translation unit, that includes this file.

    You can
#define DBC_BLIT_STATIC
    to make implementation static to the translation unit that includes it.

USAGE
    Blitter API exposes a single function
dbc_blit(src_w,src_h,src_stride_in_bytes,src_pixels,
         dst_w,dst_h,dst_stride_in_bytes,dst_pixels,
         x,y,color,mode)
    which blits (possibly color-modulated) pixel rectangle from src to dst.

    Format for both src and dst is the same and implied in 'mode'.
    DBCB_MODE_COPY, DBCB_MODE_ALPHA, DBCB_MODE_PMA, DBCB_MODE_GAMMA,
    DBCB_MODE_PMG, DBCB_MODE_MUL, DBCB_MODE_MUG, and DBCB_MODE_CPYG use
    32-bit RGBA (or BGRA; the blitter does not care, except the 'color'
    is understood to use the same order; however color[3] always corresponds
    to alpha channel).

    (w,h,stride,pixels) describes a surface - a rectangular array of pixels,
    stored such that pixel (x,y) is located at pixels+y*stride+x*pixel_size.
    If src and dst overlap the resulting image is unspecified.

    'color' if present is used to modulate src before it is applied to dst.
    'color' can be NULL, which means no modulation; specifically, it means the
    same as {1.0f,1.0f,1.0f,1.0f} in 32-bit modes (other than alpha test); in
    colorkey modes it means no colorkey, same as -1.0f; in 5551 mode it is
    ignored; in alpha test mode it means "all pass", same as 0.0f.
    'color' components can be outside [0.0f;1.0f]. For modes that expect
    integer color[0] it is rounded: down for colorkey, and up for alpha-test.

    Modes are described below. In the equations colors are understood to be
    in [0;1], not in [0;255]; 'C' denotes color component (one of R,G,B),
    'A' denotes alpha, 's' denotes source, 'd' - destination,
    'm' - modulation ('color'), 'f' - final value.

DBCB_MODE_COPY - plain copy. Still can use modulation. Equations:
    Cf=Cm*Cs
    Af=Am*As

DBCB_MODE_ALPHA - "ordinary" alpha-blending. Equations:
    Cf=Cm*Cs*Am*As+Cd*(1-Am*As)
    Af=Am*As+Ad*(1-Am*As)

DBCB_MODE_PMA - premultiplied alpha blending. Equations:
    Cf=Cm*Cs+Cd*(1-Am*As)
    Af=Am*As+Ad*(1-Am*As)
    NOTE: using premultiplied alpha instead of "ordinary" alpha blending can
    have many advantages and you might want to consider it. For the long
    discussion of the benefits of premultiplied alpha, see
http://tomforsyth1000.github.io/blog.wiki.html#[[Premultiplied%20alpha]]
http://tomforsyth1000.github.io/blog.wiki.html#[[Premultiplied%20alpha%20part%202]]
    Short version:
    * PMA is more mathematically sensible, e.g. associative
    and works better with filtering.
    * PMA allows to express both ordinary and additive blending, as well
    as continuum of in-between cases.
    * PMA may be slightly faster.

DBCB_MODE_CPYG - gamma-corrected copy (result only differs from the
    ordinary copy if modulation is present). In this mode, color
    components of both src and dst are understood to be sRGB-encoded.
    Alpha is still considered linear (same as GL_SRGB8_ALPHA8).
    Modulation ('color') is understood to be in linear space.
    Equations:
    Cf=linear2srgb(Cm*srgb2linear(Cs))
    Af=Am*As

DBCB_MODE_GAMMA - gamma-corrected (non-premultiplied) alpha-blending.
    Equations:
    Cf=linear2srgb(Cm*srgb2linear(Cs)*Am*As+srgb2linear(Cd)*(1-Am*As))
    Af=Am*As+Ad*(1-Am*As)

DBCB_MODE_PMG - gamma-corrected premultiplied alpha-blending.
    Premultiplication takes place in linear space. Equations:
    Cf=linear2srgb(Cm*srgb2linear(Cs)+srgb2linear(Cd)*(1-Am*As))
    Af=Am*As+Ad*(1-Am*As)

DBCB_MODE_MUL - color multiplication. Equations:
    Cf=Cm*Cs*Cd
    Af=Am*As*Ad

DBCB_MODE_MUG - gamma-corrected color multiplication. Equations:
    Cf=linear2srgb(Cm*srgb2linear(Cs)*srgb2linear(Cd))
    Af=Am*As*Ad

DBCB_MODE_COLORKEY8 - src and dst are 8 bit. Blit with colorkey.
    Colorkey is encoded in color[0] (e.g. 37.0f means color 37
    is transparent). Values outside [0;255] indicate
    no colorkey (straight copy).

DBCB_MODE_COLORKEY16 - src and dst are 16 bit. Blit with colorkey.
    Colorkey is encoded in color[0] (e.g. 37.0f means color
    37 is transparent). Values outside [0;65535] indicate no
    colorkey (straight copy).

DBCB_MODE_5551 - src and dst are 16 bit. High bit encodes 1-bit
    alpha. 'color' is ignored.

DBCB_MODE_ALPHATEST - alpha test. src is copied to dst (unchanged)
    iff its alpha value (in range [0;255]) equals to or exceeds threshold.
    The threshold is encoded in color[0] (e.g. 37.0f means that
    alpha>=37 passes). Threshold 0 or below (or color==NULL) means
    every pixel passes (straight copy). Threshold above 255 means every
    pixel is rejected. The implementation is optimized to be somewhat
    faster if threshold is exactly 128.

SIMD
    On x86/x64 the library attempts to detect SIMD support and
    use optimized SIMD implementations of certain functions. This,
    however, introduces some concerns. For one thing, using intrinsics
    with CPU dispatch may cause problems on older versions of GCC
    and clang:
https://www.virtualdub.org/blog2/entry_363.html
https://gist.github.com/rygorous/f26f5f60284d9d9246f6
    As such, for the compilers that support GCC extensions (GCC, clang,
    icc) the library may try to avoid intrinsics, and use GCC inline
    assembly instead. The other compilers (MSVC, etc.) always use
    intrinsics. The library choses between assembly and intrinsics
    depending on the compiler version. This can be overriden by:
#define DBC_BLIT_NO_GCC_ASM
#define DBC_BLIT_FORCE_GCC_ASM
    You can disable AVX2 specifically by
#define DBC_BLIT_NO_AVX2
    You can also suppress all SIMD implementations by
#define DBC_BLIT_NO_SIMD
    Runtime CPU detection can sometimes cause problems:
https://github.com/nothings/stb/issues/280
https://github.com/nothings/stb/issues/410
    You can
#define DBC_BLIT_NO_RUNTIME_CPU_DETECTION
    to only use SIMD if globally enabled at compile-time (-msse2, etc.).
    Note: using SIMD in MinGW may be problematic on 32-bit x86:
https://www.peterstock.co.uk/games/mingw_sse/
https://github.com/nothings/stb/issues/81
    As such, THE USAGE OF SIMD IS DISABLED ON MinGW FOR 32-BIT x86 TARGETS,
    unless explicitly specified:
#define DBC_BLIT_ENABLE_MINGW_SIMD
    This assumes appropriate precautions (e.g. -mstackrealign) have
    been taken.

    Note: on some machines SSE2 versions may end up being slower than
    pure C, and/or AVX2 versions slower than SSE2. dbc_blit.h does not
    try to detect it, and simply uses the highest instruction set available.
    However, you can #define the function-like macros
    dbcb_allow_sse2_for_mode(mode,modulated) and
    dbcb_allow_avx2_for_mode(mode,modulated) to control SIMD at
    runtime on per-mode basis. This may look something like this:

    Note: some older OSes (Windows 95 and earlier, Linux kernel
    before something like 2.4) may not have the OS-level support for SSE
    (do not preserve the state on context switch). The dbc_blit.h does not
    check for it. If you need to support them, consider checking it yourself
    and using dbcb_allow_sse2_for_mode()/dbcb_allow_avx2_for_mode().

// Determined by whatever means.
extern int sse2_is_slow;
// Suppose only alpha test is fast enough to overtake SIMD. Alpha test
// w/o modulation (i.e. threshold) is straight copy, so leave it be.
#define dbcb_allow_sse2_for_mode(mode,modulated) (modulated&&mode==DBCB_MODE_ALPHATEST? !sse2_is_slow : 1)
#define DBC_BLIT_IMPLEMENTATION
#include "dbc_blit.h"

    The expressions these macros expand to do not need to be compile-time
    constants. They are each evaluated once per dbc_blit() call (assuming
    SSE2/AVX2 are enabled).

UNROLLING
    To improve the blitting speed of small sprites, dbc_blit uses unrolling
    of inner (horizontal pixel line) loops. This is controlled by
#define DBC_BLIT_UNROLL width
    where 'width' specifies the amount of unrolling: the lines of 'width'
    or less pixels are unrolled. Currently the only supported values are 0
    (disables unrolling), 8, 16, and 32. The default is 8.
    To selectively enable unrolling you can
#define dbcb_unroll_limit_for_mode(mode,modulated) width
    to the desired maximum width (actual unrolling does not go above
    DBC_BLIT_UNROLL).
    The expression it expands to does not need to be the compile-time
    constant. However, if it is known at compile time to expand to 0,
    the compiler can remove the corresponding code path, reducing the
    binary size.

    While unrolling may improve speed for small sprites, it significantly
    increases compilation time and binary size.

ALIGNMENT
    By default dbc_blit assumes no alignment for pixel data. On some systems
    this may lead to less efficient code. If you have stronger guarantees,
    you can provide replacements for dbcb_load*()/dbcb_store*() functions
    like this (you need to #define all of them):
#define dbcb_load16(ptr)       ...implementation...
#define dbcb_store16(val,ptr)  ...implementation...
#define dbcb_load32(ptr)       ...implementation...
#define dbcb_store32(val,ptr)  ...implementation...
#define dbcb_load64(ptr)       ...implementation... (unless DBC_BLIT_NO_INT64)
#define dbcb_store64(val,ptr)  ...implementation... (unless DBC_BLIT_NO_INT64)
#define DBC_BLIT_IMPLEMENTATION
#include "dbc_blit.h"
    Expected signatures are 'uintN load(const void*)' and
    'void store(uintN,void*)'.
    There may be other reasons to replace them (e.g. some compilers producing
    suboptimal code for the default implementations).

    WARNING: if you decide to also replace SIMD versions of load/store
    functions (dbcb_load128_*(), etc.), you likely need to match
    their declarations (see DBCB_DECL_SSE2 and DBCB_DECL_AVX2).

ENDIANNESS
    By default dbc_blit interprets pixel data as little endian.
    You can
#define DBC_BLIT_DATA_BIG_ENDIAN
    to interpret it as big endian instead. This setting is global: it applies
    to all modes, and does not support changing endianness at runtime. If you
    need more control, you can create two instances of the library
    (via DBC_BLIT_STATIC) with different endianness, and pick one at runtime.

    Internally endianness only affects the default implementations of
    dbcb_load*()/dbcb_store*() functions. If you replace them, you are
    responsible for their endianness.

    dbc_blit does not do anything special about the endianness of floats.

THREAD SAFETY
    Calls to dbc_blit() from different threads are safe, if there is no
    problem with data overlap, specifically, src/src overlap is safe, but
    src/dst and dst/dst are not. However, the dbc_blit() performs
    initialization on the first call. This initialization is not thread-safe,
    when compiled as C (it should be thread-safe when compiled as C++). If
    that is an issue, you can call dbc_blit(0,0,0,0,0,0,0,0,0,0,0,0);
    beforehand to perform the initialization explicitly. Yes, this is lame.
    If you #define dbcb_allow_sse2_for_mode()/dbcb_allow_avx2_for_mode()/
    dbcb_unroll_limit_for_mode(), it is your responsibility to ensure that
    their evaluation is thread-safe. Same goes for dbcb_load*()/dbcb_store*()
    replacements.
    The library itself does not use multithreading internally.

ACCURACY
    Several of the modes are exact: the pixel is either copied verbatim, or
    discarded. For the rest, the library is set to provide maximal
    accuracy, i.e. produce the closest representable value to the exact
    mathematical answer (i.e. 'correctly-rounded result'), unless
    modulation is present. This is honored even for gamma-corrected blending
    (see below). This implies that error (in units of 1/255 of full range)
    does not exceed 0.5.
    Modulation lowers accuracy, but the error should still stay below 0.51,
    unless modulation values are large in magnitude (>250), after which
    accuracy gradually worsens. Modulation values that exceed 1e14 in
    magnitude, or NaN, or inf may cause problems in floating point, and are
    therefore advised against.

GAMMA CORRECTNESS
    Several modes in dbc_blit are gamma-corrected, i.e. src and dst
    are interpreted as sRGB-encoded. Delivering the correctly-rounded results
    in these modes might look questionable: indeed, the sRGB <-> linear
    conversion functions, as defined by the specification, have a
    discontinuity large enough to be detectable in float32 (the functions
    jump by about 0.00007% at discontinuity). The library attempts to do it
    anyway. The algorithm for these modes uses double for internal
    calculations (tests indicate, that double provides enough precision,
    while float does not) as well as precalculated tables. If the type
    double is undesirable, it can be suppressed by
#define DBC_BLIT_GAMMA_NO_DOUBLE
    in which case float is used instead. This lowers accuracy slightly (about
    0.0006% cases give result one off from correctly-rounded). On x86 there
    doesn't seem to be a speed difference.
    If tables are undesirable, you can
#define DBC_BLIT_GAMMA_NO_TABLES method
    to use approximations instead. Approximations always use float. Optional
    'method' is an integer from 0 to 3 inclusive, which specifies the
    accuracy. The accuracy (in units of 1/255 of full range) is currently
    as follows:
Method     Error      Notes
0          < 0.87     Faithfully rounded, sRGB->linear->sRGB round-trips.
1          < 2.35
2          < 8.19
3          <23.75
    The default is 0. Less accurate methods are faster.
    The approximations are slower in pure C, but may be faster with SIMD,
    depending on accuracy.
    As mentioned, modulation introduces more inaccuracies.
    The gamma-corrected modes can also be suppressed entirely by
#define DBC_BLIT_NO_GAMMA
    which also removes corresponding code.

SPEED
    Performance depends on a number of factors, including CPU architecture,
    memory speed, cache size, and use pattern.

    To give some idea of performance, here are timings from test
    machine (Intel(R) Xeon(R) Gold 5115 CPU @ 2.40GHz), in ns/pixel:
                    |              Non-modulated              |                Modulated                |
                    |    Pure C   |     SSE2    |     AVX2    |    Pure C   |     SSE2    |     AVX2    |
                    | Fill | Rand | Fill | Rand | Fill | Rand | Fill | Rand | Fill | Rand | Fill | Rand |
--------------------+------+------+------+------+------+------+------+------+------+------+------+------+
DBCB_MODE_COPY      |  0.08|  0.45|  0.09|  0.45|  0.09|  0.47|  7.16|  7.66|  3.21|  3.49|  1.74|  1.81|
DBCB_MODE_ALPHA     |  3.24|  3.76|  1.28|  1.39|  0.68|  1.23|  8.83| 10.46|  4.27|  4.86|  3.30|  3.72|
DBCB_MODE_PMA       |  3.96|  4.47|  0.97|  1.32|  0.60|  1.10| 17.47| 19.21|  3.79|  4.16|  3.43|  3.45|
DBCB_MODE_GAMMA     |  5.68|  6.51|  5.70|  6.39|  5.72|  6.54| 13.28| 14.17| 14.14| 13.95| 13.25| 14.47|
DBCB_MODE_PMG       |  6.86|  7.77|  6.84|  8.21|  7.23|  7.85| 14.80| 15.07| 14.79| 19.05| 14.96| 15.23|
DBCB_MODE_COLORKEY8 |  0.04|  0.09|  0.08|  0.11|  0.05|  0.09|  0.45|  0.46|  0.10|  0.10|  0.08|  0.09|
DBCB_MODE_COLORKEY16|  0.08|  0.24|  0.07|  0.23|  0.07|  0.24|  1.12|  1.23|  0.16|  0.21|  0.11|  0.15|
DBCB_MODE_5551      |  0.62|  0.69|  0.13|  0.21|  0.15|  0.17|  0.65|  0.70|  0.13|  0.20|  0.15|  0.17|
DBCB_MODE_MUL       |  3.84|  4.51|  0.68|  1.02|  0.50|  0.92| 11.24| 11.86|  3.02|  3.30|  2.69|  3.01|
DBCB_MODE_MUG       |  5.65|  6.92|  6.39|  7.19|  6.62|  7.49| 11.65| 13.09| 11.33| 12.75| 11.23| 12.49|
DBCB_MODE_ALPHATEST |  0.08|  0.42|  0.09|  0.41|  0.09|  0.42|  1.81|  2.30|  0.29|  0.48|  0.18|  0.36|
DBCB_MODE_CPYG      |  0.08|  0.54|  0.08|  0.52|  0.09|  0.50| 10.29| 10.51| 10.24| 10.63| 10.45| 11.12|

    These were taken by blitting 64x64 sprites (with 'reasonable'
    distribution of transparent/semitransparent/opaque pixels) on 800x600
    buffer. Column 'Fill' indicates fillrate, 'Rand' - blitting at random.
    The benchmark code was compiled with gcc at -O2 optimization level.
    Notes:
    * Straight copy uses memcpy(), which detects SSE2/AVX2 internally so the
    speed is the same.
    * DBCB_MODE_5551 ignores modulation.
    * Gamma-corrected modes are significantly (around twice) slower on fully
    semitransparent sprites (1<=alpha<=254), as are pure C versions of
    alpha-blending modes (except alpha test).
    * Alpha test is slightly (around 10%) faster with threshold equal to 128.
    * Approximated gamma is slower in pure C, and roughly similar with SIMD:
    it may be slower or faster, depending on chosen accuracy.
    * Big endian blits are about 20% slower.
    * Compiling with -O1 or -Os significantly slows blits (more than twice
    for some modes), while -O3 can be about 20% faster than -O2.

    Call to dbc_blit() itself adds roughly 25 ns.

MEMORY USAGE
    dbc_blit does not use dynamic memory allocation. It statically allocates
    around 40 KB for tables used by gamma-corrected modes. This can be reduced
    to around 22 KB by setting DBC_BLIT_GAMMA_NO_DOUBLE, and completely
    eliminated by DBC_BLIT_NO_GAMMA or DBC_BLIT_GAMMA_NO_TABLES, in which
    case only handful of bytes are statically allocated.

CODE FOOTPRINT
    How much including dbc_blit adds to executable size depends on the
    configuration. Measured on x86 it ranges from around 8 KB (no gamma,
    no SIMD, no unrolling) to around 450 KB (everything included).

MISC
    List of #define's used to configure the library:
#define DBC_BLIT_IMPLEMENTATION
#define DBC_BLIT_STATIC
#define DBC_BLIT_DATA_BIG_ENDIAN
#define DBC_BLIT_NO_GAMMA
#define DBC_BLIT_GAMMA_NO_TABLES method
#define DBC_BLIT_GAMMA_NO_DOUBLE
#define DBC_BLIT_NO_INT64
#define DBC_BLIT_ENABLE_MINGW_SIMD
#define DBC_BLIT_NO_SIMD
#define DBC_BLIT_NO_RUNTIME_CPU_DETECTION
#define DBC_BLIT_NO_GCC_ASM
#define DBC_BLIT_NO_AVX2
#define DBC_BLIT_UNROLL width
#define dbcb_unroll_limit_for_mode(mode,modulated) width
#define dbcb_allow_sse2_for_mode(mode,modulated) expr
#define dbcb_allow_avx2_for_mode(mode,modulated) expr
#define dbcb_load*(ptr)           ...implementation... [*={16,32[,64]}]
#define dbcb_store*(val,ptr)      ...implementation... [*={16,32[,64]}]
#define dbcb_load128_*(ptr)       ...implementation... [*={32,64,128}]
#define dbcb_store128_*(val,ptr)  ...implementation... [*={32,64,128}]
#define dbcb_load256_*(ptr)       ...implementation... [*={32,64,128,256}]
#define dbcb_store256_*(val,ptr)  ...implementation... [*={32,64,128,256}]
#define dbcb_int*  type [*={8,16,32[,64]}]
#define dbcb_uint* type [*={8,16,32[,64]}]

    For those curious, DBC stands for 'design by committee'.

LICENSE

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>

*/

/*============================================================================*/
/* Interface */

#ifndef DBC_BLIT_INCLUDE
#define DBC_BLIT_INCLUDE

#ifdef DBC_BLIT_STATIC
#define DBCB_DEF static
#else
#define DBCB_DEF extern
#endif

#define DBCB_MODE_COPY                   0
#define DBCB_MODE_ALPHA                  1
#define DBCB_MODE_PMA                    2
#define DBCB_MODE_GAMMA                  3
#define DBCB_MODE_PMG                    4
#define DBCB_MODE_COLORKEY8              5
#define DBCB_MODE_COLORKEY16             6
#define DBCB_MODE_5551                   7
#define DBCB_MODE_MUL                    8
#define DBCB_MODE_MUG                    9
#define DBCB_MODE_ALPHATEST             10
#define DBCB_MODE_CPYG                  11

#ifdef __cplusplus
extern "C" {
#endif

DBCB_DEF void dbc_blit(
    int src_w,int src_h,int src_stride,
    const unsigned char *src_pixels,
    int dst_w,int dst_h,int dst_stride,
    unsigned char *dst_pixels,
    int x,int y,
    const float *color,
    int mode);

#ifdef __cplusplus
}
#endif

#endif /* DBC_BLIT_INCLUDE */

/*============================================================================*/
/* Implementation */

#ifdef DBC_BLIT_IMPLEMENTATION

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4127 ) /* conditional expression is constant */
#pragma warning( disable : 4752 ) /* found Intel(R) Advanced Vector Extensions; consider using /arch:AVX */
#endif

/* You can #define these types to something else. */
#ifndef dbcb_uint8
typedef signed   char       dbcb_int8;
typedef unsigned char       dbcb_uint8;
typedef signed   short      dbcb_int16;
typedef unsigned short      dbcb_uint16;
typedef signed   int        dbcb_int32;
typedef unsigned int        dbcb_uint32;
#ifndef DBC_BLIT_NO_INT64
typedef signed   long long  dbcb_int64;
typedef unsigned long long  dbcb_uint64;
#endif
#endif

/* In case we have 64-bit type, but not 64-bit literals. */
#define DBCB_C64(lo,hi) (((dbcb_uint64)(hi)<<32)^((dbcb_uint64)(lo)))

/* Floating-point type for tables, and its constant suffix. */
#ifndef DBC_BLIT_NO_GAMMA
#if defined(DBC_BLIT_GAMMA_NO_DOUBLE) || defined(DBC_BLIT_GAMMA_NO_TABLES)
typedef float  dbcb_fp;
#define DBCB_FC(x) x##f
#else
typedef double dbcb_fp;
#define DBCB_FC(x) x
#endif
#endif /* DBC_BLIT_NO_GAMMA */

/* Static asserts. */

typedef char dbcB_check_bits_u8 [(dbcb_uint8)(-1)==255?1:-1];

typedef char dbcB_check_size_i8 [sizeof(dbcb_int8  )==1?1:-1];
typedef char dbcB_check_size_u8 [sizeof(dbcb_uint8 )==1?1:-1];
typedef char dbcB_check_size_i16[sizeof(dbcb_int16 )==2?1:-1];
typedef char dbcB_check_size_u16[sizeof(dbcb_uint16)==2?1:-1];
typedef char dbcB_check_size_i32[sizeof(dbcb_int32 )==4?1:-1];
typedef char dbcB_check_size_u32[sizeof(dbcb_uint32)==4?1:-1];
#ifndef DBC_BLIT_NO_INT64
typedef char dbcB_check_size_i64[sizeof(dbcb_int64 )==8?1:-1];
typedef char dbcB_check_size_u64[sizeof(dbcb_uint64)==8?1:-1];
#endif

/* Architecture detection. */
#if defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__i386) || defined(__THW_INTEL)
#define DBCB_X86
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64)
#define DBCB_X64
#endif

#if defined(DBCB_X86) || defined(DBCB_X64)
#define DBCB_X86_OR_X64
#endif

/* Endianness. */
#if !defined(DBC_BLIT_DATA_LITTLE_ENDIAN) && !defined(DBC_BLIT_DATA_BIG_ENDIAN)
#define DBC_BLIT_DATA_LITTLE_ENDIAN
#endif

#if defined(DBCB_X86_OR_X64) ||\
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__)) ||\
    (defined(__BYTE_ORDER) && (__BYTE_ORDER==__LITTLE_ENDIAN)) ||\
    defined(__LITTLE_ENDIAN__) || defined(__ARMEL__) || defined(__THUMBEL__) ||\
    defined (__AARCH64EL__) || defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define DBCB_SYSTEM_LITTLE_ENDIAN
#endif

#if (defined(__BYTE_ORDER__) && (__BYTE_ORDER__==__ORDER_BIG_ENDIAN__)) ||\
    (defined(__BYTE_ORDER) && (__BYTE_ORDER==__BIG_ENDIAN)) ||\
    defined(__BIG_ENDIAN__) || defined(__ARMEB__) || defined(__THUMBEB__) ||\
    defined(__AARCH64EB__) || defined(_MIPSEB) || defined(__MIPSEB) || defined(__MIPSEB__)
#define DBCB_SYSTEM_BIG_ENDIAN
#endif

#if defined(DBC_BLIT_DATA_LITTLE_ENDIAN) && defined(DBC_BLIT_DATA_BIG_ENDIAN)
#error Both DBC_BLIT_DATA_LITTLE_ENDIAN and DBC_BLIT_DATA_BIG_ENDIAN are specified
#endif

#if defined(DBCB_SYSTEM_LITTLE_ENDIAN) && defined(DBCB_SYSTEM_BIG_ENDIAN)
#warning Conflicting results during endianness detection
#undef DBCB_SYSTEM_LITTLE_ENDIAN
#undef DBCB_SYSTEM_BIG_ENDIAN
#endif

/*
   The memcpy replacement function (defaults to the standard version).
   You can #define it to your own implementation.
   It is used to copy lines of pixels.
*/
#ifndef dbcb_memcpy
#if defined(__GNUC__) && (__STDC_HOSTED__==0)
#define dbcb_memcpy(dst,src,size) __builtin_memcpy((dst),(src),(size)) /* Allow memcpy in freestanding implementation. */
#else
#include <string.h>
#define dbcb_memcpy(dst,src,size) memcpy((dst),(src),(size))
#endif
#endif /* dbcb_memcpy */

/*
    Fixed-width load/store functions. You can #define them (all of
    them, or none) to your own implementations.
    You can similarly replace SIMD versions, but then you probably need to
    match the way they are declared (DBCB_DECL_SSE2, etc.).
*/
#ifndef dbcb_load16
#if defined(DBCB_SYSTEM_LITTLE_ENDIAN) || defined(DBCB_SYSTEM_BIG_ENDIAN)
/* memcpy-based versions. */

#if defined(__GNUC__) && (__STDC_HOSTED__==0)
#define dbcB_cpy(dst,src,size) __builtin_memcpy((dst),(src),(size)) /* Allow memcpy in freestanding implementation. */
#else
#include <string.h>
#define dbcB_cpy(dst,src,size) memcpy((dst),(src),(size))
#endif

#if (defined(DBC_BLIT_DATA_LITTLE_ENDIAN) && defined(DBCB_SYSTEM_LITTLE_ENDIAN)) || (defined(DBC_BLIT_DATA_BIG_ENDIAN) && defined(DBCB_SYSTEM_BIG_ENDIAN))

static dbcb_uint16 dbcb_load16(const void *p) {dbcb_uint16 ret;dbcB_cpy(&ret,p,sizeof(ret));return ret;}
static void dbcb_store16(dbcb_uint16 v,void *p) {dbcB_cpy(p,&v,sizeof(v));}
static dbcb_uint32 dbcb_load32(const void *p) {dbcb_uint32 ret;dbcB_cpy(&ret,p,sizeof(ret));return ret;}
static void dbcb_store32(dbcb_uint32 v,void *p) {dbcB_cpy(p,&v,sizeof(v));}
#ifndef DBC_BLIT_NO_INT64
static dbcb_uint64 dbcb_load64(const void *p) {dbcb_uint64 ret;dbcB_cpy(&ret,p,sizeof(ret));return ret;}
static void dbcb_store64(dbcb_uint64 v,void *p) {dbcB_cpy(p,&v,sizeof(v));}
#endif

#else /* (defined(DBC_BLIT_DATA_LITTLE_ENDIAN) && defined(DBCB_SYSTEM_LITTLE_ENDIAN)) || (defined(DBC_BLIT_DATA_BIG_ENDIAN) && defined(DBCB_SYSTEM_BIG_ENDIAN)) */

#ifdef __GNUC__
#define dbcB_bswap32 __builtin_bswap32
#define dbcB_bswap64 __builtin_bswap64
#else
static dbcb_uint32 dbcB_bswap32(dbcb_uint32 n) {n=(dbcb_uint32)((n<<16)|(n>>16));n=(dbcb_uint32)(((n&0x00FF00FFu)<<8)|((n&0xFF00FF00u)>>8));return ret;}
#ifndef DBC_BLIT_NO_INT64
static dbcb_uint64 dbcB_bswap64(dbcb_uint64 n) {n=(dbcb_uint64)((n<<32)|(n>>32));n=(dbcb_uint64)(((n&DBCB_C64(0x0000FFFFu,0x0000FFFFu))<<16)|((n&DBCB_C64(0xFFFF0000u,0xFFFF0000u))>>16));n=(dbcb_uint64)(((n&DBCB_C64(0x00FF00FFu,0x00FF00FFu))<<8)|((n&DBCB_C64(0xFF00FF00,0xFF00FF00u))>>8));return ret;}
#endif
#endif /* __GNUC__ */

static dbcb_uint16 dbcb_load16(const void *p) {dbcb_uint16 ret;dbcB_cpy(&ret,p,sizeof(ret));return (dbcb_uint16)((ret>>8)|(ret<<8));}
static void dbcb_store16(dbcb_uint16 v,void *p) {v=(dbcb_uint16)((v>>8)|(v<<8));dbcB_cpy(p,&v,sizeof(v));}
static dbcb_uint32 dbcb_load32(const void *p) {dbcb_uint32 ret;dbcB_cpy(&ret,p,sizeof(ret));return dbcB_bswap32(ret);}
static void dbcb_store32(dbcb_uint32 v,void *p) {v=dbcB_bswap32(v);dbcB_cpy(p,&v,sizeof(v));}
#ifndef DBC_BLIT_NO_INT64
static dbcb_uint64 dbcb_load64(const void *p) {dbcb_uint64 ret;dbcB_cpy(&ret,p,sizeof(ret));return dbcB_bswap64(ret);}
static void dbcb_store64(dbcb_uint64 v,void *p) {v=dbcB_bswap64(v);dbcB_cpy(p,&v,sizeof(v));}
#endif
#endif /* (defined(DBC_BLIT_DATA_LITTLE_ENDIAN) && defined(DBCB_SYSTEM_LITTLE_ENDIAN)) || (defined(DBC_BLIT_DATA_BIG_ENDIAN) && defined(DBCB_SYSTEM_BIG_ENDIAN)) */
#undef dbcB_cpy

#else /* defined(DBCB_SYSTEM_LITTLE_ENDIAN) || defined(DBCB_SYSTEM_BIG_ENDIAN) */
/* Byte-wise versions. */
#if defined(DBC_BLIT_DATA_LITTLE_ENDIAN)
static dbcb_uint16 dbcb_load16(const void *p)
{
    const dbcb_uint8 *s=(dbcb_uint8 *)p;
    return (dbcb_uint16)(s[0]|(s[1]<<8));
}

static void dbcb_store16(dbcb_uint16 v,void *p)
{
    dbcb_uint8 *d=(dbcb_uint8 *)p;
    d[0]=(dbcb_uint8)(v   );
    d[1]=(dbcb_uint8)(v>>8);
}

static dbcb_uint32 dbcb_load32(const void *p)
{
    const dbcb_uint8 *s=(dbcb_uint8 *)p;
    return ((dbcb_uint32)s[0]    )|
           ((dbcb_uint32)s[1]<< 8)|
           ((dbcb_uint32)s[2]<<16)|
           ((dbcb_uint32)s[3]<<24);
}

static void dbcb_store32(dbcb_uint32 v,void *p)
{
    dbcb_uint8 *d=(dbcb_uint8 *)p;
    d[0]=(dbcb_uint8)(v    );
    d[1]=(dbcb_uint8)(v>> 8);
    d[2]=(dbcb_uint8)(v>>16);
    d[3]=(dbcb_uint8)(v>>24);
}

#ifndef DBC_BLIT_NO_INT64
static dbcb_uint64 dbcb_load64(const void *p)
{
    const dbcb_uint8 *s=(dbcb_uint8 *)p;
    return ((dbcb_uint64)s[0]    )|
           ((dbcb_uint64)s[1]<< 8)|
           ((dbcb_uint64)s[2]<<16)|
           ((dbcb_uint64)s[3]<<24)|
           ((dbcb_uint64)s[4]<<32)|
           ((dbcb_uint64)s[5]<<40)|
           ((dbcb_uint64)s[6]<<48)|
           ((dbcb_uint64)s[7]<<56);

}

static void dbcb_store64(dbcb_uint64 v,void *p)
{
    dbcb_uint8 *d=(dbcb_uint8 *)p;
    d[0]=(dbcb_uint8)(v    );
    d[1]=(dbcb_uint8)(v>> 8);
    d[2]=(dbcb_uint8)(v>>16);
    d[3]=(dbcb_uint8)(v>>24);
    d[4]=(dbcb_uint8)(v>>32);
    d[5]=(dbcb_uint8)(v>>40);
    d[6]=(dbcb_uint8)(v>>48);
    d[7]=(dbcb_uint8)(v>>56);
}
#endif /* DBC_BLIT_NO_INT64 */

#else /* defined(DBC_BLIT_DATA_LITTLE_ENDIAN) */

static dbcb_uint16 dbcb_load16(const void *p)
{
    const dbcb_uint8 *s=(dbcb_uint8 *)p;
    return (dbcb_uint16)(s[1]|(s[0]<<8));
}

static void dbcb_store16(dbcb_uint16 v,void *p)
{
    dbcb_uint8 *d=(dbcb_uint8 *)p;
    d[1]=(dbcb_uint8)(v   );
    d[0]=(dbcb_uint8)(v>>8);
}

static dbcb_uint32 dbcb_load32(const void *p)
{
    const dbcb_uint8 *s=(dbcb_uint8 *)p;
    return ((dbcb_uint32)s[3]    )|
           ((dbcb_uint32)s[2]<< 8)|
           ((dbcb_uint32)s[1]<<16)|
           ((dbcb_uint32)s[0]<<24);
}

static void dbcb_store32(dbcb_uint32 v,void *p)
{
    dbcb_uint8 *d=(dbcb_uint8 *)p;
    d[3]=(dbcb_uint8)(v    );
    d[2]=(dbcb_uint8)(v>> 8);
    d[1]=(dbcb_uint8)(v>>16);
    d[0]=(dbcb_uint8)(v>>24);
}

#ifndef DBC_BLIT_NO_INT64
static dbcb_uint64 dbcb_load64(const void *p)
{
    const dbcb_uint8 *s=(dbcb_uint8 *)p;
    return ((dbcb_uint64)s[7]    )|
           ((dbcb_uint64)s[6]<< 8)|
           ((dbcb_uint64)s[5]<<16)|
           ((dbcb_uint64)s[4]<<24)|
           ((dbcb_uint64)s[3]<<32)|
           ((dbcb_uint64)s[2]<<40)|
           ((dbcb_uint64)s[1]<<48)|
           ((dbcb_uint64)s[0]<<56);

}

static void dbcb_store64(dbcb_uint64 v,void *p)
{
    dbcb_uint8 *d=(dbcb_uint8 *)p;
    d[7]=(dbcb_uint8)(v    );
    d[6]=(dbcb_uint8)(v>> 8);
    d[5]=(dbcb_uint8)(v>>16);
    d[4]=(dbcb_uint8)(v>>24);
    d[3]=(dbcb_uint8)(v>>32);
    d[2]=(dbcb_uint8)(v>>40);
    d[1]=(dbcb_uint8)(v>>48);
    d[0]=(dbcb_uint8)(v>>56);
}
#endif /* DBC_BLIT_NO_INT64 */
#endif /* defined(DBC_BLIT_DATA_LITTLE_ENDIAN) */
#endif /* defined(DBCB_SYSTEM_LITTLE_ENDIAN) || defined(DBCB_SYSTEM_BIG_ENDIAN) */
#endif /* dbcb_load16 */

/* Controls using SSE2 on per mode basis. */
#ifndef dbcb_allow_sse2_for_mode
#define dbcb_allow_sse2_for_mode(mode,modulated) 1
#endif

/* Controls using AVX2 on per mode basis. */
#ifndef dbcb_allow_avx2_for_mode
#define dbcb_allow_avx2_for_mode(mode,modulated) 1
#endif

/* Controls the amount of unrolling. Only supported values are 0 (disable unroll) 8, 16, and 32. The default is 8. */
#ifndef DBC_BLIT_UNROLL
#define DBC_BLIT_UNROLL 8
#endif

/* Controls unrolling on per mode basis. */
#ifndef dbcb_unroll_limit_for_mode
#define dbcb_unroll_limit_for_mode(mode,modulated) (DBC_BLIT_UNROLL)
#endif

#if (defined(__MINGW32__)||defined(__MINGW64__))&&!defined(DBCB_X64)
#if !defined(DBC_BLIT_ENABLE_MINGW_SIMD) && !defined(DBC_BLIT_NO_SIMD)
#define DBC_BLIT_NO_SIMD
#endif
#endif

#if defined(DBC_BLIT_NO_GCC_ASM) && defined(DBC_BLIT_FORCE_GCC_ASM)
#error Both DBC_BLIT_NO_GCC_ASM and DBC_BLIT_FORCE_GCC_ASM are specified
#endif

/* Select assembly or intrinsics. */
#if defined(__GNUC__)
#if (__GNUC__ * 100 + __GNUC_MINOR__) >= 600 || ((__clang_major__ * 100 + __clang_minor__) >= 700) || defined(__INTEL_COMPILER)
#define DBCB_PREFER_INTRINSICS
#endif
#endif

/* SIMD functions attributes. */
#ifndef DBC_BLIT_NO_SIMD
#ifdef DBCB_X86_OR_X64
#if defined(__GNUC__)
#if defined(__SSE2__)
/*
    SSE2 is already globally enabled. No need to specify it,
    and possibly constrain the compiler from using better
    instructions.
*/
#define DBCB_DECL_SSE2
#else
/*
    Note: stdcall is there to work around the problem, pointed by
    "error: calling 'function' with SSE calling convention without SSE/SSE2 enabled"
    diagnostic. Short version: GCC seems to see fit to use SSE
    calling convention on static SSE functions, breaking the calls from
    FPU functions. So does clang, except with no error message.
    Using stdcall overrides that. Possibly unnecessary in this case,
    since the behavior seems to be triggered only if the function
    uses floating point arguments/returns.
    Note: fpmath=sse is not used, as it is not needed (SSE is only used for SIMD),
    and introduces additional hassle.
*/
#define DBCB_DECL_SSE2 __attribute__((target("sse2"),stdcall))
#endif /* defined(__SSE2__) */
#else
#define DBCB_DECL_SSE2
#endif /* defined(__GNUC__) */

#ifndef DBC_BLIT_NO_AVX2
#if defined(__GNUC__)
#if defined(__AVX2__)
/*
    AVX2 is already globally enabled. No need to specify it,
    and possibly constrain the compiler from using better
    instructions.
*/
#define DBCB_DECL_AVX2
#else
#ifdef DBCB_X64
#define DBCB_DECL_AVX2 __attribute__((target("avx2"))) /* No stdcall in x64. */
#else
#define DBCB_DECL_AVX2 __attribute__((target("avx2"),stdcall))
#endif /* DBCB_X64 */
#endif /* defined(__AVX2__) */
#else
#define DBCB_DECL_AVX2
#endif /* defined(__GNUC__) */
#endif /* DBC_BLIT_NO_AVX2 */

#endif /* DBCB_X86_OR_X64 */
#endif /* DBC_BLIT_NO_SIMD */

/*============================================================================*/
/* Static data */

#if !defined(DBC_BLIT_NO_SIMD) && defined(DBCB_X86_OR_X64)
static int dbcB_has_sse2;
#ifndef DBC_BLIT_NO_AVX2
static int dbcB_has_avx2;
#endif
#endif

#if !defined(DBC_BLIT_NO_GAMMA) && !defined(DBC_BLIT_GAMMA_NO_TABLES)
static dbcb_fp    dbcB_table_srgb2linear[256];
static dbcb_uint8 dbcB_table_linear2srgb_start[4097];
static dbcb_fp    dbcB_table_linear2srgb_threshold[4097];
#endif

/*============================================================================*/
/* Helper functions */

#ifndef DBC_BLIT_NO_GAMMA
#define DBCB_1div255  DBCB_FC(0.0039215686274509803)
#endif
#define DBCB_1div255f 0.00392156863f

#if !defined(DBC_BLIT_NO_GAMMA) && !defined(DBC_BLIT_GAMMA_NO_TABLES)

/*
    Reference versions are:

static double linear2srgb(double c)
{
    if(c<=0.0031308) return 12.92*c;
    else return 1.055*pow(c,1.0/2.4)-0.055;
}

static double srgb2linear(double c)
{
    if(c<=0.04045) return c/12.92;
    else return pow((c+0.055)/1.055,2.4);
}

*/

/* Returns approximation of (((x+y)/255+0.055)/(x/255+0.055))^2.4 - 1. */
/* Accurate to couple of ulp for intended inputs. */
static dbcb_fp dbcB_calc_gamma_scale_factor(dbcb_fp x,dbcb_fp y)
{
    const dbcb_fp c1=DBCB_FC(1.0);
    const dbcb_fp c2=DBCB_FC(0.5);
    const dbcb_fp c3=DBCB_FC(0.33333333333333333);
    const dbcb_fp c4=DBCB_FC(0.25);
    const dbcb_fp c5=DBCB_FC(0.2);
    const dbcb_fp c6=DBCB_FC(0.16666666666666666);
    const dbcb_fp c7=DBCB_FC(0.14285714285714285);
    const dbcb_fp c8=DBCB_FC(0.125);
    const dbcb_fp c9=DBCB_FC(0.11111111111111111);
    dbcb_fp t=y/(x+DBCB_FC(14.025)); /* 255*0.055 = 14.025 */
    dbcb_fp z=t/(DBCB_FC(2.0)+t);    /* log(1+t) = log((1+z)/(1-z)) */
    dbcb_fp z2=z*z;
    /* Taylor expansion (truncated) of log((1+z)/(1-z)). */
    z=DBCB_FC(2.0)*z*(c1+z2*(c3+z2*(c5+z2*(c7+z2*c9))));
    /* We clculate x^2.4 as (x^1.2)^2. */
    z*=DBCB_FC(1.2);
    /* Taylor expansion (truncated) of exp(z)-1. */
    /* Note: c1*z+(c1*c2)*z^2+(c1*c2*c3)*z^3+(c1*c2*c3*c4)*z^4+... */
    z=z*(c1+c2*z*(c1+c3*z*(c1+c4*z*(c1+c5*z*(c1+c6*z*(c1+c7*z*(c1+c8*z)))))));
    z=z*(DBCB_FC(2.0)+z);
    return z;
}

static void dbcB_populate_tables()
{
    /*
        We calculate srgb2linear() on i/255 and (i+0.5)/255 by
        sequentially multiplying previous term by
        (((i+1)/255+0.055)/(i/255+0.055))^2.4.
        This is surprisingly accurate, despite accumulating error
        during the iteration.
        Note: x=x+x*small is more accurate than x=x*(1+small).
        This gives us tables good enough to correctly compute
        all 256^3 possible blit(src,dst,alpha) for both lerp-alpha and
        premultiplied alpha - regardless of whether calculation is done in
        double or 80-bit double-extended (as x86 FPU does). This holds even
        if the rounding direction is set to non-default value (e.g.
        towards zero). Computing in float instead (DBC_BLIT_GAMMA_NO_DOUBLE)
        yields around 100 mismatches for each mode (around 0.0006%).
    */

    dbcb_int32 i,j=0,m;
    /* Note: srgb2linear(10/255) is on linear segment, srgb2linear(10.5/255) is not. */
    dbcb_fp X=DBCB_FC(0.0033465357638991608); /* srgb2linear(11/255) */
    for(i=0;i<256;++i)
    {
        dbcb_fp x,y;
        if(i<11)
        {
            const dbcb_fp C=DBCB_FC(3294.6); /* 255*12.92 = 3294.6 */
            x=(dbcb_fp)i/C;
            y=(i==10?
                DBCB_FC(0.003188300904430532): /* srgb2linear(10.5/255) */
                ((dbcb_fp)i+DBCB_FC(0.5))/C);
        }
        else
        {
            if(i==255) X=DBCB_FC(1.0);
            x=X;
            y=x+x*dbcB_calc_gamma_scale_factor((dbcb_fp)i,DBCB_FC(0.5));
            X=X+X*dbcB_calc_gamma_scale_factor((dbcb_fp)i,DBCB_FC(1.0));
        }
        dbcB_table_srgb2linear[i]=x;
        m=(dbcb_int32)(y*DBCB_FC(4096.0));
        for(;j<=m&&j<=4096;++j)
        {
            dbcB_table_linear2srgb_start[j]=(dbcb_uint8)i;
            dbcB_table_linear2srgb_threshold[j]=y;
        }
    }
}

#endif /* !defined(DBC_BLIT_NO_GAMMA) && !defined(DBC_BLIT_GAMMA_NO_TABLES) */

/*
    NOTE: For 0<=n<=255*255 the code
        n+=1;n=(n+(n>>8))>>8;
    produces the same result as
        n=n/255;
    It may or may not be faster (compilers can convert n/255 to
    (n*2155905153ULL)>>39 for uint32 and to (n*32897)>>23 for uint16), but
    has the advantage of keeping intermediate results in 16 bits, which may
    help both scalar and SIMD cases.

    Further reading: Jim Blinn, "Three Wrongs Make a Right".
*/

static dbcb_uint32 dbcB_div255_round(dbcb_uint32 n)
{
    if(sizeof(unsigned)>=sizeof(dbcb_uint16)&&sizeof(unsigned)<sizeof(dbcb_uint32))
    {
        unsigned t=(unsigned)n;
        t+=128u;
        t=(t+(t>>8))>>8;
        return (dbcb_uint32)t;
    }
    else
    {
        n+=128u;
        n=(n+(n>>8))>>8;
        return n;
    }
}

#ifndef DBC_BLIT_NO_GAMMA
static dbcb_fp dbcB_clamp0_1(dbcb_fp x)
{
    if(!(x>=DBCB_FC(0.0))) x=DBCB_FC(0.0); /* Note: also catches NaNs. */
    if(x>DBCB_FC(1.0))     x=DBCB_FC(1.0);
    return x;
}
#endif

/* Get i-th byte from uint32, little endian. */
static dbcb_uint8 dbcB_getb(dbcb_uint32 x,dbcb_uint32 i)
{
    return (dbcb_uint8)(x>>(8*i));
}

/* Construct uint32 from 4 bytes, little endian. */
static dbcb_uint32 dbcB_4x8to32(dbcb_uint8 b0,dbcb_uint8 b1,dbcb_uint8 b2,dbcb_uint8 b3)
{
    return
        ( (dbcb_uint32)b0     )^
        (((dbcb_uint32)b1)<< 8)^
        (((dbcb_uint32)b2)<<16)^
        (((dbcb_uint32)b3)<<24);
}

static float dbcB_clamp0_255(float x)
{
    if(!(x>=0.0f)) x=0.0f; /* Note: also catches NaNs. */
    if(x>255.0f) x=255.0f;
    return x;
}

static float dbcB_byte2float(dbcb_uint8 x)
{
    return (float)x;
}

static dbcb_uint8 dbcB_float2byte(float x)
{
    /* Note: float <-> int is faster, than float <-> uint, at least on x86. */
    return (dbcb_uint8)(dbcb_int32)(x+0.5f);
}

#ifndef DBC_BLIT_NO_GAMMA

static dbcb_fp dbcB_srgb2linear(dbcb_uint8 x)
{
#ifndef DBC_BLIT_GAMMA_NO_TABLES
    return dbcB_table_srgb2linear[x];
#else
    dbcb_fp y=(dbcb_fp)x;
/* (MACRO+0) allows MACRO to be empty. */
#if   (DBC_BLIT_GAMMA_NO_TABLES + 0)==3 || (DBC_BLIT_GAMMA_NO_TABLES + 0)==2
    /*
        3rd degree polynomial approximation on [0;255].
        Accuracy: 255*Eabs<0.342.
    */
#define DBCB_S2L_C1 DBCB_FC(7.1150847263545382e-05)
#define DBCB_S2L_C2 DBCB_FC(1.0264384548986879e-05)
#define DBCB_S2L_C3 DBCB_FC(1.8961933413237702e-08)
    y=y*(
        DBCB_S2L_C1+y*(
        DBCB_S2L_C2+y*(
        DBCB_S2L_C3)));
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==1
    /*
        5th degree polynomial approximation on [0;255].
        Accuracy: 255*Eabs<0.077.
    */
#define DBCB_S2L_C1 DBCB_FC(+2.169755680e-04)
#define DBCB_S2L_C2 DBCB_FC(+5.821137890e-06)
#define DBCB_S2L_C3 DBCB_FC(+6.077312940e-08)
#define DBCB_S2L_C4 DBCB_FC(-1.554613300e-10)
#define DBCB_S2L_C5 DBCB_FC(+2.001283376e-13)
    y=y*(
        DBCB_S2L_C1+y*(
        DBCB_S2L_C2+y*(
        DBCB_S2L_C3+y*(
        DBCB_S2L_C4+y*(
        DBCB_S2L_C5)))));
#else
    /*
        Rational 4/2 approximation on [0;255].
        Accuracy: 255*Eabs<0.012.
    */
#define DBCB_S2L_P1 DBCB_FC(3.504336698800e-04)
#define DBCB_S2L_P2 DBCB_FC(1.375386972964e-05)
#define DBCB_S2L_P3 DBCB_FC(8.945664356306e-07)
#define DBCB_S2L_P4 DBCB_FC(3.622358507480e-09)
#define DBCB_S2L_Q0 DBCB_FC(1.0)
#define DBCB_S2L_Q1 DBCB_FC(9.18394954e-02)
#define DBCB_S2L_Q2 DBCB_FC(1.03252838e-04)
    dbcb_fp P=y*(
        DBCB_S2L_P1+y*(
        DBCB_S2L_P2+y*(
        DBCB_S2L_P3+y*(
        DBCB_S2L_P4))));
    dbcb_fp Q=
        DBCB_S2L_Q0+y*(
        DBCB_S2L_Q1+y*(
        DBCB_S2L_Q2));
    y=P/Q;
#endif
    return y;
#endif /* DBC_BLIT_GAMMA_NO_TABLES */
}

static dbcb_uint8 dbcB_linear2srgb(dbcb_fp x)
{
#ifndef DBC_BLIT_GAMMA_NO_TABLES
    dbcb_int32 id=(dbcb_int32)(x*DBCB_FC(4096.0));
    return (dbcb_uint8)(dbcB_table_linear2srgb_start[id]+(x>=dbcB_table_linear2srgb_threshold[id]));
#else
/* (MACRO+0) allows MACRO to be empty. */
#if   (DBC_BLIT_GAMMA_NO_TABLES + 0)==3
    /*
        3rd degree polynomial approximation on [0;1].
        Accuracy: Eabs<21.979.
    */
#define DBCB_L2S_C1 DBCB_FC( 9.4081185756021148e+02)
#define DBCB_L2S_C2 DBCB_FC(-1.4968549011399689e+03)
#define DBCB_L2S_C3 DBCB_FC( 8.1104304357975741e+02)
    x=x*(
        DBCB_L2S_C1+x*(
        DBCB_L2S_C2+x*(
        DBCB_L2S_C3)));
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==2
    /*
        Rational 2/2 approximation on [0;1].
        Accuracy: Eabs<2.025.
    */
#define DBCB_L2S_P1 DBCB_FC(3.3262919020957675e+03)
#define DBCB_L2S_P2 DBCB_FC(1.6696757946598922e+04)
#define DBCB_L2S_Q0 DBCB_FC(1.0)
#define DBCB_L2S_Q1 DBCB_FC(4.2616518139966423e+01)
#define DBCB_L2S_Q2 DBCB_FC(3.4905245972561779e+01)
    dbcb_fp P=x*(
        DBCB_L2S_P1+x*(
        DBCB_L2S_P2));
    dbcb_fp Q=
        DBCB_L2S_Q0+x*(
        DBCB_L2S_Q1+x*(
        DBCB_L2S_Q2));
    x=P/Q;
#elif  (DBC_BLIT_GAMMA_NO_TABLES + 0)==1
    /*
        Rational 3/2 approximation on [0;1].
        Accuracy: Eabs<0.882.
    */
#define DBCB_L2S_P1 DBCB_FC(3.8763097396320477e+03)
#define DBCB_L2S_P2 DBCB_FC(4.9039460967320607e+04)
#define DBCB_L2S_P3 DBCB_FC(1.7125066590537979e+04)
#define DBCB_L2S_Q0 DBCB_FC(1.0)
#define DBCB_L2S_Q1 DBCB_FC(7.0676114602914467e+01)
#define DBCB_L2S_Q2 DBCB_FC(2.0299383558332335e+02)
    dbcb_fp P=x*(
        DBCB_L2S_P1+x*(
        DBCB_L2S_P2+x*(
        DBCB_L2S_P3)));
    dbcb_fp Q=
        DBCB_L2S_Q0+x*(
        DBCB_L2S_Q1+x*(
        DBCB_L2S_Q2));
    x=P/Q;
#else
    /*
        Rational 4/4 approximation on [0;1].
        Accuracy: Eabs<0.665.
    */
#define DBCB_L2S_P1 DBCB_FC(2.1703371664100814e+03)
#define DBCB_L2S_P2 DBCB_FC(5.4589682843458904e+06)
#define DBCB_L2S_P3 DBCB_FC(1.4517409768971977e+08)
#define DBCB_L2S_P4 DBCB_FC(2.3353720686367840e+08)
#define DBCB_L2S_Q0 DBCB_FC(1.0)
#define DBCB_L2S_Q1 DBCB_FC(1.2498314123010732e+03)
#define DBCB_L2S_Q2 DBCB_FC(1.3553779734820861e+05)
#define DBCB_L2S_Q3 DBCB_FC(9.8910205374230759e+05)
#define DBCB_L2S_Q4 DBCB_FC(3.8066791818310641e+05)
    dbcb_fp P=x*(
        DBCB_L2S_P1+x*(
        DBCB_L2S_P2+x*(
        DBCB_L2S_P3+x*(
        DBCB_L2S_P4))));
    dbcb_fp Q=
        DBCB_L2S_Q0+x*(
        DBCB_L2S_Q1+x*(
        DBCB_L2S_Q2+x*(
        DBCB_L2S_Q3+x*(
        DBCB_L2S_Q4))));
    x=P/Q;
#endif
    return (dbcb_uint8)(dbcb_int32)(x+DBCB_FC(0.5));
#endif /* DBC_BLIT_GAMMA_NO_TABLES */
}

#endif /* DBC_BLIT_NO_GAMMA */

/*============================================================================*/
/* Single-channel composition functions. */

static dbcb_uint8 dbcB_cla(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a)
{
    dbcb_uint32 ret=(dbcb_uint32)s*(dbcb_uint32)a+(dbcb_uint32)d*(255u-(dbcb_uint32)a);
    return (dbcb_uint8)dbcB_div255_round(ret);
}

static dbcb_uint8 dbcB_clp(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a)
{
    dbcb_uint32 ret=(dbcb_uint32)d*(255u-(dbcb_uint32)a);
    ret=dbcB_div255_round(ret);
    ret+=(dbcb_uint32)s;
    /* if(ret>255u) ret=255u; */
    ret|=(ret>>8)*255u;
    return (dbcb_uint8)ret;
}

static dbcb_uint8 dbcB_clam(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a,float m,float c)
{
    float ret;
    c=c*dbcB_byte2float(a);
    ret=dbcB_byte2float(s)*m*c+dbcB_byte2float(d)*(255.0f-c);
    return dbcB_float2byte(ret*DBCB_1div255f);
}

static dbcb_uint8 dbcB_clpm(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a,float m,float c)
{
    float ret;
    c=c*dbcB_byte2float(a);
    ret=dbcB_byte2float(s)*m+dbcB_byte2float(d)*(255.0f-c)*DBCB_1div255f;
    return dbcB_float2byte(dbcB_clamp0_255(ret));
}

static dbcb_uint8 dbcB_clx(dbcb_uint8 s,dbcb_uint8 d)
{
    dbcb_uint32 ret=(dbcb_uint32)(s)*(dbcb_uint32)(d);
    ret=dbcB_div255_round(ret);
    return (dbcb_uint8)(ret);
}

static dbcb_uint8 dbcB_clxm(dbcb_uint8 s,dbcb_uint8 d,float m)
{
    float ret=dbcB_byte2float(s)*dbcB_byte2float(d)*m*DBCB_1div255f;
    return dbcB_float2byte(dbcB_clamp0_255(ret));
}

#ifndef DBC_BLIT_NO_GAMMA

static dbcb_uint8 dbcB_cga(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a)
{
    dbcb_fp S=dbcB_srgb2linear(s);
    dbcb_fp D=dbcB_srgb2linear(d);
    dbcb_fp A=(dbcb_fp)(a)*DBCB_1div255;
    dbcb_fp ret=S*A+D*(DBCB_FC(1.0)-A);
    return dbcB_linear2srgb(ret);
}

static dbcb_uint8 dbcB_cgp(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a)
{
    dbcb_fp S=dbcB_srgb2linear(s);
    dbcb_fp D=dbcB_srgb2linear(d);
    dbcb_fp A=(dbcb_fp)(a)*DBCB_1div255;
    dbcb_fp ret=S+D*(DBCB_FC(1.0)-A);
    return dbcB_linear2srgb(dbcB_clamp0_1(ret));
}

static dbcb_uint8 dbcB_cgam(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a,float m,float c)
{
    dbcb_fp S=dbcB_srgb2linear(s);
    dbcb_fp D=dbcB_srgb2linear(d);
    dbcb_fp A=(dbcb_fp)(a)*DBCB_1div255;
    dbcb_fp ret;
    A*=(dbcb_fp)c;
    ret=S*(dbcb_fp)m*A+D*(DBCB_FC(1.0)-A);
    return dbcB_linear2srgb(dbcB_clamp0_1(ret));
}

static dbcb_uint8 dbcB_cgpm(dbcb_uint8 s,dbcb_uint8 d,dbcb_uint8 a,float m,float c)
{
    dbcb_fp S=dbcB_srgb2linear(s);
    dbcb_fp D=dbcB_srgb2linear(d);
    dbcb_fp A=(dbcb_fp)(a)*DBCB_1div255;
    dbcb_fp ret=S*(dbcb_fp)m+D*(DBCB_FC(1.0)-(dbcb_fp)c*A);
    return dbcB_linear2srgb(dbcB_clamp0_1(ret));
}

static dbcb_uint8 dbcB_cgx(dbcb_uint8 s,dbcb_uint8 d)
{
    dbcb_fp S=dbcB_srgb2linear(s);
    dbcb_fp D=dbcB_srgb2linear(d);
    dbcb_fp ret=S*D;
    return dbcB_linear2srgb(ret);
}

static dbcb_uint8 dbcB_cgxm(dbcb_uint8 s,dbcb_uint8 d,float m)
{
    dbcb_fp S=dbcB_srgb2linear(s);
    dbcb_fp D=dbcB_srgb2linear(d);
    dbcb_fp ret=S*D*(dbcb_fp)m;
    return dbcB_linear2srgb(dbcB_clamp0_1(ret));
}

#endif /* DBC_BLIT_NO_GAMMA */

/*============================================================================*/
/* Pixel composition functions. */
/* Note: all functions have no alignment requirements. */

/* C versions. */

/* Copies single pixel, with modulation. */
static void dbcB_b32m_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    dbcb_store32(dbcB_4x8to32(
        dbcB_float2byte(dbcB_clamp0_255(color[0]*dbcB_byte2float(dbcB_getb(S,0)))),
        dbcB_float2byte(dbcB_clamp0_255(color[1]*dbcB_byte2float(dbcB_getb(S,1)))),
        dbcB_float2byte(dbcB_clamp0_255(color[2]*dbcB_byte2float(dbcB_getb(S,2)))),
        dbcB_float2byte(dbcB_clamp0_255(color[3]*dbcB_byte2float(dbcB_getb(S,3))))),
        d);
}

/* Alpha-blends single pixel, linear. */
static void dbcB_bla_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    if(sizeof(unsigned)<sizeof(dbcb_uint32))
    {
        dbcb_uint32 S=dbcb_load32(s);
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_store32(dbcB_4x8to32(
            dbcB_cla(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3)),
            dbcB_cla(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3)),
            dbcB_cla(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3)),
            dbcB_cla(           255,dbcB_getb(D,3),dbcB_getb(S,3))),
            d);
    }
    else
    {
        dbcb_uint32 S=dbcb_load32(s);
        if(S>0x00FFFFFFu)
        {
            if(S<0xFF000000u)
            {
#if !defined(DBC_BLIT_NO_INT64)
                if(sizeof(void*)>sizeof(dbcb_uint32)) /* Actually slower on 32-bit systems, at least x86. */
                {
                    const dbcb_uint64 M=DBCB_C64(0x00FF00FFu,0x00FF00FFu);
                    dbcb_uint32 D=dbcb_load32(d);
                    dbcb_uint32 A=S>>24,B=255-A;
                    dbcb_uint64 St;
                    dbcb_uint64 Dt;
                    dbcb_uint64 T;
                    S|=0xFF000000u;
                    St=(S&0x00FF00FFu)^((dbcb_uint64)(S&0xFF00FF00u)<<24); /* RGBA -> R0B0G0A0 */
                    Dt=(D&0x00FF00FFu)^((dbcb_uint64)(D&0xFF00FF00u)<<24); /* RGBA -> R0B0G0A0 */
                    T=A*St+B*Dt+DBCB_C64(0x00800080u,0x00800080u);
                    T=((T+((T>>8)&M))>>8)&M;
                    S=(dbcb_uint32)(T^(T>>24)); /* R0B0G0A0 -> RGBA */
                }
                else
#endif
                {
                    const dbcb_uint32 M=0x00FF00FFu;
                    dbcb_uint32 D=dbcb_load32(d);
                    dbcb_uint32 A=S>>24,B=255-A;
                    dbcb_uint32 SL,SH;
                    dbcb_uint32 DL,DH;
                    dbcb_uint32 L;
                    dbcb_uint32 H;
                    S|=0xFF000000u;
                    SL=S&M,SH=(S>>8)&M;
                    DL=D&M,DH=(D>>8)&M;
                    L=A*SL+B*DL;
                    H=A*SH+B*DH;
                    L+=0x00800080u;
                    H+=0x00800080u;
                    L=((L+((L>>8)&M))>>8)&M;
                    H=((H+((H>>8)&M))>>8)&M;
                    S=L^(H<<8);
                }
            }
            dbcb_store32(S,d);
        }
    }
}

/* Alpha-blends 2 pixels, linear. */
static void dbcB_bla_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bla_1_c(s  ,d  );
    dbcB_bla_1_c(s+4,d+4);
}

/* Alpha-blends 4 pixels, linear. */
static void dbcB_bla_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bla_2_c(s  ,d  );
    dbcB_bla_2_c(s+8,d+8);
}

/* Alpha-blends (PMA) single pixel, linear. */
static void dbcB_blp_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    if(sizeof(unsigned)<sizeof(dbcb_uint32))
    {
        dbcb_uint32 S=dbcb_load32(s);
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_store32(dbcB_4x8to32(
            dbcB_clp(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3)),
            dbcB_clp(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3)),
            dbcB_clp(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3)),
            dbcB_clp(dbcB_getb(S,3),dbcB_getb(D,3),dbcB_getb(S,3))),
            d);
    }
    else
    {
        dbcb_uint32 S=dbcb_load32(s);
        dbcb_uint32 D=dbcb_load32(d);
        if(S<0xFF000000u&&D>0u)
        {
#if !defined(DBC_BLIT_NO_INT64)
            if(sizeof(void*)>sizeof(dbcb_uint32)) /* Actually slower on 32-bit systems, at least x86. */
            {
                const dbcb_uint64 M=DBCB_C64(0x00FF00FFu,0x00FF00FFu);
                dbcb_uint32 A=S>>24,B=255-A;
                dbcb_uint64 St=(S&0x00FF00FFu)^((dbcb_uint64)(S&0xFF00FF00u)<<24); /* RGBA -> R0B0G0A0 */
                dbcb_uint64 Dt=(D&0x00FF00FFu)^((dbcb_uint64)(D&0xFF00FF00u)<<24); /* RGBA -> R0B0G0A0 */
                dbcb_uint64 T=B*Dt+DBCB_C64(0x00800080u,0x00800080u);
                dbcb_uint64 C;
                T=((T+((T>>8)&M))>>8)&M;
                T+=St;
                C=T&(M<<8);
                C-=C>>8;
                T=(T&M)|C;
                S=(dbcb_uint32)(T^(T>>24)); /* R0B0G0A0 -> RGBA */
            }
            else
#endif
            {
                const dbcb_uint32 M=0x00FF00FFu;
                dbcb_uint32 A=S>>24,B=255-A;
                dbcb_uint32 SL=S&M,SH=(S>>8)&M;
                dbcb_uint32 DL=D&M,DH=(D>>8)&M;
                dbcb_uint32 L=B*DL;
                dbcb_uint32 H=B*DH;
                dbcb_uint32 CL,CH;
                L+=0x00800080u;
                H+=0x00800080u;
                L=((L+((L>>8)&M))>>8)&M;
                H=((H+((H>>8)&M))>>8)&M;
                L+=SL;
                H+=SH;
                CL=L&(M<<8);
                CH=H&(M<<8);
                CL-=CL>>8;
                CH-=CH>>8;
                L=(L&M)|CL;
                H=(H&M)|CH;
                S=L^(H<<8);
            }
        }
        dbcb_store32(S,d);
    }
}

/* Alpha-blends (PMA) 2 pixels, linear. */
static void dbcB_blp_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_blp_1_c(s  ,d  );
    dbcB_blp_1_c(s+4,d+4);    
}

/* Alpha-blends (PMA) 4 pixels, linear. */
static void dbcB_blp_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_blp_2_c(s  ,d  );
    dbcB_blp_2_c(s+8,d+8);
}

/* Alpha-blends single pixel, linear, with modulation. */
static void dbcB_blam_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>0x00FFFFFFu&&color[3]!=0.0f)
    {
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_store32(dbcB_4x8to32(
            dbcB_clam(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3),color[0],color[3]),
            dbcB_clam(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3),color[1],color[3]),
            dbcB_clam(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3),color[2],color[3]),
            dbcB_clam(           255,dbcB_getb(D,3),dbcB_getb(S,3),    1.0f,color[3])),
        d);
    }
}

/* Alpha-blends (PMA) single pixel, linear, with modulation. */
static void dbcB_blpm_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>0u)
    {
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_store32(dbcB_4x8to32(
            dbcB_clpm(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3),color[0],color[3]),
            dbcB_clpm(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3),color[1],color[3]),
            dbcB_clpm(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3),color[2],color[3]),
            dbcB_clpm(dbcB_getb(S,3),dbcB_getb(D,3),dbcB_getb(S,3),color[3],color[3])),
            d);
    }
}

/* Blits single 8-bit pixel with colorkey. */
static void dbcB_b8m_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 key)
{
    if(s[0]!=key) d[0]=s[0];
}

/* Blits 2 8-bit pixels with colorkey. */
static void dbcB_b8m_2_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 key)
{
#if 0 /* Actually slower. */
    dbcb_uint16 S=dbcb_load16(s);
    dbcb_uint16 D=dbcb_load16(d);
    dbcb_uint16 m=(dbcb_uint16)(S^(key*0x0101u));
    m=(m|(m>>1))&0x5555u;
    m=(m|(m>>2))&0x3333u;
    m=(m|(m>>4))&0x0F0Fu;
    m=((m<<8)-m); /* m*=0xFFu; Works even if high bit is shifted out. */
    S=(S&m)^(D&~m);
    dbcb_store16(S,d);
#else
    dbcB_b8m_1_c(s  ,d  ,key);
    dbcB_b8m_1_c(s+1,d+1,key);
#endif
}

/* Blits 4 8-bit pixels with colorkey. */
static void dbcB_b8m_4_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 key)
{
    if(sizeof(unsigned)>=sizeof(dbcb_uint32))
    {
        dbcb_uint32 S=dbcb_load32(s);
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_uint32 m=(dbcb_uint32)(S^(key*0x01010101u));
        m=(m|(m>>1))&0x55555555u;
        m=(m|(m>>2))&0x33333333u;
        m=(m|(m>>4))&0x0F0F0F0Fu;
        m=((m<<8)-m); /* m*=0xFFu; Works even if high bit is shifted out. */
        S=(S&m)^(D&~m);
        dbcb_store32(S,d);
    }
    else
    {
        dbcB_b8m_2_c(s  ,d  ,key);
        dbcB_b8m_2_c(s+2,d+2,key);
    }
}

/* Blits 8 8-bit pixels with colorkey. */
static void dbcB_b8m_8_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 key)
{
#ifndef DBC_BLIT_NO_INT64
    if(sizeof(void*)>sizeof(dbcb_uint32)) /* Actually slower on 32-bit systems, at least x86. */
    {
        dbcb_uint64 S=dbcb_load64(s);
        dbcb_uint64 D=dbcb_load64(d);
        dbcb_uint64 m=(S^((dbcb_uint64)key*DBCB_C64(0x01010101u,0x01010101u)));
        m=(m|(m>>1))&DBCB_C64(0x55555555u,0x55555555u);
        m=(m|(m>>2))&DBCB_C64(0x33333333u,0x33333333u);
        m=(m|(m>>4))&DBCB_C64(0x0F0F0F0Fu,0x0F0F0F0Fu);
        m=((m<<8)-m); /* m*=0xFFu; Works even if high bit is shifted out. */
        S=(S&m)^(D&~m);
        dbcb_store64(S,d);
    }
    else
#endif
    {
        dbcB_b8m_4_c(s  ,d  ,key);
        dbcB_b8m_4_c(s+4,d+4,key);
    }
}

/* Blits 16 8-bit pixels with colorkey. */
static void dbcB_b8m_16_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 key)
{
    dbcB_b8m_8_c(s  ,d  ,key);
    dbcB_b8m_8_c(s+8,d+8,key);
}

/* Blits single 16-bit pixel with colorkey. */
static void dbcB_b16m_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint16 key)
{
    dbcb_uint16 S=dbcb_load16(s);
    if(S!=key) dbcb_store16(S,d);
}

/* Blits 2 16-bit pixels with colorkey. */
static void dbcB_b16m_2_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint16 key)
{
#if 0 /* Actually slower. */
    dbcb_uint32 S=dbcb_load32(s);
    dbcb_uint32 D=dbcb_load32(d);
    dbcb_uint32 m=S^((dbcb_uint32)key*0x00010001u);
    m=(m|(m>>1))&0x55555555u;
    m=(m|(m>>2))&0x33333333u;
    m=(m|(m>>4))&0x0F0F0F0Fu;
    m=(m|(m>>8))&0x00FF00FFu;
    m=((m<<16)-m); /* m*=0xFFFFu; Works even if high bit is shifted out. */
    S=(S&m)^(D&~m);
    dbcb_store32(S,d);
#else
    dbcB_b16m_1_c(s  ,d  ,key);
    dbcB_b16m_1_c(s+2,d+2,key);
#endif
}

/* Blits 4 16-bit pixels with colorkey. */
static void dbcB_b16m_4_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint16 key)
{
#ifndef DBC_BLIT_NO_INT64
    if(sizeof(void*)>sizeof(dbcb_uint32)) /* Actually slower on 32-bit systems, at least x86. */
    {
        dbcb_uint64 S=dbcb_load64(s);
        dbcb_uint64 D=dbcb_load64(d);
        dbcb_uint64 m=S^((dbcb_uint64)key*DBCB_C64(0x00010001u,0x00010001u));
        m=(m|(m>>1))&DBCB_C64(0x55555555u,0x55555555u);
        m=(m|(m>>2))&DBCB_C64(0x33333333u,0x33333333u);
        m=(m|(m>>4))&DBCB_C64(0x0F0F0F0Fu,0x0F0F0F0Fu);
        m=(m|(m>>8))&DBCB_C64(0x00FF00FFu,0x00FF00FFu);
        m=((m<<16)-m); /* m*=0xFFFFu; Works even if high bit is shifted out. */
        S=(S&m)^(D&~m);
        dbcb_store64(S,d);
    }
    else
#endif
    {
        dbcB_b16m_2_c(s  ,d  ,key);
        dbcB_b16m_2_c(s+4,d+4,key);
    }
}

/* Blits 8 16-bit pixels with colorkey. */
static void dbcB_b16m_8_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint16 key)
{
    dbcB_b16m_4_c(s  ,d  ,key);
    dbcB_b16m_4_c(s+8,d+8,key);
}

/* Blits single 16-bit (5551) pixel. */
static void dbcB_b5551_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcb_uint16 S=dbcb_load16(s);
    if(S>=0x8000u) dbcb_store16(S,d);
}

/* Blits 2 16-bit (5551) pixels. */
static void dbcB_b5551_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    if(sizeof(unsigned)>=sizeof(dbcb_uint32))
    {
        dbcb_uint32 S=dbcb_load32(s);
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_uint32 m=(dbcb_uint32)(S&0x80008000u);
        m=(m<<1)-(m>>15); /* m=(m>>15)*0xFFFFu; Works even if high bit is shifted out. */
        S=(S&m)^(D&~m);
        dbcb_store32(S,d);
    }
    else
    {
        dbcB_b5551_1_c(s  ,d  );
        dbcB_b5551_1_c(s+2,d+2);
    }
}

/* Blits 4 16-bit (5551) pixels. */
static void dbcB_b5551_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
#ifndef DBC_BLIT_NO_INT64
    if(sizeof(void*)>sizeof(dbcb_uint32)) /* Actually slower on 32-bit systems, at least x86. */
    {
        dbcb_uint64 S=dbcb_load64(s);
        dbcb_uint64 D=dbcb_load64(d);
        dbcb_uint64 m=(dbcb_uint64)(S&DBCB_C64(0x80008000u,0x80008000u));
        m=(m<<1)-(m>>15); /* m=(m>>15)*0xFFFFu; Works even if high bit is shifted out. */
        S=(S&m)^(D&~m);
        dbcb_store64(S,d);
    }
    else
#endif
    {
        dbcB_b5551_2_c(s  ,d  );
        dbcB_b5551_2_c(s+4,d+4);
    }
}

/* Blits 8 16-bit (5551) pixels. */
static void dbcB_b5551_8_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_b5551_4_c(s  ,d  );
    dbcB_b5551_4_c(s+8,d+8);
}

/* Blits single pixel, with alpha-test. */
static void dbcB_b32t_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 threshold)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>=(dbcb_uint32)threshold<<24) dbcb_store32(S,d);
}

/* Blits 2 pixels, with alpha-test. */
static void dbcB_b32t_2_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 threshold)
{
    dbcB_b32t_1_c(s  ,d  ,threshold);
    dbcB_b32t_1_c(s+4,d+4,threshold);
}

/* Blits 4 pixels, with alpha-test. */
static void dbcB_b32t_4_c(const dbcb_uint8 *s,dbcb_uint8 *d,dbcb_uint8 threshold)
{
    dbcB_b32t_2_c(s  ,d  ,threshold);
    dbcB_b32t_2_c(s+8,d+8,threshold);
}

/* Blits single pixel, with alpha-test using threshold 128. */
static void dbcB_b32s_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>=0x80000000u) dbcb_store32(S,d);
}

/* Blits 2 pixels, with alpha-test using threshold 128. */
static void dbcB_b32s_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
#ifndef DBC_BLIT_NO_INT64
    if(sizeof(void*)>sizeof(dbcb_uint32)) /* Actually slower on 32-bit systems, at least x86. */
    {
        dbcb_uint64 S=dbcb_load64(s);
        dbcb_uint64 D=dbcb_load64(d);
        dbcb_uint64 m=(dbcb_uint64)(S&DBCB_C64(0x80000000u,0x80000000u));
        m=(m<<1)-(m>>31); /* m=(m>>31)*0xFFFFFFFFu; Works even if high bit is shifted out. */
        S=(S&m)^(D&~m);
        dbcb_store64(S,d);
    }
    else
#endif
    {
        dbcB_b32s_1_c(s  ,d  );
        dbcB_b32s_1_c(s+4,d+4);
    }
}

/* Blits 4 pixels, with alpha-test using threshold 128. */
static void dbcB_b32s_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_b32s_2_c(s  ,d  );
    dbcB_b32s_2_c(s+8,d+8);
}

/* Multiplies single pixel, linear. */
static void dbcB_blx_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S<0xFFFFFFFFu)
    {
        if(S==0u) dbcb_store32(S,d);
        else
        {
            dbcb_uint32 D=dbcb_load32(d);
            if(D>0u)
                dbcb_store32(dbcB_4x8to32(
                    dbcB_clx(dbcB_getb(S,0),dbcB_getb(D,0)),
                    dbcB_clx(dbcB_getb(S,1),dbcB_getb(D,1)),
                    dbcB_clx(dbcB_getb(S,2),dbcB_getb(D,2)),
                    dbcB_clx(dbcB_getb(S,3),dbcB_getb(D,3))),
                    d);
        }
    }
}

/* Multiplies 2 pixels, linear. */
static void dbcB_blx_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_blx_1_c(s  ,d  );
    dbcB_blx_1_c(s+4,d+4);
}

/* Multiplies 4 pixels, linear. */
static void dbcB_blx_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_blx_2_c(s  ,d  );
    dbcB_blx_2_c(s+8,d+8);
}

/* Multiplies single pixel, linear, with modulation. */
static void dbcB_blxm_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S==0u) dbcb_store32(S,d);
    else
    {
        dbcb_uint32 D=dbcb_load32(d);
        if(D>0u)
            dbcb_store32(dbcB_4x8to32(
                dbcB_clxm(dbcB_getb(S,0),dbcB_getb(D,0),color[0]),
                dbcB_clxm(dbcB_getb(S,1),dbcB_getb(D,1),color[1]),
                dbcB_clxm(dbcB_getb(S,2),dbcB_getb(D,2),color[2]),
                dbcB_clxm(dbcB_getb(S,3),dbcB_getb(D,3),color[3])),
                d);
    }
}

#ifndef DBC_BLIT_NO_GAMMA
/*
    Note: tests for the trivial cases are to speed them up, but
    also to get the exact result, in case the srgb <-> linear
    approximations do not round-trip.
*/

/* Copies single pixel, gamma-corrected, with modulation. */
static void dbcB_b32g_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    dbcb_store32(dbcB_4x8to32(
        dbcB_linear2srgb(dbcB_clamp0_1((dbcb_fp)color[0]*dbcB_srgb2linear(dbcB_getb(S,0)))),
        dbcB_linear2srgb(dbcB_clamp0_1((dbcb_fp)color[1]*dbcB_srgb2linear(dbcB_getb(S,1)))),
        dbcB_linear2srgb(dbcB_clamp0_1((dbcb_fp)color[2]*dbcB_srgb2linear(dbcB_getb(S,2)))),
        dbcB_float2byte (dbcB_clamp0_255(       color[3]*dbcB_byte2float (dbcB_getb(S,3))))),
        d);
}

/* Alpha-blends single pixel, gamma-corrected. */
static void dbcB_bga_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>0x00FFFFFFu)
    {
        if(S>=0xFF000000u) dbcb_store32(S,d);
        else
        {
            dbcb_uint32 D=dbcb_load32(d);
            dbcb_store32(dbcB_4x8to32(
                dbcB_cga(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3)),
                dbcB_cga(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3)),
                dbcB_cga(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3)),
                dbcB_cla(           255,dbcB_getb(D,3),dbcB_getb(S,3))),
                d);
        }
    }
}

/* Alpha-blends 2 pixels, gamma-corrected. */
static void dbcB_bga_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bga_1_c(s  ,d  );
    dbcB_bga_1_c(s+4,d+4);
}

/* Alpha-blends 4 pixels, gamma-corrected. */
static void dbcB_bga_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bga_2_c(s  ,d  );
    dbcB_bga_2_c(s+8,d+8);
}

/* Alpha-blends (PMA) single pixel, gamma-corrected. */
static void dbcB_bgp_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>0u)
    {
        if(S>=0xFF000000u) dbcb_store32(S,d);
        else
        {
            dbcb_uint32 D=dbcb_load32(d);
            if(D==0u) dbcb_store32(S,d);
            else dbcb_store32(dbcB_4x8to32(
                dbcB_cgp(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3)),
                dbcB_cgp(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3)),
                dbcB_cgp(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3)),
                dbcB_clp(dbcB_getb(S,3),dbcB_getb(D,3),dbcB_getb(S,3))),
                d);
        }
    }
}

/* Alpha-blends (PMA) 2 pixels, gamma-corrected. */
static void dbcB_bgp_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bgp_1_c(s  ,d  );
    dbcB_bgp_1_c(s+4,d+4);
}

/* Alpha-blends (PMA) 4 pixels, gamma-corrected. */
static void dbcB_bgp_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bgp_2_c(s  ,d  );
    dbcB_bgp_2_c(s+8,d+8);
}

/* Multiplies single pixel, gamma-corrected. */
static void dbcB_bgx_1_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S<0xFFFFFFFFu)
    {
        if(S==0u) dbcb_store32(S,d);
        else
        {
            dbcb_uint32 D=dbcb_load32(d);
            if(D==0u) dbcb_store32(D,d);
            else dbcb_store32(dbcB_4x8to32(
                dbcB_cgx(dbcB_getb(S,0),dbcB_getb(D,0)),
                dbcB_cgx(dbcB_getb(S,1),dbcB_getb(D,1)),
                dbcB_cgx(dbcB_getb(S,2),dbcB_getb(D,2)),
                dbcB_clx(dbcB_getb(S,3),dbcB_getb(D,3))),
                d);
        }
    } 
}

/* Multiplies 2 pixels, gamma-corrected. */
static void dbcB_bgx_2_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bgx_1_c(s  ,d  );
    dbcB_bgx_1_c(s+4,d+4);
}

/* Multiplies 4 pixels, gamma-corrected. */
static void dbcB_bgx_4_c(const dbcb_uint8 *s,dbcb_uint8 *d)
{
    dbcB_bgx_2_c(s  ,d  );
    dbcB_bgx_2_c(s+8,d+8);
}

/* Alpha-blends single pixel, gamma-corrected, with modulation. */
static void dbcB_bgam_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>0x00FFFFFFu&&color[3]!=0.0f)
    {
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_store32(dbcB_4x8to32(
            dbcB_cgam(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3),color[0],color[3]),
            dbcB_cgam(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3),color[1],color[3]),
            dbcB_cgam(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3),color[2],color[3]),
            dbcB_clam(           255,dbcB_getb(D,3),dbcB_getb(S,3),    1.0f,color[3])),
            d);
    }
}

/* Alpha-blends (PMA) single pixel, gamma-corrected, with modulation. */
static void dbcB_bgpm_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S>0u)
    {
        dbcb_uint32 D=dbcb_load32(d);
        dbcb_store32(dbcB_4x8to32(
            dbcB_cgpm(dbcB_getb(S,0),dbcB_getb(D,0),dbcB_getb(S,3),color[0],color[3]),
            dbcB_cgpm(dbcB_getb(S,1),dbcB_getb(D,1),dbcB_getb(S,3),color[1],color[3]),
            dbcB_cgpm(dbcB_getb(S,2),dbcB_getb(D,2),dbcB_getb(S,3),color[2],color[3]),
            dbcB_clpm(dbcB_getb(S,3),dbcB_getb(D,3),dbcB_getb(S,3),color[3],color[3])),
            d);
    }
}

/* Multiplies single pixel, gamma-corrected, with modulation. */
static void dbcB_bgxm_1_c(const dbcb_uint8 *s,dbcb_uint8 *d,const float *color)
{
    dbcb_uint32 S=dbcb_load32(s);
    if(S==0u) dbcb_store32(S,d);
    else
    {
        dbcb_uint32 D=dbcb_load32(d);
        if(D==0u) dbcb_store32(D,d);
        else dbcb_store32(dbcB_4x8to32(
            dbcB_cgxm(dbcB_getb(S,0),dbcB_getb(D,0),color[0]),
            dbcB_cgxm(dbcB_getb(S,1),dbcB_getb(D,1),color[1]),
            dbcB_cgxm(dbcB_getb(S,2),dbcB_getb(D,2),color[2]),
            dbcB_clxm(dbcB_getb(S,3),dbcB_getb(D,3),color[3])),
            d);
    }
}

#endif /* DBC_BLIT_NO_GAMMA */

/* SIMD versions. */

#if !defined(DBC_BLIT_NO_SIMD)
#if defined(DBCB_X86_OR_X64)

#if defined(__GNUC__) && !defined(DBC_BLIT_NO_GCC_ASM) && ((!defined(__SSE2__) && !defined(DBCB_PREFER_INTRINSICS)) || defined(DBC_BLIT_FORCE_GCC_ASM))
/* GCC or something, that understands GCC asm (icc, clang). */

/*
    Older versions of GCC (pre-4.9) and clang (pre-3.8) prevent you
    from including the intrinsics headers, if the corresponding instruction
    set has not been enabled. So we reimplement the neccessary intrinsics
    using GCC's vector extensions and inline assembly.
    Eh, whatever.
    The functions are prefixed to avoid name collisions if intrinsic headers
    are included separately.
*/

/*
    We do not bother with VEX-prefixed versions of the instructions, since this
    code should only compile if __SSE2__ is not defined, and so the compiler does
    not use them either.
*/

#ifdef __clang__
typedef long long dbcb_i32x4 __attribute__ ((__vector_size__ (16), __aligned__(16)));
typedef float     dbcb_f32x4 __attribute__ ((__vector_size__ (16), __aligned__(16)));
#else
typedef long long dbcb_i32x4 __attribute__ ((__vector_size__ (16), __may_alias__));
typedef float     dbcb_f32x4 __attribute__ ((__vector_size__ (16), __may_alias__));
#endif

/* Internal data types for implementing the intrinsics.  */
typedef float              dbcB_v4sf  __attribute__ ((__vector_size__ (16)));
typedef long long          dbcB_v2di  __attribute__ ((__vector_size__ (16)));
typedef unsigned long long dbcB_v2du  __attribute__ ((__vector_size__ (16)));
typedef int                dbcB_v4si  __attribute__ ((__vector_size__ (16)));
typedef unsigned int       dbcB_v4su  __attribute__ ((__vector_size__ (16)));
typedef short              dbcB_v8hi  __attribute__ ((__vector_size__ (16)));
typedef unsigned short     dbcB_v8hu  __attribute__ ((__vector_size__ (16)));
typedef char               dbcB_v16qi __attribute__ ((__vector_size__ (16)));
typedef unsigned char      dbcB_v16qu __attribute__ ((__vector_size__ (16)));

#ifdef __clang__
#define DBCB_SSE2_SPEC __attribute__((__always_inline__,__nodebug__,__target__("sse2"),unused)) static __inline__
#else
#define DBCB_SSE2_SPEC __attribute__((__gnu_inline__,__always_inline__,__artificial__,__target__("sse2"))) extern __inline
#endif

DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_add_epi16(dbcb_i32x4 __A,dbcb_i32x4 __B) {return (dbcb_i32x4)((dbcB_v8hu)__A+(dbcB_v8hu)__B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_set1_epi16(short A) {return __extension__ (dbcb_i32x4)(dbcB_v8hi){A,A,A,A,A,A,A,A};}
// Not a proper replacement.
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_slli_epi16(dbcb_i32x4 A,int B) {return (dbcb_i32x4)((dbcB_v8hu)A<<B);}
// Not a proper replacement.
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_srli_epi16(dbcb_i32x4 A,int B) {return (dbcb_i32x4)((dbcB_v8hu)A>>B);}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_min_ps(dbcb_f32x4 A,dbcb_f32x4 B) {__asm__("minps %1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_max_ps(dbcb_f32x4 A,dbcb_f32x4 B) {__asm__("maxps %1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_set1_ps(float F) {return __extension__ (dbcb_f32x4)(dbcB_v4sf){F,F,F,F};}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_cvttps_epi32(dbcb_f32x4 A) {dbcb_i32x4 ret;__asm__("cvttps2dq %1,%0":"=x"(ret):"x"(A));return ret;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_add_ps(dbcb_f32x4 A,dbcb_f32x4 B) {return (dbcb_f32x4)((dbcB_v4sf)A+(dbcB_v4sf)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_unpacklo_epi8(dbcb_i32x4 A,dbcb_i32x4 B) {__asm__("punpcklbw %1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_cvtepi32_ps(dbcb_i32x4 A) {dbcb_f32x4 ret;__asm__("cvtdq2ps %1,%0":"=x"(ret):"x"(A));return ret;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_mul_ps(dbcb_f32x4 A,dbcb_f32x4 B) {return (dbcb_f32x4)((dbcB_v4sf)A*(dbcB_v4sf)B);}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_loadu_ps(float const *P) {dbcb_f32x4 ret;__asm__("movups %1,%0":"=x"(ret):"m"(*P):"memory");return ret;}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_packus_epi16(dbcb_i32x4 A,dbcb_i32x4 B) {__asm__("packuswb %1,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement. We only call it with mask={0xB1,0xF5,0xFF}.
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_shufflelo_epi16(dbcb_i32x4 A,const int mask) {if(mask==0xB1) __asm__("pshuflw $0xB1,%0,%0":"+x"(A)); else if(mask==0xF5) __asm__("pshuflw $0xF5,%0,%0":"+x"(A)); else __asm__("pshuflw $0xFF,%0,%0":"+x"(A)); return A;}
// Not a proper replacement. We only call it with mask=0xB1.
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_shufflehi_epi16(dbcb_i32x4 A,const int mask) {(void)mask;__asm__("pshufhw $0xB1,%0,%0":"+x"(A));return A;}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_xor_si128(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v2du)A^(dbcB_v2du)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_or_si128(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v2du)A|(dbcB_v2du)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_setr_epi16(short q0,short q1,short q2,short q3,short q4,short q5,short q6,short q7) {return __extension__ (dbcb_i32x4)(dbcB_v8hi){q0,q1,q2,q3,q4,q5,q6,q7};}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_mullo_epi16(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v8hu)A*(dbcB_v8hu)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_unpacklo_epi16(dbcb_i32x4 A,dbcb_i32x4 B) {__asm__("punpcklwd %1,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement.
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_srli_epi32(dbcb_i32x4 A,int B) {return (dbcb_i32x4)((dbcB_v4su)A>>B);}
//DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_unpackhi_epi8(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)__builtin_ia32_punpckhbw128((dbcB_v16qi)A,(dbcB_v16qi)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_unpackhi_epi8(dbcb_i32x4 A,dbcb_i32x4 B) {__asm__("punpckhbw %1,%0":"+x"(A):"x"(B));return A;}
//DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_slli_epi32(dbcb_i32x4 A,int B) {return (dbcb_i32x4)__builtin_ia32_pslldi128((dbcB_v4si)A,B);}
// Not a proper replacement.
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_slli_epi32(dbcb_i32x4 A,int B) {return (dbcb_i32x4)((dbcB_v4su)A<<B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_unpackhi_epi16(dbcb_i32x4 A,dbcb_i32x4 B) {__asm__("punpckhwd %1,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement. We only call it with mask=0xFF.
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_shuffle_ps(dbcb_f32x4 A,dbcb_f32x4 B,int const mask) {(void)mask;__asm__("shufps $0xFF,%1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_sub_ps(dbcb_f32x4 A,dbcb_f32x4 B) {return (dbcb_f32x4)((dbcB_v4sf)A-(dbcB_v4sf)B);}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_xor_ps(dbcb_f32x4 A,dbcb_f32x4 B) {__asm__("xorps %1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_and_ps(dbcb_f32x4 A,dbcb_f32x4 B) {__asm__("andps %1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_castsi128_ps(dbcb_i32x4 A) {return (dbcb_f32x4)A;}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_setr_epi32(int q0,int q1,int q2,int q3) {return __extension__ (dbcb_i32x4)(dbcB_v4si){q0,q1,q2,q3};}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_setr_ps(float Z,float Y,float X,float W) {return __extension__ (dbcb_f32x4)(dbcB_v4sf){Z,Y,X,W};}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_set1_epi8(char A) {return __extension__ (dbcb_i32x4)(dbcB_v16qi){A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A};}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_cmpeq_epi8(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v16qi)A==(dbcB_v16qi)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_and_si128(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v2du)A&(dbcB_v2du)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_andnot_si128(dbcb_i32x4 A,dbcb_i32x4 B) {__asm__("pandn %1,%0":"+x"(A):"x"(B));return A;}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_cmpeq_epi16(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v8hi)A==(dbcB_v8hi)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_cmpgt_epi16(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v8hi)A>(dbcB_v8hi)B);}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_set1_epi32(int A) {return __extension__ (dbcb_i32x4)(dbcB_v4si){A,A,A,A};}
DBCB_SSE2_SPEC dbcb_i32x4 dbcB_mm_cmpgt_epi32(dbcb_i32x4 A,dbcb_i32x4 B) {return (dbcb_i32x4)((dbcB_v4si)A>(dbcB_v4si)B);}
DBCB_SSE2_SPEC dbcb_f32x4 dbcB_mm_div_ps(dbcb_f32x4 A,dbcb_f32x4 B) {return (dbcb_f32x4)((dbcB_v4sf)A/(dbcB_v4sf)B);}

#ifndef dbcb_load128_32
DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_load128_32_le (const void *p) {dbcb_i32x4 ret;__asm__("movd   %1,%0":"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_load128_64_le (const void *p) {dbcb_i32x4 ret;__asm__("movq   %1,%0":"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_load128_128_le(const void *p) {dbcb_i32x4 ret;__asm__("movdqu %1,%0":"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_SSE2 static void dbcB_store128_32_le (dbcb_i32x4 v,void *p) {__asm__("movd   %0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
DBCB_DECL_SSE2 static void dbcB_store128_64_le (dbcb_i32x4 v,void *p) {__asm__("movq   %0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
DBCB_DECL_SSE2 static void dbcB_store128_128_le(dbcb_i32x4 v,void *p) {__asm__("movdqu %0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
#endif

#else
/* MSVC, or something, that, hopefully, understands intrinsics. */

#include <emmintrin.h>

typedef __m128i dbcb_i32x4;
typedef __m128  dbcb_f32x4;

#define dbcB_mm_add_epi16               _mm_add_epi16
#define dbcB_mm_set1_epi16              _mm_set1_epi16
#define dbcB_mm_slli_epi16              _mm_slli_epi16
#define dbcB_mm_srli_epi16              _mm_srli_epi16
#define dbcB_mm_min_ps                  _mm_min_ps
#define dbcB_mm_max_ps                  _mm_max_ps
#define dbcB_mm_set1_ps                 _mm_set1_ps
#define dbcB_mm_cvttps_epi32            _mm_cvttps_epi32
#define dbcB_mm_add_ps                  _mm_add_ps
#define dbcB_mm_unpacklo_epi8           _mm_unpacklo_epi8
#define dbcB_mm_cvtepi32_ps             _mm_cvtepi32_ps
#define dbcB_mm_mul_ps                  _mm_mul_ps
#define dbcB_mm_loadu_ps                _mm_loadu_ps
#define dbcB_mm_packus_epi16            _mm_packus_epi16
#define dbcB_mm_shufflelo_epi16         _mm_shufflelo_epi16
#define dbcB_mm_shufflehi_epi16         _mm_shufflehi_epi16
#define dbcB_mm_xor_si128               _mm_xor_si128
#define dbcB_mm_or_si128                _mm_or_si128
#define dbcB_mm_setr_epi16              _mm_setr_epi16
#define dbcB_mm_mullo_epi16             _mm_mullo_epi16
#define dbcB_mm_unpacklo_epi16          _mm_unpacklo_epi16
#define dbcB_mm_srli_epi32              _mm_srli_epi32
#define dbcB_mm_unpackhi_epi8           _mm_unpackhi_epi8
#define dbcB_mm_slli_epi32              _mm_slli_epi32
#define dbcB_mm_unpackhi_epi16          _mm_unpackhi_epi16
#define dbcB_mm_shuffle_ps              _mm_shuffle_ps
#define dbcB_mm_sub_ps                  _mm_sub_ps
#define dbcB_mm_xor_ps                  _mm_xor_ps
#define dbcB_mm_and_ps                  _mm_and_ps
#define dbcB_mm_castsi128_ps            _mm_castsi128_ps
#define dbcB_mm_setr_epi32              _mm_setr_epi32
#define dbcB_mm_setr_ps                 _mm_setr_ps
#define dbcB_mm_set1_epi8               _mm_set1_epi8
#define dbcB_mm_cmpeq_epi8              _mm_cmpeq_epi8
#define dbcB_mm_and_si128               _mm_and_si128
#define dbcB_mm_andnot_si128            _mm_andnot_si128
#define dbcB_mm_cmpeq_epi16             _mm_cmpeq_epi16
#define dbcB_mm_cmpgt_epi16             _mm_cmpgt_epi16
#define dbcB_mm_set1_epi32              _mm_set1_epi32
#define dbcB_mm_cmpgt_epi32             _mm_cmpgt_epi32
#define dbcB_mm_div_ps                  _mm_div_ps

#ifndef dbcb_load128_32
/*
    For 32-bit loads/stores float is used, to avoid dealing with strict aliasing.
    There does not seem to be cross-domain penalty for loads/stores.
*/
DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_load128_32_le (const void *p) {return _mm_castps_si128(_mm_load_ss((const float *)p));}
DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_load128_64_le (const void *p) {return _mm_loadl_epi64((dbcb_i32x4 const*)p);}
DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_load128_128_le(const void *p) {return _mm_loadu_si128((dbcb_i32x4 const*)p);}
DBCB_DECL_SSE2 static void dbcB_store128_32_le (dbcb_i32x4 v,void *p) {_mm_store_ss((float *)p,_mm_castsi128_ps(v));}
DBCB_DECL_SSE2 static void dbcB_store128_64_le (dbcb_i32x4 v,void *p) {_mm_storel_epi64((dbcb_i32x4 *)p,v);}
DBCB_DECL_SSE2 static void dbcB_store128_128_le(dbcb_i32x4 v,void *p) {_mm_storeu_si128((dbcb_i32x4 *)p,v);}
#endif

#endif /* defined(__GNUC__) && !defined(DBC_BLIT_NO_GCC_ASM) && !defined(__SSE2__) */

#ifndef dbcb_load128_32
#if defined(DBC_BLIT_DATA_BIG_ENDIAN)
/* Suboptimal, if we only need a 16-bit bswap. 8-bit bswap is a no-op. */
dbcb_i32x4 dbcB_bswap128_32(dbcb_i32x4 v)
{
    v=dbcB_mm_shufflelo_epi16(dbcB_mm_shufflehi_epi16(v,0xB1),0xB1);
    v=dbcB_mm_xor_si128(dbcB_mm_slli_epi16(v,8),dbcB_mm_srli_epi16(v,8));
    return v;
}

DBCB_DECL_SSE2 static dbcb_i32x4 dbcb_load128_32 (const void *p) {return dbcB_bswap128_32(dbcB_load128_32_le (p));}
DBCB_DECL_SSE2 static dbcb_i32x4 dbcb_load128_64 (const void *p) {return dbcB_bswap128_32(dbcB_load128_64_le (p));}
DBCB_DECL_SSE2 static dbcb_i32x4 dbcb_load128_128(const void *p) {return dbcB_bswap128_32(dbcB_load128_128_le(p));}
DBCB_DECL_SSE2 static void dbcb_store128_32 (dbcb_i32x4 v,void *p) {dbcB_store128_32_le (dbcB_bswap128_32(v),p);}
DBCB_DECL_SSE2 static void dbcb_store128_64 (dbcb_i32x4 v,void *p) {dbcB_store128_64_le (dbcB_bswap128_32(v),p);}
DBCB_DECL_SSE2 static void dbcb_store128_128(dbcb_i32x4 v,void *p) {dbcB_store128_128_le(dbcB_bswap128_32(v),p);}
#else
#define dbcb_load128_32    dbcB_load128_32_le
#define dbcb_load128_64    dbcB_load128_64_le
#define dbcb_load128_128   dbcB_load128_128_le
#define dbcb_store128_32   dbcB_store128_32_le
#define dbcb_store128_64   dbcB_store128_64_le
#define dbcb_store128_128  dbcB_store128_128_le
#endif
#endif /* dbcb_load128_32 */

DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_div255_round_128(dbcb_i32x4 n)
{
    n=dbcB_mm_add_epi16(n,dbcB_mm_set1_epi16(128));
    return dbcB_mm_srli_epi16(dbcB_mm_add_epi16(n,dbcB_mm_srli_epi16(n,8)),8);
}

DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_float2byte_clamp_128(dbcb_f32x4 x)
{
    x=dbcB_mm_min_ps(dbcB_mm_max_ps(x,dbcB_mm_set1_ps(0.0f)),dbcB_mm_set1_ps(255.0f));
    return dbcB_mm_cvttps_epi32(dbcB_mm_add_ps(x,dbcB_mm_set1_ps(0.5f)));
}

/* Here #define seems preferable over functions. */

#define dbcB_setup128_32_sdac(ac)\
    s=dbcb_load128_32(src);                                     \
    d=dbcb_load128_32(dst);                                     \
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));           \
    d=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));           \
    if(ac) a=dbcB_mm_shufflelo_epi16(s,0xFF);                   \
    if(ac) c=dbcB_mm_xor_si128(a,dbcB_mm_set1_epi16(255));

#define dbcB_setup128_64_sdac(ac)\
    s=dbcb_load128_64(src);                                     \
    d=dbcb_load128_64(dst);                                     \
    if(ac) a=dbcB_mm_shufflelo_epi16(s,0xF5);                   \
    if(ac) a=dbcB_mm_srli_epi16(a,8);                           \
    if(ac) a=dbcB_mm_unpacklo_epi16(a,a);                       \
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));           \
    d=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));           \
    if(ac) c=dbcB_mm_xor_si128(a,dbcB_mm_set1_epi16(255));

#define dbcB_setup128_128_sdac(ac)\
    s=dbcb_load128_128(src);                                    \
    d=dbcb_load128_128(dst);                                    \
    if(ac) a=dbcB_mm_srli_epi32(s,24);                          \
    sl=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));          \
    dl=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));          \
    sh=dbcB_mm_unpackhi_epi8(s,dbcB_mm_set1_epi16(0));          \
    dh=dbcB_mm_unpackhi_epi8(d,dbcB_mm_set1_epi16(0));          \
    if(ac) a=dbcB_mm_xor_si128(a,dbcB_mm_slli_epi32(a,16));     \
    if(ac) al=dbcB_mm_unpacklo_epi16(a,a);                      \
    if(ac) ah=dbcB_mm_unpackhi_epi16(a,a);                      \
    if(ac) cl=dbcB_mm_xor_si128(al,dbcB_mm_set1_epi16(255));    \
    if(ac) ch=dbcB_mm_xor_si128(ah,dbcB_mm_set1_epi16(255));

#define dbcB_setup128_32_SDAC(ac)\
    s=dbcb_load128_32(src);                                     \
    d=dbcb_load128_32(dst);                                     \
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));           \
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));           \
    d=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));           \
    d=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));           \
    S=dbcB_mm_cvtepi32_ps(s);                                   \
    D=dbcB_mm_cvtepi32_ps(d);                                   \
    S=dbcB_mm_mul_ps(S,dbcB_mm_loadu_ps(color));                \
    A=dbcB_mm_shuffle_ps(S,S,0xFF);                             \
    C=dbcB_mm_sub_ps(dbcB_mm_set1_ps(255.0f),A);

#define dbcB_step128_bla(s,d,a,c,ret)\
    a=dbcB_mm_or_si128(a,dbcB_mm_setr_epi16(0,0,0,255,0,0,0,255));            \
    ret=dbcB_mm_add_epi16(dbcB_mm_mullo_epi16(s,a),dbcB_mm_mullo_epi16(d,c)); \
    ret=dbcB_div255_round_128(ret);

#define dbcB_step128_blp(s,d,c,ret)\
    ret=dbcB_mm_mullo_epi16(d,c);                               \
    ret=dbcB_div255_round_128(ret);                             \
    ret=dbcB_mm_add_epi16(ret,s);

#define dbcB_step128_blx(s,d,ret)\
    ret=dbcB_mm_mullo_epi16(s,d);                               \
    ret=dbcB_div255_round_128(ret);

/* Copies single pixel, with modulation. */
DBCB_DECL_SSE2 static void dbcB_b32m_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x4 s,ret;
    dbcb_f32x4 S;
    s=dbcb_load128_32(src);
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));
    S=dbcB_mm_cvtepi32_ps(s);
    S=dbcB_mm_mul_ps(S,dbcB_mm_loadu_ps(color));
    ret=dbcB_float2byte_clamp_128(S);
    ret=dbcB_mm_packus_epi16(ret,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

/* Alpha-blends single pixel, linear. */
DBCB_DECL_SSE2 static void dbcB_bla_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,c,ret;
    dbcB_setup128_32_sdac(1);
    dbcB_step128_bla(s,d,a,c,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

/* Alpha-blends 2 pixels, linear. */
DBCB_DECL_SSE2 static void dbcB_bla_2_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,c,ret;
    dbcB_setup128_64_sdac(1);
    dbcB_step128_bla(s,d,a,c,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_64(ret,dst);
}

/* Alpha-blends 4 pixels, linear. */
DBCB_DECL_SSE2 static void dbcB_bla_4_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,sl,sh,dl,dh,al,ah,cl,ch,l,h,ret;
    dbcB_setup128_128_sdac(1);
    dbcB_step128_bla(sl,dl,al,cl,l);
    dbcB_step128_bla(sh,dh,ah,ch,h);
    ret=dbcB_mm_packus_epi16(l,h);
    dbcb_store128_128(ret,dst);
}

/* Alpha-blends single pixel, linear with modulation. */
DBCB_DECL_SSE2 static void dbcB_blam_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x4 s,d,ret;
    dbcb_f32x4 S,D,A,C;
    dbcB_setup128_32_SDAC(1);
    S=dbcB_mm_xor_ps(dbcB_mm_and_ps(S,dbcB_mm_castsi128_ps(dbcB_mm_setr_epi32(-1,-1,-1,0))),dbcB_mm_setr_ps(0.0f,0.0f,0.0f,255.0f));
    D=dbcB_mm_add_ps(dbcB_mm_mul_ps(A,S),dbcB_mm_mul_ps(C,D));
    D=dbcB_mm_mul_ps(D,dbcB_mm_set1_ps(DBCB_1div255f));
    ret=dbcB_float2byte_clamp_128(D);
    ret=dbcB_mm_packus_epi16(ret,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

/* Alpha-blends (PMA) single pixel, linear. */
DBCB_DECL_SSE2 static void dbcB_blp_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,c,ret;
    dbcB_setup128_32_sdac(1);
    dbcB_step128_blp(s,d,c,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

/* Alpha-blends (PMA) 2 pixels, linear. */
DBCB_DECL_SSE2 static void dbcB_blp_2_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,c,ret;
    dbcB_setup128_64_sdac(1);
    dbcB_step128_blp(s,d,c,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_64(ret,dst);
}

/* Alpha-blends (PMA) 4 pixels, linear. */
DBCB_DECL_SSE2 static void dbcB_blp_4_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,sl,sh,dl,dh,al,ah,cl,ch,l,h,ret;
    dbcB_setup128_128_sdac(1);
    dbcB_step128_blp(sl,dl,cl,l);
    dbcB_step128_blp(sh,dh,ch,h);
    ret=dbcB_mm_packus_epi16(l,h);
    dbcb_store128_128(ret,dst);
}

/* Alpha-blends (PMA) single pixel, linear with modulation. */
DBCB_DECL_SSE2 static void dbcB_blpm_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x4 s,d,ret;
    dbcb_f32x4 S,D,A,C;
    dbcB_setup128_32_SDAC(1);
    D=dbcB_mm_mul_ps(C,D);
    D=dbcB_mm_mul_ps(D,dbcB_mm_set1_ps(DBCB_1div255f));
    D=dbcB_mm_add_ps(D,S);
    ret=dbcB_float2byte_clamp_128(D);
    ret=dbcB_mm_packus_epi16(ret,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

/* Multiplies single pixel, linear. */
DBCB_DECL_SSE2 static void dbcB_blx_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,c,ret;
    dbcB_setup128_32_sdac(0);
    (void)a;(void)c;
    dbcB_step128_blx(s,d,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

/* Multiplies 2 pixels, linear. */
DBCB_DECL_SSE2 static void dbcB_blx_2_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,c,ret;
    dbcB_setup128_64_sdac(0);
    (void)a;(void)c;
    dbcB_step128_blx(s,d,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_64(ret,dst);
}

/* Multiplies 4 pixels, linear. */
DBCB_DECL_SSE2 static void dbcB_blx_4_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x4 s,d,a,sl,sh,dl,dh,al,ah,cl,ch,l,h,ret;
    dbcB_setup128_128_sdac(0);
    (void)a;(void)al;(void)ah;(void)cl;(void)ch;
    dbcB_step128_blx(sl,dl,l);
    dbcB_step128_blx(sh,dh,h);
    ret=dbcB_mm_packus_epi16(l,h);
    dbcb_store128_128(ret,dst);
}

/* Multiplies single pixel, linear with modulation. */
DBCB_DECL_SSE2 static void dbcB_blxm_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x4 s,d,ret;
    dbcb_f32x4 S,D,A,C;
    dbcB_setup128_32_SDAC(0);
    (void)A;(void)C;
    D=dbcB_mm_mul_ps(S,D);
    D=dbcB_mm_mul_ps(D,dbcB_mm_set1_ps(DBCB_1div255f));
    ret=dbcB_float2byte_clamp_128(D);
    ret=dbcB_mm_packus_epi16(ret,ret);
    ret=dbcB_mm_packus_epi16(ret,ret);
    dbcb_store128_32(ret,dst);
}

#undef dbcB_setup128_32_sdac
#undef dbcB_setup128_64_sdac
#undef dbcB_setup128_128_sdac
#undef dbcB_setup128_32_SDAC
#undef dbcB_step128_bla
#undef dbcB_step128_blp
#undef dbcB_step128_blx

#define DBCB_DEF_B8M_SSE2(n,suffix) \
DBCB_DECL_SSE2 static void dbcB_b8m_##n##_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint8 key)\
{\
    dbcb_i32x4 s=dbcb_load128_##suffix(src);\
    dbcb_i32x4 d=dbcb_load128_##suffix(dst);\
    dbcb_i32x4 m=dbcB_mm_set1_epi8((char)key);\
    m=dbcB_mm_cmpeq_epi8(s,m);\
    d=dbcB_mm_xor_si128(dbcB_mm_and_si128(m,d),dbcB_mm_andnot_si128(m,s));\
    dbcb_store128_##suffix(d,dst);\
}

#define DBCB_DEF_B16M_SSE2(n,suffix) \
DBCB_DECL_SSE2 static void dbcB_b16m_##n##_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint16 key)\
{\
    dbcb_i32x4 s=dbcb_load128_##suffix(src);\
    dbcb_i32x4 d=dbcb_load128_##suffix(dst);\
    dbcb_i32x4 m=dbcB_mm_set1_epi16((short)key);\
    m=dbcB_mm_cmpeq_epi16(s,m);\
    d=dbcB_mm_xor_si128(dbcB_mm_and_si128(m,d),dbcB_mm_andnot_si128(m,s));\
    dbcb_store128_##suffix(d,dst);\
}

#define DBCB_DEF_B5551_SSE2(n,suffix)\
DBCB_DECL_SSE2 static void dbcB_b5551_##n##_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)\
{\
    dbcb_i32x4 s=dbcb_load128_##suffix(src);\
    dbcb_i32x4 d=dbcb_load128_##suffix(dst);\
    dbcb_i32x4 m=dbcB_mm_cmpgt_epi16(dbcB_mm_set1_epi16(0),s);\
    d=dbcB_mm_xor_si128(dbcB_mm_and_si128(m,s),dbcB_mm_andnot_si128(m,d));\
    dbcb_store128_##suffix(d,dst);\
}

#define DBCB_DEF_B32T_SSE2(n,suffix)\
DBCB_DECL_SSE2 static void dbcB_b32t_##n##_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint8 key)\
{\
    dbcb_i32x4 s=dbcb_load128_##suffix(src);\
    dbcb_i32x4 d=dbcb_load128_##suffix(dst);\
    dbcb_i32x4 m=dbcB_mm_set1_epi32((int)((dbcb_uint32)key<<24));\
    m=dbcB_mm_cmpgt_epi32(dbcB_mm_xor_si128(dbcB_mm_set1_epi32((int)0x80000000),m),dbcB_mm_xor_si128(dbcB_mm_set1_epi32((int)0x80000000),s));\
    d=dbcB_mm_xor_si128(dbcB_mm_and_si128(m,d),dbcB_mm_andnot_si128(m,s));\
    dbcb_store128_##suffix(d,dst);\
}

#define DBCB_DEF_B32S_SSE2(n,suffix)\
DBCB_DECL_SSE2 static void dbcB_b32s_##n##_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)\
{\
    dbcb_i32x4 s=dbcb_load128_##suffix(src);\
    dbcb_i32x4 d=dbcb_load128_##suffix(dst);\
    dbcb_i32x4 m=dbcB_mm_cmpgt_epi32(dbcB_mm_set1_epi32(0),s);\
    d=dbcB_mm_xor_si128(dbcB_mm_and_si128(m,s),dbcB_mm_andnot_si128(m,d));\
    dbcb_store128_##suffix(d,dst);\
}

DBCB_DEF_B8M_SSE2( 4, 32)   /* Blits  4 8-bit pixels with colorkey. */
DBCB_DEF_B8M_SSE2( 8, 64)   /* Blits  8 8-bit pixels with colorkey. */
DBCB_DEF_B8M_SSE2(16,128)   /* Blits 16 8-bit pixels with colorkey. */

DBCB_DEF_B16M_SSE2( 2, 32)  /* Blits  2 16-bit pixels with colorkey. */
DBCB_DEF_B16M_SSE2( 4, 64)  /* Blits  4 16-bit pixels with colorkey. */
DBCB_DEF_B16M_SSE2( 8,128)  /* Blits  8 16-bit pixels with colorkey. */

DBCB_DEF_B5551_SSE2( 2, 32) /* Blits  2 16-bit (5551) pixels. */
DBCB_DEF_B5551_SSE2( 4, 64) /* Blits  4 16-bit (5551) pixels. */
DBCB_DEF_B5551_SSE2( 8,128) /* Blits  8 16-bit (5551) pixels. */

DBCB_DEF_B32T_SSE2( 2, 64)  /* Blits  2 pixels, with alpha-test. */
DBCB_DEF_B32T_SSE2( 4,128)  /* Blits  4 pixels, with alpha-test. */

DBCB_DEF_B32S_SSE2( 2, 64)   /* Blits  2 pixels, with alpha-test using threshold 128. */
DBCB_DEF_B32S_SSE2( 4,128)   /* Blits  4 pixels, with alpha-test using threshold 128. */

#undef DBCB_DEF_B8M_SSE2
#undef DBCB_DEF_B16M_SSE2
#undef DBCB_DEF_B5111_SSE2
#undef DBCB_DEF_B32T_SSE2
#undef DBCB_DEF_B32S_SSE2

#ifndef DBC_BLIT_NO_GAMMA
#ifdef DBC_BLIT_GAMMA_NO_TABLES

#define dbcB_setr_xxxy(x,y) dbcB_mm_setr_ps(x,x,x,y)

DBCB_DECL_SSE2 static dbcb_f32x4 dbcB_srgb2linear_gggl_sse2(dbcb_i32x4 x)
{
    dbcb_f32x4 y=dbcB_mm_cvtepi32_ps(x);
/* (MACRO+0) allows MACRO to be empty. */
#if   (DBC_BLIT_GAMMA_NO_TABLES + 0)==3 || (DBC_BLIT_GAMMA_NO_TABLES + 0)==2
    dbcb_f32x4 C1=dbcB_setr_xxxy(DBCB_S2L_C1,DBCB_1div255f);
    dbcb_f32x4 C2=dbcB_setr_xxxy(DBCB_S2L_C2,         0.0f);
    dbcb_f32x4 C3=dbcB_setr_xxxy(DBCB_S2L_C3,         0.0f);
    y=dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(C1,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(C2,dbcB_mm_mul_ps(y,
                       C3)))));
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==1
    dbcb_f32x4 C1=dbcB_setr_xxxy(DBCB_S2L_C1,DBCB_1div255f);
    dbcb_f32x4 C2=dbcB_setr_xxxy(DBCB_S2L_C2,         0.0f);
    dbcb_f32x4 C3=dbcB_setr_xxxy(DBCB_S2L_C3,         0.0f);
    dbcb_f32x4 C4=dbcB_setr_xxxy(DBCB_S2L_C4,         0.0f);
    dbcb_f32x4 C5=dbcB_setr_xxxy(DBCB_S2L_C5,         0.0f);
    y=dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(C1,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(C2,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(C3,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(C4,dbcB_mm_mul_ps(y,
                       C5)))))))));
#else
    dbcb_f32x4 P1=dbcB_setr_xxxy(DBCB_S2L_P1,DBCB_1div255f);
    dbcb_f32x4 P2=dbcB_setr_xxxy(DBCB_S2L_P2,         0.0f);
    dbcb_f32x4 P3=dbcB_setr_xxxy(DBCB_S2L_P3,         0.0f);
    dbcb_f32x4 P4=dbcB_setr_xxxy(DBCB_S2L_P4,         0.0f);
    dbcb_f32x4 Q0=dbcB_setr_xxxy(DBCB_S2L_Q0,         1.0f);
    dbcb_f32x4 Q1=dbcB_setr_xxxy(DBCB_S2L_Q1,         0.0f);
    dbcb_f32x4 Q2=dbcB_setr_xxxy(DBCB_S2L_Q2,         0.0f);
    dbcb_f32x4 P=dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(P1,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(P2,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(P3,dbcB_mm_mul_ps(y,
                       P4)))))));
    dbcb_f32x4 Q=dbcB_mm_add_ps(Q0,dbcB_mm_mul_ps(y,
        dbcB_mm_add_ps(Q1,dbcB_mm_mul_ps(y,
                       Q2))));
    y=dbcB_mm_div_ps(P,Q);
#endif
    return y;
}

DBCB_DECL_SSE2 static dbcb_i32x4 dbcB_linear2srgb_gggl_sse2(dbcb_f32x4 x)
{
/* (MACRO+0) allows MACRO to be empty. */
#if   (DBC_BLIT_GAMMA_NO_TABLES + 0)==3
    dbcb_f32x4 C1=dbcB_setr_xxxy(DBCB_L2S_C1,255.0f);
    dbcb_f32x4 C2=dbcB_setr_xxxy(DBCB_L2S_C2,  0.0f);
    dbcb_f32x4 C3=dbcB_setr_xxxy(DBCB_L2S_C3,  0.0f);
    x=dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(C1,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(C2,dbcB_mm_mul_ps(x,
                       C3)))));
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==2
    dbcb_f32x4 P1=dbcB_setr_xxxy(DBCB_L2S_P1,255.0f);
    dbcb_f32x4 P2=dbcB_setr_xxxy(DBCB_L2S_P2,  0.0f);
    dbcb_f32x4 Q0=dbcB_setr_xxxy(DBCB_L2S_Q0,  1.0f);
    dbcb_f32x4 Q1=dbcB_setr_xxxy(DBCB_L2S_Q1,  0.0f);
    dbcb_f32x4 Q2=dbcB_setr_xxxy(DBCB_L2S_Q2,  0.0f);
    dbcb_f32x4 P=dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(P1,dbcB_mm_mul_ps(x,
                       P2)));
    dbcb_f32x4 Q=dbcB_mm_add_ps(Q0,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(Q1,dbcB_mm_mul_ps(x,
                       Q2))));
    x=dbcB_mm_div_ps(P,Q);
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==1
    dbcb_f32x4 P1=dbcB_setr_xxxy(DBCB_L2S_P1,255.0f);
    dbcb_f32x4 P2=dbcB_setr_xxxy(DBCB_L2S_P2,  0.0f);
    dbcb_f32x4 P3=dbcB_setr_xxxy(DBCB_L2S_P3,  0.0f);
    dbcb_f32x4 Q0=dbcB_setr_xxxy(DBCB_L2S_Q0,  1.0f);
    dbcb_f32x4 Q1=dbcB_setr_xxxy(DBCB_L2S_Q1,  0.0f);
    dbcb_f32x4 Q2=dbcB_setr_xxxy(DBCB_L2S_Q2,  0.0f);
    dbcb_f32x4 P=dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(P1,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(P2,dbcB_mm_mul_ps(x,
                       P3)))));
    dbcb_f32x4 Q=dbcB_mm_add_ps(Q0,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(Q1,dbcB_mm_mul_ps(x,
                       Q2))));
    x=dbcB_mm_div_ps(P,Q);
#else
    dbcb_f32x4 P1=dbcB_setr_xxxy(DBCB_L2S_P1,255.0f);
    dbcb_f32x4 P2=dbcB_setr_xxxy(DBCB_L2S_P2,  0.0f);
    dbcb_f32x4 P3=dbcB_setr_xxxy(DBCB_L2S_P3,  0.0f);
    dbcb_f32x4 P4=dbcB_setr_xxxy(DBCB_L2S_P4,  0.0f);
    dbcb_f32x4 Q0=dbcB_setr_xxxy(DBCB_L2S_Q0,  1.0f);
    dbcb_f32x4 Q1=dbcB_setr_xxxy(DBCB_L2S_Q1,  0.0f);
    dbcb_f32x4 Q2=dbcB_setr_xxxy(DBCB_L2S_Q2,  0.0f);
    dbcb_f32x4 Q3=dbcB_setr_xxxy(DBCB_L2S_Q3,  0.0f);
    dbcb_f32x4 Q4=dbcB_setr_xxxy(DBCB_L2S_Q4,  0.0f);
    dbcb_f32x4 P=dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(P1,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(P2,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(P3,dbcB_mm_mul_ps(x,
                       P4)))))));
    dbcb_f32x4 Q=dbcB_mm_add_ps(Q0,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(Q1,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(Q2,dbcB_mm_mul_ps(x,
        dbcB_mm_add_ps(Q3,dbcB_mm_mul_ps(x,
                       Q4))))))));
    x=dbcB_mm_div_ps(P,Q);
#endif
    return dbcB_mm_cvttps_epi32(dbcB_mm_add_ps(x,dbcB_mm_set1_ps(0.5f)));
}

#undef dbcB_setr_xxxy

/*
    Note: tests for the trivial cases are to speed them up, but
    also to get the exact result, in case the srgb <-> linear
    approximations do not round-trip.
*/

#define dbcB_setup128_32_gggl(ld,ac,m)\
    s=dbcb_load128_32(src);\
    d=dbcb_load128_32(dst);\
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));\
    s=dbcB_mm_unpacklo_epi8(s,dbcB_mm_set1_epi16(0));\
    d=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));\
    d=dbcB_mm_unpacklo_epi8(d,dbcB_mm_set1_epi16(0));\
    S=dbcB_srgb2linear_gggl_sse2(s);\
    if(ld) D=dbcB_srgb2linear_gggl_sse2(d);\
    if(m)  S=dbcB_mm_mul_ps(S,dbcB_mm_loadu_ps((const float *)color));\
    if(ac) A=dbcB_mm_shuffle_ps(S,S,0xFF);\
    if(ac) C=dbcB_mm_sub_ps(dbcB_mm_set1_ps(1.0f),A);

#define dbcB_output128_32_gggl(cl)\
    if(cl) D=dbcB_mm_min_ps(dbcB_mm_max_ps(D,dbcB_mm_set1_ps(0.0f)),dbcB_mm_set1_ps(1.0f));\
    ret=dbcB_linear2srgb_gggl_sse2(D);\
    ret=dbcB_mm_packus_epi16(ret,ret);\
    ret=dbcB_mm_packus_epi16(ret,ret);\
    dbcb_store128_32(ret,dst);

/* Alpha-blends single pixel, gamma-corrected. */
DBCB_DECL_SSE2 static void dbcB_bga_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S=dbcb_load32(src);
    if(S>0x00FFFFFFu)
    {
        if(S>=0xFF000000u) dbcb_store32(S,dst);
        else
        {
            dbcb_i32x4 s,d,ret;
            dbcb_f32x4 S,D,A,C;
            void *color=0;
            (void)color;
            dbcB_setup128_32_gggl(1,1,0);
            S=dbcB_mm_xor_ps(dbcB_mm_and_ps(S,dbcB_mm_castsi128_ps(dbcB_mm_setr_epi32(-1,-1,-1,0))),dbcB_mm_setr_ps(0.0f,0.0f,0.0f,1.0f));
            D=dbcB_mm_add_ps(dbcB_mm_mul_ps(A,S),dbcB_mm_mul_ps(C,D));
            dbcB_output128_32_gggl(0);
        }
    }
}

/* Alpha-blends (PMA) single pixel, gamma-corrected. */
DBCB_DECL_SSE2 static void dbcB_bgp_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S=dbcb_load32(src);
    dbcb_uint32 D=dbcb_load32(dst);
    if(S>0u)
    {
        if(S>=0xFF000000u||D==0u) dbcb_store32(S,dst);
        else
        {
            dbcb_i32x4 s,d,ret;
            dbcb_f32x4 S,D,A,C;
            void *color=0;
            (void)color;
            dbcB_setup128_32_gggl(1,1,0);
            D=dbcB_mm_mul_ps(C,D);
            D=dbcB_mm_add_ps(D,S);
            dbcB_output128_32_gggl(1);
        }
    }
}

/* Multiplies single pixel, gamma-corrected. */
DBCB_DECL_SSE2 static void dbcB_bgx_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S=dbcb_load32(src);
    dbcb_uint32 D=dbcb_load32(dst);
    if(S<0xFFFFFFFFu)
    {
        if(S==0u) dbcb_store32(S,dst);
        else if(D==0u) dbcb_store32(D,dst);
        else
        {
            dbcb_i32x4 s,d,ret;
            dbcb_f32x4 S,D,A,C;
            void *color=0;
            (void)color;
            (void)A;(void)C;
            dbcB_setup128_32_gggl(1,0,0);
            D=dbcB_mm_mul_ps(S,D);
            dbcB_output128_32_gggl(0);
        }
    }
}

/* Copies single pixel, gamma-corrected, with modulation. */
DBCB_DECL_SSE2 static void dbcB_b32g_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x4 s,d,ret;
    dbcb_f32x4 S,D,A,C;
    (void)A;(void)C;
    dbcB_setup128_32_gggl(0,0,1);
    D=S;
    dbcB_output128_32_gggl(1);
}

/* Alpha-blends single pixel, gamma-corrected, with modulation. */
DBCB_DECL_SSE2 static void dbcB_bgam_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S=dbcb_load32(src);
    if(S>0x00FFFFFFu&&color[3]!=0.0f)
    {
        dbcb_i32x4 s,d,ret;
        dbcb_f32x4 S,D,A,C;
        dbcB_setup128_32_gggl(1,1,1);
        S=dbcB_mm_xor_ps(dbcB_mm_and_ps(S,dbcB_mm_castsi128_ps(dbcB_mm_setr_epi32(-1,-1,-1,0))),dbcB_mm_setr_ps(0.0f,0.0f,0.0f,1.0f));
        D=dbcB_mm_add_ps(dbcB_mm_mul_ps(A,S),dbcB_mm_mul_ps(C,D));
        dbcB_output128_32_gggl(1);
    }
}

/* Alpha-blends (PMA) single pixel, gamma-corrected, with modulation. */
DBCB_DECL_SSE2 static void dbcB_bgpm_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S=dbcb_load32(src);
    if(S>0u)
    {
        dbcb_i32x4 s,d,ret;
        dbcb_f32x4 S,D,A,C;
        dbcB_setup128_32_gggl(1,1,1);
        D=dbcB_mm_mul_ps(C,D);
        D=dbcB_mm_add_ps(D,S);
        dbcB_output128_32_gggl(1);
    }
}

/* Multiplies single pixel, gamma-corrected, with modulation. */
DBCB_DECL_SSE2 static void dbcB_bgxm_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S=dbcb_load32(src);
    dbcb_uint32 D=dbcb_load32(dst);
    if(S==0u) dbcb_store32(S,dst);
    else if(D==0u) dbcb_store32(D,dst);
    else
    {
        dbcb_i32x4 s,d,ret;
        dbcb_f32x4 S,D,A,C;
        (void)A;(void)C;
        dbcB_setup128_32_gggl(1,0,1);
        D=dbcB_mm_mul_ps(S,D);
        dbcB_output128_32_gggl(1);
    }
}

#undef dbcB_setup128_32_gggl
#undef dbcB_output128_32_gggl

#else
DBCB_DECL_SSE2 static void dbcB_bga_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bga_1_c(src,dst);}
DBCB_DECL_SSE2 static void dbcB_bgp_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bgp_1_c(src,dst);}
DBCB_DECL_SSE2 static void dbcB_bgx_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bgx_1_c(src,dst);}
DBCB_DECL_SSE2 static void dbcB_b32g_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_b32g_1_c(src,dst,color);}
DBCB_DECL_SSE2 static void dbcB_bgam_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgam_1_c(src,dst,color);}
DBCB_DECL_SSE2 static void dbcB_bgpm_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgpm_1_c(src,dst,color);}
DBCB_DECL_SSE2 static void dbcB_bgxm_1_sse2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgxm_1_c(src,dst,color);}
#endif /* DBC_BLIT_GAMMA_NO_TABLES */
#endif /* DBC_BLIT_NO_GAMMA */

#ifndef DBC_BLIT_NO_AVX2
/*
    Note: some functions are reimplemented, despite having SSE2 equivalent,
    possibly with somewhat worse performance. This makes AVX2 code path
    mostly self-contained, and hopefully reduces the risk of transition
    penalties and/or functions failing to inline.
*/
#if defined(__GNUC__) && !defined(DBC_BLIT_NO_GCC_ASM) && ((!defined(__AVX2__) && !defined(DBCB_PREFER_INTRINSICS)) || defined(DBC_BLIT_FORCE_GCC_ASM))
/* GCC or something, that understands GCC asm (icc, clang). */

#ifdef __clang__
typedef long long dbcb_i32x8 __attribute__ ((__vector_size__ (32), __aligned__(32)));
typedef float     dbcb_f32x8 __attribute__ ((__vector_size__ (32), __aligned__(32)));
#else
typedef long long dbcb_i32x8 __attribute__ ((__vector_size__ (32), __may_alias__));
typedef float     dbcb_f32x8 __attribute__ ((__vector_size__ (32), __may_alias__));
#endif

/* Internal data types for implementing the intrinsics.  */
typedef float              dbcB_v8sf  __attribute__ ((__vector_size__ (32)));
typedef long long          dbcB_v4di  __attribute__ ((__vector_size__ (32)));
typedef unsigned long long dbcB_v4du  __attribute__ ((__vector_size__ (32)));
typedef int                dbcB_v8si  __attribute__ ((__vector_size__ (32)));
typedef unsigned int       dbcB_v8su  __attribute__ ((__vector_size__ (32)));
typedef short              dbcB_v16hi __attribute__ ((__vector_size__ (32)));
typedef unsigned short     dbcB_v16hu __attribute__ ((__vector_size__ (32)));
typedef char               dbcB_v32qi __attribute__ ((__vector_size__ (32)));
typedef unsigned char      dbcB_v32qu __attribute__ ((__vector_size__ (32)));

#ifdef __clang__
#define DBCB_AVX2_SPEC __attribute__((__always_inline__,__nodebug__,__target__("avx2"),unused)) static __inline__
#else
#define DBCB_AVX2_SPEC __attribute__((__gnu_inline__,__always_inline__,__artificial__,__target__("avx2"))) extern __inline
#endif

#ifdef __clang__
/* More hacks for the older versions of clang, since they apparently do not believe that "x" constraint can describe ymm. */
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_add_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hu)A+(dbcB_v16hu)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_set1_epi16(short A) {return __extension__ (dbcb_i32x8)(dbcB_v16hi){A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_srli_epi16(dbcb_i32x8 A,int B) {return (dbcb_i32x8)__builtin_ia32_psrlwi256((dbcB_v16hi)A,B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_min_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)__builtin_ia32_minps256((dbcB_v8sf)A,(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_max_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)__builtin_ia32_maxps256((dbcB_v8sf)A,(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_set1_ps(float A) {return __extension__ (dbcb_f32x8){A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cvttps_epi32(dbcb_f32x8 A) {return (dbcb_i32x8)__builtin_ia32_cvttps2dq256((dbcB_v8sf)A);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_add_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A+(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpacklo_epi8(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)__builtin_shufflevector((dbcB_v32qi)A,(dbcB_v32qi)B, 0,32, 1,33, 2,34, 3,35, 4,36, 5,37, 6,38, 7,39,16,48,17,49,18,50,19,51,20,52,21,53,22,54,23,55);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_cvtepi32_ps(dbcb_i32x8 A) {return (dbcb_f32x8)__builtin_ia32_cvtdq2ps256((dbcB_v8si)A);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_mul_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A*(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_loadu_ps(float const *P) {struct loadu_ps {float v __attribute__((__vector_size__(32),__aligned__(1)));} __attribute__((__packed__, __may_alias__));return ((const struct loadu_ps*)P)->v;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_packus_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)__builtin_ia32_packuswb256((dbcB_v16hi)A,(dbcB_v16hi)B);}
// Not a proper replacement. We only call it with mask={0xF5,0xFF}.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_shufflelo_epi16(dbcb_i32x8 A,const int mask) {if(mask==0xF5) return (dbcb_i32x8)__builtin_shufflevector((dbcB_v16hi)A,(dbcB_v16hi)A, 1, 1, 3, 3, 4, 5, 6, 7, 9, 9,11,11,12,13,14,15); else return (dbcb_i32x8)__builtin_shufflevector((dbcB_v16hi)A,(dbcB_v16hi)A, 3, 3, 3, 3, 4, 5, 6, 7,11,11,11,11,12,13,14,15);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_xor_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v4du)A^(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_or_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v4du)A|(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_setr_epi16(short q00,short q01,short q02,short q03,short q04,short q05,short q06,short q07,short q08,short q09,short q10,short q11,short q12,short q13,short q14,short q15) {return __extension__ (dbcb_i32x8)(dbcB_v16hi){q00,q01,q02,q03,q04,q05,q06,q07,q08,q09,q10,q11,q12,q13,q14,q15};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_mullo_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hu)A*(dbcB_v16hu)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpacklo_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)__builtin_shufflevector((dbcB_v16hi)A,(dbcB_v16hi)B, 0,16, 1,17, 2,18, 3,19, 8,24, 9,25,10,26,11,27);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_srli_epi32(dbcb_i32x8 A,int B) {return (dbcb_i32x8)__builtin_ia32_psrldi256((dbcB_v8si)A,B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpackhi_epi8(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)__builtin_shufflevector((dbcB_v32qi)A,(dbcB_v32qi)B, 8,40, 9,41,10,42,11,43,12,44,13,45,14,46,15,47,24,56,25,57,26,58,27,59,28,60,29,61,30,62,31,63);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_slli_epi32(dbcb_i32x8 A,int B) {return (dbcb_i32x8)__builtin_ia32_pslldi256((dbcB_v8si)A,B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpackhi_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)__builtin_shufflevector((dbcB_v16hi)A,(dbcB_v16hi)B, 4,20, 5,21, 6,22, 7,23,12,28,13,29,14,30,15,31);}
// Not a proper replacement. We only call it with mask=0xFF.
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_shuffle_ps(dbcb_f32x8 A,dbcb_f32x8 B,const int mask) {(void)mask;return (dbcb_f32x8)__builtin_shufflevector((dbcB_v8sf)A,(dbcB_v8sf)B,3,3,3,3,7,7,7,7);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_sub_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A-(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_xor_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8si)A^(dbcB_v8si)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_and_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8si)A&(dbcB_v8si)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_castsi256_ps(dbcb_i32x8 A) {return (dbcb_f32x8)A;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_castps_si256(dbcb_f32x8 A) {return (dbcb_i32x8)A;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_setr_epi32(int A,int B,int C,int D,int E,int F,int G,int H) {return __extension__ (dbcb_i32x8)(dbcB_v8si){A,B,C,D,E,F,G,H};}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_setr_ps (float A,float B,float C,float D,float E,float F,float G,float H) {return __extension__ (dbcb_f32x8){A,B,C,D,E,F,G,H};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_set1_epi8(char A) {return __extension__ (dbcb_i32x8)(dbcB_v32qi){A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpeq_epi8(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v32qi)A==(dbcB_v32qi)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_and_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v4du)A&(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_andnot_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)(~(dbcB_v4du)A&(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpeq_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hi)A==(dbcB_v16hi)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpgt_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hi)A>(dbcB_v16hi)B);}
// Not a proper replacement. We only call it with mask=0xD8.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_permute4x64_epi64(dbcb_i32x8 X,const int M) {(void)M;return (dbcb_i32x8)__builtin_shufflevector((dbcB_v8si)X,(dbcB_v8si)X,0,1,4,5,2,3,6,7);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_set1_epi32(int A) {return __extension__ (dbcb_i32x8)(dbcB_v8si){A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpgt_epi32(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8 )((dbcB_v8si)A>(dbcB_v8si)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_blendv_epi8 (dbcb_i32x8 X, dbcb_i32x8 Y, dbcb_i32x8 M) {(void)M;return (dbcb_i32x8)__builtin_ia32_pblendvb256((dbcB_v32qi)X,(dbcB_v32qi)Y,(dbcB_v32qi)M);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_div_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A/(dbcB_v8sf)B);}
// Not a proper replacement. We only call it with mask=0x77.
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_blend_ps(dbcb_f32x8 X,dbcb_f32x8 Y,int M) {(void)M;return __extension__ (dbcb_f32x8)__builtin_ia32_blendvps256((dbcB_v8sf)X,(dbcB_v8sf)Y,(dbcB_v8sf){-0.0f,-0.0f,-0.0f,0.0f,-0.0f,-0.0f,-0.0f,0.0f});}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_shuffle_epi8(dbcb_i32x8 A,dbcb_i32x8 M) {return (dbcb_i32x8)__builtin_ia32_pshufb256((dbcB_v32qi)A,(dbcB_v32qi)M);}

#ifndef dbcb_load256_32
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_32_le (const void *p) {return __extension__ (dbcb_i32x8)(dbcB_v8si){*(const int *)p,0,0,0,0,0,0,0};}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_64_le (const void *p) {typedef int dbcb_simd2i __attribute__ ((__vector_size__  (8), __may_alias__)); return __extension__ (dbcb_i32x8)(dbcB_v4di){(long long)(*(const dbcb_simd2i *)p),0LL,0LL,0LL};}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_128_le(const void *p) {typedef long long dbcB_v2di __attribute__((__vector_size__ (16)));dbcB_v2di ret=__builtin_ia32_lddqu((char const *)p);return __extension__ (dbcb_i32x8){ret[0],ret[1],0LL,0LL};}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_256_le(const void *p) {return (dbcb_i32x8)__builtin_ia32_lddqu256((char const *)p);}
DBCB_DECL_AVX2 static void dbcB_store256_32_le (dbcb_i32x8 v,void *p) {*(int *)p=((dbcB_v8si)v)[0];}
DBCB_DECL_AVX2 static void dbcB_store256_64_le (dbcb_i32x8 v,void *p) {*(long long *)p=((dbcB_v4di)v)[0];}
DBCB_DECL_AVX2 static void dbcB_store256_128_le(dbcb_i32x8 v,void *p) {typedef long long dbcB_v2di __attribute__((__vector_size__(16),__aligned__(1),__may_alias__));struct storeu_si128{long long v __attribute__((__vector_size__(16),__aligned__(1)));} __attribute__((__packed__,__may_alias__));    dbcB_v2di a=__extension__ (dbcB_v2di){v[0],v[1]};((struct storeu_si128*)p)->v=a;}
DBCB_DECL_AVX2 static void dbcB_store256_256_le(dbcb_i32x8 v,void *p) {struct storeu_si256{long long v __attribute__((__vector_size__(32),__aligned__(1)));} __attribute__((__packed__,__may_alias__));((struct storeu_si256*)p)->v=v;}
#endif

DBCB_DECL_AVX2 static dbcb_f32x8 dbcB_broadcast256_128f(const void *p) {typedef float dbcB_v4sf __attribute__ ((__vector_size__ (16)));return __builtin_ia32_vbroadcastf128_ps256((const dbcB_v4sf*)p);}
#else
/* GCC version. */
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_add_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hu)A+(dbcB_v16hu)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_set1_epi16(short A) {return __extension__ (dbcb_i32x8)(dbcB_v16hi){A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A};}
// Not a proper replacement.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_srli_epi16(dbcb_i32x8 A,int B) {return (dbcb_i32x8)((dbcB_v16hu)A>>B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_min_ps(dbcb_f32x8 A,dbcb_f32x8 B) {__asm__("vminps %1,%0,%0":"+x"(A):"x"(B));return A;}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_max_ps(dbcb_f32x8 A,dbcb_f32x8 B) {__asm__("vmaxps %1,%0,%0":"+x"(A):"x"(B));return A;}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_set1_ps(float A) {return __extension__ (dbcb_f32x8){A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cvttps_epi32(dbcb_f32x8 A) {return (dbcb_i32x8)__builtin_ia32_cvttps2dq256((dbcB_v8sf)A);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_add_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A+(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpacklo_epi8(dbcb_i32x8 A,dbcb_i32x8 B) {__asm__("vpunpcklbw %1,%0,%0":"+x"(A):"x"(B));return A;}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_cvtepi32_ps(dbcb_i32x8 A) {dbcb_f32x8 ret;__asm__("vcvtdq2ps %1,%0":"=x"(ret):"x"(A));return ret;}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_mul_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A*(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_loadu_ps(float const *P) {dbcb_f32x8 ret;__asm__("vmovups %1,%0":"=x"(ret):"m"(*P):"memory");return ret;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_packus_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {__asm__("vpackuswb %1,%0,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement. We only call it with mask={0xF5,0xFF}.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_shufflelo_epi16(dbcb_i32x8 A,const int mask) {if(mask==0xF5) __asm__("vpshuflw $0xF5,%0,%0":"+x"(A)); else __asm__("vpshuflw $0xFF,%0,%0":"+x"(A)); return A;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_xor_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v4du)A^(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_or_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v4du)A|(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_setr_epi16(short q00,short q01,short q02,short q03,short q04,short q05,short q06,short q07,short q08,short q09,short q10,short q11,short q12,short q13,short q14,short q15) {return __extension__ (dbcb_i32x8)(dbcB_v16hi){q00,q01,q02,q03,q04,q05,q06,q07,q08,q09,q10,q11,q12,q13,q14,q15};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_mullo_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hu)A*(dbcB_v16hu)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpacklo_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {__asm__("vpunpcklwd %1,%0,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_srli_epi32(dbcb_i32x8 A,int B) {return (dbcb_i32x8)((dbcB_v8su)A>>B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpackhi_epi8(dbcb_i32x8 A,dbcb_i32x8 B) {__asm__("vpunpckhbw %1,%0,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_slli_epi32(dbcb_i32x8 A,int B) {return (dbcb_i32x8)((dbcB_v8su)A<<B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_unpackhi_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {__asm__("vpunpckhwd %1,%0,%0":"+x"(A):"x"(B));return A;}
// Not a proper replacement. We only call it with mask=0xFF.
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_shuffle_ps(dbcb_f32x8 A,dbcb_f32x8 B,const int mask) {(void)mask;__asm__("vshufps $0xFF,%1,%0,%0":"+x"(A):"x"(B));return A;}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_sub_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A-(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_xor_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)__builtin_ia32_xorps256((dbcB_v8sf)A,(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_and_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)__builtin_ia32_andps256((dbcB_v8sf)A,(dbcB_v8sf)B);}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_castsi256_ps(dbcb_i32x8 A) {return (dbcb_f32x8)A;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_castps_si256(dbcb_f32x8 A) {return (dbcb_i32x8)A;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_setr_epi32(int A,int B,int C,int D,int E,int F,int G,int H) {return __extension__ (dbcb_i32x8)(dbcB_v8si){A,B,C,D,E,F,G,H};}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_setr_ps (float A,float B,float C,float D,float E,float F,float G,float H) {return __extension__ (dbcb_f32x8){A,B,C,D,E,F,G,H};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_set1_epi8(char A) {return __extension__ (dbcb_i32x8)(dbcB_v32qi){A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpeq_epi8(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v32qi)A==(dbcB_v32qi)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_and_si256(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v4du)A&(dbcB_v4du)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_andnot_si256(dbcb_i32x8 A,dbcb_i32x8 B) {__asm__("vpandnot %1,%0,%0":"+x"(A):"x"(B));return A;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpeq_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hi)A==(dbcB_v16hi)B);}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpgt_epi16(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8)((dbcB_v16hi)A>(dbcB_v16hi)B);}
// Not a proper replacement. We only call it with mask=0xD8.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_permute4x64_epi64(dbcb_i32x8 X,const int M) {dbcb_i32x8 m=__extension__ (dbcb_i32x8)(dbcB_v8si){0,1,4,5,2,3,6,7};(void)M;__asm__("vpermd %0,%1,%0":"+x"(X):"x"(m));return X;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_set1_epi32(int A) {return __extension__ (dbcb_i32x8)(dbcB_v8si){A,A,A,A,A,A,A,A};}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_cmpgt_epi32(dbcb_i32x8 A,dbcb_i32x8 B) {return (dbcb_i32x8 )((dbcB_v8si)A>(dbcB_v8si)B);}
//DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_blendv_epi8(dbcb_i32x8 X,dbcb_i32x8 Y,dbcb_i32x8 M) {return (dbcb_i32x8)__builtin_ia32_pblendvb256((dbcB_v32qi)X,(dbcB_v32qi)Y,(dbcB_v32qi)M);}
// AT&T argument order.
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_blendv_epi8(dbcb_i32x8 X,dbcb_i32x8 Y,dbcb_i32x8 M) {__asm__("vpblendvb %2,%1,%0,%0":"+x"(X):"x"(Y),"x"(M));return X;}
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_div_ps(dbcb_f32x8 A,dbcb_f32x8 B) {return (dbcb_f32x8)((dbcB_v8sf)A/(dbcB_v8sf)B);}
// Not a proper replacement. We only call it with mask=0x77.
DBCB_AVX2_SPEC dbcb_f32x8 dbcB_mm256_blend_ps(dbcb_f32x8 X,dbcb_f32x8 Y,int M) {__asm__("vblendps $0x77,%1,%0,%0":"+x"(X):"x"(Y),"x"(M));return X;}
DBCB_AVX2_SPEC dbcb_i32x8 dbcB_mm256_shuffle_epi8(dbcb_i32x8 A,dbcb_i32x8 M) {__asm__("vpshufb %1,%0,%0":"+x"(A):"x"(M));return A;}

#ifndef dbcb_load256_32
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_32_le (const void *p) {dbcb_i32x8 ret;__asm__("vmovd   %1,%x0"  :"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_64_le (const void *p) {dbcb_i32x8 ret;__asm__("vmovq   %1,%x0"  :"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_128_le(const void *p) {dbcb_i32x8 ret;__asm__("vmovdqu %1,%x0":"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_256_le(const void *p) {dbcb_i32x8 ret;__asm__("vmovdqu %1, %0":"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
DBCB_DECL_AVX2 static void dbcB_store256_32_le (dbcb_i32x8 v,void *p) {__asm__("vmovd   %x0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
DBCB_DECL_AVX2 static void dbcB_store256_64_le (dbcb_i32x8 v,void *p) {__asm__("vmovq   %x0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
DBCB_DECL_AVX2 static void dbcB_store256_128_le(dbcb_i32x8 v,void *p) {__asm__("vmovdqu %x0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
DBCB_DECL_AVX2 static void dbcB_store256_256_le(dbcb_i32x8 v,void *p) {__asm__("vmovdqu  %0,%1": :"x"(v),"m"(*(const unsigned char*)p):"memory");}
#endif

DBCB_DECL_AVX2 static dbcb_f32x8 dbcB_broadcast256_128f(const void *p) {dbcb_f32x8 ret;__asm__("vbroadcastf128 %1,%0":"=x"(ret):"m"(*(const unsigned char*)p):"memory");return ret;}
#endif /* __clang__ */

/* No need for manual zeroupper, GCC/clang insert it on their own. */
#define DBCB_ZEROUPPER()

#else
/* MSVC, or something, that, hopefully, understands intrinsics. */

#include <immintrin.h>

typedef __m256i dbcb_i32x8;
typedef __m256  dbcb_f32x8;

#define dbcB_mm256_add_epi16            _mm256_add_epi16
#define dbcB_mm256_set1_epi16           _mm256_set1_epi16
#define dbcB_mm256_srli_epi16           _mm256_srli_epi16
#define dbcB_mm256_min_ps               _mm256_min_ps
#define dbcB_mm256_max_ps               _mm256_max_ps
#define dbcB_mm256_set1_ps              _mm256_set1_ps
#define dbcB_mm256_cvttps_epi32         _mm256_cvttps_epi32
#define dbcB_mm256_add_ps               _mm256_add_ps
#define dbcB_mm256_unpacklo_epi8        _mm256_unpacklo_epi8
#define dbcB_mm256_cvtepi32_ps          _mm256_cvtepi32_ps
#define dbcB_mm256_mul_ps               _mm256_mul_ps
#define dbcB_mm256_loadu_ps             _mm256_loadu_ps
#define dbcB_mm256_packus_epi16         _mm256_packus_epi16
#define dbcB_mm256_shufflelo_epi16      _mm256_shufflelo_epi16
#define dbcB_mm256_xor_si256            _mm256_xor_si256
#define dbcB_mm256_or_si256             _mm256_or_si256
#define dbcB_mm256_setr_epi16           _mm256_setr_epi16
#define dbcB_mm256_mullo_epi16          _mm256_mullo_epi16
#define dbcB_mm256_unpacklo_epi16       _mm256_unpacklo_epi16
#define dbcB_mm256_srli_epi32           _mm256_srli_epi32
#define dbcB_mm256_unpackhi_epi8        _mm256_unpackhi_epi8
#define dbcB_mm256_slli_epi32           _mm256_slli_epi32
#define dbcB_mm256_unpackhi_epi16       _mm256_unpackhi_epi16
#define dbcB_mm256_shuffle_ps           _mm256_shuffle_ps
#define dbcB_mm256_sub_ps               _mm256_sub_ps
#define dbcB_mm256_xor_ps               _mm256_xor_ps
#define dbcB_mm256_and_ps               _mm256_and_ps
#define dbcB_mm256_castsi256_ps         _mm256_castsi256_ps
#define dbcB_mm256_castps_si256         _mm256_castps_si256
#define dbcB_mm256_setr_epi32           _mm256_setr_epi32
#define dbcB_mm256_setr_ps              _mm256_setr_ps
#define dbcB_mm256_set1_epi8            _mm256_set1_epi8
#define dbcB_mm256_cmpeq_epi8           _mm256_cmpeq_epi8
#define dbcB_mm256_and_si256            _mm256_and_si256
#define dbcB_mm256_andnot_si256         _mm256_andnot_si256
#define dbcB_mm256_cmpeq_epi16          _mm256_cmpeq_epi16
#define dbcB_mm256_cmpgt_epi16          _mm256_cmpgt_epi16
#define dbcB_mm256_permute4x64_epi64    _mm256_permute4x64_epi64
#define dbcB_mm256_set1_epi32           _mm256_set1_epi32
#define dbcB_mm256_cmpgt_epi32          _mm256_cmpgt_epi32
#define dbcB_mm256_blendv_epi8          _mm256_blendv_epi8
#define dbcB_mm256_div_ps               _mm256_div_ps
#define dbcB_mm256_blend_ps             _mm256_blend_ps
#define dbcB_mm256_shuffle_epi8         _mm256_shuffle_epi8

#ifndef dbcb_load256_32
/*
    For 32-bit loads/stores float is used, to avoid dealing with strict aliasing.
    There does not seem to be cross-domain penalty for loads/stores.
*/
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_32_le (const void *p) {return _mm256_castsi128_si256(_mm_castps_si128(_mm_load_ss((const float *)p)));}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_64_le (const void *p) {return _mm256_castsi128_si256(_mm_loadl_epi64((__m128i const*)p));}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_128_le(const void *p) {return _mm256_castsi128_si256(_mm_loadu_si128((__m128i const*)p));}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_load256_256_le(const void *p) {return _mm256_loadu_si256((dbcb_i32x8 const*)p);}
DBCB_DECL_AVX2 static void dbcB_store256_32_le (dbcb_i32x8 v,void *p) {_mm_store_ss((float *)p,_mm_castsi128_ps(_mm256_castsi256_si128(v)));}
DBCB_DECL_AVX2 static void dbcB_store256_64_le (dbcb_i32x8 v,void *p) {_mm_storel_epi64((__m128i *)p,_mm256_castsi256_si128(v));}
DBCB_DECL_AVX2 static void dbcB_store256_128_le(dbcb_i32x8 v,void *p) {_mm_storeu_si128((__m128i *)p,_mm256_castsi256_si128(v));}
DBCB_DECL_AVX2 static void dbcB_store256_256_le(dbcb_i32x8 v,void *p) {_mm256_storeu_si256((dbcb_i32x8 *)p,v);}
#endif

DBCB_DECL_AVX2 static dbcb_f32x8 dbcB_broadcast256_128f(const void *p) {return _mm256_broadcast_ps((const __m128*)p);}

#if defined(__GNUC__) || (_MSC_VER>=1900)
/* No need for manual zeroupper, GCC/clang and newer MSVC insert it on their own. */
#define DBCB_ZEROUPPER()
#else
#define DBCB_ZEROUPPER() _mm256_zeroupper()
#endif

#endif /* defined(__GNUC__) && !defined(DBC_BLIT_NO_GCC_ASM) && !defined(__AVX2__) */

#ifndef dbcb_load256_32
#if defined(DBC_BLIT_DATA_BIG_ENDIAN)
/* Suboptimal, if we only need a 16-bit bswap. 8-bit bswap is a no-op. */
DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_bswap256_32(dbcb_i32x8 v)
{
    return dbcB_mm256_shuffle_epi8(v,dbcB_mm256_setr_epi32(0x00010203,0x04050607,0x08090A0B,0x0C0D0E0F,0x00010203,0x04050607,0x08090A0B,0x0C0D0E0F));
}

DBCB_DECL_AVX2 static dbcb_i32x8 dbcb_load256_32 (const void *p) {return dbcB_bswap256_32(dbcB_load256_32_le (p));}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcb_load256_64 (const void *p) {return dbcB_bswap256_32(dbcB_load256_64_le (p));}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcb_load256_128(const void *p) {return dbcB_bswap256_32(dbcB_load256_128_le(p));}
DBCB_DECL_AVX2 static dbcb_i32x8 dbcb_load256_256(const void *p) {return dbcB_bswap256_32(dbcB_load256_256_le(p));}
DBCB_DECL_AVX2 static void dbcb_store256_32 (dbcb_i32x8 v,void *p) {dbcB_store256_32_le (dbcB_bswap256_32(v),p);}
DBCB_DECL_AVX2 static void dbcb_store256_64 (dbcb_i32x8 v,void *p) {dbcB_store256_64_le (dbcB_bswap256_32(v),p);}
DBCB_DECL_AVX2 static void dbcb_store256_128(dbcb_i32x8 v,void *p) {dbcB_store256_128_le(dbcB_bswap256_32(v),p);}
DBCB_DECL_AVX2 static void dbcb_store256_256(dbcb_i32x8 v,void *p) {dbcB_store256_256_le(dbcB_bswap256_32(v),p);}
#else
#define dbcb_load256_32    dbcB_load256_32_le
#define dbcb_load256_64    dbcB_load256_64_le
#define dbcb_load256_128   dbcB_load256_128_le
#define dbcb_load256_256   dbcB_load256_256_le
#define dbcb_store256_32   dbcB_store256_32_le
#define dbcb_store256_64   dbcB_store256_64_le
#define dbcb_store256_128  dbcB_store256_128_le
#define dbcb_store256_256  dbcB_store256_256_le
#endif
#endif /* dbcb_load256_32 */

DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_div255_round_256(dbcb_i32x8 n)
{
    n=dbcB_mm256_add_epi16(n,dbcB_mm256_set1_epi16(128));
    return dbcB_mm256_srli_epi16(dbcB_mm256_add_epi16(n,dbcB_mm256_srli_epi16(n,8)),8);
}

DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_float2byte_clamp_256(dbcb_f32x8 x)
{
    x=dbcB_mm256_min_ps(dbcB_mm256_max_ps(x,dbcB_mm256_set1_ps(0.0f)),dbcB_mm256_set1_ps(255.0f));
    return dbcB_mm256_cvttps_epi32(dbcB_mm256_add_ps(x,dbcB_mm256_set1_ps(0.5f)));
}

/* Here #define seems preferable over functions. */

#define dbcB_setup256_32_sdac(ac)\
    s=dbcb_load256_32(src);                                        \
    d=dbcb_load256_32(dst);                                        \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    if(ac) a=dbcB_mm256_shufflelo_epi16(s,0xFF);                   \
    if(ac) c=dbcB_mm256_xor_si256(a,dbcB_mm256_set1_epi16(255));

#define dbcB_setup256_64_sdac(ac)\
    s=dbcb_load256_64(src);                                        \
    d=dbcb_load256_64(dst);                                        \
    if(ac) a=dbcB_mm256_shufflelo_epi16(s,0xF5);                   \
    if(ac) a=dbcB_mm256_srli_epi16(a,8);                           \
    if(ac) a=dbcB_mm256_unpacklo_epi16(a,a);                       \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    if(ac) c=dbcB_mm256_xor_si256(a,dbcB_mm256_set1_epi16(255));

#define dbcB_setup256_128_sdac(ac)\
    s=dbcb_load256_128(src);                                       \
    d=dbcb_load256_128(dst);                                       \
    s=dbcB_mm256_permute4x64_epi64(s,0xD8);                        \
    d=dbcB_mm256_permute4x64_epi64(d,0xD8);                        \
    if(ac) a=dbcB_mm256_shufflelo_epi16(s,0xF5);                   \
    if(ac) a=dbcB_mm256_srli_epi16(a,8);                           \
    if(ac) a=dbcB_mm256_unpacklo_epi16(a,a);                       \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    if(ac) c=dbcB_mm256_xor_si256(a,dbcB_mm256_set1_epi16(255));

#define dbcB_setup256_256_sdac(ac)\
    s=dbcb_load256_256(src);                                       \
    d=dbcb_load256_256(dst);                                       \
    if(ac) a=dbcB_mm256_srli_epi32(s,24);                          \
    sl=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));       \
    dl=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));       \
    sh=dbcB_mm256_unpackhi_epi8(s,dbcB_mm256_set1_epi16(0));       \
    dh=dbcB_mm256_unpackhi_epi8(d,dbcB_mm256_set1_epi16(0));       \
    if(ac) a=dbcB_mm256_xor_si256(a,dbcB_mm256_slli_epi32(a,16));  \
    if(ac) al=dbcB_mm256_unpacklo_epi16(a,a);                      \
    if(ac) ah=dbcB_mm256_unpackhi_epi16(a,a);                      \
    if(ac) cl=dbcB_mm256_xor_si256(al,dbcB_mm256_set1_epi16(255)); \
    if(ac) ch=dbcB_mm256_xor_si256(ah,dbcB_mm256_set1_epi16(255));

#define dbcB_setup256_32_SDAC(ac)\
    s=dbcb_load256_32(src);                                        \
    d=dbcb_load256_32(dst);                                        \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    S=dbcB_mm256_cvtepi32_ps(s);                                   \
    D=dbcB_mm256_cvtepi32_ps(d);                                   \
    S=dbcB_mm256_mul_ps(S,dbcB_broadcast256_128f(color));          \
    if(ac) A=dbcB_mm256_shuffle_ps(S,S,0xFF);                      \
    if(ac) C=dbcB_mm256_sub_ps(dbcB_mm256_set1_ps(255.0f),A);

#define dbcB_setup256_64_SDAC(ac)\
    s=dbcb_load256_64(src);                                        \
    d=dbcb_load256_64(dst);                                        \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    s=dbcB_mm256_permute4x64_epi64(s,0xD8);                        \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    d=dbcB_mm256_permute4x64_epi64(d,0xD8);                        \
    d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));        \
    S=dbcB_mm256_cvtepi32_ps(s);                                   \
    D=dbcB_mm256_cvtepi32_ps(d);                                   \
    S=dbcB_mm256_mul_ps(S,dbcB_broadcast256_128f(color));          \
    if(ac) A=dbcB_mm256_shuffle_ps(S,S,0xFF);                      \
    if(ac) C=dbcB_mm256_sub_ps(dbcB_mm256_set1_ps(255.0f),A);

#define dbcB_step256_bla(s,d,a,c,ret)\
    a=dbcB_mm256_or_si256(a,dbcB_mm256_setr_epi16(0,0,0,255,0,0,0,255,0,0,0,255,0,0,0,255)); \
    ret=dbcB_mm256_add_epi16(dbcB_mm256_mullo_epi16(s,a),dbcB_mm256_mullo_epi16(d,c));       \
    ret=dbcB_div255_round_256(ret);

#define dbcB_step256_blp(s,d,c,ret)\
    ret=dbcB_mm256_mullo_epi16(d,c);                               \
    ret=dbcB_div255_round_256(ret);                                \
    ret=dbcB_mm256_add_epi16(ret,s);

#define dbcB_step256_blx(s,d,ret)\
    ret=dbcB_mm256_mullo_epi16(s,d);                               \
    ret=dbcB_div255_round_256(ret);

#define dbcB_step256_blam(S,D,A,C,ret)\
    S=dbcB_mm256_blend_ps(dbcB_mm256_set1_ps(255.0f),S,0x77);\
    D=dbcB_mm256_add_ps(dbcB_mm256_mul_ps(A,S),dbcB_mm256_mul_ps(C,D));\
    D=dbcB_mm256_mul_ps(D,dbcB_mm256_set1_ps(DBCB_1div255f));\
    ret=dbcB_float2byte_clamp_256(D);

#define dbcB_step256_blpm(S,D,C,ret)\
    D=dbcB_mm256_mul_ps(C,D);                                      \
    D=dbcB_mm256_mul_ps(D,dbcB_mm256_set1_ps(DBCB_1div255f));      \
    D=dbcB_mm256_add_ps(D,S);                                      \
    ret=dbcB_float2byte_clamp_256(D);

#define dbcB_step256_blxm(S,D,ret)\
    D=dbcB_mm256_mul_ps(D,S);                                      \
    D=dbcB_mm256_mul_ps(D,dbcB_mm256_set1_ps(DBCB_1div255f));      \
    ret=dbcB_float2byte_clamp_256(D);

/* Copies single pixel, with modulation. */
DBCB_DECL_AVX2 static void dbcB_b32m_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,ret;
    dbcb_f32x8 S;
    s=dbcb_load256_32(src);
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));
    S=dbcB_mm256_cvtepi32_ps(s);
    S=dbcB_mm256_mul_ps(S,dbcB_broadcast256_128f(color));
    ret=dbcB_float2byte_clamp_256(S);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Copies 2 pixels, with modulation. */
DBCB_DECL_AVX2 static void dbcB_b32m_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,ret;
    dbcb_f32x8 S;
    s=dbcb_load256_64(src);
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));
    s=dbcB_mm256_permute4x64_epi64(s,0xD8);
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));
    S=dbcB_mm256_cvtepi32_ps(s);
    S=dbcB_mm256_mul_ps(S,dbcB_broadcast256_128f(color));
    ret=dbcB_float2byte_clamp_256(S);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends single pixel, linear. */
DBCB_DECL_AVX2 static void dbcB_bla_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_32_sdac(1);
    dbcB_step256_bla(s,d,a,c,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends 2 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_bla_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_64_sdac(1);
    dbcB_step256_bla(s,d,a,c,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends 4 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_bla_4_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_128_sdac(1);
    dbcB_step256_bla(s,d,a,c,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    dbcb_store256_128(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends 8 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_bla_8_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,sl,sh,dl,dh,al,ah,cl,ch,l,h,ret;
    dbcB_setup256_256_sdac(1);
    dbcB_step256_bla(sl,dl,al,cl,l);
    dbcB_step256_bla(sh,dh,ah,ch,h);
    ret=dbcB_mm256_packus_epi16(l,h);
    dbcb_store256_256(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends single pixel, linear with modulation. */
DBCB_DECL_AVX2 static void dbcB_blam_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    dbcB_setup256_32_SDAC(1);
    dbcB_step256_blam(S,D,A,C,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends 2 pixels, linear with modulation. */
DBCB_DECL_AVX2 static void dbcB_blam_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    dbcB_setup256_64_SDAC(1);
    dbcB_step256_blam(S,D,A,C,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) single pixel, linear. */
DBCB_DECL_AVX2 static void dbcB_blp_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_32_sdac(1);
    dbcB_step256_blp(s,d,c,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) 2 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_blp_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_64_sdac(1);
    dbcB_step256_blp(s,d,c,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) 4 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_blp_4_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_128_sdac(1);
    dbcB_step256_blp(s,d,c,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    dbcb_store256_128(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) 8 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_blp_8_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,sl,sh,dl,dh,al,ah,cl,ch,l,h,ret;
    dbcB_setup256_256_sdac(1);
    dbcB_step256_blp(sl,dl,cl,l);
    dbcB_step256_blp(sh,dh,ch,h);
    ret=dbcB_mm256_packus_epi16(l,h);
    dbcb_store256_256(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) single pixel, linear with modulation. */
DBCB_DECL_AVX2 static void dbcB_blpm_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    dbcB_setup256_32_SDAC(1);
    dbcB_step256_blpm(S,D,C,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) 2 pixels, linear with modulation. */
DBCB_DECL_AVX2 static void dbcB_blpm_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    dbcB_setup256_64_SDAC(1);
    dbcB_step256_blpm(S,D,C,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

/* Multiplies single pixel, linear. */
DBCB_DECL_AVX2 static void dbcB_blx_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_32_sdac(0);
    (void)a;(void)c;
    dbcB_step256_blx(s,d,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Multiplies 2 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_blx_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_64_sdac(0);
    (void)a;(void)c;
    dbcB_step256_blx(s,d,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

/* Multiplies 4 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_blx_4_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,c,ret;
    dbcB_setup256_128_sdac(0);
    (void)a;(void)c;
    dbcB_step256_blx(s,d,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    dbcb_store256_128(ret,dst);
    DBCB_ZEROUPPER();
}

/* Multiplies 8 pixels, linear. */
DBCB_DECL_AVX2 static void dbcB_blx_8_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_i32x8 s,d,a,sl,sh,dl,dh,al,ah,cl,ch,l,h,ret;
    dbcB_setup256_256_sdac(0);
    (void)a;(void)al;(void)ah;(void)cl;(void)ch;
    dbcB_step256_blx(sl,dl,l);
    dbcB_step256_blx(sh,dh,h);
    ret=dbcB_mm256_packus_epi16(l,h);
    dbcb_store256_256(ret,dst);
    DBCB_ZEROUPPER();
}

/* Multiplies single pixel, linear with modulation. */
DBCB_DECL_AVX2 static void dbcB_blxm_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    dbcB_setup256_32_SDAC(0);
    (void)A;(void)C;
    dbcB_step256_blxm(S,D,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_32(ret,dst);
    DBCB_ZEROUPPER();
}

/* Multiplies 2 pixels, linear with modulation. */
DBCB_DECL_AVX2 static void dbcB_blxm_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    dbcB_setup256_64_SDAC(0);
    (void)A;(void)C;
    dbcB_step256_blxm(S,D,ret);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);
    ret=dbcB_mm256_packus_epi16(ret,ret);
    dbcb_store256_64(ret,dst);
    DBCB_ZEROUPPER();
}

#undef dbcB_setup256_32_sdac
#undef dbcB_setup256_64_sdac
#undef dbcB_setup256_128_sdac
#undef dbcB_setup256_256_sdac
#undef dbcB_setup256_32_SDAC
#undef dbcB_setup256_64_SDAC
#undef dbcB_step256_bla
#undef dbcB_step256_blp
#undef dbcB_step256_blx
#undef dbcB_step256_blam
#undef dbcB_step256_blpm
#undef dbcB_step256_blxm

#define DBCB_DEF_B8M_AVX2(n,suffix) \
DBCB_DECL_AVX2 static void dbcB_b8m_##n##_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint8 key) \
{\
    dbcb_i32x8 s=dbcb_load256_##suffix(src);\
    dbcb_i32x8 d=dbcb_load256_##suffix(dst);\
    dbcb_i32x8 m=dbcB_mm256_set1_epi8((char)key);\
    m=dbcB_mm256_cmpeq_epi8(s,m);\
    d=dbcB_mm256_blendv_epi8(s,d,m);\
    dbcb_store256_##suffix(d,dst);\
    DBCB_ZEROUPPER();\
}

#define DBCB_DEF_B16M_AVX2(n,suffix) \
DBCB_DECL_AVX2 static void dbcB_b16m_##n##_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint16 key)\
{\
    dbcb_i32x8 s=dbcb_load256_##suffix(src);\
    dbcb_i32x8 d=dbcb_load256_##suffix(dst);\
    dbcb_i32x8 m=dbcB_mm256_set1_epi16((short)key);\
    m=dbcB_mm256_cmpeq_epi16(s,m);\
    d=dbcB_mm256_blendv_epi8(s,d,m);\
    dbcb_store256_##suffix(d,dst);\
    DBCB_ZEROUPPER();\
}

#define DBCB_DEF_B5551_AVX2(n,suffix)\
DBCB_DECL_AVX2 static void dbcB_b5551_##n##_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)\
{\
    dbcb_i32x8 s=dbcb_load256_##suffix(src);\
    dbcb_i32x8 d=dbcb_load256_##suffix(dst);\
    dbcb_i32x8 m=dbcB_mm256_cmpgt_epi16(dbcB_mm256_set1_epi16(0),s);\
    d=dbcB_mm256_blendv_epi8(d,s,m);\
    dbcb_store256_##suffix(d,dst);\
    DBCB_ZEROUPPER();\
}

#define DBCB_DEF_B32T_AVX2(n,suffix)\
DBCB_DECL_AVX2 static void dbcB_b32t_##n##_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint8 key)\
{\
    dbcb_i32x8 s=dbcb_load256_##suffix(src);\
    dbcb_i32x8 d=dbcb_load256_##suffix(dst);\
    dbcb_i32x8 m=dbcB_mm256_set1_epi32((int)((dbcb_uint32)key<<24));\
    m=dbcB_mm256_cmpgt_epi32(dbcB_mm256_xor_si256(dbcB_mm256_set1_epi32((int)0x80000000),m),dbcB_mm256_xor_si256(dbcB_mm256_set1_epi32((int)0x80000000),s));\
    d=dbcB_mm256_blendv_epi8(s,d,m);\
    dbcb_store256_##suffix(d,dst);\
    DBCB_ZEROUPPER();\
}

#define DBCB_DEF_B32S_AVX2(n,suffix)\
DBCB_DECL_AVX2 static void dbcB_b32s_##n##_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)\
{\
    dbcb_i32x8 s=dbcb_load256_##suffix(src);\
    dbcb_i32x8 d=dbcb_load256_##suffix(dst);\
    dbcb_i32x8 m=dbcB_mm256_cmpgt_epi32(dbcB_mm256_set1_epi32(0),s);\
    d=dbcB_mm256_blendv_epi8(d,s,m);\
    dbcb_store256_##suffix(d,dst);\
    DBCB_ZEROUPPER();\
}

DBCB_DEF_B8M_AVX2( 4, 32)    /* Blits  4 8-bit pixels with colorkey. */
DBCB_DEF_B8M_AVX2( 8, 64)    /* Blits  8 8-bit pixels with colorkey. */
DBCB_DEF_B8M_AVX2(16,128)    /* Blits 16 8-bit pixels with colorkey. */
DBCB_DEF_B8M_AVX2(32,256)    /* Blits 32 8-bit pixels with colorkey. */

DBCB_DEF_B16M_AVX2( 2, 32)   /* Blits  2 16-bit pixels with colorkey. */
DBCB_DEF_B16M_AVX2( 4, 64)   /* Blits  4 16-bit pixels with colorkey. */
DBCB_DEF_B16M_AVX2( 8,128)   /* Blits  8 16-bit pixels with colorkey. */
DBCB_DEF_B16M_AVX2(16,256)   /* Blits 16 16-bit pixels with colorkey. */

DBCB_DEF_B5551_AVX2( 2, 32)  /* Blits  2 16-bit (5551) pixels. */
DBCB_DEF_B5551_AVX2( 4, 64)  /* Blits  4 16-bit (5551) pixels. */
DBCB_DEF_B5551_AVX2( 8,128)  /* Blits  8 16-bit (5551) pixels. */
DBCB_DEF_B5551_AVX2(16,256)  /* Blits 16 16-bit (5551) pixels. */

DBCB_DEF_B32T_AVX2( 2, 64)   /* Blits  2 pixels, with alpha-test. */
DBCB_DEF_B32T_AVX2( 4,128)   /* Blits  4 pixels, with alpha-test. */
DBCB_DEF_B32T_AVX2( 8,256)   /* Blits  8 pixels, with alpha-test. */

DBCB_DEF_B32S_AVX2( 2, 64)   /* Blits  2 pixels, with alpha-test using threshold 128. */
DBCB_DEF_B32S_AVX2( 4,128)   /* Blits  4 pixels, with alpha-test using threshold 128. */
DBCB_DEF_B32S_AVX2( 8,256)   /* Blits  8 pixels, with alpha-test using threshold 128. */
    
#undef DBCB_DEF_B8M_AVX2
#undef DBCB_DEF_B16M_AVX2
#undef DBCB_DEF_B5551_AVX2
#undef DBCB_DEF_B32T_AVX2
#undef DBCB_DEF_B32S_AVX2

#ifndef DBC_BLIT_NO_GAMMA
#ifdef DBC_BLIT_GAMMA_NO_TABLES

#define dbcB_setr_xxxyxxxy(x,y) dbcB_mm256_setr_ps(x,x,x,y,x,x,x,y)

DBCB_DECL_AVX2 static dbcb_f32x8 dbcB_srgb2linear_ggglgggl_avx2(dbcb_i32x8 x)
{
    dbcb_f32x8 y=dbcB_mm256_cvtepi32_ps(x);
/* (MACRO+0) allows MACRO to be empty. */
#if   (DBC_BLIT_GAMMA_NO_TABLES + 0)==3 || (DBC_BLIT_GAMMA_NO_TABLES + 0)==2
    dbcb_f32x8 C1=dbcB_setr_xxxyxxxy(DBCB_S2L_C1,DBCB_1div255f);
    dbcb_f32x8 C2=dbcB_setr_xxxyxxxy(DBCB_S2L_C2,         0.0f);
    dbcb_f32x8 C3=dbcB_setr_xxxyxxxy(DBCB_S2L_C3,         0.0f);
    y=dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(C1,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(C2,dbcB_mm256_mul_ps(y,
                          C3)))));
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==1
    dbcb_f32x8 C1=dbcB_setr_xxxyxxxy(DBCB_S2L_C1,DBCB_1div255f);
    dbcb_f32x8 C2=dbcB_setr_xxxyxxxy(DBCB_S2L_C2,         0.0f);
    dbcb_f32x8 C3=dbcB_setr_xxxyxxxy(DBCB_S2L_C3,         0.0f);
    dbcb_f32x8 C4=dbcB_setr_xxxyxxxy(DBCB_S2L_C4,         0.0f);
    dbcb_f32x8 C5=dbcB_setr_xxxyxxxy(DBCB_S2L_C5,         0.0f);
    y=dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(C1,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(C2,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(C3,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(C4,dbcB_mm256_mul_ps(y,
                       C5)))))))));
#else
    dbcb_f32x8 P1=dbcB_setr_xxxyxxxy(DBCB_S2L_P1,DBCB_1div255f);
    dbcb_f32x8 P2=dbcB_setr_xxxyxxxy(DBCB_S2L_P2,         0.0f);
    dbcb_f32x8 P3=dbcB_setr_xxxyxxxy(DBCB_S2L_P3,         0.0f);
    dbcb_f32x8 P4=dbcB_setr_xxxyxxxy(DBCB_S2L_P4,         0.0f);
    dbcb_f32x8 Q0=dbcB_setr_xxxyxxxy(DBCB_S2L_Q0,         1.0f);
    dbcb_f32x8 Q1=dbcB_setr_xxxyxxxy(DBCB_S2L_Q1,         0.0f);
    dbcb_f32x8 Q2=dbcB_setr_xxxyxxxy(DBCB_S2L_Q2,         0.0f);
    dbcb_f32x8 P=dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(P1,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(P2,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(P3,dbcB_mm256_mul_ps(y,
                          P4)))))));
    dbcb_f32x8 Q=dbcB_mm256_add_ps(Q0,dbcB_mm256_mul_ps(y,
        dbcB_mm256_add_ps(Q1,dbcB_mm256_mul_ps(y,
                       Q2))));
    y=dbcB_mm256_div_ps(P,Q);
#endif
    return y;
}

DBCB_DECL_AVX2 static dbcb_i32x8 dbcB_linear2srgb_ggglgggl_avx2(dbcb_f32x8 x)
{
/* (MACRO+0) allows MACRO to be empty. */
#if   (DBC_BLIT_GAMMA_NO_TABLES + 0)==3
    dbcb_f32x8 C1=dbcB_setr_xxxyxxxy(DBCB_L2S_C1,255.0f);
    dbcb_f32x8 C2=dbcB_setr_xxxyxxxy(DBCB_L2S_C2,  0.0f);
    dbcb_f32x8 C3=dbcB_setr_xxxyxxxy(DBCB_L2S_C3,  0.0f);
    x=dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(C1,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(C2,dbcB_mm256_mul_ps(x,
                          C3)))));
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==2
    dbcb_f32x8 P1=dbcB_setr_xxxyxxxy(DBCB_L2S_P1,255.0f);
    dbcb_f32x8 P2=dbcB_setr_xxxyxxxy(DBCB_L2S_P2,  0.0f);
    dbcb_f32x8 Q0=dbcB_setr_xxxyxxxy(DBCB_L2S_Q0,  1.0f);
    dbcb_f32x8 Q1=dbcB_setr_xxxyxxxy(DBCB_L2S_Q1,  0.0f);
    dbcb_f32x8 Q2=dbcB_setr_xxxyxxxy(DBCB_L2S_Q2,  0.0f);
    dbcb_f32x8 P=dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(P1,dbcB_mm256_mul_ps(x,
                       P2)));
    dbcb_f32x8 Q=dbcB_mm256_add_ps(Q0,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(Q1,dbcB_mm256_mul_ps(x,
                       Q2))));
    x=dbcB_mm256_div_ps(P,Q);
#elif (DBC_BLIT_GAMMA_NO_TABLES + 0)==1
    dbcb_f32x8 P1=dbcB_setr_xxxyxxxy(DBCB_L2S_P1,255.0f);
    dbcb_f32x8 P2=dbcB_setr_xxxyxxxy(DBCB_L2S_P2,  0.0f);
    dbcb_f32x8 P3=dbcB_setr_xxxyxxxy(DBCB_L2S_P3,  0.0f);
    dbcb_f32x8 Q0=dbcB_setr_xxxyxxxy(DBCB_L2S_Q0,  1.0f);
    dbcb_f32x8 Q1=dbcB_setr_xxxyxxxy(DBCB_L2S_Q1,  0.0f);
    dbcb_f32x8 Q2=dbcB_setr_xxxyxxxy(DBCB_L2S_Q2,  0.0f);
    dbcb_f32x8 P=dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(P1,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(P2,dbcB_mm256_mul_ps(x,
                       P3)))));
    dbcb_f32x8 Q=dbcB_mm256_add_ps(Q0,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(Q1,dbcB_mm256_mul_ps(x,
                       Q2))));
    x=dbcB_mm256_div_ps(P,Q);
#else
    dbcb_f32x8 P1=dbcB_setr_xxxyxxxy(DBCB_L2S_P1,255.0f);
    dbcb_f32x8 P2=dbcB_setr_xxxyxxxy(DBCB_L2S_P2,  0.0f);
    dbcb_f32x8 P3=dbcB_setr_xxxyxxxy(DBCB_L2S_P3,  0.0f);
    dbcb_f32x8 P4=dbcB_setr_xxxyxxxy(DBCB_L2S_P4,  0.0f);
    dbcb_f32x8 Q0=dbcB_setr_xxxyxxxy(DBCB_L2S_Q0,  1.0f);
    dbcb_f32x8 Q1=dbcB_setr_xxxyxxxy(DBCB_L2S_Q1,  0.0f);
    dbcb_f32x8 Q2=dbcB_setr_xxxyxxxy(DBCB_L2S_Q2,  0.0f);
    dbcb_f32x8 Q3=dbcB_setr_xxxyxxxy(DBCB_L2S_Q3,  0.0f);
    dbcb_f32x8 Q4=dbcB_setr_xxxyxxxy(DBCB_L2S_Q4,  0.0f);
    dbcb_f32x8 P=dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(P1,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(P2,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(P3,dbcB_mm256_mul_ps(x,
                          P4)))))));
    dbcb_f32x8 Q=dbcB_mm256_add_ps(Q0,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(Q1,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(Q2,dbcB_mm256_mul_ps(x,
        dbcB_mm256_add_ps(Q3,dbcB_mm256_mul_ps(x,
                          Q4))))))));
    x=dbcB_mm256_div_ps(P,Q);
#endif
    return dbcB_mm256_cvttps_epi32(dbcB_mm256_add_ps(x,dbcB_mm256_set1_ps(0.5f)));
}

#undef dbcB_setr_xxxy

/*
    Note: tests for the trivial cases are to speed them up, but
    also to get the exact result, in case the srgb <-> linear
    approximations do not round-trip.
*/

#define dbcB_setup256_32_gggl(dl,ac,m)\
    s=dbcb_load256_32(src);                                     \
    if(dl) d=dbcb_load256_32(dst);                              \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));     \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));     \
    if(dl) d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));\
    if(dl) d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));\
    S=dbcB_srgb2linear_ggglgggl_avx2(s);                        \
    if(dl) D=dbcB_srgb2linear_ggglgggl_avx2(d);                 \
    if(m) S=dbcB_mm256_mul_ps(S,dbcB_broadcast256_128f(color)); \
    if(ac) A=dbcB_mm256_shuffle_ps(S,S,0xFF);                   \
    if(ac) C=dbcB_mm256_sub_ps(dbcB_mm256_set1_ps(1.0f),A);

#define dbcB_output256_32_gggl(cl)\
    if(cl) D=dbcB_mm256_min_ps(dbcB_mm256_max_ps(D,dbcB_mm256_set1_ps(0.0f)),dbcB_mm256_set1_ps(1.0f));\
    ret=dbcB_linear2srgb_ggglgggl_avx2(D);                      \
    ret=dbcB_mm256_packus_epi16(ret,ret);                       \
    ret=dbcB_mm256_packus_epi16(ret,ret);                       \
    dbcb_store256_32(ret,dst);

#define dbcB_setup256_64_gggl(dl,ac,m)\
    s=dbcb_load256_64(src);                                     \
    if(dl) d=dbcb_load256_64(dst);                              \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));     \
    s=dbcB_mm256_permute4x64_epi64(s,0xD8);                     \
    s=dbcB_mm256_unpacklo_epi8(s,dbcB_mm256_set1_epi16(0));     \
    if(dl) d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));\
    if(dl) d=dbcB_mm256_permute4x64_epi64(d,0xD8);              \
    if(dl) d=dbcB_mm256_unpacklo_epi8(d,dbcB_mm256_set1_epi16(0));\
    S=dbcB_srgb2linear_ggglgggl_avx2(s);                        \
    if(dl) D=dbcB_srgb2linear_ggglgggl_avx2(d);                 \
    if(m) S=dbcB_mm256_mul_ps(S,dbcB_broadcast256_128f(color)); \
    if(ac) A=dbcB_mm256_shuffle_ps(S,S,0xFF);                   \
    if(ac) C=dbcB_mm256_sub_ps(dbcB_mm256_set1_ps(1.0f),A);

#define dbcB_output256_64_gggl(cl)\
    if(cl) D=dbcB_mm256_min_ps(dbcB_mm256_max_ps(D,dbcB_mm256_set1_ps(0.0f)),dbcB_mm256_set1_ps(1.0f));\
    ret=dbcB_linear2srgb_ggglgggl_avx2(D);                      \
    ret=dbcB_mm256_packus_epi16(ret,ret);                       \
    ret=dbcB_mm256_permute4x64_epi64(ret,0xD8);                 \
    ret=dbcB_mm256_packus_epi16(ret,ret);                       \
    dbcb_store256_64(ret,dst);

/* Alpha-blends single pixel, gamma-corrected. */
DBCB_DECL_AVX2 static void dbcB_bga_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S=dbcb_load32(src);
    if(S>0x00FFFFFFu)
    {
        if(S>=0xFF000000u) dbcb_store32(S,dst);
        else
        {
            dbcb_i32x8 s,d,ret;
            dbcb_f32x8 S,D,A,C;
            void *color=0;
            (void)color;
            dbcB_setup256_32_gggl(1,1,0);
            S=dbcB_mm256_blend_ps(dbcB_mm256_set1_ps(1.0f),S,0x77);
            D=dbcB_mm256_add_ps(dbcB_mm256_mul_ps(A,S),dbcB_mm256_mul_ps(C,D));
            dbcB_output256_32_gggl(0);
        }
    }
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) single pixel, gamma-corrected. */
DBCB_DECL_AVX2 static void dbcB_bgp_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S=dbcb_load32(src);
    dbcb_uint32 D=dbcb_load32(dst);
    if(S>0u)
    {
        if(S>=0xFF000000u||D==0u) dbcb_store32(S,dst);
        else
        {
            dbcb_i32x8 s,d,ret;
            dbcb_f32x8 S,D,A,C;
            void *color=0;
            (void)color;
            dbcB_setup256_32_gggl(1,1,0);
            D=dbcB_mm256_mul_ps(C,D);
            D=dbcB_mm256_add_ps(D,S);
            dbcB_output256_32_gggl(1);
        }
    }
    DBCB_ZEROUPPER();
}

/* Multiplies single pixel, gamma-corrected. */
DBCB_DECL_AVX2 static void dbcB_bgx_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S=dbcb_load32(src);
    dbcb_uint32 D=dbcb_load32(dst);
    if(S<0xFFFFFFFFu)
    {
        if(S==0u) dbcb_store32(S,dst);
        else if(D==0u) dbcb_store32(D,dst);
        else
        {
            dbcb_i32x8 s,d,ret;
            dbcb_f32x8 S,D,A,C;
            void *color=0;
            (void)color;
            (void)A;(void)C;
            dbcB_setup256_32_gggl(1,0,0);
            D=dbcB_mm256_mul_ps(S,D);
            dbcB_output256_32_gggl(0);
        }
    }
    DBCB_ZEROUPPER();
}

/* Copies single pixel, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_b32g_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    (void)A;(void)C;
    dbcB_setup256_32_gggl(0,0,1);
    D=S;
    dbcB_output256_32_gggl(1);
    DBCB_ZEROUPPER();
}

/* Alpha-blends single pixel, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgam_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S=dbcb_load32(src);
    if(S>0x00FFFFFFu&&color[3]!=0.0f)
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        dbcB_setup256_32_gggl(1,1,1);
        S=dbcB_mm256_blend_ps(dbcB_mm256_set1_ps(1.0f),S,0x77);
        D=dbcB_mm256_add_ps(dbcB_mm256_mul_ps(A,S),dbcB_mm256_mul_ps(C,D));
        dbcB_output256_32_gggl(1);
    }
    DBCB_ZEROUPPER();
}

/* Alpha-blends (PMA) single pixel, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgpm_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S=dbcb_load32(src);
    if(S>0u)
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        dbcB_setup256_32_gggl(1,1,1);
        D=dbcB_mm256_mul_ps(C,D);
        D=dbcB_mm256_add_ps(D,S);
        dbcB_output256_32_gggl(1);
    }
    DBCB_ZEROUPPER();
}

/* Multiplies single pixel, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgxm_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S=dbcb_load32(src);
    dbcb_uint32 D=dbcb_load32(dst);
    if(S==0u) dbcb_store32(S,dst);
    else if(D==0u) dbcb_store32(D,dst);
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        (void)A;(void)C;
        dbcB_setup256_32_gggl(1,0,1);
        D=dbcB_mm256_mul_ps(S,D);
        dbcB_output256_32_gggl(1);
    }
    DBCB_ZEROUPPER();
}

/* Alpha-blends 2 pixels, gamma-corrected. */
DBCB_DECL_AVX2 static void dbcB_bga_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S0=dbcb_load32(src  );
    dbcb_uint32 S1=dbcb_load32(src+4);
    if(S0<=0x00FFFFFFu||S1<=0x00FFFFFFu||S0>=0xFF000000u||S1>=0xFF000000u)
    {
        dbcB_bga_1_avx2(src  ,dst  );
        dbcB_bga_1_avx2(src+4,dst+4);
    }
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        void *color=0;
        (void)color;
        dbcB_setup256_64_gggl(1,1,0);
        S=dbcB_mm256_blend_ps(dbcB_mm256_set1_ps(1.0f),S,0x77);
        D=dbcB_mm256_add_ps(dbcB_mm256_mul_ps(A,S),dbcB_mm256_mul_ps(C,D));
        dbcB_output256_64_gggl(0);
	DBCB_ZEROUPPER();
    }
}

/* Multiplies 2 pixels, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgp_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S0=dbcb_load32(src  );
    dbcb_uint32 S1=dbcb_load32(src+4);
    dbcb_uint32 D0=dbcb_load32(dst  );
    dbcb_uint32 D1=dbcb_load32(dst+4);
    if(S0==0u||S1==0u||S0>=0xFF000000u||S1>=0xFF000000u||D0==0u||D1==0u)
    {
        dbcB_bgp_1_avx2(src  ,dst  );
        dbcB_bgp_1_avx2(src+4,dst+4);
    }
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        void *color=0;
        (void)color;
        dbcB_setup256_64_gggl(1,1,0);
        D=dbcB_mm256_mul_ps(C,D);
        D=dbcB_mm256_add_ps(D,S);
        dbcB_output256_64_gggl(1);
        DBCB_ZEROUPPER();
    }
}

/* Multiplies 2 pixels, gamma-corrected. */
DBCB_DECL_AVX2 static void dbcB_bgx_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst)
{
    dbcb_uint32 S0=dbcb_load32(src  );
    dbcb_uint32 S1=dbcb_load32(src+4);
    dbcb_uint32 D0=dbcb_load32(dst  );
    dbcb_uint32 D1=dbcb_load32(dst+4);
    if(S0==0xFFFFFFFFu||S1==0xFFFFFFFFu||S0==0u||S1==0u||D0==0u||D1==0u)
    {
        dbcB_bgx_1_avx2(src  ,dst  );
        dbcB_bgx_1_avx2(src+4,dst+4);
    }
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        void *color=0;
        (void)color;
        (void)A;(void)C;
        dbcB_setup256_64_gggl(1,0,0);
        D=dbcB_mm256_mul_ps(S,D);
        dbcB_output256_64_gggl(0);
        DBCB_ZEROUPPER();
    }
}

/* Copies 2 pixels, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_b32g_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_i32x8 s,d,ret;
    dbcb_f32x8 S,D,A,C;
    (void)A;(void)C;
    dbcB_setup256_64_gggl(0,0,1);
    D=S;
    dbcB_output256_64_gggl(1);
    DBCB_ZEROUPPER();
}

/* Alpha-blends 2 pixels, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgam_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S0=dbcb_load32(src  );
    dbcb_uint32 S1=dbcb_load32(src+4);
    if(S0<=0x00FFFFFFu||S1<=0x00FFFFFFu||color[3]==0.0f)
    {
        dbcB_bgam_1_avx2(src  ,dst  ,color);
        dbcB_bgam_1_avx2(src+4,dst+4,color);
    }
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        dbcB_setup256_64_gggl(1,1,1);
        S=dbcB_mm256_blend_ps(dbcB_mm256_set1_ps(1.0f),S,0x77);
        D=dbcB_mm256_add_ps(dbcB_mm256_mul_ps(A,S),dbcB_mm256_mul_ps(C,D));
        dbcB_output256_64_gggl(1);
        DBCB_ZEROUPPER();
    }
}

/* Alpha-blends (PMA) 2 pixels, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgpm_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S0=dbcb_load32(src  );
    dbcb_uint32 S1=dbcb_load32(src+4);
    if(S0==0u||S1==0u)
    {
        dbcB_bgpm_1_avx2(src  ,dst  ,color);
        dbcB_bgpm_1_avx2(src+4,dst+4,color);
    }
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        dbcB_setup256_64_gggl(1,1,1);
        D=dbcB_mm256_mul_ps(C,D);
        D=dbcB_mm256_add_ps(D,S);
        dbcB_output256_64_gggl(1);
        DBCB_ZEROUPPER();
    }
}

/* Multiplies 2 pixels, gamma-corrected, with modulation. */
DBCB_DECL_AVX2 static void dbcB_bgxm_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color)
{
    dbcb_uint32 S0=dbcb_load32(src  );
    dbcb_uint32 S1=dbcb_load32(src+4);
    dbcb_uint32 D0=dbcb_load32(dst  );
    dbcb_uint32 D1=dbcb_load32(dst+4);
    if(S0==0u||S1==0u||D0==0u||D1==0u)
    {
        dbcB_bgxm_1_avx2(src  ,dst  ,color);
        dbcB_bgxm_1_avx2(src+4,dst+4,color);
    }
    else
    {
        dbcb_i32x8 s,d,ret;
        dbcb_f32x8 S,D,A,C;
        (void)A;(void)C;
        dbcB_setup256_64_gggl(1,0,1);
        D=dbcB_mm256_mul_ps(S,D);
        dbcB_output256_64_gggl(1);
        DBCB_ZEROUPPER();
    }
}

#undef dbcB_setup256_32_gggl
#undef dbcB_output256_32_gggl

#undef dbcB_setup256_64_gggl
#undef dbcB_output256_64_gggl

#else
DBCB_DECL_AVX2 static void dbcB_bga_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bga_1_c(src,dst);}
DBCB_DECL_AVX2 static void dbcB_bgp_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bgp_1_c(src,dst);}
DBCB_DECL_AVX2 static void dbcB_bgx_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bgx_1_c(src,dst);}
DBCB_DECL_AVX2 static void dbcB_b32g_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_b32g_1_c(src,dst,color);}
DBCB_DECL_AVX2 static void dbcB_bgam_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgam_1_c(src,dst,color);}
DBCB_DECL_AVX2 static void dbcB_bgpm_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgpm_1_c(src,dst,color);}
DBCB_DECL_AVX2 static void dbcB_bgxm_1_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgxm_1_c(src,dst,color);}

DBCB_DECL_AVX2 static void dbcB_bga_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bga_1_avx2(src  ,dst  );dbcB_bga_1_avx2(src+4,dst+4);}
DBCB_DECL_AVX2 static void dbcB_bgp_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bgp_1_avx2(src  ,dst  );dbcB_bgp_1_avx2(src+4,dst+4);}
DBCB_DECL_AVX2 static void dbcB_bgx_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst) {dbcB_bgx_1_avx2(src  ,dst  );dbcB_bgx_1_avx2(src+4,dst+4);}
DBCB_DECL_AVX2 static void dbcB_b32g_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_b32g_1_avx2(src  ,dst  ,color);dbcB_b32g_1_avx2(src+4,dst+4,color);}
DBCB_DECL_AVX2 static void dbcB_bgam_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgam_1_avx2(src  ,dst  ,color);dbcB_bgam_1_avx2(src+4,dst+4,color);}
DBCB_DECL_AVX2 static void dbcB_bgpm_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgpm_1_avx2(src  ,dst  ,color);dbcB_bgpm_1_avx2(src+4,dst+4,color);}
DBCB_DECL_AVX2 static void dbcB_bgxm_2_avx2(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {dbcB_bgxm_1_avx2(src  ,dst  ,color);dbcB_bgxm_1_avx2(src+4,dst+4,color);}
#endif /* DBC_BLIT_GAMMA_NO_TABLES */
#endif /* DBC_BLIT_NO_GAMMA */

#endif /* DBC_BLIT_NO_AVX2 */

#endif /* defined(DBCB_X86_OR_X64) */

#endif /* !defined(DBC_BLIT_NO_SIMD) */

/*============================================================================*/
/* Inner loops */

#define DBCB_FN_SIG(name) \
static void name(                                                      \
    dbcb_int32 src_stride,const dbcb_uint8 *src_pixels,                \
    dbcb_int32 dst_stride,dbcb_uint8 *dst_pixels,                      \
    dbcb_int32 x0,dbcb_int32 y0,dbcb_int32 x1,dbcb_int32 y1,           \
    dbcb_int32 x,dbcb_int32 y,                                         \
    const float *color)

#define DBCB_FN_HEADER(pixel_size,mode,modulated) \
    dbcb_int32 w=x1-x0,h=y1-y0;                                        \
    dbcb_int32 iy=0;                                                   \
    const dbcb_uint8 *src=src_pixels+y0*src_stride+x0*pixel_size;      \
    dbcb_uint8 *dst=dst_pixels+(y0+y)*dst_stride+(x0+x)*pixel_size;    \
    dbcb_uint8 key8=0;                                                 \
    dbcb_uint16 key16=0;                                               \
    dbcb_int32 unroll_max=(dbcb_unroll_limit_for_mode(mode,modulated));\
    (void)color;                                                       \
    (void)mode;                                                        \
    (void)key8;                                                        \
    (void)key16;                                                       \
    (void)unroll_max;                                                  \
    if(color&&color[0]>=0.0f&&color[0]<=65535.0f)                      \
    {                                                                  \
        key16=(dbcb_uint16)(dbcb_int32)color[0];                       \
        key8=(dbcb_uint8)key16;                                        \
        if(mode==DBCB_MODE_ALPHATEST&&(float)key8!=color[0]) ++key8;   \
    }                                                                  \
    if(w<=0||h<=0) return;

#define DBCB_FN_SWITCH0_CASE(pixel_size,i)\
            case  i: for(;iy<h;++iy) {dbcb_memcpy(dst,src,(dbcb_uint32)( i*(pixel_size)));src+=src_stride;dst+=dst_stride;} return;\

#define DBCB_FN_SWITCH_LEFT(i)\
    case i: for(;iy<h;++iy) {const dbcb_uint8 *s=src;dbcb_uint8 *d=dst;

#define DBCB_FN_SWITCH_RIGHT\
    src+=src_stride;dst+=dst_stride;}return;

#if DBC_BLIT_UNROLL==32

#define DBCB_FN_SWITCH0(pixel_size)\
        if(unroll_max>0&&w<=unroll_max&&w<=(DBC_BLIT_UNROLL)) switch(w)\
        {\
            case  0: return;\
            DBCB_FN_SWITCH0_CASE(pixel_size, 1)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 2)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 3)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 4)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 5)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 6)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 7)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 8)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 9)\
            DBCB_FN_SWITCH0_CASE(pixel_size,10)\
            DBCB_FN_SWITCH0_CASE(pixel_size,11)\
            DBCB_FN_SWITCH0_CASE(pixel_size,12)\
            DBCB_FN_SWITCH0_CASE(pixel_size,13)\
            DBCB_FN_SWITCH0_CASE(pixel_size,14)\
            DBCB_FN_SWITCH0_CASE(pixel_size,15)\
            DBCB_FN_SWITCH0_CASE(pixel_size,16)\
            DBCB_FN_SWITCH0_CASE(pixel_size,17)\
            DBCB_FN_SWITCH0_CASE(pixel_size,18)\
            DBCB_FN_SWITCH0_CASE(pixel_size,19)\
            DBCB_FN_SWITCH0_CASE(pixel_size,20)\
            DBCB_FN_SWITCH0_CASE(pixel_size,21)\
            DBCB_FN_SWITCH0_CASE(pixel_size,22)\
            DBCB_FN_SWITCH0_CASE(pixel_size,23)\
            DBCB_FN_SWITCH0_CASE(pixel_size,24)\
            DBCB_FN_SWITCH0_CASE(pixel_size,25)\
            DBCB_FN_SWITCH0_CASE(pixel_size,26)\
            DBCB_FN_SWITCH0_CASE(pixel_size,27)\
            DBCB_FN_SWITCH0_CASE(pixel_size,28)\
            DBCB_FN_SWITCH0_CASE(pixel_size,29)\
            DBCB_FN_SWITCH0_CASE(pixel_size,30)\
            DBCB_FN_SWITCH0_CASE(pixel_size,31)\
            DBCB_FN_SWITCH0_CASE(pixel_size,32)\
        }

#define DBCB_FN_SWITCH(blit1,blit2,blit4,blit8,blit16,blit32)\
        if(unroll_max>0&&w<=unroll_max&&w<=(DBC_BLIT_UNROLL)) switch(w)\
        {\
            case  0: return;\
            DBCB_FN_SWITCH_LEFT( 1)                            blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 2)                     blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 3)                     blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 4)              blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 5)              blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 6)              blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 7)              blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 8)       blit8 ;                     DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 9)       blit8 ;              blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(10)       blit8 ;       blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(11)       blit8 ;       blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(12)       blit8 ;blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(13)       blit8 ;blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(14)       blit8 ;blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(15)       blit8 ;blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(16)blit16;                            DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(17)blit16;                     blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(18)blit16;              blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(19)blit16;              blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(20)blit16;       blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(21)blit16;       blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(22)blit16;       blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(23)blit16;       blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(24)blit16;blit8 ;                     DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(25)blit16;blit8 ;              blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(26)blit16;blit8 ;       blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(27)blit16;blit8 ;       blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(28)blit16;blit8 ;blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(29)blit16;blit8 ;blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(30)blit16;blit8 ;blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(31)blit16;blit8 ;blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(32)              blit32;              DBCB_FN_SWITCH_RIGHT\
        }

#elif DBC_BLIT_UNROLL==16

#define DBCB_FN_SWITCH0(pixel_size)\
        if(unroll_max>0&&w<=unroll_max&&w<=(DBC_BLIT_UNROLL)) switch(w)\
        {\
            case  0: return;\
            DBCB_FN_SWITCH0_CASE(pixel_size, 1)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 2)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 3)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 4)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 5)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 6)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 7)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 8)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 9)\
            DBCB_FN_SWITCH0_CASE(pixel_size,10)\
            DBCB_FN_SWITCH0_CASE(pixel_size,11)\
            DBCB_FN_SWITCH0_CASE(pixel_size,12)\
            DBCB_FN_SWITCH0_CASE(pixel_size,13)\
            DBCB_FN_SWITCH0_CASE(pixel_size,14)\
            DBCB_FN_SWITCH0_CASE(pixel_size,15)\
            DBCB_FN_SWITCH0_CASE(pixel_size,16)\
        }

#define DBCB_FN_SWITCH(blit1,blit2,blit4,blit8,blit16,blit32)\
        if(unroll_max>0&&w<=unroll_max&&w<=(DBC_BLIT_UNROLL)) switch(w)\
        {\
            case  0: return;\
            DBCB_FN_SWITCH_LEFT( 1)                            blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 2)                     blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 3)                     blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 4)              blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 5)              blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 6)              blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 7)              blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 8)       blit8 ;                     DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 9)       blit8 ;              blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(10)       blit8 ;       blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(11)       blit8 ;       blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(12)       blit8 ;blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(13)       blit8 ;blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(14)       blit8 ;blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(15)       blit8 ;blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT(16)blit16;                            DBCB_FN_SWITCH_RIGHT\
        }

#elif DBC_BLIT_UNROLL==8

#define DBCB_FN_SWITCH0(pixel_size)\
        if(unroll_max>0&&w<=unroll_max&&w<=(DBC_BLIT_UNROLL)) switch(w)\
        {\
            case  0: return;\
            DBCB_FN_SWITCH0_CASE(pixel_size, 1)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 2)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 3)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 4)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 5)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 6)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 7)\
            DBCB_FN_SWITCH0_CASE(pixel_size, 8)\
        }

#define DBCB_FN_SWITCH(blit1,blit2,blit4,blit8,blit16,blit32)\
        if(unroll_max>0&&w<=unroll_max&&w<=(DBC_BLIT_UNROLL)) switch(w)\
        {\
            case  0: return;\
            DBCB_FN_SWITCH_LEFT( 1)                            blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 2)                     blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 3)                     blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 4)              blit4 ;              DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 5)              blit4 ;       blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 6)              blit4 ;blit2 ;       DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 7)              blit4 ;blit2 ;blit1 ;DBCB_FN_SWITCH_RIGHT\
            DBCB_FN_SWITCH_LEFT( 8)       blit8 ;                     DBCB_FN_SWITCH_RIGHT\
        }

#elif DBC_BLIT_UNROLL==0
#define DBCB_FN_SWITCH0(pixel_size)
#define DBCB_FN_SWITCH(blit1,blit2,blit4,blit8,blit16,blit32)
#else
#error Unsupported value of DBC_BLIT_UNROLL
#endif /* DBC_BLIT_UNROLL==32 */

#define DBCB_FN_C(blit,cnt,pixel_size) blit;s+=(cnt)*(pixel_size);d+=(cnt)*(pixel_size);
#define DBCB_FN_C2(blit)  blit blit
#define DBCB_FN_C4(blit)  DBCB_FN_C2(DBCB_FN_C2(blit))
#define DBCB_FN_C8(blit)  DBCB_FN_C2(DBCB_FN_C4(blit))
#define DBCB_FN_C16(blit) DBCB_FN_C2(DBCB_FN_C8(blit))
#define DBCB_FN_C32(blit) DBCB_FN_C2(DBCB_FN_C16(blit))

#define DBCB_FN_LOOP_TOP \
    for(;iy<h;++iy)                                                   \
    {                                                                 \
        const dbcb_uint8 *s=src;                                      \
        dbcb_uint8 *d=dst;                                            \
        dbcb_int32 ix=0;

#define DBCB_FN_LOOP_BOTTOM \
        src+=src_stride;                                              \
        dst+=dst_stride;                                              \
    }

#define DBCB_FN_LOOP_FOR(pixel_size,log2width,blit) \
        for(;ix<(w>>log2width);++ix)                                  \
        {                                                             \
            blit;                                                     \
            s+=((pixel_size)<<(log2width));                           \
            d+=((pixel_size)<<(log2width));                           \
        }

#define DBCB_FN_LOOP_IF(pixel_size,width,blit) \
        if(w&width)                                                   \
        {                                                             \
            blit;                                                     \
            s+=(width)*(pixel_size);                                  \
            d+=(width)*(pixel_size);                                  \
        }

#define DBCB_DEF_FN_0(name,mode,modulated,pixel_size) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH0(pixel_size)                                   \
        for(;iy<h;++iy)                                               \
        {                                                             \
            dbcb_memcpy(dst,src,(dbcb_uint32)(w*(pixel_size)));       \
            src+=src_stride;                                          \
            dst+=dst_stride;                                          \
        }                                                             \
    }

#define DBCB_DEF_FN_1(name,mode,modulated,pixel_size,blit1) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH(DBCB_FN_C(blit1,1,pixel_size),DBCB_FN_C2(DBCB_FN_C(blit1,1,pixel_size)),DBCB_FN_C4(DBCB_FN_C(blit1,1,pixel_size)),DBCB_FN_C8(DBCB_FN_C(blit1,1,pixel_size)),DBCB_FN_C16(DBCB_FN_C(blit1,1,pixel_size)),DBCB_FN_C32(DBCB_FN_C(blit1,1,pixel_size)))\
        DBCB_FN_LOOP_TOP                                              \
        DBCB_FN_LOOP_FOR(pixel_size,0,blit1)                          \
        DBCB_FN_LOOP_BOTTOM                                           \
    }

#define DBCB_DEF_FN_2(name,mode,modulated,pixel_size,blit1,blit2) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH(DBCB_FN_C(blit1,1,pixel_size),DBCB_FN_C(blit2,2,pixel_size),DBCB_FN_C2(DBCB_FN_C(blit2,2,pixel_size)),DBCB_FN_C4(DBCB_FN_C(blit2,2,pixel_size)),DBCB_FN_C8(DBCB_FN_C(blit2,2,pixel_size)),DBCB_FN_C16(DBCB_FN_C(blit2,2,pixel_size)))\
        DBCB_FN_LOOP_TOP                                              \
        DBCB_FN_LOOP_FOR(pixel_size,1,blit2)                          \
        DBCB_FN_LOOP_IF(pixel_size,1,blit1)                           \
        DBCB_FN_LOOP_BOTTOM                                           \
    }

#define DBCB_DEF_FN_4(name,mode,modulated,pixel_size,blit1,blit2,blit4) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH(DBCB_FN_C(blit1,1,pixel_size),DBCB_FN_C(blit2,2,pixel_size),DBCB_FN_C(blit4,4,pixel_size),DBCB_FN_C2(DBCB_FN_C(blit4,4,pixel_size)),DBCB_FN_C4(DBCB_FN_C(blit4,4,pixel_size)),DBCB_FN_C8(DBCB_FN_C(blit4,4,pixel_size)))\
        DBCB_FN_LOOP_TOP                                              \
        DBCB_FN_LOOP_FOR(pixel_size,2,blit4)                          \
        DBCB_FN_LOOP_IF(pixel_size,2,blit2)                           \
        DBCB_FN_LOOP_IF(pixel_size,1,blit1)                           \
        DBCB_FN_LOOP_BOTTOM                                           \
    }

#define DBCB_DEF_FN_8(name,mode,modulated,pixel_size,blit1,blit2,blit4,blit8) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH(DBCB_FN_C(blit1,1,pixel_size),DBCB_FN_C(blit2,2,pixel_size),DBCB_FN_C(blit4,4,pixel_size),DBCB_FN_C(blit8,8,pixel_size),DBCB_FN_C2(DBCB_FN_C(blit8,8,pixel_size)),DBCB_FN_C4(DBCB_FN_C(blit8,8,pixel_size)))\
        DBCB_FN_LOOP_TOP                                              \
        DBCB_FN_LOOP_FOR(pixel_size,3,blit8)                          \
        DBCB_FN_LOOP_IF(pixel_size,4,blit4)                           \
        DBCB_FN_LOOP_IF(pixel_size,2,blit2)                           \
        DBCB_FN_LOOP_IF(pixel_size,1,blit1)                           \
        DBCB_FN_LOOP_BOTTOM                                           \
    }

#define DBCB_DEF_FN_16(name,mode,modulated,pixel_size,blit1,blit2,blit4,blit8,blit16) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH(DBCB_FN_C(blit1,1,pixel_size),DBCB_FN_C(blit2,2,pixel_size),DBCB_FN_C(blit4,4,pixel_size),DBCB_FN_C(blit8,8,pixel_size),DBCB_FN_C(blit16,16,pixel_size),DBCB_FN_C2(DBCB_FN_C(blit16,16,pixel_size)))\
        DBCB_FN_LOOP_TOP                                              \
        DBCB_FN_LOOP_FOR(pixel_size,4,blit16)                         \
        DBCB_FN_LOOP_IF(pixel_size,8,blit8)                           \
        DBCB_FN_LOOP_IF(pixel_size,4,blit4)                           \
        DBCB_FN_LOOP_IF(pixel_size,2,blit2)                           \
        DBCB_FN_LOOP_IF(pixel_size,1,blit1)                           \
        DBCB_FN_LOOP_BOTTOM                                           \
    }

#define DBCB_DEF_FN_32(name,mode,modulated,pixel_size,blit1,blit2,blit4,blit8,blit16,blit32) \
    DBCB_FN_SIG(name)                                                 \
    {                                                                 \
        DBCB_FN_HEADER(pixel_size,mode,modulated)                     \
        DBCB_FN_SWITCH(DBCB_FN_C(blit1,1,pixel_size),DBCB_FN_C(blit2,2,pixel_size),DBCB_FN_C(blit4,4,pixel_size),DBCB_FN_C(blit8,8,pixel_size),DBCB_FN_C(blit16,16,pixel_size),DBCB_FN_C(blit32,32,pixel_size))\
        DBCB_FN_LOOP_TOP                                              \
        DBCB_FN_LOOP_FOR(pixel_size,5,blit32)                         \
        DBCB_FN_LOOP_IF(pixel_size,16,blit16)                         \
        DBCB_FN_LOOP_IF(pixel_size, 8,blit8)                          \
        DBCB_FN_LOOP_IF(pixel_size, 4,blit4)                          \
        DBCB_FN_LOOP_IF(pixel_size, 2,blit2)                          \
        DBCB_FN_LOOP_IF(pixel_size, 1,blit1)                          \
        DBCB_FN_LOOP_BOTTOM                                           \
    }

DBCB_DEF_FN_0 (dbcB_f32_c   ,DBCB_MODE_COPY      ,0, 4)
DBCB_DEF_FN_1 (dbcB_f32m_c  ,DBCB_MODE_COPY      ,1, 4,(dbcB_b32m_1_c(s,d,color)))
DBCB_DEF_FN_4 (dbcB_fla_c   ,DBCB_MODE_ALPHA     ,0, 4,(dbcB_bla_1_c(s,d)),(dbcB_bla_2_c(s,d)),(dbcB_bla_4_c(s,d)))
DBCB_DEF_FN_1 (dbcB_flam_c  ,DBCB_MODE_ALPHA     ,1, 4,(dbcB_blam_1_c(s,d,color)))
DBCB_DEF_FN_4 (dbcB_flp_c   ,DBCB_MODE_PMA       ,0, 4,(dbcB_blp_1_c(s,d)),(dbcB_blp_2_c(s,d)),(dbcB_blp_4_c(s,d)))
DBCB_DEF_FN_1 (dbcB_flpm_c  ,DBCB_MODE_PMA       ,1, 4,(dbcB_blpm_1_c(s,d,color)))
DBCB_DEF_FN_0 (dbcB_f8_c    ,DBCB_MODE_COLORKEY8 ,0, 1)
DBCB_DEF_FN_16(dbcB_f8m_c   ,DBCB_MODE_COLORKEY8 ,1, 1,(dbcB_b8m_1_c(s,d,key8)),(dbcB_b8m_2_c(s,d,key8)),(dbcB_b8m_4_c(s,d,key8)),(dbcB_b8m_8_c(s,d,key8)),(dbcB_b8m_16_c(s,d,key8)))
DBCB_DEF_FN_0 (dbcB_f16_c   ,DBCB_MODE_COLORKEY16,0, 2)
DBCB_DEF_FN_8 (dbcB_f16m_c  ,DBCB_MODE_COLORKEY16,1, 2,(dbcB_b16m_1_c(s,d,key16)),(dbcB_b16m_2_c(s,d,key16)),(dbcB_b16m_4_c(s,d,key16)),(dbcB_b16m_8_c(s,d,key16)))
DBCB_DEF_FN_8 (dbcB_f5551_c ,DBCB_MODE_5551      ,0, 2,(dbcB_b5551_1_c(s,d)),(dbcB_b5551_2_c(s,d)),(dbcB_b5551_4_c(s,d)),(dbcB_b5551_8_c(s,d)))
DBCB_DEF_FN_4 (dbcB_flx_c   ,DBCB_MODE_MUL       ,0, 4,(dbcB_blx_1_c(s,d)),(dbcB_blx_2_c(s,d)),(dbcB_blx_4_c(s,d)))
DBCB_DEF_FN_1 (dbcB_flxm_c  ,DBCB_MODE_MUL       ,1, 4,(dbcB_blxm_1_c(s,d,color)))
DBCB_DEF_FN_0 (dbcB_f32a_c  ,DBCB_MODE_ALPHATEST ,0, 4)
DBCB_DEF_FN_4 (dbcB_f32t_c  ,DBCB_MODE_ALPHATEST ,1, 4,(dbcB_b32t_1_c(s,d,key8)),(dbcB_b32t_2_c(s,d,key8)),(dbcB_b32t_4_c(s,d,key8)))
DBCB_DEF_FN_4 (dbcB_f32s_c  ,DBCB_MODE_ALPHATEST ,1, 4,(dbcB_b32s_1_c(s,d)),(dbcB_b32s_2_c(s,d)),(dbcB_b32s_4_c(s,d)))
#ifndef DBC_BLIT_NO_GAMMA
DBCB_DEF_FN_0 (dbcB_f32c_c  ,DBCB_MODE_CPYG      ,0, 4)
DBCB_DEF_FN_1 (dbcB_f32g_c  ,DBCB_MODE_CPYG      ,1, 4,(dbcB_b32g_1_c(s,d,color)))
DBCB_DEF_FN_4 (dbcB_fga_c   ,DBCB_MODE_GAMMA     ,0, 4,(dbcB_bga_1_c(s,d)),(dbcB_bga_2_c(s,d)),(dbcB_bga_4_c(s,d)))
DBCB_DEF_FN_1 (dbcB_fgam_c  ,DBCB_MODE_GAMMA     ,1, 4,(dbcB_bgam_1_c(s,d,color)))
DBCB_DEF_FN_4 (dbcB_fgp_c   ,DBCB_MODE_PMG       ,0, 4,(dbcB_bgp_1_c(s,d)),(dbcB_bgp_2_c(s,d)),(dbcB_bgp_4_c(s,d)))
DBCB_DEF_FN_1 (dbcB_fgpm_c  ,DBCB_MODE_PMG       ,1, 4,(dbcB_bgpm_1_c(s,d,color)))
DBCB_DEF_FN_4 (dbcB_fgx_c   ,DBCB_MODE_MUG       ,0, 4,(dbcB_bgx_1_c(s,d)),(dbcB_bgx_2_c(s,d)),(dbcB_bgx_4_c(s,d)))
DBCB_DEF_FN_1 (dbcB_fgxm_c  ,DBCB_MODE_MUG       ,1, 4,(dbcB_bgxm_1_c(s,d,color)))
#endif /* DBC_BLIT_NO_GAMMA */

#ifndef DBC_BLIT_NO_SIMD
#ifdef DBCB_X86_OR_X64
DBCB_DECL_SSE2 DBCB_DEF_FN_0 (dbcB_f32_sse2   ,DBCB_MODE_COPY      ,0, 4)
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_f32m_sse2  ,DBCB_MODE_COPY      ,1, 4,(dbcB_b32m_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_4 (dbcB_fla_sse2   ,DBCB_MODE_ALPHA     ,0, 4,(dbcB_bla_1_sse2(s,d)),(dbcB_bla_2_sse2(s,d)),(dbcB_bla_4_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_flam_sse2  ,DBCB_MODE_ALPHA     ,1, 4,(dbcB_blam_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_4 (dbcB_flp_sse2   ,DBCB_MODE_PMA       ,0, 4,(dbcB_blp_1_sse2(s,d)),(dbcB_blp_2_sse2(s,d)),(dbcB_blp_4_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_flpm_sse2  ,DBCB_MODE_PMA       ,1, 4,(dbcB_blpm_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_0 (dbcB_f8_sse2    ,DBCB_MODE_COLORKEY8 ,0, 1)
DBCB_DECL_SSE2 DBCB_DEF_FN_16(dbcB_f8m_sse2   ,DBCB_MODE_COLORKEY8 ,1, 1,(dbcB_b8m_1_c(s,d,key8)),(dbcB_b8m_2_c(s,d,key8)),(dbcB_b8m_4_sse2(s,d,key8)),(dbcB_b8m_8_sse2(s,d,key8)),(dbcB_b8m_16_sse2(s,d,key8)))
DBCB_DECL_SSE2 DBCB_DEF_FN_0 (dbcB_f16_sse2   ,DBCB_MODE_COLORKEY16,0, 2)
DBCB_DECL_SSE2 DBCB_DEF_FN_8 (dbcB_f16m_sse2  ,DBCB_MODE_COLORKEY16,1, 2,(dbcB_b16m_1_c(s,d,key16)),(dbcB_b16m_2_sse2(s,d,key16)),(dbcB_b16m_4_sse2(s,d,key16)),(dbcB_b16m_8_sse2(s,d,key16)))
DBCB_DECL_SSE2 DBCB_DEF_FN_8 (dbcB_f5551_sse2 ,DBCB_MODE_5551      ,0, 2,(dbcB_b5551_1_c(s,d)),(dbcB_b5551_2_sse2(s,d)),(dbcB_b5551_4_sse2(s,d)),(dbcB_b5551_8_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_4 (dbcB_flx_sse2   ,DBCB_MODE_MUL       ,0, 4,(dbcB_blx_1_sse2(s,d)),(dbcB_blx_2_sse2(s,d)),(dbcB_blx_4_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_flxm_sse2  ,DBCB_MODE_MUL       ,1, 4,(dbcB_blxm_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_0 (dbcB_f32a_sse2  ,DBCB_MODE_ALPHATEST ,0, 4)
DBCB_DECL_SSE2 DBCB_DEF_FN_4 (dbcB_f32t_sse2  ,DBCB_MODE_ALPHATEST ,1, 4,(dbcB_b32t_1_c(s,d,key8)),(dbcB_b32t_2_sse2(s,d,key8)),(dbcB_b32t_4_sse2(s,d,key8)))
DBCB_DECL_SSE2 DBCB_DEF_FN_4 (dbcB_f32s_sse2  ,DBCB_MODE_ALPHATEST ,1, 4,(dbcB_b32s_1_c(s,d)),(dbcB_b32s_2_sse2(s,d)),(dbcB_b32s_4_sse2(s,d)))
#ifndef DBC_BLIT_NO_GAMMA
DBCB_DECL_SSE2 DBCB_DEF_FN_0 (dbcB_f32c_sse2  ,DBCB_MODE_CPYG      ,0, 4)
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_f32g_sse2  ,DBCB_MODE_CPYG      ,1, 4,(dbcB_b32g_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_fga_sse2   ,DBCB_MODE_GAMMA     ,0, 4,(dbcB_bga_1_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_fgam_sse2  ,DBCB_MODE_GAMMA     ,1, 4,(dbcB_bgam_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_fgp_sse2   ,DBCB_MODE_PMG       ,0, 4,(dbcB_bgp_1_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_fgpm_sse2  ,DBCB_MODE_PMG       ,1, 4,(dbcB_bgpm_1_sse2(s,d,color)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_fgx_sse2   ,DBCB_MODE_MUG       ,0, 4,(dbcB_bgx_1_sse2(s,d)))
DBCB_DECL_SSE2 DBCB_DEF_FN_1 (dbcB_fgxm_sse2  ,DBCB_MODE_MUG       ,1, 4,(dbcB_bgxm_1_sse2(s,d,color)))
#endif /* DBC_BLIT_NO_GAMMA */

#ifndef DBC_BLIT_NO_AVX2
DBCB_DECL_AVX2 DBCB_DEF_FN_0 (dbcB_f32_avx2   ,DBCB_MODE_COPY      ,0, 4)
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_f32m_avx2  ,DBCB_MODE_COPY      ,1, 4,(dbcB_b32m_1_avx2(s,d,color)),(dbcB_b32m_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_8 (dbcB_fla_avx2   ,DBCB_MODE_ALPHA     ,0, 4,(dbcB_bla_1_avx2(s,d)),(dbcB_bla_2_avx2(s,d)),(dbcB_bla_4_avx2(s,d)),(dbcB_bla_8_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_flam_avx2  ,DBCB_MODE_ALPHA     ,1, 4,(dbcB_blam_1_avx2(s,d,color)),(dbcB_blam_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_8 (dbcB_flp_avx2   ,DBCB_MODE_PMA       ,0, 4,(dbcB_blp_1_avx2(s,d)),(dbcB_blp_2_avx2(s,d)),(dbcB_blp_4_avx2(s,d)),(dbcB_blp_8_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_flpm_avx2  ,DBCB_MODE_PMA       ,1, 4,(dbcB_blpm_1_avx2(s,d,color)),(dbcB_blpm_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_0 (dbcB_f8_avx2    ,DBCB_MODE_COLORKEY8 ,0, 1)
DBCB_DECL_AVX2 DBCB_DEF_FN_32(dbcB_f8m_avx2   ,DBCB_MODE_COLORKEY8 ,1, 1,(dbcB_b8m_1_c(s,d,key8)),(dbcB_b8m_2_c(s,d,key8)),(dbcB_b8m_4_avx2(s,d,key8)),(dbcB_b8m_8_avx2(s,d,key8)),(dbcB_b8m_16_avx2(s,d,key8)),(dbcB_b8m_32_avx2(s,d,key8)))
DBCB_DECL_AVX2 DBCB_DEF_FN_0 (dbcB_f16_avx2   ,DBCB_MODE_COLORKEY16,0, 2)
DBCB_DECL_AVX2 DBCB_DEF_FN_16(dbcB_f16m_avx2  ,DBCB_MODE_COLORKEY16,1, 2,(dbcB_b16m_1_c(s,d,key16)),(dbcB_b16m_2_avx2(s,d,key16)),(dbcB_b16m_4_avx2(s,d,key16)),(dbcB_b16m_8_avx2(s,d,key16)),(dbcB_b16m_16_avx2(s,d,key16)))
DBCB_DECL_AVX2 DBCB_DEF_FN_16(dbcB_f5551_avx2 ,DBCB_MODE_5551      ,0, 2,(dbcB_b5551_1_c(s,d)),(dbcB_b5551_2_avx2(s,d)),(dbcB_b5551_4_avx2(s,d)),(dbcB_b5551_8_avx2(s,d)),(dbcB_b5551_16_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_8 (dbcB_flx_avx2   ,DBCB_MODE_MUL       ,0, 4,(dbcB_blx_1_avx2(s,d)),(dbcB_blx_2_avx2(s,d)),(dbcB_blx_4_avx2(s,d)),(dbcB_blx_8_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_flxm_avx2  ,DBCB_MODE_MUL       ,1, 4,(dbcB_blxm_1_avx2(s,d,color)),(dbcB_blxm_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_0 (dbcB_f32a_avx2  ,DBCB_MODE_ALPHATEST ,0, 4)
DBCB_DECL_AVX2 DBCB_DEF_FN_8 (dbcB_f32t_avx2  ,DBCB_MODE_ALPHATEST ,1, 4,(dbcB_b32t_1_c(s,d,key8)),(dbcB_b32t_2_avx2(s,d,key8)),(dbcB_b32t_4_avx2(s,d,key8)),(dbcB_b32t_8_avx2(s,d,key8)))
DBCB_DECL_AVX2 DBCB_DEF_FN_8 (dbcB_f32s_avx2  ,DBCB_MODE_ALPHATEST ,1, 4,(dbcB_b32s_1_c(s,d)),(dbcB_b32s_2_avx2(s,d)),(dbcB_b32s_4_avx2(s,d)),(dbcB_b32s_8_avx2(s,d)))
#ifndef DBC_BLIT_NO_GAMMA
DBCB_DECL_AVX2 DBCB_DEF_FN_0 (dbcB_f32c_avx2  ,DBCB_MODE_CPYG      ,0, 4)
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_f32g_avx2  ,DBCB_MODE_CPYG      ,1, 4,(dbcB_b32g_1_avx2(s,d,color)),(dbcB_b32g_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_fga_avx2   ,DBCB_MODE_GAMMA     ,0, 4,(dbcB_bga_1_avx2(s,d)),(dbcB_bga_2_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_fgam_avx2  ,DBCB_MODE_GAMMA     ,1, 4,(dbcB_bgam_1_avx2(s,d,color)),(dbcB_bgam_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_fgp_avx2   ,DBCB_MODE_PMG       ,0, 4,(dbcB_bgp_1_avx2(s,d)),(dbcB_bgp_2_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_fgpm_avx2  ,DBCB_MODE_PMG       ,1, 4,(dbcB_bgpm_1_avx2(s,d,color)),(dbcB_bgpm_2_avx2(s,d,color)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_fgx_avx2   ,DBCB_MODE_MUG       ,0, 4,(dbcB_bgx_1_avx2(s,d)),(dbcB_bgx_2_avx2(s,d)))
DBCB_DECL_AVX2 DBCB_DEF_FN_2 (dbcB_fgxm_avx2  ,DBCB_MODE_MUG       ,1, 4,(dbcB_bgxm_1_avx2(s,d,color)),(dbcB_bgxm_2_avx2(s,d,color)))
#endif /* DBC_BLIT_NO_GAMMA */
#endif /* DBC_BLIT_NO_AVX2 */
#endif /* DBCB_X86_OR_X64 */
#endif /* DBC_BLIT_NO_SIMD */

#undef DBCB_FN_SIG
#undef DBCB_FN_HEADER
#undef DBCB_FN_SWITCH0_CASE
#undef DBCB_FN_SWITCH0
#undef DBCB_FN_SWITCH_LEFT
#undef DBCB_FN_SWITCH_RIGHT
#undef DBCB_FN_SWITCH
#undef DBCB_FN_C
#undef DBCB_FN_C2
#undef DBCB_FN_C4
#undef DBCB_FN_C8
#undef DBCB_FN_C16
#undef DBCB_FN_LOOP_TOP
#undef DBCB_FN_LOOP_BOTTOM
#undef DBCB_FN_LOOP_FOR
#undef DBCB_FN_LOOP_IF
#undef DBCB_DEF_FN_0
#undef DBCB_DEF_FN_1
#undef DBCB_DEF_FN_2
#undef DBCB_DEF_FN_4
#undef DBCB_DEF_FN_8
#undef DBCB_DEF_FN_16
#undef DBCB_DEF_FN_32

/*============================================================================*/
/* Initialization */

#if !defined(DBC_BLIT_NO_SIMD) && !defined(DBC_BLIT_NO_RUNTIME_CPU_DETECTION) && defined(DBCB_X86_OR_X64) && !defined(_WIN16)

#if !defined(__GNUC__) && (!defined(_MSC_VER) || (_MSC_VER>=1400))
#include <intrin.h>
#endif

/* All Pentiums and later should have CPUID. Some later 486s have it as well. */
static int dbcB_has_cpuid(void)
{
#if defined(DBCB_X64)
    return 1; /* Always have CPUID on x64. */
#elif defined(__GNUC__)
    /* Check by trying to set the 21st bit in EFLAGS. */
    unsigned a,b;
    __asm__ __volatile__(
        "pushfl\n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0,%1\n\t"
        "xorl $0x00200000,%0\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        "popfl\n\t"
	:"=&r"(a),"=&r"(b));
    return !!((a^b)&0x00200000u);
#else
    return 1; /* Assume the CPU has CPUID. */
#endif
}

static void dbcB_cpuid(int level,int sublevel,unsigned *eax,unsigned *ebx,unsigned *ecx,unsigned *edx)
{
#if defined(__GNUC__)
    unsigned a,b,c,d;
    __asm__ __volatile__(
        "cpuid\n\t"
        :"=a"(a),"=b"(b),"=c"(c),"=d"(d)
        :"0"(level),"2"(sublevel));
    *eax=a;
    *ebx=b;
    *ecx=c;
    *edx=d;
#elif defined(_MSC_VER) && !(_MSC_VER>=1400) /* VC6 */
    int res;
    __asm {
        mov  eax,1
        cpuid
        mov  res,edx
    }
    /* Only check edx. */
    *eax=0;
    *ebx=0;
    *ecx=0;
    *edx=(unsigned)res;
#else
    int regs[4];
    __cpuidex(regs,level,sublevel);
    *eax=(unsigned)regs[0];
    *ebx=(unsigned)regs[1];
    *ecx=(unsigned)regs[2];
    *edx=(unsigned)regs[3];
#endif
}

/* Only return lower 32 bits of xgetbv. */
static unsigned dbcB_xgetbv(unsigned level)
{
    unsigned ret=0;
#if defined(__GNUC__)
    /*
        Using __builtin_ia32_xgetbv (or _xgetbv intrinsic) needs the function
        to be compiled as AVX or higher, but using xgetbv from within the
        inline assembly does not. Seems to cause no problems (as long as
        the availability of XGETBV is checked at runtime).
    */
    unsigned eax,edx;
    __asm__ __volatile__("xgetbv\n\t":"=a"(eax),"=d"(edx):"c"(level));
    ret=eax; /* Full result: eax^((unsigned long long)edx<<32). */
#elif defined(_MSC_VER) && !(_MSC_VER>=1400) /* VC6 */
    /* Pretend that xgetbv always returns 0. */
#else
    ret=(unsigned)_xgetbv(level);
#endif
    return ret;
}
#endif /* !defined(DBC_BLIT_NO_SIMD) && !defined(DBC_BLIT_NO_RUNTIME_CPU_DETECTION) && defined(DBCB_X86_OR_X64) && !defined(_WIN16) */

static int dbcB_init()
{
#if !defined(DBC_BLIT_NO_SIMD) && defined(DBCB_X86_OR_X64)
    dbcB_has_sse2=0;
#if defined(__SSE2__) || (_M_IX86_FP>=2) /* MSVC does not have __SSE2__ macro. */
    /* Always enable SSE2 if it is globally enabled. */
    dbcB_has_sse2=1;
#if !defined(DBC_BLIT_NO_AVX2) && defined(__AVX2__)
    /* Always enable AVX2 if it is globally enabled. */
    dbcB_has_avx2=1;
#endif
#endif
#if !defined(DBCB_NO_RUNTIME_CPU_DETECTION) && !defined(_WIN16)
    /*
        Note: some older OSes (Windows 95 and earlier, Linux kernel
        before something like 2.4) may not have the OS-level support for SSE
        (do not preserve the state on context switch). There seems to be no
        clean way to test it either. Some programs rely on exception handling
        (try and if it faults report as disabled); checking the OS version
        is also a possibility. This code doesn't do such checks.
        If you really need a binary that works on those OSes, consider
        checking it yourself, and using dbcb_allow_sse2_for_mode() and
        dbcb_allow_avx2_for_mode() to disable SSE2/AVX2 at runtime (or
        just disabling SIMD altogether).
    */
    if(dbcB_has_cpuid())
    {
        unsigned eax,ebx,ecx,edx;
        unsigned maxlevel;
        dbcB_cpuid(0,0,&maxlevel,&ebx,&ecx,&edx);
        if(maxlevel>0)
        {
            dbcB_cpuid(1,0,&eax,&ebx,&ecx,&edx);
            if(edx&0x04000000u) dbcB_has_sse2=1;
#if !defined(DBC_BLIT_NO_AVX2)
            if(ecx&0x18000000u) /* CPU has AVX & XGETBV. */
            {
                unsigned xcr0=dbcB_xgetbv(0);
                if((xcr0&0x6)==0x6) /* OS-level support for AVX. */
                {
                    if(maxlevel>=7)
                    {
                        dbcB_cpuid(7,0,&eax,&ebx,&ecx,&edx);
                        if(ebx&0x00000020u) dbcB_has_avx2=1;
                    }
                }
            }
#endif
        }
    }
#endif /* !defined(DBCB_NO_RUNTIME_CPU_DETECTION) && !defined(_WIN16) */
#endif /* !defined(DBC_BLIT_NO_SIMD) && defined(DBCB_X86_OR_X64) */

#ifndef DBC_BLIT_NO_GAMMA
#ifndef DBC_BLIT_GAMMA_NO_TABLES
    dbcB_populate_tables();
#endif
#endif
    return 1;
}

/*============================================================================*/
/* Blitter API */

DBCB_DEF void dbc_blit(
    int src_w,int src_h,int src_stride_in_bytes,
    const unsigned char *src_pixels,
    int dst_w,int dst_h,int dst_stride_in_bytes,
    unsigned char *dst_pixels,
    int x,int y,
    const float *color,
    int mode)
{
    int modulated=1,alpha128=0;
    dbcb_int32 x0,y0,x1,y1;
    static int initialized=
    /* Better thread-safety for C++. */
#ifndef __cplusplus
    0;
    if(!initialized) initialized=
#endif
    dbcB_init();
    (void)initialized;

    if(mode<DBCB_MODE_COPY||mode>DBCB_MODE_CPYG) return;

    if(!color) modulated=0;
    else
    {
        switch(mode)
        {
            case DBCB_MODE_COLORKEY8:  modulated=(color[0]>=0.0f&&color[0]<=255.0f); break;
            case DBCB_MODE_COLORKEY16: modulated=(color[0]>=0.0f&&color[0]<=65535.0f); break;
            case DBCB_MODE_5551:       modulated=0; break;
            case DBCB_MODE_ALPHATEST:
                modulated=(color[0]>=0.0f&&color[0]<=255.0f);
                alpha128=(color[0]>127.0f&&color[0]<=128.0f);
                break;
            case DBCB_MODE_COPY:
            case DBCB_MODE_ALPHA:
            case DBCB_MODE_PMA:
            case DBCB_MODE_GAMMA:
            case DBCB_MODE_PMG:
            case DBCB_MODE_MUL:
            case DBCB_MODE_MUG:
            case DBCB_MODE_CPYG:       modulated=!(color[0]==1.0f&&color[1]==1.0f&&color[2]==1.0f&&color[3]==1.0f); break;
        }
    }

    if(mode==DBCB_MODE_ALPHATEST&&color&&color[0]>255.0f)
        return;

    if(x<0) x0=-x; else x0=0;
    if(x+src_w>dst_w) x1=dst_w-x; else x1=src_w;
    if(y<0) y0=-y; else y0=0;
    if(y+src_h>dst_h) y1=dst_h-y; else y1=src_h;

    if(!modulated) color=0;

#define DBCB_LAUNCH(fn) fn(src_stride_in_bytes,src_pixels,dst_stride_in_bytes,dst_pixels,x0,y0,x1,y1,x,y,color)

#if !defined(DBC_BLIT_NO_SIMD) && defined(DBCB_X86_OR_X64)
#if !defined(DBC_BLIT_NO_AVX2)
    if(!dbcB_has_avx2||!(dbcb_allow_avx2_for_mode(mode,modulated))) goto no_avx2;
    if(!modulated)
    {
        switch(mode)
        {
            case DBCB_MODE_COPY:       DBCB_LAUNCH(dbcB_f32_avx2  ); break;
            case DBCB_MODE_ALPHA:      DBCB_LAUNCH(dbcB_fla_avx2  ); break;
            case DBCB_MODE_PMA:        DBCB_LAUNCH(dbcB_flp_avx2  ); break;
            case DBCB_MODE_COLORKEY8:  DBCB_LAUNCH(dbcB_f8_avx2   ); break;
            case DBCB_MODE_COLORKEY16: DBCB_LAUNCH(dbcB_f16_avx2  ); break;
            case DBCB_MODE_5551:       DBCB_LAUNCH(dbcB_f5551_avx2); break;
            case DBCB_MODE_MUL:        DBCB_LAUNCH(dbcB_flx_avx2  ); break;
            case DBCB_MODE_ALPHATEST:  DBCB_LAUNCH(dbcB_f32a_avx2 ); break;
#ifndef DBC_BLIT_NO_GAMMA
            case DBCB_MODE_CPYG:       DBCB_LAUNCH(dbcB_f32c_avx2 ); break;
            case DBCB_MODE_GAMMA:      DBCB_LAUNCH(dbcB_fga_avx2  ); break;
            case DBCB_MODE_PMG:        DBCB_LAUNCH(dbcB_fgp_avx2  ); break;
            case DBCB_MODE_MUG:        DBCB_LAUNCH(dbcB_fgx_avx2  ); break;
#endif
        }
    }
    else
    {
        switch(mode)
        {
            case DBCB_MODE_COPY:       DBCB_LAUNCH(dbcB_f32m_avx2 ); break;
            case DBCB_MODE_ALPHA:      DBCB_LAUNCH(dbcB_flam_avx2 ); break;
            case DBCB_MODE_PMA:        DBCB_LAUNCH(dbcB_flpm_avx2 ); break;
            case DBCB_MODE_COLORKEY8:  DBCB_LAUNCH(dbcB_f8m_avx2  ); break;
            case DBCB_MODE_COLORKEY16: DBCB_LAUNCH(dbcB_f16m_avx2 ); break;
            case DBCB_MODE_5551:       DBCB_LAUNCH(dbcB_f5551_avx2); break;
            case DBCB_MODE_MUL:        DBCB_LAUNCH(dbcB_flxm_avx2 ); break;
            case DBCB_MODE_ALPHATEST:
                          if(alpha128) DBCB_LAUNCH(dbcB_f32s_avx2 );
                          else         DBCB_LAUNCH(dbcB_f32t_avx2 );
                          break;
#ifndef DBC_BLIT_NO_GAMMA
            case DBCB_MODE_CPYG:       DBCB_LAUNCH(dbcB_f32g_avx2 ); break;
            case DBCB_MODE_GAMMA:      DBCB_LAUNCH(dbcB_fgam_avx2 ); break;
            case DBCB_MODE_PMG:        DBCB_LAUNCH(dbcB_fgpm_avx2 ); break;
            case DBCB_MODE_MUG:        DBCB_LAUNCH(dbcB_fgxm_avx2 ); break;
#endif
        }
    }
    return;
no_avx2:
#endif /* !defined(DBC_BLIT_NO_AVX2) */
    
    if(!dbcB_has_sse2||!(dbcb_allow_sse2_for_mode(mode,modulated))) goto no_sse2;
    if(!modulated)
    {
        switch(mode)
        {
            case DBCB_MODE_COPY:       DBCB_LAUNCH(dbcB_f32_sse2  ); break;
            case DBCB_MODE_ALPHA:      DBCB_LAUNCH(dbcB_fla_sse2  ); break;
            case DBCB_MODE_PMA:        DBCB_LAUNCH(dbcB_flp_sse2  ); break;
            case DBCB_MODE_COLORKEY8:  DBCB_LAUNCH(dbcB_f8_sse2   ); break;
            case DBCB_MODE_COLORKEY16: DBCB_LAUNCH(dbcB_f16_sse2  ); break;
            case DBCB_MODE_5551:       DBCB_LAUNCH(dbcB_f5551_sse2); break;
            case DBCB_MODE_MUL:        DBCB_LAUNCH(dbcB_flx_sse2  ); break;
            case DBCB_MODE_ALPHATEST:  DBCB_LAUNCH(dbcB_f32a_sse2 ); break;
#ifndef DBC_BLIT_NO_GAMMA
            case DBCB_MODE_CPYG:       DBCB_LAUNCH(dbcB_f32c_sse2 ); break;
            case DBCB_MODE_GAMMA:      DBCB_LAUNCH(dbcB_fga_sse2  ); break;
            case DBCB_MODE_PMG:        DBCB_LAUNCH(dbcB_fgp_sse2  ); break;
            case DBCB_MODE_MUG:        DBCB_LAUNCH(dbcB_fgx_sse2  ); break;
#endif
        }
    }
    else
    {
        switch(mode)
        {
            case DBCB_MODE_COPY:       DBCB_LAUNCH(dbcB_f32m_sse2 ); break;
            case DBCB_MODE_ALPHA:      DBCB_LAUNCH(dbcB_flam_sse2 ); break;
            case DBCB_MODE_PMA:        DBCB_LAUNCH(dbcB_flpm_sse2 ); break;
            case DBCB_MODE_COLORKEY8:  DBCB_LAUNCH(dbcB_f8m_sse2  ); break;
            case DBCB_MODE_COLORKEY16: DBCB_LAUNCH(dbcB_f16m_sse2 ); break;
            case DBCB_MODE_5551:       DBCB_LAUNCH(dbcB_f5551_sse2); break;
            case DBCB_MODE_MUL:        DBCB_LAUNCH(dbcB_flxm_sse2 ); break;
            case DBCB_MODE_ALPHATEST:
                          if(alpha128) DBCB_LAUNCH(dbcB_f32s_sse2 );
                          else         DBCB_LAUNCH(dbcB_f32t_sse2 );
                          break;
#ifndef DBC_BLIT_NO_GAMMA
            case DBCB_MODE_CPYG:       DBCB_LAUNCH(dbcB_f32g_sse2 ); break;
            case DBCB_MODE_GAMMA:      DBCB_LAUNCH(dbcB_fgam_sse2 ); break;
            case DBCB_MODE_PMG:        DBCB_LAUNCH(dbcB_fgpm_sse2 ); break;
            case DBCB_MODE_MUG:        DBCB_LAUNCH(dbcB_fgxm_sse2 ); break;
#endif
        }
    }
    return;
no_sse2:
#endif /* !defined(DBC_BLIT_NO_SIMD) && defined(DBCB_X86_OR_X64) */
    if(!modulated)
    {
        switch(mode)
        {
            case DBCB_MODE_COPY:       DBCB_LAUNCH(dbcB_f32_c  ); break;
            case DBCB_MODE_ALPHA:      DBCB_LAUNCH(dbcB_fla_c  ); break;
            case DBCB_MODE_PMA:        DBCB_LAUNCH(dbcB_flp_c  ); break;
            case DBCB_MODE_COLORKEY8:  DBCB_LAUNCH(dbcB_f8_c   ); break;
            case DBCB_MODE_COLORKEY16: DBCB_LAUNCH(dbcB_f16_c  ); break;
            case DBCB_MODE_5551:       DBCB_LAUNCH(dbcB_f5551_c); break;
            case DBCB_MODE_MUL:        DBCB_LAUNCH(dbcB_flx_c  ); break;
            case DBCB_MODE_ALPHATEST:  DBCB_LAUNCH(dbcB_f32a_c ); break;
#ifndef DBC_BLIT_NO_GAMMA
            case DBCB_MODE_CPYG:       DBCB_LAUNCH(dbcB_f32c_c ); break;
            case DBCB_MODE_GAMMA:      DBCB_LAUNCH(dbcB_fga_c  ); break;
            case DBCB_MODE_PMG:        DBCB_LAUNCH(dbcB_fgp_c  ); break;
            case DBCB_MODE_MUG:        DBCB_LAUNCH(dbcB_fgx_c  ); break;
#endif
        }
    }
    else
    {
        switch(mode)
        {
            case DBCB_MODE_COPY:       DBCB_LAUNCH(dbcB_f32m_c ); break;
            case DBCB_MODE_ALPHA:      DBCB_LAUNCH(dbcB_flam_c ); break;
            case DBCB_MODE_PMA:        DBCB_LAUNCH(dbcB_flpm_c ); break;
            case DBCB_MODE_COLORKEY8:  DBCB_LAUNCH(dbcB_f8m_c  ); break;
            case DBCB_MODE_COLORKEY16: DBCB_LAUNCH(dbcB_f16m_c ); break;
            case DBCB_MODE_5551:       DBCB_LAUNCH(dbcB_f5551_c); break;
            case DBCB_MODE_MUL:        DBCB_LAUNCH(dbcB_flxm_c ); break;
            case DBCB_MODE_ALPHATEST:
                          if(alpha128) DBCB_LAUNCH(dbcB_f32s_c );
                          else         DBCB_LAUNCH(dbcB_f32t_c );
                          break;
#ifndef DBC_BLIT_NO_GAMMA
            case DBCB_MODE_CPYG:       DBCB_LAUNCH(dbcB_f32g_c ); break;
            case DBCB_MODE_GAMMA:      DBCB_LAUNCH(dbcB_fgam_c ); break;
            case DBCB_MODE_PMG:        DBCB_LAUNCH(dbcB_fgpm_c ); break;
            case DBCB_MODE_MUG:        DBCB_LAUNCH(dbcB_fgxm_c ); break;
#endif
        }
    }

#undef DBCB_LAUNCH
}

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif /* DBC_BLIT_IMPLEMENTATION */

