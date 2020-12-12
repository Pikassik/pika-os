#pragma once

#include <stdint.h>
#include <stddef.h>

typedef enum state {
    TASK_NOT_ALLOCATED = 0,
    TASK_RUNNING       = 1,
    TASK_SLEEPING      = 2,
    TASK_IN_FORK       = 3,
    TASK_WAITING       = 4,
    TASK_TERMINATED    = 5
} state_t;

struct context {
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebx;
};

struct proc_map_segment {
  void* begin;
  void* end;
  struct proc_map_segment* next;
  int flags;
};

int insert_segment(struct proc_map_segment** list, void* begin, void* end, int flags);
void* find_segment(struct proc_map_segment** list, size_t length);
void dump_segment_list(struct proc_map_segment** list);
void destroy_segment_list(struct proc_map_segment** list);

struct task {
    int pid;
    state_t state;
    struct context context;
    struct regs* regs;
    void* phys_kstack;
    void* kstack;
    struct proc_map_segment* proc_map_list;
    uint32_t* pgdir;
    int ticks_remaining;
    int last_child_pid; // for fork
    int parent_pid;     // for wait
    int child_exitcode; // for wait
    int waited_child_pid;
    int exitcode;
};

extern struct task* current_task;

struct task* task_create();
void scheduler_start();
void scheduler_tick(struct regs* regs);
void fork(struct task* parent);
struct task* find_terminated_child(struct task* parent);
void cleanup(struct task* task);
void wakeup(struct task* task, int child_pid, int exitcode);
void reassign_parents(int old_parent, int new_parent);
void reschedule();
