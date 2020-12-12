#include "syscall.h"
#include "panic.h"
#include "utils.h"
#include "errno.h"
#include "timer.h"
#include "sched.h"
#include "paging.h"
#include "panic.h"

uint32_t syscall_sleep(struct regs* regs) {
  int ticks = regs->ebx;
  if (ticks <= 0) {
    return -EINVAL;
  }

  INTERRUPTS_OFF();
  current_task->ticks_remaining = ticks;
  current_task->state = TASK_SLEEPING;
  reschedule();
  return 0;
}

uint32_t syscall_print(struct regs* regs) {
  INTERRUPTS_OFF();
  printf_(">%d\n", regs->ebx);
  return 0;
}

uint32_t syscall_mmap(struct regs* regs) {
  void* addr = (void*)regs->ebx;
  uint32_t length = regs->ecx;
  uint32_t flags = regs->edx & PT_WRITEABLE; // only PT_WRITEABLE
  INTERRUPTS_OFF();
  uint32_t res = (uint32_t)mmap_task(current_task, addr,
                          NULL,length, flags | PT_USER);
  return res;
}

uint32_t syscall_fork(struct regs* regs) {
  (void)regs;
  INTERRUPTS_OFF();
  current_task->state = TASK_IN_FORK;
  reschedule();

  if (current_task->last_child_pid == -2) {
    current_task->last_child_pid = -1;
    return -1;
  }

  if (current_task->last_child_pid != -1) {
    int res = current_task->last_child_pid;
    current_task->last_child_pid = -1;
    return res;
  } else {
    return 0;
  }
}

uint32_t syscall_wait(struct regs* regs) {
  int* stat_loc = (int*)regs->ebx;
  INTERRUPTS_OFF();
  current_task->state = TASK_WAITING;
  reschedule();
  if (check_ptr_to_userspace(current_task, stat_loc, sizeof(*stat_loc))) {
    *stat_loc = current_task->child_exitcode;
  } else {
    printf_("write to 0x%x\n", stat_loc);
    panic("cant write");
  }
  return current_task->waited_child_pid;
}

uint32_t syscall_exit(struct regs* regs) {
  int status = regs->ebx;
  INTERRUPTS_OFF();
  current_task->state = TASK_TERMINATED;
  current_task->exitcode = status & 0xff;
  reschedule();
  BUG_ON_REACH();
  return 0;
}

uint32_t syscall_panic(struct regs* regs) {
  (void)regs;
  INTERRUPTS_OFF();
  panic("syscall panic");
  return 1;
}

syscall_fn syscall_table[] = {
    [0] = syscall_sleep,
    [1] = syscall_print,
    [2] = syscall_mmap,
    [3] = syscall_fork,
    [4] = syscall_wait,
    [5] = syscall_exit,
    [6] = syscall_panic // for debug
};


void syscall_irq(struct regs* regs) {
  BUG_ON(!is_userspace(regs));
  BUG_ON_NULL(current_task);

  if (regs->eax >= ARRAY_SIZE(syscall_table)) {
    regs->eax = -ENOSYS;
    return;
  }

  regs->eax = syscall_table[regs->eax](regs);
}
