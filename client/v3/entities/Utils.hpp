// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_UTILS_HPP_
#define CLIENT_V3_ENTITIES_UTILS_HPP_

#include "../core/Scene.hpp"

struct NodeParsed {
  std::vector<Node*> nodes;
};

class Utils {
 public:
  static Node* GetCurrentScene(Node* node);
  static bool IsActiveInScene(Node* node);
  static NodeParsed HTML2Node(const QString& html);
  static NodeParsed HTML2Node(const QString& html,
                              std::function<void(Node*)> on_click);
  static QString RemoveHTMLEntities(const QString& html);
  static QPixmap B64Png2Pixmap(const QString& b64);
  static QImage CropToContent(const QImage& buffer, QColor stop);
  static void MoveDatapackFolder(const QString& origin, const QString& dest);
  static void MoveFile(const QString& origin, const QString& folder,
                       const QString& name);
  static std::vector<QPixmap> GetSkillAnimation(const uint16_t &skillId);
};
#endif  // CLIENT_V3_ENTITIES_UTILS_HPP_