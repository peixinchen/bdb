#pragma once

/**
 * 开始 tracee 的命令
 * 用户传入的参数会作为启动命令的参数
 **/

#include <command.hh>

namespace BitTech {

class Run : public Command {
public:
    Run(Inferior& inferior): Command(inferior) {}

public:
    auto name() const -> std::string override {
        return "run";
    }

    auto shortcut() const -> std::string override {
        return "r";
    }

    auto brief() const -> std::string override {
        return "开始运行 program。";
    }

public:
    auto run(std::vector<std::string> const& args) const -> void {
        if (inferior.running()) {
            printf("已经在运行，重启\n");
            inferior.stop();
        }

        inferior.start(args);
    }
};

}
