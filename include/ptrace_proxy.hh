#pragma once

/**
 * 将 ptrace 系统调用封装一层
 */


#include <sys/ptrace.h>
#include <sys/user.h>
#include <signal.h>
#include <cstdint>


namespace BitTech {

class PtraceProxy {
public:
    // 唯一一个被 tracee 调用，标志自己被 trace
    static auto trace_me() -> void {
        ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
    }

    // 不发送信号的继续执行 tracee
    static auto continue_tracee(pid_t pid) -> void {
        ptrace(PTRACE_CONT, pid, nullptr, nullptr);
    }

    // 发送 signo 信号给 tracee 以继续执行
    static auto delivery_signal_tracee(pid_t pid, int signo) -> void {
        ptrace(PTRACE_CONT, pid, nullptr, signo);
    }

    // 读取 addr 地址处的内存数据，共读取 64 Bit
    static auto read_memory(pid_t pid, std::intptr_t addr) -> uint64_t {
        return ptrace(PTRACE_PEEKDATA, pid, addr, nullptr);
    }

    // 设置 addr 地址处的内存数据，设置为 data，共设置 64 Bit
    static auto write_memory(pid_t pid, std::intptr_t addr, uint64_t data) -> void {
        ptrace(PTRACE_POKEDATA, pid, addr, data);
    }

    // 获取所有通用寄存器内容，struct user_regs_struct 结构见 /usr/include/sys/user.h 文件
    static auto get_registers(pid_t pid) -> struct user_regs_struct {
        struct user_regs_struct regs;
        ptrace(PTRACE_GETREGS, pid, nullptr, &regs);
        return regs;
    }

    // 设置所有通用寄存器内容，struct user_regs_struct 结构见 /usr/include/sys/user.h 文件
    static auto set_registers(pid_t pid, user_regs_struct regs) -> void {
        ptrace(PTRACE_SETREGS, pid, nullptr, &regs);
    }

    // 获取 PC 寄存器内容
    static auto get_pc(pid_t pid) -> std::intptr_t {
        return get_registers(pid).rip;
    }

    // 设置 PC 寄存器内容
    static auto set_pc(pid_t pid, std::intptr_t pc) -> void {
        auto regs = get_registers(pid);
        regs.rip = pc;
        set_registers(pid, regs);
    }

    // 获取栈帧寄存器内容
    static auto get_frame_pointer(pid_t pid) -> uint64_t {
        return get_registers(pid).rbp;
    }

    // 获取使得 tracee 停止的信号信息
    static auto get_signal_info(pid_t pid) -> siginfo_t {
        siginfo_t info;
        ptrace(PTRACE_GETSIGINFO, pid, nullptr, &info);
        return info;
    }

    // 单步执行，一次只执行一步机器码，而不是编程语言级别的单步
    static auto single_step(pid_t pid) -> void {
        ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr);
    }
};

}
