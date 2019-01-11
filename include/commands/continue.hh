#pragma once

/**
 * 继续 tracee 的执行
 **/

#include <command.hh>

namespace BitTech {

class Continue : public Command {
public:
    Continue(Inferior& inferior): Command(inferior) {}

public:
    auto name() const -> std::string override {
        return "continue";
    }

    auto shortcut() const -> std::string override {
        return "c";
    }

    auto brief() const -> std::string override {
        return "继续运行 program。";
    }

public:
    auto run(std::vector<std::string> const& args) const -> void {
        if (!inferior.running()) {
            printf("还未运行，先启动运行。\n");
            return;
        }

        inferior.continue_execute();
    }
};

}
