gcc -std=c99 -Wall -Wextra -m32 -march=i386 -mtune=generic -mstackrealign -DDBC_BLIT_ENABLE_MINGW_SIMD -DDBC_BLIT_EXPORTS -O3 -c -o dbc_blit_dll32.o dbc_blit_dll.c
gcc -m32 -o dbc_blit32.dll dbc_blit_dll32.o -s -shared -Wl,--subsystem,windows,--output-def,dbc_blit_dll32.def
gcc -m32 -o dbc_blit32.dll dbc_blit_dll32.o -s -shared -Wl,--subsystem,windows,--kill-at
dlltool --as-flags=--32 -m i386 --kill-at -d dbc_blit_dll32.def -D dbc_blit32.dll -l dbc_blit32.lib
gcc -std=c99 -Wall -Wextra -m32 -mstackrealign -O3 -s -o test_dll32.exe test_dll.c -L. -ldbc_blit32
echo y | move dbc_blit32.dll dll/dbc_blit32.dll
echo y | move dbc_blit32.lib dll/dbc_blit32.lib
echo y | move test_dll32.exe dll/test_dll32.exe
del dbc_blit_dll32.o
del dbc_blit_dll32.def

gcc -std=c99 -Wall -Wextra -m64 -march=x86-64 -mtune=generic -DDBC_BLIT_EXPORTS -O3 -c -o dbc_blit_dll64.o dbc_blit_dll.c
gcc -m64 -o dbc_blit64.dll dbc_blit_dll64.o -s -shared -Wl,--subsystem,windows,--output-def,dbc_blit_dll64.def
gcc -m64 -o dbc_blit64.dll dbc_blit_dll64.o -s -shared -Wl,--subsystem,windows,--kill-at
dlltool --as-flags=--64 -m i386:x86-64 --kill-at -d dbc_blit_dll64.def -D dbc_blit64.dll -l dbc_blit64.lib
gcc -std=c99 -Wall -Wextra -m64 -O3 -s -o test_dll64.exe test_dll.c -L. -ldbc_blit64
echo y | move dbc_blit64.dll dll/dbc_blit64.dll
echo y | move dbc_blit64.lib dll/dbc_blit64.lib
echo y | move test_dll64.exe dll/test_dll64.exe
del dbc_blit_dll64.o
del dbc_blit_dll64.def
