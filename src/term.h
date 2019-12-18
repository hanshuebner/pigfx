#ifndef _term_h
#define _term_h

#include <vterm.h>

int term_damage(VTermRect rect, void* user);
int term_movecursor(VTermPos position, VTermPos oldPosition, int visible, void* user);

#endif
