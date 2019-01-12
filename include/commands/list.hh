#pragma once

/**
 * 打印代码
 **/

#include <command.hh>

namespace BitTech {

class List : public Command {
public:
    List(Inferior& inferior): Command(inferior) {}

public:
    auto name() const -> std::string override {
        return "list";
    }

    auto shortcut() const -> std::string override {
        return "l";
    }

    auto brief() const -> std::string override {
        return "打印代码。";
    }

public:
    auto run(std::vector<std::string> const& args) const -> void {
        if (args.size() == 0) {
            printf("指定函数名\n");
            return;
        }

        try {
            auto line_iter = inferior.get_line_iter_by_function_name(args[0]);
            printf("%d\n", line_iter->line);
            inferior.list_source(line_iter->file->path, line_iter->line, 4);
        } catch (no_debug_information const& exc) {
            printf("没有找到函数的调试信息\n");
        }
    }
};

}
