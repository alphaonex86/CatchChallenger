// Copyright 2021 CatchChallenger
#include "Action.hpp"

#include "../core/Node.hpp"

Action::Action() {
  tag_ = rand() % 10000;
}

Action::~Action() {}

void Action::SetTarget(Node* node) { node_ = node; }

Node* Action::Target() { return node_; }

void Action::SetTag(int tag) { tag_ = tag; }

int Action::Tag() { return tag_; }

void Action::OnFinish() {
  Stop();
}
