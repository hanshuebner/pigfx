#include "term.h"
#include "gfx.h"

VTerm *term;
VTermScreen *screen;

void
term_init(unsigned int rows, unsigned int columns)
{
  term = vterm_new(rows, columns);

  screen = vterm_obtain_screen(term);

  static VTermScreenCallbacks callbacks =
    {
     .damage = term_damage,
     .movecursor = term_movecursor,
     .moverect = term_moverect
    };

  vterm_screen_set_callbacks(screen, &callbacks, 0);
  vterm_screen_enable_altscreen(screen, 1);
  vterm_screen_reset(screen, 1);
}

int
term_damage(VTermRect rect, __unused void* user)
{
  VTermPos pos;
  for (pos.row = rect.start_row; pos.row < rect.end_row; pos.row++) {
    for (pos.col = rect.start_col; pos.col < rect.end_col; pos.col++) {
      VTermScreenCell cell;
      vterm_screen_get_cell(screen, pos, &cell);
      int color = cell.attrs.bold ? 15 : 7;
      int fg = cell.attrs.reverse ? 0 : color;
      int bg = cell.attrs.reverse ? color : 0;
      gfx_putc(pos.row, pos.col, cell.chars[0], fg, bg);
    }
  }
  return 1;
}

int
term_movecursor(VTermPos position, __unused VTermPos oldPosition, int visible, __unused void* user)
{
  gfx_set_cursor(position.row, position.col, visible);
  return 1;
}

int
term_moverect(VTermRect dest, VTermRect src, __unused void *user)
{
  if (src.start_row < dest.start_row
      || src.start_col < dest.start_col) {
    // scroll down or right, not implemented yet.
    return 0;
  } else {
    gfx_move_rect(src.start_row, src.start_col,
                  dest.start_row, dest.start_col,
                  src.end_row - src.start_row,
                  src.end_col - src.start_col,
                  0);
    return 1;
  }
}
