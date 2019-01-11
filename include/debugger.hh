#pragma once

/**
 * 调试器
 */


#include <exception.hh>
#include <string_utils.hh>
#include <inferior.hh>
#include <command.hh>
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <cstdio>


namespace BitTech {

class Debugger {
public:
    Debugger(std::string const& program): prev_args{}, commands{}, inferior{program} {
    }

public:
    // 主要的 命令接收 -> 命令执行 流程
    auto run() -> void {
        copyright();
        help();

        std::string line;
        while (1) {
            prompt();
            auto &in = std::getline(std::cin, line);
            // EOF 退出
            if (!in) {
                break;
            }

            auto args = split(line);

            // 处理用户输入为空的情况
            if (args.size() == 0) {
                if (prev_args.size() == 0) {
                    continue;
                }

                args = prev_args;
            } else {
                prev_args = args;
            }

            // 退出
            if (args[0] == "quit") {
                break;
            }

            // 查找合适的命令并执行
            try {
                auto const& command = find_first_matched_command(args[0]);
                command->run(args);
            } catch (no_such_command const& exc) {
                printf("不支持的命令\n");
                help();
            }
        }

        // 执行退出后的相应措施
        quit();
    }

private:
    auto copyright() const -> void {
        printf("一个演示版本的 mini 调试器\n");
    }

    // 依次按指定格式给出所有命令及帮助
    auto help() const -> void {
        printf("支持以下命令:\n");
        for (auto const& command: commands) {
            printf("  %s(%s) -- %s\n", 
                command->name().c_str(), 
                command->shortcut().c_str(), 
                command->brief().c_str());
        }
    }

    auto prompt() const -> void {
        printf("(bdb) ");
    }

    auto quit() -> void {
        printf("quit\n");
    }

private:
    // 利用顺序查找的方式，找到匹配的命令
    auto find_first_matched_command(std::string const& name_or_shortcut) -> std::shared_ptr<Command> const& {
        for (auto const& command: commands) {
            if (name_or_shortcut == command->shortcut() || name_or_shortcut == command->name()) {
                return command;
            }
        }

        NO_SUCH_COMMAND(name_or_shortcut);
    }

private:
    // 保存上次用于输入的命令, 当用户输入为空时，尝试执行上次命令
    std::vector<std::string> prev_args;

private:
    // 目前支持的所有命令
    std::vector<std::shared_ptr<Command>> commands;

private:
    // 管理 tracee 执行过程的一切事务
    Inferior inferior;
};

}
