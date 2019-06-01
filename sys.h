
#ifndef SYS_H_
#define SYS_H_

#include <stddef.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <poll.h>
#include <time.h>
#include <fcntl.h>

#include "def.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wattributes"

#if (!defined(__x86_64__) && !defined(__i386__) && !defined(__ARM_EABI__)) || !defined(__linux__)
#error "This demo won't work on your platform! Linux on i386, x86_64 or ARM-EABI is required!"
#endif

#ifndef AT_EMPTY_PATH
#define AT_EMPTY_PATH (0x1000)
#endif

#ifndef PIPE_BUF
#define PIPE_BUF (0x10000)
#endif
#define PIPE_WILL_BLOCK_BUF (PIPE_BUF + sizeof(uint32_t))

force_inline static void SYS_break(void) {
#if defined(__x86_64__) || defined(__i386__)
    // often an illegal instruction on x86 instead of int3
    asm volatile("int3");
#elif defined(__ARM_EABI__)
    asm volatile("bkpt #0");
#else
    __builtin_trap();
#endif
}

// more repetition than I'd like, but meh
#if defined(__x86_64__)
// D S d r10 r8 r9 (yeah, wat)
#define SYS_syscall0(num) ({\
    register ssize_t __retval asm("rax");\
    asm volatile("push $" STRINGIFY(num) "\npop %%rax\nsyscall\n"\
        :"=a" (__retval) ::"rcx","r11"); \
    __retval;\
})\

#define SYS_syscall1(num, a) ({\
    register ssize_t __retval asm("rax");\
    asm volatile("push $" STRINGIFY(num) "\npop %%rax\nsyscall\n"\
        :"=a" (__retval) \
        : "D" (a) \
        :"rcx","r11"); \
    __retval;\
})\

#define SYS_syscall2(num, a, b) ({\
    register ssize_t __retval asm("rax");\
    asm volatile("push $" STRINGIFY(num) "\npop %%rax\nsyscall\n"\
        :"=a" (__retval) \
        : "D" (a), "S"  (b)\
        :"rcx","r11"); \
    __retval;\
})\

#define SYS_syscall3(num, a, b, c) ({\
    register ssize_t __retval asm("rax");\
    asm volatile("push $" STRINGIFY(num) "\npop %%rax\nsyscall\n"\
        :"=a" (__retval) \
        : "D" (a), "S"  (b), "d" (c)\
        :"rcx","r11"); \
    __retval;\
})\

#define SYS_syscall4(num, a, b, c, d) ({\
    register ssize_t __retval asm("rax");\
    asm volatile( \
        "mov %[fourth], %%r10\n" /* yep, gcc won't let you specify r10 as dest */ \
        "push $" STRINGIFY(num) "\npop %%rax\nsyscall\n"\
        :"=a" (__retval) \
        : "D" (a), "S"  (b), "d" (c), [fourth] "r" ((size_t)d)\
        :"rcx","r11","r10"); \
    __retval;\
})\

#define SYS_syscall5(num, a, b, c, d, e) ({\
    register ssize_t __retval asm("rax");\
    asm volatile( \
        "mov %[fourth], %%r10\n" /* yep, gcc won't let you specify r10 as dest */ \
        "mov %[fifth], %%r8\n" \
        "push $" STRINGIFY(num) "\npop %%rax\n" \
        "syscall\n"\
    :"=a" (__retval) \
    : "D" (a), "S"  (b), "d" (c), [fourth] "c" ((size_t)d), [fifth] "b" ((size_t)e) \
    :"r11","r10","r8"); \
    __retval;\
})\

#define SYS_syscall6(num, a, b, c, d, e, f) ({\
    register ssize_t __retval asm("rax");\
    asm volatile( \
        "mov %[fourth], %%r10\n" /* yep, gcc won't let you specify r10 as dest */ \
        "mov %[fifth], %%r8\n" \
        "mov %[sixth], %%r9\n" \
        "push $" STRINGIFY(num) "\npop %%rax\n" \
        "syscall\n"\
    :"=a" (__retval) \
    : "D" (a), "S"  (b), "d" (c), [fourth] "r" ((size_t)d), [fifth] "r" ((size_t)e), [sixth] "r" ((size_t)f) \
    :"rcx","r11","r10","r8","r9"); \
    __retval;\
})\

#elif defined(__i386__)
// b c d S D
#define SYS_syscall0(num) ({\
    register ssize_t __retval asm("eax");\
    if((num)<128)\
        asm volatile("push $" STRINGIFY(num) "\npop %%eax\nint $0x80\n" \
            :"=a" (__retval)); \
    else\
        asm volatile("mov $"  STRINGIFY(num) ", %%eax\nint $0x80\n"\
            :"=a" (__retval)); \
    __retval; \
})\

#define SYS_syscall1(num, a) ({\
    register ssize_t __retval asm("eax");\
    if ((num)<128)\
        asm volatile("push $" STRINGIFY(num) "\npop %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a):); \
    else\
        asm volatile("mov $"  STRINGIFY(num) ", %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a):); \
    __retval; \
})\

#define SYS_syscall2(num, a, b) ({\
    register ssize_t __retval asm("eax");\
    if((num)<128)\
        asm volatile("push $" STRINGIFY(num) "\npop %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b):); \
    else\
        asm volatile("mov $" STRINGIFY(num) ", %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b):); \
    __retval; \
})\

#define SYS_syscall3(num, a, b, c) ({\
    register ssize_t __retval asm("eax");\
    if((num)<128)\
        asm volatile("push $" STRINGIFY(num) "\npop %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b), "d" (c):); \
    else\
        asm volatile("mov $"  STRINGIFY(num) ", %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b), "d" (c):); \
    __retval; \
})\

#define SYS_syscall4(num, a, b, c, d) ({\
    register ssize_t __retval asm("eax");\
    if((num)<128)\
        asm volatile("push $" STRINGIFY(num) "\npop %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b), "d" (c), "S" (d):); \
    else\
        asm volatile("mov $"  STRINGIFY(num) ", %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b), "d" (c), "S" (d):); \
    __retval; \
})\

#define SYS_syscall5(num, a, b, c, d, e) ({\
    register ssize_t __retval asm("eax");\
    if((num)<128)\
        asm volatile("push $" STRINGIFY(num) "\npop %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b), "d" (c), "S" (d), "D" (e):); \
    else\
        asm volatile("mov $"  STRINGIFY(num) ", %%eax\nint $0x80\n"\
            :"=a" (__retval) :"b" (a), "c" (b), "d" (c), "S" (d), "D" (e):); \
    __retval; \
})\

#define SYS_syscall6(num, a, b, c, d, e, f) ({\
    register ssize_t __retval asm("eax");\
    if((num)<128)\
        asm volatile( \
            "mov %%eax, %%ebp\n" \
            "push $" STRINGIFY(num) "\npop %%eax\n" \
            "int $0x80\n"\
            :"=a" (__retval) \
            :"b" (a), "c" (b), "d" (c), "S" (d), "D" (e), "a" (f) :"ebp"); \
    else\
        asm volatile( \
            "mov %%eax, %%ebp\n" \
            "mov $"  STRINGIFY(num) ", %%eax\n" \
            "int $0x80\n"\
            :"=a" (__retval) \
            :"b" (a), "c" (b), "d" (c), "S" (d), "D" (e), "a" (f) :"ebp"); \
    __retval; \
})\

#elif defined(__ARM_EABI__)
// r0 1 2 3 4 5
#define SYS_syscall0(num) ({\
    register ssize_t __retval asm("r0");\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"=l"(__retval):: "r7"); \
    __retval; \
})\

#define SYS_syscall1(num, a) ({\
    register ssize_t __retval asm("r0") = (ssize_t)(a);\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"+l"(__retval):: "r7"); \
    __retval; \
})\

#define SYS_syscall2(num, a, b) ({\
    register ssize_t __retval asm("r0") = (ssize_t)(a);\
    register ssize_t __bregvl asm("r1") = (ssize_t)(b);\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"+l"(__retval)\
        :"r"(__bregvl)\
        :"r7"); \
    __retval; \
})\

#define SYS_syscall3(num, a, b, c) ({\
    register ssize_t __retval asm("r0") = (ssize_t)(a);\
    register ssize_t __bregvl asm("r1") = (ssize_t)(b);\
    register ssize_t __cregvl asm("r2") = (ssize_t)(c);\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"+l"(__retval)\
        :"r"(__bregvl),"r"(__cregvl)\
        :"r7"); \
    __retval; \
})\

#define SYS_syscall4(num, a, b, c, d) ({\
    register ssize_t __retval asm("r0") = (ssize_t)(a);\
    register ssize_t __bregvl asm("r1") = (ssize_t)(b);\
    register ssize_t __cregvl asm("r2") = (ssize_t)(c);\
    register ssize_t __dregvl asm("r3") = (ssize_t)(d);\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"+l"(__retval)\
        :"r"(__bregvl),"r"(__cregvl),"r"(__dregvl)\
        :"r7"); \
    __retval; \
})\

#define SYS_syscall5(num, a, b, c, d, e) ({\
    register ssize_t __retval asm("r0") = (ssize_t)(a);\
    register ssize_t __bregvl asm("r1") = (ssize_t)(b);\
    register ssize_t __cregvl asm("r2") = (ssize_t)(c);\
    register ssize_t __dregvl asm("r3") = (ssize_t)(d);\
    register ssize_t __eregvl asm("r4") = (ssize_t)(e);\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"+l"(__retval)\
        :"r"(__bregvl),"r"(__cregvl),"r"(__dregvl),"r"(__eregvl)\
        :"r7"); \
    __retval; \
})\

#define SYS_syscall6(num, a, b, c, d, e, f) ({\
    register ssize_t __retval asm("r0") = (ssize_t)(a);\
    register ssize_t __bregvl asm("r1") = (ssize_t)(b);\
    register ssize_t __cregvl asm("r2") = (ssize_t)(c);\
    register ssize_t __dregvl asm("r3") = (ssize_t)(d);\
    register ssize_t __eregvl asm("r4") = (ssize_t)(e);\
    register ssize_t __fregvl asm("r5") = (ssize_t)(f);\
    asm volatile( \
        "mov r7, #" STRINGIFY(num) "\n" \
        "swi #0\n" \
        :"+l"(__retval)\
        :"r"(__bregvl),"r"(__cregvl),"r"(__dregvl),"r"(__eregvl),"r"(__fregvl)\
        :"r7"); \
    __retval; \
})\

#endif /* x86_64/i386/ARM */

// -- syscall numbers

#ifdef __x86_64__
#define SYS_NR_exit 60
#define SYS_NR_exit_group 231
#define SYS_NR_mmap 9
#define SYS_NR_stat 4
#define SYS_NR_fstat 5
#define SYS_NR_write 1
#define SYS_NR_read 0
#define SYS_NR_open 2
#define SYS_NR_close 3
#define SYS_NR_fcntl 72
#define SYS_NR_ioctl 16
#define SYS_NR_munmap 11
#define SYS_NR_clock_gettime 228
#define SYS_NR_clock_nanosleep 230
#define SYS_NR_fork 57
#define SYS_NR_vfork 58
#define SYS_NR_pipe 22
#define SYS_NR_pipe2 293
#define SYS_NR_dup2 33
#define SYS_NR_dup3 292
#define SYS_NR_memfd_create 319
#define SYS_NR_execve 59
#define SYS_NR_execveat 322
#define SYS_NR_poll 7
#define SYS_NR_lseek 8
#define SYS_NR_brk 12
#define SYS_NR_waitid 247
#elif defined(__i386__)
#define SYS_NR_exit 1
#define SYS_NR_exit_group 252
#define SYS_NR_mmap 90
#define SYS_NR_stat 106
#define SYS_NR_fstat 108
#define SYS_NR_write 4
#define SYS_NR_read 3
#define SYS_NR_open 5
#define SYS_NR_close 6
#define SYS_NR_fcntl 55
#define SYS_NR_ioctl 54
#define SYS_NR_munmap 91
#define SYS_NR_clock_gettime 265
#define SYS_NR_clock_nanosleep 267
#define SYS_NR_fork 2
#define SYS_NR_vfork 190
#define SYS_NR_pipe 42
#define SYS_NR_pipe2 331
#define SYS_NR_dup2 63
#define SYS_NR_dup3 330
#define SYS_NR_memfd_create 356
#define SYS_NR_execve 11
#define SYS_NR_execveat 358
#define SYS_NR_poll 168
#define SYS_NR_lseek 19
#define SYS_NR_brk 45
#define SYS_NR_waitid 284
#define SYS_NR_waitpid 7
#else /* ARM */
#define SYS_NR_exit 1
#define SYS_NR_exit_group 231
#define SYS_NR_mmap 192 /* NOTE: ARM EABI linux supports mmap2 only! */
#define SYS_NR_stat 106
#define SYS_NR_fstat 108
#define SYS_NR_write 4
#define SYS_NR_read 3
#define SYS_NR_open 5
#define SYS_NR_close 6
#define SYS_NR_fcntl 55
#define SYS_NR_ioctl 54
#define SYS_NR_munmap 91
#define SYS_NR_clock_gettime 263
#define SYS_NR_clock_nanosleep 265
#define SYS_NR_fork 2
#define SYS_NR_vfork 190
#define SYS_NR_pipe 42
#define SYS_NR_pipe2 359
#define SYS_NR_dup2 63
#define SYS_NR_dup3 358
#define SYS_NR_memfd_create 385
#define SYS_NR_execve 11
#define SYS_NR_execveat 387
#define SYS_NR_poll 168
#define SYS_NR_lseek 19
#define SYS_NR_brk 45
#define SYS_NR_waitid 280
#endif

// --- actual syscalls

SYSCALL_FUNC ssize_t SYS_stat(const char* path, struct stat* st) {
    register ssize_t r = SYS_syscall2(SYS_NR_stat, path, st);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_fstat(int fd, struct stat* st) {
    register ssize_t r = SYS_syscall2(SYS_NR_fstat, fd, st);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_write(int fd, const void* buf, size_t sz) {
    return SYS_syscall3(SYS_NR_write, fd, buf, sz);
}
SYSCALL_FUNC ssize_t SYS_read(int fd, void* buf, size_t sz) {
    register ssize_t r = SYS_syscall3(SYS_NR_read, fd, buf, sz);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC int SYS_open(const char* path, int flags) {
    return (int)SYS_syscall2(SYS_NR_open, path, flags);
}
SYSCALL_FUNC int SYS_close(int fd) {
    return (int)SYS_syscall1(SYS_NR_close, fd);
}
SYSCALL_FUNC ssize_t SYS_fcntl(int fd, size_t cmd, size_t arg) {
    return SYS_syscall3(SYS_NR_fcntl, fd, cmd, arg);
}
SYSCALL_FUNC ssize_t SYS_ioctl(int fd, size_t cmd, void* arg) {
    return SYS_syscall3(SYS_NR_ioctl, fd, cmd, arg);
}
SYSCALL_FUNC ssize_t SYS_munmap(void* addr, size_t sz) {
    register ssize_t r = SYS_syscall2(SYS_NR_munmap, addr, sz);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_clock_gettime(clockid_t clkid, struct timespec* t) {
    register ssize_t r = SYS_syscall2(SYS_NR_clock_gettime, clkid, t);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_clock_nanosleep(clockid_t clkid, int flags,
        const struct timespec* t, struct timespec* remain) {
    register ssize_t r = SYS_syscall4(SYS_NR_clock_nanosleep, clkid, flags, t, remain);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC int SYS_fork(void) {
    return SYS_syscall0(SYS_NR_fork);
}
SYSCALL_FUNC int SYS_vfork(void) {
    return SYS_syscall0(SYS_NR_vfork);
}
SYSCALL_FUNC ssize_t SYS_pipe(int fd[2]) {
    register ssize_t r = SYS_syscall1(SYS_NR_pipe, (int*)fd);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_pipe2(int fd[2], int flags) {
    register ssize_t r = SYS_syscall2(SYS_NR_pipe2, (int*)fd, flags);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_dup2(int oldfd, int newfd) {
    return SYS_syscall2(SYS_NR_dup2, oldfd, newfd);
}
SYSCALL_FUNC ssize_t SYS_dup3(int oldfd, int newfd, int flags) {
    return SYS_syscall3(SYS_NR_dup3, oldfd, newfd, flags);
}
SYSCALL_FUNC int SYS_memfd_create(const char* name, unsigned int flags) {
    return (int)SYS_syscall2(SYS_NR_memfd_create, name, flags);
}
SYSCALL_FUNC void SYS_execve(const char* file, const char * const* argv
        /*char*const argv[]*/, char*const envp[]) {
    SYS_syscall3(SYS_NR_execve, file, argv, envp);
    __builtin_unreachable();
}
SYSCALL_FUNC ssize_t SYS_poll(struct pollfd* fds, nfds_t nfds, int timeout) {
    register ssize_t r = SYS_syscall3(SYS_NR_poll, fds, nfds, timeout);
    MEMORY_BARRIER();
    return r;
}
SYSCALL_FUNC ssize_t SYS_lseek(int fd, off_t offset, int whence) {
    return SYS_syscall3(SYS_NR_lseek, fd, offset, whence);
}
SYSCALL_FUNC void SYS_execveat(int dirfd, const char* path, char*const argv[],
        char*const envp[], int flags) {
    SYS_syscall5(SYS_NR_execveat, dirfd, path, argv, envp, flags);
    __builtin_unreachable();
}
SYSCALL_FUNC void SYS_exit(int c) {
    SYS_syscall1(SYS_NR_exit, c);
    __builtin_unreachable();
}
SYSCALL_FUNC void SYS_exit_group(int c) {
    SYS_syscall1(SYS_NR_exit_group, c);
    __builtin_unreachable();
}
SYSCALL_FUNC ssize_t SYS_brk(void* p) {
    return SYS_syscall1(SYS_NR_brk, p);
}
SYSCALL_FUNC ssize_t SYS_waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options) {
    return SYS_syscall4(SYS_NR_waitid, idtype, id, infop, options);
}
#ifdef __i386__
SYSCALL_FUNC ssize_t SYS_waitpid(pid_t pid, int* wstat, int opt) {
    return SYS_syscall3(SYS_NR_waitpid, pid, wstat, opt);
}
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
__attribute__((__deprecated__("waitpid(2) isn't directly available as a "\
    "syscall on this platform. Consider using waitid(2) instead.")))
force_inline static ssize_t SYS_waitpid(pid_t pid, int* wstat, int opt) {
#pragma GCC diagnostic pop
    // TODO: write to wstat
    siginfo_t si;
    ssize_t r = SYS_waitid(pid <= (si.si_pid = 0) ? P_ALL : P_PID, pid, &si, opt);
    return r < 0 ? r : si.si_pid;
}
#endif

// --- special cases: mmap

// go home GCC, you're drunk
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
// reduced version because reasons
SYSCALL_FUNC void* SYS_mmap(void* addr, size_t len, int filed) {
#pragma GCC diagnostic pop
    register ssize_t retval
#if defined(__x86_64__)
        asm("rax");

    asm volatile("mov %[filedr], %%r8\n"
                 "mov %%r8, %%rdx\n"
                 "mov %%rdx, %%r10\n"
                 "xorl %%r9, %%r9\n"
                 "push $9\n"
                 "pop %%rax\n"
                 "syscall\n"
        :"=a" (retval)
        :"D" (addr), "S" (len), [filedr] "r" (filed)
        :"rcx","r11","r10","r8","r9");
#elif defined(__i386__)
        asm("eax");

    asm volatile("push %%ebx\n"
                 "xorl %%ebx, %%ebx\n"
                 "push %%ebx\n"
                 "push %%edx\n" // edx == 3
                 "push %%edx\n" // 1|_ -> MAP_SHARED
                 "push %%edx\n" // 3 -> PROT_READ|PROT_WRITE
                 "push %%eax\n"
                 "push %%ebx\n"
                 "push $90\n"
                 "pop %%eax\n"
                 "mov %%esp, %%ebx\n"
                 "int $0x80\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
                 "pop %%ebx\n"
        :"=a" (retval)
        :"a" (len), "d" (filed)
        :);
#else /* ARM */
    =0;
/*#error "ARM mmap is TODO"*/
#endif

    return (void*)retval;
}

#pragma GCC diagnostic pop
#pragma GCC diagnostic pop

#endif /* SYS_H_ */

