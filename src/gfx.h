#ifndef _GFX_H_
#define _GFX_H_

typedef unsigned char GFX_COL;

extern void gfx_set_env(void* p_framebuffer,
                        unsigned int width,
                        unsigned int height,
                        unsigned int pitch,
                        unsigned int size);

extern void gfx_putc(unsigned row,
                     unsigned column,
                     unsigned char c,
                     GFX_COL foreground_color,
                     GFX_COL background_color);

extern void gfx_clear(GFX_COL color);

extern void gfx_fill_rect(unsigned int x,
                          unsigned int y,
                          unsigned int width,
                          unsigned int height,
                          GFX_COL color);

void gfx_move_rect(unsigned int from_row,
                   unsigned int from_column,
                   unsigned int to_row,
                   unsigned int to_columns,
                   unsigned int rows,
                   unsigned int columns,
                   GFX_COL background_color);

extern void gfx_set_cursor(unsigned int row,
                           unsigned int column,
                           unsigned int visible);
extern void gfx_handle_cursor();

#endif
