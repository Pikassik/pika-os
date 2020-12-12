#include <stddef.h>
#include <stdint.h>

#include "asm_instrs.h"
#include "interrupts.h"
#include "terminal.h"
#include "utils/printf.h"

static const char WELCOME[] =
"Welcome to pika operation system!\n\n ";
static const char LOGO[] =
"       \\:.             .:/\n "
"        \\``._________.''/ \n "
"         \\             / \n "
" .--.--, / .':.   .':. \\\n "
"/__:  /  | '::' . '::' |\n "
"   / /   |`.   ._.   .'|\n "
"  / /    |.'         '.|\n "
" /___-_-,|.\\  \\   /  /.|\n "
"      // |''\\.;   ;,/ '|\n "
"      `==|:=         =:|\n "
"         `.          .'\n "
"           :-._____.-:\n "
"          `''       `''\n";

void kernel_main(void) {
  init_idt();

  terminal_initialize();

  set_color(VGA_COLOR_CYAN);
  printf_(WELCOME);
  set_color(VGA_COLOR_RED);
  printf_(LOGO);
  set_color(VGA_COLOR_WHITE);

  for (size_t i = 0; i < 15; ++i) {
    printf_("%d\n", i);
  }

  sti();
  asm volatile ("int $0x80");

  for(;;) {
    asm volatile ("hlt");
  }
}
