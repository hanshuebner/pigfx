#include "gfx.h"

#include <stdio.h>

#include "console.h"
#include "dma.h"
#include "ee_printf.h"
#include "pigfx_config.h"
#include "uart.h"
#include "utils.h"

extern unsigned char G_FONT_GLYPHS08;
extern unsigned char G_FONT_GLYPHS14;
extern unsigned char G_FONT_GLYPHS16;
extern unsigned char G_FONT_GLYPHS20;

#define MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define MAX(v1, v2) (((v1) > (v2)) ? (v1) : (v2))
#define PFB(X, Y) (ctx.pfb + Y * ctx.pitch + X)

void
__swap__(int* a, int* b)
{
  int aux = *a;
  *a = *b;
  *b = aux;
}

int
__abs__(int a)
{
  return a < 0 ? -a : a;
}

void
b2s(char* b, unsigned char n)
{
  *b++ = '0' + n / 100;
  n -= 100 * (n / 100);
  *b++ = '0' + n / 10;
  n -= 10 * (n / 10);
  *b++ = '0' + n;
}

typedef struct SCN_STATE
{
  // state_fun* next;
  void (*next)(char ch, struct SCN_STATE* state);
  void (*after_read_digit)(char ch, struct SCN_STATE* state);

  unsigned int cmd_params[10];
  unsigned int cmd_params_size;
  char private_mode_char;
} scn_state;

/* state forward declarations */
typedef void
state_fun(char ch, scn_state* state);
void
state_fun_normaltext(char ch, scn_state* state);
void
state_fun_read_digit(char ch, scn_state* state);

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

  struct
  {
    unsigned int columns;
    unsigned int rows;
    unsigned int cursor_row;
    unsigned int cursor_col;
    unsigned int saved_cursor[2];
    char cursor_visible;

    scn_state state;
  } term;

  GFX_COL bg;
  GFX_COL fg;
  GFX_COL cursor_color;
  unsigned int inverse;
  unsigned char line_attributes[255];

  unsigned int line_limit;

  int font_height;
  int font_width;
  unsigned char* font_data;

  unsigned char cursor_buffer[10 * 20];

} FRAMEBUFFER_CTX;

static FRAMEBUFFER_CTX ctx;
unsigned int __attribute__((aligned(0x100))) mem_buff_dma[16];

#define MKCOL32(c) ((c) << 24 | (c) << 16 | (c) << 8 | (c))
#define GET_BG32(ctx) (ctx.inverse ? MKCOL32(ctx.fg) : MKCOL32(ctx.bg))
#define GET_FG32(ctx) (ctx.inverse ? MKCOL32(ctx.bg) : MKCOL32(ctx.fg))
#define GET_BG(ctx) (ctx.inverse ? ctx.fg : ctx.bg)
#define GET_FG(ctx) (ctx.inverse ? ctx.bg : ctx.fg)

extern char*
u2s(unsigned int u);
void
gfx_term_render_cursor();

void
gfx_toggle_font_height()
{
  if (ctx.font_height == 8)
    gfx_set_font_height(14);
  else if (ctx.font_height == 14)
    gfx_set_font_height(16);
  else if (ctx.font_height == 16)
    gfx_set_font_height(20);
  else
    gfx_set_font_height(8);
}

void
gfx_set_screen_geometry()
{
  unsigned int lines, border_top_bottom = 0;

  lines = ctx.full_height / ctx.font_height;
  if (ctx.line_limit > 0 && ctx.line_limit < lines)
    lines = ctx.line_limit;
  border_top_bottom = (ctx.full_height - (lines * ctx.font_height)) / 2;
  if (ctx.line_limit == 0 && border_top_bottom == 0)
    border_top_bottom = ctx.font_height / 2;

  gfx_clear();

  ctx.pfb = ctx.full_pfb + (border_top_bottom * ctx.pitch);
  ctx.height = ctx.full_height - (border_top_bottom * 2);
  ctx.size = ctx.full_size - (border_top_bottom * 2 * ctx.pitch);

  ctx.term.rows = ctx.height / ctx.font_height;
  ctx.term.cursor_row = ctx.term.cursor_col = 0;

  gfx_term_render_cursor();
}

void
gfx_toggle_lines()
{
  if (ctx.line_limit == 0)
    ctx.line_limit = ctx.term.rows;

  ctx.line_limit--;
  if (ctx.line_limit < 16)
    ctx.line_limit = ctx.full_height / ctx.font_height;

  gfx_set_screen_geometry();
  gfx_term_putstring("[");
  gfx_term_putstring(u2s(ctx.line_limit));
  gfx_term_putstring(" lines]\n");
}

void
gfx_set_font_height(unsigned int h)
{
  ctx.font_height = h;
  ctx.font_width = (h == 20) ? 10 : 8;
  switch (h) {
    case 8:
      ctx.font_data = &G_FONT_GLYPHS08;
      break;
    case 14:
      ctx.font_data = &G_FONT_GLYPHS14;
      break;
    case 16:
      ctx.font_data = &G_FONT_GLYPHS16;
      break;
    case 20:
      ctx.font_data = &G_FONT_GLYPHS20;
      break;
  }

  ctx.line_limit = MIN(ctx.line_limit, ctx.full_height / h);
  gfx_set_screen_geometry();
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

  ctx.term.columns = ctx.width / ctx.font_width;
  ctx.term.cursor_visible = 1;
  ctx.term.state.next = state_fun_normaltext;

  ctx.bg = 0;
  ctx.fg = 15;
  ctx.cursor_color = 9;
  ctx.inverse = 0;
  ctx.line_limit = SCREEN_LINES;

  for (unsigned int i = 0; i < sizeof(ctx.line_attributes); i++) {
    ctx.line_attributes[i] = 0;
  }

  gfx_set_font_height(FONT_HEIGHT);
}

void
gfx_term_reset_attrib()
{
  gfx_set_bg(0);
  gfx_set_fg(7);
  ctx.inverse = 0;
}

void
gfx_set_bg(GFX_COL col)
{
  ctx.bg = col;
}

void
gfx_set_fg(GFX_COL col)
{
  ctx.fg = col;
}

void
gfx_swap_fg_bg()
{
  ctx.inverse = !ctx.inverse;
}

void
gfx_get_term_size(unsigned int* rows, unsigned int* cols)
{
  *rows = ctx.term.rows;
  *cols = ctx.term.columns;
}

void
gfx_clear()
{
#if ENABLED(GFX_USE_DMA)
  unsigned int* BG = (unsigned int*)mem_2uncached(mem_buff_dma);
  *BG = GET_BG32(ctx);
  *(BG + 1) = *BG;
  *(BG + 2) = *BG;
  *(BG + 3) = *BG;

  dma_enqueue_operation(
    BG, (unsigned int*)(ctx.pfb), ctx.size, 0, DMA_TI_DEST_INC);

  dma_execute_queue();
#else
  unsigned char* pf = ctx.pfb;
  unsigned char* pfb_end = pf + ctx.size;
  while (pf < pfb_end)
    *pf++ = GET_BG(ctx);
#endif
}

void
gfx_scroll_down_dma(unsigned int npixels)
{
  unsigned int* BG = (unsigned int*)mem_2uncached(mem_buff_dma);
  *BG = GET_BG32(ctx);
  *(BG + 1) = *BG;
  *(BG + 2) = *BG;
  *(BG + 3) = *BG;
  unsigned int line_height = ctx.pitch * npixels;

  dma_enqueue_operation((unsigned int*)(ctx.pfb + line_height),
                        (unsigned int*)(ctx.pfb),
                        (ctx.size - line_height),
                        0,
                        DMA_TI_SRC_INC | DMA_TI_DEST_INC);

  dma_enqueue_operation(BG,
                        (unsigned int*)(ctx.pfb + ctx.size - line_height),
                        line_height,
                        0,
                        DMA_TI_DEST_INC);
}

void
gfx_scroll_down(unsigned int npixels)
{
#if ENABLED(GFX_USE_DMA)
  gfx_scroll_down_dma(npixels);
  dma_execute_queue();
#else
  unsigned int* pf_src = (unsigned int*)(ctx.pfb + ctx.pitch * npixels);
  unsigned int* pf_dst = (unsigned int*)ctx.pfb;
  const unsigned int* const pfb_end = (unsigned int*)(ctx.pfb + ctx.size);

  while (pf_src < pfb_end)
    *pf_dst++ = *pf_src++;

  // Fill with bg at the bottom
  const unsigned int BG = GET_BG32(ctx);
  while (pf_dst < pfb_end)
    *pf_dst++ = BG;

#endif
}

void
gfx_scroll_up(unsigned int npixels)
{
  unsigned int* pf_dst = (unsigned int*)(ctx.pfb + ctx.size) - 1;
  unsigned int* pf_src =
    (unsigned int*)(ctx.pfb + ctx.size - ctx.pitch * npixels) - 1;
  const unsigned int* const pfb_end = (unsigned int*)(ctx.pfb);

  while (pf_src >= pfb_end)
    *pf_dst-- = *pf_src--;

  // Fill with bg at the top
  const unsigned int BG = GET_BG32(ctx);

  while (pf_dst >= pfb_end)
    *pf_dst-- = BG;
}

void
gfx_fill_rect_dma(unsigned int x,
                  unsigned int y,
                  unsigned int width,
                  unsigned int height)
{
  unsigned int* FG = (unsigned int*)mem_2uncached(mem_buff_dma) + 4;
  *FG = GET_FG32(ctx);
  *(FG + 1) = *FG;
  *(FG + 2) = *FG;
  *(FG + 3) = *FG;

  dma_enqueue_operation(
    FG,
    (unsigned int*)(PFB(x, y)),
    (((height - 1) & 0xFFFF) << 16) | (width & 0xFFFF),
    ((ctx.pitch - width) & 0xFFFF)
      << 16, /* bits 31:16 destination stride, 15:0 source stride */
    DMA_TI_DEST_INC | DMA_TI_2DMODE);
}

void
gfx_fill_rect(unsigned int x,
              unsigned int y,
              unsigned int width,
              unsigned int height)
{
  if (x >= ctx.width || y >= ctx.height)
    return;

  if (x + width > ctx.width)
    width = ctx.width - x;

  if (y + height > ctx.height)
    height = ctx.height - y;

#if ENABLED(GFX_USE_DMA)
  gfx_fill_rect_dma(x, y, width, height);
  dma_execute_queue();
#else
  while (height--) {
    unsigned char* pf = PFB(x, y);
    const unsigned char* const pfb_end = pf + width;

    while (pf < pfb_end)
      *pf++ = GET_FG(ctx);

    ++y;
  }
#endif
}

void
gfx_clear_rect(unsigned int x,
               unsigned int y,
               unsigned int width,
               unsigned int height)
{
  GFX_COL curr_fg = ctx.fg;
  ctx.fg = ctx.bg;
  gfx_fill_rect(x, y, width, height);
  ctx.fg = curr_fg;
}

void
gfx_line(int x0, int y0, int x1, int y1)
{
  x0 = MAX(MIN(x0, (int)ctx.width), 0);
  y0 = MAX(MIN(y0, (int)ctx.height), 0);
  x1 = MAX(MIN(x1, (int)ctx.width), 0);
  y1 = MAX(MIN(y1, (int)ctx.height), 0);

  unsigned char qrdt = __abs__(y1 - y0) > __abs__(x1 - x0);

  if (qrdt) {
    __swap__(&x0, &y0);
    __swap__(&x1, &y1);
  }
  if (x0 > x1) {
    __swap__(&x0, &x1);
    __swap__(&y0, &y1);
  }

  const int deltax = x1 - x0;
  const int deltay = __abs__(y1 - y0);
  register int error = deltax >> 1;
  register unsigned char* pfb;
  unsigned int nr = x1 - x0;

  if (qrdt) {
    const int ystep = y0 < y1 ? 1 : -1;
    pfb = PFB(y0, x0);
    while (nr--) {
      *pfb = GET_FG(ctx);
      error = error - deltay;
      if (error < 0) {
        pfb += ystep;
        error += deltax;
      }
      pfb += ctx.pitch;
    }
  } else {
    const int ystep = y0 < y1 ? ctx.pitch : -ctx.pitch;
    pfb = PFB(x0, y0);
    while (nr--) {
      *pfb = GET_FG(ctx);
      error = error - deltay;
      if (error < 0) {
        pfb += ystep;
        error += deltax;
      }
      pfb++;
    }
  }
}

void
gfx_putc(unsigned int row, unsigned int col, unsigned char c)
{
  if (col >= ctx.term.columns)
    return;

  if (row >= ctx.term.rows)
    return;

  unsigned char* p_glyph = (unsigned char*)(ctx.font_data + (c * ctx.font_width * ctx.font_height));
  unsigned char* pf = PFB((col * ctx.font_width), (row * ctx.font_height));

  for (int y = 0; y < ctx.font_height; y++) {
    for (int x = 0; x < ctx.font_width; x++) {
      pf[x] = *p_glyph++ ? GET_FG(ctx) : GET_BG(ctx);
    }
    pf += ctx.pitch;
  }
}

/*
 * Terminal functions
 *
 */

void
gfx_restore_cursor_content()
{
  // Restore framebuffer content that was overwritten by the cursor
  unsigned char* pb = ctx.cursor_buffer;
  unsigned char* pfb = PFB(ctx.term.cursor_col * ctx.font_width,
                           ctx.term.cursor_row * ctx.font_height);
  
  for (int y = 0; y < ctx.font_height; y++) {
    for (int x = 0; x < ctx.font_width; x++) {
      pfb[x] = *pb++;
    }
    pfb += ctx.pitch;
  }
}

void
gfx_term_render_cursor()
{
  // Save framebuffer content that is going to be replaced by the cursor and
  // update the new content
  //
  while (DMA_CHAN0_BUSY)
    ; // Busy wait for DMA

  unsigned char* pb = ctx.cursor_buffer;
  unsigned char* pfb = PFB(ctx.term.cursor_col * ctx.font_width,
                           ctx.term.cursor_row * ctx.font_height);
  
  for (int y = 0; y < ctx.font_height; y++) {
    for (int x = 0; x < ctx.font_width; x++) {
      *pb++ = pfb[x];
      pfb[x] = ctx.cursor_color;
    }
    pfb += ctx.pitch;
  }
}

/* gfx_term_putstring is the main entry point to the terminal
 * emulator.  Everything that is displayed on the screen goes through
 * this function */

void
gfx_term_putstring(const char* str)
{
  gfx_restore_cursor_content();

  while (*str) {
    while (DMA_CHAN0_BUSY)
      ; // Busy wait for DMA

    switch (*str) {
      case '\r':
        ctx.term.cursor_col = 0;
        break;

      case '\n':
        ++ctx.term.cursor_row;
        break;

      case 0x09: /* tab */
        ctx.term.cursor_col += 1;
        ctx.term.cursor_col = MIN(ctx.term.cursor_col + ctx.font_width - ctx.term.cursor_col % ctx.font_width,
                                  ctx.term.columns - 1);
        break;

      case 0x08:
        /* backspace */
        if (ctx.term.cursor_col > 0) {
          --ctx.term.cursor_col;
        }
        break;

      case 0x0e: // skip shift out
      case 0x0f: // skip shift in
      case 0x07: // skip BELL
        break;

      default:
        ctx.term.state.next(*str, &(ctx.term.state));
        break;
    }

    if (ctx.term.cursor_col >= ctx.term.columns) {
      ++ctx.term.cursor_row;
      ctx.term.cursor_col = 0;
    }

    if (ctx.term.cursor_row >= ctx.term.rows) {
      --ctx.term.cursor_row;

      gfx_scroll_down_dma(ctx.font_height);
      dma_execute_queue();
    }

    ++str;
  }
  gfx_term_render_cursor();
}

void
gfx_term_set_cursor_visibility(unsigned char visible)
{
  ctx.term.cursor_visible = visible;
}

void
gfx_term_move_cursor(int row, int col)
{
  ctx.term.cursor_row = MIN(ctx.term.rows - 1, row > 0 ? row : 0);
  ctx.term.cursor_col = MIN(ctx.term.columns - 1, col > 0 ? col : 0);
}

void
gfx_term_save_cursor()
{
  ctx.term.saved_cursor[0] = ctx.term.cursor_row;
  ctx.term.saved_cursor[1] = ctx.term.cursor_col;
}

void
gfx_term_restore_cursor()
{
  ctx.term.cursor_row = ctx.term.saved_cursor[0];
  ctx.term.cursor_col = ctx.term.saved_cursor[1];
}

void
gfx_term_clear_till_end()
{
  gfx_clear_rect(ctx.term.cursor_col * ctx.font_width,
                 ctx.term.cursor_row * ctx.font_height,
                 ctx.width,
                 ctx.font_height);
}

void
gfx_term_clear_till_cursor()
{
  gfx_clear_rect(0,
                 ctx.term.cursor_row * ctx.font_height,
                 (ctx.term.cursor_col + 1) * ctx.font_width,
                 ctx.font_height);
}

void
gfx_term_clear_line()
{
  gfx_clear_rect(0, ctx.term.cursor_row * ctx.font_height, ctx.width, ctx.font_height);
}

void
gfx_term_clear_screen()
{
  gfx_clear();
}

void
gfx_term_clear_lines(int from, int to)
{
  if (from < 0)
    from = 0;
  if (to > (int)ctx.term.rows - 1)
    to = ctx.term.rows - 1;
  if (from <= to) {
    gfx_clear_rect(0, from * ctx.font_height, ctx.width, (to - from + 1) * ctx.font_height);
  }
}

/*
 *  Term ansii codes scanner
 *
 */
#define TERM_ESCAPE_CHAR (0x1B)

/*
 * State implementations
 */

void
state_fun_final_letter(char ch, scn_state* state)
{
  if (state->private_mode_char == '#') {
    // Non-standard ANSI Codes
    switch (ch) {
      case 'l':
        /* render line */
        if (state->cmd_params_size == 4) {
          gfx_line(state->cmd_params[0],
                   state->cmd_params[1],
                   state->cmd_params[2],
                   state->cmd_params[3]);
        }
        goto back_to_normal;
        break;
      case 'r':
        /* render a filled rectangle */
        if (state->cmd_params_size == 4) {
          gfx_fill_rect(state->cmd_params[0],
                        state->cmd_params[1],
                        state->cmd_params[2],
                        state->cmd_params[3]);
        }
        goto back_to_normal;
        break;
    }
  }

  switch (ch) {
    case 'l':
      if (state->private_mode_char == '?' && state->cmd_params_size == 1 &&
          state->cmd_params[0] == 25) {
        gfx_term_set_cursor_visibility(0);
      }
      goto back_to_normal;
      break;

    case 'h':
      if (state->private_mode_char == '?' && state->cmd_params_size == 1 &&
          state->cmd_params[0] == 25) {
        gfx_term_set_cursor_visibility(1);
      }
      goto back_to_normal;
      break;

    case 'K':
      if (state->cmd_params_size == 0) {
        gfx_term_clear_till_end();
        goto back_to_normal;
      } else if (state->cmd_params_size == 1) {
        switch (state->cmd_params[0]) {
          case 0:
            gfx_term_clear_till_end();
            goto back_to_normal;
          case 1:
            gfx_term_clear_till_cursor();
            goto back_to_normal;
          case 2:
            gfx_term_clear_line();
            goto back_to_normal;
          default:
            goto back_to_normal;
        }
      }
      goto back_to_normal;
      break;

    case 'J': {
      switch (state->cmd_params_size >= 1 ? state->cmd_params[0] : 0) {
        case 0:
          gfx_term_clear_lines(ctx.term.cursor_row, ctx.term.rows - 1);
          break;

        case 1:
          gfx_term_clear_lines(0, ctx.term.cursor_row);
          break;

        case 2:
          gfx_term_move_cursor(0, 0);
          gfx_term_clear_screen();
          break;
      }

      goto back_to_normal;
      break;
    }

    case 'A': {
      int n = state->cmd_params_size > 0 ? state->cmd_params[0] : 1;
      gfx_term_move_cursor(MAX(0, (int)ctx.term.cursor_row - n),
                           ctx.term.cursor_col);
      goto back_to_normal;
      break;
    }

    case 'B': {
      int n = state->cmd_params_size > 0 ? state->cmd_params[0] : 1;
      gfx_term_move_cursor(
        MIN((int)ctx.term.rows - 1, (int)ctx.term.cursor_row + n),
        ctx.term.cursor_col);
      goto back_to_normal;
      break;
    }

    case 'C': {
      int n = state->cmd_params_size > 0 ? state->cmd_params[0] : 1;
      gfx_term_move_cursor(
        ctx.term.cursor_row,
        MIN((int)ctx.term.columns - 1, (int)ctx.term.cursor_col + n));
      goto back_to_normal;
      break;
    }

    case 'D': {
      int n = state->cmd_params_size > 0 ? state->cmd_params[0] : 1;
      gfx_term_move_cursor(ctx.term.cursor_row,
                           MAX(0, (int)ctx.term.cursor_col - n));
      goto back_to_normal;
      break;
    }

    case 'm': {
      if (state->cmd_params_size == 0)
        gfx_term_reset_attrib();
      else {
        unsigned int i;
        for (i = 0; i < state->cmd_params_size; i++) {
          if (i + 2 < state->cmd_params_size && state->cmd_params[i] == 38 &&
              state->cmd_params[i + 1] == 5) {
            i += 2;
            ctx.fg = state->cmd_params[i];
          } else if (i + 2 < state->cmd_params_size &&
                     state->cmd_params[i] == 48 &&
                     state->cmd_params[i + 1] == 5) {
            i += 2;
            ctx.bg = state->cmd_params[i];
          } else {
            int p = state->cmd_params[i];
            if (p == 0)
              gfx_term_reset_attrib();
            else if (p == 1)
              ctx.fg |= 8;
            else if (p == 2)
              ctx.fg &= 7;
            else if (p == 3 || p == 7)
              ctx.inverse = 1;
            else if (p == 27)
              ctx.inverse = 0;
            else if (p >= 30 && p <= 37)
              ctx.fg = (ctx.fg & 8) | (p - 30);
            else if (p == 39)
              ctx.fg = 15;
            else if (p >= 40 && p <= 47)
              gfx_set_bg(p - 40);
            else if (p == 49)
              gfx_set_bg(7);
          }
        }
      }

      goto back_to_normal;
      break;
    }

    case 'f':
    case 'H': {
      int r = state->cmd_params_size < 1 ? 1 : state->cmd_params[0];
      int c = state->cmd_params_size < 2 ? 1 : state->cmd_params[1];
      gfx_term_move_cursor(r - 1, c - 1);
      goto back_to_normal;
      break;
    }

    case 's':
      gfx_term_save_cursor();
      goto back_to_normal;
      break;

    case 'u':
      gfx_term_restore_cursor();
      goto back_to_normal;
      break;

    case 'c': {
      if (state->cmd_params_size == 0 || state->cmd_params[0] == 0) {
        // according to:
        // https://geoffg.net/Downloads/Terminal/VT100_User_Guide.pdf query
        // terminal type, respond "ESC [?1;Nc" where N is: 0: Base VT100, no
        // options 1: Preprocessor option (STP) 2: Advanced video option (AVO)
        // 3: AVO and STP 4: Graphics processor option (GO) 5: GO and STP 6: GO
        // and AVO 7: GO, STP, and AVO
        char buf[7];
        buf[0] = '\033';
        buf[1] = '[';
        buf[2] = '?';
        buf[3] = '1';
        buf[4] = ';';
        buf[5] = '0';
        buf[6] = 'c';
        uart_write(buf, 7);
      }
      goto back_to_normal;
      break;
    }

    case 'n': {
      char buf[20];
      if (state->cmd_params_size == 1) {
        if (state->cmd_params[0] == 5) {
          // query terminal status (always responde OK)
          buf[0] = '\033';
          buf[1] = '[';
          buf[2] = '0';
          buf[3] = 'n';
          uart_write(buf, 4);
        } else if (state->cmd_params[0] == 6) {
          // query cursor position
          buf[0] = '\033';
          buf[1] = '[';
          b2s(buf + 2, (unsigned char)ctx.term.cursor_row + 1);
          buf[5] = ';';
          b2s(buf + 6, (unsigned char)ctx.term.cursor_col + 1);
          buf[9] = 'R';
          uart_write(buf, 10);
        }
      }

      goto back_to_normal;
      break;
    }

    case '?': {
      goto back_to_normal;
      break;
    }

    default:
      goto back_to_normal;
  }

back_to_normal:
  // go back to normal text
  state->cmd_params_size = 0;
  state->next = state_fun_normaltext;
}

void
state_fun_read_digit(char ch, scn_state* state)
{
  if (ch >= '0' && ch <= '9') {
    // parse digit
    state->cmd_params[state->cmd_params_size - 1] =
      state->cmd_params[state->cmd_params_size - 1] * 10 + (ch - '0');
    state->next = state_fun_read_digit; // stay on this state
    return;
  }

  if (ch == ';') {
    // Another param will follow
    state->cmd_params_size++;
    state->cmd_params[state->cmd_params_size - 1] = 0;
    state->next = state_fun_read_digit; // stay on this state
    return;
  }

  // not a digit, call the final state
  state_fun_final_letter(ch, state);
}

void
state_fun_selectescape(char ch, scn_state* state)
{
  if (ch >= '0' && ch <= '9') {
    // start of a digit
    state->cmd_params_size = 1;
    state->cmd_params[0] = ch - '0';
    state->next = state_fun_read_digit;
    return;
  } else {
    if (ch == '?' || ch == '#') {
      state->private_mode_char = ch;

      // A digit will follow
      state->cmd_params_size = 1;
      state->cmd_params[0] = 0;
      state->next = state_fun_read_digit;
      return;
    }
  }

  // Already at the final letter
  state_fun_final_letter(ch, state);
}

void
state_fun_double(char ch, scn_state* state)
{
  if (ch == '3') {
    // double height top
  } else if (ch == '4') {
    // double height bottom
  } else if (ch == '5') {
    // normal
  }
  state->next = state_fun_normaltext;
}

void
state_fun_waitsquarebracket(char ch, scn_state* state)
{
  if (ch == '[') {
    state->cmd_params[0] = 1;
    state->private_mode_char = 0; /* reset private mode char */
    state->next = state_fun_selectescape;
    return;
  } else if (ch == '#') {
    state->next = state_fun_double;
    return;
  } else if (ch == TERM_ESCAPE_CHAR) {
     // Double ESCAPE prints the ESC character
    gfx_putc(ctx.term.cursor_row, ctx.term.cursor_col, ch);
    ++ctx.term.cursor_col;
  } else if (ch == 'c') {
    // ESC-c resets terminal
    gfx_term_reset_attrib();
    gfx_term_move_cursor(0, 0);
    gfx_term_clear_screen();
  }

  state->next = state_fun_normaltext;
}

void
state_fun_normaltext(char ch, scn_state* state)
{
  if (ch == TERM_ESCAPE_CHAR) {
    state->next = state_fun_waitsquarebracket;
    return;
  }

  gfx_putc(ctx.term.cursor_row, ctx.term.cursor_col, ch);
  ++ctx.term.cursor_col;
}
