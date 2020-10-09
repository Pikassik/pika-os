#include "terminal.h"
#include "utils/string.h"
#include "asm_instrs.h"
#include "utils/printf.h"

size_t terminal_column = 0;
uint16_t* terminal_buffer = NULL;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define SCROLL_BUFFER_SIZE (VGA_HEIGHT * 5)

static uint16_t scroll_buffer[SCROLL_BUFFER_SIZE][VGA_WIDTH];
static size_t scroll_buffer_row = 0;
static size_t scroll_buffer_cursor = 0;

void terminal_cursor_down();

size_t minus_vga_height(int row) {
  row -= VGA_HEIGHT;
  if (row < 0) {
    row += SCROLL_BUFFER_SIZE;
  }
  return row;
}

uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
  return fg | bg << 4;
}

uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t) uc | (uint16_t) color << 8;
}

void sync_with_scroll_buffer(size_t row) {
  for (int i = 0; i < (int)VGA_HEIGHT; ++i) {
    uint16_t* dest = terminal_buffer + i * VGA_WIDTH;
    uint16_t* src =
      scroll_buffer[(row + i) % SCROLL_BUFFER_SIZE];
    memcpy(dest, src, VGA_WIDTH);
  }
}

void terminal_initialize(void) {
  terminal_column = 0;
  uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  terminal_buffer = (uint16_t*) 0xB8000;
  for (size_t y = 0; y < SCROLL_BUFFER_SIZE; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      scroll_buffer[y][x] = vga_entry(' ', color);
    }
  }

  scroll_buffer_row = minus_vga_height(scroll_buffer_cursor);
  sync_with_scroll_buffer(scroll_buffer_row);
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) {
  uint16_t entry = vga_entry(c, color);
  scroll_buffer[scroll_buffer_cursor % SCROLL_BUFFER_SIZE][x] = entry;
}

void clear_current_row() {
  uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

  for (size_t x = 0; x < VGA_WIDTH; x++) {
    scroll_buffer[scroll_buffer_cursor][x] =
      vga_entry(' ', color);
  }
}

void terminal_putchar_color(char c, uint8_t color) {
  switch (c) {
    case '\n': {
      goto next_row;
    }

    default: {
      terminal_putentryat(c, color, terminal_column, scroll_buffer_cursor);
      terminal_column++;
      if (terminal_column == VGA_WIDTH) {
        goto next_row;
      }
      break;
    }

    next_row: {
      terminal_column = 0;
      scroll_buffer_cursor = (scroll_buffer_cursor + 1) % SCROLL_BUFFER_SIZE;
      clear_current_row();
    }
  }

  scroll_buffer_row = minus_vga_height(scroll_buffer_cursor);
  sync_with_scroll_buffer(scroll_buffer_row);
}

void terminal_write(const char* data, size_t size, uint8_t color) {
  for (size_t i = 0; i < size; i++) {
    terminal_putchar_color(data[i], color);
  }
}

void terminal_writestring(const char* data) {
  terminal_write(
    data,
    strlen(data),
    vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK)
  );
}

void terminal_writestring_color(const char* data, uint8_t color) {
  terminal_write(data, strlen(data), color);
}

void terminal_cursor_up() {
  scroll_buffer_row =
    scroll_buffer_row ? scroll_buffer_row - 1 : SCROLL_BUFFER_SIZE - 1;
  sync_with_scroll_buffer(scroll_buffer_row);
}

void terminal_cursor_down() {
  scroll_buffer_row = (scroll_buffer_row + 1) % SCROLL_BUFFER_SIZE;
  sync_with_scroll_buffer(scroll_buffer_row);
}

void _putchar(char character) {
  terminal_putchar_color(character, VGA_COLOR_WHITE);
}

__attribute__ ((interrupt)) void isr0(struct iframe* frame) {
  terminal_writestring_color(
    "Hello world!\n",
    vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK)
  );
  (void)frame;
}

static const unsigned char CURSOR_UP_CODE = 0x48;
static const unsigned char CURSOR_DOWN_CODE = 0x50;

__attribute__ ((interrupt)) void isr9(struct iframe* frame) {
  cli();
  const unsigned char x20 = 0x20;
  asm volatile ("outb %0, $0x20\n\t"::"r"(x20):);

  volatile unsigned char keycode = 0;
  asm volatile ("inb $0x60, %%al\n\t"
                "movb %%al, %0\n\t":"=r"(keycode)::"al");
  if (keycode == CURSOR_DOWN_CODE) {
    terminal_cursor_down();
  } else if (keycode == CURSOR_UP_CODE) {
    terminal_cursor_up();
  } else if (keycode == 0x04) {
    // '3' symbol on keyboard
    printf_("boop\n");
  }

  (void)frame;
  sti();
}