gcc -std=c99 -Wall -Wextra -m32 -march=i386 -mtune=generic -mwindows -mstackrealign -DDBC_BLIT_ENABLE_MINGW_SIMD -O3 -s -o demo32.exe demo.c
gcc -std=c99 -Wall -Wextra -m64 -march=x86-64 -mtune=generic -mwindows -O3 -s -o demo64.exe demo.c
