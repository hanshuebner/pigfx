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

#include "Logging.h"

using namespace std;

using GFX_COL = unsigned char;

class Framebuffer
  : protected Logging
{
public:
  Framebuffer(unsigned int width = 800,
              unsigned int height = 600);

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

  void restore_cursor() { _cursor->restore(); }

  void set_cursor(unsigned int row,
                  unsigned int column,
                  bool visible,
                  bool double_width);

  void process();

  unsigned int width() const { return _width; }
  unsigned int height() const { return _height; }
  unsigned int pitch() const { return _pitch; }
  static unsigned int font_width() { return 10; }
  static unsigned int font_height() { return 20; }

  struct ColorDefinitions {
    uint32_t _background;
    uint32_t _text;
    uint32_t _bold;
    uint32_t _cursor;
  };

private:

  class Cursor {
  public:
    Cursor(Framebuffer* framebuffer, CTimer* timer);

    void process();

    void restore();

    void set(unsigned row, unsigned column, bool visible, bool double_width);

  private:
    const float _blink_freq = 1.5;

    Framebuffer* _framebuffer;
    CTimer* _timer;
    unsigned int _last_activity;
    unsigned int _row;
    unsigned int _column;
    bool _visible;
    bool _double_width;
    uint8_t* _buffer;

    bool _blink_state;

    uint8_t* fb_pointer();
  };

  struct Glyph
  {
    Glyph(const unsigned char c,
          const GFX_COL foreground_color,
          const GFX_COL background_color,
          const VTermScreenCellAttrs attributes);
    Glyph(Framebuffer& framebuffer,
          unsigned row,
          unsigned column,
          bool double_width);
    ~Glyph() { delete _data; }

    unsigned int _width;
    unsigned int _height;
    unsigned char* _data;
  };

  CDMAChannel _channel;
  CTimer* _timer;
  CBcmFrameBuffer* _framebuffer;

  Cursor* _cursor;

  unsigned char* _pfb;
  unsigned int _width;
  unsigned int _height;
  unsigned int _pitch;

  unsigned char* _font_data;

  ColorDefinitions _color_definitions;

  enum ColorIndex {
                   background = 0,
                   normal,
                   bold,
                   blinkNormal,
                   blinkBold,
                   cursor
  };

  void flush();

  void set_xterm_colors();

  void handle_blinking();

  uint8_t* fb_pointer(unsigned x, unsigned y) { return _pfb + y * _pitch + x; }

  using GlyphKey = tuple<char, GFX_COL, GFX_COL, unsigned long>;
  shared_ptr<Glyph> get_glyph(const unsigned char c,
                              const GFX_COL foreground_color,
                              const GFX_COL background_color,
                              const VTermScreenCellAttrs attributes);

  using Cache = LRU::Cache<GlyphKey, shared_ptr<Glyph>>;
  Cache _glyph_cache;
};

