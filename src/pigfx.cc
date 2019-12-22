
#include <cstring>
#include <memory>
#include <string>

#include "uspi_types.h"

#include <uspi.h>
#include <vterm.h>

#include "console.h"
#include "irq.h"
#include "pigfx_config.h"
#include "timer.h"
#include "uart.h"
#include "hwutils.h"

#include "Terminal.h"
#include "Framebuffer.h"
#include "Keyboard.h"

#ifndef __unused
#define __unused __attribute__((unused))
#endif

using namespace std;

#define GPFSEL1 0x20200004
#define GPSET0 0x2020001C
#define GPCLR0 0x20200028

#define UART_BUFFER_SIZE 16384 /* 16k */

unsigned int led_status;
volatile unsigned int* UART0_DR;
volatile unsigned int* UART0_ITCR;
volatile unsigned int* UART0_IMSC;
volatile unsigned int* UART0_FR;

volatile char* uart_buffer;
volatile char* uart_buffer_start;
volatile char* uart_buffer_end;
volatile char* uart_buffer_limit;

extern "C" void
LogWrite(__unused const char* pSource, __unused unsigned Severity, __unused const char* fmt, ...)
{
}

/* ---------------------------------------------------------------------------------------------------
 */

volatile unsigned int* UART0 = (volatile unsigned int*)0x20201000;

void
uart0_setbaud(unsigned int baud)
{
  unsigned int divider = 3000000 / baud;
  unsigned int f1 = (3000000 % baud) * 64;
  unsigned int fractional = f1 / baud;
  if (f1 - (fractional * baud) >= baud / 2)
    fractional++;

  // unsigned char LCRH = UART0[0x0b];
  UART0[0x0c] &= ~0x01;     // disable UART
  UART0[0x0b] &= ~0x10;     // disable FIFO
  UART0[0x09] = divider;    // set integer divider
  UART0[0x0a] = fractional; // set fractional divider
  UART0[0x0b] |= 0x10;      // enable FIFO
  UART0[0x0c] |= 0x01;      // enable UART
}

unsigned int
uart0_getbaud()
{
  unsigned int f = (UART0[0x09] * 64) + (UART0[0x0a] & 63);
  unsigned int baud = f == 0 ? 0 : (3000000 * 64) / f;
  if ((3000000 * 64) - (baud * f) >= f / 2)
    baud++;
  return baud;
}

/* ---------------------------------------------------------------------------------------------------
 */

static shared_ptr<Terminal> terminal;

void
debug(const char* const s)
{
  if (terminal != nullptr) {
    terminal->output(s, strlen(s));
  }

  uart_write(s, strlen(s));
}

static shared_ptr<Keyboard> keyboard;

static void
keyboard_handler(unsigned char modifiers,
                 const unsigned char raw_keys[6])
{
  keyboard->handle_report(modifiers, raw_keys);
}

static void
_heartbeat_timer_handler(__unused unsigned hnd,
                         __unused void* pParam,
                         __unused void* pContext)
{
  if (led_status) {
    W32(GPCLR0, 1 << 16);
    led_status = 0;
  } else {
    W32(GPSET0, 1 << 16);
    led_status = 1;
  }

  attach_timer_handler(1000 / HEARTBEAT_FREQUENCY, _heartbeat_timer_handler, 0, 0);
}

void
uart_fill_queue(__unused void* data)
{
  while (!(*UART0_FR & 0x10) /*uart_poll()*/) {
    *uart_buffer_end++ = (char)(*UART0_DR & 0xFF /*uart_read_byte()*/);

    if (uart_buffer_end >= uart_buffer_limit)
      uart_buffer_end = uart_buffer;

    if (uart_buffer_end == uart_buffer_start) {
      uart_buffer_start++;
      if (uart_buffer_start >= uart_buffer_limit)
        uart_buffer_start = uart_buffer;
    }
  }

  /* Clear UART0 interrupts */
  *UART0_ITCR = 0xFFFFFFFF;
}

void
initialize_uart_irq()
{
  uart_buffer_start = uart_buffer_end = uart_buffer;
  uart_buffer_limit = &(uart_buffer[UART_BUFFER_SIZE]);

  UART0_DR = (volatile unsigned int*)0x20201000;
  UART0_IMSC = (volatile unsigned int*)0x20201038;
  UART0_ITCR = (volatile unsigned int*)0x20201044;
  UART0_FR = (volatile unsigned int*)0x20201018;

  *UART0_IMSC =
    (1 << 4) | (1 << 7) | (1 << 9); // Masked interrupts: RXIM + FEIM + BEIM
                                    // (See pag 188 of BCM2835 datasheet)
  *UART0_ITCR = 0xFFFFFFFF;         // Clear UART0 interrupts

  pIRQController->Enable_IRQs_2 = RPI_UART_INTERRUPT_IRQ;
  enable_irq();
  irq_attach_handler(57, uart_fill_queue, 0);
}

void
heartbeat_init()
{
  unsigned int ra;
  ra = R32(GPFSEL1);
  ra &= ~(7 << 18);
  ra |= 1 << 18;
  W32(GPFSEL1, ra);

  // Enable JTAG pins
  W32(0x20200000, 0x04a020);
  W32(0x20200008, 0x65b6c0);

  led_status = 0;
}

void
heartbeat_loop()
{
  unsigned int last_time = 0;
  unsigned int curr_time;

  while (1) {
    curr_time = time_microsec();
    if (curr_time - last_time > 500000) {
      if (led_status) {
        W32(GPCLR0, 1 << 16);
        led_status = 0;
      } else {
        W32(GPSET0, 1 << 16);
        led_status = 1;
      }
      last_time = curr_time;
    }
  }
}

extern "C" void heap_init();

void
initialize_hardware()
{
  heap_init();

  // UART buffer allocation
  uart_buffer = (volatile char*)malloc(UART_BUFFER_SIZE);

  uart_init();
  heartbeat_init();

  timers_init();
  attach_timer_handler(HEARTBEAT_FREQUENCY, _heartbeat_timer_handler, 0, 0);

  initialize_uart_irq();

  if (USPiInitialize()) {
    if (USPiKeyboardAvailable()) {
      USPiKeyboardRegisterKeyStatusHandlerRaw(keyboard_handler);
    }
  }
}

extern "C" void
entry_point()
{
  initialize_hardware();

  auto framebuffer = make_shared<Framebuffer>();
  auto keyboard = make_shared<Keyboard>();
  auto terminal = make_shared<Terminal>(framebuffer, keyboard);

  ::terminal = terminal;
  ::keyboard = keyboard;

  while (1) {
    if (uart_buffer_start != uart_buffer_end) {
      const char* p = (const char*) uart_buffer_start;
      uart_buffer_start = uart_buffer_end;
      if (p > uart_buffer_end) {
        terminal->output(p, uart_buffer_limit - p);
        p = (const char*) uart_buffer;
      }
      if (uart_buffer_end > p) {
        terminal->output(p, uart_buffer_end - p);
      }
    }

    uart_fill_queue(0);
    timer_poll();
    framebuffer->handle_cursor();
  }
}
