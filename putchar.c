#include "terminal.h"
#include "utils.h"

inline void _putchar(char character) {
  terminal_putchar_color(character, VGA_COLOR_WHITE);
}
