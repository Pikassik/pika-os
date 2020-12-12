#include "terminal.h"
#include "utils.h"
#include "paging.h"

size_t terminal_column = 0;
uint16_t* terminal_buffer = NULL;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define SCROLL_BUFFER_SIZE (VGA_HEIGHT * 5)

static uint16_t scroll_buffer[SCROLL_BUFFER_SIZE][VGA_WIDTH];
static size_t scroll_buffer_row = 0;
static size_t scroll_buffer_cursor = 0;
static int default_color = VGA_COLOR_WHITE;

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

void set_color(uint8_t color) {
  default_color = color;
}

void sync_with_scroll_buffer(size_t row) {
  for (int i = 0; i < (int)VGA_HEIGHT; ++i) {
    uint16_t* dest = terminal_buffer + i * VGA_WIDTH;
    uint16_t* src =
      scroll_buffer[(row + i) % SCROLL_BUFFER_SIZE];
    memmove(dest, src, sizeof(scroll_buffer[0]));
  }
}

void terminal_initialize(void) {
  terminal_column = 0;
  uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  terminal_buffer = phys2virt((uint16_t*) 0xB8000);
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
  scroll_buffer[y % SCROLL_BUFFER_SIZE][x] = entry;
}

void clear_current_row() {
  uint8_t color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

  for (size_t x = 0; x < VGA_WIDTH; x++) {
    scroll_buffer[scroll_buffer_cursor][x] =
      vga_entry(' ', color);
  }
}

void next_row() {
  terminal_column = 0;
  scroll_buffer_cursor = (scroll_buffer_cursor + 1) % SCROLL_BUFFER_SIZE;
  clear_current_row();
}

void terminal_putchar_color(char c, uint8_t color) {
  if (c == '\n') {
    next_row();
  } else {
    terminal_putentryat(c, color, terminal_column, scroll_buffer_cursor);
    terminal_column++;
    if (terminal_column == VGA_WIDTH) {
      next_row();
    }
  }

  scroll_buffer_row = minus_vga_height(scroll_buffer_cursor);
  sync_with_scroll_buffer(scroll_buffer_row + 1);
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
  terminal_putchar_color(character, default_color);
}
