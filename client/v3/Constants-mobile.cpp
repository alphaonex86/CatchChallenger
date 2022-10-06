// Copyright 2022 CatchChallenger
#include "Constants.hpp"

#define BLOCK_SIZE 150 //30
#define FONT_SIZE 48 //12

// width = height * 2.5 + 20;
QSizeF Constants::ButtonLargeSize() { return QSizeF(BLOCK_SIZE * 2.5 + 20, BLOCK_SIZE); }

// width = height * 2.5 + 20;
QSizeF Constants::ButtonMediumSize() { return QSizeF(BLOCK_SIZE * 0.8 * 2.5 + 20, BLOCK_SIZE * 0.8); }

// width = height * 2.5 + 20;
QSizeF Constants::ButtonSmallSize() { return QSizeF(BLOCK_SIZE * 0.7 + 20, BLOCK_SIZE * 0.7); }

// width = height * 0.8;
QSizeF Constants::ButtonRoundLargeSize() { return QSizeF(BLOCK_SIZE * 0.8, BLOCK_SIZE); }

// width = height * 0.8;
QSizeF Constants::ButtonRoundMediumSize() { return QSizeF(BLOCK_SIZE * 0.8 * 0.8, BLOCK_SIZE * 0.8); }

// width = height * 0.8;
QSizeF Constants::ButtonRoundSmallSize() { return QSizeF(BLOCK_SIZE * 0.8 * 0.7, BLOCK_SIZE * 0.8 * 0.7); }

int Constants::ButtonLargeHeight() { return BLOCK_SIZE; }

int Constants::ButtonMediumHeight() { return BLOCK_SIZE * 0.8; }

int Constants::ButtonSmallHeight() { return BLOCK_SIZE * 0.7; }

int Constants::TextLargeSize() { return FONT_SIZE; }

int Constants::TextTitleSize() { return FONT_SIZE * 0.8; }

int Constants::TextMediumSize() { return FONT_SIZE * 0.6; }

int Constants::TextSmallSize() { return FONT_SIZE * 0.5; }

int Constants::ItemLargeSpacing() { return 15; }

int Constants::ItemMediumSpacing() { return 10; }

int Constants::ItemSmallSpacing() { return 5; }

int Constants::ItemLargeHeight() { return BLOCK_SIZE; }

int Constants::ItemMediumHeight() { return BLOCK_SIZE * 0.8; }

int Constants::ItemSmallHeight() { return BLOCK_SIZE * 0.7; }
