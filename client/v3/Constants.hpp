// Copyright 2022 CatchChallenger
#ifndef CLIENT_V3_CONSTANTS_HPP_
#define CLIENT_V3_CONSTANTS_HPP_

#include <QSizeF>

class Constants {
   public:
    static QSizeF DialogLargeSize();
    static QSizeF DialogMediumSize();
    static QSizeF DialogSmallSize();

    static QSizeF ButtonLargeSize();
    static QSizeF ButtonMediumSize();
    static QSizeF ButtonSmallSize();

    static QSizeF ButtonRoundLargeSize();
    static QSizeF ButtonRoundMediumSize();
    static QSizeF ButtonRoundSmallSize();

    static int ButtonLargeHeight();
    static int ButtonMediumHeight();
    static int ButtonSmallHeight();

    static int ItemLargeHeight();
    static int ItemMediumHeight();
    static int ItemSmallHeight();

    static int TextLargeSize();
    static int TextTitleSize();
    static int TextMediumSize();
    static int TextSmallSize();

    static int ItemLargeSpacing();
    static int ItemMediumSpacing();
    static int ItemSmallSpacing();
};

#endif  // CLIENT_V3_CONSTANTS_HPP_
