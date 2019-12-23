#include "irq.h"

#include "hwutils.h"

rpi_irq_controller_t* pIRQController =
  (rpi_irq_controller_t*)RPI_INTERRUPT_CONTROLLER_BASE;

IntHandler*(_irq_handlers[63]) = { 0 };
void*(_irq_handlers_data[63]) = { 0 };

void
irq_attach_handler(unsigned int irq, IntHandler* phandler, void* pdata)
{
  if (irq < 63) {
    _irq_handlers[irq] = phandler;
    _irq_handlers_data[irq] = pdata;
  }
  enable_irq();
}

unsigned uart_irq_count;

void __attribute__((interrupt("IRQ"))) irq_handler_(void)
{
  // The IRQ handling that was inherited from pigfx is rather messy
  // and does not reliably detect vectored interrupts from the UART.
  // For now, we're just polling the UART (irq 57) for every interrupt
  // received.
  _irq_handlers[57](_irq_handlers_data[57]);

  if (pIRQController->IRQ_basic_pending & (1 << 11) &&
             _irq_handlers[9]) {
    // IRQ 9
    IntHandler* hnd = _irq_handlers[9];
    hnd(_irq_handlers_data[9]);
  }
}
