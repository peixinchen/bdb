#pragma once

/**
 * 管理和被调试进程的一切事务，包括读取 ELF 和 DWARF 信息
 */

#include <string>

namespace BitTech {

class Inferior {
public:
    Inferior(std::string const& program) {}
};

}
