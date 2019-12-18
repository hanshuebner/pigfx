
#include <stdio.h>

#include "gfx.h"
#include "dma.h"
#include "hwutils.h"

extern unsigned char G_FONT_GLYPHS;

#define MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define PFB(X, Y) (ctx.pfb + Y * ctx.pitch + X)

typedef struct
{
  unsigned int width;
  unsigned int height;
  unsigned int pitch;
  unsigned int size;
  unsigned char* pfb;

  unsigned char* full_pfb;
  unsigned int full_size;
  unsigned int full_height;

  int font_height;
  int font_width;
  unsigned char* font_data;

  unsigned char cursor_buffer[10 * 20];
} FRAMEBUFFER_CTX;

static FRAMEBUFFER_CTX ctx;
unsigned int __attribute__((aligned(0x100))) mem_buff_dma[16];

#define MKCOL32(c) ((c) << 24 | (c) << 16 | (c) << 8 | (c))

void
dma_execute_queue_and_wait()
{
  dma_execute_queue();

  while (DMA_CHAN0_BUSY)
    ; // Busy wait for DMA to finish
}

void
gfx_set_env(void* p_framebuffer,
            unsigned int width,
            unsigned int height,
            unsigned int pitch,
            unsigned int size)
{
  dma_init();

  ctx.full_pfb = p_framebuffer;
  ctx.full_height = height;
  ctx.full_size = size;
  ctx.width = width;
  ctx.pitch = pitch;

  ctx.font_height = 20;
  ctx.font_width = 10;
  ctx.font_data = &G_FONT_GLYPHS;

  unsigned int lines = ctx.full_height / ctx.font_height;
  unsigned border_top_bottom = (ctx.full_height - (lines * ctx.font_height)) / 2;

  ctx.pfb = ctx.full_pfb + (border_top_bottom * ctx.pitch);
  ctx.height = ctx.full_height - (border_top_bottom * 2);
  ctx.size = ctx.full_size - (border_top_bottom * 2 * ctx.pitch);
}

void
gfx_clear(GFX_COL background_color)
{
  unsigned int* BG = (unsigned int*)mem_2uncached(mem_buff_dma);
  *BG = MKCOL32(background_color);
  *(BG + 1) = *BG;
  *(BG + 2) = *BG;
  *(BG + 3) = *BG;

  dma_enqueue_operation(BG, (unsigned int*)(ctx.pfb), ctx.size, 0, DMA_TI_DEST_INC);

  dma_execute_queue_and_wait();
}

static void
gfx_scroll_up(unsigned int start_line,
              unsigned int end_line,
              unsigned int lines,
              GFX_COL background_color)
{
  unsigned int pixels_per_line = ctx.font_height * ctx.pitch;
  unsigned char* from = ctx.pfb + (lines + start_line) * pixels_per_line;
  unsigned char* to = ctx.pfb + start_line * pixels_per_line;
  unsigned int length = (end_line - start_line - lines + 1) * pixels_per_line;

  if (length) {
    dma_enqueue_operation((unsigned int*) from,
                          (unsigned int*) to,
                          length,
                          0,
                          DMA_TI_SRC_INC | DMA_TI_DEST_INC);
  }
  {
      unsigned int* BG = (unsigned int*)mem_2uncached(mem_buff_dma);
      BG[0] = MKCOL32(background_color);
      BG[1] = BG[0];
      BG[2] = BG[0];
      BG[3] = BG[0];

      dma_enqueue_operation(BG,
                            (unsigned int*)(ctx.pfb + (end_line - lines + 1) * pixels_per_line),
                            lines * pixels_per_line,
                            0,
                            DMA_TI_DEST_INC);
  }

  dma_execute_queue_and_wait();
}

static void
gfx_scroll_down(unsigned int start_line,
                unsigned int end_line,
                unsigned int lines,
                GFX_COL background_color)
{
  unsigned int pixels_per_line = ctx.font_height * ctx.pitch;
  lines = MIN(lines, end_line - start_line);
  for (unsigned int line_to_move = end_line - lines; line_to_move >= start_line; line_to_move--) {
    unsigned char* from = ctx.pfb + line_to_move * pixels_per_line;
    unsigned char* to = ctx.pfb + (line_to_move + lines) * pixels_per_line;
    dma_enqueue_operation((unsigned int*) from,
                          (unsigned int*) to,
                          pixels_per_line,
                          0,
                          DMA_TI_SRC_INC | DMA_TI_DEST_INC);
  }
  {
      unsigned int* BG = (unsigned int*)mem_2uncached(mem_buff_dma);
      BG[0] = MKCOL32(background_color);
      BG[1] = BG[0];
      BG[2] = BG[0];
      BG[3] = BG[0];

      dma_enqueue_operation(BG,
                            (unsigned int*)(ctx.pfb + start_line * pixels_per_line),
                            lines * pixels_per_line,
                            0,
                            DMA_TI_DEST_INC);
  }
  dma_execute_queue_and_wait();
}

void
gfx_fill_rect(unsigned int x,
              unsigned int y,
              unsigned int width,
              unsigned int height,
              GFX_COL color)
{
  unsigned int* FG = (unsigned int*)mem_2uncached(mem_buff_dma) + 4;
  *FG = MKCOL32(color);
  *(FG + 1) = *FG;
  *(FG + 2) = *FG;
  *(FG + 3) = *FG;

  dma_enqueue_operation(FG,
                        (unsigned int*)(PFB(x, y)),
                        (((height - 1) & 0xFFFF) << 16) | (width & 0xFFFF),
                        ((ctx.pitch - width) & 0xFFFF) << 16, /* bits 31:16 destination stride, 15:0 source stride */
                        DMA_TI_DEST_INC | DMA_TI_2DMODE);
  dma_execute_queue_and_wait();
}

void
gfx_putc(unsigned row,
         unsigned column,
         unsigned char c,
         GFX_COL foreground_color,
         GFX_COL background_color)
{
  unsigned char* p_glyph = (unsigned char*)(ctx.font_data + (c * ctx.font_width * ctx.font_height));
  unsigned char* pf = PFB((column * ctx.font_width),
                          (row * ctx.font_height));

  for (int y = 0; y < ctx.font_height; y++) {
    for (int x = 0; x < ctx.font_width; x++) {
      pf[x] = *p_glyph++ ? foreground_color : background_color;
    }
    pf += ctx.pitch;
  }
}

void
gfx_restore_cursor_content(unsigned int row,
                           unsigned int column)
{
  // Restore framebuffer content that was overwritten by the cursor
  unsigned char* pb = ctx.cursor_buffer;
  unsigned char* pfb = PFB(column * ctx.font_width,
                           row * ctx.font_height);

  for (int y = 0; y < ctx.font_height; y++) {
    for (int x = 0; x < ctx.font_width; x++) {
      pfb[x] = *pb++;
    }
    pfb += ctx.pitch;
  }
}

void
gfx_save_cursor_content(unsigned int row,
                        unsigned int column)
{
  // Save framebuffer content that is going to be replaced by the cursor

  unsigned char* pb = ctx.cursor_buffer;
  unsigned char* pfb = PFB(column * ctx.font_width,
                           row * ctx.font_height);

  for (int y = 0; y < ctx.font_height; y++) {
    for (int x = 0; x < ctx.font_width; x++) {
      *pb++ = pfb[x];
    }
    pfb += ctx.pitch;
  }
}

/*
void
gfx_term_render_cursor()
{
  static int old_cursor_blink_state;

  if (old_cursor_blink_state != ctx.term.cursor_blink_state) {
    unsigned char* pb = ctx.cursor_buffer;
    unsigned char* pfb = PFB(ctx.term.cursor_column * ctx.font_width,
                             ctx.term.cursor_row * ctx.font_height);

    for (int y = 0; y < ctx.font_height; y++) {
      for (int x = 0; x < ctx.font_width; x++) {
        if (ctx.term.cursor_visible && ctx.term.cursor_blink_state) {
          pfb[x] = ctx.cursor_color;
        } else {
          pfb[x] = *pb++;
        }
      }
      pfb += ctx.pitch;
    }

    old_cursor_blink_state = ctx.term.cursor_blink_state;
  }
}

*/
