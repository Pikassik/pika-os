#include "interrupts.h"
#include "terminal.h"
#include "utils.h"
#include "panic.h"
#include "acpi.h"
#include "apic.h"
#include "gdt.h"
#include "detect_memory.h"
#include "paging.h"
#include "sched.h"

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
  cli();
  init_gdt();
  init_idt();

  terminal_initialize();

  detect_memory();
  init_kalloc_early();
//  panic("aaaaaaaa");
  init_kernel_paging();
  struct acpi_sdt* rsdt = acpi_find_rsdt();
  if (!rsdt) {
    panic("RSDT not found!");
  }

  apic_init(rsdt);

  set_color(VGA_COLOR_CYAN);
  printf_(WELCOME);
  set_color(VGA_COLOR_RED);
  printf_(LOGO);
  set_color(VGA_COLOR_WHITE);

  sti();


  scheduler_start();
  BUG_ON_REACH();
}
