// Copyright 2021 CatchChallenger
#include "ActionUtils.hpp"

#include "../action/Delay.hpp"
#include "../action/CallFunc.hpp"

Sequence* ActionUtils::WaitAndThen(const int& ms, std::function<void()> callback) {
    auto sequence = Sequence::Create(Delay::Create(ms),
                                     CallFunc::Create(callback), nullptr);
    return sequence;
}
