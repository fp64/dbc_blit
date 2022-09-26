gcc -std=c99 -Wall -Wextra -m32 -mstackrealign -DDBC_BLIT_ENABLE_MINGW_SIMD -O3 -s -o check32.exe check.c
gcc -std=c99 -Wall -Wextra -m64 -O3 -s -o check64.exe check.c
