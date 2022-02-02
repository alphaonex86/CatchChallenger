// Copyright 2021 CatchChallenger
#include "Column.hpp"

#include <iostream>

#include "../core/Node.hpp"

using UI::Column;

Column::Column(Node *parent) : Node(parent) {
  node_type_ = __func__;
  is_processing_resize_ = false;
  spacing_ = 0;
}

Column::~Column() {
  auto children = Children();
  for (auto item : children) {
    delete item;  
  }
}

Column *Column::Create(Node *parent) {
  Column *instance = new (std::nothrow) Column(parent);
  return instance;
}

void Column::ReCalculateSize() {
  auto children = Children();
  if (children.size() == 0) return;
  int y = 0;
  int height = 0;
  int width = 0;
  for (auto item : children) {
    item->SetX(0);
    item->SetY(y);
    height = item->Height() + y;
    if (item->Width() > width) {
      width = item->Width();
    }
    y = y + item->Height() + spacing_;
  }

  SetSize(width, height);
}

void Column::SetItemSpacing(int value) {
  spacing_ = value;
  ReCalculateSize();
}

void Column::OnChildResize(Node *node) {
  if (is_processing_resize_) return;
  is_processing_resize_ = true;
  ReCalculateSize();
  is_processing_resize_ = false;
}

void Column::AddChild(Node *node) {
  Node::AddChild(node);
  ReCalculateSize();
}

void Column::RemoveChild(Node *node) {
  Node::RemoveChild(node);
  ReCalculateSize();
}

void Column::Draw(QPainter *painter) {}

void Column::RegisterEvents() {
  auto children = Children();
  for (auto item : children) {
    item->RegisterEvents();
  }
}

void Column::UnRegisterEvents() {
  auto children = Children();
  for (auto item : children) {
    item->UnRegisterEvents();
  }
}
