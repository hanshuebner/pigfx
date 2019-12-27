// -*- C++ -*-

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <lru/lru.hpp>
#pragma GCC diagnostic pop

#include <circle/dmachannel.h>
#include <circle/bcmframebuffer.h>

#include <iostream>

#include <vterm.h>

using namespace std;

using GFX_COL = unsigned char;

class Framebuffer
{
public:
  Framebuffer(unsigned int width = 800,
              unsigned int height = 600);

  void clear(GFX_COL background_color);

  void putc(const unsigned row,
            const unsigned column,
            const unsigned char c,
            const VTermColor& foreground_color,
            const VTermColor& background_color,
            const VTermScreenCellAttrs attributes);

  void move_rect(unsigned int from_row,
                 unsigned int from_column,
                 unsigned int to_row,
                 unsigned int to_column,
                 unsigned int rows,
                 unsigned int columns,
                 GFX_COL background_color);

  void fill_rect(unsigned int x,
                 unsigned int y,
                 unsigned int width,
                 unsigned int height,
                 GFX_COL color);

  void set_cursor(unsigned int row,
                  unsigned int column,
                  __unused unsigned int visible);

  void handle_cursor();

  void flush();
  void touch();

  unsigned int width() const { return _width; }
  unsigned int height() const { return _height; }
  unsigned int font_width() const { return _font_width; }
  unsigned int font_height() const { return _font_height; }

private:
  CDMAChannel _channel;
  CTimer* _timer;
  CBcmFrameBuffer* _framebuffer;

  unsigned char* _pfb;
  unsigned int _width;
  unsigned int _height;
  unsigned int _pitch;

  int _font_height;
  int _font_width;
  unsigned char* _font_data;

  const float cursor_blink_freq = 1.5;
  unsigned int _cursor_row;
  unsigned int _cursor_column;
  unsigned int _cursor_color;
  unsigned char _cursor_buffer[10 * 20];
  bool _cursor_blink_state;
  unsigned int _last_activity;

  void set_xterm_colors();

  void save_cursor_content(unsigned int row,
                           unsigned int column);
  void restore_cursor_content(unsigned int row,
                              unsigned int column);

  void scroll_up(unsigned int start_line,
                 unsigned int end_line,
                 unsigned int lines,
                 GFX_COL background_color);
  void scroll_down(unsigned int start_line,
                   unsigned int end_line,
                   unsigned int lines,
                   GFX_COL background_color);

  unsigned char* fb_pointer(unsigned x, unsigned y) { return _pfb + y * _pitch + x; }

  struct Glyph
  {
    Glyph(unsigned int width, unsigned int height);
    ~Glyph() { delete _data; }
    unsigned int _width;
    unsigned int _height;
    unsigned char* _data;
  };

  using GlyphKey = tuple<char, GFX_COL, GFX_COL, unsigned long>;
  shared_ptr<Glyph> make_glyph(const unsigned char c,
                               const GFX_COL foreground_color,
                               const GFX_COL background_color,
                               const VTermScreenCellAttrs attributes);
  shared_ptr<Glyph> get_glyph(const unsigned char c,
                              const GFX_COL foreground_color,
                              const GFX_COL background_color,
                              const VTermScreenCellAttrs attributes);

  using Cache = LRU::Cache<GlyphKey, shared_ptr<Glyph>>;
  Cache _glyph_cache;
};

