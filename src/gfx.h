#ifndef _GFX_H_
#define _GFX_H_

typedef unsigned char GFX_COL;

extern void gfx_set_env(void* p_framebuffer,
                        unsigned int width,
                        unsigned int height,
                        unsigned int pitch,
                        unsigned int size);

extern void gfx_clear(GFX_COL color);

extern void gfx_fill_rect(unsigned int x,
                          unsigned int y,
                          unsigned int width,
                          unsigned int height,
                          GFX_COL color);

#endif
