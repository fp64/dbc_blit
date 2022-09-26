#include "dbc_blit_dll.h"

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64)
#define LIBRARY_NAME "dbc_blit64.dll"
#else
#define LIBRARY_NAME "dbc_blit32.dll"
#endif

typedef void (DBC_BLIT_CALL *dbc_blit_fn)(
    int src_w,int src_h,int src_stride,
    const unsigned char *src_pixels,
    int dst_w,int dst_h,int dst_stride,
    unsigned char *dst_pixels,
    int x,int y,
    const float *color,
    int mode);

int main()
{
    HMODULE h;
    dbc_blit_fn f;
    unsigned src[128*128*4];
    unsigned dst[128*128*4];
    int i,N=20000;
    double t;
    printf("Testing link-time loading.\n");
    fflush(stdout);
    src[0]=0x80AABBCC;
    dst[0]=0x7F112233;
    printf("alphablend(%08X->%08X)=",src[0],dst[0]);
    fflush(stdout);
    dbc_blit(1,1,4,(unsigned char*)src,1,1,4,(unsigned char*)dst,0,0,0,DBCB_MODE_ALPHA);
    printf("%08X (%s)\n",dst[0],(dst[0]==0xBF5E6F80?"ok":"FAIL"));
    printf("Speed: ");
    fflush(stdout);
    t=clock();
    for(i=0;i<N;++i)
        dbc_blit(128,128,128*4,(unsigned char*)src,128,128,128*4,(unsigned char*)dst,0,0,0,DBCB_MODE_ALPHA);
    t=clock()-t;
    printf("%.3f ns/pixel.\n",1.0e+9*t/N/CLOCKS_PER_SEC/(128*128));

    printf("Testing run-time loading.\n");
    fflush(stdout);
    if(!(h=LoadLibraryA(LIBRARY_NAME)))
    {
        printf("Failed loading library: \"%s\".\n",LIBRARY_NAME);
        return 1;
    }
    if(!(f=(dbc_blit_fn)(uintptr_t)GetProcAddress(h,"dbc_blit")))
    {
        printf("Failed locating function: \"dbc_blit\" .\n");
        return 1;
    }
    printf("dbc_blit() loaded.\n");
    src[0]=0x80AABBCC;
    dst[0]=0x7F112233;
    printf("alphablend(%08X->%08X)=",src[0],dst[0]);
    fflush(stdout);
    f(1,1,4,(unsigned char*)src,1,1,4,(unsigned char*)dst,0,0,0,DBCB_MODE_ALPHA);
    printf("%08X (%s)\n",dst[0],(dst[0]==0xBF5E6F80?"ok":"FAIL"));
    printf("Speed: ");
    fflush(stdout);
    t=clock();
    for(i=0;i<N;++i)
        f(128,128,128*4,(unsigned char*)src,128,128,128*4,(unsigned char*)dst,0,0,0,DBCB_MODE_ALPHA);
    t=clock()-t;
    printf("%.3f ns/pixel.\n",1.0e+9*t/N/CLOCKS_PER_SEC/(128*128));
    FreeLibrary(h);
    return 0;
}
