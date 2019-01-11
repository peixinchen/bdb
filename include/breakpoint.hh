#pragma once

/**
 * 断点，利用 int 3（0xCC）实现的软件断点
 */

#include <ptrace_proxy.hh>
#include <cstdint>
#include <unistd.h>

namespace BitTech {

class Breakpoint {
public:
    // 保留默认构造函数，用于创建容器时的初始化
    Breakpoint() = default;
    Breakpoint(pid_t pid, std::intptr_t addr): pid{pid}, addr{addr}, is_enabled{false}, saved{} {}

public:
    auto enabled() const -> bool {
        return is_enabled;
    }

public:
    auto enable() -> void {
        if (enabled()) {
            return;
        }

        // 从内存地址处获取指令
        uint64_t data = PtraceProxy::read_memory(pid, addr);
        // 只保留 8 Bit
        saved = static_cast<uint8_t>(data & 0xFF);
        // 将指令替换成 int 3（0xCC）
        uint64_t data_int3 = (data & ~0xFF) | 0xCC;
        // 重新将指令设置回内存地址处
        PtraceProxy::write_memory(pid, addr, data_int3);

        is_enabled = true;
    }

    auto disable() -> void {
        if (!enabled()) {
            return;
        }

        // 从内存地址处获取被修改为 0xCC 的指令
        auto data_int3 = PtraceProxy::read_memory(pid, addr);
        // 将指令修改为原状
        uint64_t data = (data_int3 & ~0xFF) | saved;
        // 重新将指令设置回内存地址处
        PtraceProxy::write_memory(pid, addr, data);

        is_enabled = false;
    }

private:
    // 保存 tracee 的 pid
    pid_t pid;
    // 设置断点的指令地址
    std::intptr_t addr;
    // 当前断点是否开启
    bool is_enabled;
    // 被 0xCC 替换之前的机器码
    uint8_t saved;
};

}
