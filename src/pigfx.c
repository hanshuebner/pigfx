
#include <uspi.h>
#include <vterm.h>

#include "console.h"
#include "dma.h"
#include "ee_printf.h"
#include "framebuffer.h"
#include "gfx.h"
#include "irq.h"
#include "nmalloc.h"
#include "pigfx_config.h"
#include "timer.h"
#include "uart.h"
#include "term.h"
#include "hwutils.h"

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

extern unsigned int pheap_space;
extern unsigned int heap_sz;

VTerm *term;
VTermScreen *screen;

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

static void
_keypress_handler(const char* str)
{
  const char* c = str;

  while (*c) {
    uart_write(c++, 1);
  }
}

static void
_heartbeat_timer_handler(__attribute__((unused)) unsigned hnd,
                         __attribute__((unused)) void* pParam,
                         __attribute__((unused)) void* pContext)
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
uart_fill_queue(__attribute__((unused)) void* data)
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

void
initialize_framebuffer()
{
  usleep(10000);
  fb_release();

  unsigned char* p_fb = 0;
  unsigned int fbsize;
  unsigned int pitch;

  unsigned int p_w = 800;
  unsigned int p_h = 600;
  unsigned int v_w = p_w;
  unsigned int v_h = p_h;

  fb_init(p_w, p_h, v_w, v_h, 8, (void*)&p_fb, &fbsize, &pitch);

  fb_set_xterm_palette();

  if (fb_get_physical_buffer_size(&p_w, &p_h) != FB_SUCCESS) {
  }

  usleep(10000);

  gfx_set_env(p_fb, v_w, v_h, pitch, fbsize);

  // fixme hard coded font size
  term = vterm_new(p_w / 10, p_h / 20);

  gfx_fill_rect(100, 100, 100, 100, 9);

  screen = vterm_obtain_screen(term);

  gfx_fill_rect(200, 200, 100, 100, 9);

  static VTermScreenCallbacks callbacks =
    {
     .damage = term_damage,
     .movecursor = term_movecursor
    };
  
  gfx_fill_rect(300, 300, 100, 100, 9);

  vterm_screen_set_callbacks(screen, &callbacks, 0);
  vterm_screen_enable_altscreen(screen, 1);
  vterm_screen_reset(screen, 1);
}

void
term_main_loop()
{
  while (1) {
    if (uart_buffer_start != uart_buffer_end) {
      const char* p = (const char*) uart_buffer_start;
      uart_buffer_start = uart_buffer_end;
      if (p > uart_buffer_end) {
        vterm_input_write(term, p, uart_buffer_limit - p);
        p = (const char*) uart_buffer;
      }
      if (uart_buffer_end > p) {
        vterm_input_write(term, p, uart_buffer_end - p);
      }
    }

    uart_fill_queue(0);
    timer_poll();
    gfx_handle_cursor();
  }
}

void
term_initialize()
{
  // Heap init
  nmalloc_set_memory_area((unsigned char*)(pheap_space), heap_sz);

  // UART buffer allocation
  uart_buffer = (volatile char*)nmalloc_malloc(UART_BUFFER_SIZE);

  uart_init();
  heartbeat_init();

  initialize_framebuffer();

  timers_init();
  attach_timer_handler(HEARTBEAT_FREQUENCY, _heartbeat_timer_handler, 0, 0);

  initialize_uart_irq();

  if (USPiInitialize()) {
    if (USPiKeyboardAvailable()) {
      USPiKeyboardRegisterKeyPressedHandler(_keypress_handler);
    }
  } else {
    ee_printf("USB initialization failed.\n");
  }
}

void
entry_point()
{
  term_initialize();
  
  term_main_loop();
}
