// Copyright 2021 CatchChallenger
#include "Node.hpp"

#include <QPainter>
#include <QVariant>
#include <algorithm>
#include <iostream>

#include "../action/Action.hpp"
#include "ActionManager.hpp"
#include "SceneManager.hpp"

Node::Node(Node *parent): QGraphicsItem(parent) {
  class_name_ = __func__;
  on_click_ = nullptr;
  on_resize_ = nullptr;
  bounding_rect_ = QRectF(0.0, 0.0, 0.0, 0.0);
  action_manager_ = ActionManager::GetInstance();
  in_test_ = false;
  node_cache_ = nullptr;
  node_type_ = __func__;

  if (parent) {
    parent->AddChild(this);
  }
  setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

Node::~Node() {
  UnRegisterEvents();
  if (node_cache_ != nullptr) {
    delete node_cache_;
  }
  node_cache_ = nullptr;
}

void Node::SetParent(Node *parent) { 
  QGraphicsItem::setParentItem(parent);
}

Node *Node::Parent() { 
  return static_cast<Node*>(QGraphicsItem::parentItem());
}

Node *Node::Root() {
  if (Parent() == nullptr) return nullptr;
  Node *parent = Parent();
  while (parent->Parent() != nullptr) {
    parent = parent->Parent();
  }
  return parent;
}

void Node::AddChild(Node *child) {
  //if (child->Parent() && child->Parent() != this) {
    //child->Parent()->RemoveChild(child);
  //}
  //if (scene() != nullptr) {
    //scene()->removeItem(child);
  //}
  child->SetParent(this);
  child->OnEnter();
}

void Node::RemoveChild(Node *child) {
  if (child == nullptr) return;
  child->OnExit();
  if (child->scene() != nullptr) {
    scene()->removeItem(child);
  }
  child->SetParent(nullptr);

  //node_to_delete_.push_back(child);

  //CleanNodeToDelete();
}

QRectF Node::MapRectToScene(const QRectF inner) const {
  //QRectF tmp = inner;
  //Node *parent = parent_;
  //while (parent != nullptr) {
    //tmp =
        //QRectF(tmp.x() + parent->BoundingRect().x(),
               //tmp.y() + parent->BoundingRect().y(), tmp.width(), tmp.height());
    //parent = parent->Parent();
  //}
  //return tmp;
  return mapRectToScene(inner);
}

void Node::RunAction(Action *action) {
  action_manager_->AddAction(action, this, false, false);
}

void Node::RunAction(Action *action, bool deletable) {
  action_manager_->AddAction(action, this, false, deletable);
}

Action *Node::GetAction(int tag) {
  return action_manager_->GetActionByTag(tag, this);
}

void Node::StartAllActions() {
  auto actions = action_manager_->GetAllActionsByTarget(this);
  auto length = actions->size();
  uint index = 0;
  while (index < length) {
    Action *action = actions->at(index);
    action->Start();
    index++;
  }
}

QRectF Node::BoundingRect() const { return bounding_rect_; }

void Node::SetPos(qreal x, qreal y) { setPos(x, y); }

void Node::SetPos(QPointF point) { SetPos(point.x(), point.y()); }

void Node::SetX(qreal x) { setX(x); }

void Node::SetY(qreal y) { setY(y); }

qreal Node::X() const { return x(); }

qreal Node::Y() const { return y(); }

void Node::SetSize(const QSizeF& size) {
  Node::SetSize(size.width(), size.height());
}

void Node::SetSize(qreal w, qreal h) {
  bounding_rect_.setWidth(w);
  bounding_rect_.setHeight(h);
  OnResize();

  if (Parent() != nullptr) {
    Parent()->OnChildResize(this);
  }

  if (on_resize_) {
    on_resize_(this);
  }
  ReDraw();
}

void Node::SetHeight(qreal h) { Node::SetSize(Width(), h); }

void Node::SetWidth(qreal w) { Node::SetSize(w, Height()); }

qreal Node::Height() const { return bounding_rect_.height(); }

qreal Node::Width() const { return bounding_rect_.width(); }

void Node::SetVisible(bool visible) {
  QGraphicsItem::setVisible(visible);
}

bool Node::IsVisible() { return QGraphicsItem::isVisible(); }

void Node::SetEnabled(bool enabled) { QGraphicsItem::setEnabled(enabled); }

bool Node::IsEnabled() const { return QGraphicsItem::isEnabled(); }

void Node::SetOnClick(const std::function<void(Node *)> &callback) {
  on_click_ = callback;
}

void Node::SetData(int role, int value) { data_[role] = value; }

int Node::Data(int role) { return data_[role]; }

void Node::SetData(int role, std::string value) { data_str_[role] = value; }

std::string Node::DataStr(int role) { return data_str_[role]; }

void Node::CleanNodeToDelete() {
  //if (node_to_delete_.empty()) return;

  //int iter = 0;
  //while (iter < node_to_delete_.size()) {
    //children_.erase(std::remove(children_.begin(), children_.end(),
                                //node_to_delete_.at(iter)),
                    //children_.end());
    //iter++;
  //}

  //node_to_delete_.clear();
}

void Node::ShouldCache(bool cache) { 
  //should_cache_ = cache;
}

void Node::TestMode() { in_test_ = true; }

void Node::SetToolTip(QString text) { setToolTip(text); }

QString Node::ToolTip() { return toolTip(); }

bool Node::HasRunningActions() {
  return action_manager_->GetNumberOfRunningActionsInTarget(this) > 0;
}

void Node::ReDraw() {
  if (node_cache_) {
    delete node_cache_;
  }
  node_cache_ = nullptr;
  update(boundingRect());
}

void Node::OnEnter() {
  auto children = Children();
  if (!children.empty()) {
    for (auto it = begin(children); it != end(children); it++) {
      (*it)->OnEnter();
    }
  }
}

void Node::OnExit() {
  auto children = Children();
  if (!children.empty()) {
    for (auto it = begin(children); it != end(children); it++) {
      (*it)->OnExit();
    }
  }
}

void Node::RegisterEvents() {}

void Node::UnRegisterEvents() {}

void Node::SetZValue(int index) {
  setZValue(index);
  //z_ = index;
  //if (Parent()) {
    //Parent()->ReArrangeChild(this);
  //}
}

int Node::ZValue() { return zValue(); }

void Node::ReArrangeChild(Node *node) {
  //children_.erase(std::remove(children_.begin(), children_.end(), node),
                  //children_.end());
  //size_t index = 0;
  //for (auto child : children_) {
    //if (node->ZValue() < child->ZValue()) break;
    //index++;
  //}
  //children_.insert(children_.begin() + index, node);
}

void Node::MousePressEvent(const QPointF &, bool &) {}

void Node::MouseReleaseEvent(const QPointF &, bool &) {}

void Node::MouseMoveEvent(const QPointF &) {}

void Node::KeyPressEvent(QKeyEvent *, bool &) {}

void Node::KeyReleaseEvent(QKeyEvent *, bool &) {}

std::vector<Node *> Node::Children() {
  auto buffer = childItems();
  std::vector<Node *> children;
  QList<QGraphicsItem*>::iterator i;
  for (i = buffer.begin(); i != buffer.end(); ++i) {
    children.push_back(static_cast<Node *>(*i));
  }

  return children;
}

void Node::SetScale(qreal value) {
  setScale(value);
  //scale_ = value;
  //const auto rect = BoundingRect();
  //SetSize(rect.width(), rect.height());
}

qreal Node::Scale() { return scale(); }

std::string Node::NodeType() const { return node_type_; }

void Node::Draw(QPainter *painter) {
  std::cout << "LAN_[" << __FILE__ << ":" << __LINE__ << "] "
            << class_name_ << "~nooo" << std::endl;
}

void Node::OnResize() {}

std::string Node::ClassName() const { return class_name_; }

void Node::OnChildResize(Node *child) {}

void Node::SetOnResize(const std::function<void(Node *)> &callback) {
  on_resize_ = callback;
}

void Node::RemoveAllChildrens(bool release) {
  std::vector<Node *> tmp = Children();
  if (!tmp.empty()) {
    for (auto it = begin(tmp); it != end(tmp); it++) {
      RemoveChild(static_cast<Node *>(*it));
      if (release) {
        delete (*it);
      }
    }
  }
}

QRectF Node::boundingRect() const {
  return bounding_rect_;
}

void Node::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  if (in_test_) {
    painter->fillRect(BoundingRect(), Qt::green);
  }
  if (cacheMode() == QGraphicsItem::NoCache) {
    Draw(painter);
  } else {
    if (!node_cache_) {
      if (bounding_rect_.isEmpty()) return;
      node_cache_ = new QPixmap(Width(), Height());
      node_cache_->fill(Qt::transparent);
      QPainter painter_c(node_cache_);
      //if (scale() != 1) {
        //painter_c.scale(scale(), scale());
      //}
      Draw(&painter_c);
      painter_c.end();
    }
    painter->drawPixmap(0, 0, *node_cache_);
  }
  //Draw(painter);
}

void Node::Render(QPainter *painter) {
  if (IsVisible()) {
    if (!node_cache_) {
      if (bounding_rect_.isEmpty()) return;
      node_cache_ = new QPixmap(Width(), Height());
      node_cache_->fill(Qt::transparent);
      QPainter painter_c(node_cache_);
      //if (scale_ != 1) {
        //painter_c.scale(scale_, scale_);
      //}
      Draw(&painter_c);
      painter_c.end();
    }
    painter->drawPixmap(X(), Y(), *node_cache_);

    //if (!should_cache_) ReDraw();

    auto children = Children();
    if (children.size() == 0) return;

    auto x = X();
    auto y = Y();

    painter->translate(x, y);
    for (auto it = begin(children); it != end(children); it++) {
      (*it)->Render(painter);
    }
    painter->translate(-x, -y);
  }

  CleanNodeToDelete();
}

qreal Node::Bottom() const {
  return Y() + Height();
}

qreal Node::Right() const {
  return X() + Width();
}
