#pragma once

/**
 * 所有命令的基类，必须实现以下接口:
 * name() 返回命令全程，用于匹配用户输入
 * shortcut() 返回命令缩写，用于匹配用户输入
 * brief() 返回命令帮助说明，帮助用户理解命令功能
 *
 * run(vector<string> args) 用户输入命令
 */

#include <vector>
#include <string>

namespace BitTech {

class Command {
public:
    Command(Inferior &inferior): inferior(inferior) {}

public:
    virtual auto name() const -> std::string = 0;
    virtual auto shortcut() const -> std::string = 0;
    virtual auto brief() const -> std::string = 0;

public:
    virtual auto run(std::vector<std::string> const& args) const -> void = 0;

public:
    Inferior &inferior;
};

}
