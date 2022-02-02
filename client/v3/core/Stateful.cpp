// Copyright 2021 CatchChallenger
#include "Stateful.hpp"

#include <vector>

Stateful::~Stateful() {}

void Stateful::StoreState(int state) { states_[state] = StoreInternal(); }

void Stateful::UseState(int state) { UseInternal(states_[state]); }
