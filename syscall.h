#pragma once
#include <stdint.h>
#include "isr.h"

typedef uint32_t (*syscall_fn)(struct regs*);
