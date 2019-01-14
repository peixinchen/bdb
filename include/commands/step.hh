#pragma once

/**
 * 单步执行（代码级别），会进到函数内部
 **/

#include <commands/abs_single_step.hh>

namespace BitTech {

class Step : public AbsSingleStep {
public:
    Step(Inferior& inferior): AbsSingleStep(inferior) {}

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

protected:
    // step 和 next 的不同在这个函数里处理
    virtual auto single_step_handle() const -> void override {
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
    }
};

}
