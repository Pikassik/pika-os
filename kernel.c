#include <stddef.h>
#include <stdint.h>

#include "interrupts.h"
#include "terminal.h"
#include "utils/printf.h"

void kernel_main(void) {
  init_idt();

  terminal_initialize();
  sti();
  asm volatile ("int $0x80");

  terminal_writestring_color(
    "mathematics\n",
    vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLACK)
  );

  for (size_t i = 0; i < 15; ++i) {
    printf_("%d\n", i);
  }

  for(;;) {
    asm volatile ("hlt");
  }
}
