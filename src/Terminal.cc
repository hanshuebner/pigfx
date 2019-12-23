#include <sstream>

#include "Terminal.h"

static int
term_damage(VTermRect rect, void* terminal)
{
  return reinterpret_cast<Terminal*>(terminal)->damage(rect);
}

static int
term_movecursor(VTermPos position, VTermPos oldPosition, int visible, void* terminal)
{
  return reinterpret_cast<Terminal*>(terminal)->movecursor(position, oldPosition, visible);
}

static int
term_moverect(VTermRect dest, VTermRect src, void* terminal)
{
  return reinterpret_cast<Terminal*>(terminal)->moverect(dest, src);
}

Terminal::Terminal(shared_ptr<Framebuffer> framebuffer,
                   shared_ptr<Keyboard> keyboard)
  : _framebuffer(framebuffer),
    _keyboard(keyboard)
{
  unsigned rows = framebuffer->height() / framebuffer->font_height();
  unsigned columns = framebuffer->width() / framebuffer->font_width();

  _term = vterm_new(rows, columns);

  _screen = vterm_obtain_screen(_term);

  _callbacks.damage = term_damage;
  _callbacks.movecursor = term_movecursor;
  _callbacks.moverect = term_moverect;

  vterm_screen_set_callbacks(_screen, &_callbacks, this);
  vterm_screen_enable_altscreen(_screen, 1);
  vterm_screen_reset(_screen, 1);
}

int
Terminal::damage(VTermRect rect)
{
  VTermPos pos;
  for (pos.row = rect.start_row; pos.row < rect.end_row; pos.row++) {
    for (pos.col = rect.start_col; pos.col < rect.end_col; pos.col++) {
      VTermScreenCell cell;
      vterm_screen_get_cell(_screen, pos, &cell);
      _framebuffer->putc(pos.row, pos.col, cell.chars[0], cell.fg, cell.bg, cell.attrs);
    }
  }
  _framebuffer->flush();
  return 1;
}

int
Terminal::movecursor(VTermPos position, __unused VTermPos oldPosition, int visible)
{
  _framebuffer->set_cursor(position.row, position.col, visible);
  return 1;
}

int
Terminal::moverect(VTermRect dest, VTermRect src)
{
  if (src.start_row < dest.start_row
      || src.start_col < dest.start_col) {
    // scroll down or right, not implemented yet.
    return 0;
  } else {
    _framebuffer->move_rect(src.start_row, src.start_col,
                            dest.start_row, dest.start_col,
                            src.end_row - src.start_row,
                            src.end_col - src.start_col,
                            0);
    return 1;
  }
}

void
Terminal::output(const char* const s, unsigned length)
{
  vterm_input_write(_term, s, length);
}
