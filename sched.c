#include "sched.h"
#include "errno.h"
#include "isr.h"
#include "utils.h"
#include "paging.h"
#include "panic.h"
#include "gdt.h"
#include "panic.h"

#define MAX_TASKS          32
#define KERNEL_STACK_SIZE  4096
#define KERNEL_STACK ((void*)&KERNEL_HIGH[0] - PAGE_SIZE)

int insert_segment(struct proc_map_segment** list, void* begin, void* end, int flags) {
  BUG_ON(begin >= end);

  struct proc_map_segment* prev = NULL;
  struct proc_map_segment* current = *list;
  while (current) {
    if ((current->begin <= begin && begin < current->end) ||
        (current->begin <= end   && end   < current->end)) {
      return EINVAL;
    }

    if (current->begin >= end) {
      break;
    }

    prev = current;
    current = current->next;
  }

  struct proc_map_segment* new_seg = kalloc(PAGE_SIZE);
  new_seg->begin = begin;
  new_seg->end = end;
  new_seg->flags = flags;

  if (prev) {
    prev->next = new_seg;
  } else {
    *list = new_seg;
  }
  new_seg->next = current;

  return 0;
}

void destroy_segment_list(struct proc_map_segment** list) {
  struct proc_map_segment* current = *list;
  while (current) {
    struct proc_map_segment* next = current->next;
    kfree(current, PAGE_SIZE);
    current = next;
  }

  *list = NULL;
}

void dump_segment_list(struct proc_map_segment** list) {
  printf_("/*\\ dumping segment_list:\n");
  struct proc_map_segment* current = *list;
  while (current) {
    printf_("[0x%x:0x%x]\n", current->begin, current->end);
    current = current->next;
  }
  printf_("dump end /*\\\n");
}

void* find_segment(struct proc_map_segment** list, size_t length) {
  void* start = (void*)0;
  length = ROUNDUP_SZ(length);

  struct proc_map_segment* current = *list;
  while (current) {
    if (current->begin >= start + length) {
      return start;
    }

    start = current->end;
    current = current->next;
  }

  if (start + length > KERNEL_STACK) {
    start = (void*)-1;
  }

  return start;
}

static struct task tasks[MAX_TASKS];

// First function in task, will return to __jump_userspace.
static void task_init() {
}

extern void userspace_fn();
extern void __jump_userspace();

static int task_setup(int pid, void* pgdir, void* kstack) {
  struct task* task = &tasks[pid];
  task->pid = pid;
  task->parent_pid = -1;
  task->last_child_pid = -1;
  task->state = TASK_RUNNING;

  if (pgdir) {
    task->pgdir = pgdir;
  } else {
    task->pgdir = kalloc(PAGE_SIZE);
  }
  if (!task->pgdir) {
    return -ENOMEM;
  }

  memset(task->pgdir, '\0', PAGE_SIZE);
  memcpy(&task->pgdir[512],
         &((uint32_t*)phys2virt((void*)read_cr3()))[512],
         PAGE_SIZE / 2
  );
  task->proc_map_list = NULL;
  task->exitcode = -1;

  if (kstack) {
    task->phys_kstack = kstack;
  } else {
    task->phys_kstack = kalloc(PAGE_SIZE);
  }

  task->kstack = mmap_task(task, (void*)&KERNEL_HIGH[0] - PAGE_SIZE, virt2phys(task->phys_kstack),
                           PAGE_SIZE, PT_WRITEABLE /* no user access */);

  // Allocate fake interrupt frame.
  void* esp = task->phys_kstack + KERNEL_STACK_SIZE;
  esp -= sizeof(struct regs);

  // Prepare registers.
  task->regs = esp;
  memset(task->regs, '\0', sizeof(*task->regs));
  memset(&task->context, '\0', sizeof(task->context));
  task->regs->cs = 0x1b;
  task->regs->ds = 0x23;
  task->regs->es = 0x23;
  task->regs->fs = 0x23;
  task->regs->gs = 0x23;
  task->regs->ss = 0x23;
  task->regs->eflags = 1 << 9; // Set IF flag.
  // Prepare kernel context.
  esp -= 4;
  *(uint32_t*)esp = (uint32_t)__jump_userspace;
  esp -= 4;
  *(uint32_t*)esp = (uint32_t)task_init;

  task->context.esp = (uint32_t)esp;
  task->context.ebp = (uint32_t)esp;

  return 0;
}

int task_allocate(struct task** res, void* pgdir, void* kstack) {
  for (size_t i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_NOT_ALLOCATED) {
      int err = task_setup(i, pgdir, kstack);
      if (err != 0) {
        return err;
      }
      *res = &tasks[i];
      return 0;
    }
  }
  return -EAGAIN;
}

extern void __switch_to(void* from, void* to);

static struct context scheduler_context;
struct task* current_task = NULL;

static void switch_to(struct task* task) {
  load_cr3(virt2phys(task->pgdir));
  TSS.ss0 = 0x10;
  TSS.esp0 = (uint32_t)(task->kstack + KERNEL_STACK_SIZE);
  TSS.iopb = 0xffff;
  reload_tss();
  current_task = task;
  __switch_to(&scheduler_context, &task->context);
}

void scheduler_start() {
  // Interrupts are still disabled.
  struct task* init = NULL;
  if (task_allocate(&init, NULL, NULL) != 0) {
    panic("cannot allocate init task");
  }

  uint32_t program_length = (&USERSPACE_END[0] - &USERSPACE_START[0]);
  mmap_task(init, virt2phys(&USERSPACE_START[0]),
            virt2phys(&USERSPACE_START), program_length, PT_USER);
  init->regs->eip = (uint32_t)virt2phys(userspace_fn);
  init->regs->esp =
    (uint32_t)mmap_task(init, (void*)-1, NULL,
                        PAGE_SIZE, PT_WRITEABLE | PT_USER) + (PAGE_SIZE - 4);

  for (;;) {
    int found = 0;
    for (size_t i = 0; i < MAX_TASKS; i++) {
      switch (tasks[i].state) {
        case TASK_RUNNING: {
          switch_to(&tasks[i]);
          if (tasks[i].state == TASK_TERMINATED) {
            reassign_parents(tasks[i].pid, tasks[i].parent_pid);
          }
          found = 1;
          break;
        }
        case TASK_IN_FORK: {
          fork(&tasks[i]);
          found = 1;
          break;
        }
        case TASK_WAITING: {
          struct task* terminated = find_terminated_child(&tasks[i]);
          if (terminated) {
            int child_pid = terminated->pid;
            int exitcode = terminated->exitcode;
            cleanup(terminated);
            wakeup(&tasks[i], child_pid, exitcode);
            found = 1;
          }
          break;
        }
        default: {}
      }
    }

    if (!found) {
      // If we didn't found a runnable task, wait for next interrupt and retry scheduling.
      INTERRUPTS_ON();
      hlt();
      INTERRUPTS_OFF();
    }
  }
}

void scheduler_tick(struct regs* regs) {
  (void)regs;
  for (size_t i = 0; i < MAX_TASKS; i++) {
    if (tasks[i].state == TASK_SLEEPING) {
      tasks[i].ticks_remaining--;
      if (tasks[i].ticks_remaining == 0) {
        tasks[i].state = TASK_RUNNING;
      }
    }
  }

  if (current_task) {
    reschedule();
  }
}

void fork(struct task* parent) {
  INTERRUPTS_MODE_OFF();
  struct task* child;
  void* new_pgdir = kalloc(PAGE_SIZE);
  void* new_kstack = kalloc(PAGE_SIZE);
  if (task_allocate(&child, new_pgdir, new_kstack)) {
    parent->last_child_pid = -2;
    parent->state = TASK_RUNNING;
    kfree(new_pgdir, PAGE_SIZE);
    kfree(new_kstack, PAGE_SIZE);
    INTERRUPTS_MODE_ON();
    return;
  }

  child->context = parent->context;

  // copy list
  struct proc_map_segment* current = parent->proc_map_list;
  while (current) {
    insert_segment(&child->proc_map_list, current->begin, current->end, current->flags);
    current = current->next;
  }

  // copy virtual memory space
  current = parent->proc_map_list;
  while (current) {
    void* begin = current->begin;
    void* end = current->end;
    uint32_t length = (uint32_t)(end - begin);
    void* parent_phys_addr = virt2phys_from_pgdir(parent->pgdir, begin);
    int flags = flags_from_pgdir(parent->pgdir, begin);
    void* child_phys_addr =
      begin != KERNEL_STACK ?
      virt2phys(kalloc((uint32_t)(end - begin))) :
      virt2phys(new_kstack);
    if (begin != KERNEL_STACK) {
      map_continous(child->pgdir, begin, length, child_phys_addr, flags);
    }
    memcpy(phys2virt(child_phys_addr), phys2virt(parent_phys_addr), length);
    current = current->next;
  }

  parent->last_child_pid = child->pid;
  child->parent_pid = parent->pid;
  parent->state = TASK_RUNNING;
  INTERRUPTS_MODE_ON();
}

struct task* find_terminated_child(struct task* parent) {
  for (int i = 0; i < MAX_TASKS; ++i) {
    if (tasks[i].state == TASK_TERMINATED &&
        tasks[i].parent_pid == parent->pid) {
      return &tasks[i];
    }
  }

  return NULL;
}

void cleanup(struct task* task) {
  if (!task) {
    return;
  }

  task->pid = -1;
  task->state = TASK_NOT_ALLOCATED;
  task->context = (struct context){};
  task->regs = NULL;
  task->phys_kstack = NULL;

  free_mapping(task->pgdir, &task->proc_map_list);
  kfree(task->pgdir, PAGE_SIZE);
  task->pgdir = NULL;

  destroy_segment_list(&task->proc_map_list);
  task->proc_map_list = NULL;

  task->ticks_remaining = 0;
  task->parent_pid = -1;
  task->exitcode = -1; // ?
}

void wakeup(struct task* task, int child_pid, int exitcode) {
  task->waited_child_pid = child_pid;
  task->child_exitcode = exitcode;
  task->state = TASK_RUNNING;
}

void reassign_parents(int old_parent, int new_parent) {
  for (int i = 0; i < MAX_TASKS; ++i) {
    if (tasks[i].pid == old_parent) {
      tasks[i].pid = new_parent;
    }
  }
}

void reschedule() {
  BUG_ON_NULL(current_task);

  struct task* old = current_task;
  current_task = NULL;
  __switch_to(&old->context, &scheduler_context);
}
