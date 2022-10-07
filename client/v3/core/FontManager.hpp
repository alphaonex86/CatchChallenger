// Copyright 2022 CatchChallenger
#ifndef CLIENT_V3_CORE_FONTMANAGER_HPP_
#define CLIENT_V3_CORE_FONTMANAGER_HPP_

#include <QFont>
#include <cstdint>
#include <vector>

class HashElementFont;

class FontManager {
 public:
  ~FontManager();

  enum FontStyle { kNormal = 0, kBold = 1, kItalic = 2 };

  static FontManager* GetInstance();
  QFont* GetFont(const QString& font_family, int size,
                 FontStyle style = FontStyle::kNormal);
  int GetFontHeight(const QString& font_family, int size,
                    FontStyle style = FontStyle::kNormal);

 private:
  FontManager();

  HashElementFont* FetchFont(const QString& font_family, int size,
                             FontStyle style);

 protected:
  static FontManager* instance_;
  struct HashElementFont* fonts_;
};
#endif  // CLIENT_V3_CORE_FONTMANAGER_HPP_
