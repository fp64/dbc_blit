#ifndef DBC_BLIT_DLL_H
#define DBC_BLIT_DLL_H

#ifdef DBC_BLIT_EXPORTS
#define DBC_BLIT_API __declspec(dllexport)
#else
#define DBC_BLIT_API __declspec(dllimport)
#endif

#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64)
#define DBC_BLIT_CALL
#else
#define DBC_BLIT_CALL __stdcall
#endif

#ifdef __cplusplus
extern "C"
{
#endif

DBC_BLIT_API void DBC_BLIT_CALL dbc_blit(
    int src_w,int src_h,int src_stride,
    const unsigned char *src_pixels,
    int dst_w,int dst_h,int dst_stride,
    unsigned char *dst_pixels,
    int x,int y,
    const float *color,
    int mode);

#ifndef DBCB_MODE_COPY
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
#endif

#ifdef __cplusplus
}
#endif

#endif
