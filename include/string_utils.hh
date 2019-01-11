#pragma once

/**
 * string 的一些通用功能函数
 */

#include <vector>
#include <string>

namespace BitTech {

// 以 delimiter 将 string 分割为多个 string，并返回 vector<string>
auto split(std::string const& s, std::string const& delimiter) -> std::vector<std::string> {
    std::string::size_type pos;
    std::string::size_type last = 0;

    std::vector<std::string> r{};
    do {
        pos = s.find(delimiter, last);
        auto arg = s.substr(last, pos - last);
        if (arg.size() > 0) {
            r.push_back(arg);
        }
        last = pos + 1;
    } while (pos != std::string::npos);

    return r;
}

// 不传 delimiter，默认以空格分割
auto split(std::string const& s) -> std::vector<std::string> {
    return split(s, " ");
}

}
