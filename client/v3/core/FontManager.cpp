// Copyright 2022 CatchChallenger
#include "FontManager.hpp"

#include <QPainterPath>

#include "uthash.h"

typedef struct HashElementFont {
  QString id;
  int size;
  int height;
  QFont* font;
  UT_hash_handle hh;
} HashElementType;

FontManager* FontManager::instance_ = nullptr;

FontManager::FontManager() : fonts_(nullptr) {}

FontManager::~FontManager() {}

FontManager* FontManager::GetInstance() {
  if (instance_ == nullptr) instance_ = new FontManager();
  return instance_;
}

QFont* FontManager::GetFont(const QString& font_family, int size,
                            FontStyle style) {
  auto element = FetchFont(font_family, size, style);
  return element->font;
}

int FontManager::GetFontHeight(const QString& font_family, int size,
                               FontStyle style) {
  auto element = FetchFont(font_family, size, style);
  return element->height;
}

HashElementFont* FontManager::FetchFont(const QString& font_family, int size,
                                        FontStyle style) {
  HashElementFont* element = nullptr;
  QString id = QString("%1-%2-%3").arg(font_family).arg(size).arg(style);

  HASH_FIND_PTR(fonts_, &id, element);
  if (!element) {
    element = (HashElementType*)calloc(sizeof(*element), 1);
    element->size = size;
    QFont* font = new QFont();
    font->setFamily(font_family);
    if (style == kBold) {
      font->setBold(true);
    } else if (style == kItalic) {
      font->setItalic(true);
    }
    font->setPixelSize(size);
    element->font = font;

    int line_height = 5;
    QPainterPath temp_path;
    temp_path.addText(0, 0, *font,
                      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOQPRSTUVWXYZ");

    element->height = temp_path.boundingRect().height() + line_height;

    HASH_ADD_PTR(fonts_, id, element);
  }
  return element;
}
