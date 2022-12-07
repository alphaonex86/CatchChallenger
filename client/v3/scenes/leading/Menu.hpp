// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_SCENES_LEADING_MENU_HPP_
#define CLIENT_QTOPENGL_SCENES_LEADING_MENU_HPP_

#include <vector>

#include "../../core/Scene.hpp"
#include "../../core/Sprite.hpp"
#include "../../entities/FeedNews.hpp"
#include "../../ui/Button.hpp"
#include "../../ui/Label.hpp"
#include "../../ui/ListView.hpp"
#include "Debug.hpp"
#include "Multi.hpp"
#include "../shared/options/Options.hpp"
#ifndef NOSINGLEPLAYER
#include "Solo.hpp"
#endif

#ifndef CATCHCHALLENGER_NOAUDIO
  #include <QAudioOutput>

  #include "../../../libqtcatchchallenger/QInfiniteBuffer.hpp"
#endif

namespace Scenes {
class Menu : public Scene {
 public:
  Menu();
  static Menu *Create();
  ~Menu();
  void setError(const std::string &error);
  void OnEnter() override;
  void OnExit() override;

 private:
  UI::Button *solo_;
  UI::Button *multi_;
  UI::Button *options_;
  UI::Button *facebook_;
  UI::Button *website_;
  UI::Button *debug_;
  Sprite *news_;
  UI::ListView *news_list_;
  Sprite *news_wait_;
  UI::Button *news_update_;
  UI::Label *warning_;
  bool warning_can_be_reset_;
  QString warning_string_;
  std::vector<FeedNews::FeedEntry> entry_list_;
  uint8_t current_news_type_;
  bool have_fresh_feed_;
  bool have_update_;
  bool is_loaded_;

  Multi *multi_scene_;
#ifndef NOSINGLEPLAYER
  Solo *solo_scene_;
#endif
  Options *option_scene_;
  Debug *debug_scene_;

  void ChangeLanguage();
  void OnAssetsDataParsed();
  void Initialize();

 protected:
  void UpdateNews();

  void OnScreenResize() override;

#ifndef __EMSCRIPTEN__
  void NewUpdate(const std::string &version);
#endif
  void FeedEntryList(const std::vector<FeedNews::FeedEntry> &entryList,
                     std::string error = std::string());
  void OpenWebsite();
  void OpenFacebook();
  void OpenUpdate();
  void GoToSolo();
  void GoToMulti();
  void GoToOptions();
  void GoToDebug();
  void OnClickLink(Node *node);
};
}  // namespace Scenes
#endif  // CLIENT_QTOPENGL_SCENES_LEADING_MENU_HPP_
