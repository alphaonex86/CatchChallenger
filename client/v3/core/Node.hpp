// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_CORE_NODE_HPP_
#define CLIENT_V3_CORE_NODE_HPP_

#include <QGraphicsItem>
#include <QVariant>
#include <unordered_map>
#include <vector>

class Action;
class ActionManager;

class Node : public QGraphicsItem {
 public:
  virtual ~Node();

  void SetPos(qreal x, qreal y);
  void SetX(qreal x);
  void SetY(qreal y);
  qreal X() const;
  qreal Y() const;

  QRectF boundingRect() const override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget = 0) override;

  virtual void SetSize(qreal width, qreal height);
  virtual void SetWidth(qreal w);
  virtual void SetHeight(qreal h);
  qreal Width() const;
  qreal Height() const;

  qreal Bottom() const;
  qreal Right() const;

  void SetVisible(bool visible);
  bool IsVisible();
  void SetEnabled(bool enabled);
  bool IsEnabled() const;
  int Data(int role);
  std::string DataStr(int role);
  void SetData(int role, int value);
  void SetData(int role, std::string value);
  void SetParent(Node *parent);
  Node *Parent();
  /** Get the root node */
  Node *Root();
  std::string ClassName() const;
  QRectF MapRectToScene(QRectF inner) const;
  /** Start an action */
  void RunAction(Action *action);
  void StartAllActions();
  Action *GetAction(int tag);
  bool HasRunningActions();
  void SetOnClick(const std::function<void(Node *)> &callback);
  void SetOnResize(const std::function<void(Node *)> &callback);
  void TestMode();
  void SetToolTip(QString text);
  QString ToolTip();
  std::string NodeType() const;
  void SetZValue(int index);
  void ReArrangeChild(Node *node);
  int ZValue();
  void AddChild(Node *node);
  /** Remove a child from the node */
  void RemoveChild(Node *node);
  /** Remove all childrens from the node */
  void RemoveAllChildrens(bool release = true);
  std::vector<Node *> Children();
  /** Invalidate cache */
  void ReDraw();
  void ShouldCache(bool cache);
  void SetScale(qreal value);
  qreal Scale();
  virtual void OnChildResize(Node *child);
  virtual QRectF BoundingRect() const;

  virtual void Draw(QPainter *painter);
  void Render(QPainter *painter);
  virtual void MousePressEvent(const QPointF &point, bool &press_validated);
  virtual void MouseReleaseEvent(const QPointF &point, bool &prev_validated);
  virtual void MouseMoveEvent(const QPointF &point);
  virtual void KeyPressEvent(QKeyEvent *event, bool &event_trigger);
  virtual void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger);
  virtual void OnEnter();
  virtual void OnExit();
  virtual void RegisterEvents();
  /** Unregister all events on node, always remove the events before remove
   * parent **/
  virtual void UnRegisterEvents();

 protected:
  QRectF bounding_rect_;
  ActionManager *action_manager_;
  std::function<void(Node *)> on_click_;
  std::function<void(Node *)> on_resize_;
  std::string class_name_;
  std::string node_type_;

  explicit Node(Node *parent = nullptr);
  virtual void OnResize();

 private:
  QPixmap *node_cache_;
  std::unordered_map<int, int> data_;
  std::unordered_map<int, std::string> data_str_;
  uint unique_id_;
  bool in_test_;
  std::vector<Node *> node_to_delete_;

  void CleanNodeToDelete();
};
#endif  // CLIENT_V3_CORE_NODE_HPP_
