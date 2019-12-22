
#include <cstring>
#include <algorithm>
#include <memory>
#include <map>

#include "dma.h"
#include "hwutils.h"
#include "fb.h"
#include "timer.h"

#include "Framebuffer.h"

using namespace std;

extern unsigned char G_FONT_GLYPHS;

using dma_buffer = unsigned int __attribute__((aligned(0x100)))[16];

dma_buffer mem_buff_dma;

const unsigned GLYPH_CACHE_SIZE = 1024;

class ColorBuffer {
public:
  ColorBuffer(GFX_COL color) {
    memset(_buf, color, sizeof(_buf));
  }
  const unsigned char* bytes() const { return (const unsigned char*) mem_2uncached(_buf); }

private:
  char _buf[4];
};

void
Framebuffer::flush()
{
  dma_execute_queue();

  while (DMA_CHAN0_BUSY)
    ; // Busy wait for DMA to finish
}

Framebuffer::Framebuffer()
  :  _font_height(20),
     _font_width(10),
     _font_data(&G_FONT_GLYPHS),
     _cursor_row(0),
     _cursor_column(0),
     _cursor_color(9),
     _cursor_blink_state(0),
     _glyph_cache(GLYPH_CACHE_SIZE)
{
  dma_init();

  usleep(10000);
  fb_release();

  unsigned int p_w = 800;
  unsigned int p_h = 600;
  unsigned int v_w = p_w;
  unsigned int v_h = p_h;

  fb_init(p_w, p_h, v_w, v_h, 8, (void**)&_full_pfb, &_full_size, &_pitch);

  fb_set_xterm_palette();

  if (fb_get_physical_buffer_size(&p_w, &p_h) != FB_SUCCESS) {
  }

  _full_height = p_h;

  usleep(10000);

  unsigned int lines = _full_height / _font_height;
  unsigned border_top_bottom = (_full_height - (lines * _font_height)) / 2;

  _pfb = _full_pfb + (border_top_bottom * _pitch);
  _width = p_w;
  _height = _full_height - (border_top_bottom * 2);
  _size = _full_size - (border_top_bottom * 2 * _pitch);
}

void
Framebuffer::clear(GFX_COL background_color)
{
  ColorBuffer color_buffer(background_color);

  dma_enqueue_operation(color_buffer.bytes(), _pfb, _size, 0, DMA_TI_DEST_INC);

  flush();
}

void
Framebuffer::save_cursor_content(unsigned int row,
                                 unsigned int column)
{
  // Save framebuffer content that is going to be replaced by the cursor

  unsigned char* pb = _cursor_buffer;
  unsigned char* pfb = fb_pointer(column * _font_width,
                                  row * _font_height);

  for (int y = 0; y < _font_height; y++) {
    for (int x = 0; x < _font_width; x++) {
      *pb++ = pfb[x];
    }
    pfb += _pitch;
  }
}

void
Framebuffer::restore_cursor_content(unsigned int row,
                                    unsigned int column)
{
  // Restore framebuffer content that was overwritten by the cursor
  unsigned char* pb = _cursor_buffer;
  unsigned char* pfb = fb_pointer(column * _font_width,
                                  row * _font_height);

  for (int y = 0; y < _font_height; y++) {
    for (int x = 0; x < _font_width; x++) {
      pfb[x] = *pb++;
    }
    pfb += _pitch;
  }
}

void
Framebuffer::scroll_up(unsigned int start_line,
                       unsigned int end_line,
                       unsigned int lines,
                       GFX_COL background_color)
{
  unsigned int pixels_per_line = _font_height * _pitch;
  unsigned char* from = _pfb + (lines + start_line) * pixels_per_line;
  unsigned char* to = _pfb + start_line * pixels_per_line;
  unsigned int length = (end_line - start_line - lines + 1) * pixels_per_line;

  if (length) {
    dma_enqueue_operation(from,
                          to,
                          length,
                          0,
                          DMA_TI_SRC_INC | DMA_TI_DEST_INC);
  }
  {
    ColorBuffer color_buffer(background_color);

    dma_enqueue_operation(color_buffer.bytes(),
                          _pfb + (end_line - lines + 1) * pixels_per_line,
                          lines * pixels_per_line,
                          0,
                          DMA_TI_DEST_INC);
  }

  flush();
}

void
Framebuffer::scroll_down(unsigned int start_line,
                         unsigned int end_line,
                         unsigned int lines,
                         GFX_COL background_color)
{
  unsigned int pixels_per_line = _font_height * _pitch;
  lines = min(lines, end_line - start_line);
  for (unsigned int line_to_move = end_line - lines; line_to_move >= start_line; line_to_move--) {
    unsigned char* from = _pfb + line_to_move * pixels_per_line;
    unsigned char* to = _pfb + (line_to_move + lines) * pixels_per_line;
    dma_enqueue_operation(from,
                          to,
                          pixels_per_line,
                          0,
                          DMA_TI_SRC_INC | DMA_TI_DEST_INC);
  }
  {
    ColorBuffer color_buffer(background_color);

    dma_enqueue_operation(color_buffer.bytes(),
                          _pfb + start_line * pixels_per_line,
                          lines * pixels_per_line,
                          0,
                          DMA_TI_DEST_INC);
  }
  flush();
}

void
Framebuffer::move_rect(unsigned int from_row,
                       unsigned int from_column,
                       unsigned int to_row,
                       unsigned int to_column,
                       unsigned int rows,
                       unsigned int columns,
                       __unused GFX_COL background_color)
{
  restore_cursor_content(_cursor_row, _cursor_column);
  unsigned int width = columns * _font_width;
  unsigned int height = rows * _font_height;
  dma_enqueue_operation(fb_pointer(from_column * _font_width, from_row * _font_height),
                        fb_pointer(to_column * _font_width, to_row * _font_height),
                        (((height - 1) & 0xFFFF) << 16) | (width & 0xFFFF),
                        ((_pitch - width) & 0xFFFF) << 16 | (_pitch - width), /* bits 31:16 destination stride, 15:0 source stride */
                        DMA_TI_SRC_INC | DMA_TI_DEST_INC | DMA_TI_2DMODE);
  flush();
  save_cursor_content(_cursor_row, _cursor_column);
}

void
Framebuffer::fill_rect(unsigned int x,
                       unsigned int y,
                       unsigned int width,
                       unsigned int height,
                       GFX_COL color)
{
  ColorBuffer color_buffer(color);

  dma_enqueue_operation(color_buffer.bytes(),
                        fb_pointer(x, y),
                        (((height - 1) & 0xFFFF) << 16) | (width & 0xFFFF),
                        ((_pitch - width) & 0xFFFF) << 16, /* bits 31:16 destination stride, 15:0 source stride */
                        DMA_TI_DEST_INC | DMA_TI_2DMODE);
  flush();
}

shared_ptr<Framebuffer::Glyph>
Framebuffer::make_glyph(const Framebuffer::GlyphKey key)
{
  unsigned char* p_glyph = (unsigned char*)(_font_data + (get<0>(key) * _font_width * _font_height));
  auto glyph = make_shared<Glyph>(_font_width * _font_height);
  unsigned char* p = glyph->_data;
  const GFX_COL foreground_color = get<1>(key);
  const GFX_COL background_color = get<2>(key);
  for (int i = 0; i < _font_height * _font_width; i++) {
    *p++ = *p_glyph++ ? foreground_color : background_color;
  }
  return glyph;
}

shared_ptr<Framebuffer::Glyph>
Framebuffer::get_glyph(unsigned char c,
                       GFX_COL foreground_color,
                       GFX_COL background_color)
{
  shared_ptr<Glyph> glyph;
  const auto key = GlyphKey(c, foreground_color, background_color);
  if (_glyph_cache.contains(key)) {
    glyph = _glyph_cache.lookup(key);
  } else {
    glyph = make_glyph(key);
    _glyph_cache.emplace(key, glyph);
  }

  return glyph;
}

void
Framebuffer::putc(unsigned row,
                  unsigned column,
                  unsigned char c,
                  GFX_COL foreground_color,
                  GFX_COL background_color)
{
  unsigned char* p_glyph = get_glyph(c, foreground_color, background_color)->_data;

  if (row == _cursor_row && column == _cursor_column) {
    memcpy(_cursor_buffer, p_glyph, _font_width * _font_height);
  }

  dma_enqueue_operation(p_glyph,
                        fb_pointer(column * _font_width, row * _font_height),
                        (((_font_height - 1) & 0xFFFF) << 16) | (_font_width & 0xFFFF),
                        ((_pitch - _font_width) & 0xFFFF) << 16, /* bits 31:16 destination stride, 15:0 source stride */
                        DMA_TI_SRC_INC | DMA_TI_DEST_INC | DMA_TI_2DMODE);
}

void
Framebuffer::set_cursor(unsigned int row,
                        unsigned int column,
                        __unused unsigned int visible)
{
  restore_cursor_content(_cursor_row, _cursor_column);
  _cursor_row = row;
  _cursor_column = column;
  save_cursor_content(row, column);
}

void
Framebuffer::handle_cursor()
{
  int cursor_blink_state = (time_microsec() % 800000) > 400000;

  if (_cursor_blink_state != cursor_blink_state) {
    unsigned char* pb = _cursor_buffer;
    unsigned char* pfb = fb_pointer(_cursor_column * _font_width,
                                    _cursor_row * _font_height);

    for (int y = 0; y < _font_height; y++) {
      for (int x = 0; x < _font_width; x++) {
        if (cursor_blink_state) {
          pfb[x] = _cursor_color;
        } else {
          pfb[x] = *pb++;
        }
      }
      pfb += _pitch;
    }

    _cursor_blink_state = cursor_blink_state;
  }
}

