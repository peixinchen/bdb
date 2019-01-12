#pragma once

/**
 * 管理和被调试进程的一切事务，包括读取 ELF 和 DWARF 信息
 */

#include <exception.hh>
#include <ptrace_proxy.hh>
#include <breakpoint.hh>
#include <elf/elf++.hh>
#include <dwarf/dwarf++.hh>
#include <string>
#include <set>
#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>


namespace BitTech {

class Inferior {
public:
    Inferior(std::string const& program)
        : signo{0}, pid{-1}, is_running{false}, program{program}, 
          breakpoint_addrs_to_set{}, breakpoints{} {

        int fd = open(program.c_str(), O_RDONLY);

        elf::elf elf{elf::create_mmap_loader(fd)};
        try {
            dwarf = dwarf::dwarf{dwarf::elf::create_loader(elf)};
        } catch (dwarf::format_error const& exc) {
            printf("** 没有找到程序的调试信息 **\n");
            dwarf = dwarf::dwarf{};
        }

        close(fd);
    }

public:
    auto running() const -> bool {
        return is_running;
    }

public:
    // 开始运行 tracee 程序
    auto start(std::vector<std::string> const& args) -> void {
        pid = fork();
        if (pid == -1) {
            EXCEPTION("fork 失败");
        } else if (pid == 0) {
            // 标志自己被 trace
            PtraceProxy::trace_me();

            // 执行 program
            tracee_routine(args);

            // 内部会执行 execv 函数，失败后会抛出异常，所以永远不应该到这里
        }

        // 标记 tracee 已经开始运行了，但执行完 tracer_routine 后，is_running 可能重新变为 false
        // 因为在启动过程中进程因为一些原因直接结束了
        is_running = true;
        tracer_routine(args);
    }

    // 发送 SIGKILL 杀死被调试进程
    // 并且 waitpid 回收僵尸进程
    auto stop() -> void {
        if (pid == -1) {
            EXCEPTION("inferior 没有运行");
        }
        // 如 man ptrace 所说，SIGKILL 信号是会直接发送给 tracee 的
        kill(pid, SIGKILL);
        // 等待子进程的结束
        handle_wait_signal_and_exit();
    }

public:
    // 在 addr 地址处设置 或者 准备设置断点
    auto set_breakpoint_at_addr(std::intptr_t addr) -> void {
        if (!running()) {
            // tracee 还未开始运行，只记录地址，不真正添加断点
            breakpoint_addrs_to_set.insert(addr);
            return;
        }

        if (breakpoints.count(addr) == 0) {
            Breakpoint bp{pid, addr};
            bp.enable();
            breakpoints[addr] = bp;
        }
    }

public:
    // 继续执行 tracee
    auto continue_execute() -> void {
        // 因为当前指令可能仍然是 0xCC
        // 所以我们先确认下，如果是，就先暂停断点
        // 使用单步指令跳到下一条指令后再继续
        step_over_breakpoint();

        if (signo != 0) {
            // 因为收到非 SIGTRAP 信号而停止，将信号重新发送给 tracee
            PtraceProxy::delivery_signal_tracee(pid, signo);
            signo = 0;  // 重置信号记录
        } else {
            // 不发送信号继续
            PtraceProxy::continue_tracee(pid);
        }

        handle_wait_signal_and_exit();
    }

public:
    // 打印 filename 第 line 行左右的代码，上下文分别 n_context
    auto list_source(std::string const& filename, unsigned int line, unsigned int n_context) const -> void {
        std::ifstream source_file{filename};
        unsigned int start = n_context > line ? 1 : line - n_context;
        unsigned int end = line + n_context;
        unsigned int current = 1u;

        std::string source_line{};
        while (current < start && std::getline(source_file, source_line)) {
            ++current;
        }

        while (current <= end && std::getline(source_file, source_line)) {
            printf("%s%3d|%s\n", current == line ? "->" : "  ", current, source_line.c_str());
            ++current;
        }
    }

public:
    // 根据机器码地址返回行调试信息
    auto get_line_iter_by_addr(std::intptr_t addr) const -> dwarf::line_table::iterator {
        for (auto const& cu : dwarf.compilation_units()) {
            if (die_pc_range(cu.root()).contains(addr)) {
                auto &line_table = cu.get_line_table();
                auto it = line_table.find_address(addr);
                if (it != line_table.end()) {
                    return it;
                }
                break;
            }
        }

        NO_DEBUG_INFORMATION("没有找到函数的调试信息");
    }

    // 根据函数名称返回函数起始的行调试信息
    auto get_line_iter_by_function_name(std::string const& name) const -> dwarf::line_table::iterator {
        auto low_pc = get_addr_by_function_name(name);
        return get_line_iter_by_addr(low_pc);
    }

    // 根据函数名称返回函数起始地址
    auto get_addr_by_function_name(std::string const& name) const -> std::intptr_t {
        // 遍历调试信息的每个编译单元
        for (auto const& cu : dwarf.compilation_units()) {
            // 遍历编译单元的每个 DWARF Information Entries
            for (auto const& die : cu.root()) {
                // 如果 tag 表明是函数（subprogram）、有 name 并且 name 就是要查找的函数名
                if (die.tag == dwarf::DW_TAG::subprogram 
                    && die.has(dwarf::DW_AT::name) 
                    && at_name(die) == name) {
                    // 找到了函数对应的 DIE
                    // 取得函数的开始指令地址
                    return at_low_pc(die);
                }
            }
        }

        NO_DEBUG_INFORMATION("没有找到函数的调试信息");
    }

private:
    // 将 inferior 的状态重置
    auto reset() -> void {
        // 将已设置的断点全部清空
        breakpoints.clear();
        pid = -1;
        is_running = false;
    }

    // 处理 tracee 停止后信号的工作或者 tracee 直接退出的工作
    auto handle_wait_signal_and_exit() -> void {
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {
            printf("[(进程 %d) 正常结束]\n", pid);
            reset();
            return;
        } else if (WIFSIGNALED(status)) {
            printf("[进程 %d] 因为信号被杀.\n", pid);
            reset();
            return;
        }

        auto siginfo = PtraceProxy::get_signal_info(pid);
        switch (siginfo.si_signo) {
        case SIGTRAP:
            // 触发断点而停止
            handle_sigtrap(siginfo);
            break;
        default:
            signo = siginfo.si_signo;
            printf("收到信号 %s\n", strsignal(siginfo.si_signo));
        }
    }

    auto handle_sigtrap(siginfo_t siginfo) -> void {
        if (siginfo.si_code != SI_KERNEL && siginfo.si_code != TRAP_BRKPT) {
            // 不是因为断点触发的，直接返回
            return;
        }

        // 确实是因为断点停下来的
        // 需要将 PC 回退到执行 0xCC 之前的位置
        // 然后重新执行原状态的指令
        // 这里只处理 PC 的回退
        // 执行原状态的操作在 step_over_breakpoint 中
        auto pc = PtraceProxy::get_pc(pid);
        PtraceProxy::set_pc(pid, pc - 1);

        try {
            auto line_iter = get_line_iter_by_addr(pc - 1);
            list_source(line_iter->file->path, line_iter->line, 1);
        } catch (no_debug_information const& exc) {
        }
    }

    // 执行机器码级别单步运行的
    // 然后等 tracee 停下来
    auto single_step_instruction() -> void {
        PtraceProxy::single_step(pid);
        handle_wait_signal_and_exit();
    }

    // 判断当前要执行的指令是否是 0xCC 断点指令
    // 如果是，则暂时关闭掉该断点
    // 等执行过后再打开
    auto step_over_breakpoint() -> void {
        auto pc = PtraceProxy::get_pc(pid);
        if (breakpoints.count(pc)) {
            auto &bp = breakpoints[pc];
            if (bp.enabled()) {
                bp.disable();
                // 利用指令单步操作运行过该指令
                single_step_instruction();
                bp.enable();
            }
        }
    }

private:
    // 将传入的 args 重新组织成 execv 需要的格式
    auto tracee_routine(std::vector<std::string> const& args) -> void {
        // 多出的两个空间，一个留给开头的 program，一个留给最后的 nullptr
        char **argv = new char *[args.size() + 2];  

        argv[0] = const_cast<char *>(program.c_str());
        for (auto i = 0; i < args.size(); ++i) {
            argv[1 + i] = const_cast<char *>(args[i].c_str());
        }
        argv[args.size() + 1] = nullptr;

        execv(program.c_str(), argv);
        // 子进程抛出异常
        EXCEPTION(std::string{"execv 失败: "} + strerror(errno));
    }

    auto tracer_routine(std::vector<std::string> const& args) -> void {
        // 根据文档，execv 执行成功后，tracee 会收到 SIGTRAP 信号，我们等这个信号
        int status = 0;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            // execv 失败了，子进程抛出了异常，父进程也抛出异常
            EXCEPTION("启动失败，退出");
        }

        // 将之前记录的断点地址真正设置为断点
        for (auto addr : breakpoint_addrs_to_set) {
            if (breakpoints.count(addr) == 0) {
                Breakpoint bp{pid, addr};
                bp.enable();
                breakpoints[addr] = bp;
            }
        }

        // 继续执行
        continue_execute();
    }

private:
    // 提取 program 中的 debug 信息
    dwarf::dwarf dwarf;

private:
    // tracee 未开始运行时，记录断点地址，在 tracee 开始运行时将断点加入
    std::set<std::intptr_t> breakpoint_addrs_to_set;
    // 真正设置到的断点
    std::unordered_map<std::intptr_t, Breakpoint> breakpoints;

private:
    // 表示 tracee 目前是否在运行
    bool is_running;
    // 表示 tracee 因为什么信号而停止，0 表示默认状态
    int  signo;

private:
    // 记录要运行的程序
    std::string program;
    // 记录 tracee 运行的 pid
    pid_t pid;
};

}
