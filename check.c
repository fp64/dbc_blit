/*
    Test suite for dbc_blit.h library.

    This software is in the public domain. Where that dedication is not
    recognized, you are granted a perpetual, irrevocable license to copy
    and modify this file as you see fit.
*/

#define DBC_BLIT_IMPLEMENTATION
/* #define DBC_BLIT_STATIC // */
/* #define DBC_BLIT_DATA_BIG_ENDIAN // */
/* #define DBC_BLIT_NO_GAMMA // */
/* #define DBC_BLIT_GAMMA_NO_TABLES 0 // */
/* #define DBC_BLIT_GAMMA_NO_DOUBLE // */
/* #define DBC_BLIT_NO_INT64 // */
/* #define DBC_BLIT_ENABLE_MINGW_SIMD // */
/* #define DBC_BLIT_NO_SIMD // */
/* #define DBC_BLIT_NO_RUNTIME_CPU_DETECTION // */
/* #define DBC_BLIT_NO_GCC_ASM // */
/* #define DBC_BLIT_NO_AVX2 // */
/* #define DBC_BLIT_UNROLL 0 // */
/* #define dbcb_unroll_limit_for_mode(mode,modulated) 0 // */
/* #define dbcb_allow_sse2_for_mode(mode,modulated) 0 // */
/* #define dbcb_allow_avx2_for_mode(mode,modulated) 0 // */

/*
// Example of replacement types:
#define dbcb_int8    signed   char
#define dbcb_uint8   unsigned char
#define dbcb_int16   signed   short
#define dbcb_uint16  unsigned short
#define dbcb_int32   signed   int        
#define dbcb_uint32  unsigned int
#define dbcb_int64   signed   long long
#define dbcb_uint64  unsigned long long
*/

/*
// Example of replacement load/store:
#ifdef __GNUC__
typedef unsigned short     u16 __attribute__((__may_alias__));
typedef unsigned int       u32 __attribute__((__may_alias__));
typedef unsigned long long u64 __attribute__((__may_alias__));

#define dbcb_load16(ptr)      (*(const u16*)(ptr))
#define dbcb_store16(val,ptr) (*(u16*)(ptr)=(val))
#define dbcb_load32(ptr)      (*(const u32*)(ptr))
#define dbcb_store32(val,ptr) (*(u32*)(ptr)=(val))
#define dbcb_load64(ptr)      (*(const u64*)(ptr))
#define dbcb_store64(val,ptr) (*(u64*)(ptr)=(val))
#endif
*/

#include "dbc_blit.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#elif defined(__linux__)
#include <unistd.h>
#define ISATTY isatty
extern
#ifdef __cplusplus
"C"
#endif
int fileno(FILE *stream); /* In strict standard mode <stdio.h> does not declare this. */
#define FILENO fileno
#else
#define ISATTY(x) 0
#endif

#define W 800
#define H 600

/* Intended be used from command-line (e. g. -DONLINE_COMPILER) to streamline testing for online compilers. */
#ifdef ONLINE_COMPILER
static int online_compiler=1;
#else
static int online_compiler=0;
#endif

static unsigned char buffer[W*H*4];
static unsigned char sprite[10*1024*1024*4];

#if 0
/* PCG random number generator: https://www.pcg-random.org/index.html . */
typedef struct RNG {unsigned long long state;} RNG;

static dbcb_uint32 RNG_generate(RNG *rng)
{
    unsigned long long state=rng->state*6364136223846793005ULL+1442695040888963407ULL;
    rng->state=state;
    dbcb_uint32 xorshifted=(dbcb_uint32)(((state>>18u)^state)>>27u);
    dbcb_uint32 rot=(dbcb_uint32)(state>>59u);
    return (xorshifted>>rot)|(xorshifted<<((-rot)&31));
}

static void RNG_init(RNG *rng,dbcb_uint32 seed)
{
    rng->state=(unsigned long long)seed;
    (void)RNG_generate(rng);
}
#else
/* Preferable, as this one does not require int64. */

/* Bob Jenkins's small PRNG: http://burtleburtle.net/bob/rand/smallprng.html . */
typedef struct RNG {dbcb_uint32 a,b,c,d;} RNG;

static dbcb_uint32 RNG_generate(RNG *x)
{
    #define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
    dbcb_uint32 e = x->a - rot(x->b, 27);
    x->a = x->b ^ rot(x->c, 17);
    x->b = x->c + x->d;
    x->c = x->d + e;
    x->d = e + x->a;
    return x->d;
    #undef rot
}

static void RNG_init(RNG *x,dbcb_uint32 seed)
{
    dbcb_uint32 i;
    x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
    for (i=0; i<20; ++i)
        (void)RNG_generate(x);
}
#endif

/* Bernstein's hash. */
static dbcb_uint32 djb2(const unsigned char *data,int length)
{
    int i;
    dbcb_uint32 hash=5381;
    for(i=0;i<length;++i)
        hash=(hash*33)+data[i];
    return hash;
}

#ifndef DBC_BLIT_NO_GAMMA
#if (__cplusplus<=0) && (__STDC_VERSION__ < 199901L)
/* C89 does not have long double versions in <math.h>. */
#define powl(x,y) (long double)pow((double)(x),(double)(y))
#define fabsl(x) (long double)fabs((double)(x))
#define floorl(x) (long double)floor((double)(x))
#endif

static long double linear2srgb(long double c)
{
    if(c<=0.0031308L) return 12.92L*c;
    else return 1.055L*powl(c,1.0L/2.4L)-0.055L;
}

static long double srgb2linear(long double c)
{
    if(c<=0.04045L) return c/12.92L;
    else return powl((c+0.055L)/1.055L,2.4L);
}
#endif

static int mode_pixel_size(int mode)
{
    switch(mode)
    {
        case DBCB_MODE_COPY       :
        case DBCB_MODE_ALPHA      :
        case DBCB_MODE_PMA        :
        case DBCB_MODE_MUL        :
        case DBCB_MODE_ALPHATEST  :
#ifndef DBC_BLIT_NO_GAMMA
        case DBCB_MODE_GAMMA      :
        case DBCB_MODE_PMG        :
        case DBCB_MODE_MUG        :
        case DBCB_MODE_CPYG       :
#endif
            return 4;
        case DBCB_MODE_COLORKEY8  :
            return 1;
        case DBCB_MODE_COLORKEY16 :
        case DBCB_MODE_5551       :
            return 2;
    }
    return 0;
}

#ifndef DBC_BLIT_NO_GAMMA
static void test_gamma()
{
    unsigned hash_good[4]={0x6DE20781,0x897047CE,0x041FCA12,0};
    /* Detect if stdout is using console. */
    int print_progress=(ISATTY(FILENO(stdout)));
    int i,s,d,a;
    printf("Testing accuracy of gamma-corrected blits.\n");
    printf("Note: all errors are in units of 1/255 of full range.\n");
    printf("  Accuracy of srgb2linear:\n");
    fflush(stdout);
    {
        long double L=0.0L;
        for(s=0;s<256;++s)
        {
            long double y=srgb2linear((long double)s/255.0L);
            long double z=(long double)dbcB_srgb2linear((dbcb_uint8)s);
            long double d=fabsl(z-y);
            if(d>L) L=d;
        }
        printf("Max. abs. error: %.3f.\n",(double)(255.0L*L));
    }
    fflush(stdout);
    printf("  Accuracy of linear2srgb:\n");
    fflush(stdout);
    {
        long double L=0.0L;
        for(s=0;s<8192;++s)
        {
            long double x=(long double)s/8191.0L;
            long double y=255.0L*linear2srgb((long double)x);
            long double z=(long double)dbcB_linear2srgb((dbcb_fp)x);
            long double d=fabsl(z-y);
            if(d>L) L=d;
        }
        printf("Max. abs. error: %.3f.\n",(double)(L));
    }
    fflush(stdout);
    printf("  Round-trip:\n");
    fflush(stdout);
    {
        int cnt=0;
        for(s=0;s<256;++s)
        {
            dbcb_fp x=dbcB_srgb2linear((dbcb_uint8)s);
            dbcb_uint8 t=dbcB_linear2srgb(x);
            if(t!=s)
            {
                ++cnt;
                printf("%3d -> %.17f -> %3d\n",s,(double)x,t);
            }
        }
        printf("Mismatches: %d.\n",cnt);
    }
    fflush(stdout);
    for(i=0;i<3;++i)
    {
        unsigned hash=5381;
        int q=0;
        long double L=0.0;
        long double G=1.0;
        const char *text[]={"DBCB_MODE_GAMMA","DBCB_MODE_PMG","DBCB_MODE_MUG"};
        printf("  %s:\n",text[i]);
        for(s=0;s<256;++s)
        {
            for(d=0;d<256;++d)
                for(a=0;a<256;++a)
                {
                    long double S=srgb2linear(s/255.0L);
                    long double D=srgb2linear(d/255.0L);
                    long double A=a/255.0L;
                    dbcb_fp cS=dbcB_srgb2linear((dbcb_uint8)s);
                    dbcb_fp cD=dbcB_srgb2linear((dbcb_uint8)d);
                    dbcb_fp cA=(dbcb_fp)(a)*DBCB_1div255;
                    long double r=(i==0?S*A+D*(1.0L-A):(i==1?S+D*(1.0L-A):S*D));
                    long double f,b,g;
                    dbcb_uint8 e,c;
                    dbcb_fp cC;
                    if(i==2&&a>=1) break;
                    if(r>1.0L) r=1.0L;
                    f=linear2srgb(r);
                    e=(unsigned char)(int)(f*255.0L+0.5L);
                    cC=(i==0?cS*cA+cD*(DBCB_FC(1.0)-cA):(i==1?cS+cD*(DBCB_FC(1.0)-cA):cS*cD));
                    if(cC>DBCB_FC(1.0)) cC=DBCB_FC(1.0);
                    c=(dbcb_uint8)(i==0?dbcB_cga((dbcb_uint8)s,(dbcb_uint8)d,(dbcb_uint8)a):(dbcb_uint8)(i==1?dbcB_cgp((dbcb_uint8)s,(dbcb_uint8)d,(dbcb_uint8)a):dbcB_cgx((dbcb_uint8)s,(dbcb_uint8)d)));
                    hash=(hash*33)+c; /* djb2 (Bernstein's hash). */
                    b=fabsl(f*255.0L-(long double)c);
                    g=f*255.0L;
                    g=g-floorl(g);
                    g=fabsl(g-0.5L);
                    if(g<G) G=g;
                    if(b>L) L=b;
                    if(c!=e)
                    {
#ifndef DBC_BLIT_GAMMA_NO_TABLES
                        int id=(int)(4096.0*cC);
#endif
                        if(q<10)
                        {
                           printf("\r");
                           printf("%3d %3d %3d %3d %3d ",s,d,a,c,e);
#ifndef DBC_BLIT_GAMMA_NO_TABLES
                           printf("%3d %3d ",id,dbcB_table_linear2srgb_start[id]);
#endif
                           printf("%20.17g %25.21g\n",(double)cC,(double)(r));
#ifndef DBC_BLIT_GAMMA_NO_TABLES
                           printf("%20.17g %25.21g\n",dbcB_table_linear2srgb_threshold[id],(double)(srgb2linear((c+e)/(2.0L*255.0L))));
#endif
                           fflush(stdout);
                        }
                        if(q==10) printf("\r            \r...\n");
                    }
                    if(c!=e) q++;
                }
            if(print_progress) {printf("\r%5.1f%%",100.0*(double)s/255.0); fflush(stdout);}
        }
        if(print_progress) {printf("\r         \r"); fflush(stdout);}
        printf("Mismatches: %d.\n",q);
        printf("Hash: %08X (%s).\n",hash,(hash==hash_good[i]?"ok":"DIFFERS"));
        printf("Max error: %.2f.\n",(double)(L));
        printf("Addendum. Min. distance from n+0.5: %.2e.\n",(double)(G));
        fflush(stdout);
    }
    printf("\n");
}
#endif /* DBC_BLIT_NO_GAMMA */

/*
    Some versions of GCC claim that "ISO C forbids conversion of function pointer to object pointer type".
    So, uintptr_t.
*/
static void test_op(int mode,uintptr_t op,const float *color,int pixel_bytes,int op_pixels,int rpt_pixels,int input_bytes,int hex_print,dbcb_uint32 seed,int print_mask,const char *separator,const char *comment)
{
    RNG rng;
    unsigned char s[64]={0},d[64]={0},t[64]={0};
    int rpt_bytes=rpt_pixels*pixel_bytes;
    int op_bytes=op_pixels*pixel_bytes;
    int i,k;
    void (*op_data)(const dbcb_uint8*,dbcb_uint8*)=(void (*)(const dbcb_uint8*,dbcb_uint8*))op;
    void (*op_color)(const dbcb_uint8*,dbcb_uint8*,const float*)=(void (*)(const dbcb_uint8*,dbcb_uint8*,const float*))op;
    void (*op_key8)(const dbcb_uint8*,dbcb_uint8*,dbcb_uint8)=(void (*)(const dbcb_uint8*,dbcb_uint8*,dbcb_uint8))op;
    void (*op_key16)(const dbcb_uint8*,dbcb_uint8*,dbcb_uint16)=(void (*)(const dbcb_uint8*,dbcb_uint8*,dbcb_uint16))op;
    RNG_init(&rng,seed);
    for(i=0;i<64;++i)
    {
        s[i]=(unsigned char)RNG_generate(&rng);
        d[i]=(unsigned char)RNG_generate(&rng);
        if(i>=rpt_bytes) s[i]=s[i%rpt_bytes],d[i]=d[i%rpt_bytes];
    }
    for(i=0;i<64;++i) t[i]=d[i];
    switch(mode)
    {
        case 0: for(i=0;i<input_bytes/op_bytes;++i) op_data(s+op_bytes*i,t+op_bytes*i); break;
        case 1: for(i=0;i<input_bytes/op_bytes;++i) op_color(s+op_bytes*i,t+op_bytes*i,color); break;
        case 2: for(i=0;i<input_bytes/op_bytes;++i) op_key8(s+op_bytes*i,t+op_bytes*i,(dbcb_uint8)(dbcb_uint32)color[0]); break;
        case 3: for(i=0;i<input_bytes/op_bytes;++i) op_key16(s+op_bytes*i,t+op_bytes*i,(dbcb_uint16)(dbcb_uint32)color[0]); break;
        default: return;
    }
    for(k=0;k<3;++k)
    {
        dbcb_uint8 *p[3];
        p[0]=s;p[1]=d;p[2]=t;
        if(!(print_mask&(1<<k))) continue;
        /* Note: hex is printed in mixed endian, least significant byte first, but most significant nibble first. */
        if(hex_print) for(i=0;i<input_bytes;++i) {printf("%02X",p[k][i]);if(i%pixel_bytes==pixel_bytes-1) printf("%s",separator);}
        else          for(i=0;i<input_bytes;++i) {printf("%3d",p[k][i]);printf("%s",(i%pixel_bytes==pixel_bytes-1?separator:" "));}
        /*if((1<<(k+1))>print_mask) printf("%s",comment);*/
        if(k==0) printf("%s"," []src");
        if(k==1) printf("%s"," []dst");
        if(k==2) printf("%s",comment);
        printf("\n");
        fflush(stdout);
    }
}

/* Recover some type-safety. */
#define DEF_TEST_OP(n,type)\
static void test_op_##n(int mode,void (*op) type,const float *color,int pixel_bytes,int op_pixels,int rpt_pixels,int input_bytes,int hex_print,dbcb_uint32 seed,int print_mask,const char *separator,const char *comment)\
{\
    (void)mode;\
    test_op(n,(uintptr_t)op,color,pixel_bytes,op_pixels,rpt_pixels,input_bytes,hex_print,seed,print_mask,separator,comment);\
}

DEF_TEST_OP(0,(const dbcb_uint8*,dbcb_uint8*))
DEF_TEST_OP(1,(const dbcb_uint8*,dbcb_uint8*,const float*))
DEF_TEST_OP(2,(const dbcb_uint8*,dbcb_uint8*,dbcb_uint8))
DEF_TEST_OP(3,(const dbcb_uint8*,dbcb_uint8*,dbcb_uint16))

#undef DEF_TEST_OP

/* The functions may have different calling convention, hence the wrapper. */

#define WRAPPER_0(f) static void wrapper_##f(const dbcb_uint8 *src,dbcb_uint8 *dst) {f(src,dst);}
#define WRAPPER_1(f) static void wrapper_##f(const dbcb_uint8 *src,dbcb_uint8 *dst,const float *color) {f(src,dst,color);}
#define WRAPPER_2(f) static void wrapper_##f(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint8 key8) {f(src,dst,key8);}
#define WRAPPER_3(f) static void wrapper_##f(const dbcb_uint8 *src,dbcb_uint8 *dst,dbcb_uint16 key16) {f(src,dst,key16);}

#define WRAPPER(n,f) WRAPPER_##n(f)

WRAPPER(1,dbcB_b32m_1_c)
WRAPPER(0,dbcB_bla_1_c)
WRAPPER(0,dbcB_blp_1_c)
WRAPPER(1,dbcB_blam_1_c)
WRAPPER(1,dbcB_blpm_1_c)
WRAPPER(2,dbcB_b8m_1_c)
WRAPPER(2,dbcB_b8m_2_c)
WRAPPER(2,dbcB_b8m_4_c)
WRAPPER(2,dbcB_b8m_8_c)
WRAPPER(3,dbcB_b16m_1_c)
WRAPPER(3,dbcB_b16m_2_c)
WRAPPER(3,dbcB_b16m_4_c)
WRAPPER(0,dbcB_b5551_1_c)
WRAPPER(0,dbcB_b5551_2_c)
WRAPPER(0,dbcB_b5551_4_c)
WRAPPER(2,dbcB_b32t_1_c)
WRAPPER(2,dbcB_b32t_2_c)
WRAPPER(2,dbcB_b32t_4_c)
WRAPPER(0,dbcB_b32s_1_c)
WRAPPER(0,dbcB_b32s_2_c)
WRAPPER(0,dbcB_b32s_4_c)
WRAPPER(0,dbcB_blx_1_c)
WRAPPER(1,dbcB_blxm_1_c)
#ifndef DBC_BLIT_NO_GAMMA
WRAPPER(1,dbcB_b32g_1_c)
WRAPPER(0,dbcB_bga_1_c)
WRAPPER(1,dbcB_bgam_1_c)
WRAPPER(0,dbcB_bgp_1_c)
WRAPPER(1,dbcB_bgpm_1_c)
WRAPPER(0,dbcB_bgx_1_c)
WRAPPER(1,dbcB_bgxm_1_c)
#endif /* DBC_BLIT_NO_GAMMA */
#ifndef DBC_BLIT_NO_SIMD
#ifdef DBCB_X86_OR_X64
WRAPPER(1,dbcB_b32m_1_sse2)
WRAPPER(0,dbcB_bla_1_sse2)
WRAPPER(0,dbcB_bla_2_sse2)
WRAPPER(0,dbcB_bla_4_sse2)
WRAPPER(0,dbcB_blp_1_sse2)
WRAPPER(0,dbcB_blp_2_sse2)
WRAPPER(0,dbcB_blp_4_sse2)
WRAPPER(1,dbcB_blam_1_sse2)
WRAPPER(1,dbcB_blpm_1_sse2)
WRAPPER(2,dbcB_b8m_4_sse2)
WRAPPER(2,dbcB_b8m_8_sse2)
WRAPPER(2,dbcB_b8m_16_sse2)
WRAPPER(3,dbcB_b16m_2_sse2)
WRAPPER(3,dbcB_b16m_4_sse2)
WRAPPER(3,dbcB_b16m_8_sse2)
WRAPPER(0,dbcB_b5551_2_sse2)
WRAPPER(0,dbcB_b5551_4_sse2)
WRAPPER(0,dbcB_b5551_8_sse2)
WRAPPER(2,dbcB_b32t_2_sse2)
WRAPPER(2,dbcB_b32t_4_sse2)
WRAPPER(0,dbcB_b32s_2_sse2)
WRAPPER(0,dbcB_b32s_4_sse2)
WRAPPER(0,dbcB_blx_1_sse2)
WRAPPER(0,dbcB_blx_2_sse2)
WRAPPER(0,dbcB_blx_4_sse2)
WRAPPER(1,dbcB_blxm_1_sse2)
#ifndef DBC_BLIT_NO_GAMMA
WRAPPER(1,dbcB_b32g_1_sse2)
WRAPPER(0,dbcB_bga_1_sse2)
WRAPPER(1,dbcB_bgam_1_sse2)
WRAPPER(0,dbcB_bgp_1_sse2)
WRAPPER(1,dbcB_bgpm_1_sse2)
WRAPPER(0,dbcB_bgx_1_sse2)
WRAPPER(1,dbcB_bgxm_1_sse2)
#endif /* DBC_BLIT_NO_GAMMA */
#ifndef DBC_BLIT_NO_AVX2
WRAPPER(1,dbcB_b32m_1_avx2)
WRAPPER(1,dbcB_b32m_2_avx2)
WRAPPER(0,dbcB_bla_1_avx2)
WRAPPER(0,dbcB_bla_2_avx2)
WRAPPER(0,dbcB_bla_4_avx2)
WRAPPER(0,dbcB_bla_8_avx2)
WRAPPER(0,dbcB_blp_1_avx2)
WRAPPER(0,dbcB_blp_2_avx2)
WRAPPER(0,dbcB_blp_4_avx2)
WRAPPER(0,dbcB_blp_8_avx2)
WRAPPER(1,dbcB_blam_1_avx2)
WRAPPER(1,dbcB_blam_2_avx2)
WRAPPER(1,dbcB_blpm_1_avx2)
WRAPPER(1,dbcB_blpm_2_avx2)
WRAPPER(2,dbcB_b8m_4_avx2)
WRAPPER(2,dbcB_b8m_8_avx2)
WRAPPER(2,dbcB_b8m_16_avx2)
WRAPPER(2,dbcB_b8m_32_avx2)
WRAPPER(3,dbcB_b16m_2_avx2)
WRAPPER(3,dbcB_b16m_4_avx2)
WRAPPER(3,dbcB_b16m_8_avx2)
WRAPPER(3,dbcB_b16m_16_avx2)
WRAPPER(0,dbcB_b5551_2_avx2)
WRAPPER(0,dbcB_b5551_4_avx2)
WRAPPER(0,dbcB_b5551_8_avx2)
WRAPPER(0,dbcB_b5551_16_avx2)
WRAPPER(2,dbcB_b32t_2_avx2)
WRAPPER(2,dbcB_b32t_4_avx2)
WRAPPER(2,dbcB_b32t_8_avx2)
WRAPPER(0,dbcB_b32s_2_avx2)
WRAPPER(0,dbcB_b32s_4_avx2)
WRAPPER(0,dbcB_b32s_8_avx2)
WRAPPER(0,dbcB_blx_1_avx2)
WRAPPER(0,dbcB_blx_2_avx2)
WRAPPER(0,dbcB_blx_4_avx2)
WRAPPER(0,dbcB_blx_8_avx2)
WRAPPER(1,dbcB_blxm_1_avx2)
WRAPPER(1,dbcB_blxm_2_avx2)
#ifndef DBC_BLIT_NO_GAMMA
WRAPPER(1,dbcB_b32g_1_avx2)
WRAPPER(1,dbcB_b32g_2_avx2)
WRAPPER(0,dbcB_bga_1_avx2)
WRAPPER(0,dbcB_bga_2_avx2)
WRAPPER(1,dbcB_bgam_1_avx2)
WRAPPER(1,dbcB_bgam_2_avx2)
WRAPPER(0,dbcB_bgp_1_avx2)
WRAPPER(0,dbcB_bgp_2_avx2)
WRAPPER(1,dbcB_bgpm_1_avx2)
WRAPPER(1,dbcB_bgpm_2_avx2)
WRAPPER(0,dbcB_bgx_1_avx2)
WRAPPER(0,dbcB_bgx_2_avx2)
WRAPPER(1,dbcB_bgxm_1_avx2)
WRAPPER(1,dbcB_bgxm_2_avx2)
#endif /* DBC_BLIT_NO_GAMMA */
#endif /* DBC_BLIT_NO_AVX2 */
#endif /* DBCB_X86_OR_X64 */
#endif /* DBC_BLIT_NO_SIMD */

#undef WRAPPER_0
#undef WRAPPER_1
#undef WRAPPER_2
#undef WRAPPER_3
#undef WRAPPER

#define TEST_OP(a,b,c,d,e,f,g,h,i,j,k,l) test_op_##a(a,wrapper_##b,c,d,e,f,g,h,i,j,k,l)

#ifndef DBC_BLIT_NO_INT64
#define IF_INT64(x) (x)
#else
#define IF_INT64(x) ((void)0)
#endif

#ifndef DBC_BLIT_NO_SIMD
#define IF_SSE2(x) (dbcB_has_sse2?(x):((void)0))
#else
#define IF_SSE2(x) ((void)0)
#endif

#if !defined(DBC_BLIT_NO_SIMD) && !defined(DBC_BLIT_NO_AVX2)
#define IF_AVX2(x) (dbcB_has_avx2?(x):((void)0))
#else
#define IF_AVX2(x) ((void)0)
#endif

static void test_ops()
{
    float color[4]={0.5f,0.5f,0.25f,1.0f};
#ifdef DBC_BLIT_DATA_BIG_ENDIAN
    float key[4]={(float)0x44A1u,0.0f,0.0f,0.0f};
#else
    float key[4]={(float)0xA144u,0.0f,0.0f,0.0f};
#endif
    printf("Testing operations.\n");
               printf("dbcB_b32m_*:\n");
             TEST_OP(1,dbcB_b32m_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_b32m_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_b32m_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_b32m_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bla_*:\n");
             TEST_OP(0,dbcB_bla_1_c      ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_bla_1_sse2   ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_bla_2_sse2   ,color,4, 2,64,32,1,0,4,"|"," 2_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_bla_4_sse2   ,color,4, 4,64,32,1,0,4,"|"," 4_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_bla_1_avx2   ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_bla_2_avx2   ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_bla_4_avx2   ,color,4, 4,64,32,1,0,4,"|"," 4_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_bla_8_avx2   ,color,4, 8,64,32,1,0,4,"|"," 8_avx2")));
               printf("dbcB_blp_*:\n");
             TEST_OP(0,dbcB_blp_1_c      ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_blp_1_sse2   ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_blp_2_sse2   ,color,4, 2,64,32,1,0,4,"|"," 2_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_blp_4_sse2   ,color,4, 4,64,32,1,0,4,"|"," 4_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_blp_1_avx2   ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_blp_2_avx2   ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_blp_4_avx2   ,color,4, 4,64,32,1,0,4,"|"," 4_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_blp_8_avx2   ,color,4, 8,64,32,1,0,4,"|"," 8_avx2")));
               printf("dbcB_blam_*:\n");
             TEST_OP(1,dbcB_blam_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_blam_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_blam_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_blam_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_blpm_*:\n");
             TEST_OP(1,dbcB_blpm_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_blpm_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_blpm_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_blpm_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_b8m_*:\n");
             TEST_OP(2,dbcB_b8m_1_c      ,key  ,1, 1, 4,32,1,0,7,""," 1_c");
             TEST_OP(2,dbcB_b8m_2_c      ,key  ,1, 2, 4,32,1,0,4,""," 2_c");
             TEST_OP(2,dbcB_b8m_4_c      ,key  ,1, 4, 4,32,1,0,4,""," 4_c");
             TEST_OP(2,dbcB_b8m_8_c      ,key  ,1, 8, 4,32,1,0,4,""," 8_c");
    IF_SSE2((TEST_OP(2,dbcB_b8m_4_sse2   ,key  ,1, 4, 4,32,1,0,4,""," 4_sse2")));
    IF_SSE2((TEST_OP(2,dbcB_b8m_8_sse2   ,key  ,1, 8, 4,32,1,0,4,""," 8_sse2")));
    IF_SSE2((TEST_OP(2,dbcB_b8m_16_sse2  ,key  ,1,16, 4,32,1,0,4,""," 16_sse2")));
    IF_AVX2((TEST_OP(2,dbcB_b8m_4_avx2   ,key  ,1, 4, 4,32,1,0,4,""," 4_avx2")));
    IF_AVX2((TEST_OP(2,dbcB_b8m_8_avx2   ,key  ,1, 8, 4,32,1,0,4,""," 8_avx2")));
    IF_AVX2((TEST_OP(2,dbcB_b8m_16_avx2  ,key  ,1,16, 4,32,1,0,4,""," 16_avx2")));
    IF_AVX2((TEST_OP(2,dbcB_b8m_32_avx2  ,key  ,1,32, 4,32,1,0,4,""," 32_avx2")));
               printf("dbcB_b16m_*:\n");
             TEST_OP(3,dbcB_b16m_1_c     ,key  ,2, 1, 4,32,1,0,7,""," 1_c");
             TEST_OP(3,dbcB_b16m_2_c     ,key  ,2, 2, 4,32,1,0,4,""," 2_c");
             TEST_OP(3,dbcB_b16m_4_c     ,key  ,2, 4, 4,32,1,0,4,""," 4_c");
    IF_SSE2((TEST_OP(3,dbcB_b16m_2_sse2  ,key  ,2, 2, 4,32,1,0,4,""," 2_sse2")));
    IF_SSE2((TEST_OP(3,dbcB_b16m_4_sse2  ,key  ,2, 4, 4,32,1,0,4,""," 4_sse2")));
    IF_SSE2((TEST_OP(3,dbcB_b16m_8_sse2  ,key  ,2, 8, 4,32,1,0,4,""," 8_sse2")));
    IF_AVX2((TEST_OP(3,dbcB_b16m_2_avx2  ,key  ,2, 2, 4,32,1,0,4,""," 2_avx2")));
    IF_AVX2((TEST_OP(3,dbcB_b16m_4_avx2  ,key  ,2, 4, 4,32,1,0,4,""," 4_avx2")));
    IF_AVX2((TEST_OP(3,dbcB_b16m_8_avx2  ,key  ,2, 8, 4,32,1,0,4,""," 8_avx2")));
    IF_AVX2((TEST_OP(3,dbcB_b16m_16_avx2 ,key  ,2,16, 4,32,1,0,4,""," 16_avx2")));
               printf("dbcB_b5551_*:\n");
             TEST_OP(0,dbcB_b5551_1_c    ,key  ,2, 1, 4,32,1,0,7,""," 1_c");
             TEST_OP(0,dbcB_b5551_2_c    ,key  ,2, 2, 4,32,1,0,4,""," 2_c");
             TEST_OP(0,dbcB_b5551_4_c    ,key  ,2, 4, 4,32,1,0,4,""," 4_c");
    IF_SSE2((TEST_OP(0,dbcB_b5551_2_sse2 ,key  ,2, 2, 4,32,1,0,4,""," 2_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_b5551_4_sse2 ,key  ,2, 4, 4,32,1,0,4,""," 4_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_b5551_8_sse2 ,key  ,2, 8, 4,32,1,0,4,""," 8_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_b5551_2_avx2 ,key  ,2, 2, 4,32,1,0,4,""," 2_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_b5551_4_avx2 ,key  ,2, 4, 4,32,1,0,4,""," 4_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_b5551_8_avx2 ,key  ,2, 8, 4,32,1,0,4,""," 8_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_b5551_16_avx2,key  ,2,16, 4,32,1,0,4,""," 16_avx2")));
               printf("dbcB_blx_*:\n");
             TEST_OP(0,dbcB_blx_1_c      ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_blx_1_sse2   ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_blx_2_sse2   ,color,4, 2,64,32,1,0,4,"|"," 2_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_blx_4_sse2   ,color,4, 4,64,32,1,0,4,"|"," 4_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_blx_1_avx2   ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_blx_2_avx2   ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_blx_4_avx2   ,color,4, 4,64,32,1,0,4,"|"," 4_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_blx_8_avx2   ,color,4, 8,64,32,1,0,4,"|"," 8_avx2")));
               printf("dbcB_blxm_*:\n");
             TEST_OP(1,dbcB_blxm_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_blxm_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_blxm_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_blxm_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_b32t_*:\n");
             TEST_OP(2,dbcB_b32t_1_c     ,key  ,4, 1,64,32,1,0,7,"|"," 1_c");
             TEST_OP(2,dbcB_b32t_2_c     ,key  ,4, 2,64,32,1,0,4,"|"," 1_c");
             TEST_OP(2,dbcB_b32t_4_c     ,key  ,4, 4,64,32,1,0,4,"|"," 1_c");
    IF_SSE2((TEST_OP(2,dbcB_b32t_2_sse2  ,key  ,4, 2,64,32,1,0,4,"|"," 2_sse2")));
    IF_SSE2((TEST_OP(2,dbcB_b32t_4_sse2  ,key  ,4, 4,64,32,1,0,4,"|"," 4_sse2")));
    IF_AVX2((TEST_OP(2,dbcB_b32t_2_avx2  ,key  ,4, 2,64,32,1,0,4,"|"," 2_avx2")));
    IF_AVX2((TEST_OP(2,dbcB_b32t_4_avx2  ,key  ,4, 4,64,32,1,0,4,"|"," 4_avx2")));
    IF_AVX2((TEST_OP(2,dbcB_b32t_8_avx2  ,key  ,4, 8,64,32,1,0,4,"|"," 8_avx2")));
               printf("dbcB_b32s_*:\n");
             TEST_OP(0,dbcB_b32s_1_c     ,key  ,4, 1,64,32,1,0,7,"|"," 1_c");
             TEST_OP(0,dbcB_b32s_2_c     ,key  ,4, 2,64,32,1,0,4,"|"," 1_c");
             TEST_OP(0,dbcB_b32s_4_c     ,key  ,4, 4,64,32,1,0,4,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_b32s_2_sse2  ,key  ,4, 2,64,32,1,0,4,"|"," 2_sse2")));
    IF_SSE2((TEST_OP(0,dbcB_b32s_4_sse2  ,key  ,4, 4,64,32,1,0,4,"|"," 4_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_b32s_2_avx2  ,key  ,4, 2,64,32,1,0,4,"|"," 2_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_b32s_4_avx2  ,key  ,4, 4,64,32,1,0,4,"|"," 4_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_b32s_8_avx2  ,key  ,4, 8,64,32,1,0,4,"|"," 8_avx2")));
#ifndef DBC_BLIT_NO_GAMMA
               printf("dbcB_b32g_*:\n");
             TEST_OP(1,dbcB_b32g_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_b32g_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_b32g_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_b32g_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bga_*:\n");
             TEST_OP(0,dbcB_bga_1_c      ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_bga_1_sse2   ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_bga_1_avx2   ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_bga_2_avx2   ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bgam_*:\n");
             TEST_OP(1,dbcB_bgam_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_bgam_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_bgam_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_bgam_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bgp_*:\n");
             TEST_OP(0,dbcB_bgp_1_c      ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_bgp_1_sse2   ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_bgp_1_avx2   ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_bgp_2_avx2   ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bgpm_*:\n");
             TEST_OP(1,dbcB_bgpm_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_bgpm_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_bgpm_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_bgpm_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bgx_*:\n");
             TEST_OP(0,dbcB_bgx_1_c      ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(0,dbcB_bgx_1_sse2   ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(0,dbcB_bgx_1_avx2   ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(0,dbcB_bgx_2_avx2   ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
               printf("dbcB_bgxm_*:\n");
             TEST_OP(1,dbcB_bgxm_1_c     ,color,4, 1,64,32,1,0,7,"|"," 1_c");
    IF_SSE2((TEST_OP(1,dbcB_bgxm_1_sse2  ,color,4, 1,64,32,1,0,4,"|"," 1_sse2")));
    IF_AVX2((TEST_OP(1,dbcB_bgxm_1_avx2  ,color,4, 1,64,32,1,0,4,"|"," 1_avx2")));
    IF_AVX2((TEST_OP(1,dbcB_bgxm_2_avx2  ,color,4, 2,64,32,1,0,4,"|"," 2_avx2")));
#endif /* DBC_BLIT_NO_GAMMA */
    printf("\n");
}

static void gen_sprite(unsigned char *dst,int T,int mode,int key,dbcb_uint32 seed)
{
    RNG rng;
    int pixel_size=mode_pixel_size(mode);
    dbcb_int32 x,y;
    RNG_init(&rng,seed);
    for(y=0;y<T;++y)
        for(x=0;x<T;++x)
        {
            int id=(y*T+x)*pixel_size;
            dbcb_int32 cx=(2*x+1-T);
            dbcb_int32 cy=(2*y+1-T);
            dbcb_int32 r2=T*T-(cx*cx+cy*cy);
            dbcb_int32 w=2*256*r2/(T*T);
            switch(mode)
            {
                case DBCB_MODE_COLORKEY8:
                {
                    if((dbcb_int32)(RNG_generate(&rng)%255)<w) dst[id]=(unsigned char)RNG_generate(&rng);
                    else                                       dst[id]=(unsigned char)key;
                    break;
                }
                case DBCB_MODE_COLORKEY16:
                {
                    dbcb_uint16 c;
                    if((dbcb_int32)(RNG_generate(&rng)%255)<w) c=(dbcb_uint16)RNG_generate(&rng);
                    else                                       c=(dbcb_uint16)key;
                    dbcb_store16(c,dst+id);
                    break;
                }
                case DBCB_MODE_5551:
                {
                    dbcb_uint16 c=(dbcb_uint16)RNG_generate(&rng);
                    if((dbcb_int32)(RNG_generate(&rng)%255)<w) c=(dbcb_uint16)(c|0x8000u);
                    else                                       c=(dbcb_uint16)(c&0x7FFFu);
                    dbcb_store16(c,dst+id);
                    break;
                }
                case DBCB_MODE_COPY:
                case DBCB_MODE_ALPHA:
                case DBCB_MODE_PMA:
                case DBCB_MODE_MUL:
                case DBCB_MODE_GAMMA:
                case DBCB_MODE_PMG:
                case DBCB_MODE_MUG:
                case DBCB_MODE_ALPHATEST:
                case DBCB_MODE_CPYG:
                {
                    dbcb_int32 a=w+(dbcb_int32)(RNG_generate(&rng)%17-8);
                    if(a<0) a=0;
                    if(a>255) a=255;
                    dst[id  ]=(unsigned char)RNG_generate(&rng);
                    dst[id+1]=(unsigned char)RNG_generate(&rng);
                    dst[id+2]=(unsigned char)RNG_generate(&rng);
                    dst[id+3]=(unsigned char)a;
                    if((mode==DBCB_MODE_PMA||mode==DBCB_MODE_PMG)&&(RNG_generate(&rng)&255))
                    {
                        dst[id+0]=(unsigned char)dbcB_div255_round((dbcb_uint32)(dst[id+0]*a));
                        dst[id+1]=(unsigned char)dbcB_div255_round((dbcb_uint32)(dst[id+1]*a));
                        dst[id+2]=(unsigned char)dbcB_div255_round((dbcb_uint32)(dst[id+2]*a));
                    }
                    if((mode==DBCB_MODE_MUL||mode==DBCB_MODE_MUG))
                    {
                        dst[id+3]=(unsigned char)RNG_generate(&rng);
                        if(RNG_generate(&rng)&255)
                        {
                            dst[id+0]=(unsigned char)(255-dbcB_div255_round((dbcb_uint32)(dst[id+0]*a)));
                            dst[id+1]=(unsigned char)(255-dbcB_div255_round((dbcb_uint32)(dst[id+1]*a)));
                            dst[id+2]=(unsigned char)(255-dbcB_div255_round((dbcb_uint32)(dst[id+2]*a)));
                            dst[id+3]=(unsigned char)(255-dbcB_div255_round((dbcb_uint32)(dst[id+3]*a)));
                        }
                    }
#if defined(DBC_BLIT_DATA_BIG_ENDIAN)
                    dbcb_store32(dbcB_4x8to32(dst[id+0],dst[id+1],dst[id+2],dst[id+3]),dst+id);
#endif
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(0x00000000u,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(0xFFFFFFFFu,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(0xFF000000u,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(0x00FFFFFFu,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(RNG_generate(&rng),dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(RNG_generate(&rng)&0xFF000000u,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(RNG_generate(&rng)&0x00FFFFFFu,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(RNG_generate(&rng)|0xFF000000u,dst+id);
                    if(!(RNG_generate(&rng)&511)) dbcb_store32(RNG_generate(&rng)|0x00FFFFFFu,dst+id);
                    break;
                }
            }
        }
}

static double test_performance(int N,int sprite_size,int test,int mode,int g,dbcb_uint32 seed)
{
    RNG rng;
    int T=sprite_size;
    int x0=0,y0=0,w=W,h=H;
    int num_sprites=1;
    int pixel_size=mode_pixel_size(mode);
    int F=0;
    double t;
    float color[4]={1.0f,0.5f,0.25f,0.5f};
    int j,k;
    RNG_init(&rng,seed);
    RNG_generate(&rng);
    switch(test)
    {
       case 0: w=1; h=1; break;
       case 1: w=W-T; h=H-T; break;
       case 2: x0=-T; y0=-T; w=W+T; h=H+T; break;
       case 3: w=W-T; h=H-T; num_sprites=200; break;

       return -1.0;
    }
    if(!(pixel_size>0)) return -1.0;
    for(k=0;k<num_sprites;++k)
    {
        gen_sprite(sprite+T*T*pixel_size*k,sprite_size,mode,(mode==DBCB_MODE_5551?-1:1),seed+(dbcb_uint32)k);
    }
    memset(buffer,0x89u,(size_t)(W*H*pixel_size));
    if(mode==DBCB_MODE_ALPHATEST) color[0]=73.0f;
    if(mode==DBCB_MODE_MUL||mode==DBCB_MODE_MUG) F=(test==0?4:4*(W*H/(T*T+1)+1));
    t=(double)clock();
    for(j=0;j<N;++j)
    {
        dbcb_int32 x,y,s;
        x=(dbcb_int32)(RNG_generate(&rng)%(dbcb_uint32)w)+x0;
        y=(dbcb_int32)(RNG_generate(&rng)%(dbcb_uint32)h)+y0;
        s=(dbcb_int32)(RNG_generate(&rng)%(dbcb_uint32)num_sprites);
        dbc_blit(
            T,T,pixel_size*T,sprite+s*(T*T)*pixel_size,
            W,H,pixel_size*W,buffer,
            x,y,
            (g?color:0),
            mode);
        if((mode==DBCB_MODE_MUL||mode==DBCB_MODE_MUG)&&j%F==F-1&&j<N-F)
        {
            /*
                Multiplication turns buffer to black, which than makes pixel blit
                early-out, producing erroneously high speed, so we reset it once
                in a while. Time added by this should not affect the result much.
            */
            if(test==0)
                dbc_blit(
                    T,T,pixel_size*T,sprite+s*(T*T)*pixel_size,
                    W,H,pixel_size*W,buffer,
                    x,y,
                    0,
                    DBCB_MODE_COPY);
            else memset(buffer,0x89u,(size_t)(W*H*pixel_size));
        }
    }
    t=(double)clock()-t;
    t/=CLOCKS_PER_SEC;
    t/=(double)T*(double)T;
    return 1.0e+9*t/(double)N;
}

static void test_perf(int N0,int N1,int sprite_size,int mode,int t,int p)
{
    int i;
    for(i=0;i<2;++i)
    {
        int N=(i?N1:N0);
        if(t&(1<<i))
        {
            double g=0.0;
            g=test_performance(N,sprite_size,0,mode,i,1);
            printf("%6.2f|",(p?1.0/g:g));
            fflush(stdout);
            g=test_performance(N,sprite_size,3,mode,i,1);
            printf("%6.2f|",(p?1.0/g:g));
            fflush(stdout);
        }
    }
    printf("\n");
}

static void write_screen(const unsigned char *buffer,int w,int h,int pixel_size,const char *filename)
{
    FILE *f;
    int x,y;
    if(online_compiler) return;
    f=fopen(filename,"wb");
    if(!f) return;
    /* Netpbm image format: https://en.wikipedia.org/wiki/Netpbm . */
    fprintf(f,"P6 %d %d %d ",w,h,255);
    for(y=0;y<h;++y)
        for(x=0;x<w;++x)
        {
            int id=(y*W+x)*pixel_size;
            int r,g,b;
            r=0;
            g=0;
            b=0;
            switch(pixel_size)
            {
                case 1: {dbcb_uint8 c=buffer[id];r=((c&7)*255+3)/7;g=(((c>>3)&7)*255+3)/7;b=((c>>6)*255+1)/3;break;}
                case 2: {dbcb_uint16 c=dbcb_load16(buffer+id);r=((c&31)*255+15)/31;g=(((c>>5)&31)*255+15)/31;b=(((c>>10)&31)*255+15)/31;break;}
                case 4: {dbcb_uint32 c=dbcb_load32(buffer+id);r=(int)(c&255);g=(int)((c>>8)&255);b=(int)((c>>16)&255);break;}
            }
            fprintf(f,"%c%c%c",r,g,b);
        }
    fclose(f);
}

static dbcb_uint32 test_render(int mode,int g)
{
    int pixel_size=0;
    pixel_size=mode_pixel_size(mode);
    if(pixel_size==0) return 0xBAD11111u;
    test_performance((W*H/(63*63))*2,63,2,mode,g,1);
    if(0)
    {
        char name[256]={0};
        sprintf(name,"image_%02d_%d.ppm",mode,g);
        write_screen(buffer,W,H,pixel_size,name);
    }
    return djb2(buffer,W*H*pixel_size);
}

static void test_modes()
{
#define TEST_RENDER(mode) do{dbcb_uint32 h0,h1;h0=test_render(mode,0);h1=test_render(mode,1);printf("%-20s|",#mode);printf(" %08X (%-7s)|",h0,(h0==ref[mode][0]?"ok":"DIFFERS"));printf(" %08X (%-7s)|",h1,(h1==ref[mode][1]?"ok":"DIFFERS"));printf("\n");fflush(stdout);}while(0)

    dbcb_uint32 ref[][2]={
#ifdef DBC_BLIT_DATA_BIG_ENDIAN
/* DBCB_MODE_COPY      */ {0xD0CEA91Au , 0x24434FE0u},
/* DBCB_MODE_ALPHA     */ {0xDF302328u , 0xE07C354Cu},
/* DBCB_MODE_PMA       */ {0x619D222Cu , 0xBDEDFE33u},
/* DBCB_MODE_GAMMA     */ {0xE6D78F2Au , 0x2089596Eu},
/* DBCB_MODE_PMG       */ {0x23B19F1Au , 0x9109CD75u},
/* DBCB_MODE_COLORKEY8 */ {0x2EF729A2u , 0xDB9AF6FFu},
/* DBCB_MODE_COLORKEY16*/ {0x6396C8BDu , 0x02A32284u},
/* DBCB_MODE_5551      */ {0x49F2057Au , 0x49F2057Au},
/* DBCB_MODE_MUL       */ {0xB595469Fu , 0xFE9A12EBu},
/* DBCB_MODE_MUG       */ {0xB1EF3DF7u , 0x96A2BE39u},
/* DBCB_MODE_ALPHATEST */ {0xD0CEA91Au , 0x693FCC76u},
/* DBCB_MODE_CPYG      */ {0xD0CEA91Au , 0x40689476u}
#else
/* DBCB_MODE_COPY      */ {0x055CA7BAu , 0x7566D2C0u},
/* DBCB_MODE_ALPHA     */ {0xADBF7A88u , 0xD409C9ACu},
/* DBCB_MODE_PMA       */ {0x520E53CCu , 0x78B1A433u},
/* DBCB_MODE_GAMMA     */ {0x8E78D6CAu , 0x956B66CEu},
/* DBCB_MODE_PMG       */ {0xA8030CBAu , 0xA1B68EF5u},
/* DBCB_MODE_COLORKEY8 */ {0x2EF729A2u , 0xDB9AF6FFu},
/* DBCB_MODE_COLORKEY16*/ {0x3AA50CFDu , 0x4C2472A4u},
/* DBCB_MODE_5551      */ {0x384854DAu , 0x384854DAu},
/* DBCB_MODE_MUL       */ {0x14A6ED5Fu , 0x75ABC32Bu},
/* DBCB_MODE_MUG       */ {0xB0769277u , 0x2060DBB9u},
/* DBCB_MODE_ALPHATEST */ {0x055CA7BAu , 0xAA5DF296u},
/* DBCB_MODE_CPYG      */ {0x055CA7BAu , 0xDAC59256u}
#endif
    };

    printf("Testing modes.\n");

    TEST_RENDER(DBCB_MODE_COPY       );
    TEST_RENDER(DBCB_MODE_ALPHA      );
    TEST_RENDER(DBCB_MODE_PMA        );
#ifndef DBC_BLIT_NO_GAMMA
    TEST_RENDER(DBCB_MODE_GAMMA      );
    TEST_RENDER(DBCB_MODE_PMG        );
#endif
    TEST_RENDER(DBCB_MODE_COLORKEY8  );
    TEST_RENDER(DBCB_MODE_COLORKEY16 );
    TEST_RENDER(DBCB_MODE_5551       );
    TEST_RENDER(DBCB_MODE_MUL        );
#ifndef DBC_BLIT_NO_GAMMA
    TEST_RENDER(DBCB_MODE_MUG        );
#endif
    TEST_RENDER(DBCB_MODE_ALPHATEST  );
#ifndef DBC_BLIT_NO_GAMMA
    TEST_RENDER(DBCB_MODE_CPYG       );
#endif

    printf("\n");
}

static void test_speed()
{
#define TEST(N0,N1,size,mode,t,p) do{printf("%-20s|%4d|",#mode,size); test_perf(N0,N1,size,mode,t,p);} while(0)

    int g=3;
    int size=64;
    int p=0;
    int m=(online_compiler?50:100);
#if defined(ONLINE_COMPILER) && defined(DBC_BLIT_NO_SIMD)
    m/=2;
#endif
    printf("Testing performance.\n");
    if(p) printf("Timings are in gigapixels/second.\n");
    else  printf("Timings are in ns/pixel.\n");
    printf("Sprites are %dx%d, and contain a combination of transparent,\n",size,size);
    printf("semitransparent (where applicable), and opaque pixels.\n");
    printf("Column 'Fill' estimates pure fillrate: blit(0,0,sprites[0]).\n");
    printf("Column 'Rand' estimates random access: blit(rnd(W),rnd(H),sprites[rnd(N)]).\n");
    printf("                    |    |Non-modulated|  Modulated  |\n");
    printf("                    |    |------+------+------+------|\n");
    printf("                    |Size| Fill | Rand | Fill | Rand |\n");
    printf("--------------------+----+------+------+------+------|\n");
    fflush(stdout);
    /* Warm-up. */
    (void)test_performance(50*m,size,3,DBCB_MODE_COPY,0,1);
    TEST(  500*m,   50*m,size,DBCB_MODE_COPY        ,g,p);
    TEST(  100*m,   50*m,size,DBCB_MODE_ALPHA       ,g,p);
    TEST(  100*m,   50*m,size,DBCB_MODE_PMA         ,g,p);
#ifndef DBC_BLIT_NO_GAMMA
    TEST(   10*m,   10*m,size,DBCB_MODE_GAMMA       ,g,p);
    TEST(   10*m,   10*m,size,DBCB_MODE_PMG         ,g,p);
#endif
    TEST( 1000*m, 1000*m,size,DBCB_MODE_COLORKEY8   ,g,p);
    TEST(  500*m,  500*m,size,DBCB_MODE_COLORKEY16  ,g,p);
    TEST(  500*m,  500*m,size,DBCB_MODE_5551        ,g,p);
    TEST(  100*m,   50*m,size,DBCB_MODE_MUL         ,g,p);
#ifndef DBC_BLIT_NO_GAMMA
    TEST(   20*m,   10*m,size,DBCB_MODE_MUG         ,g,p);
#endif
    TEST(  500*m,  200*m,size,DBCB_MODE_ALPHATEST   ,g,p);
#ifndef DBC_BLIT_NO_GAMMA
    TEST(  500*m,   20*m,size,DBCB_MODE_CPYG        ,g,p);
#endif
    printf("\n");
}

int main()
{
    printf("Testing dbc_blit.h.\n");
    fflush(stdout);
    printf("Compiler:\n");
#ifdef __cplusplus
    printf("  Compiled as C++.\n");
#if (__cplusplus+0)>0
    printf("  __cplusplus                       is set to %ld.\n",(long)(__cplusplus));
#endif
#else
    printf("  Compiled as C.\n");
#endif
#ifdef __STDC__
    printf("  __STDC__                          is set to %d.\n",(__STDC__ + 0));
#endif
#if defined(__STDC_VERSION__)
    printf("  __STDC_VERSION__                  is set to %ld.\n",(long)(__STDC_VERSION__ + 0));
#endif
#if defined(__STDC_HOSTED__)
    printf("  __STDC_HOSTED__                   is set to %d.\n",(__STDC_HOSTED__ + 0));
#endif
#if defined(__MINGW32__)
    printf("  __MINGW32__                       is set.\n");
#endif
#if defined(__MINGW64__)
    printf("  __MINGW64__                       is set.\n");
#endif
#if (defined(__MINGW32__)||defined(__MINGW64__))&&!defined(DBCB_X64)
    printf("Warning: using SIMD with MinGW may be problematic on 32-bit x86.\n");
    printf("See documentation for DBC_BLIT_ENABLE_MINGW_SIMD.\n");
#endif
    fflush(stdout);
    printf("Environment:\n");
    /*
        Note: we rely on size of char being 8 bit.
        dbc_blit.h should already check it.
    */
    printf("  Architecture: %d bit.\n",(int)(sizeof(void*)*8));
#ifdef DBCB_SYSTEM_LITTLE_ENDIAN
    printf("  System detected as little-endian at compile-time.\n");
#endif
#ifdef DBCB_SYSTEM_BIG_ENDIAN
    printf("  System detected as big-endian at compile-time.\n");
#endif
#if defined(DBCB_SYSTEM_LITTLE_ENDIAN) && defined(DBCB_SYSTEM_BIG_ENDIAN)
    printf("    ...this doesn't sound right.\n");
#endif
#ifdef __BYTE_ORDER__
    printf("  __BYTE_ORDER__                    is %ld.\n",(long)(__BYTE_ORDER__));
#endif
#ifdef __FLOAT_WORD_ORDER__
    printf("  __FLOAT_WORD_ORDER__              is %ld.\n",(long)(__FLOAT_WORD_ORDER__));
#endif
#if defined(__STDC_IEC_559__)
    printf("  __STDC_IEC_559__                  is set to %d.\n",(__STDC_IEC_559__ + 0));
#elif defined(__GCC_IEC_559)
    printf("  __GCC_IEC_559                     is set to %d.\n",(__GCC_IEC_559 + 0));
#else
    printf("  __STDC_IEC_559__                  is not set.\n");
#endif
#if defined(FLT_EVAL_METHOD)
    printf("  FLT_EVAL_METHOD                   is set to %d.\n",(FLT_EVAL_METHOD + 0));
#elif defined(__FLT_EVAL_METHOD__)
    printf("  __FLT_EVAL_METHOD__               is set to %d.\n",(__FLT_EVAL_METHOD__ + 0));
#else
    printf("  FLT_EVAL_METHOD                   is not set.\n");
#endif
#ifdef __FAST_MATH__
    printf("  __FAST_MATH__                     is set.\n");
#endif
#ifdef FP_FAST_FMAF
    printf("  FP_FAST_FMAF                      is set.\n");
#endif
#ifdef FP_FAST_FMA
    printf("  FP_FAST_FMA                       is set.\n");
#endif
#ifdef FP_FAST_FMAL
    printf("  FP_FAST_FMAL                      is set.\n");
#endif
#ifdef __SSE2__
    printf("  __SSE2__                          is set.\n");
#endif
#ifdef __AVX2__
    printf("  __AVX2__                          is set.\n");
#endif
    printf("  Size of float       is %3d bit.\n",(int)(sizeof(float)*8));
    printf("  Size of double      is %3d bit.\n",(int)(sizeof(double)*8));
    printf("  Size of long double is %3d bit.\n",(int)(sizeof(long double)*8));
    fflush(stdout);
    printf("Configuration:\n");
#ifdef DBC_BLIT_STATIC
    printf("  DBC_BLIT_STATIC                   is set.\n");
#endif
#ifdef DBC_BLIT_DATA_LITTLE_ENDIAN
    printf("  DBC_BLIT_DATA_LITTLE_ENDIAN       is set.\n");
#endif
#ifdef DBC_BLIT_DATA_BIG_ENDIAN
    printf("  DBC_BLIT_DATA_BIG_ENDIAN          is set.\n");
#endif
#if defined(DBC_BLIT_DATA_LITTLE_ENDIAN) && defined(DBC_BLIT_DATA_BIG_ENDIAN)
    printf("    ...this seems like an error.\n");
#endif
#ifdef DBC_BLIT_NO_GAMMA
    printf("  DBC_BLIT_NO_GAMMA                 is set.\n");
#endif
#ifdef DBC_BLIT_GAMMA_NO_TABLES
    printf("  DBC_BLIT_GAMMA_NO_TABLES          is set to %d.\n",(DBC_BLIT_GAMMA_NO_TABLES + 0));
#endif
#ifdef DBC_BLIT_GAMMA_NO_DOUBLE
    printf("  DBC_BLIT_GAMMA_NO_DOUBLE          is set.\n");
#endif
#ifdef DBC_BLIT_NO_INT64
    printf("  DBC_BLIT_NO_INT64                 is set.\n");
#endif
#ifdef DBC_BLIT_ENABLE_MINGW_SIMD
    printf("  DBC_BLIT_ENABLE_MINGW_SIMD        is set.\n");
#endif
#ifdef DBC_BLIT_NO_SIMD
    printf("  DBC_BLIT_NO_SIMD                  is set.\n");
#endif
#ifdef DBC_BLIT_NO_RUNTIME_CPU_DETECTION
    printf("  DBC_BLIT_NO_RUNTIME_CPU_DETECTION is set.\n");
#endif
#ifdef DBC_BLIT_NO_GCC_ASM
    printf("  DBC_BLIT_NO_GCC_ASM               is set.\n");
#endif
#ifdef DBC_BLIT_NO_AVX2
    printf("  DBC_BLIT_NO_AVX2                  is set.\n");
#endif
#ifdef DBC_BLIT_UNROLL
    printf("  DBC_BLIT_UNROLL                   is set to %d.\n",(DBC_BLIT_UNROLL + 0));
#endif
    printf("\n");
    printf("Initialization...");
    fflush(stdout);
    dbc_blit(0,0,0,0,0,0,0,0,0,0,0,0);
    printf(" done!\n");

#ifndef DBC_BLIT_NO_SIMD
#ifdef DBCB_X86_OR_X64
    if(dbcB_has_sse2) printf("  SSE2 detected.\n");
    else              printf("  SSE2 not detected.\n");
#ifndef DBC_BLIT_NO_AVX2
    if(dbcB_has_avx2) printf("  AVX2 detected.\n");
    else              printf("  AVX2 not detected.\n");
#endif
#endif
#endif
    printf("\n");
    fflush(stdout);

    if(1) test_speed();
    if(1) test_modes();
    if(1) test_ops();
#ifndef DBC_BLIT_NO_GAMMA
    if(!online_compiler) test_gamma();
#endif
    fflush(stdout);

#ifdef __linux__
    printf("Machine information:\n");
    {
        FILE *f=fopen("/proc/cpuinfo","rb");
        if(f)
        {
            int c=0;
            while((c=fgetc(f))!=EOF) printf("%c",c);
            fclose(f);
        }
    }
    printf("\n");
#endif
    fflush(stdout);
    return 0;
}
