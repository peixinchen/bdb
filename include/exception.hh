#pragma once

/**
 * 项目中用到的异常
 */

#include <exception>
#include <string>

namespace BitTech {

/**
 * 我们所有异常的基类
 */
class exception : public std::exception {
public:
    exception(std::string const& file, int line, std::string const& reason)
        : std::exception{}, file{file}, line{line}, reason{reason}
    {}

    std::string file;
    int line;
    std::string reason;
};


/**
 * 不支持的 debugger 命令
 */
class no_such_command : public exception {
public:
    no_such_command(std::string const& file, int line, std::string const& reason)
        : exception{file, line, reason} {}
};


/**
 * 没有调试信息
 */
class no_debug_information : public exception {
public:
    no_debug_information(std::string const& file, int line, std::string const& reason)
        : exception{file, line, reason} {}
};


/**
 * 不要直接 throw 异常，使用下面的类抛异常
 */

#define EXCEPTION(reason) throw exception{__FILE__, __LINE__, (reason)}

#define NO_SUCH_COMMAND(reason) throw no_such_command{__FILE__, __LINE__, (reason)}

#define NO_DEBUG_INFORMATION(reason) throw no_debug_information{__FILE__, __LINE__, (reason)}

};
