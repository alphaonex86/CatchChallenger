// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_UTILS_HPP_
#define CLIENT_V3_ENTITIES_UTILS_HPP_

#include "../core/Scene.hpp"
#include "CommonTypes.hpp"

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
  static void MoveDatapackFolder(const QString& origin, const QString& dest);
  static void MoveFile(const QString& origin, const QString& folder,
                       const QString& name);
  static std::vector<QPixmap> GetSkillAnimation(const uint16_t& skillId);
  static ObjectCategory GetObjectCategory(const int& item_id);
};
#endif  // CLIENT_V3_ENTITIES_UTILS_HPP_
