## dbc_blit.h — single file public domain software blitter

An easy-to-use API to blit pixel rectangles (no scaling/rotation/shear
support). Supports several blending modes. Should be cross-platform,
but only x86 has an optimized SIMD implementation.

Repository also includes test/benchmark suite (`check.c`),
a simple graphical demo (`demo.c`), and prebuilt DLLs for
Windows (see `test_dll.c` for usage example).

#### Usage

In the style of [STB](https://github.com/nothings/stb) libraries,
`dbc_blit.h` acts as its own header, and expects you to
select _one_ file in which to create an implementation:
```c
#define DBC_BLIT_IMPLEMENTATION // <-- only define in ONE file.
#include "dbc_blit.h"
```
The single exposed API function is:
```c
void dbc_blit(
    int src_w, int src_h, int src_stride_in_bytes, const unsigned char *src_pixels,
    int dst_w, int dst_h, int dst_stride_in_bytes,       unsigned char *dst_pixels,
    int x, int y,
    const float *color, int mode);
```
- `w, h, stride, pixel` describe a rectangular pixel surface.
- `x, y` specify where `src`'s (0,0) is placed on `dst`.
- `color` (optional) specifies color modulation (or colorkey, or alphatest threshold, depending on `mode`).
- `mode` is one of the following constants:

| **Mode**               | **Description**     |
|------------------------|---------------------|
| `DBCB_MODE_COPY`       | Straight copy       |
| `DBCB_MODE_ALPHA`      | Alpha-blending      |
| `DBCB_MODE_PMA`        | [Premultiplied alpha](https://en.wikipedia.org/wiki/Alpha_compositing#Straight_versus_premultiplied) ([see](http://tomforsyth1000.github.io/blog.wiki.html#[[Premultiplied%20alpha]]) [also](http://tomforsyth1000.github.io/blog.wiki.html#[[Premultiplied%20alpha%20part%202]]))|
| `DBCB_MODE_GAMMA`      | Alpha-blending in [sRGB](https://en.wikipedia.org/wiki/SRGB) |
| `DBCB_MODE_PMG`        | Premultiplied alpha in sRGB |
| `DBCB_MODE_COLORKEY8`  | 8-bit with optional colorkey |
| `DBCB_MODE_COLORKEY16` | 16-bit with optional colorkey |
| `DBCB_MODE_5551`       | 16-bit with 1-bit alpha channel |
| `DBCB_MODE_MUL`        | Color multiplication |
| `DBCB_MODE_MUG`        | Color multiplication in sRGB|
| `DBCB_MODE_ALPHATEST`  | Alpha-test |
| `DBCB_MODE_CPYG`       | Copy in sRGB (only matters with color modulation) |

See documentation in `dbc_blit.h` for more details (including
blending equations).

#### Speed

The performance depends on many factors, including the architecture
(both CPU and memory), blending mode (sRGB modes may be about 10 times
slower), and usage pattern (blitting a lot of small sprites at random
coordinates may introduce cache misses). Broadly speaking, on a
modern (as of 2022) x86 you should get fillrate of about 2 gigapixels
per second in alpha-blending mode, single-threaded (`dbc_blit.h` does
not use threading internally, but the API should be thread-safe, as long
as there's no dst/dst or src/dst overlap). See table in `dbc_blit.h` for
some more detailed timings.

#### Misc

Version 2.0.7.

Original developmnet thread: https://gamedev.ru/flame/forum/?id=218515 (in Russian).

