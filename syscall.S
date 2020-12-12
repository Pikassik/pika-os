    .section .userspace.text, "ax"
    .global userspace_fn
    .type userspace_fn, @function
userspace_fn:
    # tests that stack exists and writeable
    mov $4042, %eax

    push %eax
    push %eax
    push %eax
    push %eax
    push %eax
    push %eax
    push %eax
    push %eax
    # print value from stack
    pop %ebx
    mov $1, %eax
    int $0x80 # 4042
    # check stack is still correct
    pop %ebx
    mov $1, %eax
    int $0x80 # 4042
    #do mmap (writeable)
    mov $2, %eax
    mov $0xffffffff, %ebx
    mov $4096, %ecx
    mov $2, %edx
    int $0x80
    #check we can write
    movl $12345, (%eax)
    #check we can read
    mov (%eax), %ebx
    #check value is correct (print 12345)
    mov $0x1, %eax
    int $0x80

    mov $3, %eax
    int $0x80
    push %eax
    push %eax
    # print child pid
    mov %eax, %ebx
    mov $1, %eax
    int $0x80
    # sleep
#    mov $0x0, %eax
#    mov $100, %ebx
#    int $0x80
    pop %eax
    cmp $0, %eax
    jne .parent
.child:
    # exit with status 152
    mov $0x5, %eax
    mov $152, %ebx
    int $0x80
.parent:
    # wait
    mov $0x4, %eax
    # | save status on stack
    push %eax
    mov %esp, %ebx
    int $0x80
    mov (%esp), %ebx
    pop %eax
    mov $1, %eax
    int $0x80

# XXXXXXXXXXXXXXxx
.l2:
    # fork
    mov $3, %eax
    int $0x80
    # save child pid
    push %eax
    push %eax
    push %eax
    # print child pid
    mov %eax, %ebx
    mov $1, %eax
    int $0x80
    # sleep
    mov $0x0, %eax
    mov $1000, %ebx
    int $0x80
    # check child pid
    pop %eax
    mov %eax, %ebx
    mov $1, %eax
    int $0x80
    pop %eax
    mov %eax, %ecx
    # | if limit exceeded then exit
    cmp $-1, %eax
    je .exit
    # | if proc is child then fork again
    cmp $0, %eax
    je .l2
    # | if proc is parent then wait child
    mov $0x4, %eax
    # | save status on stack
    push %eax
    mov %esp, %ebx
    int $0x80
    # print status
    mov (%esp), %ebx
    pop %eax
    mov $1, %eax
    int $0x80
    jmp .l2
.exit:
    # | if proc forked proc with pid 1 then fork again
    cmp $1, %eax
    je .l2
    # | else exit with status 152
    mov $0x5, %eax
    mov $152, %ebx
    int $0x80
    # unreachable
    mov $6, %eax
    int $0x80
