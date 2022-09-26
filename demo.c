#if !defined(USE_SDL) && !defined(_WIN32)
#define USE_SDL
#endif

/*============================================================================*/

static int W=800;
static int H=600;
static unsigned char *colorbuffer;
static float         *depthbuffer;
static char title[4096];

static int init();
static int finish();
static int update();

/*============================================================================*/
/* Platform-specific. */

#ifdef USE_SDL
/* SDL specific. */
#include <SDL.h>

#define K_ESC SDLK_ESCAPE
#define K_F1  SDLK_F1
#define K_Q   SDLK_q
#define K_W   SDLK_w
#define K_E   SDLK_e
#define K_A   SDLK_a
#define K_S   SDLK_s
#define K_D   SDLK_d
#define K_Z   SDLK_z
#define K_X   SDLK_x
#define K_C   SDLK_c
#define GETKEY(key)     (keys[(key)%1337&4095])
#define KEYDOWN(key)    GETKEY(key)
#define KEYPRESSED(key) (keys[(key)%1337&4095]==1?!!(keys[(key)%1337&4095]=2):0)

SDL_Window *window=NULL;
SDL_Surface *screen_surface=NULL,*surface=NULL;
static int keys[4096];

static double get_time()
{
    return (double)SDL_GetTicks()/1000.0;
}
static void message_box(const char *title,const char *text)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,title,text,window);
}

int main(int argc,const char **argv)
{
    int fullscreen=0;
    (void)argc;(void)argv;
    if(SDL_Init(SDL_INIT_VIDEO)<0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n",SDL_GetError());
        return 1;
    }
    else
    {
        window=SDL_CreateWindow("Metaballs",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,W,H,(fullscreen?SDL_WINDOW_FULLSCREEN_DESKTOP:SDL_WINDOW_RESIZABLE)|SDL_WINDOW_SHOWN); 
        if(window==NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n",SDL_GetError());
            return 1;
        }
        else
        {
            screen_surface=SDL_GetWindowSurface(window);
            W=screen_surface->w;
            H=screen_surface->h;
            surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_RGBA32);
            SDL_SetSurfaceBlendMode(surface,SDL_BLENDMODE_NONE);
            colorbuffer=(unsigned char*)(surface->pixels);
            depthbuffer=(float*)malloc((size_t)(W*H*4));
            if(!depthbuffer) return 1;
        }
    }
    if(init()) return -1;
    while(1)
    {
        SDL_Event e;
        int resized=0;
        while(SDL_PollEvent(&e)!=0)
        {
            if(e.type==SDL_QUIT) break;
            else if(e.type==SDL_KEYDOWN) GETKEY(e.key.keysym.sym)=1;
            else if(e.type==SDL_KEYUP) GETKEY(e.key.keysym.sym)=0;
            else if(e.type==SDL_WINDOWEVENT)
            {
                if(e.window.event==SDL_WINDOWEVENT_RESIZED) resized=1;
            }
        }
        if(resized)
        {
            SDL_FreeSurface(surface);
            screen_surface=SDL_GetWindowSurface(window);
            W=screen_surface->w;
            H=screen_surface->h;
            surface=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_RGBA32);
            SDL_SetSurfaceBlendMode(surface,SDL_BLENDMODE_NONE);
            colorbuffer=(unsigned char*)surface->pixels;
            free(depthbuffer);
            depthbuffer=(float*)malloc((size_t)(W*H*4));
            if(!depthbuffer) return -1;
            resized=0;
        }
        if(update()) break;
        SDL_BlitSurface(surface,NULL,screen_surface,NULL);
	SDL_SetWindowTitle(window,title);
        SDL_UpdateWindowSurface(window);
    }
    finish();
    free(depthbuffer);
    SDL_FreeSurface(surface);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#else /* USE_SDL */
/* WinAPI specific. */
#include <windows.h>

#define K_ESC VK_ESCAPE
#define K_F1  VK_F1
#define K_Q   'Q'
#define K_W   'W'
#define K_E   'E'
#define K_A   'A'
#define K_S   'S'
#define K_D   'D'
#define K_Z   'Z'
#define K_X   'X'
#define K_C   'C'
#define KEYDOWN(key)    (GetAsyncKeyState(key))
#define KEYPRESSED(key) (GetAsyncKeyState(key)&0x1)

static double QPF=-1.0;

static double get_time()
{
    LARGE_INTEGER x;
    if(QPF<=0.0)
    {
        QueryPerformanceFrequency(&x);
        QPF=(double)(x.QuadPart);
    }
    QueryPerformanceCounter(&x);
    return (double)(x.QuadPart)/QPF;
}

static void message_box(const char *title,const char *text)
{
    MessageBox(NULL,text,title,MB_OK);
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
    static BITMAPINFOHEADER bmih={sizeof(BITMAPINFOHEADER),0,0,1,32,BI_RGB,0,0,0,0,0};
    WNDCLASSEX wcex;
    RECT rect={0,0,W,H};
    HWND hwnd;
    HDC hdc;
    MSG msg;

    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    memset(&wcex,0,sizeof(wcex));

    wcex.cbSize=sizeof(WNDCLASSEX);
    wcex.style=CS_OWNDC;
    wcex.lpfnWndProc=DefWindowProc;
    wcex.hInstance=hInstance;
    wcex.lpszClassName="Metaballs";

    if(!RegisterClassEx(&wcex)) return 1;

    AdjustWindowRect(&rect,WS_OVERLAPPEDWINDOW,0);

    hwnd=CreateWindowA("Metaballs","Metaballs",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        (int)(CW_USEDEFAULT),(int)(CW_USEDEFAULT),rect.right-rect.left,rect.bottom-rect.top,
        NULL,NULL,hInstance,NULL);

    hdc=GetDC(hwnd);
    bmih.biWidth=W;
    bmih.biHeight=H;
    colorbuffer=(unsigned char*)malloc((size_t)(W*H*4));
    if(!colorbuffer) return -1;

    depthbuffer=(float*)malloc((size_t)(W*H*4));
    if(!depthbuffer) return -1;

    if(init()) return -1;
    while(1)
    {
        while(PeekMessage(&msg,NULL,0,0,PM_REMOVE))
        {
            if(msg.message==WM_QUIT) break;
            else
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        RECT rect;
        GetClientRect(hwnd,&rect);
        if(rect.right-rect.left!=W||rect.bottom-rect.top!=H)
        {
            W=rect.right-rect.left;
            H=rect.bottom-rect.top;
            free(colorbuffer);
            free(depthbuffer);
            colorbuffer=(unsigned char*)malloc((size_t)(W*H*4));
            if(!colorbuffer) return -1;
            depthbuffer=(float*)malloc((size_t)(W*H*4));
            if(!depthbuffer) {free(colorbuffer); return -1;}
            bmih.biWidth=W;
            bmih.biHeight=H;
        }
        if(update()) break;
        /* GDI uses BGRA. */
        for(int y=H-1;y>=0;--y)
            for(int x=W-1;x>=0;--x)
            {
                unsigned char r=colorbuffer[(y*W+x)*4+0];
                unsigned char b=colorbuffer[(y*W+x)*4+2];
                colorbuffer[(y*W+x)*4+2]=r;
                colorbuffer[(y*W+x)*4+0]=b;
            }
        StretchDIBits(
            hdc,
            0,0,W,H,
            0,0,W,H,
            colorbuffer,
            (BITMAPINFO*)(&bmih),
            DIB_RGB_COLORS,
            SRCCOPY);
        if(!SetWindowTextA(hwnd,title)) break;
        InvalidateRect(hwnd,NULL,FALSE);
    }

    ReleaseDC(hwnd,hdc);
    DestroyWindow(hwnd);
    free(colorbuffer);
    free(depthbuffer);

    return 0;
}
#endif /* USE_SDL */

/*============================================================================*/

#include <stdio.h>
#include <stdlib.h>

#define DBC_BLIT_IMPLEMENTATION
#include "dbc_blit.h"

#define M 1024

static unsigned char *sprite;
static char level[16];

static int N;
static int mode;
static int C;
static int w,h;
static int b;
static double f=0.0;

static unsigned char float2uint8(float x)
{
    if(!(x>=0.0f)) x=0.0f; /* Also catches NaNs. */
    if(x>1.0f) x=1.0f;
    return (unsigned char)(255.0f*x+0.5f);
}

static void init_sprite32()
{
    int x,y;
    for(y=0;y<h;++y)
    {
        for(x=0;x<w;++x)
        {
            int id=(y*w+x)*4;
            float u=((float)x+0.5f)/(float)w;
            float v=((float)y+0.5f)/(float)h;
            float c;
            u=2.0f*u-1.0f;
            v=2.0f*v-1.0f;
            c=1.0f-(u*u+v*v);
            if(c<0.0f) c=0.0f;
            c=c*c*(3.0f-2.0f*c);
            c=c*c*(3.0f-2.0f*c);
            sprite[id+0]=float2uint8(0.5f+u);
            sprite[id+1]=float2uint8(0.5f+v);
            sprite[id+2]=float2uint8(2.0f*c-c*c);
            sprite[id+3]=float2uint8(c);
        }
    }   
}

static void init_sprite8()
{
    int x,y;
    for(y=0;y<h;++y)
    {
        for(x=0;x<w;++x)
        {
            int id=(y*w+x);
            float u=((float)x+0.5f)/(float)w;
            float v=((float)y+0.5f)/(float)h;
            unsigned int value;
            float c;
            u=2.0f*u-1.0f;
            v=2.0f*v-1.0f;
            c=1.0f-(u*u+v*v);
            if(c<0.0f) c=0.0f;
            c=c*c*(3.0f-2.0f*c);
            c=c*c*(3.0f-2.0f*c);
            value=(unsigned int)(float2uint8(0.5f+u)>>5);
            value|=((unsigned int)(float2uint8(0.5f+v)>>5))<<3;
            value|=((unsigned int)(float2uint8(2.0f*c-c*c)>>6))<<6;
            sprite[id+0]=(unsigned char)(value&255);
            if(c<=0.0f) sprite[id+0]=1;
        }
    }
}

static void init_sprite16()
{
    int x,y;
    for(y=0;y<h;++y)
    {
        for(x=0;x<w;++x)
        {
            int id=(y*w+x)*2;
            float u=((float)x+0.5f)/(float)w;
            float v=((float)y+0.5f)/(float)h;
            unsigned int value;
            float c;
            u=2.0f*u-1.0f;
            v=2.0f*v-1.0f;
            c=1.0f-(u*u+v*v);
            if(c<0.0f) c=0.0f;
            c=c*c*(3.0f-2.0f*c);
            c=c*c*(3.0f-2.0f*c);
            value=(unsigned int)(float2uint8(0.5f+u)>>3);
            value|=((unsigned int)(float2uint8(0.5f+v)>>3))<<5;
            value|=((unsigned int)(float2uint8(2.0f*c-c*c)>>3))<<10;
            value|=((unsigned int)(c>0.5f))<<15;
            sprite[id+0]=(unsigned char)(value&255);
            sprite[id+1]=(unsigned char)(value>>8);
            if(c<=0.0f) {sprite[id+0]=1;sprite[id+1]=0;}
        }
    }
}

static int init()
{
    dbc_blit(0,0,0,0,0,0,0,0,0,0,0,0);
    sprintf(level,"C");
#ifndef DBC_BLIT_NO_SIMD
    if(dbcB_has_sse2) sprintf(level,"SSE2");
#ifndef DBC_BLIT_NO_AVX2
    if(dbcB_has_avx2) sprintf(level,"AVX2");
#endif
#endif
    N=1000;
    C=0;
    w=h=37;
    b=4;
    mode=DBCB_MODE_COPY;
    sprite=malloc(M*M*4);
    if(!sprite) {return -1;}

    init_sprite32();
    return 0;
}

static int finish()
{
    free(sprite);
    return 0;
}

static int update()
{
    static float color[4]={1.0f,0.25f,0.75f,0.5f};
    double t;

    static const char *modes[]={
        "DBCB_MODE_COPY",
        "DBCB_MODE_ALPHA",
        "DBCB_MODE_PMA",
        "DBCB_MODE_GAMMA",
        "DBCB_MODE_PMG",
        "DBCB_MODE_COLORKEY8",
        "DBCB_MODE_COLORKEY16",
        "DBCB_MODE_5551",
        "DBCB_MODE_MUL",
        "DBCB_MODE_MUG",
        "DBCB_MODE_ALPHATEST",
        "DBCB_MODE_CPYG"
    };
    int i,x,y;

    if(KEYDOWN(K_F1))
    {
#ifdef USE_SDL
#define TAB ""
#else
#define TAB "\t"
#endif
        const char *s=
            "Esc " TAB "- exit\n"
            "F1  " TAB "- help\n"
            "Q/A " TAB "- increase/decrease sprite count\n"
            "W   " TAB "- change mode\n"
            "S   " TAB "- toggle modulation\n"
            "E/D " TAB "- increase/decrease sprite size\n";
        message_box("Controls",s);
        if(KEYPRESSED(K_ESC)) {}
    }
    if(KEYPRESSED(K_ESC)) return 1;
    if(KEYDOWN(K_Q)) N+=(N/16)+1;
    if(KEYDOWN(K_A)) {N-=(N/16)+1; if(N<1) N=1;}
    if(KEYPRESSED(K_W))
    {
        mode=(mode+1)%(int)(sizeof(modes)/sizeof(modes[0]));
        if(mode==DBCB_MODE_COLORKEY8) init_sprite8();
        else if(mode==DBCB_MODE_COLORKEY16||mode==DBCB_MODE_5551) init_sprite16();
        else init_sprite32();
    }
    if(KEYPRESSED(K_S)) C=!C; 
    if(KEYDOWN(K_E))
    {
        w+=1;
        h+=1;
        if(w>M) {w=M;h=M;}
        if(mode==DBCB_MODE_COLORKEY8) init_sprite8();
        else if(mode==DBCB_MODE_COLORKEY16||mode==DBCB_MODE_5551) init_sprite16();
        else init_sprite32();
    }
    if(KEYDOWN(K_D))
    {
        w-=1;
        h-=1;
        if(w<1) {w=1;h=1;}
        if(mode==DBCB_MODE_COLORKEY8) init_sprite8();
        else if(mode==DBCB_MODE_COLORKEY16||mode==DBCB_MODE_5551) init_sprite16();
        else init_sprite32();
    }

    if(mode==DBCB_MODE_COLORKEY8) b=1;
    else if(mode==DBCB_MODE_COLORKEY16||mode==DBCB_MODE_5551) b=2;
    else b=4;

    memset(colorbuffer,(mode==DBCB_MODE_MUL||mode==DBCB_MODE_MUG?255:0),W*H*4);
    t=get_time();
    for(i=0;i<N;++i)
    {
        dbc_blit(
            w,h,b*w,sprite,
            W,H,b*W,colorbuffer,
            rand()%(W+w)-w,rand()%(H+h)-h,
            (C?color:NULL),
            mode);
    }
    t=(get_time()-t);
    f=0.125*t+(1.0-0.125)*f;
    if(mode==DBCB_MODE_COLORKEY8)
    {
        for(y=H-1;y>=0;--y)
            for(x=W-1;x>=0;--x)
            {
                unsigned int c=(unsigned int)(colorbuffer[y*W+x]);
                colorbuffer[(y*W+x)*4+0]=(unsigned char)((c&0x07)<<5);
                colorbuffer[(y*W+x)*4+1]=(unsigned char)((c&0x31)<<2);
                colorbuffer[(y*W+x)*4+2]=(unsigned char)((c&0xC0));
                colorbuffer[(y*W+x)*4+3]=255;
            }
    }
    else if(mode==DBCB_MODE_COLORKEY16||mode==DBCB_MODE_5551)
    {
        for(y=H-1;y>=0;--y)
            for(x=W-1;x>=0;--x)
            {
                unsigned int c=
                    (unsigned int)(colorbuffer[(y*W+x)*2+0])|
                    ((unsigned int)(colorbuffer[(y*W+x)*2+1])<<8);
                colorbuffer[(y*W+x)*4+0]=(unsigned char)((c&0x001F)<<3);
                colorbuffer[(y*W+x)*4+1]=(unsigned char)((c&0x03E0)>>2);
                colorbuffer[(y*W+x)*4+2]=(unsigned char)((c&0x7C00)>>7);
                colorbuffer[(y*W+x)*4+3]=((c&0x80)?255:0);
            }
    }
    sprintf(title,"F1=help|%7.2f ms|%3dx%3d * %5d|%4ldx%4ld|%3s|%-10s[%-9s]",1000.0*f,w,h,N,(long)W,(long)H,level,modes[mode]+10,(C?"modulated":"straight"));
    return 0;
}

