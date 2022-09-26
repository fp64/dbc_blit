.PHONY: all shared clean

ifeq ($(OS),Windows_NT)
.PHONY: check demo

all: check demo shared

check: check.c dbc_blit.h
	build_check_mingw.cmd

demo: demo.c dbc_blit.h
	build_demo_mingw.cmd

shared: dbc_blit_dll.c dbc_blit_dll.h test_dll.c dbc_blit.h
	build_dll_mingw.cmd
else
all: check demo

check: check.c dbc_blit.h
	gcc -std=c99 -Wall -Wextra -O3 -o check check.c -lm

# gcc -std=c99 -Wall -Wextra -O3 -I/usr/local/include/SDL2 -o demo demo.c -L/usr/local/lib -lm -lSDL2
demo: demo.c dbc_blit.h
	gcc -std=c99 -Wall -Wextra -O3 `sdl2-config --cflags` -o demo demo.c `sdl2-config --libs` -lm
endif

clean:
	rm -f *.exe
	rm -f *.zip
	rm -f dll/*.exe
	rm -f check
	rm -f demo

