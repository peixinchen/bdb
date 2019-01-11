#pragma once

/**
 * 管理和被调试进程的一切事务，包括读取 ELF 和 DWARF 信息
 */

#include <exception.hh>
#include <ptrace_proxy.hh>
#include <string>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

namespace BitTech {

class Inferior {
public:
    Inferior(std::string const& program): signo{0}, pid{-1}, is_running{false}, program{program} {}

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

private:
    // 将 inferior 的状态重置
    auto reset() -> void {
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
            // 现阶段我们没有什么事情要做
            break;
        default:
            signo = siginfo.si_signo;
            printf("收到信号 %s\n", strsignal(siginfo.si_signo));
        }
    }

    auto continue_execute() -> void {
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

        // 现在这个阶段，我们还没有什么事情要做，直接继续 tracee
        continue_execute();
    }

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
