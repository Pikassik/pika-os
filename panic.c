#include "panic.h"
#include "utils.h"

void panic(const char* msg) {
  cli();
  printf_("\n/xxxxx KERNEL PANIC xxxxx\\\n");
  printf_("panic called from = 0x%x\n", __builtin_return_address(0));
  printf_("message: \n%s\n", msg);
  while(1) {
    hlt();
  }
}
