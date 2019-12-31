
#include <cstring>
#include <algorithm>
#include <utility>
#include <memory>
#include <map>

#include <circle/actled.h>
#include <circle/timer.h>

#include "Framebuffer.h"

using namespace std;

unsigned char G_FONT_GLYPHS[] = {
#include "font.inc"
};

using dma_buffer = unsigned int __attribute__((aligned(0x100)))[16];

dma_buffer mem_buff_dma;

const unsigned GLYPH_CACHE_SIZE = 1024;

class ColorBuffer {
public:
  ColorBuffer(GFX_COL color) {
    memset(_buf, color, sizeof(_buf));
  }
  const unsigned char* bytes() const { return (unsigned char*) _buf; }

private:
  char _buf[4];
};

void
Framebuffer::flush()
{
  _channel.Start();
  _channel.Wait();
}

void
Framebuffer::set_xterm_colors()
{
  static const unsigned int xterm_colors[256] = {
    0x000000, 0x800000, 0x008000, 0x808000, 0x000080, 0x800080, 0x008080,
    0xc0c0c0, 0x808080, 0xff0000, 0x00ff00, 0xffff00, 0x0000ff, 0xff00ff,
    0x00ffff, 0xffffff, 0x000000, 0x00005f, 0x000087, 0x0000af, 0x0000df,
    0x0000ff, 0x005f00, 0x005f5f, 0x005f87, 0x005faf, 0x005fdf, 0x005fff,
    0x008700, 0x00875f, 0x008787, 0x0087af, 0x0087df, 0x0087ff, 0x00af00,
    0x00af5f, 0x00af87, 0x00afaf, 0x00afdf, 0x00afff, 0x00df00, 0x00df5f,
    0x00df87, 0x00dfaf, 0x00dfdf, 0x00dfff, 0x00ff00, 0x00ff5f, 0x00ff87,
    0x00ffaf, 0x00ffdf, 0x00ffff, 0x5f0000, 0x5f005f, 0x5f0087, 0x5f00af,
    0x5f00df, 0x5f00ff, 0x5f5f00, 0x5f5f5f, 0x5f5f87, 0x5f5faf, 0x5f5fdf,
    0x5f5fff, 0x5f8700, 0x5f875f, 0x5f8787, 0x5f87af, 0x5f87df, 0x5f87ff,
    0x5faf00, 0x5faf5f, 0x5faf87, 0x5fafaf, 0x5fafdf, 0x5fafff, 0x5fdf00,
    0x5fdf5f, 0x5fdf87, 0x5fdfaf, 0x5fdfdf, 0x5fdfff, 0x5fff00, 0x5fff5f,
    0x5fff87, 0x5fffaf, 0x5fffdf, 0x5fffff, 0x870000, 0x87005f, 0x870087,
    0x8700af, 0x8700df, 0x8700ff, 0x875f00, 0x875f5f, 0x875f87, 0x875faf,
    0x875fdf, 0x875fff, 0x878700, 0x87875f, 0x878787, 0x8787af, 0x8787df,
    0x8787ff, 0x87af00, 0x87af5f, 0x87af87, 0x87afaf, 0x87afdf, 0x87afff,
    0x87df00, 0x87df5f, 0x87df87, 0x87dfaf, 0x87dfdf, 0x87dfff, 0x87ff00,
    0x87ff5f, 0x87ff87, 0x87ffaf, 0x87ffdf, 0x87ffff, 0xaf0000, 0xaf005f,
    0xaf0087, 0xaf00af, 0xaf00df, 0xaf00ff, 0xaf5f00, 0xaf5f5f, 0xaf5f87,
    0xaf5faf, 0xaf5fdf, 0xaf5fff, 0xaf8700, 0xaf875f, 0xaf8787, 0xaf87af,
    0xaf87df, 0xaf87ff, 0xafaf00, 0xafaf5f, 0xafaf87, 0xafafaf, 0xafafdf,
    0xafafff, 0xafdf00, 0xafdf5f, 0xafdf87, 0xafdfaf, 0xafdfdf, 0xafdfff,
    0xafff00, 0xafff5f, 0xafff87, 0xafffaf, 0xafffdf, 0xafffff, 0xdf0000,
    0xdf005f, 0xdf0087, 0xdf00af, 0xdf00df, 0xdf00ff, 0xdf5f00, 0xdf5f5f,
    0xdf5f87, 0xdf5faf, 0xdf5fdf, 0xdf5fff, 0xdf8700, 0xdf875f, 0xdf8787,
    0xdf87af, 0xdf87df, 0xdf87ff, 0xdfaf00, 0xdfaf5f, 0xdfaf87, 0xdfafaf,
    0xdfafdf, 0xdfafff, 0xdfdf00, 0xdfdf5f, 0xdfdf87, 0xdfdfaf, 0xdfdfdf,
    0xdfdfff, 0xdfff00, 0xdfff5f, 0xdfff87, 0xdfffaf, 0xdfffdf, 0xdfffff,
    0xff0000, 0xff005f, 0xff0087, 0xff00af, 0xff00df, 0xff00ff, 0xff5f00,
    0xff5f5f, 0xff5f87, 0xff5faf, 0xff5fdf, 0xff5fff, 0xff8700, 0xff875f,
    0xff8787, 0xff87af, 0xff87df, 0xff87ff, 0xffaf00, 0xffaf5f, 0xffaf87,
    0xffafaf, 0xffafdf, 0xffafff, 0xffdf00, 0xffdf5f, 0xffdf87, 0xffdfaf,
    0xffdfdf, 0xffdfff, 0xffff00, 0xffff5f, 0xffff87, 0xffffaf, 0xffffdf,
    0xffffff, 0x080808, 0x121212, 0x1c1c1c, 0x262626, 0x303030, 0x3a3a3a,
    0x444444, 0x4e4e4e, 0x585858, 0x606060, 0x666666, 0x767676, 0x808080,
    0x8a8a8a, 0x949494, 0x9e9e9e, 0xa8a8a8, 0xb2b2b2, 0xbcbcbc, 0xc6c6c6,
    0xd0d0d0, 0xdadada, 0xe4e4e4, 0xeeeeee
  };

  for (int i = 0; i < 256; i++) {
    _framebuffer->SetPalette32(i, ((xterm_colors[i] & 0xff0000) >> 16) | (xterm_colors[i] & 0x00ff00) | ((xterm_colors[i] & 0x0000ff) << 16));
  }

  _framebuffer->UpdatePalette();
}

Framebuffer::Framebuffer(unsigned int width,
                         unsigned int height)
  :  Logging("Framebuffer"),
     _channel(DMA_CHANNEL_NORMAL),
     _timer(CTimer::Get()),
     _color_definitions({ 0x000000, 0x808080, 0xffffff, 0x0000ff }),
     _glyph_cache(GLYPH_CACHE_SIZE)
{
  _framebuffer = new CBcmFrameBuffer(width, height, 8);
  if (!_framebuffer->Initialize()) {
    log(LogError, "Framebuffer initialization failed");
  }

  unsigned int lines = height / font_height();
  unsigned border_top_bottom = (height - (lines * font_height())) / 2;

  _width = width;
  _height = height - (border_top_bottom * 2);
  _pitch = _framebuffer->GetPitch();

  log(LogDebug,
      "Framebuffer initialized, _width=%u _height=%u lines=%u border_top_bottom=%u _pitch=%u",
      _width, _height, lines, border_top_bottom, _pitch);

  _pfb = reinterpret_cast<unsigned char*>(_framebuffer->GetBuffer() + (border_top_bottom * _pitch));

  _glyph_cache.monitor();

  _framebuffer->SetPalette32(ColorIndex::background, _color_definitions._background);
  _framebuffer->SetPalette32(ColorIndex::normal, _color_definitions._text);
  _framebuffer->SetPalette32(ColorIndex::bold, _color_definitions._bold);
  _framebuffer->SetPalette32(ColorIndex::blinkNormal, _color_definitions._text);
  _framebuffer->SetPalette32(ColorIndex::blinkBold, _color_definitions._bold);
  _framebuffer->SetPalette32(ColorIndex::cursor, _color_definitions._cursor);

  _framebuffer->UpdatePalette();
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
  _cursor->restore();
  unsigned int width = columns * font_width();
  unsigned int height = rows * font_height();

  log(LogDebug, "Move width %u height %u, setting up DMA", width, height);
  _channel.SetupMemCopy2D(fb_pointer(from_column * font_width(), from_row * font_height()),
                          fb_pointer(to_column * font_width(), to_row * font_height()),
                          width, height,
                          _pitch - width, _pitch - width,
                          0);
  log(LogDebug, "Starting DMA");
  flush();
  log(LogDebug, "Save new cursor content");
}

Framebuffer::Glyph::Glyph(const unsigned char c,
                          __unused const GFX_COL _foreground_color,
                          __unused const GFX_COL _background_color,
                          const VTermScreenCellAttrs attributes)
{
  const unsigned font_width = Framebuffer::font_width();
  const unsigned font_height = Framebuffer::font_height();

  _width = font_width * (attributes.dwl ? 2 : 1);
  _height = font_height;

  _data = new unsigned char[_width * _height];

  GFX_COL foreground_color = attributes.bold ? ColorIndex::bold : (attributes.conceal ? ColorIndex::background : ColorIndex::normal);
  if (attributes.blink && !attributes.conceal) {
    foreground_color += 2;
  }
  GFX_COL background_color = 0;
  if (attributes.reverse) {
    swap(foreground_color, background_color);
  }

  unsigned char* p_font_glyph = G_FONT_GLYPHS + c * font_width * font_height;
  unsigned char* p = _data;

  auto get_font_data = [=](unsigned x, unsigned y) {
    if (attributes.underline && y == (font_height - 1)) {
      return foreground_color;
    } else {
      return p_font_glyph[y * font_width + x] ? foreground_color : background_color;
    }
  };

  if (attributes.dwl) {
    switch (attributes.dhl) {
    case 0:
      // Double width
      for (unsigned y = 0; y < font_height; y++) {
        for (unsigned x = 0; x < font_width; x++) {
          const unsigned char color = get_font_data(x, y);
          *p++ = color;
          *p++ = color;
        }
      }
      break;
    case 1:
      // Double width and double height, top half
      for (unsigned y = 0; y < font_height / 2; y++) {
        for (unsigned i = 0; i < 2; i++) {
          for (unsigned x = 0; x < font_width; x++) {
            const unsigned char color = get_font_data(x, y);
            *p++ = color;
            *p++ = color;
          }
        }
      }
      break;
    case 2:
      // Double width and double height, bottom half
      for (unsigned y = font_height / 2; y < font_height; y++) {
        for (unsigned i = 0; i < 2; i++) {
          for (unsigned x = 0; x < font_width; x++) {
            const unsigned char color = get_font_data(x, y);
            *p++ = color;
            *p++ = color;
          }
        }
      }
      break;
    }
  } else {
      for (unsigned y = 0; y < font_height; y++) {
        for (unsigned x = 0; x < font_width; x++) {
          const unsigned char color = get_font_data(x, y);
          *p++ = color;
        }
      }
  }
}

Framebuffer::Glyph::Glyph(Framebuffer& framebuffer,
                          unsigned row,
                          unsigned column,
                          bool double_width)
{
  const unsigned font_width = Framebuffer::font_width();
  const unsigned font_height = Framebuffer::font_height();

  _width = font_width * (double_width ? 2 : 1);
  _height = font_height;
  _data = new unsigned char[_width * _height];

  unsigned char* pb = _data;

  for (unsigned int y = 0; y < font_height; y++) {
    unsigned char* pfb = framebuffer.fb_pointer(column * _width,
                                                row * _height + y);
    for (unsigned int x = 0; x < _width; x++) {
      *pb++ = pfb[x];
    }
  }
}

shared_ptr<Framebuffer::Glyph>
Framebuffer::get_glyph(const unsigned char c,
                       const GFX_COL foreground_color,
                       const GFX_COL background_color,
                       const VTermScreenCellAttrs attributes)
{
  shared_ptr<Glyph> glyph;
  union {
    VTermScreenCellAttrs attrs;
    uint16_t binary;
  } cast_attributes;
  cast_attributes.attrs = attributes;
  const auto key = GlyphKey(c, foreground_color, background_color, cast_attributes.binary);
  if (_glyph_cache.contains(key)) {
    glyph = _glyph_cache.lookup(key);
  } else {
    glyph = make_shared<Glyph>(c, foreground_color, background_color, attributes);
    _glyph_cache.emplace(key, glyph);
  }

  return glyph;
}

void
Framebuffer::putc(const unsigned row,
                  const unsigned column,
                  const unsigned char c,
                  __unused const VTermColor& foreground_color,
                  __unused const VTermColor& background_color,
                  const VTermScreenCellAttrs attributes)
{
  GFX_COL foreground_color_index = ColorIndex::normal;
  GFX_COL background_color_index = ColorIndex::background;
  shared_ptr<Glyph> glyph = get_glyph(c, foreground_color_index, background_color_index, attributes);

  _channel.SetupMemCopy2D(fb_pointer(column * glyph->_width, row * font_height()),
                          glyph->_data,
                          glyph->_width,
                          font_height(),
                          _pitch - glyph->_width);
  flush();
}

void
Framebuffer::set_cursor(unsigned int row,
                        unsigned int column,
                        bool visible,
                        bool double_width)
{
  _cursor->set(row, column, visible, double_width);
}

void
Framebuffer::handle_blinking()
{
  if ((_timer->GetTicks() % HZ) < (HZ / 2)) {
    _framebuffer->SetPalette32(3, _color_definitions._text);
    _framebuffer->SetPalette32(4, _color_definitions._bold);
  } else {
    _framebuffer->SetPalette32(3, _color_definitions._background);
    _framebuffer->SetPalette32(4, _color_definitions._background);
  }

  _framebuffer->UpdatePalette();
}

void
Framebuffer::process()
{
  _cursor->process();
  handle_blinking();
}

Framebuffer::Cursor::Cursor(Framebuffer* framebuffer, CTimer* timer)
  : _framebuffer(framebuffer),
    _timer(timer),
    _last_activity(timer->GetTicks()),
    _row(0),
    _column(0),
    _visible(true),
    _double_width(false),
    _blink_state(false)
{
  _buffer = new uint8_t[Framebuffer::font_height() + Framebuffer::font_width() * 2];
}

uint8_t*
Framebuffer::Cursor::fb_pointer()
{
  return _framebuffer->fb_pointer(_column * font_width() * (_double_width ? 2 : 1),
                                  _row * font_height());
}

void
Framebuffer::Cursor::set(unsigned row, unsigned column, bool visible, bool double_width)
{
  restore();

  _last_activity = _timer->GetTicks();
  _row = row;
  _column = column;
  _visible = visible;
  _double_width = double_width;
}

void
Framebuffer::Cursor::process()
{
  const unsigned blink_period = _blink_freq * HZ;
  const bool blink_state = ((_timer->GetTicks() - _last_activity) % blink_period) < (blink_period / 2);

  if (_blink_state != blink_state) {
    unsigned char* pb = _buffer;
    unsigned char* pfb = fb_pointer();

    for (unsigned y = 0; y < font_height(); y++) {
      for (unsigned x = 0; x < font_width() * (_double_width ? 2 : 1); x++) {
        if (blink_state && _visible) {
          *pb++ = pfb[x];
          pfb[x] = ColorIndex::cursor;
        } else {
          pfb[x] = *pb++;
        }
      }
      pfb += _framebuffer->pitch();
    }

    _blink_state = blink_state;
  }
}

void
Framebuffer::Cursor::restore()
{
  unsigned char* pb = _buffer;
  unsigned char* pfb = fb_pointer();

  if (_blink_state && _visible) {
    for (unsigned y = 0; y < font_height(); y++) {
      for (unsigned x = 0; x < font_width() * (_double_width ? 2 : 1); x++) {
        pfb[x] = *pb++;
      }
      pfb += _framebuffer->pitch();
    }
  }
  _blink_state = false;
}
