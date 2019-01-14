#pragma once

/**
 * 单步执行（代码级别），会进到函数内部
 **/

#include <command.hh>
#include <set>

namespace BitTech {

class Step : public Command {
public:
    Step(Inferior& inferior): Command(inferior) {}

public:
    auto name() const -> std::string override {
        return "step";
    }

    auto shortcut() const -> std::string override {
        return "s";
    }

    auto brief() const -> std::string override {
        return "单步执行（进入代码内部）。";
    }

public:
    auto run(std::vector<std::string> const& args) const -> void {
        if (!inferior.running()) {
            printf("还未运行，先启动运行。\n");
            return;
        }

        try {
            run_step();
        } catch (no_debug_information const& exc) {
            printf("没有调试信息，跳过。\n");
            continue_to_return_address();
        }
    }

private:
    // 继续执行到本函数结束
    auto continue_to_return_address() const -> void {
        std::set<std::intptr_t> to_remove{};
        auto pid = inferior.pid;
        // 根据 ABI 的规范，返回地址在当前 栈帧 + 8 位置处
        auto frame_pointer = PtraceProxy::get_frame_pointer(pid);
        auto return_address = PtraceProxy::read_memory(pid, frame_pointer + 8);
        if (inferior.breakpoints.count(return_address) == 0) {
            inferior.set_breakpoint_at_addr(return_address);
            to_remove.insert(return_address);
        }

        // 继续执行被调试程序，直到触发断点
        inferior.continue_execute();

        // 删除执行 step 命令阶段添加的所有断点
        for (auto addr : to_remove) {
            if (inferior.breakpoints[addr].enabled()) {
                inferior.breakpoints[addr].disable();
            }
            inferior.breakpoints.erase(addr);
        }
    }

    /**
     * 异常的情况留给外面考虑
     * 通过把函数的每一行都加上断点的方式实现 step
     */
    auto run_step() const -> void {
        auto func_die = inferior.get_function_die_by_pc();
        auto begin_addr = at_low_pc(func_die);
        auto end_addr = at_high_pc(func_die);

        // 函数起始位置
        auto line_iter = inferior.get_line_iter_by_addr(begin_addr);
        // 当前执行位置
        auto current_line_iter = inferior.get_line_iter_by_pc();

        // 把每一行都加上断点
        std::set<std::intptr_t> to_remove{};
        for (; line_iter->address < end_addr; ++line_iter) {
            if (line_iter->address != current_line_iter->address
                && inferior.breakpoints.count(line_iter->address) == 0) {
                inferior.set_breakpoint_at_addr(line_iter->address);
                to_remove.insert(line_iter->address);
            }
        }

        // 如果当前函数不是 main 函数，则需要找到本函数返回后的第一个指令处，也设置断点
        if (at_name(func_die) != "main") {
            auto pid = inferior.pid;
            // 根据 ABI 的规范，返回地址在当前 栈帧 + 8 位置处
            auto frame_pointer = PtraceProxy::get_frame_pointer(pid);
            auto return_address = PtraceProxy::read_memory(pid, frame_pointer + 8);
            if (inferior.breakpoints.count(return_address) == 0) {
                inferior.set_breakpoint_at_addr(return_address);
                to_remove.insert(return_address);
            }
        }

        try {
            // 当前代码行
            auto line = inferior.get_line_iter_by_pc()->line;
            // 一直执行下一条指令，直到我们不在同一代码行
            while (inferior.get_line_iter_by_pc()->line == line) {
                inferior.single_step_instruction_with_breakpoint_check();
            }

            // 每次 step 停下后，显示上下文代码
            try {
                auto line_iter = inferior.get_line_iter_by_pc();
                inferior.list_source(line_iter->file->path, line_iter->line, 1);
            } catch (exception const& exc) {
                // 没有调试信息，不显示上下文代码
            }
        } catch (no_debug_information const& exc) {
            // 中途遇到没有调试信息的情况，直接放弃，跳到下个可能的断点处
            inferior.continue_execute();
        }

        // 已经彻底从一个函数返回了
        // 删除执行 step 命令阶段添加的所有断点
        for (auto addr : to_remove) {
            if (inferior.breakpoints[addr].enabled()) {
                inferior.breakpoints[addr].disable();
            }
            inferior.breakpoints.erase(addr);
        }
    }
};

}
