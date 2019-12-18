#include "term.h"
#include "gfx.h"

int
term_damage(VTermRect rect, void* user)
{
  return 1;
}

int
term_movecursor(VTermPos position, __unused VTermPos oldPosition, int visible, __unused void* user)
{
  gfx_set_cursor(position.row, position.col, visible);
  return 1;
}
