/*
    Some useful links:
https://www.transmissionzero.co.uk/computing/building-dlls-with-mingw/
https://www.transmissionzero.co.uk/computing/advanced-mingw-dll-topics/
https://www.willus.com/mingw/yongweiwu_stdcall.html
https://sourceforge.net/p/mingw-w64/mailman/mingw-w64-public/thread/AANLkTinD%2BimPyetiX0XeDXQ3-ofqdebcvMH2gvhk7tv6%40mail.gmail.com/
*/

#include <windows.h>

#include "dbc_blit_dll.h"

static int has_os_level_sse_support=0;

#define dbcb_allow_sse2_for_mode(mode,modulated) (has_os_level_sse_support)
#define dbcb_allow_avx2_for_mode(mode,modulated) (has_os_level_sse_support)

#define DBC_BLIT_IMPLEMENTATION
#define DBC_BLIT_STATIC
#define dbc_blit dbc_blit_implementation
#include "dbc_blit.h"
#undef dbc_blit

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
    (void)hinstDLL;
    (void)lpvReserved;
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            dbc_blit_implementation(0,0,0,0,0,0,0,0,0,0,0,0); /* Initialization. */
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64)
            has_os_level_sse_support=1;
#elif defined(_WIN16)
            has_os_level_sse_support=0;
#else
            if((GetVersion()&0xFF)<=4) has_os_level_sse_support=0; /* Windows 95 or earlier. */
            else                       has_os_level_sse_support=1;
#endif
            break;
        case DLL_THREAD_ATTACH:  break;
        case DLL_THREAD_DETACH:  break;
        case DLL_PROCESS_DETACH: break;
    }

    return TRUE;
}

DBC_BLIT_API void DBC_BLIT_CALL dbc_blit(
    int src_w,int src_h,int src_stride,
    const unsigned char *src_pixels,
    int dst_w,int dst_h,int dst_stride,
    unsigned char *dst_pixels,
    int x,int y,
    const float *color,
    int mode)
{
    dbc_blit_implementation(
        src_w,src_h,src_stride,src_pixels,
        dst_w,dst_h,dst_stride,dst_pixels,
        x,y,color,mode);
}
