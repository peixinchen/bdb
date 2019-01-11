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

        std::intptr_t addr = std::stol(args[0], 0, 16);
        inferior.set_breakpoint_at_addr(addr);
    }
};

}
