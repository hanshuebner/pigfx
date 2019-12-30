#include <cstring>

#include <sstream>
#include <vector>

#include <circle/serial.h>
#include <circle/devicenameservice.h>

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

Terminal::Terminal(CSerialDevice* serial_port)
  : Logging("Terminal"),
    _serial_port(serial_port),
    _serial_speed(9600)
{
  _framebuffer = make_shared<Framebuffer>();
  _keyboard = make_shared<Keyboard>(this);

  unsigned rows = _framebuffer->height() / _framebuffer->font_height();
  unsigned columns = _framebuffer->width() / _framebuffer->font_width();

  log(LogDebug, "Got %u rows %u columns", rows, columns);

  _term = vterm_new(rows, columns);

  _screen = vterm_obtain_screen(_term);

  memset(&_callbacks, 0, sizeof _callbacks);
  _callbacks.damage = term_damage;
  _callbacks.movecursor = term_movecursor;
  _callbacks.moverect = term_moverect;

  vterm_screen_set_callbacks(_screen, &_callbacks, this);
  vterm_screen_enable_altscreen(_screen, 1);
  vterm_screen_reset(_screen, 1);

  uart_set_speed(_serial_speed);
}

int
Terminal::damage(VTermRect rect)
{
  VTermPos pos;
  for (pos.row = rect.start_row; pos.row < rect.end_row; pos.row++) {
    for (pos.col = rect.start_col; pos.col < rect.end_col; pos.col++) {
      VTermScreenCell cell;
      vterm_screen_get_cell(_screen, pos, &cell);

      _framebuffer->putc(pos.row, pos.col,
                         _unicode_map.to_dec_char(cell.chars[0]),
                         cell.fg, cell.bg, cell.attrs);
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
  log(LogDebug, "moverect called %u/%u:%u/%u -> %u:%u/%u:%u",
      src.start_row, src.start_col, src.end_row, src.end_col,
      dest.start_row, dest.start_col, dest.end_row, dest.end_col);
  return 0;
  
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
Terminal::uart_write(const string& s)
{
  _serial_port->Write(s.c_str(), s.length());
}

void
Terminal::uart_set_speed(unsigned speed)
{
  _serial_port->SetSpeed(speed);
  _serial_speed = speed;
}

Terminal::UnicodeMap::UnicodeMap()
{
  _map[0x0020] = 0x00; // SPACE
  _map[0x25C6] = 0x01; // BLACK DIAMOND
  _map[0x2592] = 0x02; // MEDIUM SHADE (checkerboard)
  _map[0x2409] = 0x03; // SYMBOL FOR HORIZONTAL TAB
  _map[0x240C] = 0x04; // SYMBOL FOR FORM FEED
  _map[0x240D] = 0x05; // SYMBOL FOR CARRIAGE RETURN
  _map[0x240A] = 0x06; // SYMBOL FOR LINE FEED
  _map[0x00B0] = 0x07; // DEGREE SIGN
  _map[0x00B1] = 0x08; // PLUS-MINUS SIGN (plus or minus)
  _map[0x2424] = 0x09; // SYMBOL FOR NEW LINE
  _map[0x240B] = 0x0a; // SYMBOL FOR VERTICAL TAB
  _map[0x2518] = 0x0b; // BOX DRAWINGS LIGHT UP AND LEFT (bottom-right corner)
  _map[0x2510] = 0x0c; // BOX DRAWINGS LIGHT DOWN AND LEFT (top-right corner)
  _map[0x250C] = 0x0d; // BOX DRAWINGS LIGHT DOWN AND RIGHT (top-left corner)
  _map[0x2514] = 0x0e; // BOX DRAWINGS LIGHT UP AND RIGHT (bottom-left corner)
  _map[0x253C] = 0x0f; // BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL (crossing lines)
  _map[0x23BA] = 0x10; // HORIZONTAL SCAN LINE-1
  _map[0x23BB] = 0x11; // HORIZONTAL SCAN LINE-3
  _map[0x2500] = 0x12; // BOX DRAWINGS LIGHT HORIZONTAL
  _map[0x23BC] = 0x13; // HORIZONTAL SCAN LINE-7
  _map[0x23BD] = 0x14; // HORIZONTAL SCAN LINE-9
  _map[0x251C] = 0x15; // BOX DRAWINGS LIGHT VERTICAL AND RIGHT
  _map[0x2524] = 0x16; // BOX DRAWINGS LIGHT VERTICAL AND LEFT
  _map[0x2534] = 0x17; // BOX DRAWINGS LIGHT UP AND HORIZONTAL
  _map[0x252C] = 0x18; // BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
  _map[0x2502] = 0x19; // BOX DRAWINGS LIGHT VERTICAL
  _map[0x2A7D] = 0x1a; // LESS-THAN OR SLANTED EQUAL-TO
  _map[0x2A7E] = 0x1b; // GREATER-THAN OR SLANTED EQUAL-TO
  _map[0x03C0] = 0x1c; // GREEK SMALL LETTER PI
  _map[0x2260] = 0x1d; // NOT EQUAL TO
  _map[0x00A3] = 0x1e; // POUND SIGN
  _map[0x00B7] = 0x1f; // MIDDLE DOT
}

void
Terminal::process()
{
  char buf[1024];
  int serial_bytes_available = _serial_port->Read(buf, sizeof buf);
  if (serial_bytes_available > 0) {
    vterm_input_write(_term, buf, serial_bytes_available);
    _framebuffer->touch();
  } else if (serial_bytes_available < 0) {
    switch (serial_bytes_available) {
    case -SERIAL_ERROR_BREAK:
      log(LogError, "Could not read from serial port (break)");
      break;
    case -SERIAL_ERROR_OVERRUN:
      log(LogError, "Could not read from serial port (overrun)");
      break;
    case -SERIAL_ERROR_FRAMING:
      log(LogError, "Could not read from serial port (framing error)");
      break;
    default:
      log(LogError, "Could not read from serial port (unexpected return code %d)", serial_bytes_available);
    }
  }

  _framebuffer->process();
}

void
Terminal::display_status(const string& s)
{
  ostringstream os;
  os << '\x1b' << "7" << '\x1b' << "[H[" << s << "]" << '\x1b' << "[K" << '\x1b' << "8";
  auto status = os.str();
  vterm_input_write(_term, status.c_str(), status.length());
}

void
Terminal::cycle_serial_speed()
{
  static const vector<unsigned> speeds { 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 };

  auto speed = speeds.begin();
  for (; speed != speeds.end(); speed++) {
    if (*speed == _serial_speed) {
      speed++;
      break;
    }
  }
  if (speed == speeds.end()) {
    speed = speeds.begin();
  }
  uart_set_speed(*speed);
  ostringstream os;
  os << "Serial speed set to " << *speed << " bps";
  display_status(os.str());
}

void
Terminal::toggle_screen_size()
{
  display_status("Not yet implemented");
}
