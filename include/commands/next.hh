#pragma once

/**
 * 单步执行（代码级别），不会进到函数内部
 **/

#include <commands/abs_single_step.hh>

namespace BitTech {

class Next : public AbsSingleStep {
public:
    Next(Inferior& inferior): AbsSingleStep(inferior) {}

public:
    auto name() const -> std::string override {
        return "next";
    }

    auto shortcut() const -> std::string override {
        return "n";
    }

    auto brief() const -> std::string override {
        return "单步执行（不进入代码内部）。";
    }

protected:
    // step 和 next 的不同在这个函数里处理
    virtual auto single_step_handle() const -> void override {
        inferior.continue_execute();
    }
};

}
