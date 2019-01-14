#pragma once

/**
 * 打断点命令
 * 目前只支持传入指令地址进行断点
 **/

#include <command.hh>

namespace BitTech {

class Break : public Command {
public:
    Break(Inferior& inferior): Command(inferior) {}

public:
    auto name() const -> std::string override {
        return "break";
    }

    auto shortcut() const -> std::string override {
        return "b";
    }

    auto brief() const -> std::string override {
        return "打断点。";
    }

public:
    auto run(std::vector<std::string> const& args) const -> void {
        if (args.size() == 0) {
            printf("需要以 16 进制形式给出断点指令地址\n");
            return;
        }

        if (args[0][0] == '*') {
            // 假设是 *0x 开头，按指令地址断点
            std::intptr_t addr = std::stol(std::string{args[0], 3}, 0, 16);
            inferior.set_breakpoint_at_addr(addr);
        } else {
            // 按函数断点
            try {
                auto func_die = inferior.get_die_by_function_name(args[0]);
                auto addr = at_low_pc(func_die);
                inferior.set_breakpoint_at_addr(addr);
            } catch (no_debug_information const& exc) {
                printf("没有找到函数的调试信息\n");
            }
        }
    }
};

}
